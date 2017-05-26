#include "TestStageHeader.h"

#include "Win32Header.h"
#include "WidgetUtility.h"
#include "Math/Base.h"

#include "stb/stb_image.h"

#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

#include "RenderGL/GLUtility.h"

namespace SR
{
	using namespace RenderGL;
	struct Color
	{
		union
		{
			struct
			{
				uint8 b, g, r, a;
			};
			uint32 word;
		};
		
		operator uint32() const{  return word;  }

		Color(){}
		Color( uint8 r , uint8 g , uint8 b , uint8 a = 255 )
			:r(r),g(g),b(b),a(a){}
		Color( Color const& other ):word(other.word){}
	};

	struct LinearColor
	{
		float r, g, b, a;

		operator Color() const
		{
			uint8 byteR = uint8(255 * Math::Clamp<float>(r, 0, 1));
			uint8 byteG = uint8(255 * Math::Clamp<float>(g, 0, 1));
			uint8 byteB = uint8(255 * Math::Clamp<float>(b, 0, 1));
			uint8 byteA = uint8(255 * Math::Clamp<float>(a, 0, 1));
			return Color(byteR, byteG, byteB, byteA);
		}
		LinearColor operator + (LinearColor const& rhs) const
		{
			return LinearColor(r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a);
		}
		LinearColor operator * (LinearColor const& rhs) const
		{
			return LinearColor(r * rhs.r, g * rhs.g, b * rhs.b, a * rhs.a);
		}
		LinearColor() {}
		LinearColor(float r, float g, float b, float a = 1.0f)
			:r(r), g(g), b(b), a(a)
		{
		}
		LinearColor(Color const& rhs)
			:r(float(rhs.r)/255.0f), g(float(rhs.g) / 255.0f), b(float(rhs.b) / 255.0f), a(float(rhs.a) / 255.0f)
		{
		}
	};

	LinearColor operator * (float s, LinearColor const& rhs) { return LinearColor(s * rhs.r, s * rhs.g, s * rhs.b, s * rhs.a); }


	
	class ColorBuffer
	{
	public:
		bool create( Vec2i const& size)
		{
			int const alignedSize = 4;
			mSize = size;
			mRowStride = alignedSize * ( (size.x + alignedSize - 1 ) / alignedSize );

			BITMAPINFO bmpInfo;
			bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmpInfo.bmiHeader.biWidth  = mRowStride;
			bmpInfo.bmiHeader.biHeight = mSize.y;
			bmpInfo.bmiHeader.biPlanes = 1;
			bmpInfo.bmiHeader.biBitCount = 32;
			bmpInfo.bmiHeader.biCompression = BI_RGB;
			bmpInfo.bmiHeader.biXPelsPerMeter = 0;
			bmpInfo.bmiHeader.biYPelsPerMeter = 0;
			bmpInfo.bmiHeader.biSizeImage = 0;
			if( !mBufferDC.create( NULL , &bmpInfo, (void**)&mData) )
				return false;
			return true;
		}

		void clear( Color const& color )
		{
			std::fill_n(mData, mSize.y * mRowStride, uint32(color));
		}

		Color getPixel(uint32 x, uint32 y)
		{
			Color result;
			result.word = mData[(x + mRowStride * y)];
			return result;
		}

		void setPixel(uint32 x, uint32 y , Color color )
		{
			uint32* pData = mData + (x + mRowStride * y);
			*pData = color;
		}
		void setPixelCheck(uint32 x, uint32 y, Color color)
		{
			if( 0 <= x && x < mSize.x &&
			   0 <= y && y < mSize.y )
				setPixel(x, y, color);
		}

		void draw( Graphics2D& g )
		{
			mBufferDC.bitBlt( g.getRenderDC() , 0 , 0 );
		}

		Vec2i const& getSize() const { return mSize;  }

		Vec2i    mSize;
		uint32   mRowStride;
		uint32*  mData;
		BitmapDC mBufferDC;
	};

	struct PixelShaderType
	{


	};

	class Texture
	{
	public:
		bool load(char const* path)
		{
			int w;
			int h;
			int comp;
			unsigned char* image = stbi_load(path, &w, &h, &comp, STBI_default);

			if( !image )
				return false;

			mSize.x = w;
			mSize.y = h;

			
			//#TODO
			switch( comp )
			{
			case 3:
				{
					mData.resize(w * h);
					unsigned char* pPixel = image;
					for( int i = 0; i < mData.size(); ++i )
					{
						mData[i] = Color(pPixel[0], pPixel[1], pPixel[2], 0xff);
						pPixel += 3;
					}
				}
				break;
			case 4:
				{
					mData.resize(w * h);
					unsigned char* pPixel = image;
					for( int i = 0; i < mData.size(); ++i )
					{
						mData[i] = Color(pPixel[0], pPixel[1], pPixel[2], pPixel[3]);
						pPixel += 4;
					}
				}
				break;
			}
			//glGenerateMipmap( texType);
			stbi_image_free(image);
			return mData.empty() == false;
		}

		Color getColor(Vec2i const& pos) const
		{
			assert(0 <= pos.x && pos.x < mSize.x);
			assert(0 <= pos.y && pos.y < mSize.y);
			return mData[pos.x + pos.y * mSize.x];
		}

		LinearColor sample(Vec2f const& UV) const
		{
			Vec2f pos = UV.max( Vec2f(0, 0) ).min( Vec2f(1, 1) ).mul( mSize - Vec2i(1,1));
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

#if 0
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

		Vec2i mSize;
		std::vector< Color > mData;

	};


	void GenerateDistanceFieldTexture( Texture const& texture )
	{
		Vec2i size = texture.mSize;
		std::vector< Vec2i > countMap(size.x * size.y, Vec2i(0, 0));

		{
			{
				uint32 c = texture.getColor(Vec2i(0, 0));
				Vec2i& count = countMap[0 + size.x * 0];
				if( c & 0x00ffffff )
				{
					count.x += 1;
				}
				else
				{
					count.y += 1;
				}
			}
			for( int i = 1; i < size.x; ++i )
			{
				uint32 c = texture.getColor(Vec2i(i, 0));
				Vec2i& count = countMap[i + size.x * 0];
				Vec2i& countLeft = countMap[i - 1 + size.x * 0];

				count = countLeft;
				if( c & 0x00ffffff )
				{
					count.x += 1;
				}
				else
				{
					count.y += 1;
				}
			}
		}
		
		for( int j = 1; j < size.y; ++j )
		{
			{
				uint32 c = texture.getColor(Vec2i(0, j));
				Vec2i& countDown = countMap[0 + size.x * (j - 1)];
				Vec2i& count = countMap[0 + size.x * j];

				count = countDown;
				if( c & 0x00ffffff )
				{
					count.x += 1;
				}
				else
				{
					count.y += 1;
				}
			}
			for( int i = 1; i < size.x; ++i )
			{
				uint32 c = texture.getColor(Vec2i(i, j));
				Vec2i& count = countMap[i + size.x * 0];
				Vec2i& countLeft = countMap[i - 1 + size.x * j];
				Vec2i& countDown = countMap[i + size.x * (j - 1)];
				count = countLeft + countDown;
				if( c & 0x00ffffff )
				{
					count.x += 1;
				}
				else
				{
					count.y += 1;
				}
			}
		}

		for( int j = 0; j < size.y; ++j )
		{
			for( int i = 0; i < size.x; ++i )
			{
				uint32 c = texture.getColor(Vec2i(i, j));


			}
		}
	}
	Texture SimpleTexture;


	int PixelCut(float value)
	{
		return Math::FloorToInt(value + 0.5f);
	}

	struct VertexData
	{
		float  w;
		Vec2f  uv;
		LinearColor color;
	};

	// 1/z = 1/z0 * ( 1- a ) + 1 / z1 * a
	// v = v0 * ( z / z0 ) * ( 1 - a ) + v1 * ( z / z1 ) * a

	// w = w0 * ( 1- a ) + w1 * a => Dw = ( w1 - w0 ) / D
	// v = v0 * ( w0 / w ) * ( 1 - a ) + v1 * ( w1 / w ) * a
	template< class T >
	T PerspectiveLerp(T const& v0, T const& v1, float w0, float w1, float w , float alpha)
	{
		return  (w0 / w) * (1 - alpha) * v0 + (w1 / w) * alpha * v1;
	}

	VertexData PerspectiveLerp(VertexData const& v0, VertexData const& v1, float alpha)
	{
		VertexData result;
		result.w = Math::Lerp(v0.w, v1.w, alpha);
		result.uv = PerspectiveLerp(v0.uv, v1.uv, v0.w, v1.w, result.w, alpha);
		result.color = PerspectiveLerp(v0.color, v1.color, v0.w, v1.w, result.w, alpha);
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

	void ClipAndInterpolantColor(ColorBuffer& buffer, ScanLineIterator& lineIter, VertexData const& vL , VertexData const& vR , VertexData const& vS , bool bInverse )
	{
		Vec2i size = buffer.getSize();

		if( lineIter.yStart < 0 )
			lineIter.yStart = 0;
		if( lineIter.yEnd > size.y )
			lineIter.yEnd = size.y;

		if( lineIter.yStart < lineIter.yEnd )
		{
			float day = 1 / ( lineIter.yMax - lineIter.yMin );
			float ay = 0;
			for( int y = lineIter.yStart; y < lineIter.yEnd; ++y )
			{
				float temp = ay;

				int xStart = PixelCut(lineIter.xMin);
				if( xStart < 0 )
					xStart = 0;
				int xEnd = PixelCut(lineIter.xMax) + 1;
				if( xEnd > size.x )
					xEnd = size.x;

				if( xStart < xEnd )
				{
					if( bInverse )
						temp = 1 - temp;
					VertexData vMin = PerspectiveLerp(vL, vS, temp);
					VertexData vMax = PerspectiveLerp(vR, vS, temp);

					float dax = 1 / ( lineIter.xMax - lineIter.xMin );
					float ax = 0;
					for( int x = xStart; x < xEnd; ++x )
					{
						VertexData v = PerspectiveLerp(vMin, vMax, ax);
#if 0
						
						if ( Math::Fmod( 10 * v.uv.x , 1.0 ) > 0.5 &&
							 Math::Fmod(10 * v.uv.y, 1.0) > 0.5 )
							buffer.setPixel(x, y, LinearColor(1,1,1,1));
#else
						LinearColor destC = buffer.getPixel(x, y);
						buffer.setPixel( x , y , Math::LinearLerp( destC,  v.color * SimpleTexture.sample(v.uv)  , 0.8 ) );
	
#endif
						ax += dax;
					}
				}
				lineIter.advance();
				ay += day;
			}
		}
	}

	void DrawTriangle(ColorBuffer& buffer, Vec2f const& v0, Vec2f const& v1, Vec2f const& v2, 
					  VertexData const& vd0 , VertexData const& vd1  , VertexData const& vd2)
	{
		Vec2f maxV, minV, midV;
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

		Vec2f delta = maxV - minV;

		if( Math::Abs(delta.y) < 1 )
		{
			return;
		}

		float dxdy = delta.x / delta.y;
		int yMid = PixelCut(midV.y);

		Vec2f deltaB = midV - minV;
		VertexData pVD = PerspectiveLerp(*minVD, *maxVD, deltaB.y / delta.y );
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
				ClipAndInterpolantColor(buffer, lineIter, *midVD , pVD , *minVD , true);
			}
			else
			{
				lineIter.dxdyMax = deltaB.x / deltaB.y;
				lineIter.dxdyMin = dxdy;
				ClipAndInterpolantColor(buffer, lineIter, pVD, *midVD, *minVD, true);
			}
			
		}

		Vec2f deltaT = maxV - midV;
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

	void DrawTriangle(ColorBuffer& buffer, Vec2f const& v0, Vec2f const& v1, Vec2f const& v2, Color const& color)
	{
		Vec2f maxV, minV, midV;
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

		Vec2f delta = maxV - minV;

		if( Math::Abs(delta.y) < 1 )
		{
			return;
		}

		float dxdy = delta.x / delta.y;
		int yMid = PixelCut(midV.y);

		Vec2f deltaB = midV - minV;
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

		Vec2f deltaT = maxV - midV;
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

	void DrawLine(ColorBuffer& buffer, Vec2f const& from, Vec2f const& to , Color const& color )
	{
		Vec2i bufferSize = buffer.getSize();
		Vec2f delta = to - from;
		Vec2f deltaAbs = Vec2f(Math::Abs(delta.x), Math::Abs(delta.y));

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
				buffer.setPixelCheck(vi, i , color);
				v += dDelta;
			}
		}
	}

	class Renderer
	{
	public:

		bool init( Vec2i const& size )
		{
			if( !mColorBuffer.create( size ) )
				return false;

			return true;
		}
		void clearBuffer(LinearColor const& color)
		{
			mColorBuffer.clear(color);
		}


		Vec2f viewportOrg;
		Vec2f viewportSize;
		Matrix4 worldToClip;

		Vec2f toScreenPos(Vector4 clipPos)
		{
			return viewportOrg + viewportSize.mul(0.5 *Vec2f(clipPos.x / clipPos.w, clipPos.y / clipPos.w ) + Vec2f(0.5,0.5) );
		}

		void drawTriangle(Vector3 v0, LinearColor color0, Vec2f uv0,
						  Vector3 v1, LinearColor color1, Vec2f uv1,
						  Vector3 v2, LinearColor color2, Vec2f uv2)
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

			DrawTriangle(mColorBuffer, toScreenPos(clip0), toScreenPos(clip1), toScreenPos(clip2),
						 vd0, vd1, vd2);
		}

		void draw(Graphics2D& g)
		{
			mColorBuffer.draw(g);
		}


		ColorBuffer mColorBuffer;
	};

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:

		Renderer mRenderer;
		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			if( !mRenderer.init(::Global::getDrawEngine()->getScreenSize()) )
				return false;

			if( !SimpleTexture.load(
#if 0
				"Texture/tile1.tga"
#else
				"Texture/Gird.png"
#endif
			) )
				return false;

			::Global::GUI().cleanupWidget();

			DevFrame* frame = WidgetUtility::CreateDevFrame();
			frame->addButton(UI_RESTART_GAME, "Restart");

			restart();
			return true;
		}


		virtual void onEnd()
		{

			BaseClass::onEnd();
		}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			if ( !bPause )
			{
				for( int i = 0; i < frame; ++i )
					tick();
			}

			updateFrame(frame);
		}

		void onRender(float dFrame)
		{
			Graphics2D& g = Global::getGraphics2D();
			mRenderer.clearBuffer(LinearColor(0.2, 0.2 , 0.2, 0));

			DrawLine(mRenderer.mColorBuffer, Vec2f(0, 0), Vec2f(100, 200), LinearColor(1, 0, 0));
			DrawTriangle(mRenderer.mColorBuffer, Vec2f(123, 100), Vec2f(400, 200), Vec2f(200, 300), LinearColor(1, 1, 0));
			DrawTriangle(mRenderer.mColorBuffer, Vec2f(400, 200), Vec2f(200, 300), Vec2f(400, 300), LinearColor(0, 1, 1));

			{
				VertexData vd0 = { 1 , Vec2f(0,0) , LinearColor(1,0,0) };
				VertexData vd1 = { 1 , Vec2f(1,0) ,LinearColor(0,1,0) };
				VertexData vd2 = { 1 , Vec2f(1,1) ,LinearColor(0,0,1) };
				DrawTriangle(mRenderer.mColorBuffer, Vec2f(100, 400), Vec2f(200, 500), Vec2f(400, 300), vd0, vd1, vd2);
			}

			{
				VertexData vd0 = { 1 , Vec2f(0,0) ,LinearColor(1,0,0) };
				VertexData vd1 = { 0.5 , Vec2f(1,0) ,LinearColor(0,1,0) };
				VertexData vd2 = { 0.4 , Vec2f(1,1) ,LinearColor(0,0,1) };
				DrawTriangle(mRenderer.mColorBuffer, Vec2f(300, 400), Vec2f(400, 500), Vec2f(600, 300), vd0, vd1, vd2);
			}
			using namespace RenderGL;
			Vec2i screenSize = ::Global::getDrawEngine()->getScreenSize();
			float aspect = float(screenSize.x) / screenSize.y;
			mRenderer.worldToClip = Matrix4::Rotate( Vector3(0,1,0) , angle ) * LookAtMatrix( Vector3(0,0,20) , Vector3(0,0,-1), Vector3(0,1,0) ) * PerspectiveMatrix(Math::Deg2Rad(90), aspect, 0.01, 500);
			mRenderer.viewportOrg = Vec2f(0, 0);
			mRenderer.viewportSize = Vec2f(::Global::getDrawEngine()->getScreenSize());

			mRenderer.drawTriangle(Vector3(-10,-10, 0), LinearColor(1, 0, 0), Vec2f(0, 0) ,
								   Vector3( 10,-10, 0), LinearColor(1, 0, 0), Vec2f(1, 0) ,
								   Vector3( 10, 10, 0), LinearColor(1, 0, 0), Vec2f(1, 1));
			mRenderer.drawTriangle(Vector3(-10,-10, 0), LinearColor(0, 1, 0), Vec2f(0, 0),
								   Vector3( 10, 10, 0), LinearColor(0, 1, 0), Vec2f(1, 1),
								   Vector3(-10, 10, 0), LinearColor(0, 1, 0), Vec2f(0, 1));

			mRenderer.draw(g);
		}

		void restart()
		{

		}

		bool bPause = false;
		float angle = 0;

		void tick()
		{

			angle += float( gDefaultTickTime ) / 1000.0;
		}

		void updateFrame(int frame)
		{

		}

		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;

			switch( key )
			{
			case Keyboard::eR: restart(); break;
			case Keyboard::eP: bPause = !bPause; break;
			}
			return false;
		}


	};

}//namespace SWR


REGISTER_STAGE("Software Render", SR::TestStage, StageRegisterGroup::GraphicsTest);