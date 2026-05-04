#include "Stage/TestRenderStageBase.h"

#include "CString.h"
#include "DataCacheInterface.h"
#include "FileSystem.h"
#include "PropertySet.h"
#include "ProfileSystem.h"
#include "Async/AsyncWork.h"
#include "PlatformThread.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/ShaderManager.h"
#include "Widget/WidgetUtility.h"

#define TIFF_DISABLE_DEPRECATED
#include "../OtherLib/tiff/include/tiffio.h"

#include <cmath>
#include <cfloat>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "../OtherLib/tiff/lib/tiff.lib")

namespace Render
{
	class TerrainProgram : public GlobalShaderProgram
	{
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(TerrainProgram, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Terrain";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
	};

	IMPLEMENT_SHADER_PROGRAM(TerrainProgram);

	struct WinHttpHandle
	{
		~WinHttpHandle()
		{
			reset();
		}

		void reset(HINTERNET inHandle = nullptr)
		{
			if (handle)
			{
				WinHttpCloseHandle(handle);
			}
			handle = inHandle;
		}

		operator HINTERNET() const { return handle; }
		HINTERNET handle = nullptr;
	};

	struct TerrainChunkCacheData
	{
		uint32 width = 0;
		uint32 height = 0;
		float minHeight = 0;
		float maxHeight = 0;
		TArray< uint16 > heightData;
		TArray< uint16 > haloHeightData;

		template< class OP >
		void serialize(OP& op)
		{
			op & width & height & minHeight & maxHeight & heightData & haloHeightData;
		}

		bool isValid() const
		{
			return width > 0 && height > 0 &&
				heightData.size() == width * height &&
				haloHeightData.size() == (width + 2) * (height + 2);
		}
	};

	struct TerrainChunkCoord
	{
		int latIndex = 0;
		int lonIndex = 0;
		float latitude = 0;
		float longitude = 0;
		float south = 0;
		float north = 0;
		float west = 0;
		float east = 0;
	};

	struct TerrainChunkResult
	{
		uint32 generation = 0;
		bool bSuccess = false;
		TerrainChunkCoord coord;
		TerrainChunkCacheData data;
		TArray< uint8 > textureData;
	};

	struct PendingTerrainChunkRaw
	{
		uint32 generation = 0;
		uint64 order = 0;
		TerrainChunkCoord coord;
		TerrainChunkCacheData data;
	};

	struct TerrainRenderChunk
	{
		TerrainChunkCoord coord;
		RHITexture2DRef texture;
		uint32 width = 0;
		uint32 height = 0;
		float minHeight = 0;
		float maxHeight = 0;
		float heightWorldScale = 0;
		TArray< uint16 > heightData;
		TArray< uint16 > haloHeightData;
		TArray< uint8 > textureData;
		Vector3 scale = Vector3(1, 1, 1);
		Matrix4 localToWorld = Matrix4::Identity();
		Vector2 heightRange = Vector2(0, 1);
		uint8 currentLOD = 0;
		uint8 renderLOD = 0;
		Vector4 lodBlend = Vector4(0, 0, 0, 0);
	};

	struct ITerrainDataProvider
	{
		virtual ~ITerrainDataProvider() = default;
		virtual char const* getName() const = 0;
		virtual bool canDownload() const = 0;
		virtual void logMissingCredential() const = 0;
		virtual bool buildChunkCacheData(TerrainChunkCoord const& coord, TerrainChunkCacheData& outData) = 0;
	};

	static constexpr char const* kOpenTopoHost = "portal.opentopography.org";
	static constexpr char const* kOpenTopoConfigSection = "OpenTopography";
	static constexpr char const* kOpenTopoDemType = "COP30";
	static constexpr char const* kCopernicusProcessHost = "sh.dataspace.copernicus.eu";
	static constexpr char const* kCopernicusIdentityHost = "identity.dataspace.copernicus.eu";
	static constexpr char const* kCopernicusProcessPath = "/process/v1";
	static constexpr char const* kCopernicusTokenPath = "/auth/realms/CDSE/protocol/openid-connect/token";
	static constexpr uint32 kTerrainChunkTileCount = 256;
	static constexpr uint32 kTerrainChunkSampleCount = kTerrainChunkTileCount + 1;
	static constexpr uint32 kTerrainChunkHaloSampleCount = kTerrainChunkSampleCount + 2;
	static constexpr float kGeoSampleDegree = 1.0f / 3600.0f;
	static constexpr float kDefaultChunkDegreeSize = kGeoSampleDegree * kTerrainChunkTileCount;
	static constexpr int kTerrainLODCount = 4;
	static constexpr uint32 kTerrainLODMeshTileCounts[kTerrainLODCount] = { 256, 128, 64, 32 };
	enum EChunkBorderMask
	{
		ChunkBorderMask_None = 0,
		ChunkBorderMask_Left = 1 << 0,
		ChunkBorderMask_Right = 1 << 1,
		ChunkBorderMask_Bottom = 1 << 2,
		ChunkBorderMask_Top = 1 << 3,
	};

	struct TerrainChunkNeighborBorders
	{
		uint8 mask = ChunkBorderMask_None;
		TArray< float > leftHeights;
		TArray< float > rightHeights;
		TArray< float > bottomHeights;
		TArray< float > topHeights;
	};

	class TerrainProviderService
	{
	public:
		bool convertHeightDataToChunkCache(TArray< float > const& heightValues, uint32_t width, uint32_t height, double noDataValue, TerrainChunkCoord const& coord, float sourceSouth, float sourceNorth, float sourceWest, float sourceEast, TerrainChunkCacheData& outData)
		{
			bool bHasValidValue = false;
			float minHeight = FLT_MAX;
			float maxHeight = -FLT_MAX;
			for (float value : heightValues)
			{
				if (!std::isfinite(value))
					continue;
				if (noDataValue != DBL_MAX && value == float(noDataValue))
					continue;
				minHeight = Math::Min(minHeight, value);
				maxHeight = Math::Max(maxHeight, value);
				bHasValidValue = true;
			}
			if (!bHasValidValue)
				return false;

			outData.width = kTerrainChunkSampleCount;
			outData.height = kTerrainChunkSampleCount;
			outData.minHeight = minHeight;
			outData.maxHeight = maxHeight;
			outData.heightData.resize(outData.width * outData.height);
			outData.haloHeightData.resize((outData.width + 2) * (outData.height + 2));

			float heightRange = Math::Max(maxHeight - minHeight, 1e-6f);
			float sourceWidthDegree = sourceEast - sourceWest;
			float sourceHeightDegree = sourceNorth - sourceSouth;
			float sampleStepDegree = (coord.east - coord.west) / kTerrainChunkTileCount;
			auto sampleHeightAt = [&](float targetLongitude, float targetLatitude)
			{
				float sourceV = (sourceHeightDegree > 0) ? ((targetLatitude - sourceSouth) / sourceHeightDegree) : 0.0f;
				float srcY = Math::Clamp(sourceV * float(height - 1), 0.0f, float(height - 1));
				uint32_t y0 = uint32_t(std::floor(srcY));
				uint32_t y1 = Math::Min(y0 + 1, height - 1);
				float fy = srcY - y0;

				float sourceU = (sourceWidthDegree > 0) ? ((targetLongitude - sourceWest) / sourceWidthDegree) : 0.0f;
				float srcX = Math::Clamp(sourceU * float(width - 1), 0.0f, float(width - 1));
				uint32_t x0 = uint32_t(std::floor(srcX));
				uint32_t x1 = Math::Min(x0 + 1, width - 1);
				float fx = srcX - x0;

				auto sampleValue = [&](uint32_t sx, uint32_t sy)
				{
					float value = heightValues[sx + sy * width];
					if (!std::isfinite(value) || (noDataValue != DBL_MAX && value == float(noDataValue)))
						return minHeight;
					return value;
				};

				float v00 = sampleValue(x0, y0);
				float v10 = sampleValue(x1, y0);
				float v01 = sampleValue(x0, y1);
				float v11 = sampleValue(x1, y1);
				return Math::Lerp(Math::Lerp(v00, v10, fx), Math::Lerp(v01, v11, fx), fy);
			};
			for (uint32_t y = 0; y < outData.height; ++y)
			{
				float targetLatitude = coord.south + float(outData.height - 1 - y) * sampleStepDegree;

				for (uint32_t x = 0; x < outData.width; ++x)
				{
					float targetLongitude = coord.west + float(x) * sampleStepDegree;
					float value = sampleHeightAt(targetLongitude, targetLatitude);
					float normalizedValue = Math::Clamp((value - minHeight) / heightRange, 0.0f, 1.0f);
					outData.heightData[x + y * outData.width] = uint16(normalizedValue * 65535.0f + 0.5f);
				}
			}
			for (int y = -1; y <= int(outData.height); ++y)
			{
				float targetLatitude = coord.south + float(int(outData.height) - 1 - y) * sampleStepDegree;
				for (int x = -1; x <= int(outData.width); ++x)
				{
					float targetLongitude = coord.west + float(x) * sampleStepDegree;
					float value = sampleHeightAt(targetLongitude, targetLatitude);
					float normalizedValue = Math::Clamp((value - minHeight) / heightRange, 0.0f, 1.0f);
					outData.haloHeightData[(x + 1) + (y + 1) * (outData.width + 2)] = uint16(normalizedValue * 65535.0f + 0.5f);
				}
			}
			return true;
		}

		static TerrainChunkCoord calcExpandedSampleCoord(TerrainChunkCoord const& coord)
		{
			TerrainChunkCoord result = coord;
			float stepDegree = (coord.east - coord.west) / kTerrainChunkTileCount;
			result.south -= stepDegree;
			result.north += stepDegree;
			result.west -= stepDegree;
			result.east += stepDegree;
			return result;
		}

		bool readGeoTiffFloatData(void const* fileData, size_t fileSize, TArray< float >& outHeightValues, uint32_t& outWidth, uint32_t& outHeight, double& outNoDataValue)
		{
			MemoryTiffStream stream;
			stream.data = reinterpret_cast<uint8 const*>(fileData);
			stream.size = toff_t(fileSize);
			stream.offset = 0;

			TIFF* tif = TIFFClientOpen("MemoryGeoTiff", "rm", &stream,
				readMemoryTiff,
				writeMemoryTiff,
				seekMemoryTiff,
				closeMemoryTiff,
				sizeMemoryTiff,
				mapMemoryTiff,
				unmapMemoryTiff);
			if (tif == nullptr)
			{
				LogWarning(0, "TIFFClientOpen failed for in-memory GeoTIFF");
				return false;
			}

			ON_SCOPE_EXIT
			{
				TIFFClose(tif);
			};

			uint16_t bitsPerSample = 0;
			uint16_t samplesPerPixel = 0;
			uint16_t sampleFormat = SAMPLEFORMAT_UINT;
			uint16_t planarConfig = PLANARCONFIG_CONTIG;
			TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &outWidth);
			TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &outHeight);
			TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
			TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
			TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);
			TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &planarConfig);

			if (outWidth == 0 || outHeight == 0 || bitsPerSample != 32 || samplesPerPixel != 1 || sampleFormat != SAMPLEFORMAT_IEEEFP || planarConfig != PLANARCONFIG_CONTIG)
			{
				LogWarning(0, "Unsupported in-memory GeoTIFF format");
				return false;
			}

			outNoDataValue = tryGetGeoTiffNoData(tif);
			return TIFFIsTiled(tif) ? loadTiledFloatGeoTiff(tif, outWidth, outHeight, outHeightValues) : loadScanlineFloatGeoTiff(tif, outWidth, outHeight, outHeightValues);
		}

		bool getCopernicusAccessToken(char const* clientId, char const* clientSecret, std::string& outToken)
		{
			uint64 nowTick = ::GetTickCount64();
			{
				Mutex::Locker lock(mCopernicusTokenMutex);
				if (!mCopernicusAccessToken.empty() && nowTick + 60000 < mCopernicusTokenExpireTick)
				{
					outToken = mCopernicusAccessToken;
					return true;
				}
			}

			InlineString<1024> body;
			body += "grant_type=client_credentials&client_id=";
			body += urlEncode(clientId);
			body += "&client_secret=";
			body += urlEncode(clientSecret);

			TArray< std::wstring > headers;
			headers.push_back(L"Content-Type: application/x-www-form-urlencoded");

			TArray< uint8 > responseBuffer;
			DWORD statusCode = 0;
			std::string responseText;
			if (!sendHttpRequestWinHttp(kCopernicusIdentityHost, INTERNET_DEFAULT_HTTPS_PORT, L"POST", kCopernicusTokenPath, headers, body.c_str(), uint32(body.length()), responseBuffer, statusCode, responseText))
				return false;

			if (statusCode != HTTP_STATUS_OK)
			{
				LogWarning(0, "Copernicus token request failed with HTTP %u : %s", statusCode, responseText.c_str());
				return false;
			}

			std::string token;
			if (!extractJsonStringField(responseText, "access_token", token))
			{
				LogWarning(0, "Copernicus token response missing access_token");
				return false;
			}

			int expiresIn = 3600;
			extractJsonIntField(responseText, "expires_in", expiresIn);

			{
				Mutex::Locker lock(mCopernicusTokenMutex);
				mCopernicusAccessToken = token;
				mCopernicusTokenExpireTick = nowTick + uint64(Math::Max(60, expiresIn - 60)) * 1000;
			}
			outToken = token;
			return true;
		}

		bool sendHttpRequestWinHttp(char const* host, INTERNET_PORT port, wchar_t const* method, char const* objectPath, TArray< std::wstring > const& headers, char const* body, uint32 bodySize, TArray< uint8 >& outBuffer, DWORD& outStatusCode, std::string& outResponseText)
		{
			std::wstring hostW = FCString::CharToWChar(host);
			std::wstring objectPathW = FCString::CharToWChar(objectPath);

			WinHttpHandle session;
			session.reset(WinHttpOpen(L"GameProject Terrain Downloader/1.0",
				WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
				WINHTTP_NO_PROXY_NAME,
				WINHTTP_NO_PROXY_BYPASS,
				0));
			if (!session)
			{
				LogWarning(0, "WinHttpOpen failed : %u", GetLastError());
				return false;
			}

			WinHttpHandle connection;
			connection.reset(WinHttpConnect(session, hostW.c_str(), port, 0));
			if (!connection)
			{
				LogWarning(0, "WinHttpConnect failed : %u", GetLastError());
				return false;
			}

			DWORD flags = (port == INTERNET_DEFAULT_HTTPS_PORT) ? WINHTTP_FLAG_SECURE : 0;
			WinHttpHandle request;
			request.reset(WinHttpOpenRequest(connection, method, objectPathW.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
			if (!request)
			{
				LogWarning(0, "WinHttpOpenRequest failed : %u", GetLastError());
				return false;
			}

			for (std::wstring const& header : headers)
			{
				if (!WinHttpAddRequestHeaders(request, header.c_str(), DWORD(-1L), WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE))
				{
					LogWarning(0, "WinHttpAddRequestHeaders failed : %u", GetLastError());
					return false;
				}
			}

			void* requestBody = (bodySize > 0) ? (void*)body : WINHTTP_NO_REQUEST_DATA;
			if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, requestBody, bodySize, bodySize, 0))
			{
				LogWarning(0, "WinHttpSendRequest failed : %u", GetLastError());
				return false;
			}

			if (!WinHttpReceiveResponse(request, nullptr))
			{
				LogWarning(0, "WinHttpReceiveResponse failed : %u", GetLastError());
				return false;
			}

			DWORD statusSize = sizeof(outStatusCode);
			if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &outStatusCode, &statusSize, WINHTTP_NO_HEADER_INDEX))
			{
				LogWarning(0, "WinHttpQueryHeaders failed : %u", GetLastError());
				return false;
			}

			outBuffer.clear();
			for (;;)
			{
				DWORD bytesAvailable = 0;
				if (!WinHttpQueryDataAvailable(request, &bytesAvailable))
				{
					LogWarning(0, "WinHttpQueryDataAvailable failed : %u", GetLastError());
					return false;
				}
				if (bytesAvailable == 0)
					break;

				int startIndex = outBuffer.size();
				outBuffer.resize(startIndex + bytesAvailable);

				DWORD bytesRead = 0;
				if (!WinHttpReadData(request, outBuffer.data() + startIndex, bytesAvailable, &bytesRead))
				{
					LogWarning(0, "WinHttpReadData failed : %u", GetLastError());
					return false;
				}
				outBuffer.resize(startIndex + bytesRead);
			}

			outResponseText.assign((char const*)outBuffer.data(), (char const*)outBuffer.data() + outBuffer.size());
			return true;
		}

	private:
		struct MemoryTiffStream
		{
			uint8 const* data = nullptr;
			toff_t size = 0;
			toff_t offset = 0;
		};

		static tmsize_t readMemoryTiff(thandle_t handle, void* buffer, tmsize_t size)
		{
			MemoryTiffStream& stream = *reinterpret_cast< MemoryTiffStream* >(handle);
			toff_t remain = (stream.offset < stream.size) ? (stream.size - stream.offset) : 0;
			tmsize_t readSize = tmsize_t(Math::Min<uint64>(uint64(size), uint64(remain)));
			if (readSize > 0)
			{
				std::memcpy(buffer, stream.data + stream.offset, readSize);
				stream.offset += readSize;
			}
			return readSize;
		}

		static tmsize_t writeMemoryTiff(thandle_t, void*, tmsize_t)
		{
			return 0;
		}

		static toff_t seekMemoryTiff(thandle_t handle, toff_t offset, int origin)
		{
			MemoryTiffStream& stream = *reinterpret_cast< MemoryTiffStream* >(handle);
			toff_t base = 0;
			switch (origin)
			{
			case SEEK_SET: base = 0; break;
			case SEEK_CUR: base = stream.offset; break;
			case SEEK_END: base = stream.size; break;
			default: return toff_t(-1);
			}

			toff_t newOffset = base + offset;
			if (newOffset > stream.size)
				return toff_t(-1);

			stream.offset = newOffset;
			return stream.offset;
		}

		static int closeMemoryTiff(thandle_t)
		{
			return 0;
		}

		static toff_t sizeMemoryTiff(thandle_t handle)
		{
			MemoryTiffStream& stream = *reinterpret_cast< MemoryTiffStream* >(handle);
			return stream.size;
		}

		static int mapMemoryTiff(thandle_t handle, void** base, toff_t* size)
		{
			MemoryTiffStream& stream = *reinterpret_cast< MemoryTiffStream* >(handle);
			*base = const_cast<uint8*>(stream.data);
			*size = stream.size;
			return 1;
		}

		static void unmapMemoryTiff(thandle_t, void*, toff_t)
		{
		}

		static bool loadTiledFloatGeoTiff(TIFF* tif, uint32_t width, uint32_t height, TArray< float >& outHeightValues)
		{
			uint32_t tileWidth = 0;
			uint32_t tileHeight = 0;
			TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tileWidth);
			TIFFGetField(tif, TIFFTAG_TILELENGTH, &tileHeight);
			if (tileWidth == 0 || tileHeight == 0)
				return false;

			tmsize_t tileByteSize = TIFFTileSize(tif);
			if (tileByteSize <= 0)
				return false;

			outHeightValues.resize(width * height);
			TArray< uint8 > tileBuffer;
			tileBuffer.resize((int)tileByteSize);

			for (uint32_t tileY = 0; tileY < height; tileY += tileHeight)
			{
				for (uint32_t tileX = 0; tileX < width; tileX += tileWidth)
				{
					ttile_t tileIndex = TIFFComputeTile(tif, tileX, tileY, 0, 0);
					tmsize_t readSize = TIFFReadEncodedTile(tif, tileIndex, tileBuffer.data(), tileByteSize);
					if (readSize <= 0)
						return false;

					float* srcData = reinterpret_cast<float*>(tileBuffer.data());
					uint32_t copyHeight = Math::Min(tileHeight, height - tileY);
					uint32_t copyWidth = Math::Min(tileWidth, width - tileX);
					for (uint32_t localY = 0; localY < copyHeight; ++localY)
					{
						float* srcRow = srcData + localY * tileWidth;
						float* dstRow = outHeightValues.data() + (tileY + localY) * width + tileX;
						FMemory::Copy(dstRow, srcRow, sizeof(float) * copyWidth);
					}
				}
			}
			return true;
		}

		static bool loadScanlineFloatGeoTiff(TIFF* tif, uint32_t width, uint32_t height, TArray< float >& outHeightValues)
		{
			tmsize_t scanlineByteSize = TIFFScanlineSize(tif);
			if (scanlineByteSize <= 0)
				return false;

			outHeightValues.resize(width * height);
			TArray< uint8 > scanlineBuffer;
			scanlineBuffer.resize((int)scanlineByteSize);
			for (uint32_t y = 0; y < height; ++y)
			{
				if (TIFFReadScanline(tif, scanlineBuffer.data(), y, 0) < 0)
					return false;

				std::memcpy(outHeightValues.data() + y * width, scanlineBuffer.data(), sizeof(float) * width);
			}
			return true;
		}

		static double tryGetGeoTiffNoData(TIFF* tif)
		{
			char* noDataText = nullptr;
			if (TIFFGetField(tif, 42113, &noDataText) && noDataText && noDataText[0] != 0)
				return std::atof(noDataText);
			return DBL_MAX;
		}

		static bool extractJsonStringField(std::string const& text, char const* fieldName, std::string& outValue)
		{
			std::string fieldTag = "\"";
			fieldTag += fieldName;
			fieldTag += "\"";
			size_t pos = text.find(fieldTag);
			if (pos == std::string::npos)
				return false;
			pos = text.find(':', pos);
			if (pos == std::string::npos)
				return false;
			pos = text.find('"', pos);
			if (pos == std::string::npos)
				return false;
			size_t end = text.find('"', pos + 1);
			if (end == std::string::npos)
				return false;
			outValue = text.substr(pos + 1, end - pos - 1);
			return true;
		}

		static bool extractJsonIntField(std::string const& text, char const* fieldName, int& outValue)
		{
			std::string fieldTag = "\"";
			fieldTag += fieldName;
			fieldTag += "\"";
			size_t pos = text.find(fieldTag);
			if (pos == std::string::npos)
				return false;
			pos = text.find(':', pos);
			if (pos == std::string::npos)
				return false;
			size_t start = text.find_first_of("-0123456789", pos + 1);
			if (start == std::string::npos)
				return false;
			size_t end = text.find_first_not_of("0123456789", start + 1);
			outValue = std::atoi(text.substr(start, end - start).c_str());
			return true;
		}

		static std::string urlEncode(char const* text)
		{
			char const* hex = "0123456789ABCDEF";
			std::string result;
			for (; *text; ++text)
			{
				unsigned char ch = (unsigned char)*text;
				if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' || ch == '~')
				{
					result.push_back(char(ch));
				}
				else
				{
					result.push_back('%');
					result.push_back(hex[(ch >> 4) & 0xF]);
					result.push_back(hex[ch & 0xF]);
				}
			}
			return result;
		}

		Mutex mCopernicusTokenMutex;
		std::string mCopernicusAccessToken;
		uint64 mCopernicusTokenExpireTick = 0;
	};

	class OpenTopographyProvider : public ITerrainDataProvider
	{
	public:
		OpenTopographyProvider(TerrainProviderService& inService, std::string inApiKey)
			: service(inService)
			, apiKey(std::move(inApiKey))
		{
		}

		char const* getName() const override
		{
			return "OpenTopography";
		}

		bool canDownload() const override
		{
			return !apiKey.empty();
		}

		void logMissingCredential() const override
		{
			LogWarning(0, "OpenTopography API key is empty. Set [%s] ApiKey in Game.ini first.", kOpenTopoConfigSection);
		}

		bool buildChunkCacheData(TerrainChunkCoord const& coord, TerrainChunkCacheData& outData) override
		{
			TerrainChunkCoord requestCoord = TerrainProviderService::calcExpandedSampleCoord(coord);
			char query[1024];
			std::snprintf(query, sizeof(query),
				"/API/globaldem?demtype=%s&south=%.8f&north=%.8f&west=%.8f&east=%.8f&outputFormat=GTiff&API_Key=%s",
				kOpenTopoDemType, requestCoord.south, requestCoord.north, requestCoord.west, requestCoord.east, apiKey.c_str());

			TArray< uint8 > responseBuffer;
			DWORD statusCode = 0;
			std::string responseText;
			if (!service.sendHttpRequestWinHttp(kOpenTopoHost, INTERNET_DEFAULT_HTTPS_PORT, L"GET", query, {}, nullptr, 0, responseBuffer, statusCode, responseText))
				return false;
			if (statusCode != HTTP_STATUS_OK)
			{
				LogWarning(0, "OpenTopography request failed with HTTP %u : %s", statusCode, responseText.c_str());
				return false;
			}

			TArray< float > heightValues;
			uint32_t width = 0;
			uint32_t height = 0;
			double noDataValue = DBL_MAX;
			if (!service.readGeoTiffFloatData(responseBuffer.data(), responseBuffer.size(), heightValues, width, height, noDataValue))
				return false;

			return service.convertHeightDataToChunkCache(heightValues, width, height, noDataValue, coord, requestCoord.south, requestCoord.north, requestCoord.west, requestCoord.east, outData);
		}

	private:
		TerrainProviderService& service;
		std::string apiKey;
	};

	class CopernicusProvider : public ITerrainDataProvider
	{
	public:
		CopernicusProvider(TerrainProviderService& inService, std::string inClientId, std::string inClientSecret, std::string inDemInstance)
			: service(inService)
			, clientId(std::move(inClientId))
			, clientSecret(std::move(inClientSecret))
			, demInstance(std::move(inDemInstance))
		{
		}

		char const* getName() const override
		{
			return "Copernicus";
		}

		bool canDownload() const override
		{
			return !clientId.empty() && !clientSecret.empty();
		}

		void logMissingCredential() const override
		{
			LogWarning(0, "Copernicus credentials are incomplete. Set [%s] CopernicusClientId / CopernicusClientSecret in Game.ini first.", kOpenTopoConfigSection);
		}

		bool buildChunkCacheData(TerrainChunkCoord const& coord, TerrainChunkCacheData& outData) override
		{
			TerrainChunkCoord requestCoord = TerrainProviderService::calcExpandedSampleCoord(coord);
			std::string accessToken;
			if (!service.getCopernicusAccessToken(clientId.c_str(), clientSecret.c_str(), accessToken))
				return false;

			InlineString<4096> requestBody;
			InlineString<256> bboxText;
			InlineString<256> outputText;
			bboxText.format("%.8f,%.8f,%.8f,%.8f", requestCoord.west, requestCoord.south, requestCoord.east, requestCoord.north);
			outputText.format("\"output\":{\"width\":%u,\"height\":%u,\"responses\":[{\"identifier\":\"default\",\"format\":{\"type\":\"image/tiff\"}}]},", kTerrainChunkHaloSampleCount, kTerrainChunkHaloSampleCount);
			requestBody += "{";
			requestBody += "\"input\":{\"bounds\":{\"properties\":{\"crs\":\"http://www.opengis.net/def/crs/OGC/1.3/CRS84\"},\"bbox\":[";
			requestBody += bboxText;
			requestBody += "]},\"data\":[{\"type\":\"dem\",\"dataFilter\":{\"demInstance\":\"";
			requestBody += demInstance.c_str();
			requestBody += "\"},\"processing\":{\"upsampling\":\"BILINEAR\",\"downsampling\":\"BILINEAR\"}}]},";
			requestBody += outputText;
			requestBody += "\"evalscript\":\"//VERSION=3\\nfunction setup(){return { input:['DEM'], output:{ id:'default', bands:1, sampleType: SampleType.FLOAT32 } };}\\nfunction evaluatePixel(sample){return [sample.DEM];}\"";
			requestBody += "}";

			TArray< std::wstring > headers;
			std::wstring authHeader = L"Authorization: Bearer ";
			authHeader += FCString::CharToWChar(accessToken.c_str());
			headers.push_back(authHeader);
			headers.push_back(L"Content-Type: application/json");

			TArray< uint8 > responseBuffer;
			DWORD statusCode = 0;
			std::string responseText;
			if (!service.sendHttpRequestWinHttp(kCopernicusProcessHost, INTERNET_DEFAULT_HTTPS_PORT, L"POST", kCopernicusProcessPath, headers, requestBody.c_str(), uint32(requestBody.length()), responseBuffer, statusCode, responseText))
				return false;

			if (statusCode != HTTP_STATUS_OK)
			{
				LogWarning(0, "Copernicus request failed with HTTP %u : %s", statusCode, responseText.c_str());
				return false;
			}

			TArray< float > heightValues;
			uint32_t width = 0;
			uint32_t height = 0;
			double noDataValue = DBL_MAX;
			if (!service.readGeoTiffFloatData(responseBuffer.data(), responseBuffer.size(), heightValues, width, height, noDataValue))
				return false;

			return service.convertHeightDataToChunkCache(heightValues, width, height, noDataValue, coord, requestCoord.south, requestCoord.north, requestCoord.west, requestCoord.east, outData);
		}

	private:
		TerrainProviderService& service;
		std::string clientId;
		std::string clientSecret;
		std::string demInstance;
	};

	class TerrainRenderingStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		enum
		{
			UI_DOWNLOAD_DEM = BaseClass::NEXT_UI_ID,
			UI_OPEN_DEM,
			UI_WIREFRAME,
			UI_USE_LOD,
		};

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			mChunkWorkPool.init(4);
			mChunkFinalizeWorkPool.init(1);
			mViewFrustum.mNear = 1.0;
			mViewFrustum.mFar = 80000.0;
			::Global::GUI().cleanupWidget();
			auto frame = WidgetUtility::CreateDevFrame();
			frame->addButton(UI_DOWNLOAD_DEM, "Request Chunk");
			frame->addButton(UI_OPEN_DEM, "Reload Chunk");
			FWidgetProperty::Bind(frame->addCheckBox(UI_WIREFRAME, "Wireframe"), mbWireframe);
			FWidgetProperty::Bind(frame->addCheckBox(UI_USE_LOD, "Use LOD"), mbUseLOD);
			restart();
			return true;
		}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::None;
		}

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1280;
			systemConfigs.screenHeight = 800;
		}

		bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
			VERIFY_RETURN_FALSE(mDebugPrimitives.initializeRHI());

			for (int i = 0; i < kTerrainLODCount; ++i)
			{
				InlineString<64> meshName;
				meshName.format("TerrainChunkLOD%d_%u", i, kTerrainLODMeshTileCounts[i]);
				VERIFY_RETURN_FALSE(BuildMesh(mTerrainMeshes[i], meshName, static_cast<bool(*)(Mesh&, int, float, bool)>(FMeshBuild::Tile), kTerrainLODMeshTileCounts[i], 1.0f, true));
			}

			VERIFY_RETURN_FALSE(mTexTerrainBaseColor = RHIUtility::LoadTexture2DFromFile("Terrain/colormap.bmp", TextureLoadOption().SRGB()));

			VERIFY_RETURN_FALSE(mProgTerrain = ShaderManager::Get().getGlobalShaderT<TerrainProgram>(true));

			requestConfiguredTerrainChunks();

			return true;
		}

		void preShutdownRenderSystem(bool bReInit) override
		{
			mDebugPrimitives.releaseRHI();
			mProgTerrain = nullptr;
			mTexTerrainBaseColor.release();
			for (Mesh& mesh : mTerrainMeshes)
			{
				mesh.releaseRHIResource();
			}
			mRenderChunks.clear();

			BaseClass::preShutdownRenderSystem(bReInit);
		}

		void restart() override
		{
			float worldChunkSize = getConfiguredChunkWorldSize(mCenterChunkCoord);
			mCamera.lookAt(Vector3(0.5f * worldChunkSize, 0.15f * worldChunkSize, 0.8f * worldChunkSize),
			              Vector3(0.5f * worldChunkSize, 0.5f * worldChunkSize, 0),
			              Vector3(0, 0, 1));
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
			applyPendingChunkResult();
			ensureTerrainChunksForCamera();
		}

		void onRender(float dFrame) override
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			applyPendingChunkResult();

			initializeRenderState();
			RHISetRasterizerState(commandList, mbWireframe ? TStaticRasterizerState<ECullMode::Back, EFillMode::Wireframe>::GetRHI() : TStaticRasterizerState<ECullMode::Back>::GetRHI());


			uint32 numDrawChunks = 0;
			uint32 numDrawTriangles = 0;
			uint32 numLODChunks[kTerrainLODCount] = {};

			{
				GPU_PROFILE("Terrain Render");
				RHISetShaderProgram(commandList, mProgTerrain->getRHI());
				mView.setupShader(commandList, *mProgTerrain);
				ViewInfo cullView = mView;
				cullView.setupTransform(mCamera.getPos(), mCamera.getRotation(), mViewFrustum.getPerspectiveMatrix());
				TArray< TerrainRenderChunk* > visibleChunks;
				visibleChunks.reserve(mRenderChunks.size());
				TArray< TerrainRenderChunk* > texturedChunks;
				texturedChunks.reserve(mRenderChunks.size());
				for (auto& pair : mRenderChunks)
				{
					TerrainRenderChunk& chunk = pair.second;
					if (!chunk.texture)
						continue;
					texturedChunks.push_back(&chunk);
					if (!isChunkVisible(chunk, cullView))
						continue;
					visibleChunks.push_back(&chunk);
				}
				if (visibleChunks.empty())
				{
					visibleChunks = texturedChunks;
				}
				for (TerrainRenderChunk* pChunk : visibleChunks)
				{
					TerrainRenderChunk& chunk = *pChunk;
					chunk.currentLOD = uint8(mbUseLOD ? updateChunkLOD(chunk) : 0);
				}
				for (TerrainRenderChunk* pChunk : visibleChunks)
				{
					TerrainRenderChunk& chunk = *pChunk;
					chunk.renderLOD = uint8(mbUseLOD ? resolveRenderLOD(chunk) : 0);
					chunk.lodBlend = mbUseLOD ? calcChunkLODBlend(chunk) : Vector4(0, 0, 0, 0);
				}

				for (TerrainRenderChunk* pChunk : visibleChunks)
				{
					TerrainRenderChunk& chunk = *pChunk;

					int lod = chunk.renderLOD;
					Mesh& mesh = mTerrainMeshes[lod];
					mProgTerrain->setParam(commandList, SHADER_PARAM(LocalToWorld), chunk.localToWorld);
					mProgTerrain->setParam(commandList, SHADER_PARAM(TerrainScale), chunk.scale);
					mProgTerrain->setParam(commandList, SHADER_PARAM(TerrainHeightRange), chunk.heightRange);
					mProgTerrain->setParam(commandList, SHADER_PARAM(TerrainMeshTileCount), int(kTerrainLODMeshTileCounts[lod]));
					mProgTerrain->setParam(commandList, SHADER_PARAM(TerrainLODBlend), chunk.lodBlend);
					mProgTerrain->setParam(commandList, SHADER_PARAM(TerrainStitchDelta), calcChunkStitchDelta(chunk));
					mProgTerrain->setTexture(commandList, SHADER_PARAM(TextureHeight), *chunk.texture, SHADER_SAMPLER(TextureHeight), TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());
					mProgTerrain->setTexture(commandList, SHADER_PARAM(TextureNormal), *chunk.texture, SHADER_SAMPLER(TextureNormal), TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp>::GetRHI());
					mesh.draw(commandList);
					++numDrawChunks;
					++numLODChunks[lod];
					if (mesh.mIndexBuffer.isValid())
					{
						numDrawTriangles += mesh.mIndexBuffer->getNumElements() / 3;
					}
				}

				if (mbUseDebugCamera)
				{
					Vector3 vertices[8];
					FViewUtils::GetFrustumVertices(cullView.clipToWorld, vertices, false);
					mDebugPrimitives.clear();
					for (int i = 0; i < 4; ++i)
					{
						mDebugPrimitives.addLine(vertices[i], vertices[4 + i], Color3f(1, 0.5, 0), 2);
						mDebugPrimitives.addLine(vertices[i], vertices[(i + 1) % 4], Color3f(1, 0.5, 0), 2);
						mDebugPrimitives.addLine(vertices[i + 4], vertices[4 + (i + 1) % 4], Color3f(1, 0.5, 0), 2);
					}
					mDebugPrimitives.drawDynamic(commandList, mView);
				}
			}

			double latMeters = 0;
			double lonMeters = 0;
			double maxMeters = 0;
			calcChunkMeters(mCenterChunkCoord, latMeters, lonMeters, maxMeters);
			float chunkWorldSize = getConfiguredChunkWorldSize(mCenterChunkCoord);
			float terrain1KmWorldSize = (maxMeters > 0.0) ? (chunkWorldSize * 1000.0f / float(maxMeters)) : 0.0f;
			Vector3 cameraPos = mCamera.getPos();
			float cameraDistance = cameraPos.length();
			Vector4 centerViewPos = Vector4(0, 0, 0, 1) * mView.worldToView;
			Vector4 centerClipPos = Vector4(0, 0, 0, 1) * mView.worldToClipRHI;
			float centerNdcZ = (Math::Abs(centerClipPos.w) > 1e-6f) ? (centerClipPos.z / centerClipPos.w) : 0.0f;
			float borderGapX = 0;
			float borderGapY = 0;
			calcChunkBorderGap(borderGapX, borderGapY);
			Vector4 centerHaloErr;
			Vector4 centerPackedNormalErr;
			calcCenterChunkNormalDebug(centerHaloErr, centerPackedNormalErr);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();
			g.drawText(Vector2(10, 10), InlineString<>::Make("Chunks = %u", numDrawChunks));
			g.drawText(Vector2(10, 28), InlineString<>::Make("Triangles = %u", numDrawTriangles));
			g.drawText(Vector2(10, 46), InlineString<>::Make("1km = %.3f wu", terrain1KmWorldSize));
			g.drawText(Vector2(10, 64), InlineString<>::Make("ChunkSize = %.3f wu", chunkWorldSize));
			g.drawText(Vector2(10, 82), InlineString<>::Make("Far = %.3f  CamDist = %.3f", mViewFrustum.mFar, cameraDistance));
			g.drawText(Vector2(10, 100), InlineString<>::Make("CenterViewZ = %.3f  CenterNdcZ = %.6f", centerViewPos.z, centerNdcZ));
			g.drawText(Vector2(10, 118), InlineString<>::Make("LOD0/1/2/3 = %u / %u / %u / %u", numLODChunks[0], numLODChunks[1], numLODChunks[2], numLODChunks[3]));
			g.drawText(Vector2(10, 136), InlineString<>::Make("UseLOD = %s", mbUseLOD ? "On" : "Off"));
			g.drawText(Vector2(10, 154), InlineString<>::Make("BorderGap XY = %.6f / %.6f", borderGapX, borderGapY));
			g.drawText(Vector2(10, 172), InlineString<>::Make("CenterHaloErr L/R/B/T = %.6f / %.6f / %.6f / %.6f", centerHaloErr.x, centerHaloErr.y, centerHaloErr.z, centerHaloErr.w));
			g.drawText(Vector2(10, 190), InlineString<>::Make("CenterNormErr L/R/B/T = %.6f / %.6f / %.6f / %.6f", centerPackedNormalErr.x, centerPackedNormalErr.y, centerPackedNormalErr.z, centerPackedNormalErr.w));
			g.endRender();
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::T:
					requestConfiguredTerrainChunks();
					return MsgReply::Handled();
				case EKeyCode::O:
					requestConfiguredTerrainChunks(true);
					return MsgReply::Handled();
				case EKeyCode::V:
					mbWireframe = !mbWireframe;
					return MsgReply::Handled();
				case EKeyCode::L:
					mbUseLOD = !mbUseLOD;
					return MsgReply::Handled();
				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			case UI_DOWNLOAD_DEM:
				requestConfiguredTerrainChunks();
				return true;
			case UI_OPEN_DEM:
				requestConfiguredTerrainChunks(true);
				return true;
			case UI_WIREFRAME:
			case UI_USE_LOD:
				return true;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}

	private:
		void requestConfiguredTerrainChunks(bool bForceRefresh = false)
		{
			TerrainChunkCoord centerCoord = getConfiguredChunkCoord();
			requestTerrainChunksAround(centerCoord, bForceRefresh, true);
		}

		void ensureTerrainChunksForCamera()
		{
			if (mRenderChunks.empty())
				return;

			TerrainChunkCoord cameraChunkCoord = calcCameraChunkCoord();
			bool bCenterChanged = (cameraChunkCoord.latIndex != mCenterChunkCoord.latIndex) || (cameraChunkCoord.lonIndex != mCenterChunkCoord.lonIndex);
			if (bCenterChanged)
			{
				requestTerrainChunksAround(cameraChunkCoord, false, false);
			}
			else if (hasMissingChunksAround(mCenterChunkCoord))
			{
				requestTerrainChunksAround(mCenterChunkCoord, false, false);
			}
		}

		void requestTerrainChunksAround(TerrainChunkCoord const& centerCoord, bool bForceRefresh, bool bResetExisting)
		{
			if (bResetExisting)
			{
				mCenterChunkCoord = centerCoord;
				mRenderChunks.clear();
				mbHasTerrainBaseHeight = false;
				mTerrainBaseHeight = 0;
				restart();

				{
					Mutex::Locker lock(mChunkMutex);
					mPendingChunkResults.clear();
					mPendingChunkKeys.clear();
					mPendingChunkRawData.clear();
					mChunkTaskOrder.clear();
					mChunkFinalizeQueued.clear();
					mChunkTasksInFlight.clear();
				}
				++mChunkRequestGeneration;
			}
			else if (centerCoord.latIndex != mCenterChunkCoord.latIndex || centerCoord.lonIndex != mCenterChunkCoord.lonIndex)
			{
				shiftCamerasForCenterChange(mCenterChunkCoord, centerCoord);
				mCenterChunkCoord = centerCoord;
				updateAllChunkTransforms();
				pruneChunksOutsideRange(centerCoord, getConfiguredChunkLoadDistance() + 1);
			}

			uint32 generation = mChunkRequestGeneration.load();
			std::unique_ptr< ITerrainDataProvider > provider = createConfiguredProvider();
			bool bCanDownload = provider->canDownload();
			bool bNeedCredential = false;
			int loadDistance = getConfiguredChunkLoadDistance();
			LogMsg("Request terrain chunks : provider=%s centerLat=%g centerLon=%g bbox=[south=%g north=%g west=%g east=%g] degreeSize=%g loadDistance=%d force=%d reset=%d",
				provider->getName(),
				centerCoord.latitude,
				centerCoord.longitude,
				centerCoord.south,
				centerCoord.north,
				centerCoord.west,
				centerCoord.east,
				getConfiguredChunkDegreeSize(),
				loadDistance,
				bForceRefresh ? 1 : 0,
				bResetExisting ? 1 : 0);

			for (int ring = 0; ring <= loadDistance; ++ring)
			{
				for (int offsetY = -ring; offsetY <= ring; ++offsetY)
				{
					for (int offsetX = -ring; offsetX <= ring; ++offsetX)
					{
						if (ring > 0 && Math::Max(std::abs(offsetX), std::abs(offsetY)) != ring)
						{
							continue;
						}

						TerrainChunkCoord coord = makeOffsetChunkCoord(centerCoord, offsetX, offsetY);
						int64 chunkKey = makeChunkCoordKey(coord);
						if (!bForceRefresh)
						{
							if (mRenderChunks.find(chunkKey) != mRenderChunks.end())
								continue;

							Mutex::Locker lock(mChunkMutex);
							if (mChunkTasksInFlight.count(chunkKey) || mPendingChunkKeys.count(chunkKey))
								continue;
						}

						TerrainChunkCacheData cachedData;
						if (!bForceRefresh && loadCachedTerrainChunk(coord, cachedData))
						{
							uint64 taskOrder = ++mChunkTaskOrderCounter;
							{
								Mutex::Locker lock(mChunkMutex);
								mChunkTasksInFlight.insert(chunkKey);
								mChunkTaskOrder[chunkKey] = taskOrder;
							}

						mChunkWorkPool.addFunctionWork([this, coord, generation, chunkKey, taskOrder, cachedData = std::move(cachedData)]() mutable
						{
							TIME_SCOPE("Terrain.CacheChunkPrepare");
							Mutex::Locker lock(mChunkMutex);
							if (generation == mChunkRequestGeneration.load() && cachedData.isValid())
							{
								PendingTerrainChunkRaw& pending = mPendingChunkRawData[chunkKey];
								pending.generation = generation;
								pending.order = taskOrder;
								pending.coord = coord;
								pending.data = std::move(cachedData);
								queueFinalizeChunk_NoLock(chunkKey, generation);
							}
							else
							{
								mChunkTasksInFlight.erase(chunkKey);
								mChunkTaskOrder.erase(chunkKey);
							}
						});
							continue;
						}

						if (!bCanDownload)
						{
							bNeedCredential = true;
							continue;
						}

						uint64 taskOrder = ++mChunkTaskOrderCounter;
						{
							Mutex::Locker lock(mChunkMutex);
							mChunkTasksInFlight.insert(chunkKey);
							mChunkTaskOrder[chunkKey] = taskOrder;
						}

						mChunkWorkPool.addFunctionWork([this, coord, generation, chunkKey, taskOrder, provider = createConfiguredProvider()]() mutable
						{
							TIME_SCOPE("Terrain.DownloadChunkPrepare");
							TerrainChunkCacheData rawData;
							bool bSuccess = provider->buildChunkCacheData(coord, rawData);
							if (bSuccess)
							{
								saveCachedTerrainChunk(coord, rawData);
							}

							Mutex::Locker lock(mChunkMutex);
							if (generation == mChunkRequestGeneration.load() && bSuccess && rawData.isValid())
							{
								PendingTerrainChunkRaw& pending = mPendingChunkRawData[chunkKey];
								pending.generation = generation;
								pending.order = taskOrder;
								pending.coord = coord;
								pending.data = std::move(rawData);
								queueFinalizeChunk_NoLock(chunkKey, generation);
							}
							else
							{
								mChunkTasksInFlight.erase(chunkKey);
								mChunkTaskOrder.erase(chunkKey);
							}
						});
					}
				}
			}

			if (bNeedCredential)
			{
				provider->logMissingCredential();
			}
		}

		TerrainChunkCoord getConfiguredChunkCoord() const
		{
			float latitude = ::Global::GameConfig().getFloatValue("Latitude", kOpenTopoConfigSection, 25.0330f);
			float longitude = ::Global::GameConfig().getFloatValue("Longitude", kOpenTopoConfigSection, 121.5654f);
			float chunkDegreeSize = getConfiguredChunkDegreeSize();

			TerrainChunkCoord result;
			result.latitude = latitude;
			result.longitude = longitude;

			result.latIndex = int(std::floor((latitude + 90.0f) / chunkDegreeSize));
			result.lonIndex = int(std::floor((longitude + 180.0f) / chunkDegreeSize));
			result.south = -90.0f + result.latIndex * chunkDegreeSize;
			result.west = -180.0f + result.lonIndex * chunkDegreeSize;
			result.north = result.south + chunkDegreeSize;
			result.east = result.west + chunkDegreeSize;
			return result;
		}

		float getConfiguredChunkDegreeSize() const
		{
			return ::Global::GameConfig().getFloatValue("ChunkDegreeSize", kOpenTopoConfigSection, kDefaultChunkDegreeSize);
		}

		float getConfiguredTerrain1KmWorldSize() const
		{
			return ::Global::GameConfig().getFloatValue("Terrain1KmWorldSize", kOpenTopoConfigSection, 0.0f);
		}

		float getConfiguredChunkWorldSize() const
		{
			return ::Global::GameConfig().getFloatValue("ChunkWorldSize", kOpenTopoConfigSection, 50.0f);
		}

		float getConfiguredChunkWorldSize(TerrainChunkCoord const& coord) const
		{
			float terrain1KmWorldSize = getConfiguredTerrain1KmWorldSize();
			if (terrain1KmWorldSize > 0.0f)
			{
				double latMeters = 0;
				double lonMeters = 0;
				double maxMeters = 0;
				calcChunkMeters(coord, latMeters, lonMeters, maxMeters);
				return terrain1KmWorldSize * float(maxMeters / 1000.0);
			}
			return getConfiguredChunkWorldSize();
		}

		float getConfiguredVerticalExaggeration() const
		{
			return ::Global::GameConfig().getFloatValue("VerticalExaggeration", kOpenTopoConfigSection, 1.0f);
		}

		char const* getConfiguredProviderName() const
		{
			char const* value = ::Global::GameConfig().getStringValue("Provider", kOpenTopoConfigSection, "Copernicus");
			if (FCString::CompareIgnoreCase(value, "OpenTopography") == 0)
				return "OpenTopography";
			return "Copernicus";
		}

		std::string getConfiguredCopernicusClientId() const
		{
			char const* value = ::Global::GameConfig().getStringValue("CopernicusClientId", kOpenTopoConfigSection, "");
			return value ? value : "";
		}

		std::string getConfiguredCopernicusClientSecret() const
		{
			char const* value = ::Global::GameConfig().getStringValue("CopernicusClientSecret", kOpenTopoConfigSection, "");
			return value ? value : "";
		}

		char const* getConfiguredCopernicusDemInstance() const
		{
			return ::Global::GameConfig().getStringValue("CopernicusDemInstance", kOpenTopoConfigSection, "COPERNICUS_30");
		}

		std::unique_ptr< ITerrainDataProvider > createConfiguredProvider()
		{
			char const* value = ::Global::GameConfig().getStringValue("Provider", kOpenTopoConfigSection, "Copernicus");
			if (FCString::CompareIgnoreCase(value, "OpenTopography") == 0)
			{
				return std::make_unique< OpenTopographyProvider >(mProviderService, ::Global::GameConfig().getStringValue("ApiKey", kOpenTopoConfigSection, ""));
			}
			return std::make_unique< CopernicusProvider >(mProviderService, getConfiguredCopernicusClientId(), getConfiguredCopernicusClientSecret(), getConfiguredCopernicusDemInstance());
		}

		int getConfiguredChunkLoadDistance() const
		{
			return Math::Max(0, ::Global::GameConfig().getIntValue("ChunkLoadDistance", kOpenTopoConfigSection, 4));
		}

		TerrainChunkCoord makeChunkCoord(int latIndex, int lonIndex) const
		{
			float chunkDegreeSize = getConfiguredChunkDegreeSize();

			TerrainChunkCoord result;
			result.latIndex = latIndex;
			result.lonIndex = lonIndex;
			result.south = -90.0f + result.latIndex * chunkDegreeSize;
			result.west = -180.0f + result.lonIndex * chunkDegreeSize;
			result.north = result.south + chunkDegreeSize;
			result.east = result.west + chunkDegreeSize;
			result.latitude = 0.5f * (result.south + result.north);
			result.longitude = 0.5f * (result.west + result.east);
			return result;
		}

		TerrainChunkCoord makeOffsetChunkCoord(TerrainChunkCoord const& centerCoord, int offsetX, int offsetY) const
		{
			return makeChunkCoord(centerCoord.latIndex + offsetY, centerCoord.lonIndex + offsetX);
		}

		TerrainChunkCoord calcCameraChunkCoord() const
		{
			float planarSizeX = 0;
			float planarSizeY = 0;
			float worldUnitsPerMeter = 0;
			calcChunkProjectionScale(planarSizeX, planarSizeY, worldUnitsPerMeter);
			if (planarSizeX <= 0.0f || planarSizeY <= 0.0f)
				return mCenterChunkCoord;

			int offsetX = Math::FloorToInt(mCamera.getPos().x / planarSizeX);
			int offsetY = Math::FloorToInt(mCamera.getPos().y / planarSizeY);
			return makeChunkCoord(mCenterChunkCoord.latIndex + offsetY, mCenterChunkCoord.lonIndex + offsetX);
		}

		bool hasMissingChunksAround(TerrainChunkCoord const& centerCoord)
		{
			int loadDistance = getConfiguredChunkLoadDistance();
			for (int offsetY = -loadDistance; offsetY <= loadDistance; ++offsetY)
			{
				for (int offsetX = -loadDistance; offsetX <= loadDistance; ++offsetX)
				{
					TerrainChunkCoord coord = makeOffsetChunkCoord(centerCoord, offsetX, offsetY);
					int64 chunkKey = makeChunkCoordKey(coord);
					if (mRenderChunks.find(chunkKey) != mRenderChunks.end())
						continue;

					{
						Mutex::Locker lock(mChunkMutex);
						if (mChunkTasksInFlight.count(chunkKey) ||
							mPendingChunkKeys.count(chunkKey) ||
							mPendingChunkRawData.count(chunkKey) ||
							mChunkFinalizeQueued.count(chunkKey))
						{
							continue;
						}
					}

					TerrainChunkCacheData cachedData;
					if (loadCachedTerrainChunk(coord, cachedData))
						return true;
					return true;
				}
			}
			return false;
		}

		DataCacheKey makeChunkCacheKey(TerrainChunkCoord const& coord) const
		{
			DataCacheKey key;
			key.typeName = "OT-DEM-CHUNK";
			key.version = "9C96D9F8-1A0D-4980-A3FD-4AE8D9F9B5CE";
			key.keySuffix.add(getConfiguredProviderName(), coord.latIndex, coord.lonIndex, kTerrainChunkTileCount, int(getConfiguredChunkDegreeSize() * 100000000.0f + 0.5f));
			return key;
		}

		bool loadCachedTerrainChunk(TerrainChunkCoord const& coord, TerrainChunkCacheData& outData)
		{
			return ::Global::DataCache().loadT(makeChunkCacheKey(coord), outData) && outData.isValid();
		}

		void saveCachedTerrainChunk(TerrainChunkCoord const& coord, TerrainChunkCacheData const& data)
		{
			if (!::Global::DataCache().saveT(makeChunkCacheKey(coord), data))
			{
				LogWarning(0, "Failed to save terrain chunk to DataCache : lat=%d lon=%d", coord.latIndex, coord.lonIndex);
			}
		}

		int64 makeChunkCoordKey(TerrainChunkCoord const& coord) const
		{
			return (int64(coord.latIndex) << 32) | uint32(coord.lonIndex);
		}

		void queueFinalizeChunk_NoLock(int64 chunkKey, uint32 generation)
		{
			if (mChunkFinalizeQueued.count(chunkKey))
				return;
			if (!mPendingChunkRawData.count(chunkKey))
				return;

			mChunkFinalizeQueued.insert(chunkKey);
			mChunkFinalizeWorkPool.addFunctionWork([this, chunkKey, generation]()
			{
				processFinalizeChunk(chunkKey, generation);
			});
		}

		bool tryCaptureFinalizeNeighborBorders_NoLock(PendingTerrainChunkRaw const& pending, TerrainChunkNeighborBorders& outBorders) const
		{
			auto captureRenderedNeighbor = [&](int latIndex, int lonIndex, uint8 borderMask, bool bVertical, bool bTakeMaxSide)
			{
				auto it = mRenderChunks.find(makeChunkCoordKey(TerrainChunkCoord{ latIndex, lonIndex }));
				if (it == mRenderChunks.end() || it->second.heightData.empty())
					return false;

				TerrainRenderChunk const& neighbor = it->second;
				if (bVertical)
				{
					TArray< float >& heights = (borderMask == ChunkBorderMask_Left) ? outBorders.leftHeights : outBorders.rightHeights;
					heights.resize(neighbor.height);
					uint32 sampleX = bTakeMaxSide ? (neighbor.width - 1) : 0;
					for (uint32 y = 0; y < neighbor.height; ++y)
					{
						heights[y] = decodeChunkSampleHeight(neighbor, neighbor.heightData[sampleX + y * neighbor.width]);
					}
				}
				else
				{
					TArray< float >& heights = (borderMask == ChunkBorderMask_Bottom) ? outBorders.bottomHeights : outBorders.topHeights;
					heights.resize(neighbor.width);
					uint32 sampleY = bTakeMaxSide ? (neighbor.height - 1) : 0;
					for (uint32 x = 0; x < neighbor.width; ++x)
					{
						heights[x] = decodeChunkSampleHeight(neighbor, neighbor.heightData[x + sampleY * neighbor.width]);
					}
				}
				outBorders.mask |= borderMask;
				return true;
			};

			auto capturePendingNeighbor = [&](int latIndex, int lonIndex, uint8 borderMask, bool bVertical, bool bTakeMaxSide)
			{
				int64 neighborKey = makeChunkCoordKey(TerrainChunkCoord{ latIndex, lonIndex });
				auto itRaw = mPendingChunkRawData.find(neighborKey);
				if (itRaw == mPendingChunkRawData.end() || !itRaw->second.data.isValid())
					return false;

				PendingTerrainChunkRaw const& neighbor = itRaw->second;
				if (neighbor.generation != pending.generation)
					return false;

				if (bVertical)
				{
					TArray< float >& heights = (borderMask == ChunkBorderMask_Left) ? outBorders.leftHeights : outBorders.rightHeights;
					heights.resize(neighbor.data.height);
					uint32 sampleX = bTakeMaxSide ? (neighbor.data.width - 1) : 0;
					for (uint32 y = 0; y < neighbor.data.height; ++y)
					{
						heights[y] = decodeCachedSampleHeight(neighbor.data, neighbor.data.heightData[sampleX + y * neighbor.data.width]);
					}
				}
				else
				{
					TArray< float >& heights = (borderMask == ChunkBorderMask_Bottom) ? outBorders.bottomHeights : outBorders.topHeights;
					heights.resize(neighbor.data.width);
					uint32 sampleY = bTakeMaxSide ? (neighbor.data.height - 1) : 0;
					for (uint32 x = 0; x < neighbor.data.width; ++x)
					{
						heights[x] = decodeCachedSampleHeight(neighbor.data, neighbor.data.heightData[x + sampleY * neighbor.data.width]);
					}
				}
				outBorders.mask |= borderMask;
				return true;
			};

			auto needWaitForNeighbor = [&](int latIndex, int lonIndex)
			{
				int64 neighborKey = makeChunkCoordKey(TerrainChunkCoord{ latIndex, lonIndex });
				auto itOrder = mChunkTaskOrder.find(neighborKey);
				if (itOrder == mChunkTaskOrder.end())
					return false;
				if (itOrder->second >= pending.order)
					return false;
				if (mRenderChunks.count(neighborKey))
					return false;
				if (mPendingChunkRawData.count(neighborKey))
					return false;
				return mChunkTasksInFlight.count(neighborKey) != 0;
			};

			if (needWaitForNeighbor(pending.coord.latIndex, pending.coord.lonIndex - 1) ||
				needWaitForNeighbor(pending.coord.latIndex - 1, pending.coord.lonIndex))
			{
				return false;
			}

			captureRenderedNeighbor(pending.coord.latIndex, pending.coord.lonIndex - 1, ChunkBorderMask_Left, true, true) ||
				capturePendingNeighbor(pending.coord.latIndex, pending.coord.lonIndex - 1, ChunkBorderMask_Left, true, true);
			captureRenderedNeighbor(pending.coord.latIndex, pending.coord.lonIndex + 1, ChunkBorderMask_Right, true, false) ||
				capturePendingNeighbor(pending.coord.latIndex, pending.coord.lonIndex + 1, ChunkBorderMask_Right, true, false);
			captureRenderedNeighbor(pending.coord.latIndex - 1, pending.coord.lonIndex, ChunkBorderMask_Bottom, false, true) ||
				capturePendingNeighbor(pending.coord.latIndex - 1, pending.coord.lonIndex, ChunkBorderMask_Bottom, false, true);
			captureRenderedNeighbor(pending.coord.latIndex + 1, pending.coord.lonIndex, ChunkBorderMask_Top, false, false) ||
				capturePendingNeighbor(pending.coord.latIndex + 1, pending.coord.lonIndex, ChunkBorderMask_Top, false, false);
			return true;
		}

		void processFinalizeChunk(int64 chunkKey, uint32 generation)
		{
			PendingTerrainChunkRaw pending;
			{
				Mutex::Locker lock(mChunkMutex);
				mChunkFinalizeQueued.erase(chunkKey);
				auto it = mPendingChunkRawData.find(chunkKey);
				if (it == mPendingChunkRawData.end())
					return;
				if (it->second.generation != generation || generation != mChunkRequestGeneration.load())
				{
					mPendingChunkRawData.erase(it);
					mChunkTasksInFlight.erase(chunkKey);
					mChunkTaskOrder.erase(chunkKey);
					return;
				}
				pending = it->second;
				mPendingChunkRawData.erase(it);
			}

			TerrainChunkResult result;
			result.generation = generation;
			result.coord = pending.coord;
			result.bSuccess = pending.data.isValid();
			result.data = std::move(pending.data);
			if (result.bSuccess)
			{
				buildPreparedChunkTextureData(result.coord, result.data, result.textureData);
			}

			Mutex::Locker lock(mChunkMutex);
			if (generation == mChunkRequestGeneration.load() && !mPendingChunkKeys.count(chunkKey))
			{
				mPendingChunkResults.push_back(std::move(result));
				mPendingChunkKeys.insert(chunkKey);
			}
			else
			{
				mChunkTasksInFlight.erase(chunkKey);
				mChunkTaskOrder.erase(chunkKey);
			}

			auto queueNeighborIfPending = [&](int latIndex, int lonIndex)
			{
				int64 neighborKey = makeChunkCoordKey(TerrainChunkCoord{ latIndex, lonIndex });
				if (mPendingChunkRawData.count(neighborKey))
				{
					queueFinalizeChunk_NoLock(neighborKey, generation);
				}
			};
			queueNeighborIfPending(result.coord.latIndex, result.coord.lonIndex - 1);
			queueNeighborIfPending(result.coord.latIndex, result.coord.lonIndex + 1);
			queueNeighborIfPending(result.coord.latIndex - 1, result.coord.lonIndex);
			queueNeighborIfPending(result.coord.latIndex + 1, result.coord.lonIndex);
		}

		float decodeCachedSampleHeight(TerrainChunkCacheData const& data, uint16 value) const
		{
			if (data.maxHeight <= data.minHeight)
				return data.minHeight;

			float t = float(value) / 65535.0f;
			return Math::Lerp(data.minHeight, data.maxHeight, t);
		}

		uint16 encodeCachedSampleHeight(TerrainChunkCacheData const& data, float value) const
		{
			float range = Math::Max(data.maxHeight - data.minHeight, 1e-6f);
			float t = Math::Clamp((value - data.minHeight) / range, 0.0f, 1.0f);
			return uint16(t * 65535.0f + 0.5f);
		}

		void remapCachedHeightRange(TerrainChunkCacheData& data, float newMinHeight, float newMaxHeight) const
		{
			if (!data.isValid())
				return;
			if (newMinHeight >= data.minHeight && newMaxHeight <= data.maxHeight)
				return;

			newMinHeight = Math::Min(newMinHeight, data.minHeight);
			newMaxHeight = Math::Max(newMaxHeight, data.maxHeight);
			if (newMaxHeight <= newMinHeight)
				return;

			TArray<uint16> oldHeightData = std::move(data.heightData);
			TArray<uint16> oldHaloHeightData = std::move(data.haloHeightData);
			float oldMinHeight = data.minHeight;
			float oldMaxHeight = data.maxHeight;

			data.minHeight = newMinHeight;
			data.maxHeight = newMaxHeight;
			data.heightData.resize(oldHeightData.size());
			data.haloHeightData.resize(oldHaloHeightData.size());

			auto decodeOldValue = [&](uint16 value)
			{
				if (oldMaxHeight <= oldMinHeight)
					return oldMinHeight;
				float t = float(value) / 65535.0f;
				return Math::Lerp(oldMinHeight, oldMaxHeight, t);
			};

			for (int i = 0; i < oldHeightData.size(); ++i)
			{
				data.heightData[i] = encodeCachedSampleHeight(data, decodeOldValue(oldHeightData[i]));
			}
			for (int i = 0; i < oldHaloHeightData.size(); ++i)
			{
				data.haloHeightData[i] = encodeCachedSampleHeight(data, decodeOldValue(oldHaloHeightData[i]));
			}
		}

		float decodeCachedHaloSampleHeight(TerrainChunkCacheData const& data, int x, int y) const
		{
			if (!data.isValid())
				return data.minHeight;

			x = Math::Clamp(x, -1, int(data.width));
			y = Math::Clamp(y, -1, int(data.height));
			uint32 haloWidth = data.width + 2;
			uint16 value = data.haloHeightData[(x + 1) + (y + 1) * haloWidth];
			return decodeCachedSampleHeight(data, value);
		}

		TerrainChunkNeighborBorders captureChunkNeighborBorders(TerrainChunkCoord const& coord) const
		{
			TerrainChunkNeighborBorders result;

			auto captureNeighbor = [&](int latIndex, int lonIndex, uint8 borderMask, bool bVertical, bool bTakeMaxSide)
			{
				auto it = mRenderChunks.find(makeChunkCoordKey(TerrainChunkCoord{ latIndex, lonIndex }));
				if (it == mRenderChunks.end() || it->second.heightData.empty())
					return;

				TerrainRenderChunk const& neighbor = it->second;
				if (bVertical)
				{
					TArray< float >& heights = (borderMask == ChunkBorderMask_Left) ? result.leftHeights : result.rightHeights;
					heights.resize(neighbor.height);
					uint32 sampleX = bTakeMaxSide ? (neighbor.width - 1) : 0;
					for (uint32 y = 0; y < neighbor.height; ++y)
					{
						heights[y] = decodeChunkSampleHeight(neighbor, neighbor.heightData[sampleX + y * neighbor.width]);
					}
				}
				else
				{
					TArray< float >& heights = (borderMask == ChunkBorderMask_Bottom) ? result.bottomHeights : result.topHeights;
					heights.resize(neighbor.width);
					uint32 sampleY = bTakeMaxSide ? (neighbor.height - 1) : 0;
					for (uint32 x = 0; x < neighbor.width; ++x)
					{
						heights[x] = decodeChunkSampleHeight(neighbor, neighbor.heightData[x + sampleY * neighbor.width]);
					}
				}
				result.mask |= borderMask;
			};

			captureNeighbor(coord.latIndex, coord.lonIndex - 1, ChunkBorderMask_Left, true, true);
			captureNeighbor(coord.latIndex, coord.lonIndex + 1, ChunkBorderMask_Right, true, false);
			captureNeighbor(coord.latIndex - 1, coord.lonIndex, ChunkBorderMask_Bottom, false, true);
			captureNeighbor(coord.latIndex + 1, coord.lonIndex, ChunkBorderMask_Top, false, false);
			return result;
		}

		void applyNeighborBordersToChunkData(TerrainChunkCacheData& data, TerrainChunkNeighborBorders const& borders) const
		{
			if (!data.isValid() || borders.mask == ChunkBorderMask_None)
				return;

			float borderMin = data.minHeight;
			float borderMax = data.maxHeight;
			auto includeHeights = [&](TArray<float> const& heights)
			{
				for (float value : heights)
				{
					borderMin = Math::Min(borderMin, value);
					borderMax = Math::Max(borderMax, value);
				}
			};
			if (borders.mask & ChunkBorderMask_Left) includeHeights(borders.leftHeights);
			if (borders.mask & ChunkBorderMask_Right) includeHeights(borders.rightHeights);
			if (borders.mask & ChunkBorderMask_Bottom) includeHeights(borders.bottomHeights);
			if (borders.mask & ChunkBorderMask_Top) includeHeights(borders.topHeights);
			remapCachedHeightRange(data, borderMin, borderMax);

			uint32 haloWidth = data.width + 2;
			if (borders.mask & ChunkBorderMask_Left)
			{
				uint32 count = Math::Min<uint32>(data.height, borders.leftHeights.size());
				for (uint32 y = 0; y < count; ++y)
				{
					uint16 encoded = encodeCachedSampleHeight(data, borders.leftHeights[y]);
					data.heightData[0 + y * data.width] = encoded;
					data.haloHeightData[1 + (y + 1) * haloWidth] = encoded;
				}
			}
			if (borders.mask & ChunkBorderMask_Right)
			{
				uint32 count = Math::Min<uint32>(data.height, borders.rightHeights.size());
				uint32 x = data.width - 1;
				for (uint32 y = 0; y < count; ++y)
				{
					uint16 encoded = encodeCachedSampleHeight(data, borders.rightHeights[y]);
					data.heightData[x + y * data.width] = encoded;
					data.haloHeightData[(x + 1) + (y + 1) * haloWidth] = encoded;
				}
			}
			if (borders.mask & ChunkBorderMask_Bottom)
			{
				uint32 count = Math::Min<uint32>(data.width, borders.bottomHeights.size());
				for (uint32 x = 0; x < count; ++x)
				{
					uint16 encoded = encodeCachedSampleHeight(data, borders.bottomHeights[x]);
					data.heightData[x] = encoded;
					data.haloHeightData[(x + 1) + (data.height + 0) * haloWidth] = encoded;
				}
			}
			if (borders.mask & ChunkBorderMask_Top)
			{
				uint32 count = Math::Min<uint32>(data.width, borders.topHeights.size());
				uint32 y = data.height - 1;
				for (uint32 x = 0; x < count; ++x)
				{
					uint16 encoded = encodeCachedSampleHeight(data, borders.topHeights[x]);
					data.heightData[x + y * data.width] = encoded;
					data.haloHeightData[(x + 1) + 1 * haloWidth] = encoded;
				}
			}
		}

		void applyPendingChunkResult()
		{
			PROFILE_ENTRY("ApplyPendingChunkResult");
			TArray< TerrainChunkResult > results;
			{
				Mutex::Locker lock(mChunkMutex);
				constexpr int kMaxApplyPerFrame = 2;
				int numResults = Math::Min<int>(int(mPendingChunkResults.size()), kMaxApplyPerFrame);
				results.reserve(numResults);
				for (int i = 0; i < numResults; ++i)
				{
					TerrainChunkResult& pending = mPendingChunkResults.back();
					mPendingChunkKeys.erase(makeChunkCoordKey(pending.coord));
					results.push_back(std::move(pending));
					mPendingChunkResults.pop_back();
				}
			}

			for (TerrainChunkResult& result : results)
			{
				int64 chunkKey = makeChunkCoordKey(result.coord);
				if (result.bSuccess && result.generation == mChunkRequestGeneration.load())
				{
					applyChunkData(std::move(result));
				}
				Mutex::Locker lock(mChunkMutex);
				mChunkTasksInFlight.erase(chunkKey);
				mChunkTaskOrder.erase(chunkKey);
			}
		}

		void applyChunkData(TerrainChunkResult&& result)
		{
			PROFILE_ENTRY("ApplyChunkData");
			TerrainChunkCacheData& data = result.data;
			TerrainChunkCoord const& coord = result.coord;
			if (!data.isValid())
				return;

			int64 chunkKey = makeChunkCoordKey(coord);
			if (mRenderChunks.find(chunkKey) != mRenderChunks.end())
				return;

			if (!mbHasTerrainBaseHeight || data.minHeight < mTerrainBaseHeight)
			{
				mbHasTerrainBaseHeight = true;
				mTerrainBaseHeight = data.minHeight;
			}

			TerrainRenderChunk chunk;
			chunk.coord = coord;
			chunk.width = data.width;
			chunk.height = data.height;
			chunk.minHeight = data.minHeight;
			chunk.maxHeight = data.maxHeight;
			chunk.heightWorldScale = calcChunkHeightWorldScale(coord);
			chunk.heightData = std::move(data.heightData);
			chunk.haloHeightData = std::move(data.haloHeightData);
			chunk.textureData = std::move(result.textureData);
			chunk.heightRange = Vector2(data.minHeight, data.maxHeight);
			chunk.scale = calcTerrainScale(coord, data.minHeight, data.maxHeight);
			chunk.localToWorld = Matrix4::Translate(calcChunkWorldOffset(coord, data.minHeight));
			chunk.currentLOD = selectLODTarget(chunk);

			mRenderChunks[chunkKey] = std::move(chunk);
			for (auto& pair : mRenderChunks)
			{
				TerrainRenderChunk& updateChunk = pair.second;
				updateChunk.localToWorld = Matrix4::Translate(calcChunkWorldOffset(updateChunk.coord, updateChunk.minHeight));
			}
			if (!uploadChunkTexture(mRenderChunks[chunkKey]))
			{
				mRenderChunks.erase(chunkKey);
				return;
			}
			stitchChunkBorders(chunkKey);
			stitchChunkBorders(makeChunkCoordKey(makeChunkCoord(coord.latIndex, coord.lonIndex - 1)));
			stitchChunkBorders(makeChunkCoordKey(makeChunkCoord(coord.latIndex, coord.lonIndex + 1)));
			stitchChunkBorders(makeChunkCoordKey(makeChunkCoord(coord.latIndex - 1, coord.lonIndex)));
			stitchChunkBorders(makeChunkCoordKey(makeChunkCoord(coord.latIndex + 1, coord.lonIndex)));
			LogMsg("Applied terrain chunk : lat=%d lon=%d height=[%g, %g] scale=(%g, %g, %g)",
				coord.latIndex, coord.lonIndex, data.minHeight, data.maxHeight,
				mRenderChunks[chunkKey].scale.x,
				mRenderChunks[chunkKey].scale.y,
				mRenderChunks[chunkKey].scale.z);
		}

		void updateAllChunkTransforms()
		{
			for (auto& pair : mRenderChunks)
			{
				TerrainRenderChunk& chunk = pair.second;
				chunk.scale = calcTerrainScale(chunk.coord, chunk.minHeight, chunk.maxHeight);
				chunk.heightWorldScale = calcChunkHeightWorldScale(chunk.coord);
				chunk.localToWorld = Matrix4::Translate(calcChunkWorldOffset(chunk.coord, chunk.minHeight));
			}
		}

		void shiftCamerasForCenterChange(TerrainChunkCoord const& oldCenterCoord, TerrainChunkCoord const& newCenterCoord)
		{
			float planarSizeX = 0;
			float planarSizeY = 0;
			float worldUnitsPerMeter = 0;
			calcChunkProjectionScale(planarSizeX, planarSizeY, worldUnitsPerMeter);

			Vector3 delta(
				float(oldCenterCoord.lonIndex - newCenterCoord.lonIndex) * planarSizeX,
				float(oldCenterCoord.latIndex - newCenterCoord.latIndex) * planarSizeY,
				0.0f);

			mCamera.setPos(mCamera.getPos() + delta);
			mDebugCamera.setPos(mDebugCamera.getPos() + delta);
		}

		void pruneChunksOutsideRange(TerrainChunkCoord const& centerCoord, int keepDistance)
		{
			for (auto iter = mRenderChunks.begin(); iter != mRenderChunks.end(); )
			{
				TerrainRenderChunk const& chunk = iter->second;
				int dx = std::abs(chunk.coord.lonIndex - centerCoord.lonIndex);
				int dy = std::abs(chunk.coord.latIndex - centerCoord.latIndex);
				if (Math::Max(dx, dy) > keepDistance)
				{
					iter = mRenderChunks.erase(iter);
				}
				else
				{
					++iter;
				}
			}
		}

		Vector3 calcTerrainScale(TerrainChunkCoord const& coord, float minHeight, float maxHeight) const
		{
			float planarSizeX = 0;
			float planarSizeY = 0;
			float worldUnitsPerMeter = 0;
			calcChunkProjectionScale(planarSizeX, planarSizeY, worldUnitsPerMeter);
			if (worldUnitsPerMeter <= 0.0f)
			{
				float worldChunkSize = getConfiguredChunkWorldSize(mCenterChunkCoord);
				return Vector3(worldChunkSize, worldChunkSize, worldChunkSize * 0.1f);
			}

			float heightRangeMeters = Math::Max(maxHeight - minHeight, 1e-3f);
			return Vector3(
				planarSizeX,
				planarSizeY,
				getConfiguredVerticalExaggeration() * heightRangeMeters * worldUnitsPerMeter);
		}

		void calcChunkMeters(TerrainChunkCoord const& coord, double& outLatMeters, double& outLonMeters, double& outMaxMeters) const
		{
			double chunkDegreeSize = double(getConfiguredChunkDegreeSize());
			float centerLatitude = 0.5f * (mCenterChunkCoord.south + mCenterChunkCoord.north);
			double latitudeRadians = Math::DegToRad(double(centerLatitude));
			outLatMeters = 111132.0 * chunkDegreeSize;
			outLonMeters = 111320.0 * std::cos(latitudeRadians) * chunkDegreeSize;
			outMaxMeters = Math::Max(outLatMeters, outLonMeters);
		}

		Vector2 calcChunkPlanarSize(TerrainChunkCoord const& coord) const
		{
			double latMeters = 0;
			double lonMeters = 0;
			double maxMeters = 0;
			calcChunkMeters(coord, latMeters, lonMeters, maxMeters);

			float worldChunkSize = getConfiguredChunkWorldSize(coord);
			if (maxMeters <= 0.0)
				return Vector2(worldChunkSize, worldChunkSize);

			return Vector2(
				worldChunkSize * float(lonMeters / maxMeters),
				worldChunkSize * float(latMeters / maxMeters));
		}

		float calcChunkHeightWorldScale(TerrainChunkCoord const& coord) const
		{
			float planarSizeX = 0;
			float planarSizeY = 0;
			float worldUnitsPerMeter = 0;
			calcChunkProjectionScale(planarSizeX, planarSizeY, worldUnitsPerMeter);
			if (worldUnitsPerMeter <= 0.0f)
				return getConfiguredChunkWorldSize(coord) * 0.1f;

			return getConfiguredVerticalExaggeration() * worldUnitsPerMeter;
		}

		Vector3 calcChunkWorldOffset(TerrainChunkCoord const& coord, float minHeight) const
		{
			float planarSizeX = 0;
			float planarSizeY = 0;
			float worldUnitsPerMeter = 0;
			calcChunkProjectionScale(planarSizeX, planarSizeY, worldUnitsPerMeter);

			float offsetX = float(coord.lonIndex - mCenterChunkCoord.lonIndex) * planarSizeX;
			float offsetY = float(coord.latIndex - mCenterChunkCoord.latIndex) * planarSizeY;

			float heightOffset = 0;
			if (mbHasTerrainBaseHeight)
			{
				heightOffset = (minHeight - mTerrainBaseHeight) * calcChunkHeightWorldScale(coord);
			}
			return Vector3(offsetX, offsetY, heightOffset);
		}

		void calcChunkProjectionScale(float& outPlanarSizeX, float& outPlanarSizeY, float& outWorldUnitsPerMeter) const
		{
			double latMeters = 0;
			double lonMeters = 0;
			double maxMeters = 0;
			calcChunkMeters(mCenterChunkCoord, latMeters, lonMeters, maxMeters);
			if (maxMeters <= 0.0)
			{
				float worldChunkSize = getConfiguredChunkWorldSize(mCenterChunkCoord);
				outPlanarSizeX = worldChunkSize;
				outPlanarSizeY = worldChunkSize;
				outWorldUnitsPerMeter = 0.0f;
				return;
			}

			double latitudeRadians = Math::DegToRad(double(0.5f * (mCenterChunkCoord.south + mCenterChunkCoord.north)));
			double metersPerLatDegree = 111132.0;
			double metersPerLonDegree = 111320.0 * std::cos(latitudeRadians);
			double chunkDegreeSize = double(getConfiguredChunkDegreeSize());
			outWorldUnitsPerMeter = float(double(getConfiguredChunkWorldSize(mCenterChunkCoord)) / maxMeters);
			outPlanarSizeX = float(chunkDegreeSize * metersPerLonDegree * outWorldUnitsPerMeter);
			outPlanarSizeY = float(chunkDegreeSize * metersPerLatDegree * outWorldUnitsPerMeter);
		}

		Vector3 calcChunkCenterWorldPos(TerrainRenderChunk const& chunk) const
		{
			return chunk.localToWorld.getTranslation() + Vector3(0.5f * chunk.scale.x, 0.5f * chunk.scale.y, 0.5f * chunk.scale.z);
		}

		bool isChunkVisible(TerrainRenderChunk const& chunk, ViewInfo const& view) const
		{
			Vector3 center = calcChunkCenterWorldPos(chunk);
			float radius = 0.5f * Math::Sqrt(chunk.scale.x * chunk.scale.x + chunk.scale.y * chunk.scale.y + chunk.scale.z * chunk.scale.z);
			return view.frustumTest(center, radius);
		}

		void calcChunkBorderGap(float& outGapX, float& outGapY) const
		{
			outGapX = 0;
			outGapY = 0;

			float maxGapX = 0;
			float maxGapY = 0;
			for (auto const& pair : mRenderChunks)
			{
				TerrainRenderChunk const& chunk = pair.second;
				if (!chunk.texture)
					continue;

				int64 rightKey = makeChunkCoordKey(TerrainChunkCoord{ chunk.coord.latIndex, chunk.coord.lonIndex + 1 });
				auto itRight = mRenderChunks.find(rightKey);
				if (itRight != mRenderChunks.end() && itRight->second.texture)
				{
					TerrainRenderChunk const& rhs = itRight->second;
					float chunkRight = chunk.localToWorld.getTranslation().x + chunk.scale.x;
					float rhsLeft = rhs.localToWorld.getTranslation().x;
					maxGapX = Math::Max(maxGapX, Math::Abs(rhsLeft - chunkRight));
				}

				int64 topKey = makeChunkCoordKey(TerrainChunkCoord{ chunk.coord.latIndex + 1, chunk.coord.lonIndex });
				auto itTop = mRenderChunks.find(topKey);
				if (itTop != mRenderChunks.end() && itTop->second.texture)
				{
					TerrainRenderChunk const& top = itTop->second;
					float chunkTop = chunk.localToWorld.getTranslation().y + chunk.scale.y;
					float topBottom = top.localToWorld.getTranslation().y;
					maxGapY = Math::Max(maxGapY, Math::Abs(topBottom - chunkTop));
				}
			}

			outGapX = maxGapX;
			outGapY = maxGapY;
		}

		Vector3 decodePackedChunkTextureNormal(TerrainRenderChunk const& chunk, int x, int y) const
		{
			if (chunk.textureData.empty() || x < 0 || x >= int(chunk.width) || y < 0 || y >= int(chunk.height))
				return Vector3(0, 0, 1);

			uint32 index = x + y * chunk.width;
			float nx = (float(chunk.textureData[index * 4 + 2]) / 255.0f) * 2.0f - 1.0f;
			float ny = (float(chunk.textureData[index * 4 + 3]) / 255.0f) * 2.0f - 1.0f;
			float nz2 = Math::Max(0.0f, 1.0f - nx * nx - ny * ny);
			return Math::GetNormal(Vector3(nx, ny, Math::Sqrt(nz2)));
		}

		void calcCenterChunkNormalDebug(Vector4& outHaloErr, Vector4& outPackedNormalErr) const
		{
			outHaloErr = Vector4(0, 0, 0, 0);
			outPackedNormalErr = Vector4(0, 0, 0, 0);

			auto it = mRenderChunks.find(makeChunkCoordKey(mCenterChunkCoord));
			if (it == mRenderChunks.end())
				return;

			TerrainRenderChunk const& chunk = it->second;
			if (!chunk.texture || chunk.heightData.empty() || chunk.haloHeightData.empty() || chunk.textureData.empty())
				return;

			auto updateEdgeError = [&](int edgeIndex, int x, int y, int hx, int hy)
			{
				float h0 = decodeChunkSampleHeight(chunk, chunk.heightData[x + y * chunk.width]);
				float h1 = fetchChunkSampleHeight(chunk, hx, hy);
				outHaloErr[edgeIndex] = Math::Max(outHaloErr[edgeIndex], Math::Abs(h1 - h0));

				Vector3 packedNormal = decodePackedChunkTextureNormal(chunk, x, y);
				Vector3 recomputedNormal = calcChunkSampleNormal(chunk, x, y);
				outPackedNormalErr[edgeIndex] = Math::Max(outPackedNormalErr[edgeIndex], (packedNormal - recomputedNormal).length());
			};

			for (int y = 0; y < int(chunk.height); ++y)
			{
				updateEdgeError(0, 0, y, -1, y);
				updateEdgeError(1, int(chunk.width) - 1, y, int(chunk.width), y);
			}
			for (int x = 0; x < int(chunk.width); ++x)
			{
				updateEdgeError(2, x, 0, x, -1);
				updateEdgeError(3, x, int(chunk.height) - 1, x, int(chunk.height));
			}
		}

		float getLODThresholdScale(int lodLevel) const
		{
			static float const scales[kTerrainLODCount - 1] = { 1.5f, 3.0f, 6.0f };
			assert(lodLevel >= 0 && lodLevel < kTerrainLODCount - 1);
			return scales[lodLevel];
		}

		int selectLODTarget(TerrainRenderChunk const& chunk) const
		{
			float chunkWorldSize = getConfiguredChunkWorldSize(chunk.coord);
			float distance = (calcChunkCenterWorldPos(chunk) - mCamera.getPos()).length();
			if (distance < chunkWorldSize * getLODThresholdScale(0))
				return 0;
			if (distance < chunkWorldSize * getLODThresholdScale(1))
				return 1;
			if (distance < chunkWorldSize * getLODThresholdScale(2))
				return 2;
			return 3;
		}

		int updateChunkLOD(TerrainRenderChunk& chunk)
		{
			constexpr float kLODHysteresis = 0.15f;
			float chunkWorldSize = getConfiguredChunkWorldSize(chunk.coord);
			float distance = (calcChunkCenterWorldPos(chunk) - mCamera.getPos()).length();
			int lod = Math::Clamp<int>(chunk.currentLOD, 0, kTerrainLODCount - 1);

			while (lod > 0)
			{
				float enterThreshold = chunkWorldSize * getLODThresholdScale(lod - 1) * (1.0f - kLODHysteresis);
				if (distance < enterThreshold)
					--lod;
				else
					break;
			}

			while (lod < kTerrainLODCount - 1)
			{
				float exitThreshold = chunkWorldSize * getLODThresholdScale(lod) * (1.0f + kLODHysteresis);
				if (distance > exitThreshold)
					++lod;
				else
					break;
			}

			chunk.currentLOD = uint8(lod);
			return lod;
		}

		int getNeighborChunkLOD(TerrainChunkCoord const& coord) const
		{
			auto it = mRenderChunks.find(makeChunkCoordKey(coord));
			if (it == mRenderChunks.end() || !it->second.texture)
				return -1;
			return it->second.currentLOD;
		}

		int resolveRenderLOD(TerrainRenderChunk const& chunk) const
		{
			int lod = chunk.currentLOD;
			TerrainChunkCoord neighbors[] =
			{
				makeChunkCoord(chunk.coord.latIndex - 1, chunk.coord.lonIndex),
				makeChunkCoord(chunk.coord.latIndex + 1, chunk.coord.lonIndex),
				makeChunkCoord(chunk.coord.latIndex, chunk.coord.lonIndex - 1),
				makeChunkCoord(chunk.coord.latIndex, chunk.coord.lonIndex + 1),
			};

			for (TerrainChunkCoord const& neighborCoord : neighbors)
			{
				int neighborLOD = getNeighborChunkLOD(neighborCoord);
				if (neighborLOD >= 0)
					lod = Math::Max(lod, neighborLOD - 1);
			}
			return Math::Clamp(lod, 0, kTerrainLODCount - 1);
		}

		float calcLODBlendForDistance(int lod, float distance, float chunkWorldSize) const
		{
			if (lod <= 0 || lod >= kTerrainLODCount - 1)
				return 0.0f;

			float bandStart = (lod == 0) ? 0.0f : chunkWorldSize * getLODThresholdScale(lod - 1);
			float bandEnd = chunkWorldSize * getLODThresholdScale(lod);
			float blendRange = Math::Max(bandEnd - bandStart, 1e-3f);
			float t = Math::Clamp((distance - bandStart) / blendRange, 0.0f, 1.0f);
			return t * t * (3.0f - 2.0f * t);
		}

		Vector4 calcChunkLODBlend(TerrainRenderChunk const& chunk) const
		{
			int lod = chunk.renderLOD;
			float chunkWorldSize = getConfiguredChunkWorldSize(chunk.coord);
			Vector3 origin = chunk.localToWorld.getTranslation();
			Vector3 corners[] =
			{
				origin,
				origin + Vector3(chunk.scale.x, 0, 0),
				origin + Vector3(0, chunk.scale.y, 0),
				origin + Vector3(chunk.scale.x, chunk.scale.y, 0),
			};

			Vector3 const& cameraPos = mCamera.getPos();
			return Vector4(
				calcLODBlendForDistance(lod, (corners[0] - cameraPos).length(), chunkWorldSize),
				calcLODBlendForDistance(lod, (corners[1] - cameraPos).length(), chunkWorldSize),
				calcLODBlendForDistance(lod, (corners[2] - cameraPos).length(), chunkWorldSize),
				calcLODBlendForDistance(lod, (corners[3] - cameraPos).length(), chunkWorldSize)
			);
		}

		IntVector4 calcChunkStitchDelta(TerrainRenderChunk const& chunk) const
		{
			auto calcEdgeDelta = [&](TerrainChunkCoord const& neighborCoord)
			{
				auto it = mRenderChunks.find(makeChunkCoordKey(neighborCoord));
				if (it == mRenderChunks.end() || !it->second.texture)
					return 0;
				int delta = int(it->second.renderLOD) - int(chunk.renderLOD);
				return Math::Max(delta, 0);
			};

			return IntVector4(
				calcEdgeDelta(makeChunkCoord(chunk.coord.latIndex - 1, chunk.coord.lonIndex)),
				calcEdgeDelta(makeChunkCoord(chunk.coord.latIndex + 1, chunk.coord.lonIndex)),
				calcEdgeDelta(makeChunkCoord(chunk.coord.latIndex, chunk.coord.lonIndex - 1)),
				calcEdgeDelta(makeChunkCoord(chunk.coord.latIndex, chunk.coord.lonIndex + 1)));
		}

		bool buildPreparedChunkTextureData(TerrainChunkCoord const& coord, TerrainChunkCacheData const& data, TArray< uint8 >& outTextureData) const
		{
			TIME_SCOPE("Terrain.BuildPreparedChunkTextureData");
			if (!data.isValid())
				return false;

			double chunkDegreeSize = double(getConfiguredChunkDegreeSize());
			double latitudeRadians = Math::DegToRad(double(0.5f * (coord.south + coord.north)));
			double latMeters = 111132.0 * chunkDegreeSize;
			double lonMeters = 111320.0 * std::cos(latitudeRadians) * chunkDegreeSize;
			double maxMeters = Math::Max(latMeters, lonMeters);
			float worldChunkSize = getConfiguredChunkWorldSize(coord);
			Vector3 scale(
				(maxMeters > 0.0) ? worldChunkSize * float(lonMeters / maxMeters) : worldChunkSize,
				(maxMeters > 0.0) ? worldChunkSize * float(latMeters / maxMeters) : worldChunkSize,
				getConfiguredVerticalExaggeration() * Math::Max(data.maxHeight - data.minHeight, 1e-3f) * ((maxMeters > 0.0) ? float(worldChunkSize / maxMeters) : 0.1f * worldChunkSize));

			outTextureData.resize(data.width * data.height * 4);
			float dx = scale.x / float(data.width - 1);
			float dy = scale.y / float(data.height - 1);
			float heightWorldScale = (data.maxHeight > data.minHeight) ? (scale.z / (data.maxHeight - data.minHeight)) : 0.0f;
			for (uint32 y = 0; y < data.height; ++y)
			{
				for (uint32 x = 0; x < data.width; ++x)
				{
					uint32 index = x + y * data.width;
					uint16 h = data.heightData[index];
					float hL = decodeCachedHaloSampleHeight(data, int(x) - 1, int(y));
					float hR = decodeCachedHaloSampleHeight(data, int(x) + 1, int(y));
					float hD = decodeCachedHaloSampleHeight(data, int(x), int(y) - 1);
					float hU = decodeCachedHaloSampleHeight(data, int(x), int(y) + 1);
					Vector3 tangentX(2.0f * dx, 0, (hR - hL) * heightWorldScale);
					Vector3 tangentY(0, 2.0f * dy, (hU - hD) * heightWorldScale);
					Vector3 normal = Math::GetNormal(tangentX.cross(tangentY));

					uint8 nx = uint8(Math::Clamp(normal.x * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f + 0.5f);
					uint8 ny = uint8(Math::Clamp(normal.y * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f + 0.5f);
					outTextureData[index * 4 + 0] = uint8(h >> 8);
					outTextureData[index * 4 + 1] = uint8(h & 0xff);
					outTextureData[index * 4 + 2] = nx;
					outTextureData[index * 4 + 3] = ny;
				}
			}
			return true;
		}

		bool uploadChunkTexture(TerrainRenderChunk& chunk)
		{
			TIME_SCOPE("Terrain.UploadChunkTexture");
			if (chunk.textureData.empty())
				return false;

			RHITexture2DRef texture = RHICreateTexture2D(ETexture::RGBA8, int(chunk.width), int(chunk.height), 1, 1, TCF_DefalutValue, chunk.textureData.data(), 1);
			if (!texture)
			{
				LogWarning(0, "Failed to create terrain chunk texture");
				return false;
			}

			chunk.texture = texture;
			return true;
		}

		bool rebuildChunkTexture(TerrainRenderChunk& chunk)
		{
			TIME_SCOPE("Terrain.RebuildChunkTexture");
			if (chunk.heightData.empty())
				return false;

			chunk.textureData.resize(chunk.width * chunk.height * 4);
			for (uint32 y = 0; y < chunk.height; ++y)
			{
				for (uint32 x = 0; x < chunk.width; ++x)
				{
					uint32 index = x + y * chunk.width;
					uint16 h = chunk.heightData[index];
					Vector3 normal = calcChunkSampleNormal(chunk, int(x), int(y));

					uint8 nx = uint8(Math::Clamp(normal.x * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f + 0.5f);
					uint8 ny = uint8(Math::Clamp(normal.y * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f + 0.5f);
					chunk.textureData[index * 4 + 0] = uint8(h >> 8);
					chunk.textureData[index * 4 + 1] = uint8(h & 0xff);
					chunk.textureData[index * 4 + 2] = nx;
					chunk.textureData[index * 4 + 3] = ny;
				}
			}

			return uploadChunkTexture(chunk);
		}

		void updateChunkTextureSample(TerrainRenderChunk& chunk, int x, int y) const
		{
			if (x < 0 || x >= int(chunk.width) || y < 0 || y >= int(chunk.height))
				return;

			uint32 index = x + y * chunk.width;
			uint16 h = chunk.heightData[index];
			Vector3 normal = calcChunkSampleNormal(chunk, x, y);

			uint8 nx = uint8(Math::Clamp(normal.x * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f + 0.5f);
			uint8 ny = uint8(Math::Clamp(normal.y * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f + 0.5f);
			chunk.textureData[index * 4 + 0] = uint8(h >> 8);
			chunk.textureData[index * 4 + 1] = uint8(h & 0xff);
			chunk.textureData[index * 4 + 2] = nx;
			chunk.textureData[index * 4 + 3] = ny;
		}

		void rebuildChunkTextureBorders(TerrainRenderChunk& chunk, uint8 borderMask)
		{
			TIME_SCOPE("Terrain.RebuildChunkTextureBorders");
			if (chunk.heightData.empty() || chunk.textureData.size() != chunk.width * chunk.height * 4)
				return;

			auto updateXRange = [&](int xMin, int xMax)
			{
				xMin = Math::Clamp(xMin, 0, int(chunk.width) - 1);
				xMax = Math::Clamp(xMax, 0, int(chunk.width) - 1);
				for (int y = 0; y < int(chunk.height); ++y)
				{
					for (int x = xMin; x <= xMax; ++x)
					{
						updateChunkTextureSample(chunk, x, y);
					}
				}
			};
			auto updateYRange = [&](int yMin, int yMax)
			{
				yMin = Math::Clamp(yMin, 0, int(chunk.height) - 1);
				yMax = Math::Clamp(yMax, 0, int(chunk.height) - 1);
				for (int y = yMin; y <= yMax; ++y)
				{
					for (int x = 0; x < int(chunk.width); ++x)
					{
						updateChunkTextureSample(chunk, x, y);
					}
				}
			};

			if (borderMask & ChunkBorderMask_Left)
				updateXRange(0, 1);
			if (borderMask & ChunkBorderMask_Right)
				updateXRange(int(chunk.width) - 2, int(chunk.width) - 1);
			if (borderMask & ChunkBorderMask_Bottom)
				updateYRange(0, 1);
			if (borderMask & ChunkBorderMask_Top)
				updateYRange(int(chunk.height) - 2, int(chunk.height) - 1);
		}

		float fetchChunkSampleHeight(TerrainRenderChunk const& chunk, int x, int y) const
		{
			if (!chunk.haloHeightData.empty() &&
				-1 <= x && x <= int(chunk.width) &&
				-1 <= y && y <= int(chunk.height))
			{
				uint32 haloWidth = chunk.width + 2;
				uint16 value = chunk.haloHeightData[(x + 1) + (y + 1) * haloWidth];
				return decodeChunkSampleHeight(chunk, value);
			}

			if (0 <= x && x < int(chunk.width) && 0 <= y && y < int(chunk.height))
				return decodeChunkSampleHeight(chunk, chunk.heightData[x + y * chunk.width]);

			auto fetchNeighbor = [&](int latIndex, int lonIndex) -> TerrainRenderChunk const*
			{
				auto it = mRenderChunks.find(makeChunkCoordKey(TerrainChunkCoord{ latIndex, lonIndex }));
				if (it == mRenderChunks.end() || it->second.heightData.empty())
					return nullptr;
				return &it->second;
			};

			if (x < 0)
			{
				if (auto neighbor = fetchNeighbor(chunk.coord.latIndex, chunk.coord.lonIndex - 1))
					return decodeChunkSampleHeight(*neighbor, neighbor->heightData[(neighbor->width - 1 + x) + y * neighbor->width]);
				x = 0;
			}
			else if (x >= int(chunk.width))
			{
				if (auto neighbor = fetchNeighbor(chunk.coord.latIndex, chunk.coord.lonIndex + 1))
					return decodeChunkSampleHeight(*neighbor, neighbor->heightData[(x - (int(chunk.width) - 1)) + y * neighbor->width]);
				x = int(chunk.width) - 1;
			}

			if (y < 0)
			{
				if (auto neighbor = fetchNeighbor(chunk.coord.latIndex - 1, chunk.coord.lonIndex))
					return decodeChunkSampleHeight(*neighbor, neighbor->heightData[x + (neighbor->height - 1 + y) * neighbor->width]);
				y = 0;
			}
			else if (y >= int(chunk.height))
			{
				if (auto neighbor = fetchNeighbor(chunk.coord.latIndex + 1, chunk.coord.lonIndex))
					return decodeChunkSampleHeight(*neighbor, neighbor->heightData[x + (y - (int(chunk.height) - 1)) * neighbor->width]);
				y = int(chunk.height) - 1;
			}

			return decodeChunkSampleHeight(chunk, chunk.heightData[x + y * chunk.width]);
		}

		Vector3 calcChunkSampleNormal(TerrainRenderChunk const& chunk, int x, int y) const
		{
			float hL = fetchChunkSampleHeight(chunk, x - 1, y);
			float hR = fetchChunkSampleHeight(chunk, x + 1, y);
			float hD = fetchChunkSampleHeight(chunk, x, y - 1);
			float hU = fetchChunkSampleHeight(chunk, x, y + 1);
			float dx = chunk.scale.x / float(chunk.width - 1);
			float dy = chunk.scale.y / float(chunk.height - 1);
			Vector3 tangentX(2.0f * dx, 0, (hR - hL) * chunk.heightWorldScale);
			Vector3 tangentY(0, 2.0f * dy, (hU - hD) * chunk.heightWorldScale);
			return Math::GetNormal(tangentX.cross(tangentY));
		}

		float decodeChunkSampleHeight(TerrainRenderChunk const& chunk, uint16 value) const
		{
			if (chunk.maxHeight <= chunk.minHeight)
				return chunk.minHeight;

			float t = float(value) / 65535.0f;
			return Math::Lerp(chunk.minHeight, chunk.maxHeight, t);
		}

		uint16 encodeChunkSampleHeight(TerrainRenderChunk const& chunk, float value) const
		{
			float range = Math::Max(chunk.maxHeight - chunk.minHeight, 1e-6f);
			float t = Math::Clamp((value - chunk.minHeight) / range, 0.0f, 1.0f);
			return uint16(t * 65535.0f + 0.5f);
		}

		bool stitchChunkToNeighbor(TerrainRenderChunk& center, TerrainRenderChunk const& neighbor, uint8& centerBorderMask)
		{
			if (center.width == 0 || center.height == 0 || neighbor.width == 0 || neighbor.height == 0)
				return false;

			int dx = neighbor.coord.lonIndex - center.coord.lonIndex;
			int dy = neighbor.coord.latIndex - center.coord.latIndex;
			if (Math::Abs(dx) + Math::Abs(dy) != 1)
				return false;

			if (dx == 1)
			{
				uint32 cx = center.width - 1;
				uint32 nx = 0;
				uint32 count = Math::Min(center.height, neighbor.height);
				uint32 haloWidth = center.width + 2;
				for (uint32 y = 0; y < count; ++y)
				{
					uint16& cValue = center.heightData[cx + y * center.width];
					cValue = encodeChunkSampleHeight(center, decodeChunkSampleHeight(neighbor, neighbor.heightData[nx + y * neighbor.width]));
					center.haloHeightData[(cx + 1) + (y + 1) * haloWidth] = cValue;
				}
				centerBorderMask |= ChunkBorderMask_Right;
				return true;
			}
			if (dx == -1)
			{
				uint32 cx = 0;
				uint32 nx = neighbor.width - 1;
				uint32 count = Math::Min(center.height, neighbor.height);
				uint32 haloWidth = center.width + 2;
				for (uint32 y = 0; y < count; ++y)
				{
					uint16& cValue = center.heightData[cx + y * center.width];
					cValue = encodeChunkSampleHeight(center, decodeChunkSampleHeight(neighbor, neighbor.heightData[nx + y * neighbor.width]));
					center.haloHeightData[(cx + 1) + (y + 1) * haloWidth] = cValue;
				}
				centerBorderMask |= ChunkBorderMask_Left;
				return true;
			}
			if (dy == 1)
			{
				uint32 cy = center.height - 1;
				uint32 ny = 0;
				uint32 count = Math::Min(center.width, neighbor.width);
				uint32 haloWidth = center.width + 2;
				for (uint32 x = 0; x < count; ++x)
				{
					uint16& cValue = center.heightData[x + cy * center.width];
					cValue = encodeChunkSampleHeight(center, decodeChunkSampleHeight(neighbor, neighbor.heightData[x + ny * neighbor.width]));
					center.haloHeightData[(x + 1) + (cy + 1) * haloWidth] = cValue;
				}
				centerBorderMask |= ChunkBorderMask_Top;
				return true;
			}
			if (dy == -1)
			{
				uint32 cy = 0;
				uint32 ny = neighbor.height - 1;
				uint32 count = Math::Min(center.width, neighbor.width);
				uint32 haloWidth = center.width + 2;
				for (uint32 x = 0; x < count; ++x)
				{
					uint16& cValue = center.heightData[x + cy * center.width];
					cValue = encodeChunkSampleHeight(center, decodeChunkSampleHeight(neighbor, neighbor.heightData[x + ny * neighbor.width]));
					center.haloHeightData[(x + 1) + (cy + 1) * haloWidth] = cValue;
				}
				centerBorderMask |= ChunkBorderMask_Bottom;
				return true;
			}

			return false;
		}

		void stitchChunkBorders(int64 chunkKey)
		{
			auto itCenter = mRenderChunks.find(chunkKey);
			if (itCenter == mRenderChunks.end())
				return;

			TerrainChunkCoord const& coord = itCenter->second.coord;
			TerrainChunkCoord neighbors[] =
			{
				makeChunkCoord(coord.latIndex, coord.lonIndex - 1),
				makeChunkCoord(coord.latIndex, coord.lonIndex + 1),
				makeChunkCoord(coord.latIndex - 1, coord.lonIndex),
				makeChunkCoord(coord.latIndex + 1, coord.lonIndex),
			};

			uint8 centerBorderMask = ChunkBorderMask_None;
			for (TerrainChunkCoord const& neighborCoord : neighbors)
			{
				auto itNeighbor = mRenderChunks.find(makeChunkCoordKey(neighborCoord));
				if (itNeighbor == mRenderChunks.end())
					continue;

				stitchChunkToNeighbor(itCenter->second, itNeighbor->second, centerBorderMask);
			}

			if (centerBorderMask != ChunkBorderMask_None)
			{
				rebuildChunkTextureBorders(itCenter->second, centerBorderMask);
			}
		}

		TerrainProgram* mProgTerrain = nullptr;
		Mesh mTerrainMeshes[kTerrainLODCount];
		PrimitivesCollection mDebugPrimitives;
		RHITexture2DRef mTexTerrainBaseColor;
		TerrainChunkCoord mCenterChunkCoord;
		std::unordered_map< int64, TerrainRenderChunk > mRenderChunks;
		TArray< TerrainChunkResult > mPendingChunkResults;
		Mutex mChunkMutex;
		std::unordered_set< int64 > mPendingChunkKeys;
		std::unordered_map< int64, PendingTerrainChunkRaw > mPendingChunkRawData;
		std::unordered_map< int64, uint64 > mChunkTaskOrder;
		std::unordered_set< int64 > mChunkFinalizeQueued;
		std::unordered_set< int64 > mChunkTasksInFlight;
		std::atomic< uint64 > mChunkTaskOrderCounter = 0;
		std::atomic< uint32 > mChunkRequestGeneration = 0;
		TerrainProviderService mProviderService;
		QueueThreadPool mChunkWorkPool;
		QueueThreadPool mChunkFinalizeWorkPool;
		float mTerrainBaseHeight = 0;
		bool mbHasTerrainBaseHeight = false;
		bool mbWireframe = false;
		bool mbUseLOD = true;
	};


	REGISTER_STAGE_ENTRY("Terrain Rendering", TerrainRenderingStage, EExecGroup::FeatureDev, 1, "Render|Test");
}
