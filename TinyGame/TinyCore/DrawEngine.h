#ifndef DrawEngine_H_DF5BDDB2_F138_477D_88F6_4D1A6637C253
#define DrawEngine_H_DF5BDDB2_F138_477D_88F6_4D1A6637C253

#include "GameConfig.h"
#include "BitmapDC.h"

#include "Math/Vector2.h"
#include "glew/GL/glew.h"
#include "WGLContext.h"
#include "WinGDIRenderSystem.h"


#include <memory>

using Math::Vector2;


#define OPENGL_RENDER_DIB 0

class Graphics2D : public WinGdiGraphics2D
{
public:
	Graphics2D( HDC hDC = NULL ):WinGdiGraphics2D(hDC){}

	using WinGdiGraphics2D::drawPolygon;
	void  drawPolygon(Vector2 pos[], int num)
	{
		Vec2i* pts = (Vec2i*)alloca(sizeof(Vec2i) * num);
		for( int i = 0; i < num; ++i )
		{
			pts[i].x = Math::FloorToInt(pos[i].x);
			pts[i].y = Math::FloorToInt(pos[i].y);
		}
		drawPolygon(pts, num);
	}
};


class RHIGraphics2D;

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
	virtual void  drawPixel  (Vector2 const& p , Color3ub const& color )= 0;
	virtual void  drawLine   (Vector2 const& p1 , Vector2 const& p2 ) = 0;

	virtual void  drawRect   (Vector2 const& pos , Vector2 const& size ) = 0;
	virtual void  drawCircle (Vector2 const& center , float radius ) = 0;
	virtual void  drawEllipse(Vector2 const& pos , Vector2 const& size ) = 0;
	virtual void  drawRoundRect(Vector2 const& pos , Vector2 const& rectSize , Vector2 const& circleSize ) = 0;
	virtual void  drawPolygon(Vector2 pos[], int num) = 0;
	virtual void  setTextColor(Color3ub const& color) = 0;
	virtual void  drawText(Vector2 const& pos , char const* str ) = 0;
	virtual void  drawText(Vector2 const& pos , Vector2 const& size , char const* str , bool beClip = false ) = 0;

	void  drawRect(int left, int top, int right, int bottom)
	{
		drawRect(Vector2(left, top), Vector2(right - left, bottom - right));
	}
	class Visitor
	{
	public:
		virtual void visit( Graphics2D& g ) = 0;
		virtual void visit( RHIGraphics2D& g ) = 0;
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

enum class RHITargetName
{
	None   ,
	OpenGL ,
	D3D11  ,
	D3D12  ,
	Vulkan ,
};

struct RHIInitializeParams
{
	int32 numSamples;


	RHIInitializeParams()
	{
		numSamples = 1;
	}

};

class IGameWindowProvider
{
public:
	virtual ~IGameWindowProvider(){}
	virtual GameWindow& getMainWindow() = 0;
	virtual bool        reconstructWindow(GameWindow& window) = 0;
	virtual GameWindow* createWindow(Vec2i const& pos, Vec2i const& size, char const* title) = 0;
};

class DrawEngine
{
public:
	TINY_API DrawEngine();
	TINY_API ~DrawEngine();

	TINY_API void  initialize( IGameWindowProvider& provider);
	TINY_API void  release();
	TINY_API void  changeScreenSize( int w , int h );
	TINY_API void  update(long deltaTime);

	Vec2i         getScreenSize(){ return Vec2i( mBufferDC.getWidth() , mBufferDC.getHeight() ); }
	int           getScreenWidth(){ return mBufferDC.getWidth(); }
	int           getScreenHeight(){ return mBufferDC.getHeight(); }
	Graphics2D&   getPlatformGraphics(){ return *mPlatformGraphics; }
	RHIGraphics2D& getRHIGraphics(){ return *mRHIGraphics; }

	TINY_API IGraphics2D&  getIGraphics();

	HFONT       createFont( int size , char const* faceName , bool beBold , bool beItalic );
	WindowsGLContext*    getGLContext(){ return mGLContext; }
	
	GameWindow& getWindow(){ return *mGameWindow; }
	TINY_API void* getWindowHandle();

	TINY_API void  drawProfile(Vec2i const& pos);


	bool isRHIEnabled() const { return mRHIName != RHITargetName::None; }
	bool isOpenGLEnabled() const { return mRHIName == RHITargetName::OpenGL; }
	TINY_API bool isUsageRHIGraphic2D() const;
	bool isInitialized() { return mbInitialized; }

	TINY_API bool  initializeRHI(RHITargetName targetName , RHIInitializeParams initParams = RHIInitializeParams() );
	TINY_API void  shutdownRHI(bool bDeferred = true);
	TINY_API bool  beginRender();
	TINY_API void  endRender();

	bool        bUsePlatformBuffer = true;
private:
	void        setupBuffer( int w , int h );

	bool        mbInitialized;
	bool        bRHIShutdownDeferred;
	bool        bHasUseRHI = false;
	
	IGameWindowProvider* mWindowProvider = nullptr;
	GameWindow* mGameWindow;
	BitmapDC    mBufferDC;
	std::unique_ptr< Graphics2D > mPlatformGraphics;

	RHITargetName mRHIName = RHITargetName::None;
	WindowsGLContext*  mGLContext = nullptr;
	std::unique_ptr< RHIGraphics2D > mRHIGraphics;


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



#endif // DrawEngine_H_DF5BDDB2_F138_477D_88F6_4D1A6637C253