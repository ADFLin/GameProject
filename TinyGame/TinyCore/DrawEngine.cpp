#include "TinyGamePCH.h"
#include "DrawEngine.h"

#include "ProfileSystem.h"
#include "ProfileSampleVisitor.h"
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
#include "ColorName.h"
#include "Core/FNV1a.h"

using namespace Render;

ERenderSystem GDefaultRHIName = ERenderSystem::D3D11;
bool GbDefaultUsePlatformGraphic = false;

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

	TConsoleVariable< bool > CVarUseMultisample{ false , "g.UseMultisample", CVF_TOGGLEABLE };
	TConsoleVariableDelegate< char const* > CVarDefalultRHISystem
	{
		&GetDefaultRHI, &SetDefaultRHI,
		"g.DefaultRHI",
		0
	};

	ERenderSystem ConvTo(RHISystemName name)
	{
		switch (name)
		{
		case RHISystemName::D3D11: return ERenderSystem::D3D11;
		case RHISystemName::D3D12: return ERenderSystem::D3D12;
		case RHISystemName::OpenGL: return ERenderSystem::OpenGL;
		case RHISystemName::Vulkan: return ERenderSystem::Vulkan;
		}
		return ERenderSystem::None;
	}

	RHISystemName ConvTo(ERenderSystem systemName)
	{
		switch (systemName)
		{
		case ERenderSystem::OpenGL: return RHISystemName::OpenGL;
		case ERenderSystem::D3D11: return RHISystemName::D3D11;
		case ERenderSystem::D3D12: return RHISystemName::D3D12;
		case ERenderSystem::Vulkan: return RHISystemName::Vulkan;
		}
		NEVER_REACH("Unknown systemName");
		return RHISystemName::OpenGL;
	}
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

	struct VRef
	{
		VRef(Vector2 const& v):v(v){ }
		operator Vector2() { return v; }
		operator Vec2i() { return Vec2i(Math::RoundToInt(v.x), Math::RoundToInt(v.y)); }
		Vector2 const& v;
	};

	struct SRef
	{
		SRef(float const& v) :v(v) {}
		operator float() { return v; }
		operator int()   { return Math::RoundToInt(v); }
		float const& v;
	};
	void beginFrame() override { mImpl->beginFrame(); }
	void endFrame() override { mImpl->endFrame(); }
	void beginRender() override { mImpl->beginRender(); }
	void endRender() override { mImpl->endRender(); }

	void beginClip(Vec2i const& pos, Vec2i const& size) { mImpl->beginClip(pos, size); }
	void endClip() { mImpl->endClip(); }
	void beginBlend( Vec2i const& pos , Vec2i const& size , float alpha)  override { mImpl->beginBlend( pos , size , alpha ); }
	void endBlend() override { mImpl->endBlend(); }
	void setPen( Color3ub const& color, int width = 1) override { mImpl->setPen( color, width ); }
	void setBrush( Color3ub const& color ) override { mImpl->setBrush( color ); }
	void drawPixel  (Vector2 const& p , Color3ub const& color) override { mImpl->drawPixel( p , color ); }
	void drawLine   (Vector2 const& p1 , Vector2 const& p2) override { mImpl->drawLine(VRef(p1) , VRef(p2) ); }
	void drawRect   (Vector2 const& pos , Vector2 const& size) override { mImpl->drawRect(VRef(pos) , VRef(size) ); }
	void drawCircle (Vector2 const& center , float radius) override { mImpl->drawCircle(VRef(center) , SRef(radius)); }
	void drawEllipse(Vector2 const& pos , Vector2 const& size) override { mImpl->drawEllipse(VRef(pos), VRef(size)); }
	void drawRoundRect(Vector2 const& pos , Vector2 const& rectSize , Vector2 const& circleSize) override { mImpl->drawRoundRect(VRef(pos), VRef(rectSize), VRef(circleSize) ); }
	void drawPolygon(Vector2 pos[], int num) override { mImpl->drawPolygon(pos, num); }

	void setTextColor(Color3ub const& color) override { mImpl->setTextColor(color);  }
	void drawText(Vector2 const& pos , char const* str ) override { mImpl->drawText(VRef(pos) , str ); }
	void drawText(Vector2 const& pos , Vector2 const& size , char const* str , bool beClip) override { mImpl->drawText(VRef(pos) , size , str , beClip  ); }

	void  beginXForm() override { mImpl->beginXForm(); }
	void  finishXForm() override { mImpl->finishXForm(); }
	void  pushXForm() override {  mImpl->pushXForm();  }
	void  popXForm() override { mImpl->popXForm(); }
	void  identityXForm() override { mImpl->identityXForm(); }
	void  translateXForm(float ox, float oy) override { mImpl->translateXForm(ox,oy); }
	void  rotateXForm(float angle) override { mImpl->rotateXForm(angle); }
	void  scaleXForm(float sx, float sy) override { mImpl->scaleXForm(sx,sy); }

	bool  isUseRHI() const { return Meta::IsSameType<T, RHIGraphics2D >::Value; }
	void* getImplPtr() const { return mImpl; }
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
	mPlatformGraphics = std::make_unique< Graphics2D >( mBufferDC.getHandle() );

	mPlatformProxy = std::make_unique< TGraphics2DProxy< Graphics2D > >(*mPlatformGraphics);
	mRHIProxy = std::make_unique< TGraphics2DProxy< RHIGraphics2D > >(*mRHIGraphics);

	RenderUtility::Initialize();

	mbInitialized = true;
}

void DrawEngine::release()
{
	mSwapChain.release();
	mPlatformGraphics->releaseReources();

	RenderUtility::Finalize();

	if (GRHISystem)
	{
		RHIClearResourceReference();
		RHISystemShutdown();
	}
}

void DrawEngine::update(long deltaTime)
{
	if( !isRHIEnabled() && bRHIShutdownDeferred )
	{
		bRHIShutdownDeferred = false;
		RHISystemShutdown();
	}
}

bool DrawEngine::setupSystem(IGameRenderSetup* renderSetup, bool bSetupDeferred)
{
	if (isRHIEnabled())
	{
		shutdownSystem(renderSetup == nullptr, false);
	}

	mRenderSetup = renderSetup;
	if (mRenderSetup)
	{
		if (!setupSystemInternal(mRenderSetup->getDefaultRenderSystem(), false, bSetupDeferred))
			return false;
	}
	return true;
}

bool DrawEngine::setupRenderResource()
{
	if (!mRenderSetup->setupRenderResource(mSystemName))
	{
		return false;
	}
	return true;
}

bool DrawEngine::resetupSystem(ERenderSystem systemName)
{
	if (mRenderSetup == nullptr)
		return false;

	if (!mRenderSetup->isRenderSystemSupported(systemName))
		return false;

	shutdownSystem(false , true);

	if (!setupSystemInternal(systemName, true))
	{
		return false;
	}

	if (!mRenderSetup->setupRenderResource(systemName))
	{
		return false;
	}
}

bool DrawEngine::setupSystemInternal(ERenderSystem systemName, bool bForceRHI, bool bSetupDeferred)
{
	if (mRenderSetup == nullptr)
		return false;

	if (mSystemLocked != ERenderSystem::None)
	{
		systemName = mSystemLocked;

	}
	else if (systemName == ERenderSystem::None)
	{
		systemName = GDefaultRHIName;
	}

	if (systemName == ERenderSystem::None || !mRenderSetup->isRenderSystemSupported(systemName))
	{
		return false;
	}

	RenderSystemConfigs configs;
	mRenderSetup->configRenderSystem(systemName, configs);

	if (!bForceRHI && configs.bWasUsedPlatformGraphics)
	{
		if (GbDefaultUsePlatformGraphic)
		{
			mRenderSetup->setupPlatformGraphic();
			return true;
		}
	}

	if (!startupSystem(systemName, configs))
	{
		return false;
	}

	if (bSetupDeferred == false)
	{
		if (!mRenderSetup->setupRenderResource(systemName))
		{
			LogWarning(0, "Can't Setup Render Resource");
			return false;
		}
	}
	return true;
}


void DrawEngine::createRHIGraphics()
{
	if (mRHIGraphics == nullptr)
	{
		mRHIGraphics.reset(new RHIGraphics2D);
		mRHIGraphics->setViewportSize(mGameWindow->getWidth(), mGameWindow->getHeight());
		static_cast<TGraphics2DProxy< RHIGraphics2D >&>(*mRHIProxy).mImpl = mRHIGraphics.get();
	}
	else
	{
		mRHIGraphics->setViewportSize(mGameWindow->getWidth(), mGameWindow->getHeight());
	}

	if (bRHIGraphicsInitialized == false)
	{
		bRHIGraphicsInitialized = true;
		RenderUtility::InitializeRHI();
		mRHIGraphics->initializeRHI();
	}
}

bool IsSupportRHIGraphic2D(ERenderSystem systemName)
{
	if (systemName == ERenderSystem::OpenGL || 
		systemName == ERenderSystem::D3D11 || 
		systemName == ERenderSystem::D3D12 )
		return true;

	return false;
}

bool DrawEngine::isUsageRHIGraphic2D() const
{
	return IsSupportRHIGraphic2D(mSystemName);
}

bool DrawEngine::lockSystem(ERenderSystem systemLocked, RenderSystemConfigs const& configs)
{
	CHECK(mSystemLocked == ERenderSystem::None && mSystemName == ERenderSystem::None);

	RHISystemInitParams initParam;
	initParam.numSamples = configs.numSamples;
	initParam.bVSyncEnable = configs.bVSyncEnable;
	initParam.bDebugMode = configs.bDebugMode;
	initParam.hWnd = NULL;
	initParam.hDC = NULL;
	initParam.bMultithreadingSupported = configs.bMultithreadingSupported;
	VERIFY_RETURN_FALSE(RHISystemInitialize(ConvTo(systemLocked), initParam));
	
	mSystemLocked = systemLocked;

	createRHIGraphics();

	return true;
}

void DrawEngine::unlockSystem()
{
	mSystemLocked = ERenderSystem::None;
}

bool DrawEngine::startupSystem(ERenderSystem systemName, RenderSystemConfigs const& configs)
{
	if( isRHIEnabled() )
		return true;

	if (mSystemLocked != ERenderSystem::None && mSystemLocked!= systemName)
		return false;

	if (configs.screenWidth > 0 && configs.screenHeight > 0)
	{
		if (configs.screenWidth != getScreenWidth() ||
			configs.screenHeight != getScreenHeight())
		{
			changeScreenSize(configs.screenWidth, configs.screenHeight, false);
		}
	}

	RHISystemInitParams initParam;
	initParam.numSamples = configs.numSamples;
	initParam.bMultithreadingSupported = configs.bMultithreadingSupported;
	if (CVarUseMultisample)
	{
		if (initParam.numSamples == 1)
		{
			initParam.numSamples = 4;
		}
	}

	if (mSystemLocked != ERenderSystem::None)
	{
		initParam.numSamples = 1;
		//IGlobalRenderResource::RestoreAllResources();
	}
	else
	{

		if (systemName == ERenderSystem::None)
			systemName = GDefaultRHIName;

		if (bHadUseRHI)
		{
			TGuardValue<bool> value(bBlockRender, true);
			if (!mWindowProvider->reconstructWindow(*mGameWindow))
			{
				return false;
			}
		}

		initParam.bVSyncEnable = configs.bVSyncEnable;
		initParam.bDebugMode = configs.bDebugMode;
		initParam.hWnd = getWindow().getHWnd();
		initParam.hDC = getWindow().getHDC();
		if (!RHISystemInitialize(ConvTo(systemName), initParam))
			return false;
	}

	mSystemName = systemName;

	setupBuffer(getScreenWidth(), getScreenHeight());

	if (mSystemName == ERenderSystem::OpenGL)
	{
		mGLContext = &static_cast<OpenGLSystem*>(GRHISystem)->mGLContext;
	}
	else if (mSystemName == ERenderSystem::D3D11 ||
		     mSystemName == ERenderSystem::D3D12)
	{
		SwapChainCreationInfo info;
		info.windowHandle = mGameWindow->getHWnd();
		info.bWindowed = !mGameWindow->isFullscreen();
		info.extent.x = mGameWindow->getWidth();
		info.extent.y = mGameWindow->getHeight();
		info.numSamples = initParam.numSamples;
		info.bCreateDepth = true;
		info.depthFormat = ETexture::D32FS8;
		info.textureCreationFlags = TCF_CreateSRV | TCF_PlatformGraphicsCompatible;
		info.depthCreateionFlags = TCF_CreateSRV;
		mSwapChain = RHICreateSwapChain(info);
	}

	createRHIGraphics();

	bHadUseRHI = true;
	bUsePlatformBuffer = false;
	bWasUsedPlatformGraphics = configs.bWasUsedPlatformGraphics;
	return true;
}

void DrawEngine::shutdownSystem(bool bDeferred, bool bReInit)
{
	if( !isRHIEnabled())
		return;

	RHIClearResourceReference();

	if (mRenderSetup)
	{
		mRenderSetup->preShutdownRenderSystem(bReInit);
	}

	bool bReconsructWindow = false;
	if (mSystemLocked != ERenderSystem::None)
	{
		bReconsructWindow = true;
	}
	else
	{
		if (mSwapChain)
		{
			mSwapChain.release();
		}
		RenderUtility::ReleaseRHI();
		mRHIGraphics->releaseRHI();
		bRHIGraphicsInitialized = false;

		if (bReInit || bDeferred == false)
		{
			RHISystemShutdown();
		}
		else
		{
			bRHIShutdownDeferred = true;
		}

		if (mSystemName == ERenderSystem::D3D12 /*|| mSystemName == ERenderSystem::D3D11*/)
		{
			bReconsructWindow = true;
		}
		mLastRHIName = mSystemName;
	}

	mSystemName = ERenderSystem::None;
	bUsePlatformBuffer = true;

	if (bReconsructWindow)
	{
		TGuardValue<bool> value(bBlockRender, true);
		bHadUseRHI = false;
		mWindowProvider->reconstructWindow(*mGameWindow);
	}

	setupBuffer(getScreenWidth(), getScreenHeight());
}

void DrawEngine::toggleGraphics()
{
	if (isRHIEnabled())
	{
		shutdownSystem();
		if (mRenderSetup)
		{
			mRenderSetup->setupPlatformGraphic();
		}
	}
	else
	{
		if (mRenderSetup)
		{
			setupSystemInternal(mLastRHIName != ERenderSystem::None ? mLastRHIName : mRenderSetup->getDefaultRenderSystem(), true );
		}
		else
		{
			RenderSystemConfigs configs;
			configs.bWasUsedPlatformGraphics = true;
			startupSystem(mLastRHIName != ERenderSystem::None ? mLastRHIName : GDefaultRHIName, configs);
		}
	}
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

		if (bWasUsedPlatformGraphics)
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0, 0, 0, 1), 1, 1, 0);
		}

		//mRHIGraphics->enableMultisample(CVarUseMultisample);
		mRHIGraphics->beginFrame();
	}
	else
	{
		mPlatformGraphics->beginFrame();	
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
		mPlatformGraphics->endFrame();
	}

	if( bUsePlatformBuffer )
	{
		mBufferDC.bitBltTo(getWindow().getHDC());
	}
}

IGraphics2D* DrawEngine::createGraphicInterface(Graphics2D& g)
{
	return new TGraphics2DProxy< Graphics2D >(g);
}

int GColors[] =
{
	EColor::Red,
	EColor::Green,
	EColor::Blue,
	EColor::Orange,
	EColor::Green,
	EColor::Purple,
	EColor::Cyan,
};

class ProfileFrameVisualizeDraw : public ProfileNodeVisitorT< ProfileFrameVisualizeDraw >
{
public:
	ProfileFrameVisualizeDraw(IGraphics2D& g, Vector2 const& pos)
		:mGraphics2D(g), mBasePos(pos)
	{

	}

	float mTotalTime;
	float mCurTime;
	float mScale;
	int mLevel;
	TArray<float> mLevelTimePos;
	void onRoot(VisitContext const& context)
	{
		SampleNode* node = context.node;
		mTotalTime = node->getFrameExecTime();
		mScale = mMaxLength / mTotalTime;
		mCurTime = 0;
		mLevel = 0;
		mLevelTimePos.push_back(mCurTime);
	}

	void onNode(VisitContext& context)
	{
		SampleNode* node = context.node;
		Vector2 pos = mBasePos + Vector2(mScale * (mCurTime + context.displayTimeAcc), mLevel * offsetY);
		Vector2 size = Vector2( Math::Max<float>(mScale * node->getFrameExecTime(), 3.0f ) , offsetY);

		int color = FNV1a::MakeStringHash<uint32>(node->getName()) % ARRAY_SIZE(GColors);
		RenderUtility::SetBrush(mGraphics2D, GColors[color]);

		mGraphics2D.drawRect(pos, size);
		mGraphics2D.drawText(pos, size, node->getName(), true);
	}
	bool onEnterChild(VisitContext const& context)
	{ 
		mLevelTimePos.push_back(mCurTime);
		mCurTime += context.displayTime;
		++mLevel;
		return true; 
	}
	void onReturnParent(VisitContext const& context, VisitContext const& childContext)
	{
		SampleNode* node = context.node;
		mCurTime = mLevelTimePos.back();
		mLevelTimePos.pop_back();
		--mLevel;
	}

	bool filterNode(SampleNode* node)
	{
		return node->getLastFrame() + 1 == ProfileSystem::Get().getFrameCountSinceReset();
	}
	float mMaxLength = 600;
	float offsetY = 20;
	Vector2      mBasePos;
	IGraphics2D& mGraphics2D;
};


struct NodeContentData 
{
	bool bShow;
};
class ProfileTextDraw : public ProfileNodeVisitorT< ProfileTextDraw, NodeContentData >
{
public:
	ProfileTextDraw(IGraphics2D& g, Vec2i const& pos, char const* category)
		:mGraphics2D(g), mTextPos(pos), mCategory(category)
	{

	}

	void onRoot(VisitContext& context)
	{
		SampleNode* node = context.node;

		InlineString<512> str;
		double time_since_reset = ProfileSystem::Get().getDurationSinceReset();
		double lastFrameDuration = ProfileSystem::Get().getLastFrameDuration();
		str.format("--- Profiling: %s (total running time: %.03lf(%.03lf) ms) ---",
			node->getName(), time_since_reset, lastFrameDuration);
		mGraphics2D.drawText(mTextPos, str);
		mTextPos.y += OffsetY;
	}

	void onNode(VisitContext& context)
	{
		SampleNode* node = context.node;
		context.bShow = true;
		if (mCategory)
		{
			if (node->mCategory == nullptr)
			{
				context.bShow = false;
				return;
			}
			if (strcmp(node->mCategory, mCategory) != 0)
			{
				context.bShow = false;
				return;
			}
		}

		InlineString<512> str;
		str.format("|-> %s (%.2lf %%) :: %.3lf(%.3lf) ms / frame (%d calls)",
			node->getName(),
			context.timeTotalParent > CLOCK_EPSILON ? (node->getTotalTime() / context.timeTotalParent) * 100 : 0.f,
			node->getTotalTime() / double(node->getTotalCalls()),
			node->getFrameExecTime() / double(node->getFrameCalls()),
			node->getTotalCalls());
		mGraphics2D.drawText(mTextPos, str);
		mTextPos.y += OffsetY;
	}
	bool onEnterChild(VisitContext const& context)
	{
		if (context.bShow)
		{
			mTextPos += Vec2i(OffsetX, 0);
		}
		return true;
	}
	void onReturnParent(VisitContext const& context, VisitContext const& childContext)
	{
		if (!context.bShow)
			return;
		SampleNode* node = context.node;
		int    numChildren = childContext.indexNode;
		double timeAcc = childContext.totalTimeAcc;

		if (numChildren)
		{
			InlineString<512> str;
			double time = childContext.timeTotalParent;
			double delta = time - timeAcc;
			str.format("|-> %s (%.3lf %%) :: %.3lf(%.3lf) ms / frame",
				"Other",
				// (min(0, time_since_reset - totalTime) / time_since_reset) * 100);
				(time > CLOCK_EPSILON) ? (delta / time * 100) : 0.f,
				delta / context.node->getTotalCalls(),
				(childContext.displayTimeParent - childContext.displayTimeAcc) / context.node->getFrameCalls());
			mGraphics2D.drawText(mTextPos, str);
			mTextPos.y += OffsetY;
			mGraphics2D.drawText(mTextPos, "-------------------------------------------------");
			mTextPos.y += OffsetY;
		}

		mTextPos += Vec2i(-OffsetX, 0);
	}
	bool filterNode(SampleNode* node)
	{
		//return node->getLastFrame() + 1 == ProfileSystem::Get().getFrameCountSinceReset();
		return true;
	}
	static int const OffsetX = 20;
	static int const OffsetY = 15;

	bool bShowChildren;

	char const*  mCategory;
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


TConsoleVariable< int > CVarProfileDrawType(0 , "Profile.DrawType");

void DrawEngine::drawProfile(Vec2i const& pos, char const* category)
{
	IGraphics2D& g = getIGraphics();
	if (CVarProfileDrawType == 0 || CVarProfileDrawType == 2)
	{
		g.setTextColor(Color3ub(255, 255, 0));
		ProfileTextDraw draw(g, pos, category);
		draw.visitNodes();
	}
	if (CVarProfileDrawType == 1 || CVarProfileDrawType == 2)
	{
		g.setPen(Color3ub(0, 0, 0));
		g.setTextColor(Color3ub(0, 0, 0));
		ProfileFrameVisualizeDraw draw(g, pos);
		draw.visitNodes();
	}
}

void DrawEngine::changeScreenSize(int w, int h, bool bUpdateViewport /*= true*/)
{
	getWindow().resize(w, h);
	if (bUpdateViewport)
	{
		changetViewportSize(w, h);
	}
}

void DrawEngine::changetViewportSize(int w, int h)
{
	if (mBufferDC.getWidth() != w || mBufferDC.getHeight() != h)
	{
		mBufferDC.release();
		setupBuffer(w, h);
		if (mRHIGraphics)
			mRHIGraphics->setViewportSize(w, h);

		if (mSwapChain.isValid())
		{
			mSwapChain->resizeBuffer(w, h);
		}
	}
}

void DrawEngine::setupBuffer( int w , int h )
{
	mBufferDC.release();
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
