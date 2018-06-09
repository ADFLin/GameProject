#include "TinyGamePCH.h"
#include "DrawEngine.h"

#include "ProfileSystem.h"
#include "Thread.h"

#include "RHI/RHIGraphics2D.h"
#include "GLGraphics2D.h"
#include "GameGlobal.h"
#include "RenderUtility.h"
#include <algorithm>
#include <cmath>

#include "resource.h"

WORD GameWindow::getIcon()
{
	return IDI_ICON1;
}

WORD GameWindow::getSmallIcon()
{
	return IDI_ICON1;
}

void WINAPI GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
	if( type == GL_DEBUG_TYPE_OTHER )
		return;

	LogMsg(message);
}


template< class T >
class TGraphics2DProxy : public IGraphics2D
{
public:
	TGraphics2DProxy( T& g ):mImpl(g){}

	virtual void beginRender() override { mImpl.beginRender(); }
	virtual void endRender() override { mImpl.endRender(); }
	virtual void beginClip(Vec2i const& pos, Vec2i const& size) { mImpl.beginClip(pos, size); }
	virtual void endClip() { mImpl.endClip(); }
	virtual void beginBlend( Vec2i const& pos , Vec2i const& size , float alpha )  override { mImpl.beginBlend( pos , size , alpha ); }
	virtual void endBlend() override { mImpl.endBlend(); }
	virtual void setPen( Color3ub const& color ) override { mImpl.setPen( color ); }
	virtual void setBrush( Color3ub const& color ) override { mImpl.setBrush( color ); }
	virtual void drawPixel  ( Vec2i const& p , Color3ub const& color ) override { mImpl.drawPixel( p , color ); }
	virtual void drawLine   ( Vec2i const& p1 , Vec2i const& p2 ) override { mImpl.drawLine( p1 , p2 ); }
	virtual void drawRect   ( int left , int top , int right , int bottom ) override { mImpl.drawRect( left , top , right , bottom ); }
	virtual void drawRect   ( Vec2i const& pos , Vec2i const& size ) override { mImpl.drawRect( pos , size ); }
	virtual void drawCircle ( Vec2i const& center , int r ) override { mImpl.drawCircle( center ,  r ); }
	virtual void drawEllipse( Vec2i const& pos , Vec2i const& size ) override { mImpl.drawEllipse(  pos ,  size ); }
	virtual void drawRoundRect( Vec2i const& pos , Vec2i const& rectSize , Vec2i const& circleSize ) override { mImpl.drawRoundRect( pos , rectSize , circleSize ); }
	virtual void drawPolygon(Vec2i pos[], int num) override { mImpl.drawPolygon(pos, num); }

	virtual void setTextColor(Color3ub const& color) override { mImpl.setTextColor(color);  }
	virtual void drawText( Vec2i const& pos , char const* str ) override { mImpl.drawText( pos , str ); }
	virtual void drawText( Vec2i const& pos , Vec2i const& size , char const* str , bool beClip ) override { mImpl.drawText( pos , size , str , beClip  ); }

	virtual void accept( Visitor& visitor ) { visitor.visit( mImpl ); }
	T& mImpl;
};

IGraphics2D& DrawEngine::getIGraphics()
{
	static TGraphics2DProxy< Graphics2D > proxyPlatform( *mPlatformGraphics );
	static TGraphics2DProxy< GLGraphics2D > proxyGL( *mGLGraphics );
	static TGraphics2DProxy< RenderGL::RHIGraphics2D > proxyRHI( *mRHIGraphics );
	if ( mbGLEnabled ) 
		return proxyGL;
	return proxyPlatform;
}

DrawEngine::DrawEngine()
{
	mbGLEnabled = false;
	mbInitialized = false;
	mbSweepBuffer = true;
	mbCleaupGLDefferred = false;

}

DrawEngine::~DrawEngine()
{
	mBufferDC.release();
	mPlatformGraphics.release();
	mGLGraphics.release();
	mRHIGraphics.release();

}

void DrawEngine::init( GameWindow& window )
{
	mGameWindow = &window;
	mBufferDC.initialize( window.getHDC() , window.getHWnd() );
	mPlatformGraphics.reset ( new Graphics2D( mBufferDC.getDC() ) );
	RenderUtility::Initialize();

	mGLGraphics.reset(new GLGraphics2D);
	mGLGraphics->init(mGameWindow->getWidth(), mGameWindow->getHeight());
	mRHIGraphics.reset(new RenderGL::RHIGraphics2D);
	mRHIGraphics->init(mGameWindow->getWidth(), mGameWindow->getHeight());


	mbInitialized = true;
}

void DrawEngine::release()
{
	mPlatformGraphics->releaseReources();

	if( mbGLEnabled )
		stopOpenGL();
	else
		cleanupGLContextDeferred();
	RenderUtility::Finalize();
}

bool DrawEngine::startOpenGL( bool useGLEW , int numSamples )
{
	if ( mbGLEnabled )
		return true;

	setupBuffer( getScreenWidth() , getScreenHeight() );
	WGLPixelFormat format;
	format.numSamples = numSamples;
	if ( !mGLContext.init( getWindow().getHDC() , format ) )
		return false;

	if ( useGLEW )
	{
		if ( glewInit() != GLEW_OK )
			return false;

		if( 1 )
		{
			//glDebugMessageCallback( NULL ?,  NULL );
			glDebugMessageCallback(GLDebugCallback, nullptr);
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		}
	}

	RenderUtility::StartOpenGL();
	mbGLEnabled = true;

	return true;
}

void DrawEngine::stopOpenGL(bool bDeferred)
{
	if ( !mbGLEnabled )
		return;

	mbGLEnabled = false;

	RenderUtility::StopOpenGL();
	if( bDeferred == false )
	{
		mGLContext.cleanup();
	}
	else
	{
		mbCleaupGLDefferred = true;
	}

	setupBuffer(getScreenWidth(), getScreenHeight());
}


bool DrawEngine::beginRender()
{
	if ( mbGLEnabled )
	{
		if ( !mGLContext.makeCurrent( getWindow().getHDC() ) )
			return false;
	}
#if !OPENGL_RENDER_DIB
	else
#endif
	{
		if( mbCleaupGLDefferred )
		{
			mbCleaupGLDefferred = false;
			mGLContext.cleanup();
		}
		if ( !bStopPlatformGraphicsRender )
		{
			mBufferDC.clear();
			mPlatformGraphics->beginRender();
		}
	}
	return true;
}

void DrawEngine::endRender()
{
	if ( mbGLEnabled )
	{
		::glFlush();

#if !OPENGL_RENDER_DIB
		if ( mbSweepBuffer )
			::SwapBuffers( getWindow().getHDC() );
#endif
	}
#if !OPENGL_RENDER_DIB
	else
#endif
	{
		if( !bStopPlatformGraphicsRender )
		{
			mPlatformGraphics->endRender();
			if( mbSweepBuffer )
				mBufferDC.bitBlt(getWindow().getHDC());
		}
	}
}

HFONT DrawEngine::createFont( int size , char const* faceName , bool beBold , bool beItalic )
{
	LOGFONT lf;

	lf.lfHeight = -(int)(fabs( ( float)10 * size *GetDeviceCaps( getWindow().getHDC() ,LOGPIXELSY)/72)/10.0+0.5);
	lf.lfWeight        = 0;
	lf.lfEscapement    = 0;
	lf.lfOrientation   = 0;
	lf.lfWeight        = ( beBold ) ? FW_BOLD : FW_NORMAL ;
	lf.lfItalic        = beItalic;
	lf.lfUnderline     = FALSE;
	lf.lfStrikeOut     = FALSE;
	lf.lfCharSet       = CHINESEBIG5_CHARSET;
	lf.lfOutPrecision  = OUT_TT_PRECIS;
	lf.lfClipPrecision = CLIP_TT_ALWAYS;
	lf.lfQuality       = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = DEFAULT_PITCH;
	strcpy_s( lf.lfFaceName , faceName );

	return CreateFontIndirect( &lf );
}


void DrawEngine::changePixelSample(int numSamples)
{
	WGLPixelFormat format;
	format.numSamples = numSamples;
	mGLContext.setupPixelFormat(getWindow().getHDC(), format, true);
}


class ProfileTextDraw : public ProfileNodeVisitorT< ProfileTextDraw >
{
public:
	ProfileTextDraw(IGraphics2D& g, Vec2i const& pos)
		:mGraphics2D(g), mTextPos(pos)
	{

	}
	static int const OffsetY = 15;
	void onRoot(SampleNode* node) 
	{
		FixString<512> str;
		double time_since_reset = ProfileSystem::Get().getTimeSinceReset();
		str.format("--- Profiling: %s (total running time: %.3f ms) ---",
					 node->getName(), time_since_reset);
		mGraphics2D.drawText(mTextPos, str);
		mTextPos.y += OffsetY;
	}
	void onNode(SampleNode* node, double parentTime) 
	{
		FixString<512> str;
		str.format("|-> %s (%.2f %%) :: %.3f ms / frame (%d calls)",
					 node->getName(),
					 parentTime > CLOCK_EPSILON ? (node->getTotalTime() / parentTime) * 100 : 0.f,
					 node->getTotalTime() / (double)ProfileSystem::Get().getFrameCountSinceReset(),
					 node->getTotalCalls());
		mGraphics2D.drawText(mTextPos, str);
		mTextPos.y += OffsetY;
	}
	bool onEnterChild(SampleNode* node) 
	{ 
		mTextPos += Vec2i(20, 0);
		return true; 
	}
	void onEnterParent(SampleNode* node, int numChildren, double accTime) 
	{
		if( numChildren )
		{
			FixString<512> str;
			double time;
			if( node->getParent() != NULL )
				time = node->getTotalTime();
			else
				time = ProfileSystem::Get().getTimeSinceReset();

			double delta = time - accTime;
			str.format("|-> %s (%.3f %%) :: %.3f ms / frame", "Other",
						 // (min(0, time_since_reset - totalTime) / time_since_reset) * 100);
				(time > CLOCK_EPSILON) ? (delta / time * 100) : 0.f,
						 delta / (double)ProfileSystem::Get().getFrameCountSinceReset());
			mGraphics2D.drawText(mTextPos, str);
			mTextPos.y += OffsetY;
			mGraphics2D.drawText(mTextPos, "-------------------------------------------------");
			mTextPos.y += OffsetY;
		}

		mTextPos += Vec2i(-20, 0);
	}
	Vec2i        mTextPos;
	IGraphics2D& mGraphics2D;
};

void DrawEngine::drawProfile(Vec2i const& pos)
{
	ProfileTextDraw textDraw( getIGraphics() , pos );
	textDraw.visitNodes();
}

void DrawEngine::changeScreenSize( int w , int h )
{
	getWindow().resize( w , h );
	if ( mBufferDC.getWidth() != w || mBufferDC.getHeight() != h )
	{
		setupBuffer( w , h );
		if ( mGLGraphics )
			mGLGraphics->init( w , h );
		if( mRHIGraphics )
			mRHIGraphics->init(w, h);

	}
}

void DrawEngine::setupBuffer( int w , int h )
{
	if ( mbGLEnabled )
	{
		int bitsPerPixel = GetDeviceCaps( getWindow().getHDC() , BITSPIXEL );

		switch (bitsPerPixel) 
		{
		case 8:
			/* bmiColors is 256 WORD palette indices */
			//bmiSize += (256 * sizeof(WORD)) - sizeof(RGBQUAD);
			break;
		case 16:
			/* bmiColors is 3 WORD component masks */
			//bmiSize += (3 * sizeof(DWORD)) - sizeof(RGBQUAD);
			break;
		case 24:
		case 32:
		default:
			/* bmiColors not used */
			break;
		}

		BITMAPINFO bmpInfo;
		bmpInfo.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
		bmpInfo.bmiHeader.biWidth    = w;
		bmpInfo.bmiHeader.biHeight   = h;
		bmpInfo.bmiHeader.biPlanes   = 1;
		bmpInfo.bmiHeader.biBitCount = bitsPerPixel;
		bmpInfo.bmiHeader.biCompression = BI_RGB;
		bmpInfo.bmiHeader.biXPelsPerMeter = 0;
		bmpInfo.bmiHeader.biYPelsPerMeter = 0;
		bmpInfo.bmiHeader.biSizeImage = 0;

		if ( !mBufferDC.initialize( getWindow().getHDC() , &bmpInfo ) )
			return;
	}
	else
	{
		mBufferDC.initialize( getWindow().getHDC() , w , h );
	}

	mPlatformGraphics->setTargetDC( mBufferDC.getDC() );
}

void DrawEngine::enableSweepBuffer(bool beS)
{
	if ( mbSweepBuffer == beS )
		return;
	mbSweepBuffer = beS;
	if ( !mbSweepBuffer )
		mPlatformGraphics->setTargetDC( mGameWindow->getHDC() );
	else
		mPlatformGraphics->setTargetDC( mBufferDC.getDC() );
}

bool DrawEngine::cleanupGLContextDeferred()
{
	if( mbGLEnabled == false && mGLContext.isValid() )
	{
		mGLContext.cleanup();
		return true;
	}
	return false;
}

