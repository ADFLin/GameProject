#include "TinyGamePCH.h"
#include "DrawEngine.h"

#include "ProfileSystem.h"
#include "PlatformThread.h"

#include "RHI/RHIGraphics2D.h"
#include "RHI/RHICommand.h"
#include "RHI/OpenGLCommand.h"

#include "RHI/RHIGraphics2D.h"
#include "GameGlobal.h"
#include "RenderUtility.h"
#include <algorithm>
#include <cmath>

#include "resource.h"
#include "ConsoleSystem.h"

using namespace Render;

ERenderSystem GDefaultRHIName = ERenderSystem::D3D11;

namespace
{
	static char const* GetDefaultRHI()
	{
		switch (GDefaultRHIName)
		{
		case ERenderSystem::OpenGL: return "OpenGL";
		case ERenderSystem::D3D11:  return "D3D11";
		case ERenderSystem::D3D12: return "D3D12";
		case ERenderSystem::Vulkan: return "Vulkan";
		}
		return "Unknown";
	}
	static void SetDefaultRHI(char const* str)
	{
		if (FCString::CompareIgnoreCase(str, "OpenGL") == 0)
		{
			GDefaultRHIName = ERenderSystem::OpenGL;
		}
		else if (FCString::CompareIgnoreCase(str, "D3D11") == 0)
		{
			GDefaultRHIName = ERenderSystem::D3D11;
		}
		else if (FCString::CompareIgnoreCase(str, "D3D12") == 0)
		{
			GDefaultRHIName = ERenderSystem::D3D12;
		}
		else if (FCString::CompareIgnoreCase(str, "Vulkan") == 0)
		{
			GDefaultRHIName = ERenderSystem::Vulkan;
		}
	}

	TConsoleVariable< bool > CVarUseMultisample{ true , "g.UseMultisample", CVF_TOGGLEABLE };
	TConsoleVariableDelegate< char const* > CVarDefalultRHISystem
	{
		&GetDefaultRHI, &SetDefaultRHI,
		"g.DefaultRHI",
		0
	};
};

WORD GameWindow::getIcon()
{
	return IDI_ICON1;
}

WORD GameWindow::getSmallIcon()
{
	return IDI_ICON1;
}

template< class T >
class TGraphics2DProxy : public IGraphics2D
{
public:
	TGraphics2DProxy( T& g ):mImpl(&g){}

	void beginFrame() override { mImpl->beginFrame(); }
	void endFrame() override { mImpl->endFrame(); }
	void beginRender() override { mImpl->beginRender(); }
	void endRender() override { mImpl->endRender(); }

	void beginClip(Vec2i const& pos, Vec2i const& size) { mImpl->beginClip(pos, size); }
	void endClip() { mImpl->endClip(); }
	void beginBlend( Vec2i const& pos , Vec2i const& size , float alpha)  override { mImpl->beginBlend( pos , size , alpha ); }
	void endBlend() override { mImpl->endBlend(); }
	void setPen( Color3ub const& color ) override { mImpl->setPen( color ); }
	void setBrush( Color3ub const& color ) override { mImpl->setBrush( color ); }
	void drawPixel  (Vector2 const& p , Color3ub const& color) override { mImpl->drawPixel( p , color ); }
	void drawLine   (Vector2 const& p1 , Vector2 const& p2) override { mImpl->drawLine( p1 , p2 ); }
	void drawRect   (Vector2 const& pos , Vector2 const& size) override { mImpl->drawRect( pos , size ); }
	void drawCircle (Vector2 const& center , float radius) override { mImpl->drawCircle( center , radius); }
	void drawEllipse(Vector2 const& pos , Vector2 const& size) override { mImpl->drawEllipse(  pos ,  size ); }
	void drawRoundRect(Vector2 const& pos , Vector2 const& rectSize , Vector2 const& circleSize) override { mImpl->drawRoundRect( pos , rectSize , circleSize ); }
	void drawPolygon(Vector2 pos[], int num) override { mImpl->drawPolygon(pos, num); }

	void setTextColor(Color3ub const& color) override { mImpl->setTextColor(color);  }
	void drawText(Vector2 const& pos , char const* str ) override { mImpl->drawText( pos , str ); }
	void drawText(Vector2 const& pos , Vector2 const& size , char const* str , bool beClip) override { mImpl->drawText( pos , size , str , beClip  ); }

	void  beginXForm() override { mImpl->beginXForm(); }
	void  finishXForm() override { mImpl->finishXForm(); }
	void  pushXForm() override {  mImpl->pushXForm();  }
	void  popXForm() override { mImpl->popXForm(); }
	void  identityXForm() override { mImpl->identityXForm(); }
	void  translateXForm(float ox, float oy) override { mImpl->translateXForm(ox,oy); }
	void  rotateXForm(float angle) override { mImpl->rotateXForm(angle); }
	void  scaleXForm(float sx, float sy) override { mImpl->scaleXForm(sx,sy); }

	bool  isUseRHI() const { return Meta::IsSameType<T, RHIGraphics2D >::Value; }

	void accept( Visitor& visitor ) { visitor.visit( *mImpl ); }
	T* mImpl;
};

IGraphics2D& DrawEngine::getIGraphics()
{
	if (isUsageRHIGraphic2D())
	{
		return *mRHIProxy;
	}
	return *mPlatformProxy;
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
	mRHIGraphics.release();

}

void DrawEngine::initialize(IGameWindowProvider& provider)
{
	mWindowProvider = &provider;
	mGameWindow = &provider.getMainWindow();
	mBufferDC.initialize(mGameWindow->getHDC() , mGameWindow->getHWnd() );
	mPlatformGraphics.reset ( new Graphics2D( mBufferDC.getHandle() ) );

	mPlatformProxy.reset(new TGraphics2DProxy< Graphics2D >(*mPlatformGraphics));
	mRHIProxy.reset(new TGraphics2DProxy< RHIGraphics2D >(*mRHIGraphics));

	RenderUtility::Initialize();

	mbInitialized = true;
}

void DrawEngine::release()
{
	mPlatformGraphics->releaseReources();

	if( isRHIEnabled() || bRHIShutdownDeferred )
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

bool DrawEngine::setupSystem(IGameRenderSetup* renderSetup)
{
	if (isRHIEnabled())
	{
		shutdownSystem(renderSetup == nullptr);
	}

	mRenderSetup = renderSetup;
	if (mRenderSetup)
	{
		if (!setupSystemInternal(mRenderSetup->getDefaultRenderSystem()))
			return false;
	}
	return true;
}

bool DrawEngine::resetupSystem(ERenderSystem systemName)
{
	if (mRenderSetup == nullptr)
		return false;

	mRenderSetup->preShutdownRenderSystem(true);
	shutdownSystem(false);

	if (!setupSystemInternal(systemName))
	{
		return false;
	}

	if (!mRenderSetup->setupRenderSystem(systemName))
	{
		return false;
	}
}

bool DrawEngine::setupSystemInternal(ERenderSystem systemName)
{
	if (mRenderSetup == nullptr)
		return false;

	if (systemName == ERenderSystem::None)
	{
		if (mRenderSetup->isRenderSystemSupported(ERenderSystem::OpenGL))
			systemName = ERenderSystem::OpenGL;
	}
	else if (!mRenderSetup->isRenderSystemSupported(systemName))
	{
		return false;
	}

	RenderSystemConfigs configs;
	mRenderSetup->configRenderSystem(systemName, configs);

	if (!startupSystem(systemName, configs))
	{
		return false;
	}

	return true;
}

bool DrawEngine::isUsageRHIGraphic2D() const
{
	if (mSystemName == ERenderSystem::OpenGL || mSystemName == ERenderSystem::D3D11 )
		return true;

	return false;
}

bool DrawEngine::startupSystem(ERenderSystem systemName, RenderSystemConfigs const& configs)
{
	if( isRHIEnabled() )
		return true;

	if (configs.screenWidth > 0 && configs.screenHeight > 0)
	{
		if (configs.screenWidth != getScreenWidth() &&
			configs.screenHeight != getScreenHeight())
		{
			changeScreenSize(configs.screenWidth, configs.screenHeight);
		}
	}

	if (systemName == ERenderSystem::None)
		systemName = GDefaultRHIName;

	if( bHasUseRHI )
	{
		TGuardValue<bool> value(bBlockRender, true);
		if( !mWindowProvider->reconstructWindow(*mGameWindow) )
		{
			return false;
		}
	}

	setupBuffer(getScreenWidth(), getScreenHeight());

	RHISystemInitParams initParam;
	initParam.numSamples = configs.numSamples;
	if (CVarUseMultisample)
	{
		if (initParam.numSamples == 1)
		{
			initParam.numSamples = 4;
		}
	}
	initParam.bVSyncEnable = configs.bVSyncEnable;
	initParam.bDebugMode = configs.bDebugMode;
	initParam.hWnd = getWindow().getHWnd();
	initParam.hDC = getWindow().getHDC();

	RHISystemName targetName = [systemName]
	{
		switch(systemName)
		{
		case ERenderSystem::OpenGL: return RHISystemName::OpenGL;
		case ERenderSystem::D3D11: return RHISystemName::D3D11;
		case ERenderSystem::D3D12: return RHISystemName::D3D12;
		case ERenderSystem::Vulkan: return RHISystemName::Vulkan;
		}
		return RHISystemName::OpenGL;
	}();
	if( !RHISystemInitialize(targetName, initParam) )
		return false;

	mSystemName = systemName;

	RenderUtility::InitializeRHI();
	if( mSystemName == ERenderSystem::OpenGL )
	{
		mGLContext = &static_cast<OpenGLSystem*>(GRHISystem)->mGLContext;
	}
	else if (mSystemName == ERenderSystem::D3D11 ||
		     mSystemName == ERenderSystem::D3D12 )
	{
		SwapChainCreationInfo info;
		info.windowHandle = mGameWindow->getHWnd();
		info.bWindowed = !mGameWindow->isFullscreen();
		info.extent.x = mGameWindow->getWidth();
		info.extent.y = mGameWindow->getHeight();
		info.numSamples = initParam.numSamples;
		info.bCreateDepth = true;
		RHICreateSwapChain(info);
	}

	if ( mRHIGraphics == nullptr )
	{
		mRHIGraphics.reset(new RHIGraphics2D);
		mRHIGraphics->init(mGameWindow->getWidth(), mGameWindow->getHeight());
		static_cast<TGraphics2DProxy< RHIGraphics2D >&>(*mRHIProxy).mImpl = mRHIGraphics.get();
	}

	mRHIGraphics->initializeRHI();

	bHasUseRHI = true;
	bUsePlatformBuffer = false;
	bWasUsedPlatformGrapthics = false;
	return true;
}

void DrawEngine::shutdownSystem(bool bDeferred)
{
	if( !isRHIEnabled() )
		return;

	RenderUtility::ReleaseRHI();
	mSystemName = ERenderSystem::None;
	mRHIGraphics->releaseRHI();

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

bool DrawEngine::beginFrame()
{
	if (bBlockRender)
	{
		LogMsg("====== Render blocked !! =====");
		return false;
	}

	if ( isRHIEnabled() )
	{
		if( !RHIBeginRender() )
			return false;

		if (bWasUsedPlatformGrapthics)
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0, 0, 0, 1), 1, 1, 0);
		}

		mRHIGraphics->enableMultisample(CVarUseMultisample);
		mRHIGraphics->beginFrame();
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

void DrawEngine::endFrame()
{
	if ( isRHIEnabled() )
	{
		mRHIGraphics->endFrame();
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
		InlineString<512> str;
		double time_since_reset = ProfileSystem::Get().getTimeSinceReset();
		str.format("--- Profiling: %s (total running time: %.3f ms) ---",
					 node->getName(), time_since_reset);
		mGraphics2D.drawText(mTextPos, str);
		mTextPos.y += OffsetY;
	}
	void onNode(SampleNode* node, double parentTime) 
	{
		InlineString<512> str;
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
			InlineString<512> str;
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

void* DrawEngine::getWindowHandle()
{
#if SYS_PLATFORM_WIN
	return (void*)mGameWindow->getHWnd();
#else
	return nullptr;
#endif
}

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

	mPlatformGraphics->setTargetDC( mBufferDC.getHandle() );
}
