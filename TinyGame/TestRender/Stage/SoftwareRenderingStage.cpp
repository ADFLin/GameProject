#include "SoftwareRenderingStage.h"

#include "ProfileSystem.h"

#define USE_OMP 1
#define USE_EDGE_FUNCTION 0
#define USE_EDGE_TILE 1
#define USE_LANE_SCALAR 1

#if USE_OMP
#include "omp.h"
#endif
#include "Image/ImageData.h"
#include "SystemPlatform.h"
#include "BitUtility.h"
#include "DrawEngine.h"

#if CPP_COMPILER_MSVC
#include <intrin.h>
#endif

namespace SR
{
	REGISTER_STAGE_ENTRY("Software Renderering", TestStage, EExecGroup::GraphicsTest, "Render");

	FORCEINLINE uint64 ReadCPUCycles()
	{
#if CPP_COMPILER_MSVC
		return __rdtsc();
#else
		return 0;
#endif
	}

	alignas(32) float constexpr GLaneOffsetsData[LaneScalar::Size] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };


	bool ColorBuffer::create(Vec2i const& size)
	{
		int const alignedSize = 4;
		mSize = size;
		mRowStride = AlignArbitrary(size.x, alignedSize);

		BITMAPINFO bmpInfo;
		bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmpInfo.bmiHeader.biWidth = mRowStride;
		bmpInfo.bmiHeader.biHeight = mSize.y;
		bmpInfo.bmiHeader.biPlanes = 1;
		bmpInfo.bmiHeader.biBitCount = 32;
		bmpInfo.bmiHeader.biCompression = BI_RGB;
		bmpInfo.bmiHeader.biXPelsPerMeter = 0;
		bmpInfo.bmiHeader.biYPelsPerMeter = 0;
		bmpInfo.bmiHeader.biSizeImage = 0;
		if( !mBufferDC.initialize(NULL, &bmpInfo, (void**)&mData) )
			return false;
		return true;
	}

	bool Texture::load(char const* path)
	{
		ImageData imageData;
		if (!imageData.load(path))
			return false;

		mSize.x = imageData.width;
		mSize.y = imageData.height;

		//#TODO
		switch( imageData.numComponent )
		{
		case 3:
			{
				mData.resize(mSize.x * mSize.y);
				unsigned char* pPixel = (unsigned char*)imageData.data;
				for( int i = 0; i < mData.size(); ++i )
				{
					mData[i] = Color4ub(pPixel[0], pPixel[1], pPixel[2], 0xff);
					pPixel += 3;
				}
			}
			break;
		case 4:
			{
				mData.resize(mSize.x * mSize.y);
				unsigned char* pPixel = (unsigned char*)imageData.data;
				for( int i = 0; i < mData.size(); ++i )
				{
					mData[i] = Color4ub(pPixel[0], pPixel[1], pPixel[2], pPixel[3]);
					pPixel += 4;
				}
			}
			break;
		}

		return mData.empty() == false;
	}

	Color4ub Texture::getColor(Vec2i const& pos) const
	{
		assert(0 <= pos.x && pos.x < mSize.x);
		assert(0 <= pos.y && pos.y < mSize.y);
		return mData[pos.x + pos.y * mSize.x];
	}

	FORCEINLINE Color4ub SampleBilinearFixed(Texture const& tex, float u, float v)
	{
		constexpr int FixedBits = 8;
		constexpr int FixedScale = 1 << FixedBits;
		constexpr int FixedMask = FixedScale - 1;
		constexpr int WeightShift = 2 * FixedBits;

		u = Math::Clamp(u, 0.0f, 1.0f);
		v = Math::Clamp(v, 0.0f, 1.0f);

		int fx = int(u * ((tex.mSize.x - 1) << FixedBits));
		int fy = int(v * ((tex.mSize.y - 1) << FixedBits));

		int x0 = fx >> FixedBits;
		int y0 = fy >> FixedBits;
		int x1 = std::min(x0 + 1, tex.mSize.x - 1);
		int y1 = std::min(y0 + 1, tex.mSize.y - 1);

		int tx = fx & FixedMask;
		int ty = fy & FixedMask;

		int w00 = (FixedScale - tx) * (FixedScale - ty);
		int w10 = tx * (FixedScale - ty);
		int w01 = (FixedScale - tx) * ty;
		int w11 = tx * ty;

		Color4ub const* row0 = tex.mData.data() + y0 * tex.mSize.x;
		Color4ub const* row1 = tex.mData.data() + y1 * tex.mSize.x;

		Color4ub const& c00 = row0[x0];
		Color4ub const& c10 = row0[x1];
		Color4ub const& c01 = row1[x0];
		Color4ub const& c11 = row1[x1];

		Color4ub out;
		out.r = uint8((c00.r * w00 + c10.r * w10 + c01.r * w01 + c11.r * w11) >> WeightShift);
		out.g = uint8((c00.g * w00 + c10.g * w10 + c01.g * w01 + c11.g * w11) >> WeightShift);
		out.b = uint8((c00.b * w00 + c10.b * w10 + c01.b * w01 + c11.b * w11) >> WeightShift);
		out.a = uint8((c00.a * w00 + c10.a * w10 + c01.a * w01 + c11.a * w11) >> WeightShift);
		return out;
	}

	LinearColor Texture::sample(Vector2 const& UV) const
	{
#if 0
		float u = Math::Clamp<float>(UV.x, 0.0f, 1.0f) * (mSize.x - 1);
		float v = Math::Clamp<float>(UV.y, 0.0f, 1.0f) * (mSize.y - 1);

		int x0 = Math::FloorToInt(u);
		int y0 = Math::FloorToInt(v);
		int x1 = std::min(x0 + 1, mSize.x - 1);
		int y1 = std::min(y0 + 1, mSize.y - 1);
		float dx = u - x0;
		float dy = v - y0;

		assert(0 <= x0 && x0 < mSize.x);
		assert(0 <= y0 && y0 < mSize.y);
		assert(0 <= x1 && x1 < mSize.x);
		assert(0 <= y1 && y1 < mSize.y);

		Color4ub const* row0 = mData.data() + y0 * mSize.x;
		Color4ub const* row1 = mData.data() + y1 * mSize.x;
		Color4ub const& c00 = row0[x0];
		Color4ub const& c10 = row0[x1];
		Color4ub const& c01 = row1[x0];
		Color4ub const& c11 = row1[x1];

		float const w00 = (1.0f - dx) * (1.0f - dy);
		float const w10 = dx * (1.0f - dy);
		float const w01 = (1.0f - dx) * dy;
		float const w11 = dx * dy;
		float constexpr Inv255 = 1.0f / 255.0f;

		return LinearColor(
			Inv255 * (w00 * c00.r + w10 * c10.r + w01 * c01.r + w11 * c11.r),
			Inv255 * (w00 * c00.g + w10 * c10.g + w01 * c01.g + w11 * c11.g),
			Inv255 * (w00 * c00.b + w10 * c10.b + w01 * c01.b + w11 * c11.b),
			Inv255 * (w00 * c00.a + w10 * c10.a + w01 * c01.a + w11 * c11.a));
#else
		return SampleBilinearFixed(*this, UV.x, UV.y);
#endif
	}

	LaneLinearColor Texture::sample(LaneVector2 const& UV) const
	{
		constexpr int FixedBits = 8;
		constexpr int FixedScale = 1 << FixedBits;
		constexpr int FixedMask = FixedScale - 1;
		constexpr float Inv255FixedWeight = 1.0f / float(255 * FixedScale * FixedScale);

		float uValues[LaneScalar::Size];
		float vValues[LaneScalar::Size];
		UV.x.store(uValues);
		UV.y.store(vValues);

		int index00Values[LaneScalar::Size];
		int index10Values[LaneScalar::Size];
		int index01Values[LaneScalar::Size];
		int index11Values[LaneScalar::Size];
		float w00Values[LaneScalar::Size];
		float w10Values[LaneScalar::Size];
		float w01Values[LaneScalar::Size];
		float w11Values[LaneScalar::Size];

		for (int lane = 0; lane < LaneScalar::Size; ++lane)
		{
			float u = Math::Clamp(uValues[lane], 0.0f, 1.0f);
			float v = Math::Clamp(vValues[lane], 0.0f, 1.0f);

			int fx = int(u * ((mSize.x - 1) << FixedBits));
			int fy = int(v * ((mSize.y - 1) << FixedBits));

			int x0 = fx >> FixedBits;
			int y0 = fy >> FixedBits;
			int x1 = std::min(x0 + 1, mSize.x - 1);
			int y1 = std::min(y0 + 1, mSize.y - 1);

			int tx = fx & FixedMask;
			int ty = fy & FixedMask;

			int w00 = (FixedScale - tx) * (FixedScale - ty);
			int w10 = tx * (FixedScale - ty);
			int w01 = (FixedScale - tx) * ty;
			int w11 = tx * ty;

			index00Values[lane] = x0 + y0 * mSize.x;
			index10Values[lane] = x1 + y0 * mSize.x;
			index01Values[lane] = x0 + y1 * mSize.x;
			index11Values[lane] = x1 + y1 * mSize.x;
			w00Values[lane] = float(w00);
			w10Values[lane] = float(w10);
			w01Values[lane] = float(w01);
			w11Values[lane] = float(w11);
		}

		__m256i index00 = _mm256_loadu_si256((__m256i const*)index00Values);
		__m256i index10 = _mm256_loadu_si256((__m256i const*)index10Values);
		__m256i index01 = _mm256_loadu_si256((__m256i const*)index01Values);
		__m256i index11 = _mm256_loadu_si256((__m256i const*)index11Values);
		__m256i c00 = _mm256_i32gather_epi32((int const*)mData.data(), index00, 4);
		__m256i c10 = _mm256_i32gather_epi32((int const*)mData.data(), index10, 4);
		__m256i c01 = _mm256_i32gather_epi32((int const*)mData.data(), index01, 4);
		__m256i c11 = _mm256_i32gather_epi32((int const*)mData.data(), index11, 4);

		__m256i mask = _mm256_set1_epi32(0xff);
		auto unpackChannel = [mask](__m256i color, int shift)
		{
			return LaneScalar(_mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(color, shift), mask)));
		};

		LaneScalar w00(w00Values);
		LaneScalar w10(w10Values);
		LaneScalar w01(w01Values);
		LaneScalar w11(w11Values);
		LaneScalar scale(Inv255FixedWeight);

		LaneLinearColor result;
		result.r = (unpackChannel(c00, 0) * w00 + unpackChannel(c10, 0) * w10 + unpackChannel(c01, 0) * w01 + unpackChannel(c11, 0) * w11) * scale;
		result.g = (unpackChannel(c00, 8) * w00 + unpackChannel(c10, 8) * w10 + unpackChannel(c01, 8) * w01 + unpackChannel(c11, 8) * w11) * scale;
		result.b = (unpackChannel(c00, 16) * w00 + unpackChannel(c10, 16) * w10 + unpackChannel(c01, 16) * w01 + unpackChannel(c11, 16) * w11) * scale;
		result.a = (unpackChannel(c00, 24) * w00 + unpackChannel(c10, 24) * w10 + unpackChannel(c01, 24) * w01 + unpackChannel(c11, 24) * w11) * scale;
		return result;
	}


	struct ScanLineIterator
	{
		float xMin;
		float xMax;
		float dxdyMin;
		float dxdyMax;
		float yMin;
		float yMax;
		int yStart;
		int yEnd;
		void advance()
		{
			xMin += dxdyMin;
			xMax += dxdyMax;
		}
	};



	int PixelCut(float value)
	{
		return Math::FloorToInt(value + 0.5f);
	}


	// 1/z = 1/z0 * ( 1- a ) + 1 / z1 * a
	// v = v0 * ( z / z0 ) * ( 1 - a ) + v1 * ( z / z1 ) * a

	// w = w0 * ( 1- a ) + w1 * a = w0 + ( w1 - w0 ) * a
	//=> dw = ( w1 - w0 ) * da
	// v = v0 * ( w0 / w ) * ( 1 - a ) + v1 * ( w1 / w ) * a
	//   = ( v0 * w0 + ( v1 * w1 - v0 * w0 ) * a ) / w
	//=> dv = ( v1 * w1 - v0 * w0 ) * d( a / w )
	template< class T >
	T PerspectiveLerp(T const& v0, T const& v1, float w0, float w1, float w, float alpha)
	{
#if 1
		return  (w0 / w) * (1 - alpha) * v0 + (w1 / w) * alpha * v1;
#else
		float invW = 1.0 / w;
		return  ( invW * w0 ) * v0  + (alpha * invW) * (w1 * v1 - w0 * v0);
#endif
	}

	struct DefaultVSOutput
	{
		struct LaneType
		{
			LaneVector2 uv;
			LaneLinearColor color;
		};

		Vector2  uv;
		LinearColor color;
	};

	struct VertexColorPixelShader
	{
		using Input = DefaultVSOutput;

		FORCEINLINE LinearColor operator()(Input const& input) const
		{
			return input.color;
		}

		FORCEINLINE LaneLinearColor operator()(Input::LaneType const& input) const
		{
			return input.color;
		}
	};


#define USE_InterpolantsParam_SIMD 1

	template< typename TVertexData >
	struct TPixelInterpolantsParam
	{
		static_assert(sizeof(TVertexData) % sizeof(float) == 0, "TVertexData must be float-packed");

		static constexpr int NumParams = sizeof(TVertexData) / sizeof(float);
#if USE_InterpolantsParam_SIMD
		using FloatVector = SIMD::TFloatVector<4>;
		static constexpr int LaneSize = FloatVector::Size;
		static constexpr int NumParamBlocks = (NumParams + LaneSize - 1) / LaneSize;
		static constexpr int NumFullParamBlocks = NumParams / LaneSize;
		static constexpr int NumTailParams = NumParams % LaneSize;
#endif

		float w0;
		float dw;
#if USE_InterpolantsParam_SIMD
		FloatVector wv0[NumParamBlocks];
		FloatVector dwv[NumParamBlocks];
#else
		float wv0[NumParams];
		float dwv[NumParams];
#endif

		TPixelInterpolantsParam(TVertexData const& v0, TVertexData const& v1, float inW0, float inW1)
		{
			float const* p0 = reinterpret_cast<float const*>(&v0);
			float const* p1 = reinterpret_cast<float const*>(&v1);

			w0 = inW0;
			dw = inW1 - inW0;

#if USE_InterpolantsParam_SIMD
			for (int block = 0; block < NumParamBlocks; ++block)
			{
				float wv0Values[LaneSize] = {};
				float dwvValues[LaneSize] = {};

				if constexpr (NumTailParams == 0)
				{
					int indexStart = block * LaneSize;
					int indexEnd = indexStart + LaneSize;
					for (int i = indexStart; i < indexEnd; ++i)
					{
						int lane = i - indexStart;
						wv0Values[lane] = inW0 * p0[i];
						dwvValues[lane] = inW1 * p1[i] - wv0Values[lane];
					}
				}
				else
				{
					int indexStart = block * LaneSize;
					int indexEnd = Math::Min(indexStart + LaneSize, NumParams);
					for (int i = indexStart; i < indexEnd; ++i)
					{
						int lane = i - indexStart;
						wv0Values[lane] = inW0 * p0[i];
						dwvValues[lane] = inW1 * p1[i] - wv0Values[lane];
					}

				}

				wv0[block] = FloatVector(wv0Values);
				dwv[block] = FloatVector(dwvValues);
			}
#else
			for (int i = 0; i < NumParams; ++i)
			{
				wv0[i] = inW0 * p0[i];
				dwv[i] = inW1 * p1[i] - wv0[i];
			}
#endif
		}

		void perspectiveLerp(TVertexData& outData, float alpha, float& outW) const
		{
			float* pOut = reinterpret_cast<float*>(&outData);

			outW = w0 + alpha * dw;
			float const wInv = 1.0f / outW;
			float const alphaW = alpha * wInv;

#if USE_InterpolantsParam_SIMD
			for (int block = 0; block < NumFullParamBlocks; ++block)
			{
				(wv0[block] * wInv + dwv[block] * alphaW).store(pOut + block * LaneSize);
			}
			if constexpr (NumTailParams != 0)
			{
				float tailValues[LaneSize];
				(wv0[NumFullParamBlocks] * wInv + dwv[NumFullParamBlocks] * alphaW).store(tailValues);
				for (int i = 0; i < NumTailParams; ++i)
				{
					pOut[NumFullParamBlocks * LaneSize + i] = tailValues[i];
				}
			}
#else
			for (int i = 0; i < NumParams; ++i)
			{
				pOut[i] = wInv * wv0[i] + alphaW * dwv[i];
			}
#endif
		}

		struct Stepper
		{
			float w;
			float dw;
			float depth;
			float dDepth;

#if USE_InterpolantsParam_SIMD
			FloatVector paramW[NumParamBlocks];
			FloatVector dParamW[NumParamBlocks];
#else
			float paramW[NumParams];
			float dParamW[NumParams];
#endif

			Stepper(TVertexData const& v0, TVertexData const& v1, float inW0, float inW1, float alpha, float dAlpha, float depth0, float dDepthIn)
			{		
				float paramW0 = inW0;
				float paramDw = inW1 - inW0;

				w = paramW0 + alpha * paramDw;
				dw = dAlpha * paramDw;
				depth = depth0 + alpha * dDepthIn;
				dDepth = dAlpha * dDepthIn;

				float const* p0 = reinterpret_cast<float const*>(&v0);
				float const* p1 = reinterpret_cast<float const*>(&v1);
#if USE_InterpolantsParam_SIMD
				for (int block = 0; block < NumParamBlocks; ++block)
				{
					float paramWValues[LaneSize] = {};
					float dParamWValues[LaneSize] = {};
					int indexStart = block * LaneSize;
					int indexEnd = Math::Min(indexStart + LaneSize, NumParams);
					for (int i = indexStart; i < indexEnd; ++i)
					{
						float wv0 = inW0 * p0[i];
						float dwv = inW1 * p1[i] - wv0;
						int lane = i - indexStart;
						paramWValues[lane] = wv0 + alpha * dwv;
						dParamWValues[lane] = dAlpha * dwv;
					}
					paramW[block] = FloatVector(paramWValues);
					dParamW[block] = FloatVector(dParamWValues);
				}
#else
				for (int i = 0; i < NumParams; ++i)
				{
					float wv0 = inW0 * p0[i];
					float dwv = inW1 * p1[i] - wv0;
					paramW[i] = wv0 + alpha * dwv;
					dParamW[i] = dAlpha * dwv;
				}
#endif
			}

			Stepper(Stepper const& v0, Stepper const& v1, float alpha, float dAlpha)
			{
				float paramDw = v1.w - v0.w;
				float dDepthIn = v1.depth - v0.depth;

				w = v0.w + alpha * paramDw;
				dw = dAlpha * paramDw;
				depth = v0.depth + alpha * dDepthIn;
				dDepth = dAlpha * dDepthIn;

#if USE_InterpolantsParam_SIMD
				FloatVector alphaVec(alpha);
				FloatVector dAlphaVec(dAlpha);
				for (int block = 0; block < NumParamBlocks; ++block)
				{
					FloatVector dParam = v1.paramW[block] - v0.paramW[block];
					paramW[block] = v0.paramW[block] + alphaVec * dParam;
					dParamW[block] = dAlphaVec * dParam;
				}
#else
				for (int i = 0; i < NumParams; ++i)
				{
					float dParam = v1.paramW[i] - v0.paramW[i];
					paramW[i] = v0.paramW[i] + alpha * dParam;
					dParamW[i] = dAlpha * dParam;
				}
#endif
			}

			FORCEINLINE void get(TVertexData& outData) const
			{
				float* pOut = reinterpret_cast<float*>(&outData);
				float const wInv = 1.0f / w;

#if USE_InterpolantsParam_SIMD
				for (int block = 0; block < NumFullParamBlocks; ++block)
				{
					(paramW[block] * wInv).store(pOut + block * LaneSize);
				}
				if constexpr (NumTailParams != 0)
				{
					float tailValues[LaneSize];
					(paramW[NumFullParamBlocks] * wInv).store(tailValues);
					for (int i = 0; i < NumTailParams; ++i)
					{
						pOut[NumFullParamBlocks * LaneSize + i] = tailValues[i];
					}
				}
#else
				for (int i = 0; i < NumParams; ++i)
				{
					pOut[i] = wInv * paramW[i];
				}
#endif
			}

#if USE_LANE_SCALAR
			FORCEINLINE void getLane(typename TVertexData::LaneType& outData, LaneScalar& outDepth) const
			{
				LaneScalar const LaneOffsets(GLaneOffsetsData, EAligned{});
				static_assert(sizeof(typename TVertexData::LaneType) == NumParams * sizeof(LaneScalar), "LaneType must match TVertexData float layout");

				LaneScalar const laneW = LaneScalar(w) + LaneScalar(dw) * LaneOffsets;
				LaneScalar const laneWInv = LaneScalar(1.0f) / laneW;
				outDepth = LaneScalar(depth) + LaneScalar(dDepth) * LaneOffsets;

				LaneScalar* pOut = reinterpret_cast<LaneScalar*>(&outData);
#if USE_InterpolantsParam_SIMD
				float paramValues[LaneSize];
				float dParamValues[LaneSize];
				for (int block = 0; block < NumParamBlocks; ++block)
				{
					paramW[block].store(paramValues);
					dParamW[block].store(dParamValues);

					int indexStart = block * LaneSize;
					int indexEnd = Math::Min(indexStart + LaneSize, NumParams);
					for (int i = indexStart; i < indexEnd; ++i)
					{
						int lane = i - indexStart;
						pOut[i] = (LaneScalar(paramValues[lane]) + LaneScalar(dParamValues[lane]) * LaneOffsets) * laneWInv;
					}
				}
#else
				for (int i = 0; i < NumParams; ++i)
				{
					pOut[i] = (LaneScalar(paramW[i]) + LaneScalar(dParamW[i]) * LaneOffsets) * laneWInv;
				}
#endif
			}
#endif

			FORCEINLINE void advance()
			{
				w += dw;
				depth += dDepth;
#if USE_InterpolantsParam_SIMD
				for (int block = 0; block < NumParamBlocks; ++block)
				{
					paramW[block] = paramW[block] + dParamW[block];
				}
#else
				for (int i = 0; i < NumParams; ++i)
				{
					paramW[i] += dParamW[i];
				}
#endif
			}

#if USE_LANE_SCALAR
			FORCEINLINE void advance(int count)
			{
				float const countFloat = float(count);
				w += dw * countFloat;
				depth += dDepth * countFloat;
#if USE_InterpolantsParam_SIMD
				for (int block = 0; block < NumParamBlocks; ++block)
				{
					paramW[block] = paramW[block] + dParamW[block] * countFloat;
				}
#else
				for (int i = 0; i < NumParams; ++i)
				{
					paramW[i] += dParamW[i] * countFloat;
				}
#endif
			}
#endif
		};
	};

	template< typename TVertexData >
	struct TPixelTriangleInterpolantsParam
	{
		static_assert(sizeof(TVertexData) % sizeof(float) == 0, "TVertexData must be float-packed");



		static constexpr int NumParams = sizeof(TVertexData) / sizeof(float);
#if USE_MATH_SIMD
		using FloatVector = SIMD::TFloatVector<4>;
		static constexpr int LaneSize = FloatVector::Size;
		static constexpr int NumParamBlocks = (NumParams + LaneSize - 1) / LaneSize;
		static constexpr int NumFullParamBlocks = NumParams / LaneSize;
		static constexpr int NumTailParams = NumParams % LaneSize;
#endif

		float w0;
		float w1;
		float w2;
#if USE_MATH_SIMD
		FloatVector wv0[NumParamBlocks];
		FloatVector wv1[NumParamBlocks];
		FloatVector wv2[NumParamBlocks];
#else
		float wv0[NumParams];
		float wv1[NumParams];
		float wv2[NumParams];
#endif

		TPixelTriangleInterpolantsParam(TVertexData const& v0, TVertexData const& v1, TVertexData const& v2, float inW0, float inW1, float inW2)
		{
			float const* p0 = reinterpret_cast<float const*>(&v0);
			float const* p1 = reinterpret_cast<float const*>(&v1);
			float const* p2 = reinterpret_cast<float const*>(&v2);

			w0 = inW0;
			w1 = inW1;
			w2 = inW2;

#if USE_MATH_SIMD
			for (int block = 0; block < NumParamBlocks; ++block)
			{
				float wv0Values[LaneSize] = {};
				float wv1Values[LaneSize] = {};
				float wv2Values[LaneSize] = {};

				int indexStart = block * LaneSize;
				int indexEnd = Math::Min(indexStart + LaneSize, NumParams);
				for (int i = indexStart; i < indexEnd; ++i)
				{
					int lane = i - indexStart;
					wv0Values[lane] = inW0 * p0[i];
					wv1Values[lane] = inW1 * p1[i];
					wv2Values[lane] = inW2 * p2[i];
				}

				wv0[block] = FloatVector(wv0Values);
				wv1[block] = FloatVector(wv1Values);
				wv2[block] = FloatVector(wv2Values);
			}
#else
			for (int i = 0; i < NumParams; ++i)
			{
				wv0[i] = inW0 * p0[i];
				wv1[i] = inW1 * p1[i];
				wv2[i] = inW2 * p2[i];
			}
#endif
		}

		FORCEINLINE void perspectiveLerp(TVertexData& outData, float b0, float b1, float b2, float& outW) const
		{
			float* pOut = reinterpret_cast<float*>(&outData);

			outW = b0 * w0 + b1 * w1 + b2 * w2;
			float const wInv = 1.0f / outW;

#if USE_MATH_SIMD
			FloatVector b0Vec(b0);
			FloatVector b1Vec(b1);
			FloatVector b2Vec(b2);
			FloatVector wInvVec(wInv);
			for (int block = 0; block < NumFullParamBlocks; ++block)
			{
				((wv0[block] * b0Vec + wv1[block] * b1Vec + wv2[block] * b2Vec) * wInvVec).store(pOut + block * LaneSize);
			}
			if constexpr (NumTailParams != 0)
			{
				float tailValues[LaneSize];
				(wv0[NumFullParamBlocks] * b0Vec + wv1[NumFullParamBlocks] * b1Vec + wv2[NumFullParamBlocks] * b2Vec).store(tailValues);
				for (int i = 0; i < NumTailParams; ++i)
				{
					pOut[NumFullParamBlocks * LaneSize + i] = tailValues[i] * wInv;
				}
			}
#else
			for (int i = 0; i < NumParams; ++i)
			{
				pOut[i] = wInv * (b0 * wv0[i] + b1 * wv1[i] + b2 * wv2[i]);
			}
#endif
		}

		struct Stepper
		{
			float w;
			float dw;
			float depth;
			float dDepth;
#if USE_MATH_SIMD
			FloatVector paramW[NumParamBlocks];
			FloatVector dParamW[NumParamBlocks];
#else
			float paramW[NumParams];
			float dParamW[NumParams];
#endif

			Stepper(TPixelTriangleInterpolantsParam const& interpolants,
					float b0, float b1, float b2,
					float db0, float db1, float db2,
					float depth0, float depth1, float depth2)
			{
				w = b0 * interpolants.w0 + b1 * interpolants.w1 + b2 * interpolants.w2;
				dw = db0 * interpolants.w0 + db1 * interpolants.w1 + db2 * interpolants.w2;
				depth = b0 * depth0 + b1 * depth1 + b2 * depth2;
				dDepth = db0 * depth0 + db1 * depth1 + db2 * depth2;

#if USE_MATH_SIMD
				FloatVector b0Vec(b0);
				FloatVector b1Vec(b1);
				FloatVector b2Vec(b2);
				FloatVector db0Vec(db0);
				FloatVector db1Vec(db1);
				FloatVector db2Vec(db2);
				for (int block = 0; block < NumParamBlocks; ++block)
				{
					paramW[block] = interpolants.wv0[block] * b0Vec + interpolants.wv1[block] * b1Vec + interpolants.wv2[block] * b2Vec;
					dParamW[block] = interpolants.wv0[block] * db0Vec + interpolants.wv1[block] * db1Vec + interpolants.wv2[block] * db2Vec;
				}
#else
				for (int i = 0; i < NumParams; ++i)
				{
					paramW[i] = b0 * interpolants.wv0[i] + b1 * interpolants.wv1[i] + b2 * interpolants.wv2[i];
					dParamW[i] = db0 * interpolants.wv0[i] + db1 * interpolants.wv1[i] + db2 * interpolants.wv2[i];
				}
#endif
			}

			FORCEINLINE void get(TVertexData& outData) const
			{
				float* pOut = reinterpret_cast<float*>(&outData);
				float const wInv = 1.0f / w;

#if USE_MATH_SIMD
				for (int block = 0; block < NumFullParamBlocks; ++block)
				{
					(paramW[block] * wInv).store(pOut + block * LaneSize);
				}
				if constexpr (NumTailParams != 0)
				{
					float tailValues[LaneSize];
					(paramW[NumFullParamBlocks] * wInv).store(tailValues);
					for (int i = 0; i < NumTailParams; ++i)
					{
						pOut[NumFullParamBlocks * LaneSize + i] = tailValues[i];
					}
				}
#else
				for (int i = 0; i < NumParams; ++i)
				{
					pOut[i] = wInv * paramW[i];
				}
#endif
			}

			FORCEINLINE void advance()
			{
				w += dw;
				depth += dDepth;
#if USE_MATH_SIMD
				for (int block = 0; block < NumParamBlocks; ++block)
				{
					paramW[block] = paramW[block] + dParamW[block];
				}
#else
				for (int i = 0; i < NumParams; ++i)
				{
					paramW[i] += dParamW[i];
				}
#endif
			}
		};
	};



	template< typename TVSOutput >
	TVSOutput PerspectiveLerp(TVSOutput const& v0, TVSOutput const& v1, float w0, float w1, float alpha)
	{
		TPixelInterpolantsParam<TVSOutput> lnterpolantsParam(v0, v1, w0, w1);
		TVSOutput result;
		float w;
		lnterpolantsParam.perspectiveLerp(result, alpha, w);
		return result;
	}

	struct RasterPosition
	{
		Vector2 screenPos;
		float w;
		float depth;
	};

	template< class TDepthState, class TBlendState, class TVSOutput, class TPixelShader >
	FORCEINLINE void ProcessPixel(uint32* colorPtr, float* depthPtr, float depth, TVSOutput const& v, TPixelShader const& pixelShader)
	{
		if constexpr (TDepthState::EnableTest)
		{
			if (!TDepthState::Pass(depth, *depthPtr))
			{
				return;
			}
		}

		LinearColor srcC = pixelShader(v);
		LinearColor outC = srcC;
		if constexpr (TBlendState::Enable)
		{
			ColorBGRA8 dest;
			dest.word = *colorPtr;
			LinearColor destC = dest;
			outC = TBlendState::Blend(srcC, destC);
		}
		
#if 1
		ColorBGRA8 colorOut;
		if constexpr (TBlendState::Enable)
		{
			colorOut = outC.toBGRA();
		}
		else
		{
			colorOut = outC.toOpaqueBGRA();
		}
#else
		ColorBGRA8 colorOut = ColorBGRA8{ 0xff, 0 , 0 };
#endif
		*colorPtr = colorOut.word;
		if constexpr (TDepthState::EnableWrite)
		{
			*depthPtr = depth;
		}
	}

	FORCEINLINE __m256i PackOpaqueBGRA(LaneLinearColor const& color)
	{
		LaneScalar const zero = LaneScalar::Zero();
		LaneScalar const one(1.0f);
		LaneScalar const scale(255.0f);
		LaneScalar r = min(max(color.r, zero), one) * scale;
		LaneScalar g = min(max(color.g, zero), one) * scale;
		LaneScalar b = min(max(color.b, zero), one) * scale;

		__m256i ri = _mm256_cvttps_epi32(r.reg);
		__m256i gi = _mm256_cvttps_epi32(g.reg);
		__m256i bi = _mm256_cvttps_epi32(b.reg);
		__m256i ai = _mm256_set1_epi32(0xff000000);
		return _mm256_or_si256(
			_mm256_or_si256(bi, _mm256_slli_epi32(gi, 8)),
			_mm256_or_si256(_mm256_slli_epi32(ri, 16), ai));
	}

	template< class TDepthState, class TBlendState, class TVSOutput, class TPixelShader >
	FORCEINLINE void ProcessPixel(LaneMask writeMask, uint32* colorPtr, float* depthPtr, LaneScalar depth, typename TVSOutput::LaneType const& v, TPixelShader const& pixelShader)
	{
		if constexpr (TDepthState::EnableTest)
		{
			LaneScalar destDepth(_mm256_maskload_ps(depthPtr, writeMask.reg));
			LaneMask depthMask = TDepthState::Pass(depth, destDepth);
			writeMask = LaneMask(_mm256_and_si256(writeMask.reg, depthMask.reg));
		}

		int const activeMask = _mm256_movemask_ps(_mm256_castsi256_ps(writeMask.reg));
		if (activeMask == 0)
			return;


		if constexpr (TDepthState::EnableWrite)
		{
			_mm256_maskstore_ps(depthPtr, writeMask.reg, depth.reg);
		}

		LaneLinearColor srcC = pixelShader(v);
		if constexpr (!TBlendState::Enable)
		{
			_mm256_maskstore_epi32((int*)colorPtr, writeMask.reg, PackOpaqueBGRA(srcC));
			return;
		}

		float srcR[LaneScalar::Size];
		float srcG[LaneScalar::Size];
		float srcB[LaneScalar::Size];
		float srcA[LaneScalar::Size];
		srcC.r.store(srcR);
		srcC.g.store(srcG);
		srcC.b.store(srcB);
		srcC.a.store(srcA);

		for (int lane = 0; lane < LaneScalar::Size; ++lane)
		{
			if ((activeMask & (1 << lane)) == 0)
				continue;

			LinearColor src(srcR[lane], srcG[lane], srcB[lane], srcA[lane]);
			LinearColor outC = src;
			if constexpr (TBlendState::Enable)
			{
				ColorBGRA8 dest;
				dest.word = colorPtr[lane];
				outC = TBlendState::Blend(src, LinearColor(dest));
			}

			ColorBGRA8 colorOut;
			if constexpr (TBlendState::Enable)
			{
				colorOut = outC.toBGRA();
			}
			else
			{
				colorOut = outC.toOpaqueBGRA();
			}

			colorPtr[lane] = colorOut.word;
		}
	}

	template< class TDepthState, class TBlendState, class TVSOutput, class TPixelShader >
	FORCEINLINE void ProcessPixelFullLane(uint32* colorPtr, float* depthPtr, LaneScalar depth, typename TVSOutput::LaneType const& v, TPixelShader const& pixelShader)
	{
		int activeMask = 0xff;
		LaneMask writeMask(-1);
		if constexpr (TDepthState::EnableTest)
		{
			LaneMask depthMask = TDepthState::Pass(depth, LaneScalar(depthPtr));
			writeMask = depthMask;
			activeMask = _mm256_movemask_ps(_mm256_castsi256_ps(depthMask.reg));
			if (activeMask == 0)
				return;
		}

		if constexpr (TDepthState::EnableWrite)
		{
			if (activeMask == 0xff)
			{
				_mm256_storeu_ps(depthPtr, depth.reg);
			}
			else
			{
				_mm256_maskstore_ps(depthPtr, writeMask.reg, depth.reg);
			}
		}

		LaneLinearColor srcC = pixelShader(v);
		if constexpr (!TBlendState::Enable)
		{
			__m256i packedBGRA = PackOpaqueBGRA(srcC);
			if (activeMask == 0xff)
			{
				_mm256_storeu_si256((__m256i*)colorPtr, packedBGRA);
			}
			else
			{
				_mm256_maskstore_epi32((int*)colorPtr, writeMask.reg, packedBGRA);
			}
			return;
		}

		float srcR[LaneScalar::Size];
		float srcG[LaneScalar::Size];
		float srcB[LaneScalar::Size];
		float srcA[LaneScalar::Size];
		srcC.r.store(srcR);
		srcC.g.store(srcG);
		srcC.b.store(srcB);
		srcC.a.store(srcA);

		for (int lane = 0; lane < LaneScalar::Size; ++lane)
		{
			if ((activeMask & (1 << lane)) == 0)
				continue;

			LinearColor src(srcR[lane], srcG[lane], srcB[lane], srcA[lane]);
			ColorBGRA8 dest;
			dest.word = colorPtr[lane];
			colorPtr[lane] = TBlendState::Blend(src, LinearColor(dest)).toBGRA().word;
		}
	}

	//constexpr float RasterEpsilon = 1e-4f;
	constexpr float RasterEpsilon = 0;

	template< class TDepthState , class TBlendState , class TVSOutput , class TPixelShader >
	void ProcessScanLines(RenderTarget& renderTarget, ScanLineIterator& lineIter,
					      RasterPosition const& pL, TVSOutput const& vL,
					      RasterPosition const& pR, TVSOutput const& vR,
					      RasterPosition const& pS, TVSOutput const& vS,
					      bool bInverse, TPixelShader const& pixelShader)
	{
		using InterpolantsStepper = typename TPixelInterpolantsParam<TVSOutput>::Stepper;

		Vec2i size = renderTarget.colorBuffer->getSize();

		if (LIKELY(lineIter.yStart < lineIter.yEnd))
		{
			float day = 1 / (lineIter.yMax - lineIter.yMin);
			float ay = (lineIter.yStart + 0.5f - lineIter.yMin) * day;

			if (bInverse)
			{
				day = -day;
				ay = 1 + (lineIter.yStart + 0.5f - lineIter.yMin) * day;
			}

			float depthMin0 = pL.depth;
			float dDepthMin = pS.depth - pL.depth;
			float depthMax0 = pR.depth;
			float dDepthMax = pS.depth - pR.depth;
			InterpolantsStepper yStepperMin(vL, vS, pL.w, pS.w, ay, day, depthMin0, dDepthMin);
			InterpolantsStepper yStepperMax(vR, vS, pR.w, pS.w, ay, day, depthMax0, dDepthMax);

			for (int y = lineIter.yStart; y < lineIter.yEnd; ++y)
			{
				CHECK(lineIter.xMin <= lineIter.xMax);
				float xMin = lineIter.xMin;
				float xMax = lineIter.xMax;

				int xStart = Math::Max(0, Math::FloorToInt(xMin + 0.5f + RasterEpsilon));
				int xEnd = Math::Min(size.x, Math::FloorToInt(xMax + 0.5f + RasterEpsilon));

				if (LIKELY(xStart < xEnd))
				{
					float dax = 1 / (lineIter.xMax - lineIter.xMin);
					float ax = (xStart + 0.5f - lineIter.xMin) * dax;

					InterpolantsStepper xStepper(yStepperMin, yStepperMax, ax, dax);

					TVSOutput v;
					uint32* colorPtr = renderTarget.colorBuffer->mData + y * renderTarget.colorBuffer->mRowStride + xStart;
					float* depthPtr = nullptr;
					if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
					{
						depthPtr = renderTarget.depthBuffer->mData.data() + y * renderTarget.depthBuffer->mSize.x + xStart;
					}
#if USE_LANE_SCALAR
					constexpr int LaneCount = LaneScalar::Size;
					typename TVSOutput::LaneType laneV;
					LaneScalar laneDepth;
					int x = xStart;
					int const xLaneEnd = xStart + (xEnd - xStart) / LaneCount * LaneCount;
					for (; x < xLaneEnd; x += LaneCount)
					{
						xStepper.getLane(laneV, laneDepth);
						ProcessPixelFullLane< TDepthState, TBlendState, TVSOutput >(colorPtr, depthPtr, laneDepth, laneV, pixelShader);
						colorPtr += LaneCount;
						if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
						{
							depthPtr += LaneCount;
						}
						xStepper.advance(LaneCount);
					}
					if (x < xEnd)
					{
						int const tailCount = xEnd - x;
						LaneMask tailMask(_mm256_cmpgt_epi32(
							_mm256_set1_epi32(tailCount),
							_mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7)));
						xStepper.getLane(laneV, laneDepth);
						ProcessPixel< TDepthState, TBlendState, TVSOutput >(tailMask, colorPtr, depthPtr, laneDepth, laneV, pixelShader);
					}
#else
					for (int x = xStart; x < xEnd; ++x)
					{
						xStepper.get(v);
						ProcessPixel< TDepthState, TBlendState >(colorPtr, depthPtr, xStepper.depth, v, pixelShader);
						++colorPtr;
						if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
						{
							++depthPtr;
						}
						xStepper.advance();
					}
#endif
				}
				lineIter.advance();
				yStepperMin.advance();
				yStepperMax.advance();
			}
		}
	}

	FORCEINLINE float EdgeFunction(Vector2 const& a, Vector2 const& b, Vector2 const& p)
	{
		return (b - a).cross(p - a);
	}

	FORCEINLINE bool IsEdgeTileOutside(float e00, float e10, float e01, float e11, float orient)
	{
		return e00 * orient < 0.0f && e10 * orient < 0.0f && e01 * orient < 0.0f && e11 * orient < 0.0f;
	}

	FORCEINLINE bool IsEdgeTileInside(float e00, float e10, float e01, float e11, float orient)
	{
		return e00 * orient >= 0.0f && e10 * orient >= 0.0f && e01 * orient >= 0.0f && e11 * orient >= 0.0f;
	}

	template< class TDepthState, class TBlendState, class TVSOutput, class TPixelShader >
	void DrawTrianglePSEdge(RenderTarget& renderTarget,
						    RasterPosition const& p0, RasterPosition const& p1, RasterPosition const& p2,
						    TVSOutput const& vd0, TVSOutput const& vd1, TVSOutput const& vd2,
						    TPixelShader const& pixelShader)
	{
		Vector2 const& v0 = p0.screenPos;
		Vector2 const& v1 = p1.screenPos;
		Vector2 const& v2 = p2.screenPos;

		float const area = EdgeFunction(v0, v1, v2);
		if (Math::Abs(area) < 1e-4f)
			return;

		Vec2i size = renderTarget.colorBuffer->getSize();
		float minX = Math::Min(v0.x, Math::Min(v1.x, v2.x));
		float maxX = Math::Max(v0.x, Math::Max(v1.x, v2.x));
		float minY = Math::Min(v0.y, Math::Min(v1.y, v2.y));
		float maxY = Math::Max(v0.y, Math::Max(v1.y, v2.y));

		int xStart = Math::Max(0, Math::CeilToInt(minX - 0.5f));
		int xEnd = Math::Min(size.x, Math::CeilToInt(maxX - 0.5f));
		int yStart = Math::Max(0, Math::CeilToInt(minY - 0.5f));
		int yEnd = Math::Min(size.y, Math::CeilToInt(maxY - 0.5f));
		if (xStart >= xEnd || yStart >= yEnd)
			return;

		float const invArea = 1.0f / area;
		float const orient = (area > 0.0f) ? 1.0f : -1.0f;

		Vector2 const e12Delta = v2 - v1;
		Vector2 const e20Delta = v0 - v2;
		Vector2 const e01Delta = v1 - v0;
		float const e12dx = -e12Delta.y;
		float const e20dx = -e20Delta.y;
		float const e01dx = -e01Delta.y;
		float const e12dy = e12Delta.x;
		float const e20dy = e20Delta.x;
		float const e01dy = e01Delta.x;

		Vector2 rowSample(float(xStart) + 0.5f, float(yStart) + 0.5f);
		float rowE12 = EdgeFunction(v1, v2, rowSample);
		float rowE20 = EdgeFunction(v2, v0, rowSample);
		float rowE01 = EdgeFunction(v0, v1, rowSample);

		TPixelTriangleInterpolantsParam<TVSOutput> interpolants(vd0, vd1, vd2, p0.w, p1.w, p2.w);
		float const db0dx = e12dx * invArea;
		float const db1dx = e20dx * invArea;
		float const db2dx = e01dx * invArea;
		TVSOutput v;
#if USE_EDGE_TILE
		constexpr int TileSize = 8;

		auto processFullPixels = [&](int inXStart, int inXEnd, int inYStart, int inYEnd)
		{
			Vector2 tileSample(float(inXStart) + 0.5f, float(inYStart) + 0.5f);
			float tileRowE12 = EdgeFunction(v1, v2, tileSample);
			float tileRowE20 = EdgeFunction(v2, v0, tileSample);
			float tileRowE01 = EdgeFunction(v0, v1, tileSample);

			for (int y = inYStart; y < inYEnd; ++y)
			{
				float e12 = tileRowE12;
				float e20 = tileRowE20;
				float e01 = tileRowE01;
				typename TPixelTriangleInterpolantsParam<TVSOutput>::Stepper xStepper(
					interpolants,
					e12 * invArea, e20 * invArea, e01 * invArea,
					db0dx, db1dx, db2dx,
					p0.depth, p1.depth, p2.depth);

				uint32* colorPtr = renderTarget.colorBuffer->mData + y * renderTarget.colorBuffer->mRowStride + inXStart;
				float* depthPtr = nullptr;
				if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
				{
					depthPtr = renderTarget.depthBuffer->mData.data() + y * renderTarget.depthBuffer->mSize.x + inXStart;
				}

				for (int x = inXStart; x < inXEnd; ++x)
				{
					xStepper.get(v);
					ProcessPixel< TDepthState, TBlendState >(colorPtr, depthPtr, xStepper.depth, v, pixelShader);
					xStepper.advance();
					++colorPtr;
					if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
					{
						++depthPtr;
					}
				}

				tileRowE12 += e12dy;
				tileRowE20 += e20dy;
				tileRowE01 += e01dy;
			}
		};

		auto processPartialPixels = [&](int inXStart, int inXEnd, int inYStart, int inYEnd)
		{
			Vector2 tileSample(float(inXStart) + 0.5f, float(inYStart) + 0.5f);
			float tileRowE12 = EdgeFunction(v1, v2, tileSample);
			float tileRowE20 = EdgeFunction(v2, v0, tileSample);
			float tileRowE01 = EdgeFunction(v0, v1, tileSample);

			for (int y = inYStart; y < inYEnd; ++y)
			{
				float e12 = tileRowE12;
				float e20 = tileRowE20;
				float e01 = tileRowE01;
				typename TPixelTriangleInterpolantsParam<TVSOutput>::Stepper xStepper(
					interpolants,
					e12 * invArea, e20 * invArea, e01 * invArea,
					db0dx, db1dx, db2dx,
					p0.depth, p1.depth, p2.depth);

				uint32* colorPtr = renderTarget.colorBuffer->mData + y * renderTarget.colorBuffer->mRowStride + inXStart;
				float* depthPtr = nullptr;
				if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
				{
					depthPtr = renderTarget.depthBuffer->mData.data() + y * renderTarget.depthBuffer->mSize.x + inXStart;
				}

				for (int x = inXStart; x < inXEnd; ++x)
				{
					if (e12 * orient >= 0.0f && e20 * orient >= 0.0f && e01 * orient >= 0.0f)
					{
						xStepper.get(v);
						ProcessPixel< TDepthState, TBlendState >(colorPtr, depthPtr, xStepper.depth, v, pixelShader);
					}

					e12 += e12dx;
					e20 += e20dx;
					e01 += e01dx;
					xStepper.advance();
					++colorPtr;
					if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
					{
						++depthPtr;
					}
				}

				tileRowE12 += e12dy;
				tileRowE20 += e20dy;
				tileRowE01 += e01dy;
			}
		};

		for (int tileY = yStart; tileY < yEnd; tileY += TileSize)
		{
			int tileYEnd = Math::Min(tileY + TileSize, yEnd);
			for (int tileX = xStart; tileX < xEnd; tileX += TileSize)
			{
				int tileXEnd = Math::Min(tileX + TileSize, xEnd);

				Vector2 p00(float(tileX) + 0.5f, float(tileY) + 0.5f);
				Vector2 p10(float(tileXEnd) - 0.5f, float(tileY) + 0.5f);
				Vector2 p01(float(tileX) + 0.5f, float(tileYEnd) - 0.5f);
				Vector2 p11(float(tileXEnd) - 0.5f, float(tileYEnd) - 0.5f);

				float e12_00 = EdgeFunction(v1, v2, p00);
				float e12_10 = EdgeFunction(v1, v2, p10);
				float e12_01 = EdgeFunction(v1, v2, p01);
				float e12_11 = EdgeFunction(v1, v2, p11);
				if (IsEdgeTileOutside(e12_00, e12_10, e12_01, e12_11, orient))
					continue;

				float e20_00 = EdgeFunction(v2, v0, p00);
				float e20_10 = EdgeFunction(v2, v0, p10);
				float e20_01 = EdgeFunction(v2, v0, p01);
				float e20_11 = EdgeFunction(v2, v0, p11);
				if (IsEdgeTileOutside(e20_00, e20_10, e20_01, e20_11, orient))
					continue;

				float e01_00 = EdgeFunction(v0, v1, p00);
				float e01_10 = EdgeFunction(v0, v1, p10);
				float e01_01 = EdgeFunction(v0, v1, p01);
				float e01_11 = EdgeFunction(v0, v1, p11);
				if (IsEdgeTileOutside(e01_00, e01_10, e01_01, e01_11, orient))
					continue;

				bool bFullCover =
					IsEdgeTileInside(e12_00, e12_10, e12_01, e12_11, orient) &&
					IsEdgeTileInside(e20_00, e20_10, e20_01, e20_11, orient) &&
					IsEdgeTileInside(e01_00, e01_10, e01_01, e01_11, orient);

				if (bFullCover)
				{
					processFullPixels(tileX, tileXEnd, tileY, tileYEnd);
				}
				else
				{
					processPartialPixels(tileX, tileXEnd, tileY, tileYEnd);
				}
			}
		}
#else
		for (int y = yStart; y < yEnd; ++y)
		{
			float e12 = rowE12;
			float e20 = rowE20;
			float e01 = rowE01;
			typename TPixelTriangleInterpolantsParam<TVSOutput>::Stepper xStepper(
				interpolants,
				e12 * invArea, e20 * invArea, e01 * invArea,
				db0dx, db1dx, db2dx,
				p0.depth, p1.depth, p2.depth);

			uint32* colorPtr = renderTarget.colorBuffer->mData + y * renderTarget.colorBuffer->mRowStride + xStart;
			float* depthPtr = nullptr;
			if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
			{
				depthPtr = renderTarget.depthBuffer->mData.data() + y * renderTarget.depthBuffer->mSize.x + xStart;
			}

			for (int x = xStart; x < xEnd; ++x)
			{
				if (e12 * orient >= 0.0f && e20 * orient >= 0.0f && e01 * orient >= 0.0f)
				{
					xStepper.get(v);
					ProcessPixel< TDepthState, TBlendState >(colorPtr, depthPtr, xStepper.depth, v, pixelShader);
				}

				e12 += e12dx;
				e20 += e20dx;
				e01 += e01dx;
				xStepper.advance();
				++colorPtr;
				if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
				{
					++depthPtr;
				}
			}

			rowE12 += e12dy;
			rowE20 += e20dy;
			rowE01 += e01dy;
		}
#endif
	}


	template< class TDepthState , class TBlendState , class TVSOutput , class TPixelShader >
	void DrawTrianglePS(RenderTarget& renderTarget,
						RasterPosition const& p0, RasterPosition const& p1, RasterPosition const& p2,
					    TVSOutput const& vd0, TVSOutput const& vd1, TVSOutput const& vd2,
					    TPixelShader const& pixelShader)
	{
#if USE_EDGE_FUNCTION
		DrawTrianglePSEdge<TDepthState, TBlendState>(renderTarget, p0, p1, p2, vd0, vd1, vd2, pixelShader);
#else
		Vector2 maxV, minV, midV;
		RasterPosition const* maxP;
		RasterPosition const* minP;
		RasterPosition const* midP;
		TVSOutput const* maxVD;
		TVSOutput const* minVD;
		TVSOutput const* midVD;
		if( p0.screenPos.y > p1.screenPos.y )
		{
			minV = p1.screenPos; maxV = p0.screenPos;
			minP = &p1; maxP = &p0;
			minVD = &vd1, maxVD = &vd0;
		}
		else
		{
			minV = p0.screenPos; maxV = p1.screenPos;
			minP = &p0; maxP = &p1;
			minVD = &vd0, maxVD = &vd1;
		}
		if( p2.screenPos.y > maxV.y )
		{
			midV = maxV; maxV = p2.screenPos;
			midP = maxP; maxP = &p2;
			midVD = maxVD; maxVD = &vd2;
		}
		else if( p2.screenPos.y < minV.y )
		{
			midV = minV; minV = p2.screenPos;
			midP = minP; minP = &p2;
			midVD = minVD; minVD = &vd2;
		}
		else
		{
			midV = p2.screenPos;
			midP = &p2;
			midVD = &vd2;
		}

		Vector2 delta = maxV - minV;

		//constexpr float RasterEpsilon = 1e-4f;
		constexpr float RasterEpsilon = 0;
		if( Math::Abs(delta.y) < RasterEpsilon )
		{
			return;
		}

		float dxdy = delta.x / delta.y;

		Vec2i rtSize = renderTarget.colorBuffer->getSize();

		Vector2 deltaB = midV - minV;
		float splitAlpha = deltaB.y / delta.y;
		RasterPosition splitP;
		splitP.screenPos = minV + splitAlpha * delta;
		splitP.w = Math::Lerp(minP->w, maxP->w, splitAlpha);
		splitP.depth = Math::Lerp(minP->depth, maxP->depth, splitAlpha);
		TVSOutput pVD = PerspectiveLerp(*minVD, *maxVD, minP->w, maxP->w, splitAlpha);
		if( Math::Abs(deltaB.y) > RasterEpsilon )
		{
			float dxdySide = deltaB.x / deltaB.y;
			int yStart = Math::Max(0, Math::FloorToInt(minV.y + 0.5f + RasterEpsilon));
			int yEnd = Math::Min(rtSize.y, Math::FloorToInt(midV.y + 0.5f + RasterEpsilon));
			if (yStart < yEnd)
			{
				float scanY = yStart + 0.5f;
				float xSide = minV.x + dxdySide * (scanY - minV.y);
				float xLong = minV.x + dxdy * (scanY - minV.y);

				ScanLineIterator lineIter;
				lineIter.yStart = yStart;
				lineIter.yEnd = yEnd;
				lineIter.yMin = minV.y;
				lineIter.yMax = midV.y;
				float px = minV.x + dxdy * deltaB.y;
				if( px > midV.x )
				{
					lineIter.dxdyMin = dxdySide;
					lineIter.dxdyMax = dxdy;
					lineIter.xMin = xSide;
					lineIter.xMax = xLong;
					ProcessScanLines<TDepthState, TBlendState>(renderTarget, lineIter, *midP, *midVD, splitP, pVD, *minP, *minVD, true, pixelShader);
				}
				else
				{
					lineIter.dxdyMax = dxdySide;
					lineIter.dxdyMin = dxdy;
					lineIter.xMax = xSide;
					lineIter.xMin = xLong;
					ProcessScanLines<TDepthState, TBlendState>(renderTarget, lineIter, splitP, pVD, *midP, *midVD, *minP, *minVD, true, pixelShader);
				}
			}

		}

		Vector2 deltaT = maxV - midV;
		if( Math::Abs(deltaT.y) > RasterEpsilon )
		{
			float dxdySide = deltaT.x / deltaT.y;
			int yStart = Math::Max(0, Math::CeilToInt(midV.y - 0.5f - RasterEpsilon));
			int yEnd = Math::Min(rtSize.y, Math::CeilToInt(maxV.y - 0.5f - RasterEpsilon));
			if (yStart >= yEnd)
				return;

			float scanY = yStart + 0.5f;
			float xSide = midV.x + dxdySide * (scanY - midV.y);
			float xLong = minV.x + dxdy * (scanY - minV.y);

			ScanLineIterator lineIter;
			lineIter.yStart = yStart;
			lineIter.yEnd = yEnd;
			lineIter.yMin = midV.y;
			lineIter.yMax = maxV.y;
			float px = maxV.x - dxdy * deltaT.y;
			if( px > midV.x )
			{
				lineIter.dxdyMin = dxdySide;
				lineIter.dxdyMax = dxdy;
				lineIter.xMin = xSide;
				lineIter.xMax = xLong;
				ProcessScanLines<TDepthState, TBlendState>(renderTarget, lineIter, *midP, *midVD, splitP, pVD, *maxP, *maxVD, false, pixelShader);
			}
			else
			{
				lineIter.dxdyMax = dxdySide;
				lineIter.dxdyMin = dxdy;
				lineIter.xMax = xSide;
				lineIter.xMin = xLong;
				ProcessScanLines<TDepthState, TBlendState>(renderTarget, lineIter, splitP, pVD, *midP, *midVD, *maxP, *maxVD, false, pixelShader);
			}
		}
#endif
	}

	template< class TVSOutput >
	void DrawTriangle(RenderTarget& renderTarget, Vector2 const& v0, Vector2 const& v1, Vector2 const& v2,
					  TVSOutput const& vd0, TVSOutput const& vd1, TVSOutput const& vd2)
	{
		RasterPosition p0 = { v0, 1.0f, 0.0f };
		RasterPosition p1 = { v1, 1.0f, 0.0f };
		RasterPosition p2 = { v2, 1.0f, 0.0f };
		DrawTrianglePS<DepthDisableState, TBlendState<EBlendMode::Opaque>>(renderTarget, p0, p1, p2, vd0, vd1, vd2, VertexColorPixelShader{});
	}

	template< class TDepthState, class TBlendState, class TVSOutput, class TPixelShader >
	void DrawLinePS(RenderTarget& renderTarget, RasterPosition const& p0, RasterPosition const& p1, TVSOutput const& output0, TVSOutput const& output1, TPixelShader const& pixelShader);

	template< class TPipeline , class TPixelShader >
	void RasterizedRenderer::drawTrianglePS(Vector3 v0, LinearColor color0, Vector2 uv0, Vector3 v1, LinearColor color1, Vector2 uv1, Vector3 v2, LinearColor color2, Vector2 uv2, TPixelShader const& pixelShader)
	{
		DefaultVSOutput output0;
		output0.uv = uv0;
		output0.color = color0;

		DefaultVSOutput output1;
		output1.uv = uv1;
		output1.color = color1;

		DefaultVSOutput output2;
		output2.uv = uv2;
		output2.color = color2;

		drawTrianglePS<TPipeline>(
			Vector4(v0, 1) * worldToClip, output0,
			Vector4(v1, 1) * worldToClip, output1,
			Vector4(v2, 1) * worldToClip, output2,
			pixelShader);
	}

	template< class TPipeline , class TVSOutput , class TPixelShader >
	void RasterizedRenderer::drawTrianglePS(Vector4 const& sv0, TVSOutput const& output0, Vector4 const& sv1, TVSOutput const& output1, Vector4 const& sv2, TVSOutput const& output2, TPixelShader const& pixelShader)
	{
		using TRasterState = typename TPipeline::RasterState;
		using TDepthState = typename TPipeline::DepthState;
		using TBlendState = typename TPipeline::BlendState;

		Vector4 const& clip0 = sv0;
		Vector4 const& clip1 = sv1;
		Vector4 const& clip2 = sv2;

		if (clip0.w <= 0.01f || clip1.w <= 0.01f || clip2.w <= 0.01f)
			return;

		RasterPosition p0;
		p0.screenPos = toScreenPos(clip0);
		RasterPosition p1;
		p1.screenPos = toScreenPos(clip1);
		RasterPosition p2;
		p2.screenPos = toScreenPos(clip2);

		float signedArea = (p1.screenPos - p0.screenPos).cross(p2.screenPos - p0.screenPos);
		if (Math::Abs(signedArea) < 1e-4f)
			return;

		if (TRasterState::ShouldCull(signedArea))
			return;

		p0.w = 1.0f / clip0.w;
		p0.depth = clip0.z / clip0.w;
		p1.w = 1.0f / clip1.w;
		p1.depth = clip1.z / clip1.w;
		p2.w = 1.0f / clip2.w;
		p2.depth = clip2.z / clip2.w;

		DrawTrianglePS<TDepthState, TBlendState>(mRenderTarget, p0, p1, p2, output0, output1, output2, pixelShader);
	}

	template< class TPipeline >
	void RasterizedRenderer::drawTriangle(Vector3 v0, LinearColor color0, Vector2 uv0, Vector3 v1, LinearColor color1, Vector2 uv1, Vector3 v2, LinearColor color2, Vector2 uv2)
	{
		drawTrianglePS<TPipeline>(v0, color0, uv0, v1, color1, uv1, v2, color2, uv2, VertexColorPixelShader{});
	}

	template< class TPipeline, class TVSOutput, class TPixelShader >
	void RasterizedRenderer::drawLinePS(Vector4 const& sv0, TVSOutput const& output0, Vector4 const& sv1, TVSOutput const& output1, TPixelShader const& pixelShader)
	{
		using TDepthState = typename TPipeline::DepthState;
		using TBlendState = typename TPipeline::BlendState;

		if (sv0.w <= 0.01f || sv1.w <= 0.01f)
			return;

		RasterPosition p0;
		p0.screenPos = toScreenPos(sv0);
		p0.w = 1.0f / sv0.w;
		p0.depth = sv0.z / sv0.w;

		RasterPosition p1;
		p1.screenPos = toScreenPos(sv1);
		p1.w = 1.0f / sv1.w;
		p1.depth = sv1.z / sv1.w;

		DrawLinePS<TDepthState, TBlendState>(mRenderTarget, p0, p1, output0, output1, pixelShader);
	}

	template< class TPipeline, class TVertex, class TVertexShader, class TPixelShader >
	void RasterizedRenderer::drawIndexedTriangleListPS(TVertex const* vertices, int numVertices, uint32 const* indices, int numIndices, TVertexShader const& vertexShader, TPixelShader const& pixelShader)
	{
		if (vertices == nullptr || indices == nullptr)
			return;

		int const numValidIndices = numIndices - numIndices % 3;
		for (int i = 0; i < numValidIndices; i += 3)
		{
			uint32 const index0 = indices[i];
			uint32 const index1 = indices[i + 1];
			uint32 const index2 = indices[i + 2];

			if (index0 >= uint32(numVertices) || index1 >= uint32(numVertices) || index2 >= uint32(numVertices))
				continue;

			auto v0 = vertexShader(vertices[index0]);
			auto v1 = vertexShader(vertices[index1]);
			auto v2 = vertexShader(vertices[index2]);
			drawTrianglePS<TPipeline>(v0.svPosition, v0.output,
									  v1.svPosition, v1.output,
									  v2.svPosition, v2.output,
									  pixelShader);
		}
	}

	template< class TDepthState, class TBlendState, class TVSOutput, class TPixelShader >
	void DrawLinePS(RenderTarget& renderTarget, RasterPosition const& p0, RasterPosition const& p1, TVSOutput const& output0, TVSOutput const& output1, TPixelShader const& pixelShader)
	{
		Vec2i bufferSize = renderTarget.colorBuffer->getSize();
		Vector2 delta = p1.screenPos - p0.screenPos;
		Vector2 deltaAbs = Vector2(Math::Abs(delta.x), Math::Abs(delta.y));
		float steps = Math::Max(deltaAbs.x, deltaAbs.y);

		if (steps < 1.0f)
		{
			int x = Math::FloorToInt(p0.screenPos.x);
			int y = Math::FloorToInt(p0.screenPos.y);
			if (0 <= x && x < bufferSize.x && 0 <= y && y < bufferSize.y)
			{
				uint32* colorPtr = renderTarget.colorBuffer->mData + y * renderTarget.colorBuffer->mRowStride + x;
				float* depthPtr = nullptr;
				if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
				{
					depthPtr = renderTarget.depthBuffer->mData.data() + y * renderTarget.depthBuffer->mSize.x + x;
				}
				ProcessPixel< TDepthState, TBlendState >(colorPtr, depthPtr, p0.depth, output0, pixelShader);
			}
			return;
		}

		float dAlpha = 1.0f / steps;
		typename TPixelInterpolantsParam<TVSOutput>::Stepper stepper(output0, output1, p0.w, p1.w, 0.0f, dAlpha, p0.depth, p1.depth - p0.depth);

		Vector2 pos = p0.screenPos;
		Vector2 dPos = delta * dAlpha;
		TVSOutput output;
		for (int i = 0; i <= Math::FloorToInt(steps); ++i)
		{
			int x = Math::FloorToInt(pos.x);
			int y = Math::FloorToInt(pos.y);
			if (0 <= x && x < bufferSize.x && 0 <= y && y < bufferSize.y)
			{
				stepper.get(output);
				uint32* colorPtr = renderTarget.colorBuffer->mData + y * renderTarget.colorBuffer->mRowStride + x;
				float* depthPtr = nullptr;
				if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
				{
					depthPtr = renderTarget.depthBuffer->mData.data() + y * renderTarget.depthBuffer->mSize.x + x;
				}
				ProcessPixel< TDepthState, TBlendState >(colorPtr, depthPtr, stepper.depth, output, pixelShader);
			}

			pos += dPos;
			stepper.advance();
		}
	}

	template< class TPipeline, class TVertex, class TVertexShader >
	void RasterizedRenderer::drawIndexedTriangleList(TVertex const* vertices, int numVertices, uint32 const* indices, int numIndices, TVertexShader const& vertexShader)
	{
		drawIndexedTriangleListPS<TPipeline>(vertices, numVertices, indices, numIndices, vertexShader, VertexColorPixelShader{});
	}

	void TestStage::togglePause()
	{
		bPause = !bPause;

		if (mPauseButton)
		{
			mPauseButton->setTitle(bPause ? "Resume" : "Pause");
		}

	}

	bool TestStage::onInit()
	{
		if( !BaseClass::onInit() )
			return false;


		VERIFY_RETURN_FALSE( mColorBuffer.create(::Global::GetScreenSize()) );
		VERIFY_RETURN_FALSE( mDepthBuffer.create(::Global::GetScreenSize()) );

		VERIFY_RETURN_FALSE(mRenderer.init());

		VERIFY_RETURN_FALSE(mRTRenderer.init());


#if 1
		char const* texPath =
#if 0
			"Texture/tile1.tga";
#else
			"Texture/rocks.png";
#endif
		VERIFY_RETURN_FALSE(simpleTexture.load(texPath));
#endif

		setupScene();

		::Global::GUI().cleanupWidget();

		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addButton(UI_RESTART_GAME, "Restart");
		mPauseButton = frame->addButton("Pause", [this](int, GWidget*) -> bool
		{
			togglePause();
			return false;
		});

		restart();
		return true;
	}

	void TestStage::onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();


		g.beginRender();

		double renderTime = 0;
		uint64 renderCycleCount = 0;
		mPixelCountRendered = 0;

		if (bRender)
		{
			TIME_SCOPE(renderTime);
			uint64 cycleStart = ReadCPUCycles();
			RenderTarget renderTarget;
			renderTarget.colorBuffer = &mColorBuffer;
			renderTarget.depthBuffer = &mDepthBuffer;

			if (bRayTracerUsed)
			{
				renderTest2(renderTarget);
			}
			else
			{
				renderTest1(renderTarget);
			}
			renderCycleCount = ReadCPUCycles() - cycleStart;
		}


		mColorBuffer.draw(g);

		RenderUtility::SetBrush(g, EColor::Red);
		RenderUtility::SetPen(g, EColor::Red);
		g.drawRect(Vec2i(0, 0), Vec2i(100, 100));


		double pixelThroughputMpxPerSec = (renderTime > 0.0) ? (double(mPixelCountRendered) / (renderTime * 1000.0)) : 0.0;
		double cyclesPerPixel = (mPixelCountRendered > 0) ? (double(renderCycleCount) / mPixelCountRendered) : 0.0;
		g.drawText(Vec2i(10, 10), InlineString<>::Make("Frame: %.2f ms | Pixels: %d | Throughput: %.2f Mpx/s | Cycles/Pixel: %.1f", renderTime, mPixelCountRendered, pixelThroughputMpxPerSec, cyclesPerPixel));

		g.endRender();

		{
			PROFILE_ENTRY("bitbltPlatformBufferToRHI");
			::Global::GetDrawEngine().bitbltPlatformBufferToRHI();
		}

	}

	void TestStage::renderTest1(RenderTarget& renderTarget)
	{
		PROFILE_ENTRY("RasterizedRenderer");

		mRenderer.setRenderTarget(renderTarget);
		mRenderer.clearBuffer<DefaultRasterPipeline>(LinearColor(0.2, 0.2, 0.2, 0));

		using namespace Render;
		Vec2i screenSize = ::Global::GetScreenSize();
		float aspect = float(screenSize.x) / screenSize.y;
		mRenderer.worldToClip = LookAtMatrix(Vector3(0, -20, 10), Vector3(0, 1, -0.5), Vector3(0, 0, 1)) * PerspectiveMatrix(Math::DegToRad(90), aspect, 0.01, 500);
		mRenderer.viewportOrg = Vector2(0, 0);
		mRenderer.viewportSize = Vector2(::Global::GetScreenSize());

#if 1
		Vector3 cameraPos(0, -15, 8);
		Matrix4 model = Matrix4::Scale(4.0f) *
						Matrix4::Rotate(Vector3(0, 0, 1), mCubeRotateYaw) *
						Matrix4::Rotate(Vector3(1, 0, 0), mCubeRotatePitch) *
						Matrix4::Translate(Vector3(0, 0, 2));

		mRenderer.worldToClip = LookAtMatrix(cameraPos, Vector3(0, 0, 2) - cameraPos, Vector3(0, 0, 1)) *
								PerspectiveMatrix(Math::DegToRad(60), aspect, 0.01, 500);

		struct LaneDrawVertex
		{
			LaneVector3 pos;
			LaneLinearColor color;
			LaneVector2 uv;
		};


		struct DrawVertex
		{
			using LaneType = LaneDrawVertex;

			Vector3 pos;
			LinearColor color;
			Vector2 uv;
		};


		struct CubeMsh
		{
			CubeMsh()
			{

				Vector3 localVertices[] =
				{
					Vector3(-1, -1, -1), Vector3(1, -1, -1), Vector3(1,  1, -1), Vector3(-1,  1, -1),
					Vector3(-1, -1,  1), Vector3(1, -1,  1), Vector3(1,  1,  1), Vector3(-1,  1,  1),
				};

				struct Face
				{
					int index[4];
					LinearColor color;
				};

				Face faces[] =
				{
					{ { 0, 1, 5, 4 }, LinearColor(1.0f, 0.1f, 0.1f) },
					{ { 3, 7, 6, 2 }, LinearColor(0.1f, 1.0f, 0.1f) },
					{ { 0, 4, 7, 3 }, LinearColor(0.1f, 0.3f, 1.0f) },
					{ { 1, 2, 6, 5 }, LinearColor(1.0f, 0.9f, 0.1f) },
					{ { 0, 3, 2, 1 }, LinearColor(1.0f, 0.1f, 1.0f) },
					{ { 4, 5, 6, 7 }, LinearColor(0.1f, 1.0f, 1.0f) },
				};
				int numDrawVertices = 0;
				int numDrawIndices = 0;

				for (Face const& face : faces)
				{
					Vector3 const& p0 = localVertices[face.index[0]];
					Vector3 const& p1 = localVertices[face.index[1]];
					Vector3 const& p2 = localVertices[face.index[2]];
					Vector3 const& p3 = localVertices[face.index[3]];

					uint32 baseIndex = numDrawVertices;
					drawVertices[numDrawVertices++] = { p0, face.color, Vector2(0, 0) };
					drawVertices[numDrawVertices++] = { p1, face.color, Vector2(1, 0) };
					drawVertices[numDrawVertices++] = { p2, face.color, Vector2(1, 1) };
					drawVertices[numDrawVertices++] = { p3, face.color, Vector2(0, 1) };

					drawIndices[numDrawIndices++] = baseIndex;
					drawIndices[numDrawIndices++] = baseIndex + 1;
					drawIndices[numDrawIndices++] = baseIndex + 2;
					drawIndices[numDrawIndices++] = baseIndex;
					drawIndices[numDrawIndices++] = baseIndex + 2;
					drawIndices[numDrawIndices++] = baseIndex + 3;
				}
			}

			DrawVertex drawVertices[24];
			uint32 drawIndices[36];
		};

		static CubeMsh localMesh;

		struct MyVSOutput
		{
			struct LaneType
			{
				LaneLinearColor color;
				LaneVector2 uv;
			};

			LinearColor color;
			Vector2 uv;
		};

		Matrix4 localToClip = model * mRenderer.worldToClip;

		auto vertexShader = [this, &localToClip](DrawVertex const& input)
		{
			struct Output
			{
				Vector4 svPosition;
				MyVSOutput output;
			};

			Output result;
			result.svPosition = Vector4(input.pos, 1) * localToClip;
			result.output.uv = input.uv;
			result.output.color = input.color;
			return result;
		};

		auto pixelShader = [this](auto const& input)
		{
			mPixelCountRendered += std::is_same_v< std::decay_t< decltype(input)> , MyVSOutput > ? 1 : LaneScalar::Size;
			//return input.color;
			return simpleTexture.sample(input.uv) * input.color;
		};

		mRenderer.drawIndexedTriangleListPS<DefaultRasterPipeline>(
			localMesh.drawVertices, ARRAY_SIZE(localMesh.drawVertices), 
			localMesh.drawIndices, ARRAY_SIZE(localMesh.drawIndices), vertexShader, pixelShader);

#if 0
		mRenderer.drawTriangle(p0, face.color, Vector2(0, 0),
			p1, face.color, Vector2(1, 0),
			p2, face.color, Vector2(1, 1));

		mRenderer.drawTriangle(p0, face.color, Vector2(0, 0),
			p2, face.color, Vector2(1, 1),
			p3, face.color, Vector2(0, 1));
#endif
#if 0
		struct TestPixelShader
		{
			FORCEINLINE LinearColor operator()(DefaultVSOutput const& input) const
			{
				float checker = (Math::Fmod(10 * input.uv.x, 1.0f) > 0.5f) ==
							    (Math::Fmod(10 * input.uv.y, 1.0f) > 0.5f) ? 1.0f : 0.35f;
				return checker * input.color;
			}
		};

		mRenderer.drawTrianglePS<DefaultRasterPipeline>(Vector3(-20, 0, -25), LinearColor(1, 0, 0), Vector2(0, 0),
														Vector3(20, 0, -25), LinearColor(0, 1, 0), Vector2(1, 0),
														Vector3(0, 0, 10), LinearColor(0, 0, 1), Vector2(0.5, 1),
														TestPixelShader{});

		mRenderer.drawTriangle<DefaultRasterPipeline>(Vector3(-10, -10, 0), LinearColor(1, 1, 1), Vector2(0, 0),
													  Vector3(10, -10, 0), LinearColor(1, 1, 1), Vector2(1, 0),
													  Vector3(10, 0, 0), LinearColor(1, 1, 1), Vector2(1, 1));


		mRenderer.drawTriangle<DefaultRasterPipeline>(Vector3(-10, -10, 0), LinearColor(1, 1, 1), Vector2(0, 0),
													  Vector3(10, 0, 0), LinearColor(1, 1, 1), Vector2(1, 1),
													  Vector3(-10, 0, 0), LinearColor(1, 1, 1), Vector2(0, 1));
#endif

#endif
	}

	void TestStage::renderTest2(RenderTarget& renderTarget)
	{
		mRTRenderer.setRenderTarget(renderTarget);
		mRTRenderer.clearBuffer(LinearColor(0.2, 0.2, 0.2, 0));

		mCamera.lookAt(mCameraControl.getPos(), mCameraControl.getPos() + mCameraControl.getViewDir(), mCameraControl.getUpDir());

		mRTRenderer.render(mScene, mCamera);
	}

	void TestStage::setupScene()
	{
		mCamera.lookAt(Vector3(0, 5, 1), Vector3(0, 0, 0.5), Vector3(0, 0, 1));

		mCamera.fov = Math::DegToRad(70);
		mCamera.aspect = float(mColorBuffer.getSize().x) / mColorBuffer.getSize().y;

		//mCameraControl.lookAt(Vector3(0, 0, 20), Vector3(0, 0, -1), Vector3(0, 1, 0));
		mCameraControl.lookAt(Vector3(0, 5, 1), Vector3(0, 0, 0.5), Vector3(0, 0, 1));
		TRefCountPtr< PlaneShape > plane = new PlaneShape;

		TRefCountPtr< SphereShape > sphere = new SphereShape;
		sphere->radius = 0.5;

		MaterialPtr mat[3];
		mat[0] = new Material;
		mat[0]->emissiveColor = LinearColor(1, 0, 0);
		mat[1] = new Material;
		mat[1]->emissiveColor = LinearColor(0, 1, 0);
		mat[2] = new Material;
		mat[2]->emissiveColor = LinearColor(0, 0, 1);
		{
			PrimitiveObjectPtr obj = new PrimitiveObject;
			obj->shape = plane;
			obj->material = new Material;
			obj->material->emissiveColor = LinearColor(0.5, 0.5, 0.5);
			mScene.addPrimitive(obj);
		}

		if( 1 )
		{
			PrimitiveObjectPtr obj = new PrimitiveObject;
			obj->shape = plane;
			obj->transform.location = Vector3(0, 0, 20);
			obj->transform.rotation = Quaternion::Rotate( Vector3(1,0,0) , Math::DegToRad(180) );
			obj->material = new Material;
			obj->material->emissiveColor = LinearColor(0.5, 0.5, 0.8);
			mScene.addPrimitive(obj);
		}

		for( int i = 0; i < 10; ++i )
		{
			PrimitiveObjectPtr obj = new PrimitiveObject;
			obj->shape = sphere;
			obj->transform.location = Vector3(1.5 * (i - 5), 0, 0.5);
			obj->material = mat[i % 3];
			mScene.addPrimitive(obj);
		}
	}

	MsgReply TestStage::onKey(KeyMsg const& msg)
	{
		float baseImpulse = 500;
		switch (msg.getCode())
		{
		case EKeyCode::W: mCameraControl.moveForwardImpulse = msg.isDown() ? baseImpulse : 0; break;
		case EKeyCode::S: mCameraControl.moveForwardImpulse = msg.isDown() ? -baseImpulse : 0; break;
		case EKeyCode::D: mCameraControl.moveRightImpulse = msg.isDown() ? baseImpulse : 0; break;
		case EKeyCode::A: mCameraControl.moveRightImpulse = msg.isDown() ? -baseImpulse : 0; break;
		case EKeyCode::Z: mCameraControl.moveUp(0.5); break;
		case EKeyCode::X: mCameraControl.moveUp(-0.5); break;
		}


		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::P: togglePause(); break;
			case EKeyCode::F2: bRayTracerUsed = !bRayTracerUsed; break;
			}
		}
		return BaseClass::onKey(msg);
	}

	bool RayTraceRenderer::init()
	{
#if USE_OMP
		int cout = SystemPlatform::GetProcessorNumber();
		omp_set_num_threads(Math::Max(cout - 4, 1));
#endif
		return true;
	}

	void RayTraceRenderer::render(Scene& scene, Camera& camera)
	{
		Vector3 lookDir = camera.transform.transformVectorNoScale(Vector3(0, 0, 1));
		Vector3 axisX = camera.transform.transformVectorNoScale(Vector3(1, 0, 0));
		Vector3 axisY = camera.transform.transformVectorNoScale(Vector3(0, 1, 0));

		Vec2i screenSize = mRenderTarget.colorBuffer->getSize();

		Vector3 const pixelOffsetX = ( 2 * Math::Tan(0.5 * camera.fov) * camera.aspect / screenSize.x ) * axisX;
		Vector3 const pixelOffsetY = ( 2 * Math::Tan(0.5 * camera.fov) / screenSize.y ) * axisY;

		float const centerPixelPosX = 0.5 * screenSize.x + 0.5;
		float const centerPixelPosY = 0.5 * screenSize.y + 0.5;

#if USE_OMP
#pragma omp parallel for
#endif
		for( int j = 0; j < screenSize.y; ++j )
		{
			Vector3 offsetY = ( float(j) - centerPixelPosY) * pixelOffsetY;

			for( int i = 0; i < screenSize.x; ++i )
			{
				Vector3 offsetX = ( float(i) - centerPixelPosX ) * pixelOffsetX;

				RayTrace trace;
				trace.pos = camera.transform.location;
				trace.dir = Math::GetNormal(lookDir + offsetX + offsetY);

				RayHitResult result;
				if( !scene.raycast(trace, result) )
					continue;
				
				if( result.material )
				{
					float attenuation = Math::Clamp< float >(-result.normal.dot(trace.dir), 0, 1) /*/ Math::Squre(result.distance)*/;
					LinearColor c = result.material->emissiveColor;

					RayTrace reflectTrace;
					reflectTrace.dir = Math::GetNormal(trace.dir - 2 * trace.dir.projectNormal(result.normal));
					reflectTrace.pos = trace.pos + result.distance * trace.dir;

#if 0
					RayHitResult reflectResult;
					if( scene.raycast(reflectTrace, reflectResult) )
					{
						if( reflectResult.material )
						{
							c += reflectResult.material->emissiveColor;
							c *= attenuation;
						}
					}
#endif

					mRenderTarget.colorBuffer->setPixel(i, j, c.toBGRA());
				}
				else
				{
					Vector3 c = 0.5 * (result.normal + Vector3(1));
					mRenderTarget.colorBuffer->setPixel(i, j, LinearColor(c).toBGRA());
				}

			}
		}
	}

}
