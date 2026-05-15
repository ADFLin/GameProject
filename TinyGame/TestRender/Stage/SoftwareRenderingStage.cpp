#include "SoftwareRenderingStage.h"

#include "ProfileSystem.h"

#define USE_OMP 1

#if USE_OMP
#include "omp.h"
#endif
#include "Image/ImageData.h"
#include "SystemPlatform.h"
#include "BitUtility.h"
#include "DrawEngine.h"

namespace SR
{
	REGISTER_STAGE_ENTRY("Software Renderering", TestStage, EExecGroup::GraphicsTest, "Render");

	Texture simpleTexture;


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

		ColorBGRA8 const* row0 = mData.data() + y0 * mSize.x;
		ColorBGRA8 const* row1 = mData.data() + y1 * mSize.x;
		ColorBGRA8 const& c00 = row0[x0];
		ColorBGRA8 const& c10 = row0[x1];
		ColorBGRA8 const& c01 = row1[x0];
		ColorBGRA8 const& c11 = row1[x1];

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
	};

	template< typename TVertexData >
	struct TVertexLerpParams
	{
		static_assert(sizeof(TVertexData) % sizeof(float) == 0, "TVertexData must be float-packed");

		static constexpr int NumParams = sizeof(TVertexData) / sizeof(float);

		float w0;
		float dw;
		float wv0[NumParams];
		float dwv[NumParams];

		TVertexLerpParams(TVertexData const& v0, TVertexData const& v1, float inW0, float inW1)
		{
			float const* p0 = reinterpret_cast<float const*>(&v0);
			float const* p1 = reinterpret_cast<float const*>(&v1);

			w0 = inW0;
			dw = inW1 - inW0;

			for (int i = 0; i < NumParams; ++i)
			{
				wv0[i] = inW0 * p0[i];
				dwv[i] = inW1 * p1[i] - wv0[i];
			}
		}

		void perspectiveLerp(TVertexData& outData, float alpha, float& outW) const
		{
			float* pOut = reinterpret_cast<float*>(&outData);

			outW = w0 + alpha * dw;
			float const wInv = 1.0f / outW;
			float const alphaW = alpha * wInv;

			for (int i = 0; i < NumParams; ++i)
			{
				pOut[i] = wInv * wv0[i] + alphaW * dwv[i];
			}
		}

		struct Stepper
		{
			float w;
			float dw;
			float depth;
			float dDepth;

			float paramW[NumParams];
			float dParamW[NumParams];

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
				for (int i = 0; i < NumParams; ++i)
				{
					float wv0 = inW0 * p0[i];
					float dwv = inW1 * p1[i] - wv0;
					paramW[i] = wv0 + alpha * dwv;
					dParamW[i] = dAlpha * dwv;
				}
			}

			FORCEINLINE void get(TVertexData& outData) const
			{
				float* pOut = reinterpret_cast<float*>(&outData);
				float const wInv = 1.0f / w;

				for (int i = 0; i < NumParams; ++i)
				{
					pOut[i] = wInv * paramW[i];
				}
			}

			FORCEINLINE void advance()
			{
				w += dw;
				depth += dDepth;
				for (int i = 0; i < NumParams; ++i)
				{
					paramW[i] += dParamW[i];
				}
			}
		};
	};



	template< typename TVSOutput >
	TVSOutput PerspectiveLerp(TVSOutput const& v0, TVSOutput const& v1, float w0, float w1, float alpha)
	{
		TVertexLerpParams<TVSOutput> lerpParams(v0, v1, w0, w1);
		TVSOutput result;
		float w;
		lerpParams.perspectiveLerp(result, alpha, w);
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
		
		ColorBGRA8 colorOut = outC.toBGRA();
		//ColorBGRA8 colorOut = ColorBGRA8{0xff, 0 , 0};
		*colorPtr = colorOut.word;
		if constexpr (TDepthState::EnableWrite)
		{
			*depthPtr = depth;
		}
	}

	template< class TDepthState , class TBlendState , class TVSOutput , class TPixelShader >
	void ProcessPixels(RenderTarget& renderTarget, ScanLineIterator& lineIter,
					   RasterPosition const& pL, TVSOutput const& vL,
					   RasterPosition const& pR, TVSOutput const& vR,
					   RasterPosition const& pS, TVSOutput const& vS,
					   bool bInverse, TPixelShader const& pixelShader)
	{
		using LerpStepper = typename TVertexLerpParams<TVSOutput>::Stepper;

		Vec2i size = renderTarget.colorBuffer->getSize();

		if (lineIter.yStart < 0)
			lineIter.yStart = 0;
		if (lineIter.yEnd > size.y)
			lineIter.yEnd = size.y;

		
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
			LerpStepper lerpStepperMin(vL, vS, pL.w, pS.w, ay, day, depthMin0, dDepthMin);
			LerpStepper lerpStepperMax(vR, vS, pR.w, pS.w, ay, day, depthMax0, dDepthMax);

			for (int y = lineIter.yStart; y < lineIter.yEnd; ++y)
			{
				CHECK(lineIter.xMin <= lineIter.xMax);
				float xMin = lineIter.xMin;
				float xMax = lineIter.xMax;
				//constexpr float RasterEpsilon = 1e-4f;
				constexpr float RasterEpsilon = 0;
				int xStart = Math::CeilToInt(xMin - 0.5f - RasterEpsilon);
				if (xStart < 0)
					xStart = 0;
				int xEnd = Math::CeilToInt(xMax - 0.5f - RasterEpsilon);
				if (xEnd > size.x)
					xEnd = size.x;

				if (LIKELY(xStart < xEnd))
				{
					TVSOutput vMin;
					lerpStepperMin.get(vMin);
					TVSOutput vMax;
					lerpStepperMax.get(vMax);

					float dax = 1 / (lineIter.xMax - lineIter.xMin);
					float ax = (xStart + 0.5f - lineIter.xMin) * dax;

					LerpStepper lerpStepperX(vMin, vMax, lerpStepperMin.w, lerpStepperMax.w, ax, dax, lerpStepperMin.depth, lerpStepperMax.depth - lerpStepperMin.depth);

					TVSOutput v;
					uint32* colorPtr = renderTarget.colorBuffer->mData + y * renderTarget.colorBuffer->mRowStride + xStart;
					float* depthPtr = nullptr;
					if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
					{
						depthPtr = renderTarget.depthBuffer->mData.data() + y * renderTarget.depthBuffer->mSize.x + xStart;
					}
					for (int x = xStart; x < xEnd; ++x)
					{
						lerpStepperX.get(v);
						ProcessPixel< TDepthState, TBlendState >(colorPtr, depthPtr, lerpStepperX.depth, v, pixelShader);
						++colorPtr;
						if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
						{
							++depthPtr;
						}
						lerpStepperX.advance();
					}
				}
				lineIter.advance();
				lerpStepperMin.advance();
				lerpStepperMax.advance();
			}
		}
	}


	template< class TDepthState , class TBlendState , class TVSOutput , class TPixelShader >
	void DrawTrianglePS(RenderTarget& renderTarget,
						RasterPosition const& p0, RasterPosition const& p1, RasterPosition const& p2,
					    TVSOutput const& vd0, TVSOutput const& vd1, TVSOutput const& vd2,
					    TPixelShader const& pixelShader)
	{
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
			int yStart = Math::Max(0, Math::CeilToInt(minV.y - 0.5f - RasterEpsilon));
			int yEnd = Math::Min(renderTarget.colorBuffer->getSize().y, Math::CeilToInt(midV.y - 0.5f - RasterEpsilon));
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
					ProcessPixels<TDepthState, TBlendState>(renderTarget, lineIter, *midP, *midVD, splitP, pVD, *minP, *minVD, true, pixelShader);
				}
				else
				{
					lineIter.dxdyMax = dxdySide;
					lineIter.dxdyMin = dxdy;
					lineIter.xMax = xSide;
					lineIter.xMin = xLong;
					ProcessPixels<TDepthState, TBlendState>(renderTarget, lineIter, splitP, pVD, *midP, *midVD, *minP, *minVD, true, pixelShader);
				}
			}

		}

		Vector2 deltaT = maxV - midV;
		if( Math::Abs(deltaT.y) > RasterEpsilon )
		{
			float dxdySide = deltaT.x / deltaT.y;
			int yStart = Math::Max(0, Math::CeilToInt(midV.y - 0.5f - RasterEpsilon));
			int yEnd = Math::Min(renderTarget.colorBuffer->getSize().y, Math::CeilToInt(maxV.y - 0.5f - RasterEpsilon));
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
				ProcessPixels<TDepthState, TBlendState>(renderTarget, lineIter, *midP, *midVD, splitP, pVD, *maxP, *maxVD, false, pixelShader);
			}
			else
			{
				lineIter.dxdyMax = dxdySide;
				lineIter.dxdyMin = dxdy;
				lineIter.xMax = xSide;
				lineIter.xMin = xLong;
				ProcessPixels<TDepthState, TBlendState>(renderTarget, lineIter, splitP, pVD, *midP, *midVD, *maxP, *maxVD, false, pixelShader);
			}
		}
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
		typename TVertexLerpParams<TVSOutput>::Stepper lerpStepper(output0, output1, p0.w, p1.w, 0.0f, dAlpha, p0.depth, p1.depth - p0.depth);

		Vector2 pos = p0.screenPos;
		Vector2 dPos = delta * dAlpha;
		TVSOutput output;
		for (int i = 0; i <= Math::FloorToInt(steps); ++i)
		{
			int x = Math::FloorToInt(pos.x);
			int y = Math::FloorToInt(pos.y);
			if (0 <= x && x < bufferSize.x && 0 <= y && y < bufferSize.y)
			{
				lerpStepper.get(output);
				uint32* colorPtr = renderTarget.colorBuffer->mData + y * renderTarget.colorBuffer->mRowStride + x;
				float* depthPtr = nullptr;
				if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
				{
					depthPtr = renderTarget.depthBuffer->mData.data() + y * renderTarget.depthBuffer->mSize.x + x;
				}
				ProcessPixel< TDepthState, TBlendState >(colorPtr, depthPtr, lerpStepper.depth, output, pixelShader);
			}

			pos += dPos;
			lerpStepper.advance();
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

		if (bRender)
		{
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
		}


		mColorBuffer.draw(g);

		RenderUtility::SetBrush(g, EColor::Red);
		RenderUtility::SetPen(g, EColor::Red);
		g.drawRect(Vec2i(0, 0), Vec2i(100, 100));

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



		struct DrawVertex
		{
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
			LinearColor color;
			//Vector2 uv;
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
			//result.output.uv = input.uv;
			result.output.color = input.color;
			return result;
		};

		auto pixelShader = [this](MyVSOutput const& input)
		{
			return input.color;
			//return simpleTexture.sample(input.uv) * input.color;
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
