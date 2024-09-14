#ifndef DrawEngine_H_DF5BDDB2_F138_477D_88F6_4D1A6637C253
#define DrawEngine_H_DF5BDDB2_F138_477D_88F6_4D1A6637C253

#include "GameRenderSetup.h"
#include "GameGraphics2D.h"
#include "GameConfig.h"
#include "BitmapDC.h"

#include "Math/Vector2.h"

#include <memory>
#include "WindowsPlatform.h"

#include "RHI/RHICommon.h"

#define OPENGL_RENDER_DIB 0

namespace Render
{
	struct ViewportInfo;
}

class IGameWindow
{
public:
	virtual ~IGameWindow() = default;
	virtual void*         getPlatformHandle() = 0;
	virtual TVector2<int> getSize() = 0;
};

class TINY_API GameWindow : public WinFrameT< GameWindow >
{
public:
	WORD   getIcon();
	WORD   getSmallIcon();
};


class IGameWindowProvider
{
public:
	virtual ~IGameWindowProvider() = default;
	virtual GameWindow& getMainWindow() = 0;
	virtual bool        reconstructWindow(GameWindow& window) = 0;
	virtual GameWindow* createWindow(Vec2i const& pos, Vec2i const& size, char const* title) = 0;
};


class IGameViewport
{
public:
	virtual ~IGameViewport() = default;
	virtual void getViewportInfo(Render::ViewportInfo& outInfo) = 0;
	virtual void present(bool bVSync) = 0;


	class IGameViewportClient* viewportClient = nullptr;
};

class IGameViewportClient
{
public:
	virtual ~IGameViewportClient() = default;
	virtual void drawViewport(IGameViewport* viewport) = 0;
};


class WindowsGLContext;

class DrawEngine
{
public:
	TINY_API DrawEngine();
	TINY_API ~DrawEngine();

	TINY_API void  initialize( IGameWindowProvider& provider);
	TINY_API void  release();
	TINY_API void  changeScreenSize( int w , int h ,bool bUpdateViewport = true);
	TINY_API void  changetViewportSize(int w, int h);
	TINY_API void  update(long deltaTime);

	TINY_API bool setupSystem(IGameRenderSetup* renderSetup, bool bSetupDeferred = false);
	TINY_API bool resetupSystem(ERenderSystem systemName);

	IGameRenderSetup* getRenderSetup() { return mRenderSetup; }


	void draw(IGameViewport& viewport)
	{
		if (isRHIEnabled())
		{
			



		}



	}


	Vec2i          getScreenSize() { return Vec2i(mGameWindow->getWidth(), mGameWindow->getHeight()); }
	int            getScreenWidth(){ return mGameWindow->getWidth(); }
	int            getScreenHeight(){ return mGameWindow->getHeight(); }
	Graphics2D&    getPlatformGraphics(){ return *mPlatformGraphics; }
	RHIGraphics2D& getRHIGraphics(){ return *mRHIGraphics; }

	TINY_API IGraphics2D&  getIGraphics();

	WindowsGLContext*    getGLContext(){ return mGLContext; }
	
	GameWindow& getWindow(){ return *mGameWindow; }
	TINY_API void* getWindowHandle();

	TINY_API void  drawProfile(Vec2i const& pos);
	TINY_API void  toggleGraphics();

	bool          isRHIEnabled() const { return mSystemName != ERenderSystem::None; }
	ERenderSystem getSystemName() const { return mSystemName; }
	TINY_API bool isUsageRHIGraphic2D() const;
	TINY_API bool lockSystem(ERenderSystem systemLocked, RenderSystemConfigs const& configs);
	TINY_API void unlockSystem();

	bool isInitialized() { return mbInitialized; }

	TINY_API bool  startupSystem(ERenderSystem targetName , RenderSystemConfigs const& configs = RenderSystemConfigs() );
	TINY_API bool  setupRenderResource();
	TINY_API void  shutdownSystem(bool bDeferred = true, bool bReInit = false);

	TINY_API bool  beginFrame();
	TINY_API void  endFrame();

	TINY_API IGraphics2D* createGraphicInterface(Graphics2D& g);

	bool        bUsePlatformBuffer = true;
	bool        bBlockRender = false;
	bool        bWasUsedPlatformGraphics = false;
private:
	void        setupBuffer( int w , int h );
	bool        setupSystemInternal(ERenderSystem systemName, bool bForceRHI, bool bSetupDeferred = false);
	void        createRHIGraphics();

	bool        mbInitialized;
	bool        bRHIShutdownDeferred;
	bool        bHadUseRHI = false;
	
	IGameRenderSetup* mRenderSetup = nullptr;
	IGameWindowProvider* mWindowProvider = nullptr;
	GameWindow* mGameWindow;
	BitmapDC    mBufferDC;

	Render::RHISwapChainRef mSwapChain;

	std::unique_ptr< Graphics2D > mPlatformGraphics;

	std::unique_ptr< IGraphics2D > mPlatformProxy;
	std::unique_ptr< IGraphics2D > mRHIProxy;

	ERenderSystem mSystemLocked = ERenderSystem::None;
	ERenderSystem mSystemName = ERenderSystem::None;
	ERenderSystem mLastRHIName = ERenderSystem::None;
	WindowsGLContext*  mGLContext = nullptr;

	bool bRHIGraphicsInitialized = false;
	std::unique_ptr< RHIGraphics2D > mRHIGraphics;

};


#endif // DrawEngine_H_DF5BDDB2_F138_477D_88F6_4D1A6637C253