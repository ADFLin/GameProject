#include "Font.h"

#include "PlatformConfig.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#include "BitmapDC.h"
#include "WinGDIRenderSystem.h"
#endif //SYS_PLATFORM_WIN

#include "FileSystem.h"
#include "Core/ScopeGuard.h"
#include "Rect.h"
#include "ProfileSystem.h"
#include "Renderer/TrueType.h"
#include "Core/FNV1a.h"

#include <cwchar>
#include <memory>
#include <unordered_map>

template< class FormCharT, class ToCharT >
struct FCharConv
{
};

template< class CharT >
struct FCharConv< CharT, CharT >
{
	static int Do(CharT const* c, CharT* outConv) { *outConv = *c; return 1; }
};
template<>
struct FCharConv< char, wchar_t >
{
	static int Do(char const* c, wchar_t* outConv)
	{
		std::mbstate_t state{};
		return std::mbrtowc(outConv, c, MB_CUR_MAX, &state);
	}
};

namespace Render
{
#if SYS_PLATFORM_WIN
	class GDIFontCharDataProvider : public ICharDataProvider
	{
	public:
		~GDIFontCharDataProvider()
		{
			finalize();
		}

		bool initialize(FontFaceInfo const& fontFace, HFONT inFont, bool bTakeFontOwnership);
		void finalize();

		static void CopyImage(uint8* dest, int w, int h, int pixeSize, uint8* src, int pixelStride)
		{
			int copySize = pixeSize * w;
			int dataStride = pixelStride * pixeSize;
			for( int i = 0; i < h; ++i )
			{
				std::copy(src, src + copySize, dest);
				src += dataStride;
				dest += copySize;
			}
		}

		struct RenderGlyphResult
		{
			ABCFLOAT abcFloat;
			int width = 1;
			int height = 1;
			int offsetY = 0;
			bool bHasImage = false;
		};

		bool renderGlyph(wchar_t charW, RenderGlyphResult& outResult);
		bool getCharData(uint32 charWord, CharImageData& outData) override;
		bool getCharDesc(uint32 charWord, CharImageDesc& outDesc) override;

		int      getFontHeight() const override { return textureDC.getHeight(); }
		int      mSize;
		int      mAscent = 0;
		BitmapDC textureDC;
		uint8*   pDataTexture = nullptr;
		HFONT    hFont = NULL;
		bool     bOwnFont = false;

		void getKerningPairs(std::unordered_map<uint32, float>& outKerningMap) const override;

	};

	class GDITrueTypeLoader : public ITrueTypeLoader
	{
	public:
		bool initialize(HFONT inFont)
		{
			hFont = inFont;
			HDC hDC = ::GetDC(NULL);
			ON_SCOPE_EXIT
			{
				::ReleaseDC(NULL, hDC);
			};

			HGDIOBJ hOldFont = ::SelectObject(hDC, hFont);
			TEXTMETRIC tm;
			::GetTextMetrics(hDC, &tm);
			::SelectObject(hDC, hOldFont);

			mFontMetrics.unitsPerEm = uint16(Math::Max<int>(int(tm.tmHeight), 1));
			mFontMetrics.ascender = int16(tm.tmAscent);
			mFontMetrics.descender = int16(tm.tmDescent);
			mFontMetrics.lineGap = int16(Math::Max<int>(0, int(tm.tmHeight - tm.tmAscent - tm.tmDescent)));

			int num = GetKerningPairsW(hDC, 0, nullptr);
			if (num > 0)
			{
				TArray<KERNINGPAIR> pairs;
				pairs.resize(num);
				GetKerningPairsW(hDC, num, pairs.data());
				mKerningPairs.clear();
				for (auto const& pair : pairs)
				{
					if (pair.iKernAmount == 0)
						continue;
					mKerningPairs.push_back({ uint32(pair.wFirst), uint32(pair.wSecond), int16(pair.iKernAmount) });
				}
			}
			return true;
		}

		FontMetrics const& getFontMetrics() const override { return mFontMetrics; }
		TArray< KerningPair > const& getKerningPairs() const override { return mKerningPairs; }

		bool loadGlyph(uint32 codepoint, GlyphData& outGlyph) const override
		{
			if (!hFont)
				return false;

			HDC hDC = ::GetDC(NULL);
			ON_SCOPE_EXIT
			{
				::ReleaseDC(NULL, hDC);
			};

			HGDIOBJ hOldFont = ::SelectObject(hDC, hFont);
			ON_SCOPE_EXIT
			{
				::SelectObject(hDC, hOldFont);
			};

			wchar_t charW = wchar_t(codepoint);
			GLYPHMETRICS gm;
			MAT2 mat = {};
			mat.eM11.value = 1;
			mat.eM22.value = 1;

			outGlyph = GlyphData();
			outGlyph.codepoint = codepoint;
			ABCFLOAT abcFloat;
			if (::GetCharABCWidthsFloatW(hDC, charW, charW, &abcFloat))
			{
				outGlyph.advanceWidth = uint16(Math::Max(0, Math::RoundToInt(abcFloat.abcfA + abcFloat.abcfB + abcFloat.abcfC)));
				outGlyph.leftSideBearing = int16(Math::RoundToInt(abcFloat.abcfA));
				outGlyph.rightSideBearing = int16(Math::RoundToInt(abcFloat.abcfC));
			}

			if (::GetGlyphOutlineW(hDC, charW, GGO_METRICS, &gm, 0, nullptr, &mat) == GDI_ERROR)
				return false;

			outGlyph.advanceWidth = uint16(gm.gmCellIncX);
			outGlyph.leftSideBearing = int16(gm.gmptGlyphOrigin.x);
			outGlyph.rightSideBearing = int16(gm.gmCellIncX - gm.gmptGlyphOrigin.x - gm.gmBlackBoxX);

			DWORD bufferSize = ::GetGlyphOutlineW(hDC, charW, GGO_NATIVE, &gm, 0, nullptr, &mat);
			if (bufferSize == GDI_ERROR)
				return false;
			if (bufferSize == 0)
				return true;

			TArray<uint8> buffer;
			buffer.resize(bufferSize);
			if (::GetGlyphOutlineW(hDC, charW, GGO_NATIVE, &gm, bufferSize, buffer.data(), &mat) == GDI_ERROR)
				return false;

			auto ToFloat = [](FIXED const& value)
			{
				return float(value.value) + float(value.fract) / 65536.0f;
			};
			auto ToVector = [&](POINTFX const& value)
			{
				return Vector2(ToFloat(value.x), ToFloat(value.y));
			};
			auto AddLine = [&](Vector2 const& start, Vector2 const& end)
			{
				if ((end - start).length2() < 1e-6f)
					return;
				CurveSegment segment;
				segment.start = start;
				segment.control = 0.5f * (start + end);
				segment.end = end;
				outGlyph.segments.push_back(segment);
			};
			auto AddQuad = [&](Vector2 const& start, Vector2 const& control, Vector2 const& end)
			{
				if ((end - start).length2() < 1e-6f)
					return;
				CurveSegment segment;
				segment.start = start;
				segment.control = control;
				segment.end = end;
				outGlyph.segments.push_back(segment);
			};

			size_t pos = 0;
			while (pos + sizeof(TTPOLYGONHEADER) <= buffer.size())
			{
				auto* header = reinterpret_cast<TTPOLYGONHEADER const*>(buffer.data() + pos);
				size_t polyEnd = pos + header->cb;
				if (polyEnd > buffer.size() || header->cb < sizeof(TTPOLYGONHEADER))
					return false;

				Vector2 contourStart = ToVector(header->pfxStart);
				Vector2 current = contourStart;
				size_t curvePos = pos + sizeof(TTPOLYGONHEADER);
				int segmentOffset = int(outGlyph.segments.size());
				while (curvePos + sizeof(TTPOLYCURVE) <= polyEnd)
				{
					auto* curve = reinterpret_cast<TTPOLYCURVE const*>(buffer.data() + curvePos);
					size_t curveSize = sizeof(TTPOLYCURVE) + sizeof(POINTFX) * (curve->cpfx - 1);
					if (curve->cpfx == 0 || curvePos + curveSize > polyEnd)
						return false;

					if (curve->wType == TT_PRIM_LINE)
					{
						for (uint32 i = 0; i < curve->cpfx; ++i)
						{
							Vector2 next = ToVector(curve->apfx[i]);
							AddLine(current, next);
							current = next;
						}
					}
					else if (curve->wType == TT_PRIM_QSPLINE)
					{
						for (uint32 i = 0; i + 1 < curve->cpfx; ++i)
						{
							Vector2 control = ToVector(curve->apfx[i]);
							Vector2 end = (i + 2 < curve->cpfx) ? 0.5f * (control + ToVector(curve->apfx[i + 1])) : ToVector(curve->apfx[i + 1]);
							AddQuad(current, control, end);
							current = end;
						}
					}

					curvePos += curveSize;
				}

				AddLine(current, contourStart);
				outGlyph.contours.push_back({ segmentOffset, int(outGlyph.segments.size()) - segmentOffset });
				pos = polyEnd;
			}

			outGlyph.bHasOutline = !outGlyph.segments.empty();
			if (outGlyph.bHasOutline)
			{
				outGlyph.bound.invalidate();
				for (auto const& segment : outGlyph.segments)
				{
					outGlyph.bound.addPoint(segment.start);
					outGlyph.bound.addPoint(segment.control);
					outGlyph.bound.addPoint(segment.end);
				}
			}
			return true;
		}

		bool hintGlyph(GlyphData& ioGlyph, float pixelSize) const override
		{
			return true;
		}

	private:
		HFONT hFont = NULL;
		FontMetrics mFontMetrics;
		TArray< KerningPair > mKerningPairs;
	};

	bool GDIFontCharDataProvider::initialize(FontFaceInfo const& fontFace, HFONT inFont, bool bTakeFontOwnership)
	{
		HDC hDC = ::GetDC(NULL);
		ON_SCOPE_EXIT
		{
			::ReleaseDC(NULL, hDC);
		};

		hFont = inFont;
		bOwnFont = bTakeFontOwnership;

		TEXTMETRIC tm;
		HGDIOBJ hOldFont = ::SelectObject(hDC, hFont);
		::GetTextMetrics(hDC, &tm);
		::SelectObject(hDC, hOldFont);

		BITMAPINFO& bmpInfo = *(BITMAPINFO*)(alloca(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 0));
		bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
		bmpInfo.bmiHeader.biWidth = tm.tmMaxCharWidth;
		bmpInfo.bmiHeader.biHeight = -tm.tmHeight;
		bmpInfo.bmiHeader.biPlanes = 1;
		bmpInfo.bmiHeader.biBitCount = 32;
		bmpInfo.bmiHeader.biCompression = BI_RGB;
		bmpInfo.bmiHeader.biXPelsPerMeter = 0;
		bmpInfo.bmiHeader.biYPelsPerMeter = 0;
		bmpInfo.bmiHeader.biSizeImage = 0;
		if( !textureDC.initialize(hDC, &bmpInfo, (void**)&pDataTexture) )
			return false;


		::SelectObject(textureDC, GetStockObject(BLACK_PEN));
		::SelectObject(textureDC, GetStockObject(BLACK_BRUSH));
		::SetBkMode(textureDC, TRANSPARENT);
		::SelectObject(textureDC, hFont);
		::SetTextColor(textureDC, RGB(255, 255, 255));
		mSize = fontFace.size;
		mAscent = tm.tmAscent;
		return true;
	}

	void GDIFontCharDataProvider::finalize()
	{
		textureDC.release();
		if (bOwnFont && hFont)
		{
			::DeleteObject(hFont);
		}
		hFont = NULL;
		bOwnFont = false;
	}

	bool GDIFontCharDataProvider::renderGlyph(wchar_t charW, RenderGlyphResult& outResult)
	{
		if (!::GetCharABCWidthsFloatW(textureDC, charW, charW, &outResult.abcFloat))
			return false;

		outResult.width = Math::Max(1, Math::Min((int)outResult.abcFloat.abcfB, textureDC.getWidth()));
		outResult.offsetY = 0;
		outResult.height = 1;
		outResult.bHasImage = false;

		GLYPHMETRICS glyphMetrics;
		MAT2 mat = {};
		mat.eM11.value = 1;
		mat.eM22.value = 1;
		if (::GetGlyphOutlineW(textureDC, charW, GGO_METRICS, &glyphMetrics, 0, nullptr, &mat) != GDI_ERROR)
		{
			outResult.bHasImage = glyphMetrics.gmBlackBoxX > 0 && glyphMetrics.gmBlackBoxY > 0;
			if (outResult.bHasImage)
			{
				int offsetY = mAscent - glyphMetrics.gmptGlyphOrigin.y;
				offsetY = Math::Clamp(offsetY, 0, Math::Max(0, textureDC.getHeight() - 1));
				outResult.offsetY = offsetY;
				outResult.height = Math::Max(1, Math::Min(int(glyphMetrics.gmBlackBoxY), textureDC.getHeight() - offsetY));
			}
		}

		::Rectangle(textureDC, 0, 0, textureDC.getWidth(), textureDC.getHeight());
		RECT rect;
		rect.left = -(int)outResult.abcFloat.abcfA;
		rect.right = (int)outResult.abcFloat.abcfB;
		rect.top = 0;
		rect.bottom = textureDC.getHeight();
		::DrawTextW(textureDC, &charW, 1, &rect, DT_LEFT | DT_TOP);

		return true;
	}

	bool GDIFontCharDataProvider::getCharData(uint32 charWord, CharImageData& outData)
	{
		wchar_t charW = charWord;
#if 0
		WORD index = 0;
		int numConv = ::GetGlyphIndicesW(textureDC, &charW, 1, &index, GGI_MARK_NONEXISTING_GLYPHS);
		if( numConv == GDI_ERROR || index == 0xffff )
		{
			int i = 1;
		}
#endif

		RenderGlyphResult glyphResult;
		if (!renderGlyph(charW, glyphResult))
			return false;

		outData.width = glyphResult.width;
		outData.height = glyphResult.height;
		outData.kerning = glyphResult.abcFloat.abcfA;
		outData.offsetX = 0;
		outData.offsetY = float(glyphResult.offsetY);
		outData.advance = glyphResult.abcFloat.abcfB + glyphResult.abcFloat.abcfC;
		outData.imageWidth = outData.width;
		outData.imageHeight = outData.height;
		outData.pixelSize = 4;
		outData.imageData.resize(outData.imageWidth * outData.imageHeight * outData.pixelSize, 0);
		if (glyphResult.bHasImage)
		{
			uint8* src = pDataTexture + glyphResult.offsetY * textureDC.getWidth() * outData.pixelSize;
			CopyImage(&outData.imageData[0], outData.imageWidth, outData.imageHeight, outData.pixelSize, src, textureDC.getWidth());
		}

		if (GRHISystem->getName() == RHISystemName::D3D11 || 
			GRHISystem->getName() == RHISystemName::D3D12 ||
			GRHISystem->getName() == RHISystemName::Vulkan)
		{
			uint8* pData = outData.imageData.data();
			int count = outData.imageData.size() / 4;
			for (int i = count; i ; --i )
			{
				pData[3] = pData[0];
				pData += 4;
			}
		}

		return true;
	}

	bool GDIFontCharDataProvider::getCharDesc(uint32 charWord, CharImageDesc& outDesc)
	{
		wchar_t charW = charWord;
		RenderGlyphResult glyphResult;
		if (!renderGlyph(charW, glyphResult))
			return false;

		outDesc.width = glyphResult.width;
		outDesc.height = glyphResult.height;
		outDesc.kerning = glyphResult.abcFloat.abcfA;
		outDesc.offsetX = 0;
		outDesc.offsetY = float(glyphResult.offsetY);
		outDesc.advance = glyphResult.abcFloat.abcfB + glyphResult.abcFloat.abcfC;
		return true;
	}

	void GDIFontCharDataProvider::getKerningPairs(std::unordered_map<uint32, float>& outKerningMap) const
	{
		int num = GetKerningPairsW(textureDC.getHandle(), 0, nullptr);
		if (num > 0)
		{
			TArray<KERNINGPAIR> pairs;
			pairs.resize(num);
			GetKerningPairsW(textureDC.getHandle(), num, pairs.data());

			for (auto& pair : pairs)
			{
				if ( pair.iKernAmount == 0)
					continue;

				uint32 key = Math::PairingFunction(uint32(pair.wFirst), uint32(pair.wSecond));
				outKerningMap[key] = float(pair.iKernAmount);
			}
		}
	}

	struct TrueTypeLoaderKey
	{
		uint64 hash = 0;
		size_t size = 0;

		bool operator == (TrueTypeLoaderKey const& rhs) const
		{
			return hash == rhs.hash && size == rhs.size;
		}
	};

	struct TrueTypeLoaderKeyHasher
	{
		size_t operator()(TrueTypeLoaderKey const& key) const
		{
			return size_t(key.hash ^ (key.hash >> 32) ^ key.size);
		}
	};

	class TrueTypeLoaderManager
	{
	public:
		using LoaderPtr = std::shared_ptr< ITrueTypeLoader >;

		static TrueTypeLoaderManager& Get()
		{
			static TrueTypeLoaderManager sInstance;
			return sInstance;
		}

		LoaderPtr getOrCreate(TArrayView< uint8 const > fontData, HFONT hFont)
		{
			Mutex::Locker locker(mMutex);
			TrueTypeLoaderKey key{ FNV1a::MakeHash<uint64>( fontData.data(), fontData.size()), fontData.size() };
			auto iter = mLoaders.find(key);
			if (iter != mLoaders.end())
			{
				LoaderPtr loader = iter->second.lock();
				if (loader)
					return loader;

				mLoaders.erase(iter);
			}

			LoaderPtr loader;
			if (mUseGDILoaderImpl)
			{
				auto gdiLoader = std::make_shared< GDITrueTypeLoader >();
				if (!gdiLoader->initialize(hFont))
					return nullptr;
				loader = gdiLoader;
			}
			else
			{
				auto ttLoader = std::make_shared< TrueTypeFileLoader >();
				if (!ttLoader->loadFromBuffer(fontData))
					return nullptr;
				loader = ttLoader;
			}

			mLoaders.emplace(key, loader);
			cleanupExpiredLoaders();
			return loader;
		}

	private:
		void cleanupExpiredLoaders()
		{
			for (auto iter = mLoaders.begin(); iter != mLoaders.end(); )
			{
				if (iter->second.expired())
				{
					iter = mLoaders.erase(iter);
				}
				else
				{
					++iter;
				}
			}
		}

		std::unordered_map< TrueTypeLoaderKey, std::weak_ptr< ITrueTypeLoader >, TrueTypeLoaderKeyHasher > mLoaders;
		Mutex mMutex;
		bool mUseGDILoaderImpl = true;
	};

	class TrueTypeWithGDIFallbackCharDataProvider : public ICharDataProvider
	{
	public:
		~TrueTypeWithGDIFallbackCharDataProvider()
		{
			mGDIProvider.finalize();
			if (mFont)
			{
				::DeleteObject(mFont);
				mFont = NULL;
			}
		}

		bool initialize(FontFaceInfo const& fontFace, HFONT font, TArrayView< uint8 const > fontData, int pixelSize, int fontHeight, int baseline)
		{
			mFont = font;
			mLoader = TrueTypeLoaderManager::Get().getOrCreate(fontData, mFont);
			if (!mLoader)
				return false;

			if (!mTrueTypeProvider.initialize(fontFace.name, *mLoader, pixelSize, fontHeight, baseline))
				return false;

			return mGDIProvider.initialize(fontFace, mFont, false);
		}

		bool getCharData(uint32 charWord, CharImageData& outData) override
		{
			if (mTrueTypeProvider.getCharData(charWord, outData))
				return true;

			return mGDIProvider.getCharData(charWord, outData);
		}

		bool getCharDesc(uint32 charWord, CharImageDesc& outDesc) override
		{
			if (mTrueTypeProvider.getCharDesc(charWord, outDesc))
				return true;

			return mGDIProvider.getCharDesc(charWord, outDesc);
		}

		int getFontHeight() const override
		{
			return mGDIProvider.getFontHeight();
		}

		void getKerningPairs(std::unordered_map< uint32, float >& outKerningMap) const override
		{
			mTrueTypeProvider.getKerningPairs(outKerningMap);
			std::unordered_map< uint32, float > fallbackKerningMap;
			mGDIProvider.getKerningPairs(fallbackKerningMap);
			for (auto const& pair : fallbackKerningMap)
			{
				outKerningMap.emplace(pair.first, pair.second);
			}
		}

	private:
		HFONT mFont = NULL;
		TrueTypeLoaderManager::LoaderPtr mLoader;
		TrueTypeCharDataProvider mTrueTypeProvider;
		GDIFontCharDataProvider mGDIProvider;
	};

	bool IsSupportedByTrueTypeLoader(HDC hdc)
	{
		DWORD size = GetFontData(hdc, 0, 0, nullptr, 0);
		if (size == GDI_ERROR || size < 4)
			return false;

		uint8 header[4];
		if (GetFontData(hdc, 0, 0, header, 4) == GDI_ERROR)
			return false;

		uint32 tag =
			(uint32(header[0]) << 24) |
			(uint32(header[1]) << 16) |
			(uint32(header[2]) << 8) |
			uint32(header[3]);

		return tag == 0x00010000 || tag == MAKE_MAGIC_ID('t', 'r', 'u', 'e');
	}

#endif //SYS_PLATFORM_WIN


	ICharDataProvider* ICharDataProvider::Create(FontFaceInfo const& fontFace)
	{
#if SYS_PLATFORM_WIN
		HDC hDC = ::GetDC(NULL);
		ON_SCOPE_EXIT
		{
			::ReleaseDC(NULL, hDC);
		};

		HFONT hFont = FWinGDI::CreateFont(hDC, fontFace.name.c_str(), fontFace.size, fontFace.bBold, false, fontFace.bUnderLine);
		if (hFont == NULL)
		{
			return nullptr;
		}

		HGDIOBJ hOldFont = ::SelectObject(hDC, hFont);
		int pixelSize = MulDiv(fontFace.size, GetDeviceCaps(hDC, LOGPIXELSY), 72);
		TEXTMETRIC tm;
		::GetTextMetrics(hDC, &tm);

		if (IsSupportedByTrueTypeLoader(hDC) && false)
		{
			DWORD size = GetFontData(hDC, 0, 0, nullptr, 0);
			TArray<uint8> data;
			data.resize(size);
			GetFontData(hDC, 0, 0, data.data(), size);

			::SelectObject(hDC, hOldFont);
			TrueTypeWithGDIFallbackCharDataProvider* result = new TrueTypeWithGDIFallbackCharDataProvider;
			if (result->initialize(fontFace, hFont, data, pixelSize, tm.tmHeight, tm.tmAscent))
			{
				return result;
			}

			delete result;
			hOldFont = ::SelectObject(hDC, hFont);
		}

		::SelectObject(hDC, hOldFont);

		GDIFontCharDataProvider* result = new GDIFontCharDataProvider;
		if( !result->initialize(fontFace, hFont, true) )
		{
			delete result;
			return nullptr;
		}
		return result;
#else
#error No Impl!
#endif
	}

#if CORE_SHARE_CODE
	FontCharCache::FontCharCache()
		:IGlobalRenderResource()
	{
	}
	FontCharCache& FontCharCache::Get()
	{
		static FontCharCache sInstance;
		return sInstance;
	}


	bool FontCharCache::initialize()
	{
		return true;
	}

	void FontCharCache::finalize()
	{
		releaseRHI();
		for( auto& pair : mCharDataSetMap )
		{
			delete pair.second;
		}
		mCharDataSetMap.clear();
	}

	void FontCharCache::releaseRHI()
	{
		mTextAtlas.finalize();
		for( auto& pair : mCharDataSetMap )
		{
			pair.second->clearRHIResource();
		}
	}

	void FontCharCache::restoreRHI()
	{
		TRACE_RESOURCE_TAG("FontCharCache");
		if (!mTextAtlas.initialize(ETexture::RGBA8, 1024, 1024, 1))
		{
			return;
		}

		if (GRHISystem->getName() == RHISystemName::OpenGL)
		{
			GL_SCOPED_BIND_OBJECT(mTextAtlas.getTexture());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		}

		for (auto& pair : mCharDataSetMap)
		{
			pair.second->mUsedTextAtlas = &mTextAtlas;
		}
	}

	CharDataSet* FontCharCache::getCharDataSet(FontFaceInfo const& fontFace)
	{
		auto iter = mCharDataSetMap.find(fontFace);
		if( iter == mCharDataSetMap.end() )
		{
			CharDataSet* dataSet = new CharDataSet;
			if( !dataSet->initialize(fontFace) )
			{
				LogWarning(0, "Can't Init Char Data Set");
				delete dataSet;
				return nullptr;
			}
			dataSet->mUsedTextAtlas = &mTextAtlas;
			mCharDataSetMap.insert({ fontFace , dataSet });
			return dataSet;
		}
		return iter->second;
	}

	
	CharDataSet* FontCharCache::getCharDataSet(ICustomCharDataProvider& provider)
	{
		FontFaceInfo fontFace{ provider.getName(), provider.getFontHeight(), false };

		auto iter = mCharDataSetMap.find(fontFace);
		if (iter == mCharDataSetMap.end())
		{
			CharDataSet* dataSet = new CharDataSet;
			if (!dataSet->initialize(provider))
			{
				LogWarning(0, "Can't Init Char Data Set");
				delete dataSet;
				return nullptr;
			}
			dataSet->mUsedTextAtlas = &mTextAtlas;
			mCharDataSetMap.insert({ fontFace , dataSet });
			return dataSet;
		}
		return iter->second;
	}

	void FontCharCache::removeCharDataSet(ICustomCharDataProvider& provider)
	{
		FontFaceInfo fontFace{ provider.getName(), provider.getFontHeight(), false };
		auto iter = mCharDataSetMap.find(fontFace);
		if (iter != mCharDataSetMap.end())
		{
			delete iter->second;
			mCharDataSetMap.erase(iter);
		}
	}

#endif //CORE_SHARE_CODE

	CharDataSet::~CharDataSet()
	{
		clearRHIResource();
		if (bOwnProvider)
		{
			delete mProvider;
		}
		mProvider = nullptr;
		bOwnProvider = false;
	}

	bool CharDataSet::initialize(FontFaceInfo const& font)
	{
		mProvider = ICharDataProvider::Create(font);
		if( mProvider == nullptr )
			return false;

		bOwnProvider = true;
		mFontHeight = mProvider->getFontHeight();
		mProvider->getKerningPairs(mKerningPairMap);
		return true;
	}

	bool CharDataSet::initialize(ICustomCharDataProvider& provider)
	{
		mProvider = &provider;
		bOwnProvider = false;
		mFontHeight = mProvider->getFontHeight();
		mProvider->getKerningPairs(mKerningPairMap);
		return true;
	}

	void CharDataSet::clearRHIResource()
	{
		mCharMap.clear();
		mUsedTextAtlas = nullptr;
	}

	CharDataSet::CharData const& CharDataSet::findOrAddChar(uint32 charWord)
	{
		{
			RWLock::ReadLocker locker(mLock);
			auto iter = mCharMap.find(charWord);
			if (iter != mCharMap.end() && iter->second.atlasId != INDEX_NONE)
				return iter->second;
		}

		RWLock::ReadLocker locker(mLock);
		CharImageData imageData;
		mProvider->getCharData(charWord, imageData);

		CharData& charData = mCharMap[charWord];
		charData.width = imageData.width;
		charData.height = imageData.height;
		if( imageData.width != imageData.imageWidth )
		{
			charData.atlasId = mUsedTextAtlas->addImage(imageData.width, imageData.height, ETexture::RGBA8, &imageData.imageData[0], imageData.imageWidth);
		}
		else
		{
			charData.atlasId = mUsedTextAtlas->addImage(imageData.width, imageData.height, ETexture::RGBA8, &imageData.imageData[0]);
		}

		if (charData.atlasId != -1)
		{
			mUsedTextAtlas->getRectUV(charData.atlasId, charData.uvMin, charData.uvMax);
		}

		charData.advance = imageData.advance;
		charData.kerning = imageData.kerning;
		charData.offsetX = imageData.offsetX;
		charData.offsetY = imageData.offsetY;
		return charData;
	}

	CharDataSet::CharDesc const& CharDataSet::getCharDesc(uint32 charWord)
	{
		{
			RWLock::ReadLocker locker(mLock);
			auto iter = mCharMap.find(charWord);
			if (iter != mCharMap.end())
				return iter->second;
		}


		RWLock::WriteLocker locker(mLock);
		CharData& charData = mCharMap[charWord];
		CharImageDesc imageDesc;
		mProvider->getCharDesc(charWord, imageDesc);
		charData.atlasId = INDEX_NONE;
		charData.width = imageDesc.width;
		charData.height = imageDesc.height;
		charData.kerning = imageDesc.kerning;
		charData.offsetX = imageDesc.offsetX;
		charData.offsetY = imageDesc.offsetY;
		charData.advance = imageDesc.advance;

		return charData;
	}

	FontDrawer::FontDrawer()
	{
	}


	FontDrawer::~FontDrawer()
	{
		cleanup();
	}

	void FontDrawer::draw(RHICommandList& commandList, Vector2 const& pos, Matrix4 const& transform, LinearColor const& color, char const* str)
	{
		assert(isValid());
		if( str == nullptr )
			return;

		std::wstring text = FCString::CharToWChar(str);
		drawImpl(commandList, pos, transform, color, text.c_str());
	}

	void FontDrawer::draw(RHICommandList& commandList, Vector2 const& pos, Matrix4 const& transform, LinearColor const& color, wchar_t const* str)
	{
		assert(isValid());
		if( str == nullptr )
			return;
		drawImpl(commandList, pos, transform, color, str);
	}

	template< typename CharT >
	Vector2 FontDrawer::calcTextExtentT(CharT const* str, int* outCharCount)
	{
		CHECK(isValid());

		if (str == nullptr || *str == 0)
		{
			if (outCharCount)
				*outCharCount = 0;
			return Vector2::Zero();
		}

		Vector2 result = Vector2::Zero();

		int count = 0;
		wchar_t prevChar = 0;

		float xOffset = 0.0f;
		float lineMaxHeight = 0.0f;

		bool bApplyKerning = false;
		while( *str != 0 )
		{
			wchar_t c;
			str += FCharConv< CharT, wchar_t >::Do(str, &c);

			if( c == L'\n' )
			{
				result.x = Math::Max(result.x, xOffset);
				result.y += lineMaxHeight + 2;

				xOffset = 0;
				lineMaxHeight = 0;
				bApplyKerning = false;
				continue;
			}

			CharDataSet::CharData const& data = mCharDataSet->findOrAddChar(c);

			if (FCString::IsSpace(c))
			{
				xOffset += data.advance;
				bApplyKerning = false;
			}
			else
			{
				++count;
				if (bApplyKerning)
				{
					xOffset += data.kerning;
#if 1
					float kerning;
					if (mCharDataSet->getKerningPair(prevChar, c, kerning))
					{
						xOffset += kerning;
					}
#endif
				}
				result.x = Math::Max(result.x, xOffset + data.offsetX + data.width);
				lineMaxHeight = Math::Max(lineMaxHeight, data.offsetY + data.height);
				xOffset += data.advance;
				bApplyKerning = !FCString::IsSpace(c);
				prevChar = c;
			}
		}

		if (outCharCount)
		{
			*outCharCount = count;
		}

		result.x = Math::Max(result.x, xOffset);
		result.y += lineMaxHeight;
		return result;
	}

	template< typename CharT >
	Vector2 FontDrawer::calcTextExtentT(TStringView<CharT> str, int* outCharCount)
	{
		CHECK(isValid());

		if (str.length() == 0)
		{
			if (outCharCount)
				*outCharCount = 0;
			return Vector2::Zero();
		}

		Vector2 result = Vector2::Zero();

		int count = 0;
		wchar_t prevChar = 0;

		float xOffset = 0.0f;
		float lineMaxHeight = 0.0f;

		bool bApplyKerning = false;
		CharT const* pStr = str.data();
		while (pStr < str.end())
		{
			wchar_t c;
			pStr += FCharConv< CharT, wchar_t >::Do(pStr, &c);

			if (c == L'\n')
			{
				result.x = Math::Max(result.x, xOffset);
				result.y += lineMaxHeight + 2;

				xOffset = 0;
				lineMaxHeight = 0;
				bApplyKerning = false;
				continue;
			}


			CharDataSet::CharData const& data = mCharDataSet->findOrAddChar(c);

			if (FCString::IsSpace(c))
			{
				xOffset += data.advance;
				bApplyKerning = false;
			}
			else
			{
				++count;
				if (bApplyKerning)
				{
					xOffset += data.kerning;
#if 1
					float kerning;
					if (mCharDataSet->getKerningPair(prevChar, c, kerning))
					{
						xOffset += kerning;
					}
#endif
				}
				result.x = Math::Max(result.x, xOffset + data.offsetX + data.width);
				lineMaxHeight = Math::Max(lineMaxHeight, data.offsetY + data.height);
				xOffset += data.advance;
				bApplyKerning = !FCString::IsSpace(c);
				prevChar = c;
			}
		}

		if (outCharCount)
		{
			*outCharCount = count;
		}

		result.x = Math::Max(result.x, xOffset);
		result.y += lineMaxHeight;
		return result;
	}

	template< typename CharT >
	int GetCharCountT(CharT const* str)
	{
		if (str == nullptr || *str == 0)
			return 0;

		int result = 0;
		while (*str != 0)
		{
			wchar_t c;
			str += FCharConv< CharT, wchar_t >::Do(str, &c);
			
			if (c == L'\n' || FCString::IsSpace(c))
			{
				continue;
			}
			++result;
		}
		return result;
	}

	int FontDrawer::getCharCount(wchar_t const* str)
	{
		return GetCharCountT(str);
	}

	int FontDrawer::getCharCount(char const* str)
	{
		return GetCharCountT(str);
	}

	Vector2 FontDrawer::calcTextExtent(wchar_t const* str, int* outCharCount)
	{
		assert(isValid());
		return calcTextExtentT(str, outCharCount);
	}

	Vector2 FontDrawer::calcTextExtent(char const* str, int* outCharCount)
	{
		assert(isValid());
		return calcTextExtentT(str, outCharCount);
	}

	Vector2 FontDrawer::calcTextExtent(wchar_t const* str, float scale, int* outCharCount)
	{
		assert(isValid());
		return scale * calcTextExtentT(str, outCharCount);
	}

	Vector2 FontDrawer::calcTextExtent(char const* str, float scale, int* outCharCount)
	{
		assert(isValid());
		return scale * calcTextExtentT(str, outCharCount);
	}


	Vector2 FontDrawer::calcTextExtent(WStringView str, int* outCharCount)
	{
		assert(isValid());
		return calcTextExtentT(str, outCharCount);
	}

	Vector2 FontDrawer::calcTextExtent(StringView str, int* outCharCount)
	{
		assert(isValid());
		return calcTextExtentT(str, outCharCount);
	}

	Vector2 FontDrawer::calcTextExtent(WStringView str, float scale, int* outCharCount)
	{
		assert(isValid());
		return scale * calcTextExtentT(str, outCharCount);
	}

	Vector2 FontDrawer::calcTextExtent(StringView str, float scale, int* outCharCount)
	{
		assert(isValid());
		return scale * calcTextExtentT(str, outCharCount);
	}

	void FontDrawer::drawImpl(RHICommandList& commandList, Vector2 const& pos, Matrix4 const& transform, LinearColor const& color, wchar_t const* str)
	{
		static thread_local TArray< GlyphVertex > GTextBuffer;
		GTextBuffer.clear();
		generateVertices(pos, str, GTextBuffer);
		draw(commandList, transform, color, GTextBuffer);
	}


	void FontDrawer::draw(RHICommandList& commandList, Matrix4 const& transform, LinearColor const& color, TArray< GlyphVertex > const& buffer )
	{
		if (!buffer.empty())
		{
			RHISetFixedShaderPipelineState(commandList, transform, color, &mCharDataSet->getTexture());
			TRenderRT< RTVF_XY_T2 >::Draw(commandList, EPrimitive::Quad, buffer.data(), buffer.size());
		}
	}

	bool FontDrawer::initialize(FontFaceInfo const& fontFace)
	{
		if( !FontCharCache::Get().initialize() )
			return false;

		mCharDataSet = FontCharCache::Get().getCharDataSet(fontFace);
		if( mCharDataSet == nullptr )
			return false;

		return true;
	}

	bool FontDrawer::initialize(ICustomCharDataProvider& provider)
	{
		if (!FontCharCache::Get().initialize())
			return false;

		mCharDataSet = FontCharCache::Get().getCharDataSet(provider);
		if (mCharDataSet == nullptr)
			return false;

		return true;

	}

	void FontDrawer::cleanup()
	{
		mCharDataSet = nullptr;
	}

	template< typename CharT , typename TAddQuad >
	void FontDrawer::generateVerticesT(Vector2 const& pos, CharT const* str, TAddQuad& addQuad, Vector2* outBoundSize)
	{
		if (outBoundSize)
		{
			*outBoundSize = calcTextExtentT(str, nullptr);
		}

		Vector2 curPos = pos;
		wchar_t prevChar = 0;

		bool bApplyKerning = false;
		while (*str != 0)
		{
			wchar_t c;
			str += FCharConv< CharT, wchar_t >::Do(str, &c);

			if (c == L'\n')
			{
				curPos.x = pos.x;
				curPos.y += ( mCharDataSet->getFontHeight() + 2 );
				bApplyKerning = false;
				continue;
			}

			CharDataSet::CharData const& data = mCharDataSet->findOrAddChar(c);

			if (FCString::IsSpace(c))
			{
				curPos.x += data.advance;
				bApplyKerning = false;
			}
			else
			{
				if (bApplyKerning)
				{
					curPos.x += data.kerning;
#if 1
					float kerning;
					if (mCharDataSet->getKerningPair(prevChar, c, kerning))
					{
						curPos.x += kerning;
					}
#endif
				}
				addQuad(curPos + Vector2(data.offsetX, data.offsetY), Vector2(data.width, data.height), data.uvMin, data.uvMax);
				curPos.x += data.advance;
				bApplyKerning = true;
				prevChar = c;
			}
		}
	}

	template< typename CharT, typename TAddQuad >
	void FontDrawer::generateVerticesT(Vector2 const& pos, CharT const* str, float scale, TAddQuad& addQuad, Vector2* outBoundSize)
	{
		if (outBoundSize)
		{
			*outBoundSize = scale * calcTextExtentT(str, nullptr);
		}

		Vector2 curPos = pos;
		wchar_t prevChar = 0;

		bool bApplyKerning = false;
		while (*str != 0)
		{
			wchar_t c;
			str += FCharConv< CharT, wchar_t >::Do(str, &c);

			if (c == L'\n')
			{
				curPos.x = pos.x;
				curPos.y += scale * (mCharDataSet->getFontHeight() + 2);
				bApplyKerning = false;
				continue;
			}

			CharDataSet::CharData const& data = mCharDataSet->findOrAddChar(c);

			if (FCString::IsSpace(c))
			{
				curPos.x += scale * data.advance;
				bApplyKerning = false;
			}
			else
			{
				if (bApplyKerning)
				{
					curPos.x += scale * data.kerning;
#if 1
					float kerning;
					if (mCharDataSet->getKerningPair(prevChar, c, kerning))
					{
						curPos.x += scale * kerning;
					}
#endif
				}
				addQuad(curPos + scale * Vector2(data.offsetX, data.offsetY), scale * Vector2(data.width, data.height), data.uvMin, data.uvMax);
				curPos.x += scale * data.advance;
				bApplyKerning = true;
				prevChar = c;
			}
		}
	}

	template< typename CharT >
	void FontDrawer::generateVerticesT( Vector2 const& pos , CharT const* str, TArray< GlyphVertex >& outVertices, Vector2* outBoundSize)
	{
		auto AddQuad = [&](Vector2 const& pos, Vector2 const& size, Vector2 const& uvMin, Vector2 const& uvMax)
		{
			Vector2 posMax = pos + size;
			outVertices.push_back({ pos , uvMin });
			outVertices.push_back({ Vector2(posMax.x , pos.y) , Vector2(uvMax.x , uvMin.y) });
			outVertices.push_back({ posMax , uvMax });
			outVertices.push_back({ Vector2(pos.x , posMax.y) , Vector2(uvMin.x , uvMax.y) });
		};
		generateVerticesT(pos, str, AddQuad, outBoundSize);
	}

	template< typename CharT >
	void FontDrawer::generateVerticesT(Vector2 const& pos, CharT const* str, float scale, TArray< GlyphVertex >& outVertices, Vector2* outBoundSize)
	{
		auto AddQuad = [&](Vector2 const& pos, Vector2 const& size, Vector2 const& uvMin, Vector2 const& uvMax)
		{
			Vector2 posMax = pos + size;
			outVertices.push_back({ pos , uvMin });
			outVertices.push_back({ Vector2(posMax.x , pos.y) , Vector2(uvMax.x , uvMin.y) });
			outVertices.push_back({ posMax , uvMax });
			outVertices.push_back({ Vector2(pos.x , posMax.y) , Vector2(uvMin.x , uvMax.y) });
		};
		generateVerticesT(pos, str, scale, AddQuad, outBoundSize);
	}

	void FontDrawer::generateVertices(Vector2 const& pos, char const* str, TArray< GlyphVertex >& outVertices, Vector2* outBoundSize)
	{
		generateVerticesT(pos, str, outVertices, outBoundSize);
	}

	void FontDrawer::generateVertices(Vector2 const& pos, wchar_t const* str, TArray< GlyphVertex >& outVertices, Vector2* outBoundSize)
	{
		generateVerticesT(pos, str, outVertices, outBoundSize);
	}

	void FontDrawer::generateVertices(Vector2 const& pos, char const* str, float scale, TArray< GlyphVertex >& outVertices, Vector2* outBoundSize)
	{
		generateVerticesT(pos, str, scale, outVertices, outBoundSize);
	}

	void FontDrawer::generateVertices(Vector2 const& pos, wchar_t const* str, float scale, TArray< GlyphVertex >& outVertices, Vector2* outBoundSize)
	{
		generateVerticesT(pos, str, scale, outVertices, outBoundSize);
	}


	float FontDrawer::generateLineVertices(Vector2 const& pos, StringView line, float scale, TArray< GlyphVertex >& outVertices)
	{
		auto AddQuad = [&](Vector2 const& pos, Vector2 const& size, Vector2 const& uvMin, Vector2 const& uvMax)
		{
			Vector2 posMax = pos + size;
			outVertices.push_back({ pos , uvMin });
			outVertices.push_back({ Vector2(posMax.x , pos.y) , Vector2(uvMax.x , uvMin.y) });
			outVertices.push_back({ posMax , uvMax });
			outVertices.push_back({ Vector2(pos.x , posMax.y) , Vector2(uvMin.x , uvMax.y) });
		};

		Vector2 curPos = pos;
		wchar_t prevChar = 0;

		bool bApplyKerning = false;
		char const* strEnd = line.data() + line.size();
		char const* str = line.data();
		while( str < strEnd)
		{
			wchar_t c;
			str += FCharConv< char, wchar_t >::Do(str, &c);

			CharDataSet::CharData const& data = mCharDataSet->findOrAddChar(c);
			if (FCString::IsSpace(c))
			{
				curPos.x += scale * data.advance;
				bApplyKerning = false;
			}
			else
			{
				if (bApplyKerning)
				{
					curPos.x += scale * data.kerning;
#if 1
					float kerning;
					if (mCharDataSet->getKerningPair(prevChar, c, kerning))
					{
						curPos.x += scale * kerning;
					}
#endif
				}

				AddQuad(curPos + scale * Vector2(data.offsetX, data.offsetY), scale * Vector2(data.width, data.height), data.uvMin, data.uvMax);
				curPos.x += scale * data.advance;
				bApplyKerning = true;
				prevChar = c;
			}
		}

		return curPos.x;
	}

	template< typename CharT >
	void FontDrawer::generateVerticesT(Vector2 const& pos, CharT const* str, GlyphVertex* outVertices, Vector2* outBoundSize)
	{
		auto AddQuad = [&](Vector2 const& pos, Vector2 const& size, Vector2 const& uvMin, Vector2 const& uvMax)
		{
			Vector2 posMax = pos + size;
			outVertices[0] = { pos , uvMin };
			outVertices[1] = { Vector2(posMax.x , pos.y) , Vector2(uvMax.x , uvMin.y) };
			outVertices[2] = { posMax , uvMax };
			outVertices[3] = { Vector2(pos.x , posMax.y) , Vector2(uvMin.x , uvMax.y) };
			outVertices += 4;
		};
		generateVerticesT(pos, str, AddQuad, outBoundSize);
	}
	template< typename CharT >
	void FontDrawer::generateVerticesT(Vector2 const& pos, CharT const* str, float scale, GlyphVertex* outVertices, Vector2* outBoundSize)
	{
		auto AddQuad = [&](Vector2 const& pos, Vector2 const& size, Vector2 const& uvMin, Vector2 const& uvMax)
		{
			Vector2 posMax = pos + size;
			outVertices[0] = { pos , uvMin };
			outVertices[1] = { Vector2(posMax.x , pos.y) , Vector2(uvMax.x , uvMin.y) };
			outVertices[2] = { posMax , uvMax };
			outVertices[3] = { Vector2(pos.x , posMax.y) , Vector2(uvMin.x , uvMax.y) };
			outVertices += 4;
		};
		generateVerticesT(pos, str, scale, AddQuad, outBoundSize);
	}

	void FontDrawer::generateVertices(Vector2 const& pos, char const* str, GlyphVertex* outVertices, Vector2* outBoundSize)
	{
		generateVerticesT(pos, str, outVertices, outBoundSize);
	}

	void FontDrawer::generateVertices(Vector2 const& pos, wchar_t const* str, GlyphVertex* outVertices, Vector2* outBoundSize)
	{
		generateVerticesT(pos, str, outVertices, outBoundSize);
	}

	void FontDrawer::generateVertices(Vector2 const& pos, char const* str, float scale, GlyphVertex* outVertices, Vector2* outBoundSize)
	{
		generateVerticesT(pos, str, scale, outVertices, outBoundSize);
	}

	void FontDrawer::generateVertices(Vector2 const& pos, wchar_t const* str, float scale, GlyphVertex* outVertices, Vector2* outBoundSize)
	{
		generateVerticesT(pos, str, scale, outVertices, outBoundSize);
	}


	void FontDrawer::generateClippedVertices(Vector2 const& pos, char const* str, Rect const& clipRect, TArray< GlyphVertex >& outVertices, Vector2* outBoundSize)
	{
		generateClippedVerticesT(pos, str, clipRect, outVertices, outBoundSize);
	}

	void FontDrawer::generateClippedVertices(Vector2 const& pos, wchar_t const* str, Rect const& clipRect, TArray< GlyphVertex >& outVertices, Vector2* outBoundSize)
	{
		generateClippedVerticesT(pos, str, clipRect, outVertices, outBoundSize);
	}

	void FontDrawer::generateClippedVertices(Vector2 const& pos, char const* str, float scale, Rect const& clipRect, TArray< GlyphVertex >& outVertices, Vector2* outBoundSize)
	{
		generateClippedVerticesT(pos, str, scale, clipRect, outVertices, outBoundSize);
	}

	void FontDrawer::generateClippedVertices(Vector2 const& pos, wchar_t const* str, float scale, Rect const& clipRect, TArray< GlyphVertex >& outVertices, Vector2* outBoundSize)
	{
		generateClippedVerticesT(pos, str, scale, clipRect, outVertices, outBoundSize);
	}

	template< typename CharT >
	void FontDrawer::generateClippedVerticesT(Vector2 const& pos, CharT const* str, Rect const& clipRect, TArray< GlyphVertex >& outVertices, Vector2* outBoundSize)
	{
		generateClippedVerticesT(pos, str, 1.0f, clipRect, outVertices, outBoundSize);
	}

	template< typename CharT >
	void FontDrawer::generateClippedVerticesT(Vector2 const& pos, CharT const* str, float scale, Rect const& clipRect, TArray< GlyphVertex >& outVertices, Vector2* outBoundSize)
	{
		if (outBoundSize)
		{
			*outBoundSize = scale * calcTextExtentT(str, nullptr);
		}

		Vector2 curPos = pos;
		wchar_t prevChar = 0;
		bool bApplyKerning = false;
		float lineDist = scale * (mCharDataSet->getFontHeight() + 2);

		Vector2 c0 = clipRect.pos;
		Vector2 c1 = clipRect.pos + clipRect.size;

		while (*str != 0)
		{
			wchar_t c;
			int consumed = FCharConv< CharT, wchar_t >::Do(str, &c);
			CharT const* nextStr = str + consumed;

			if (c == L'\n')
			{
				curPos.x = pos.x;
				curPos.y += lineDist;
				bApplyKerning = false;
				str = nextStr;

				if (curPos.y >= c1.y)
					break;
				continue;
			}

			if (curPos.y + lineDist <= c0.y)
			{
				str = nextStr;
				while (*str != 0)
				{
					wchar_t tempC;
					int tempConsumed = FCharConv< CharT, wchar_t >::Do(str, &tempC);
					if (tempC == L'\n')
						break;
					str += tempConsumed;
				}
				continue;
			}

			CharDataSet::CharData const& data = mCharDataSet->findOrAddChar(c);

			if (FCString::IsSpace(c))
			{
				curPos.x += scale * data.advance;
				bApplyKerning = false;
			}
			else
			{
				if (bApplyKerning)
				{
					curPos.x += scale * data.kerning;
					float kerning;
					if (mCharDataSet->getKerningPair(prevChar, c, kerning))
					{
						curPos.x += scale * kerning;
					}
				}

				Vector2 size = scale * Vector2(data.width, data.height);
				Vector2 p0 = curPos + scale * Vector2(data.offsetX, data.offsetY);
				Vector2 p1 = p0 + size;

				if (p0.x >= c1.x)
				{
					str = nextStr;
					while (*str != 0)
					{
						wchar_t tempC;
						int tempConsumed = FCharConv< CharT, wchar_t >::Do(str, &tempC);
						if (tempC == L'\n')
							break;
						str += tempConsumed;
					}
					continue;
				}

				if (p1.x > c0.x && p1.y > c0.y && p0.y < c1.y)
				{
					Vector2 np0 = p0.max(c0);
					Vector2 np1 = p1.min(c1);

					if (size.x > 0 && size.y > 0)
					{
						Vector2 invSize = Vector2(1.0f / size.x, 1.0f / size.y);
						Vector2 nuv0 = data.uvMin + (data.uvMax - data.uvMin) * (np0 - p0) * invSize;
						Vector2 nuv1 = data.uvMin + (data.uvMax - data.uvMin) * (np1 - p0) * invSize;

						outVertices.push_back({ np0 , nuv0 });
						outVertices.push_back({ Vector2(np1.x , np0.y) , Vector2(nuv1.x , nuv0.y) });
						outVertices.push_back({ np1 , nuv1 });
						outVertices.push_back({ Vector2(np0.x , np1.y) , Vector2(nuv0.x , nuv1.y) });
					}
				}

				curPos.x += scale * data.advance;
				bApplyKerning = true;
				prevChar = c;
			}
			str = nextStr;
		}
	}

}//namespace Render

