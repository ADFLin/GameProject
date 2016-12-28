#include "TinyGamePCH.h"
#include "DrawEngine.h"

#include "GLGraphics2D.h"
#include "GameGlobal.h"
#include "RenderUtility.h"
#include <algorithm>
#include <cmath>

template< class T >
class TGraphics2DProxy : public IGraphics2D
{
public:
	TGraphics2DProxy( T& g ):mImpl(g){}

	virtual void beginBlend( Vec2i const& pos , Vec2i const& size , float alpha ){ mImpl.beginBlend( pos , size , alpha ); }
	virtual void endBlend(){ mImpl.endBlend(); }
	virtual void setPen( ColorKey3 const& color ){ mImpl.setPen( color ); }
	virtual void setBrush( ColorKey3 const& color ){ mImpl.setBrush( color ); }
	virtual void drawPixel  ( Vec2i const& p , ColorKey3 const& color ){ mImpl.drawPixel( p , color ); }
	virtual void drawLine   ( Vec2i const& p1 , Vec2i const& p2 ){ mImpl.drawLine( p1 , p2 ); }
	virtual void drawRect   ( int left , int top , int right , int bottom ){ mImpl.drawRect( left , top , right , bottom ); }
	virtual void drawRect   ( Vec2i const& pos , Vec2i const& size ) { mImpl.drawRect( pos , size ); }
	virtual void drawCircle ( Vec2i const& center , int r ) { mImpl.drawCircle( center ,  r ); }
	virtual void drawEllipse( Vec2i const& pos , Vec2i const& size ) { mImpl.drawEllipse(  pos ,  size ); }
	virtual void drawRoundRect( Vec2i const& pos , Vec2i const& rectSize , Vec2i const& circleSize ) { mImpl.drawRoundRect( pos , rectSize , circleSize ); }
	virtual void setTextColor( uint8 r , uint8 g, uint8 b ) { mImpl.setTextColor(  r ,  g,  b );  }
	virtual void drawText( Vec2i const& pos , char const* str ) { mImpl.drawText( pos , str ); }
	virtual void drawText( Vec2i const& pos , Vec2i const& size , char const* str , bool beClip ) { mImpl.drawText( pos , size , str , beClip  ); }

	virtual void accept( Visitor& visitor ) { visitor.visit( mImpl ); }
	T& mImpl;
};

IGraphics2D& DrawEngine::getIGraphics()
{
	static TGraphics2DProxy< Graphics2D > proxySimple( *mScreenGraphics );
	static TGraphics2DProxy< GLGraphics2D > proxyGL( *mGLGraphics );
	if ( mbGLEnable ) 
		return proxyGL;
	return proxySimple;
}

DrawEngine::DrawEngine()
{
	mbGLEnable = false;
	mbInitialized = false;
	mbSweepBuffer = true;
	mBufferDC = NULL;
	mScreenGraphics = NULL;
	mGLGraphics = NULL;

}

DrawEngine::~DrawEngine()
{
	delete mBufferDC;
	delete mScreenGraphics;
	delete mGLGraphics;

}

void DrawEngine::init( GameWindow& window )
{
	mGameWindow = &window;
	mBufferDC = new BitmapDC( window.getHDC() , window.getHWnd() );
	mScreenGraphics = new Graphics2D( mBufferDC->getDC() );
	mGLGraphics = new GLGraphics2D;
	mGLGraphics->init( mGameWindow->getWidth() , mGameWindow->getHeight() );
	RenderUtility::init();

	mbInitialized = true;
}

void DrawEngine::release()
{
	stopOpenGL();
	RenderUtility::release();
}

bool DrawEngine::startOpenGL( bool useGLEW )
{
	if ( mbGLEnable )
		return true;

	setupBuffer( getScreenWidth() , getScreenHeight() );
	if ( !mGLContext.init( getWindow().getHDC() , WGLContext::InitSetting() ) )
		return false;

	if ( useGLEW )
	{
		if ( glewInit() != GLEW_OK )
			return false;
	}
	RenderUtility::startOpenGL();
	mbGLEnable = true;

	return true;
}

void DrawEngine::stopOpenGL()
{
	if ( !mbGLEnable )
		return;

	mbGLEnable = false;

	RenderUtility::stopOpenGL();
	setupBuffer( getScreenWidth() , getScreenHeight() );
	mGLContext.cleanup();
}


bool DrawEngine::beginRender()
{
	if ( mbGLEnable )
	{
		if ( !mGLContext.makeCurrent( getWindow().getHDC() ) )
			return false;
	}
#if !OPENGL_RENDER_DIB
	else
#endif
	{
		mBufferDC->clear();
		mScreenGraphics->beginRender();
	}
	return true;
}

void DrawEngine::endRender()
{
	if ( mbGLEnable )
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
		mScreenGraphics->endRender();
		if ( mbSweepBuffer )
			mBufferDC->bitBlt( getWindow().getHDC() );
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

void DrawEngine::changeScreenSize( int w , int h )
{
	getWindow().resize( w , h );
	if ( mBufferDC->getWidth() != w || mBufferDC->getHeight() != h )
	{
		setupBuffer( w , h );
		mGLGraphics->init( w , h );
	}
}

void DrawEngine::setupBuffer( int w , int h )
{
	if ( mbGLEnable )
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
		bmpInfo.bmiHeader.biWidth    = getScreenWidth();
		bmpInfo.bmiHeader.biHeight   = getScreenHeight();
		bmpInfo.bmiHeader.biPlanes   = 1;
		bmpInfo.bmiHeader.biBitCount = bitsPerPixel;
		bmpInfo.bmiHeader.biCompression = BI_RGB;
		bmpInfo.bmiHeader.biXPelsPerMeter = 0;
		bmpInfo.bmiHeader.biYPelsPerMeter = 0;
		bmpInfo.bmiHeader.biSizeImage = 0;


		if ( !mBufferDC->create( getWindow().getHDC() , &bmpInfo ) )
			return;
	}
	else
	{
		mBufferDC->create( getWindow().getHDC() , w , h );
	}

	mScreenGraphics->setTargetDC( mBufferDC->getDC() );
}

GAME_API void DrawEngine::enableSweepBuffer(bool beS)
{
	if ( mbSweepBuffer == beS )
		return;
	mbSweepBuffer = beS;
	if ( !mbSweepBuffer )
		mScreenGraphics->setTargetDC( mGameWindow->getHDC() );
	else
		mScreenGraphics->setTargetDC( mBufferDC->getDC() );
}

