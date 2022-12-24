#include "SoftwareRenderStage.h"

#include "ProfileSystem.h"

#define USE_OMP 1

#if USE_OMP
#include "omp.h"
#endif
#include "Image/ImageData.h"

REGISTER_STAGE_ENTRY("Software Renderer", SR::TestStage, EExecGroup::GraphicsTest, "Render");

namespace SR
{

	Texture simpleTexture;


	bool ColorBuffer::create(Vec2i const& size)
	{
		int const alignedSize = 4;
		mSize = size;
		mRowStride = alignedSize * ((size.x + alignedSize - 1) / alignedSize);

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
		Vector2 pos = Math::Clamp( UV , Vector2(0, 0) , Vector2(1, 1)).mul(mSize - Vec2i(1, 1));
		int x0 = Math::FloorToInt(pos.x);
		int y0 = Math::FloorToInt(pos.y);

		int x1 = std::min(x0 + 1, mSize.x - 1);
		int y1 = std::min(y0 + 1, mSize.y - 1);
		float dx = pos.x - x0;
		float dy = pos.y - y0;

		assert(0 <= x0 && x0 < mSize.x);
		assert(0 <= y0 && y0 < mSize.y);
		assert(0 <= x1 && x1 < mSize.x);
		assert(0 <= y1 && y1 < mSize.y);

#if 1
		LinearColor c00 = mData[x0 + y0* mSize.x];
		LinearColor c10 = mData[x1 + y0* mSize.x];
		LinearColor c01 = mData[x0 + y1* mSize.x];
		LinearColor c11 = mData[x1 + y1* mSize.x];
		return Math::LinearLerp(Math::LinearLerp(c00, c01, dy), Math::LinearLerp(c10, c11, dy), dx);
#else


		LinearColor c = mData[x1 + y1* mSize.x];

		return c;
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
		Vector2  uv;
		LinearColor color;
	};

	struct VertexLerpParams
	{
		TDataLerpParams<Vector2, VertexData, &VertexData::uv > uv;
		TDataLerpParams<LinearColor, VertexData, &VertexData::color > color;

		float w0;
		float dw;

		VertexLerpParams(VertexData const& v0, VertexData const& v1)
			:uv(v0, v1)
			,color(v0, v1)
		{
			w0 = v0.w;
			dw = v1.w - v0.w;
		}

		void perspectiveLerp(VertexData& outData, float alpha)
		{
			outData.w = w0 + alpha * dw;
			float wInv = 1.0 / outData.w;
			float alpha_w = alpha * wInv;
			uv.lerpTo(outData, wInv, alpha_w);
			color.lerpTo(outData, wInv, alpha_w);
		}
	};



	VertexData PerspectiveLerp(VertexData const& v0, VertexData const& v1, float alpha)
	{
		VertexData result;
		result.w = Math::Lerp(v0.w, v1.w, alpha);
		result.uv = PerspectiveLerp(v0.uv, v1.uv, v0.w, v1.w, result.w, alpha);
		result.color = PerspectiveLerp(v0.color, v1.color, v0.w, v1.w, result.w, alpha);
		return result;
	}


#if 1
	void ClipAndInterpolantColor(ColorBuffer& buffer, ScanLineIterator& lineIter, VertexData const& vL, VertexData const& vR, VertexData const& vS, bool bInverse)
	{
		Vec2i size = buffer.getSize();

		if (lineIter.yStart < 0)
			lineIter.yStart = 0;
		if (lineIter.yEnd > size.y)
			lineIter.yEnd = size.y;

		if (lineIter.yStart < lineIter.yEnd)
		{
			float day = 1 / (lineIter.yMax - lineIter.yMin);
			float ay = 0;

			if (bInverse)
			{
				day = -day;
				ay = 1;
			}

			VertexLerpParams lerpParamsMin(vL, vS);
			VertexLerpParams lerpParamsMax(vR, vS);

			for (int y = lineIter.yStart; y < lineIter.yEnd; ++y)
			{
				int xStart = PixelCut(lineIter.xMin);
				if (xStart < 0)
					xStart = 0;
				int xEnd = PixelCut(lineIter.xMax) + 1;
				if (xEnd > size.x)
					xEnd = size.x;

				if (xStart < xEnd)
				{
					VertexData vMin;
					lerpParamsMin.perspectiveLerp(vMin, ay);
					VertexData vMax;
					lerpParamsMax.perspectiveLerp(vMax, ay);

					float dax = 1 / (lineIter.xMax - lineIter.xMin);
					float ax = 0;

					VertexLerpParams lerpParamsX( vMin , vMax );

					for (int x = xStart; x < xEnd; ++x)
					{
						VertexData v;
						lerpParamsX.perspectiveLerp(v, ax);
#if 0

						if (Math::Fmod(10 * v.uv.x, 1.0) > 0.5 &&
							Math::Fmod(10 * v.uv.y, 1.0) > 0.5)
							buffer.setPixel(x, y, LinearColor(1, 1, 1, 1));
#else
						LinearColor destC = buffer.getPixel(x, y);
						buffer.setPixel(x, y, Math::LinearLerp(destC, v.color /** simpleTexture.sample(v.uv)*/, 1.0));

						buffer.setPixel(x, y, LinearColor(1 , 1, 1 , 1));

#endif
						ax += dax;
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



	void DrawTriangle(ColorBuffer& buffer, Vector2 const& v0, Vector2 const& v1, Vector2 const& v2,
					  VertexData const& vd0, VertexData const& vd1, VertexData const& vd2)
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

		if( Math::Abs(delta.y) < 1 )
		{
			return;
		}

		float dxdy = delta.x / delta.y;
		int yMid = PixelCut(midV.y);

		Vector2 deltaB = midV - minV;
		VertexData pVD = PerspectiveLerp(*minVD, *maxVD, deltaB.y / delta.y);
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
				ClipAndInterpolantColor(buffer, lineIter, *midVD, pVD, *minVD, true);
			}
			else
			{
				lineIter.dxdyMax = deltaB.x / deltaB.y;
				lineIter.dxdyMin = dxdy;
				ClipAndInterpolantColor(buffer, lineIter, pVD, *midVD, *minVD, true);
			}

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
				ClipAndInterpolantColor(buffer, lineIter, *midVD, pVD, *maxVD, false);
			}
			else
			{
				lineIter.dxdyMax = deltaT.x / deltaT.y;
				lineIter.dxdyMin = dxdy;
				lineIter.xMax = midV.x;
				lineIter.xMin = px;
				ClipAndInterpolantColor(buffer, lineIter, pVD, *midVD, *maxVD, false);
			}
		}
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



	void RasterizedRenderer::drawTriangle(Vector3 v0, LinearColor color0, Vector2 uv0, Vector3 v1, LinearColor color1, Vector2 uv1, Vector3 v2, LinearColor color2, Vector2 uv2)
	{
		Vector4 clip0 = Vector4(v0, 1) * worldToClip;
		Vector4 clip1 = Vector4(v1, 1) * worldToClip;
		Vector4 clip2 = Vector4(v2, 1) * worldToClip;
		VertexData vd0;
		vd0.uv = uv0;
		vd0.w = 1.0 / clip0.w;
		vd0.color = color0;

		VertexData vd1;
		vd1.uv = uv1;
		vd1.w = 1.0 / clip1.w;
		vd1.color = color1;

		VertexData vd2;
		vd2.uv = uv2;
		vd2.w = 1.0 / clip2.w;
		vd2.color = color2;

		DrawTriangle(*mRenderTarget.colorBuffer, toScreenPos(clip0), toScreenPos(clip1), toScreenPos(clip2),
					 vd0, vd1, vd2);
	}

	bool TestStage::onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		VERIFY_RETURN_FALSE( mColorBuffer.create(::Global::GetScreenSize()) );

		VERIFY_RETURN_FALSE(mRenderer.init());

		VERIFY_RETURN_FALSE(mRTRenderer.init());


		char const* texPath =
#if 0
			"Texture/tile1.tga";
#else
			"Texture/rocks.png";
#endif
		VERIFY_RETURN_FALSE(simpleTexture.load(texPath));

		setupScene();

		::Global::GUI().cleanupWidget();

		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addButton(UI_RESTART_GAME, "Restart");


		restart();
		return true;
	}

	void TestStage::renderTest1()
	{
		Graphics2D& g = Global::GetGraphics2D();

		RenderTarget renderTarget;
		renderTarget.colorBuffer = &mColorBuffer;
		mRenderer.setRenderTarget(renderTarget);
		mRenderer.clearBuffer(LinearColor(0.2, 0.2, 0.2, 0));

		//DrawLine(mColorBuffer, Vector2(0, 0), Vector2(100, 200), LinearColor(1, 0, 0));
		//DrawTriangle(mColorBuffer, Vector2(123, 100), Vector2(400, 200), Vector2(200, 300), LinearColor(1, 1, 0));
		//DrawTriangle(mColorBuffer, Vector2(400, 200), Vector2(200, 300), Vector2(400, 300), LinearColor(0, 1, 1));

		{
			VertexData vd0 = { 1 , Vector2(0,0) , LinearColor(1,0,0) };
			VertexData vd1 = { 1 , Vector2(1,0) ,LinearColor(0,1,0) };
			VertexData vd2 = { 1 , Vector2(1,1) ,LinearColor(0,0,1) };
			//DrawTriangle(mColorBuffer, Vector2(100, 400), Vector2(200, 500), Vector2(400, 300), vd0, vd1, vd2);
		}

		{
			VertexData vd0 = { 1 , Vector2(0,0) ,LinearColor(1,0,0) };
			VertexData vd1 = { 0.5 , Vector2(1,0) ,LinearColor(0,1,0) };
			VertexData vd2 = { 0.4 , Vector2(1,1) ,LinearColor(0,0,1) };
			//DrawTriangle(mColorBuffer, Vector2(300, 400), Vector2(400, 500), Vector2(600, 300), vd0, vd1, vd2);
		}
		using namespace Render;
		Vec2i screenSize = ::Global::GetScreenSize();
		float aspect = float(screenSize.x) / screenSize.y;
		mRenderer.worldToClip = Matrix4::Rotate(Vector3(0, 1, 0), angle) * mCameraControl.getViewMatrix() * PerspectiveMatrix(Math::Deg2Rad(90), aspect, 0.01, 500);
		mRenderer.viewportOrg = Vector2(0, 0);
		mRenderer.viewportSize = Vector2(::Global::GetScreenSize());

#if 1
		mRenderer.drawTriangle(Vector3(-10, -10, 0), LinearColor(1, 1, 1), Vector2(0, 0),
							   Vector3(10, -10, 0), LinearColor(1, 1, 1), Vector2(1, 0),
							   Vector3(10, 10, 0), LinearColor(1, 1, 1), Vector2(1, 1));


		mRenderer.drawTriangle(Vector3(-10, -10, 0), LinearColor(1, 1, 1), Vector2(0, 0),
							   Vector3(10, 10, 0), LinearColor(1, 1, 1), Vector2(1, 1),
							   Vector3(-10, 10, 0), LinearColor(1, 1, 1), Vector2(0, 1));

#endif

		mColorBuffer.draw(g);
	}

	void TestStage::renderTest2()
	{
		Graphics2D& g = Global::GetGraphics2D();

		RenderTarget renderTarget;
		renderTarget.colorBuffer = &mColorBuffer;
		mRTRenderer.setRenderTarget(renderTarget);
		mRTRenderer.clearBuffer(LinearColor(0.2, 0.2, 0.2, 0));

		mRTRenderer.render(mScene, mCamera);

		mColorBuffer.draw(g);
	}

	void TestStage::setupScene()
	{
		mCamera.lookAt(Vector3(0, 5, 1), Vector3(0, 0, 0.5), Vector3(0, 0, 1));
		mCamera.fov = Math::Deg2Rad(70);
		mCamera.aspect = float(mColorBuffer.getSize().x) / mColorBuffer.getSize().y;

		mCameraControl.lookAt(Vector3(0, 0, 20), Vector3(0, 0, -1), Vector3(0, 1, 0));

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
			obj->transform.rotation = Quaternion::Rotate( Vector3(1,0,0) , Math::Deg2Rad(180) );
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
			case EKeyCode::P: bPause = !bPause; break;
			case EKeyCode::F2: bRayTracerUsed = !bRayTracerUsed; break;
			}
		}
		return BaseClass::onKey(msg);
	}

	bool RayTraceRenderer::init()
	{
#if USE_OMP
		omp_set_num_threads(10);
#endif
		return true;
	}

	void RayTraceRenderer::render(Scene& scene, Camera& camera)
	{
		Vector3 lookDir = camera.transform.transformVectorNoScale(Vector3(0, 0, -1));
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
				
				if( result.material && 0 )
				{
					float attenuation = Math::Clamp< float >(-result.normal.dot(trace.dir), 0, 1) /*/ Math::Squre(result.distance)*/;
					LinearColor c = result.material->emissiveColor;

					RayTrace reflectTrace;
					reflectTrace.dir = Math::GetNormal(trace.dir - 2 * trace.dir.projectNormal(result.normal));
					reflectTrace.pos = trace.pos + result.distance * trace.dir;

					RayHitResult reflectResult;
					if( scene.raycast(reflectTrace, reflectResult) )
					{
						if( reflectResult.material )
						{
							c += reflectResult.material->emissiveColor;
							c *= attenuation;
						}

					}

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