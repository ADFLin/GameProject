#ifndef DrawEngine_h__
#define DrawEngine_h__

#include "GameConfig.h"
#include "BitmapDC.h"

#include "glew/GL/glew.h"

#include "WinGLPlatform.h"
#include "WinGDIRenderSystem.h"
#include "WGLContext.h"

#include <memory>


#define OPENGL_RENDER_DIB 0

class Graphics2D : public WinGdiGraphics2D
{
public:
	Graphics2D( HDC hDC = NULL ):WinGdiGraphics2D(hDC){}

};

class GLGraphics2D;


namespace RenderGL
{
	class RHIGraphics2D;
}


class IGraphics2D
{
public:
	virtual void  beginRender() = 0;
	virtual void  endRender() = 0;
	virtual void  beginClip(Vec2i const& pos, Vec2i const& size) = 0;
	virtual void  endClip() = 0;
	virtual void  beginBlend( Vec2i const& pos , Vec2i const& size , float alpha ) = 0;
	virtual void  endBlend() = 0;
	virtual void  setPen( Color3ub const& color ) = 0;
	virtual void  setBrush( Color3ub const& color ) = 0;
	virtual void  drawPixel  ( Vec2i const& p , Color3ub const& color )= 0;
	virtual void  drawLine   ( Vec2i const& p1 , Vec2i const& p2 ) = 0;
	virtual void  drawRect   ( int left , int top , int right , int bottom )= 0;
	virtual void  drawRect   ( Vec2i const& pos , Vec2i const& size ) = 0;
	virtual void  drawCircle ( Vec2i const& center , int r ) = 0;
	virtual void  drawEllipse( Vec2i const& pos , Vec2i const& size ) = 0;
	virtual void  drawRoundRect( Vec2i const& pos , Vec2i const& rectSize , Vec2i const& circleSize ) = 0;
	virtual void  drawPolygon(Vec2i pos[], int num) = 0;
	virtual void  setTextColor(Color3ub const& color) = 0;
	virtual void  drawText( Vec2i const& pos , char const* str ) = 0;
	virtual void  drawText( Vec2i const& pos , Vec2i const& size , char const* str , bool beClip = false ) = 0;

	class Visitor
	{
	public:
		virtual void visit( Graphics2D& g ) = 0;
		virtual void visit( GLGraphics2D& g ) = 0;
		virtual void visit( RenderGL::RHIGraphics2D& g) = 0;
	};
	virtual void  accept( Visitor& visitor ) = 0;
};


class RenderOverlay
{
public:
	RenderOverlay()
	{

	}

	Graphics2D* createGraphics()
	{
		return nullptr;

	}
	void  destroyGraphics(){}


	void  render()
	{

	}

	Vec2i mRectMin;
	Vec2i mRectMax;
	HDC   mhDC;
};

class TINY_API GameWindow : public WinFrameT< GameWindow >
{
public:
	WORD   getIcon();
	WORD   getSmallIcon();
};

class DrawEngine
{
public:
	TINY_API DrawEngine();
	TINY_API ~DrawEngine();

	TINY_API void  init( GameWindow& window );
	TINY_API void  release();
	TINY_API void  changeScreenSize( int w , int h );
	
	Vec2i         getScreenSize(){ return Vec2i( mBufferDC.getWidth() , mBufferDC.getHeight() ); }
	int           getScreenWidth(){ return mBufferDC.getWidth(); }
	int           getScreenHeight(){ return mBufferDC.getHeight(); }
	Graphics2D&   getScreenGraphics(){ return *mPlatformGraphics; }
	GLGraphics2D& getGLGraphics(){ return *mGLGraphics; }

	TINY_API IGraphics2D&  getIGraphics();

	HFONT       createFont( int size , char const* faceName , bool beBold , bool beItalic );
	WindowsGLContext& getGLContext(){ return mGLContext; }
	
	GameWindow& getWindow(){ return *mGameWindow; }

	TINY_API void changePixelSample(int numSamples);

	TINY_API void drawProfile(Vec2i const& pos);


	bool isOpenGLEnabled(){ return mbGLEnabled; }
	bool isInitialized() { return mbInitialized; }

	TINY_API bool  startOpenGL( bool useGLEW = true , int numSamples = 1 );
	TINY_API void  stopOpenGL(bool bDeferred = false);
	TINY_API bool  beginRender();
	TINY_API void  endRender();
	TINY_API void  enableSweepBuffer( bool beS );
	TINY_API bool  cleanupGLContextDeferred();
private:
	void        setupBuffer( int w , int h );

	bool        mbInitialized;
	bool        mbGLEnabled;
	bool        mbSweepBuffer;
	bool        mbCleaupGLDefferred;
	
	GameWindow* mGameWindow;
	BitmapDC    mBufferDC;
	std::unique_ptr< Graphics2D > mPlatformGraphics;


	WindowsGLContext    mGLContext;
	std::unique_ptr< GLGraphics2D >   mGLGraphics;
	std::unique_ptr< RenderGL::RHIGraphics2D > mRHIGraphics;

};

class RenderSurface
{
public:
	RenderSurface()
		:mScenePos(0,0)
		,mScale( 1.0 )
	{

	}

	void         setSurfaceScale( float scale ){  mScale = scale; }
	void         setSurfacePos( Vec2i const& pos ) { mScenePos = pos; }
	Vec2i const& getSurfacePos() const { return mScenePos; }
	float        getSurfaceScale(){ return mScale; }
protected:
	Vec2i        mScenePos;
	float        mScale;
};



#endif // DrawEngine_h__