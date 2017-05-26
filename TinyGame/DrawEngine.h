#ifndef DrawEngine_h__
#define DrawEngine_h__

#include "GameConfig.h"
#include "BitmapDC.h"

#include "glew/GL/glew.h"
#include "WinGLPlatform.h"
#include "WinGDIRenderSystem.h"
#include "WGLContext.h"

#define OPENGL_RENDER_DIB 0

class Graphics2D : public WinGdiGraphics2D
{
public:
	Graphics2D( HDC hDC = NULL ):WinGdiGraphics2D(hDC){}

};

class GLGraphics2D;



class IGraphics2D
{
public:
	virtual void  beginBlend( Vec2i const& pos , Vec2i const& size , float alpha ) = 0;
	virtual void  endBlend() = 0;
	virtual void  setPen( ColorKey3 const& color ) = 0;
	virtual void  setBrush( ColorKey3 const& color ) = 0;
	virtual void  drawPixel  ( Vec2i const& p , ColorKey3 const& color )= 0;
	virtual void  drawLine   ( Vec2i const& p1 , Vec2i const& p2 ) = 0;
	virtual void  drawRect   ( int left , int top , int right , int bottom )= 0;
	virtual void  drawRect   ( Vec2i const& pos , Vec2i const& size ) = 0;
	virtual void  drawCircle ( Vec2i const& center , int r ) = 0;
	virtual void  drawEllipse( Vec2i const& pos , Vec2i const& size ) = 0;
	virtual void  drawRoundRect( Vec2i const& pos , Vec2i const& rectSize , Vec2i const& circleSize ) = 0;
	virtual void  setTextColor( uint8 r , uint8 g, uint8 b ) = 0;
	virtual void  drawText( Vec2i const& pos , char const* str ) = 0;
	virtual void  drawText( Vec2i const& pos , Vec2i const& size , char const* str , bool beClip = false ) = 0;

	class Visitor
	{
	public:
		virtual void visit( Graphics2D& g ) = 0;
		virtual void visit( GLGraphics2D& g ) = 0;
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

class GameWindow : public WinFrameT< GameWindow >
{
public:

};

class DrawEngine
{
public:
	GAME_API DrawEngine();
	GAME_API ~DrawEngine();

	GAME_API void  init( GameWindow& window );
	GAME_API void  release();
	GAME_API void  changeScreenSize( int w , int h );
	
	Vec2i         getScreenSize(){ return Vec2i( mBufferDC->getWidth() , mBufferDC->getHeight() ); }
	int           getScreenWidth(){ return mBufferDC->getWidth(); }
	int           getScreenHeight(){ return mBufferDC->getHeight(); }
	Graphics2D&   getScreenGraphics(){ return *mScreenGraphics; }
	GLGraphics2D& getGLGraphics(){ return *mGLGraphics; }

	GAME_API IGraphics2D&  getIGraphics();

	HFONT       createFont( int size , char const* faceName , bool beBold , bool beItalic );
	WGLContext& getGLContext(){ return mGLContext; }
	
	GameWindow& getWindow(){ return *mGameWindow; }

	bool isOpenGLEnabled(){ return mbGLEnabled; }
	bool isInitialized() { return mbInitialized; }

	GAME_API bool  startOpenGL( bool useGLEW = true );
	GAME_API void  stopOpenGL(bool bDeferred = false);
	GAME_API bool  beginRender();
	GAME_API void  endRender();
	GAME_API void  enableSweepBuffer( bool beS );
	GAME_API bool  cleanupGLContextDeferred();
private:
	void        setupBuffer( int w , int h );

	bool        mbInitialized;
	bool        mbGLEnabled;
	bool        mbSweepBuffer;
	
	GameWindow* mGameWindow;
	BitmapDC*   mBufferDC;
	Graphics2D* mScreenGraphics;


	WGLContext    mGLContext;
	GLGraphics2D* mGLGraphics;

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