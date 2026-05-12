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
					mData[i] = Color(pPixel[0], pPixel[1], pPixel[2], 0xff);
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
					mData[i] = Color(pPixel[0], pPixel[1], pPixel[2], pPixel[3]);
					pPixel += 4;
				}
			}
			break;
		}

		return mData.empty() == false;
	}

	Color Texture::getColor(Vec2i const& pos) const
	{
		assert(0 <= pos.x && pos.x < mSize.x);
		assert(0 <= pos.y && pos.y < mSize.y);
		return mData[pos.x + pos.y * mSize.x];
	}

	LinearColor Texture::sample(Vector2 const& UV) const
	{
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

		Color const* row0 = mData.data() + y0 * mSize.x;
		Color const* row1 = mData.data() + y1 * mSize.x;
		Color const& c00 = row0[x0];
		Color const& c10 = row0[x1];
		Color const& c01 = row1[x0];
		Color const& c11 = row1[x1];

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

	template< class T , class TVertexData, T TVertexData::*Member >
	struct TDataLerpParams
	{
		T wv0;
		T dwv;

		TDataLerpParams(TVertexData const& v0, TVertexData const& v1)
		{
			wv0 =   v0.w * (v0.*Member);
			T temp = v1.w * (v1.*Member);
			dwv = temp - wv0;
		}

		void lerpTo(TVertexData& to, float wInv , float alpha_w)
		{
			(to.*Member) = wInv * wv0 + (alpha_w) * dwv;
		}
	};

	struct VertexData
	{
		float    w;
		float    depth;
		Vector2  uv;
		LinearColor color;
	};

	struct VertexColorPixelShader
	{
		FORCEINLINE LinearColor operator()(VertexData const& input) const
		{
			return input.color;
		}
	};

	struct VertexLerpParams
	{
		TDataLerpParams<Vector2, VertexData, &VertexData::uv > uv;
		TDataLerpParams<LinearColor, VertexData, &VertexData::color > color;

		float w0;
		float dw;
		float depth0;
		float dDepth;

		VertexLerpParams(VertexData const& v0, VertexData const& v1)
			:uv(v0, v1)
			,color(v0, v1)
		{
			w0 = v0.w;
			dw = v1.w - v0.w;
			depth0 = v0.depth;
			dDepth = v1.depth - v0.depth;
		}

		void perspectiveLerp(VertexData& outData, float alpha)
		{
			outData.w = w0 + alpha * dw;
			outData.depth = depth0 + alpha * dDepth;
			float wInv = 1.0 / outData.w;
			float alpha_w = alpha * wInv;
			uv.lerpTo(outData, wInv, alpha_w);
			color.lerpTo(outData, wInv, alpha_w);
		}
	};

	struct VertexLerpStepper
	{
		float w;
		float dw;
		float depth;
		float dDepth;
		Vector2 uvW;
		Vector2 duvW;
		LinearColor colorW;
		LinearColor dColorW;

		VertexLerpStepper(VertexLerpParams const& params, float alpha, float dAlpha)
		{
			w = params.w0 + alpha * params.dw;
			dw = dAlpha * params.dw;
			depth = params.depth0 + alpha * params.dDepth;
			dDepth = dAlpha * params.dDepth;
			uvW = params.uv.wv0 + alpha * params.uv.dwv;
			duvW = dAlpha * params.uv.dwv;
			colorW = params.color.wv0 + alpha * params.color.dwv;
			dColorW = dAlpha * params.color.dwv;
		}

		FORCEINLINE void get(VertexData& outData) const
		{
			float wInv = 1.0f / w;
			outData.w = w;
			outData.depth = depth;
			outData.uv = wInv * uvW;
			outData.color = wInv * colorW;
		}

		FORCEINLINE void advance()
		{
			w += dw;
			depth += dDepth;
			uvW += duvW;
			colorW += dColorW;
		}
	};

	VertexData PerspectiveLerp(VertexData const& v0, VertexData const& v1, float alpha)
	{
		VertexData result;
		result.w = Math::Lerp(v0.w, v1.w, alpha);
		result.depth = Math::Lerp(v0.depth, v1.depth, alpha);
		result.uv = PerspectiveLerp(v0.uv, v1.uv, v0.w, v1.w, result.w, alpha);
		result.color = PerspectiveLerp(v0.color, v1.color, v0.w, v1.w, result.w, alpha);
		return result;
	}


#if 1
	template< class TPixelShader >
	void ClipAndInterpolantColor(ColorBuffer& buffer, DepthBuffer* depthBuffer, ScanLineIterator& lineIter, VertexData const& vL, VertexData const& vR, VertexData const& vS, bool bInverse, TPixelShader const& pixelShader)
	{
		Vec2i size = buffer.getSize();

		if (lineIter.yStart < 0)
			lineIter.yStart = 0;
		if (lineIter.yEnd > size.y)
			lineIter.yEnd = size.y;

		if (lineIter.yStart < lineIter.yEnd)
		{
			float day = 1 / (lineIter.yMax - lineIter.yMin);
			float ay = (lineIter.yStart + 0.5f - lineIter.yMin) * day;

			if (bInverse)
			{
				day = -day;
				ay = 1 + (lineIter.yStart + 0.5f - lineIter.yMin) * day;
			}

			VertexLerpParams lerpParamsMin(vL, vS);
			VertexLerpParams lerpParamsMax(vR, vS);

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

				if (xStart < xEnd)
				{
					VertexData vMin;
					lerpParamsMin.perspectiveLerp(vMin, ay);
					VertexData vMax;
					lerpParamsMax.perspectiveLerp(vMax, ay);

					float dax = 1 / (lineIter.xMax - lineIter.xMin);
					float ax = (xStart + 0.5f - lineIter.xMin) * dax;

					VertexLerpParams lerpParamsX( vMin , vMax );
					VertexLerpStepper lerpStepperX(lerpParamsX, ax, dax);

					VertexData v;
					for (int x = xStart; x < xEnd; ++x)
					{
						lerpStepperX.get(v);
						if (depthBuffer == nullptr || depthBuffer->testAndSet(x, y, v.depth))
						{
							LinearColor destC = buffer.getPixel(x, y);
							buffer.setPixel(x, y, Math::LinearLerp(destC, pixelShader(v), 1.0));
						}
						lerpStepperX.advance();
					}
				}
				lineIter.advance();
				ay += day;
			}
		}
	}

#else

	void ClipAndInterpolantColor(ColorBuffer& buffer, ScanLineIterator& lineIter, VertexData const& vL, VertexData const& vR, VertexData const& vS, bool bInverse)
	{

		Vec2i size = buffer.getSize();

		if( lineIter.yStart < 0 )
			lineIter.yStart = 0;
		if( lineIter.yEnd > size.y )
			lineIter.yEnd = size.y;

		if( lineIter.yStart < lineIter.yEnd )
		{
			float day = 1 / (lineIter.yMax - lineIter.yMin);
			float ay = 0;
			if (bInverse)
			{
				day = -day;
				ay = 1;
			}

			for( int y = lineIter.yStart; y < lineIter.yEnd; ++y )
			{

				int xStart = PixelCut(lineIter.xMin);
				if( xStart < 0 )
					xStart = 0;
				int xEnd = PixelCut(lineIter.xMax) + 1;
				if( xEnd > size.x )
					xEnd = size.x;

				if( xStart < xEnd )
				{
					VertexData vMin = PerspectiveLerp(vL, vS, ay);
					VertexData vMax = PerspectiveLerp(vR, vS, ay);

					float dax = 1 / (lineIter.xMax - lineIter.xMin);
					float ax = 0;
					for( int x = xStart; x < xEnd; ++x )
					{
						VertexData v = PerspectiveLerp(vMin, vMax, ax);
#if 0

						if( Math::Fmod(10 * v.uv.x, 1.0) > 0.5 &&
						    Math::Fmod(10 * v.uv.y, 1.0) > 0.5 )
							buffer.setPixel(x, y, LinearColor(1, 1, 1, 1));
#else
						LinearColor destC = buffer.getPixel(x, y);
						buffer.setPixel(x, y, Math::LinearLerp(destC, v.color * simpleTexture.sample(v.uv), 0.8));
#endif
						ax += dax;
					}
				}
				lineIter.advance();
				ay += day;
			}
		}
	}
#endif



	template< class TPixelShader >
	void DrawTrianglePS(ColorBuffer& buffer, DepthBuffer* depthBuffer, Vector2 const& v0, Vector2 const& v1, Vector2 const& v2,
					    VertexData const& vd0, VertexData const& vd1, VertexData const& vd2,
					    TPixelShader const& pixelShader)
	{
		Vector2 maxV, minV, midV;
		VertexData const* maxVD;
		VertexData const* minVD;
		VertexData const* midVD;
		if( v0.y > v1.y )
		{
			minV = v1; maxV = v0;
			minVD = &vd1, maxVD = &vd0;
		}
		else
		{
			minV = v0; maxV = v1;
			minVD = &vd0, maxVD = &vd1;
		}
		if( v2.y > maxV.y )
		{
			midV = maxV; maxV = v2;
			midVD = maxVD; maxVD = &vd2;
		}
		else if( v2.y < minV.y )
		{
			midV = minV; minV = v2;
			midVD = minVD; minVD = &vd2;
		}
		else
		{
			midV = v2;
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
		VertexData pVD = PerspectiveLerp(*minVD, *maxVD, deltaB.y / delta.y);
		if( Math::Abs(deltaB.y) > RasterEpsilon )
		{
			float dxdySide = deltaB.x / deltaB.y;
			int yStart = Math::Max(0, Math::CeilToInt(minV.y - 0.5f - RasterEpsilon));
			int yEnd = Math::Min(buffer.getSize().y, Math::CeilToInt(midV.y - 0.5f - RasterEpsilon));
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
					ClipAndInterpolantColor(buffer, depthBuffer, lineIter, *midVD, pVD, *minVD, true, pixelShader);
				}
				else
				{
					lineIter.dxdyMax = dxdySide;
					lineIter.dxdyMin = dxdy;
					lineIter.xMax = xSide;
					lineIter.xMin = xLong;
					ClipAndInterpolantColor(buffer, depthBuffer, lineIter, pVD, *midVD, *minVD, true, pixelShader);
				}
			}

		}

		Vector2 deltaT = maxV - midV;
		if( Math::Abs(deltaT.y) > RasterEpsilon )
		{
			float dxdySide = deltaT.x / deltaT.y;
			int yStart = Math::Max(0, Math::CeilToInt(midV.y - 0.5f - RasterEpsilon));
			int yEnd = Math::Min(buffer.getSize().y, Math::CeilToInt(maxV.y - 0.5f - RasterEpsilon));
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
				ClipAndInterpolantColor(buffer, depthBuffer, lineIter, *midVD, pVD, *maxVD, false, pixelShader);
			}
			else
			{
				lineIter.dxdyMax = dxdySide;
				lineIter.dxdyMin = dxdy;
				lineIter.xMax = xSide;
				lineIter.xMin = xLong;
				ClipAndInterpolantColor(buffer, depthBuffer, lineIter, pVD, *midVD, *maxVD, false, pixelShader);
			}
		}
	}

	void DrawTriangle(ColorBuffer& buffer, Vector2 const& v0, Vector2 const& v1, Vector2 const& v2,
					  VertexData const& vd0, VertexData const& vd1, VertexData const& vd2)
	{
		DrawTrianglePS(buffer, nullptr, v0, v1, v2, vd0, vd1, vd2, VertexColorPixelShader{});
	}

	void ClipAndFillColor(ColorBuffer& buffer, ScanLineIterator& lineIter, Color const& color)
	{
		Vec2i size = buffer.getSize();

		if( lineIter.yStart < 0 )
			lineIter.yStart = 0;
		if( lineIter.yEnd > size.y )
			lineIter.yEnd = size.y;

		if( lineIter.yStart < lineIter.yEnd )
		{
			for( int y = lineIter.yStart; y < lineIter.yEnd; ++y )
			{
				int xStart = PixelCut(lineIter.xMin);
				if( xStart < 0 )
					xStart = 0;
				int xEnd = PixelCut(lineIter.xMax);
				if( xEnd > size.x )
					xEnd = size.x;

				if( xStart < xEnd )
				{
					for( int x = xStart; x < xEnd; ++x )
					{
						buffer.setPixel(x, y, color);
					}
				}
				lineIter.advance();
			}
		}
	}

	void DrawTriangle(ColorBuffer& buffer, Vector2 const& v0, Vector2 const& v1, Vector2 const& v2, Color const& color)
	{
		Vector2 maxV, minV, midV;
		if( v0.y > v1.y )
		{
			minV = v1; maxV = v0;
		}
		else
		{
			minV = v0; maxV = v1;
		}
		if( v2.y > maxV.y )
		{
			midV = maxV; maxV = v2;
		}
		else if( v2.y < minV.y )
		{
			midV = minV; minV = v2;
		}
		else
		{
			midV = v2;
		}

		Vector2 delta = maxV - minV;

		if( Math::Abs(delta.y) < 1 )
		{
			return;
		}

		float dxdy = delta.x / delta.y;
		int yMid = PixelCut(midV.y);

		Vector2 deltaB = midV - minV;
		if( Math::Abs(deltaB.y) > 1 )
		{
			ScanLineIterator lineIter;
			lineIter.yStart = PixelCut(minV.y);
			lineIter.yEnd = yMid;
			lineIter.yMin = minV.y;
			lineIter.yMax = midV.y;
			lineIter.xMin = minV.x;
			lineIter.xMax = minV.x;
			float px = minV.x + dxdy * deltaB.y;
			if( px > midV.x )
			{
				lineIter.dxdyMin = deltaB.x / deltaB.y;
				lineIter.dxdyMax = dxdy;
			}
			else
			{
				lineIter.dxdyMax = deltaB.x / deltaB.y;
				lineIter.dxdyMin = dxdy;
			}
			ClipAndFillColor(buffer, lineIter, color);
		}

		Vector2 deltaT = maxV - midV;
		if( Math::Abs(deltaT.y) > 1 )
		{
			ScanLineIterator lineIter;
			lineIter.yStart = yMid;
			lineIter.yEnd = PixelCut(maxV.y);
			lineIter.yMin = midV.y;
			lineIter.yMax = maxV.y;
			float px = maxV.x - dxdy * deltaT.y;
			if( px > midV.x )
			{
				lineIter.dxdyMin = deltaT.x / deltaT.y;
				lineIter.dxdyMax = dxdy;
				lineIter.xMin = midV.x;
				lineIter.xMax = px;
			}
			else
			{
				lineIter.dxdyMax = deltaT.x / deltaT.y;
				lineIter.dxdyMin = dxdy;
				lineIter.xMax = midV.x;
				lineIter.xMin = px;
			}
			ClipAndFillColor(buffer, lineIter, color);
		}
	}

	void DrawLine(ColorBuffer& buffer, Vector2 const& from, Vector2 const& to, Color const& color)
	{
		Vec2i bufferSize = buffer.getSize();
		Vector2 delta = to - from;
		Vector2 deltaAbs = Vector2(Math::Abs(delta.x), Math::Abs(delta.y));

		if( deltaAbs.x < 1 && deltaAbs.y < 1 )
		{
			int x = Math::FloorToInt(from.x);
			int y = Math::FloorToInt(from.y);
			buffer.setPixelCheck(x, y, color);
		}

		// #TODO : Clip line first
		if( deltaAbs.x > deltaAbs.y )
		{
			float dDelta = delta.y / delta.x;
			int start = Math::FloorToInt(from.x);
			int end = Math::FloorToInt(to.x);

			float v;
			if( start > end )
			{
				std::swap(start, end);
				dDelta = -dDelta;
				v = to.y;
			}
			else
			{
				v = from.y;
			}

			for( int i = start; i <= end; ++i )
			{
				int vi = Math::FloorToInt(v);
				buffer.setPixelCheck(i, vi, color);
				v += dDelta;
			}
		}
		else
		{
			float dDelta = delta.x / delta.y;
			int start = Math::FloorToInt(from.y);
			int end = Math::FloorToInt(to.y);

			float v;
			if( start > end )
			{
				std::swap(start, end);
				dDelta = -dDelta;
				v = to.x;
			}
			else
			{
				v = from.x;
			}

			for( int i = start; i <= end; ++i )
			{
				int vi = Math::FloorToInt(v);
				buffer.setPixelCheck(vi, i, color);
				v += dDelta;
			}
		}
	}



	template< class TPixelShader >
	void RasterizedRenderer::drawTrianglePS(Vector3 v0, LinearColor color0, Vector2 uv0, Vector3 v1, LinearColor color1, Vector2 uv1, Vector3 v2, LinearColor color2, Vector2 uv2, TPixelShader const& pixelShader)
	{
		Vector4 clip0 = Vector4(v0, 1) * worldToClip;
		Vector4 clip1 = Vector4(v1, 1) * worldToClip;
		Vector4 clip2 = Vector4(v2, 1) * worldToClip;

		if (clip0.w <= 0.01f || clip1.w <= 0.01f || clip2.w <= 0.01f)
			return;

		Vector2 screen0 = toScreenPos(clip0);
		Vector2 screen1 = toScreenPos(clip1);
		Vector2 screen2 = toScreenPos(clip2);

		float signedArea = (screen1 - screen0).cross(screen2 - screen0);
		if (Math::Abs(signedArea) < 1e-4f)
			return;

		switch (cullMode)
		{
		case ECullMode::Back:
			if (signedArea >= 0)
				return;
			break;
		case ECullMode::Front:
			if (signedArea <= 0)
				return;
			break;
		default:
			break;
		}

		VertexData vd0;
		vd0.uv = uv0;
		vd0.w = 1.0 / clip0.w;
		vd0.depth = clip0.z / clip0.w;
		vd0.color = color0;

		VertexData vd1;
		vd1.uv = uv1;
		vd1.w = 1.0 / clip1.w;
		vd1.depth = clip1.z / clip1.w;
		vd1.color = color1;

		VertexData vd2;
		vd2.uv = uv2;
		vd2.w = 1.0 / clip2.w;
		vd2.depth = clip2.z / clip2.w;
		vd2.color = color2;

		DrawTrianglePS(*mRenderTarget.colorBuffer, mRenderTarget.depthBuffer, screen0, screen1, screen2, vd0, vd1, vd2, pixelShader);
	}

	void RasterizedRenderer::drawTriangle(Vector3 v0, LinearColor color0, Vector2 uv0, Vector3 v1, LinearColor color1, Vector2 uv1, Vector3 v2, LinearColor color2, Vector2 uv2)
	{
		drawTrianglePS(v0, color0, uv0, v1, color1, uv1, v2, color2, uv2, VertexColorPixelShader{});
	}

	template< class TPixelShader >
	void RasterizedRenderer::drawIndexedTriangleListPS(RasterVertex const* vertices, int numVertices, uint32 const* indices, int numIndices, TPixelShader const& pixelShader)
	{
		if (mRenderTarget.colorBuffer == nullptr || vertices == nullptr || indices == nullptr)
			return;

		int const numValidIndices = numIndices - numIndices % 3;
		for (int i = 0; i < numValidIndices; i += 3)
		{
			uint32 const index0 = indices[i];
			uint32 const index1 = indices[i + 1];
			uint32 const index2 = indices[i + 2];

			if (index0 >= uint32(numVertices) || index1 >= uint32(numVertices) || index2 >= uint32(numVertices))
				continue;

			RasterVertex const& v0 = vertices[index0];
			RasterVertex const& v1 = vertices[index1];
			RasterVertex const& v2 = vertices[index2];
			drawTrianglePS(v0.pos, v0.color, v0.uv,
						   v1.pos, v1.color, v1.uv,
						   v2.pos, v2.color, v2.uv,
						   pixelShader);
		}
	}

	void RasterizedRenderer::drawIndexedTriangleList(RasterVertex const* vertices, int numVertices, uint32 const* indices, int numIndices)
	{
		drawIndexedTriangleListPS(vertices, numVertices, indices, numIndices, VertexColorPixelShader{});
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

	void TestStage::renderTest1(RenderTarget& renderTarget)
	{
		PROFILE_ENTRY("RasterizedRenderer");

		mRenderer.setRenderTarget(renderTarget);
		mRenderer.clearBuffer(LinearColor(0.2, 0.2, 0.2, 0));

		//DrawLine(mColorBuffer, Vector2(0, 0), Vector2(100, 200), LinearColor(1, 0, 0));
		//DrawTriangle(mColorBuffer, Vector2(123, 100), Vector2(400, 200), Vector2(200, 300), LinearColor(1, 1, 0));
		//DrawTriangle(mColorBuffer, Vector2(400, 200), Vector2(200, 300), Vector2(400, 300), LinearColor(0, 1, 1));

		{
			VertexData vd0 = { 1 , 0 , Vector2(0,0) , LinearColor(1,0,0) };
			VertexData vd1 = { 1 , 0 , Vector2(1,0) ,LinearColor(0,1,0) };
			VertexData vd2 = { 1 , 0 , Vector2(1,1) ,LinearColor(0,0,1) };
			//DrawTriangle(mColorBuffer, Vector2(100, 400), Vector2(200, 500), Vector2(400, 300), vd0, vd1, vd2);
		}

		{
			VertexData vd0 = { 1 , 0 , Vector2(0,0) ,LinearColor(1,0,0) };
			VertexData vd1 = { 0.5 , 0 , Vector2(1,0) ,LinearColor(0,1,0) };
			VertexData vd2 = { 0.4 , 0 , Vector2(1,1) ,LinearColor(0,0,1) };
			//DrawTriangle(mColorBuffer, Vector2(300, 400), Vector2(400, 500), Vector2(600, 300), vd0, vd1, vd2);
		}
		using namespace Render;
		Vec2i screenSize = ::Global::GetScreenSize();
		float aspect = float(screenSize.x) / screenSize.y;
		mRenderer.worldToClip = LookAtMatrix(Vector3(0, -20, 10), Vector3(0, 1, -0.5), Vector3(0, 0, 1)) * PerspectiveMatrix(Math::DegToRad(90), aspect, 0.01, 500);
		mRenderer.viewportOrg = Vector2(0, 0);
		mRenderer.viewportSize = Vector2(::Global::GetScreenSize());

#if 1
		Vector3 cameraPos(0, -20, 10);
		Matrix4 model = Matrix4::Scale(4.0f) *
						Matrix4::Rotate(Vector3(0, 0, 1), mCubeRotateYaw) *
						Matrix4::Rotate(Vector3(1, 0, 0), mCubeRotatePitch) *
						Matrix4::Translate(Vector3(0, 0, 2));

		mRenderer.worldToClip = LookAtMatrix(cameraPos, Vector3(0, 0, 2) - cameraPos, Vector3(0, 0, 1)) *
								PerspectiveMatrix(Math::DegToRad(60), aspect, 0.01, 500);
		mRenderer.cullMode = RasterizedRenderer::ECullMode::Back;

		Vector3 localVertices[] =
		{
			Vector3(-1, -1, -1), Vector3( 1, -1, -1), Vector3( 1,  1, -1), Vector3(-1,  1, -1),
			Vector3(-1, -1,  1), Vector3( 1, -1,  1), Vector3( 1,  1,  1), Vector3(-1,  1,  1),
		};

		Vector3 vertices[8];
		for (int i = 0; i < 8; ++i)
		{
			vertices[i] = localVertices[i] * model;
		}

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

		RasterVertex drawVertices[24];
		uint32 drawIndices[36];
		int numDrawVertices = 0;
		int numDrawIndices = 0;

		for (Face const& face : faces)
		{
			Vector3 const& p0 = vertices[face.index[0]];
			Vector3 const& p1 = vertices[face.index[1]];
			Vector3 const& p2 = vertices[face.index[2]];
			Vector3 const& p3 = vertices[face.index[3]];

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

#if 0
			mRenderer.drawTriangle(p0, face.color, Vector2(0, 0),
								   p1, face.color, Vector2(1, 0),
								   p2, face.color, Vector2(1, 1));

			mRenderer.drawTriangle(p0, face.color, Vector2(0, 0),
								   p2, face.color, Vector2(1, 1),
								   p3, face.color, Vector2(0, 1));
#endif
		}


		auto pixelShader = [this](VertexData const& input)
		{
			return 	simpleTexture.sample(input.uv) * input.color;
		};

		mRenderer.drawIndexedTriangleListPS(drawVertices, numDrawVertices, drawIndices, numDrawIndices, pixelShader);

#if 0
		struct TestPixelShader
		{
			FORCEINLINE LinearColor operator()(VertexData const& input) const
			{
				float checker = (Math::Fmod(10 * input.uv.x, 1.0f) > 0.5f) ==
							    (Math::Fmod(10 * input.uv.y, 1.0f) > 0.5f) ? 1.0f : 0.35f;
				return checker * input.color;
			}
		};

		mRenderer.drawTrianglePS(Vector3(-20, 0, -25), LinearColor(1, 0, 0), Vector2(0, 0),
							     Vector3(20, 0, -25), LinearColor(0, 1, 0), Vector2(1, 0),
							     Vector3(0, 0, 10), LinearColor(0, 0, 1), Vector2(0.5, 1),
							     TestPixelShader{});

		mRenderer.drawTriangle(Vector3(-10, -10, 0), LinearColor(1, 1, 1), Vector2(0, 0),
							   Vector3(10, -10, 0), LinearColor(1, 1, 1), Vector2(1, 0),
							   Vector3(10, 0, 0), LinearColor(1, 1, 1), Vector2(1, 1));


		mRenderer.drawTriangle(Vector3(-10, -10, 0), LinearColor(1, 1, 1), Vector2(0, 0),
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

					mRenderTarget.colorBuffer->setPixel(i, j, c);
				}
				else
				{
					Vector3 c = 0.5 * (result.normal + Vector3(1));
					mRenderTarget.colorBuffer->setPixel(i, j, LinearColor(c));
				}

			}
		}
	}

}
