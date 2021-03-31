#include "TinyGamePCH.h"
#include "TinyGameApp.h"

#include "DrawEngine.h"
#include "RenderUtility.h"

#include "PropertySet.h"
#include "InputManager.h"
#include "FileSystem.h"
#include "ProfileSystem.h"
#include "ConsoleSystem.h"
#include "Asset.h"

#include "GameGUISystem.h"
#include "GameStage.h"
#include "GameStageMode.h"
#include "GameGlobal.h"
#include "MiscTestRegister.h"

#include "GameServer.h"
#include "GameClient.h"


#include "GameWidget.h"
#include "GameWidgetID.h"
#include "Widget/ConsoleFrame.h"

#include "Localization.h"

#include "Stage/MainMenuStage.h"

#include "NetGameMode.h"

#include "SingleStageMode.h"
#include "ReplayStageMode.h"

#include "PlatformThread.h"
#include "SystemPlatform.h"

#include "RHI/GpuProfiler.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"


#include <iostream>
#include "Hardware/GPUDeviceQuery.h"



#define GAME_SETTING_PATH "Game.ini"
#define CONFIG_SECTION "SystemSetting"

int  GDevMsgLevel = -1;

TConsoleVariable<bool> GbProfileGPU(true , "ProfileGPU", CVF_TOGGLEABLE);
TConsoleVariable<bool> GbDrawGPUUsage(false, "r.GPUUsage", CVF_TOGGLEABLE);

TINY_API IGameNetInterface* GGameNetInterfaceImpl;
TINY_API IDebugInterface*   GDebugInterfaceImpl;

TConsoleVariable< bool > CVarShowFPS(false, "ShowFPS" , CVF_TOGGLEABLE);

AutoConsoleCommand CmdRHIDumpResource("r.dumpResource", Render::RHIResource::DumpResource);


void Foo(int a, int b)
{
	LogMsg("%d+%d=%d", a, b, a + b);
}

void Foo2(Vector2 const& a, Vector2 const& b)
{
	Vector2 c = a + b;
	LogMsg("%f %f", c.x, c.y);
}
AutoConsoleCommand CmdFoo("Foo", Foo);
AutoConsoleCommand CmdFoo2("Foo2", Foo2);

namespace EOutputColor
{
	enum Type
	{
		Black = 0,
		Red     = PLATFORM_WIN_VALUE(FOREGROUND_RED, 0x004),
		Blue    = PLATFORM_WIN_VALUE(FOREGROUND_BLUE, 0x001),
		Green   = PLATFORM_WIN_VALUE(FOREGROUND_GREEN, 0x002),
		Cyan    = Green | Blue,
		
		Magenta = Red | Blue,
		Yellow  = Red | Green,
		Gray    = Red | Green | Blue,
		LightGray    = PLATFORM_WIN_VALUE(FOREGROUND_INTENSITY,0x8),
		LightBlue    = PLATFORM_WIN_VALUE(FOREGROUND_INTENSITY,0x8) | Blue,
		LightGreen   = PLATFORM_WIN_VALUE(FOREGROUND_INTENSITY,0x8) | Green,
		LightCyan    = PLATFORM_WIN_VALUE(FOREGROUND_INTENSITY,0x8) | Green | Blue,
		LightRed     = PLATFORM_WIN_VALUE(FOREGROUND_INTENSITY,0x8) | Red,
		LightMagenta = PLATFORM_WIN_VALUE(FOREGROUND_INTENSITY,0x8) | Red | Blue,
		LightYellow  = PLATFORM_WIN_VALUE(FOREGROUND_INTENSITY,0x8) | Red | Green,
		White        = PLATFORM_WIN_VALUE(FOREGROUND_INTENSITY,0x8) | Red | Green | Blue,
	};
}

class FConsoleConfigUtilities
{
public:
	static int Export(PropertySet& configs, char const* sectionGroup)
	{
		int result = 0;
		ConsoleSystem::Get().visitAllVariables(
			[sectionGroup, &configs ,&result](VariableConsoleCommadBase* var)
			{
				if (var->getFlags() & CVF_CONFIG)
				{
					configs.setKeyValue(var->mName.c_str(), sectionGroup, var->toString().c_str());
					++result;
				}
			}
		);
		return result;
	}

	static int Import(PropertySet& configs, char const* sectionGroup)
	{
		int result = 0;
		configs.visitGroup(sectionGroup , 
			[&configs, &result](char const* key, KeyValue const& value)
			{
			   auto com = ConsoleSystem::Get().findCommand(key);
			   if (com)
			   {
				   auto var = com->asVariable();
				   if ( var && (var->getFlags() & CVF_CONFIG) )
				   {
					   if (var->setFromString(value.getStringView()))
					   {
						   ++result;
					   }				   
				   }
			   }
			}
		);
		return result;
	}
};
class GameLogPrinter : public LogOutput
	                 , public IDebugInterface
{
public:

	GameLogPrinter()
	{

	}
	void init()
	{
		std::cout.sync_with_stdio(true);
	}
	static int const MaxLineNum = 20;

	char const* GetChannelPrefixStr(LogChannel channel)
	{
		switch (channel)
		{
		case LOG_MSG: return "Log: ";
		case LOG_DEV: return "Dev: ";
		case LOG_WARNING: return "Warning: ";
		case LOG_ERROR: return "Error: ";
		}
		return "";

	}

	static void OutputConsoleString(char const* prefixStr, char const* text, EOutputColor::Type prefixStrColor, EOutputColor::Type textColor)
	{
#if SYS_PLATFORM_WIN
		auto hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hConsole, prefixStrColor);
		std::cout << prefixStr;
		SetConsoleTextAttribute(hConsole, textColor);
		std::cout << text << std::endl;
#else
		std::cout << prefixStr << text << std::endl;
#endif
	}


	void receiveLog( LogChannel channel , char const* str ) override
	{
		Mutex::Locker locker(mMutex);

		switch (channel)
		{
		case LOG_MSG:     OutputConsoleString(GetChannelPrefixStr(channel), str, EOutputColor::White, EOutputColor::White); break;
		case LOG_DEV:     OutputConsoleString(GetChannelPrefixStr(channel), str, EOutputColor::Blue, EOutputColor::LightBlue); break;
		case LOG_WARNING: OutputConsoleString(GetChannelPrefixStr(channel), str, EOutputColor::Yellow , EOutputColor::LightYellow); break;
		case LOG_ERROR:   OutputConsoleString(GetChannelPrefixStr(channel), str, EOutputColor::Red, EOutputColor::LightRed); break;
		}

#if SYS_PLATFORM_WIN
		if (IsDebuggerPresent())
		{
			std::string outText = GetChannelPrefixStr(channel);
			outText += str;
			outText += "\n";
			OutputDebugString(outText.c_str());
		}
#endif
		if ( mLogTextList.size() > MaxLineNum )
			mLogTextList.pop_front();

		mLogTextList.emplace_back( str );
		if ( mLogTextList.size() == 1 )
			mTime = 0;
	}

	bool filterLog(LogChannel channel, int level) override
	{
		if( channel == LOG_DEV )
		{
			return GDevMsgLevel < level;
		}
		return true;
	}

	void update( long time )
	{
		mTime += time;
		if ( mTime > 2000 )
		{
			Mutex::Locker locker( mMutex );
			if ( !mLogTextList.empty() )
				mLogTextList.pop_front();
			mTime = 0;
		}
	}

	void render( Vec2i const& renderPos )
	{
		IGraphics2D& g = Global::GetIGraphics2D();

		Vec2i pos = renderPos;
		RenderUtility::SetFont( g , FONT_S8 );
		g.setTextColor(Color3ub(255 , 255 , 0));
		Mutex::Locker locker( mMutex );
		for( auto& text : mLogTextList)
		{
			g.drawText( pos , text.c_str() );
			pos.y += 14;
		}
	}

	void clearDebugMsg() override
	{
		Mutex::Locker locker(mMutex);
		mLogTextList.clear();
		mTime = 0;
	}

	//bool     beInited;
	Mutex    mMutex;
	unsigned mTime;
	using StringList = std::list< std::string >;
	StringList mLogTextList;
};


class GameConfigAsset : public IAssetViewer
{
public:
	void getDependentFilePaths(std::vector< std::wstring >& paths) override 
	{
		paths.push_back(FCString::CharToWChar(GAME_SETTING_PATH));
	}

	void postFileModify(EFileAction action) override 
	{
		if (action == EFileAction::Modify)
		{
			Global::GameConfig().loadFile(GAME_SETTING_PATH);
		}
	}

};

static GameLogPrinter gLogPrinter;
static GameConfigAsset gGameConfigAsset;

TinyGameApp::TinyGameApp()
	:mRenderEffect( nullptr )
	,mNetWorker( nullptr )
	,mStageMode( nullptr )
{
	mShowErrorMsg = false;
	mbLockFPS = false;

	GGameNetInterfaceImpl = this;
	GDebugInterfaceImpl = &gLogPrinter;
}

TinyGameApp::~TinyGameApp()
{

}


#if SYS_PLATFORM_WIN
#include <fcntl.h>
#include <corecrt_io.h>
#include <iostream>
#endif
void RedirectStdIO()
{
#if SYS_PLATFORM_WIN
	if (AllocConsole()) 
	{
		FILE* pCout;
		freopen_s(&pCout, "CONOUT$", "w", stdout);
		SetConsoleTitle(TEXT("Debug Console"));
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), EOutputColor::White);
	}
#endif
}


#include "Carcassonne/CARGame.h"


bool TinyGameApp::initializeGame()
{
	EngineInitialize();

	RedirectStdIO();

	gLogPrinter.addChannel(LOG_DEV);
	gLogPrinter.addChannel(LOG_MSG);
	gLogPrinter.addChannel(LOG_WARNING);
	gLogPrinter.addChannel(LOG_ERROR);

	LogMsg("OS Loc = %s", SystemPlatform::GetUserLocaleName().c_str());

	if (!Global::GameConfig().loadFile(GAME_SETTING_PATH))
	{
		LogWarning(0,"Can't load config file : %s" , GAME_SETTING_PATH );
	}

	if (!createWindowInternal(mGameWindow, gDefaultScreenWidth, gDefaultScreenHeight, TEXT("Tiny Game")))
		return false;

	ConsoleSystem::Get().initialize();

	FConsoleConfigUtilities::Import(Global::GameConfig(), CONFIG_SECTION);

	::Global::Initialize();

	Render::ShaderManager::Get().setDataCache(&::Global::DataCache());

	GameLoop::setUpdateTime( gDefaultTickTime );

	::Global::GetAssetManager().init();
	::Global::GetAssetManager().registerViewer(&gGameConfigAsset);

	Render::ShaderManager::Get().setAssetViewerRegister(&Global::GetAssetManager());

	exportUserProfile();

	mbLockFPS = ::Global::GameConfig().getIntValue("bLockFPS", nullptr, 0);

	::Global::GetDrawEngine().initialize( *this );

	::Global::GUI().initialize( *this );

	mConsoleWidget = new ConsoleFrame(UI_ANY, Vec2i(10, 10), Vec2i(600, 500), nullptr);
	mConsoleWidget->setGlobal();
	mConsoleWidget->addChannel(LOG_ERROR);
	mConsoleWidget->addChannel(LOG_DEV);
	mConsoleWidget->addChannel(LOG_MSG);
	mConsoleWidget->addChannel(LOG_WARNING);
	::Global::GUI().addWidget(mConsoleWidget);

	loadModules();

	setupStage();

	setConsoleShowMode(ConsoleShowMode::None);

	bool havePlayGame = false;
	char const* gameName;
	if ( ::Global::GameConfig().tryGetStringValue( "DefaultGame" , nullptr , gameName ) )
	{
		IGameModule* game = ::Global::ModuleManager().changeGame( gameName );
		if ( game )
		{
			game->beginPlay( *this, EGameStageMode::Single );
			havePlayGame = true;
		}
	}
	
	if ( havePlayGame == false )
	{
		changeStage( STAGE_MAIN_MENU );
	}
	
	mFPSCalc.init( getMillionSecond() );

	return true;
}

void TinyGameApp::finalizeGame()
{
	cleanup();
}

void TinyGameApp::cleanup()
{
	FConsoleConfigUtilities::Export(Global::GameConfig(), CONFIG_SECTION);

	StageManager::cleanup();

	closeNetwork();

	//cleanup widget before delete game instance
	Global::GUI().cleanupWidget(true , true);

	MiscTestRegister::GetList().clear();

	Global::ModuleManager().cleanupModuleInstances();

	Global::GUI().finalize();

	Global::GetDrawEngine().release();

	Global::GetAssetManager().cleanup();

	importUserProfile();

	Global::GameConfig().saveFile(GAME_SETTING_PATH);

	ILocalization::Get().saveTranslateAsset("LocText.txt");

	Global::Finalize();

	EngineFinalize();
	
	Global::ModuleManager().cleanupModuleMemory();
}

long TinyGameApp::handleGameUpdate( long shouldTime )
{
	int  numFrame = shouldTime / getUpdateTime();
	long updateTime = numFrame * getUpdateTime();
	
	::Global::GetAssetManager().tick(updateTime);

	::Global::GetDrawEngine().update(updateTime);

	for( int i = 0 ; i < numFrame ; ++i )
	{
		ProfileSystem::Get().incrementFrameCount();

		if( mStageMode && mStageMode->getStage() == nullptr )
		{
			delete mStageMode;
			mStageMode = nullptr;
		}
		checkNewStage();

		if ( mNetWorker )
			mNetWorker->update( getUpdateTime() );

		runTask( getUpdateTime() );

		getCurStage()->update( getUpdateTime() );

		::Global::GUI().update();

		IGameModule* game = Global::ModuleManager().getRunningGame();
		if( game )
		{
			game->getController().clearFrameInput();
		}
	}

	gLogPrinter.update( updateTime );
	return updateTime;
}


void TinyGameApp::handleGameIdle(long time)
{
	if ( mbLockFPS )
		SystemPlatform::Sleep(time);
	else
		render( 0.0f );
}


void TinyGameApp::handleGameRender()
{
	render( 0.0f );
}

void TinyGameApp::loadModules()
{
	FileIterator fileIter;

	FixString<MAX_PATH + 1> dir;
#if _DEBUG
	GetCurrentDirectory(
		dir.max_size(),
		dir.data()
	);
	if (FileSystem::FindFiles(dir, ".dll", fileIter))
#else
	
	::GetModuleFileName(NULL, dir.data(), dir.max_size());
	if (FileSystem::FindFiles(FileUtility::GetDirectory(dir).toCString(), ".dll", fileIter))
#endif
	{
		for ( ; fileIter.haveMore() ; fileIter.goNext() )
		{
			char const* fileName = fileIter.getFileName();
			if ( FCString::CompareN( fileName, "Game" , 4 ) != 0 &&
				 FCString::CompareN( fileName, "Test", 4) != 0  )
				continue;
#if _DEBUG
			if ( fileName[ FCString::Strlen( fileName ) - 5 ] != 'D' )
				continue;
#else 
			if ( fileName[ FCString::Strlen( fileName ) - 5 ] == 'D' )
				continue;
#endif

			Global::ModuleManager().loadModule( fileName );
		}
	}
}

void TinyGameApp::closeNetwork()
{
	if ( mNetWorker )
	{
		mNetWorker->closeNetwork();
		delete mNetWorker;
		mNetWorker = nullptr;
	}
}


ServerWorker* TinyGameApp::createServer()
{
	closeNetwork();

	ServerWorker* server = new ServerWorker;
	if ( !server->startNetwork() )
	{
		::Global::GUI().showMessageBox( 
			UI_ANY , LOCTEXT("Can't Create Server") , GMB_OK );
		delete server;
		return nullptr;
	}
	mNetWorker = server;

	return server;
}

ClientWorker* TinyGameApp::createClinet()
{
	closeNetwork();

	ClientWorker* worker;
	if ( ::Global::GameConfig().getIntValue("SimNetLag", nullptr, 0) )
	{
		DelayClientWorker* delayWorker = new DelayClientWorker();
		delayWorker->setDelay(
			::Global::GameConfig().getIntValue("SimNetLagDelay", nullptr, 30),
			::Global::GameConfig().getIntValue("SimNetLagDelayRand", nullptr, 0));
		worker = delayWorker;
	}
	else
	{
		worker = new ClientWorker();
	}

	if ( !worker->startNetwork() )
	{
		delete worker;
		::Global::GUI().showMessageBox( UI_ANY , LOCTEXT("Can't Create Client") ,  GMB_OK);
		return nullptr;
	}

	mNetWorker = worker;
	return worker;
}

void TinyGameApp::setConsoleShowMode(ConsoleShowMode mode)
{
	mConsoleShowMode = mode;
	if( mConsoleShowMode == ConsoleShowMode::GUI )
	{
		mConsoleWidget->show(true);
		mConsoleWidget->makeFocus();
		mConsoleWidget->setTop();
	}
	else
	{
		mConsoleWidget->show(false);
	}
	
}

bool TinyGameApp::handleMouseEvent( MouseMsg const& msg )
{
	bool result = true;

	IGameModule* game = Global::ModuleManager().getRunningGame();
	if ( game )
	{
		GameController& controller = game->getController();

		if ( !controller.shouldLockMouse() )
		{
			result = ::Global::GUI().procMouseMsg( msg );
		}

		if ( result )
			controller.recvMouseMsg( msg );
	}
	else
	{
		result = ::Global::GUI().procMouseMsg(msg);
	}

	if ( result )
	{
		result = getCurStage()->onMouse( msg );
		InputManager::Get().procMouseEvent( msg );
	}
	return result;
}

bool TinyGameApp::handleKeyEvent(KeyMsg const& msg)
{
	if ( msg.isDown() )
	{
		switch (msg.getCode())
		{
		case EKeyCode::F1:
			{
				mGameWindow.toggleFullscreen();
			}
			break;
		case EKeyCode::Oem3:
			{
				setConsoleShowMode(ConsoleShowMode((int(mConsoleShowMode) + 1) % int(ConsoleShowMode::Count)));
			}
			break;
		case EKeyCode::X:
			loadModules();
			break;
		case EKeyCode::F11:
			{
				using namespace Render;
				if (mRenderSetup)
				{
					mRenderSetup->preShutdownRenderSystem(true);
					ERenderSystem systemName = GRHISystem->getName() == RHISystemName::OpenGL ? ERenderSystem::D3D11 : ERenderSystem::OpenGL;
					Global::GetDrawEngine().shutdownSystem(false);
					RenderSystemConfigs configs;

					mRenderSetup->configRenderSystem(systemName, configs);
					
					if (!Global::GetDrawEngine().startupSystem(systemName, configs))
					{


					}

					if (!mRenderSetup->setupRenderSystem(systemName))
					{


					}
				}
			}
			break;
		}
	}
	bool result = ::Global::GUI().procKeyMsg(msg);
	if( result )
	{
		result = getCurStage()->onKey(msg);
	}
	return result;
}

bool TinyGameApp::handleCharEvent( unsigned code )
{
	bool result = ::Global::GUI().procCharMsg(code);
	if( result )
	{
		result = getCurStage()->onChar(code);
	}
	return result;
}

bool TinyGameApp::handleWindowDestroy(HWND hWnd )
{
	if (::Global::GetDrawEngine().bBlockRender)
		return false;

	if( mGameWindow.getHWnd() == hWnd )
	{
		setLoopOver(true);
		return true;
	}	
	return false;
}

void TinyGameApp::handleWindowPaint(HDC hDC)
{

	if( !::Global::GetDrawEngine().isInitialized() )
		return;

	render(0.0f);
}

bool TinyGameApp::handleWindowActivation( bool beA )
{
	LogMsg("Window %s", beA ? "Active" : "Deactive");
	InputManager::Get().bActive = beA;

	return true;
}

void TinyGameApp::onTaskMessage( TaskBase* task , TaskMsg const& msg )
{
	getCurStage()->onTaskMessage( task , msg );

	if ( msg.onStart() )
	{
		RenderEffect* effect = dynamic_cast< RenderEffect* >( task );
		if ( effect )
			mRenderEffect = effect;
	}
	else if ( msg.onEnd() )
	{
		if ( mRenderEffect == task )
			mRenderEffect = nullptr;
	}
}

void TinyGameApp::render( float dframe )
{
	using namespace Render;

	if ( getNextStage() || mbInitializingStage )
		return;

	DrawEngine& drawEngine = Global::GetDrawEngine();

	if ( !drawEngine.beginRender() )
		return;

	if ( GbProfileGPU )
		GpuProfiler::Get().beginFrame();

	bool bDrawScene = ( mStageMode == nullptr ) || mStageMode->canRender();

	if( bDrawScene )
	{
		getCurStage()->render(dframe);

		long dt = long(dframe * getUpdateTime());

		if( mRenderEffect )
			mRenderEffect->onRender(dt);

		if( drawEngine.isUsageRHIGraphic2D() )
			::Global::GetRHIGraphics2D().beginRender();

		{
			GPU_PROFILE("GUI");
			::Global::GUI().render();
		}
	}
	else
	{
		if( drawEngine.isUsageRHIGraphic2D() )
			::Global::GetRHIGraphics2D().beginRender();
	}

	if( mConsoleShowMode == ConsoleShowMode::Screen )
	{
		gLogPrinter.render(Vec2i(5, 25));
	}


	if( GbProfileGPU )
		GpuProfiler::Get().endFrame();
	
	mFPSCalc.increaseFrame(getMillionSecond());
	IGraphics2D& g = ::Global::GetIGraphics2D();
	if ( CVarShowFPS )
	{
		FixString< 256 > str;
		RenderUtility::SetFont(g, FONT_S8);
		g.setTextColor(Color3ub(255, 255, 0));
		str.format("FPS = %3.1f", mFPSCalc.getFPS());
		g.drawText(Vec2i(5, 5), str);
		//g.drawText(Vec2i(5, 15), str.format("mode = %d", (int)mConsoleShowMode));
	}

	if (GbDrawGPUUsage)
	{
		struct LocalInit
		{
			GPUDeviceQuery* deviceQuery = nullptr;
			LocalInit()
			{
				deviceQuery = GPUDeviceQuery::Create();
				if (deviceQuery == nullptr)
				{
					LogWarning(0, "GPU device query can't create");
				}
			}
			~LocalInit()
			{
				if (deviceQuery)
				{
					deviceQuery->release();
				}
			}
		};

		static LocalInit sLocalInit;

		GPUDeviceQuery* deviceQuery = sLocalInit.deviceQuery;
		if (deviceQuery)
		{
			GPUStatus status;
			if (deviceQuery->getGPUStatus(0, status))
			{
				RenderUtility::SetFont(g, FONT_S8);

				float const width = 15;

				Vector2 renderPos = ::Global::GetScreenSize() - Vector2(5, 5);
				Vector2 renderSize = Vector2(width, 100 * float(status.usage) / 100);
				RenderUtility::SetPen(g, EColor::Red);
				RenderUtility::SetBrush(g, EColor::Red);
				g.drawRect(renderPos - renderSize, renderSize);

				Vector2 textSize = Vector2(width, 20);
				g.setTextColor(Color3ub(0, 255, 255));
				g.drawText(renderPos - textSize, textSize, FStringConv::From(status.usage));

				renderPos.x -= width + 5;
				renderSize = Vector2(width, 100 * float(status.temperature) / 100);
				RenderUtility::SetPen(g, EColor::Yellow);
				RenderUtility::SetBrush(g, EColor::Yellow);
				g.drawRect(renderPos - renderSize, renderSize);

				g.setTextColor(Color3ub(0, 0, 255));
				g.drawText(renderPos - textSize, textSize, FStringConv::From(status.temperature));
			}
		}
	}

	if( GbProfileGPU && RHIIsInitialized() )
	{
		g.setTextColor(Color3ub(255, 0, 0));
		RenderUtility::SetFont(g, FONT_S10);

		SimpleTextLayout textlayout;
		textlayout.offset = 15;
		textlayout.posX = 500;
		textlayout.posY = 10;
		FixString< 512 > str;
		FixString< 512 > temp;
		int curLevel = 0;
		auto GetSystemNameString = [](RHISystemName name)
		{
			switch (name)
			{
			case RHISystemName::OpenGL: return "OpenGL";
			case RHISystemName::D3D11:  return "D3D11";
			case RHISystemName::D3D12: return "D3D12";
			case RHISystemName::Vulkan: return "Vulkan";
			}
			return "Unknown";
		};
		for( int i = 0; i < GpuProfiler::Get().getSampleNum(); ++i )
		{
			GpuProfileSample* sample = GpuProfiler::Get().getSample(i);

			if( curLevel != sample->level )
			{
				if( sample->level > curLevel )
				{
					assert(curLevel == sample->level - 1);
					temp += "  |";
				}
				else
				{
					temp[3 * sample->level] = 0;

				}
				curLevel = sample->level;
			}
			textlayout.show( g , "%7.4lf %s--> %s", sample->time, temp.c_str() , sample->name.c_str());
		}
	}

	if( drawEngine.isUsageRHIGraphic2D() )
		::Global::GetRHIGraphics2D().endRender();
		
	drawEngine.endRender();
}

void TinyGameApp::exportUserProfile()
{
	PropertySet& setting = Global::GameConfig();
	UserProfile& userPorfile = Global::GetUserProfile();

	userPorfile.name = setting.getStringValue( "Name" , "Player" , "Player" );
	char const* lan = setting.getStringValue( "Language" , "Player" , "Chinese-T" );
	if ( strcmp( lan , "Chinese-T" ) == 0 )
	{
		userPorfile.language = LAN_CHINESE_T;
	}
	else if ( strcmp( lan , "English" ) == 0 )
	{
		userPorfile.language = LAN_ENGLISH;
	}
	else
	{
		userPorfile.language = LAN_CHINESE_T;
	}

	ILocalization::Get().initialize( ( Language )userPorfile.language );
}

void TinyGameApp::importUserProfile()
{
	PropertySet& setting = Global::GameConfig();

	UserProfile& userPorfile = Global::GetUserProfile();

	setting.setKeyValue( "Name" , "Player" , userPorfile.name.c_str() );
	switch( userPorfile.language )
	{
	case LAN_ENGLISH:
		setting.setKeyValue( "Language" , "Player" , "English" );
		break;
	case LAN_CHINESE_T:
	default:
		setting.setKeyValue( "Language" , "Player" , "Chinese-T" );
		break;
	}

}

StageBase* TinyGameApp::createStage( StageID stageId )
{
	StageBase* newStage = nullptr;

	IGameModule* curGame = Global::ModuleManager().getRunningGame();

	if ( curGame )
	{
		newStage = curGame->createStage( stageId );
		if ( newStage )
		{
			if (GameStageBase* gameStage = newStage->getGameStage())
			{
				GameStageMode* stageMode = createGameStageMode(stageId);
				gameStage->setupStageMode(stageMode);
			}
		}
	}

	if ( !newStage )
	{
		switch ( stageId )
		{

#define CASE_STAGE( id , Class )\
		case id : newStage = new Class ; break;

		CASE_STAGE( STAGE_MAIN_MENU   , MainMenuStage )
		CASE_STAGE( STAGE_NET_ROOM    , NetRoomStage )
		CASE_STAGE( STAGE_REPLAY_EDIT , ReplayEditStage )
#undef CASE_STAGE
		default:
			LogError( "Can't find Stage %d " , stageId );
			return nullptr;
		}
	}

	return newStage;
}


GameStageMode* TinyGameApp::createGameStageMode(StageID stageId)
{
	GameStageMode* stageMode = nullptr;

	switch( stageId )
	{
#define CASE_STAGE( idx , Class )\
				case idx : stageMode  = new Class; break;

		CASE_STAGE(STAGE_SINGLE_GAME, SingleStageMode)
		CASE_STAGE(STAGE_NET_GAME, NetLevelStageMode)
		CASE_STAGE(STAGE_REPLAY_GAME, ReplayStageMode)

#undef CASE_STAGE
	}
	return stageMode;
}

StageBase* TinyGameApp::resolveChangeStageFail( FailReason reason )
{
	LogMsg("Change Stage Fail!!");
	switch( reason )
	{
	case FailReason::InitFail:
		if( ::Global::GetDrawEngine().isRHIEnabled() )
		{
			::Global::GetDrawEngine().shutdownSystem(false);
		}
		break;
	case FailReason::NoStage:
		//if ( !mErrorMsg.empty() )
		//	mShowErrorMsg = true;
		break;
	}
	return createStage( STAGE_MAIN_MENU );
}

bool TinyGameApp::initializeStage(StageBase* stage)
{
	TGuardValue< bool > initializingStageGuard(mbInitializingStage, true);
	GameStageBase* gameStage = stage->getGameStage();

	if(gameStage)
	{
		mStageMode = gameStage->getStageMode();
		if( !mStageMode->prevStageInit() )
			return false;
	}

	ERenderSystem systemName = ERenderSystem::None;

	IGameRenderSetup* renderSetup = dynamic_cast<IGameRenderSetup*>(stage);
	if ( renderSetup )
	{
		mRenderSetup = renderSetup;
		systemName = renderSetup->getDefaultRenderSystem();
		if (systemName == ERenderSystem::None)
		{
			if (renderSetup->isRenderSystemSupported(ERenderSystem::OpenGL))
				systemName = ERenderSystem::OpenGL;
		}

		RenderSystemConfigs configs;
		renderSetup->configRenderSystem(systemName, configs);

		if (!::Global::GetDrawEngine().startupSystem(systemName, configs))
		{
			return false;
		}
	}
	else
	{


	}

	if( !stage->onInit() )
		return false;

	if (renderSetup)
	{
		if (!renderSetup->setupRenderSystem(systemName))
		{
			LogWarning(0, "Can't Initialize Stage Render Resource");
			return false;
		}
	}

	if (gameStage)
	{
		if (!mStageMode->postStageInit())
			return false;
	}

	stage->postInit();
	return true;
}

void TinyGameApp::prevStageChange()
{
	DrawEngine& de = ::Global::GetDrawEngine();
	Graphics2D& g = ::Global::GetGraphics2D();
	if ( de.beginRender() )
	{
		RenderUtility::SetBrush( g , EColor::Black );
		RenderUtility::SetPen( g , EColor::Black );
		g.drawRect( Vec2i(0,0) , ::Global::GetScreenSize() );
		de.endRender();
	}
}

void TinyGameApp::postStageEnd(StageBase* stage)
{
	IGameRenderSetup* renderSetup = dynamic_cast<IGameRenderSetup*>(stage);
	if (renderSetup)
	{
		Global::GetDrawEngine().shutdownSystem(true);
	}
}

void TinyGameApp::postStageChange( StageBase* stage )
{
	addTask( new FadeInEffect( EColor::Black , 1000 ) , this );
	if ( mShowErrorMsg )
	{
		//::Global::getGUI().showMessageBox( UI_ANY , mErrorMsg.c_str() , GMB_OK );
		mShowErrorMsg = false;
	}
}

NetWorker* TinyGameApp::buildNetwork( bool beServer )
{
	if ( beServer )
		return createServer();
	return createClinet();
}

void TinyGameApp::addGUITask(TaskBase* task, bool bGlobal)
{
	TaskHandler* handler = (bGlobal) ? static_cast<TaskHandler*>( this ) : getCurStage();
	handler->addTask(task);
}

void TinyGameApp::dispatchWidgetEvent( int event , int id , GWidget* ui )
{
	if ( !getCurStage()->onWidgetEvent( event , id , ui ) )
		return;
	if ( id >= UI_STAGE_ID )
		return;
	//if ( !mGameMode->onWidgetEvent( event , id , ui ) )
	//	return false;
	if ( id >= UI_GAME_STAGE_MODE_ID )
		return;

	switch ( id )
	{
	//case UI_SINGLE_GAME:
		//changeNextStage( MAIN_STAGE );
		//restartGame( 0 ); 
		break;
	case UI_EXIT_GAME:
		if ( event == EVT_BOX_YES )
		{
			setLoopOver( true );
		}
		else if ( event == EVT_BOX_NO )
		{

		}
		else
		{
			::Global::GUI().showMessageBox( 
				UI_EXIT_GAME , LOCTEXT("Be Sure To Exit The Game?") );
		}
		break;
	case UI_MAIN_MENU:
		if ( event == EVT_BOX_YES )
		{
			changeStage( STAGE_MAIN_MENU );
		}
		else if ( event == EVT_BOX_NO )
		{

		}
		else
		{
			::Global::GUI().showMessageBox( 
				UI_MAIN_MENU , LOCTEXT("Be Sure Back To Main Menu?") );
		}

		break;
	case UI_BUILD_CLIENT:
		{
			closeNetwork();

			ClientWorker* worker = createClinet();
			if ( !worker )
				return;
			NetRoomStage* stage = static_cast< NetRoomStage* >( changeStage( STAGE_NET_ROOM ) );
			stage->initWorker( worker );
		}
		break;
	case UI_CREATE_SERVER:
		{
			closeNetwork();

			ServerWorker* server = createServer();
			if ( !server )
				return;

			LocalWorker* worker = server->createLocalWorker(::Global::GetUserProfile().name );

			NetRoomStage* stage = static_cast< NetRoomStage* >( changeStage( STAGE_NET_ROOM ) );
			stage->initWorker( worker , server );	
		}
		break;
	case UI_GAME_MENU:
		if ( event == EVT_BOX_YES )
		{
			if ( mNetWorker )
			{
				mNetWorker->closeNetwork();
			}
			changeStage( STAGE_GAME_MENU );

		}
		break;
	}
}

GameWindow& TinyGameApp::getMainWindow()
{
	return mGameWindow;
}

bool TinyGameApp::reconstructWindow( GameWindow& window )
{
	int width = window.getWidth();
	int height = window.getHeight();

	Vec2i pos = window.getPosition();
	TCHAR const* title = TEXT("Tiny Game");
	window.destroy();
	if( !createWindowInternal(window , width, height , title) )
	{
		return false;
	}

	window.setPosition(pos);
	return true;
}

GameWindow* TinyGameApp::createWindow(Vec2i const& pos, Vec2i const& size, char const* title)
{
	return nullptr;
}

bool TinyGameApp::createWindowInternal(GameWindow& window , int width, int height ,TCHAR const* title)
{
	if( !window.create(title, width, height, WindowsMessageHandler::MsgProc) )
		return false;

	return true;
}

FadeInEffect::FadeInEffect( int _color , long time )
	:RenderEffect( time )
{
	color = _color;
	totalTime = time;
}

void FadeInEffect::onRender( long dt )
{
	DrawEngine& de = Global::GetDrawEngine();

	Vec2i size = de.getScreenSize() + Vec2i(5,5); 
	Graphics2D& g = Global::GetGraphics2D();

	g.beginBlend( Vec2i(0,0) , size , float( getLifeTime() - dt ) / totalTime  ); 

	RenderUtility::SetBrush( g , color );
	g.drawRect( Vec2i(0,0) , size );

	g.endBlend();
}
