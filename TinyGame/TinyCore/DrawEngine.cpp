#include "TinyGamePCH.h"
#include "DrawEngine.h"

#include "ProfileSystem.h"
#include "PlatformThread.h"

#include "RHI/RHIGraphics2D.h"
#include "RHI/RHICommand.h"
#include "RHI/OpenGLCommand.h"

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

using namespace Render;

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
	virtual void drawPixel  ( Vector2 const& p , Color3ub const& color ) override { mImpl.drawPixel( p , color ); }
	virtual void drawLine   (Vector2 const& p1 , Vector2 const& p2 ) override { mImpl.drawLine( p1 , p2 ); }
	virtual void drawRect   (Vector2 const& pos , Vector2 const& size ) override { mImpl.drawRect( pos , size ); }
	virtual void drawCircle (Vector2 const& center , float radius ) override { mImpl.drawCircle( center , radius); }
	virtual void drawEllipse(Vector2 const& pos , Vector2 const& size ) override { mImpl.drawEllipse(  pos ,  size ); }
	virtual void drawRoundRect(Vector2 const& pos , Vector2 const& rectSize , Vector2 const& circleSize ) override { mImpl.drawRoundRect( pos , rectSize , circleSize ); }
	virtual void drawPolygon(Vector2 pos[], int num) override { mImpl.drawPolygon(pos, num); }

	virtual void setTextColor(Color3ub const& color) override { mImpl.setTextColor(color);  }
	virtual void drawText(Vector2 const& pos , char const* str ) override { mImpl.drawText( pos , str ); }
	virtual void drawText(Vector2 const& pos , Vector2 const& size , char const* str , bool beClip ) override { mImpl.drawText( pos , size , str , beClip  ); }

	virtual void accept( Visitor& visitor ) { visitor.visit( mImpl ); }
	T& mImpl;
};

IGraphics2D& DrawEngine::getIGraphics()
{
	static TGraphics2DProxy< Graphics2D > proxyPlatform( *mPlatformGraphics );
	static TGraphics2DProxy< GLGraphics2D > proxyGL( *mGLGraphics );
	static TGraphics2DProxy< Render::RHIGraphics2D > proxyRHI( *mRHIGraphics );
	if ( mRHIName == RHITargetName::OpenGL ) 
		return proxyGL;
	return proxyPlatform;
}

DrawEngine::DrawEngine()
{
	mbInitialized = false;
	bRHIShutdownDeferred = false;
}

DrawEngine::~DrawEngine()
{
	mBufferDC.release();
	mPlatformGraphics.release();
	mGLGraphics.release();
	mRHIGraphics.release();

}

void DrawEngine::initialize(IGameWindowProvider& provider)
{
	mWindowProvider = &provider;
	mGameWindow = &provider.getMainWindow();
	mBufferDC.initialize(mGameWindow->getHDC() , mGameWindow->getHWnd() );
	mPlatformGraphics.reset ( new Graphics2D( mBufferDC.getDC() ) );
	RenderUtility::Initialize();

	mGLGraphics.reset(new GLGraphics2D);
	mGLGraphics->init(mGameWindow->getWidth(), mGameWindow->getHeight());
	mRHIGraphics.reset(new Render::RHIGraphics2D);
	mRHIGraphics->init(mGameWindow->getWidth(), mGameWindow->getHeight());


	mbInitialized = true;
}

void DrawEngine::release()
{
	mPlatformGraphics->releaseReources();

	if( isRHIEnabled() )
		RHISystemShutdown();

	RenderUtility::Finalize();
}

void DrawEngine::update(long deltaTime)
{
	if( !isRHIEnabled() && bRHIShutdownDeferred )
	{
		bRHIShutdownDeferred = false;
		RHISystemShutdown();
	}
}

bool DrawEngine::initializeRHI(RHITargetName targetName, int numSamples)
{
	if( isRHIEnabled() )
		return true;

	if( bHasUseRHI )
	{
		if( !mWindowProvider->reconstructWindow(*mGameWindow) )
		{
			return false;
		}
	}

	setupBuffer(getScreenWidth(), getScreenHeight());

	RHISystemInitParam initParam;
	initParam.numSamples = numSamples;
	initParam.hWnd = getWindow().getHWnd();
	initParam.hDC = getWindow().getHDC();

	RHISytemName name = [targetName]
	{
		switch( targetName )
		{
		case RHITargetName::OpenGL: return RHISytemName::Opengl;
		case RHITargetName::D3D11: return RHISytemName::D3D11;
		}
		return RHISytemName::Opengl;
	}();
	if( !RHISystemInitialize(name, initParam) )
		return false;

	mRHIName = targetName;
	if( mRHIName == RHITargetName::OpenGL )
	{
		RenderUtility::InitializeRHI();
		mGLContext = &static_cast<OpenGLSystem*>(gRHISystem)->mGLContext;
	}
	bHasUseRHI = true;
	bUsePlatformBuffer = false;
	return true;
}

void DrawEngine::shutdownRHI(bool bDeferred)
{
	if( !isRHIEnabled() )
		return;

	if( mRHIName == RHITargetName::OpenGL )
	{
		RenderUtility::ReleaseRHI();
	}

	mRHIName = RHITargetName::None;

	if( bDeferred == false )
	{
		RHISystemShutdown();
	}
	else
	{
		bRHIShutdownDeferred = true;
	}

	bUsePlatformBuffer = true;
	setupBuffer(getScreenWidth(), getScreenHeight());
}

bool DrawEngine::startOpenGL( int numSamples )
{
	return initializeRHI(RHITargetName::OpenGL, numSamples);
}

void DrawEngine::stopOpenGL(bool bDeferred)
{
	shutdownRHI(bDeferred);
}


bool DrawEngine::beginRender()
{
	if ( isRHIEnabled() )
	{
		if( !RHIBeginRender() )
			return false;
	}
	else
	{
		mPlatformGraphics->beginRender();
		
	}

	if( bUsePlatformBuffer )
	{	
		mBufferDC.clear();
	}
	return true;
}

void DrawEngine::endRender()
{
	if ( isRHIEnabled() )
	{
		RHIEndRender(!bUsePlatformBuffer);
	}
	else
	{
		mPlatformGraphics->endRender();
	}
	if( bUsePlatformBuffer )
	{
		mBufferDC.bitBltTo(getWindow().getHDC());
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
	if ( isRHIEnabled() )
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
