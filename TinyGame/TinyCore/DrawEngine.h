#ifndef DrawEngine_H_DF5BDDB2_F138_477D_88F6_4D1A6637C253
#define DrawEngine_H_DF5BDDB2_F138_477D_88F6_4D1A6637C253

#include "GameRenderSetup.h"
#include "GameGraphics2D.h"
#include "GameConfig.h"
#include "BitmapDC.h"

#include "Math/Vector2.h"

#include <memory>
#include "WindowsPlatform.h"


#define OPENGL_RENDER_DIB 0

class TINY_API GameWindow : public WinFrameT< GameWindow >
{
public:
	WORD   getIcon();
	WORD   getSmallIcon();
};


class IGameWindowProvider
{
public:
	virtual ~IGameWindowProvider() {}
	virtual GameWindow& getMainWindow() = 0;
	virtual bool        reconstructWindow(GameWindow& window) = 0;
	virtual GameWindow* createWindow(Vec2i const& pos, Vec2i const& size, char const* title) = 0;
};


class WindowsGLContext;

class DrawEngine
{
public:
	TINY_API DrawEngine();
	TINY_API ~DrawEngine();

	TINY_API void  initialize( IGameWindowProvider& provider);
	TINY_API void  release();
	TINY_API void  changeScreenSize( int w , int h );
	TINY_API void  update(long deltaTime);

	Vec2i          getScreenSize(){ return Vec2i( mBufferDC.getWidth() , mBufferDC.getHeight() ); }
	int            getScreenWidth(){ return mBufferDC.getWidth(); }
	int            getScreenHeight(){ return mBufferDC.getHeight(); }
	Graphics2D&    getPlatformGraphics(){ return *mPlatformGraphics; }
	RHIGraphics2D& getRHIGraphics(){ return *mRHIGraphics; }

	TINY_API IGraphics2D&  getIGraphics();

	HFONT       createFont( int size , char const* faceName , bool beBold , bool beItalic );
	WindowsGLContext*    getGLContext(){ return mGLContext; }
	
	GameWindow& getWindow(){ return *mGameWindow; }
	TINY_API void* getWindowHandle();

	TINY_API void  drawProfile(Vec2i const& pos);


	bool          isRHIEnabled() const { return mSystemName != ERenderSystem::None; }
	ERenderSystem getSystemName() const { return mSystemName; }
	TINY_API bool isUsageRHIGraphic2D() const;
	bool isInitialized() { return mbInitialized; }

	TINY_API bool  startupSystem(ERenderSystem targetName , RenderSystemConfigs const& configs = RenderSystemConfigs() );
	TINY_API void  shutdownSystem(bool bDeferred = true);

	TINY_API bool  beginRender();
	TINY_API void  endRender();

	bool        bUsePlatformBuffer = true;
	bool        bBlockRender = false;
	bool        bWasUsedPlatformGrapthics = false;
private:
	void        setupBuffer( int w , int h );

	bool        mbInitialized;
	bool        bRHIShutdownDeferred;
	bool        bHasUseRHI = false;
	
	IGameWindowProvider* mWindowProvider = nullptr;
	GameWindow* mGameWindow;
	BitmapDC    mBufferDC;
	std::unique_ptr< Graphics2D > mPlatformGraphics;

	ERenderSystem mSystemName = ERenderSystem::None;
	WindowsGLContext*  mGLContext = nullptr;
	std::unique_ptr< RHIGraphics2D > mRHIGraphics;

};


#endif // DrawEngine_H_DF5BDDB2_F138_477D_88F6_4D1A6637C253