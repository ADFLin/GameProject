#include "TinyGamePCH.h"
#include "TinyGameApp.h"

#include "DrawEngine.h"
#include "RenderUtility.h"

#include "PropertySet.h"
#include "InputManager.h"
#include "FileSystem.h"
#include "ProfileSystem.h"
#include "ConsoleSystem.h"
#include "DataCacheInterface.h"
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

#include "Hardware/GPUDeviceQuery.h"
#include "StringParse.h"
#include "Core/Tickable.h"
#include "Module/ModuleManager.h"

#include <iostream>
#include "Launch/CommandlLine.h"
#include <unordered_set>

#define GAME_SETTING_PATH "Game.ini"
#define CONFIG_SECTION "SystemSetting"

int  GDevMsgLevel = -1;

namespace
{
	TConsoleVariable<bool> CVarProfileGPU{ true , "ProfileGPU", CVF_TOGGLEABLE };
	TConsoleVariable<bool> CVarProfileCPU{ true , "ProfileCPU", CVF_TOGGLEABLE };
	TConsoleVariable<bool> CVarDrawGPUUsage{ false, "r.GPUUsage", CVF_TOGGLEABLE };
	TConsoleVariable<bool> CVarShowFPS{ false, "ShowFPS" , CVF_TOGGLEABLE };
	TConsoleVariable<bool> CVarLockFPS{ false, "LockFPS" , CVF_TOGGLEABLE };
	TConsoleVariable<bool> CVarShowProifle{ false, "ShowProfile" , CVF_TOGGLEABLE };
	TConsoleVariable<bool> CVarShowGPUProifle{ true, "ShowGPUProfile" , CVF_TOGGLEABLE };
	AutoConsoleCommand CmdRHIDumpResource("r.dumpResource", Render::RHIResource::DumpResource);
}

TINY_API IGameNetInterface* GGameNetInterfaceImpl;
TINY_API IDebugInterface*   GDebugInterfaceImpl;

void ToggleGraphics()
{
	static ERenderSystem sSavedRenderSystem = ERenderSystem::OpenGL;

	IGameModule* game = ::Global::ModuleManager().getRunningGame();
	GameAttribute attribute(ATTR_GRAPRHICS_SWAP_SUPPORT);
	if (game && game->queryAttribute(attribute) && attribute.iVal != 0)
	{
		::Global::GetDrawEngine().toggleGraphics();
	}
}
AutoConsoleCommand CmdToggleRHISystem("g.ToggleRHI", ToggleGraphics);

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


struct EOutputColorGeneric
{
	enum Type
	{
		Black = 0,
		Red,
		Blue,
		Green,
		Cyan,
		Magenta,
		Yellow,
		DarkGray,
		DarkRed,
		DarkBlue,
		DarkGreen,
		DarkCyan,
		DarkMagenta,
		DarkYellow,
		Gray,
		White,
	};
};

#if SYS_PLATFORM_WIN
struct EOutputColorWindows
{
	enum Type
	{
		Black = 0,
		DarkRed   = FOREGROUND_RED,
		DarkBlue  = FOREGROUND_BLUE,
		DarkGreen = FOREGROUND_GREEN,
		DarkCyan  = DarkGreen | DarkBlue,
		
		DarkMagenta = DarkRed | DarkBlue,
		DarkYellow  = DarkRed | DarkGreen,
		Gray        = DarkRed | DarkGreen | DarkBlue,
		DarkGray    = FOREGROUND_INTENSITY,
		Red         = FOREGROUND_INTENSITY | DarkRed,
		Green       = FOREGROUND_INTENSITY | DarkGreen,
		Blue        = FOREGROUND_INTENSITY | DarkBlue,
		Cyan        = FOREGROUND_INTENSITY | DarkGreen | DarkBlue,
		Magenta     = FOREGROUND_INTENSITY | DarkRed | DarkBlue,
		Yellow      = FOREGROUND_INTENSITY | DarkRed | DarkGreen,
		White       = FOREGROUND_INTENSITY | DarkRed | DarkGreen | DarkBlue,
	};
};
using EOutputColor = EOutputColorWindows;
#else
using EOutputColor = EOutputColorGeneric;
#endif

class FConsoleConfigUtilities
{
public:

	static bool SpiltSection(char const* & inoutStr , StringView& outSection)
	{
		char const* ptrFound = FCString::FindLastChar(inoutStr, '.');
		if (*ptrFound == 0)
			return false;
		
		outSection = StringView(inoutStr, ptrFound - inoutStr);
		inoutStr = ptrFound + 1;
		return true;
	}
	static int Export(PropertySet& configs, char const* sectionGroup)
	{
		int result = 0;
		ConsoleSystem::Get().visitAllVariables(
			[sectionGroup, &configs ,&result](VariableConsoleCommadBase* var)
			{
				if (var->getFlags() & CVF_CONFIG)
				{
					char const* name = var->mName.c_str();
					if (var->getFlags() & CVF_SECTION_GROUP)
					{
						StringView section;
						if (SpiltSection(name, section))
						{
							configs.setKeyValue(name, section.toMutableCString(), var->toString().c_str());
							++result;
							return;
						}
					}

					configs.setKeyValue(name, sectionGroup, var->toString().c_str());
					++result;
				}
			}
		);
		return result;
	}

	static int Import(PropertySet& configs, char const* sectionGroup, std::unordered_set<VariableConsoleCommadBase*>& inoutVarSet)
	{
		int result = 0;
		configs.visitSection(sectionGroup,
			[&configs, &inoutVarSet, &result](char const* key, KeyValue const& value)
			{
				auto com = ConsoleSystem::Get().findCommand(key);
				if (com == nullptr)
					return;
				auto var = com->asVariable();
				if (var == nullptr)
					return;

				if (var->getFlags() & CVF_CONFIG)
				{
					if (!inoutVarSet.emplace(var).second)
						return;

					if (var->setFromString(value.getStringView()))
					{
						++result;
					}
				}
			}
		);

		ConsoleSystem::Get().visitAllVariables(
			[&configs, &inoutVarSet, &result](VariableConsoleCommadBase* var)
			{
				uint32 const Flags = CVF_CONFIG | CVF_SECTION_GROUP;
				if ( (var->getFlags() & Flags) == Flags)
				{
					if (!inoutVarSet.emplace(var).second)
						return;

					char const* name = var->mName.c_str();

					StringView section;
					if (SpiltSection(name, section))
					{
						char const* valueStr;
						if (configs.tryGetStringValue(name, section.toMutableCString(), valueStr))
						{
							var->setFromString(valueStr);
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
		std::ios_base::sync_with_stdio(true);
	}


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
		WriteConsoleA(hConsole, prefixStr, FCString::Strlen(prefixStr), NULL , NULL);
		//std::cout << prefixStr;
		SetConsoleTextAttribute(hConsole, textColor);
		WriteConsoleA(hConsole, text, FCString::Strlen(text), NULL, NULL);
		WriteConsoleA(hConsole, "\n", FCString::Strlen("\n"), NULL, NULL);
		//std::cout << text << std::endl;
#else
		std::cout << prefixStr << text << std::endl;
#endif
	}


	void receiveLog( LogChannel channel , char const* str ) override
	{
		Mutex::Locker locker(mMutex);

		switch (channel)
		{
		case LOG_MSG:     OutputConsoleString(GetChannelPrefixStr(channel), str, EOutputColor::White, EOutputColor::Gray); break;
		case LOG_DEV:     OutputConsoleString(GetChannelPrefixStr(channel), str, EOutputColor::Blue, EOutputColor::Blue); break;
		case LOG_WARNING: OutputConsoleString(GetChannelPrefixStr(channel), str, EOutputColor::Yellow , EOutputColor::Yellow); break;
		case LOG_ERROR:   OutputConsoleString(GetChannelPrefixStr(channel), str, EOutputColor::Red, EOutputColor::Red); break;
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

		mLogTextList.emplace( str );
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
		{
			Mutex::Locker locker(mMutex);
			for (auto& text : mLogTextList)
			{
				g.drawText(pos, text.c_str());
				pos.y += 14;
			}
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

	static int const MaxLineNum = 20;
	TCycleBuffer< std::string , MaxLineNum > mLogTextList;
};


class GameConfigAsset : public IAssetViewer
{
public:
	void getDependentFilePaths(TArray< std::wstring >& paths) override
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
static TinyGameApp* GApp;

TinyGameApp::TinyGameApp()
	:mRenderEffect( nullptr )
	,mNetWorker( nullptr )
	,mStageMode( nullptr )
{
	mShowErrorMsg = false;

	GGameNetInterfaceImpl = this;
	GDebugInterfaceImpl = &gLogPrinter;
	GApp = this;
}

TinyGameApp::~TinyGameApp()
{

}


#if SYS_PLATFORM_WIN
#include <fcntl.h>
#include <corecrt_io.h>
#include <iostream>
BOOL WINAPI HandleConsoleEvent(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT:
		GApp->setLoopOver(true);
		return TRUE;
	}
	return TRUE;
}
#endif
void CreateConsole()
{
#if SYS_PLATFORM_WIN
	if (AllocConsole()) 
	{
		FILE* pCout;
		freopen_s(&pCout, "CONOUT$", "w", stdout);
		freopen_s(&pCout, "CONOUT$", "w", stderr);

		SetConsoleTitle(TEXT("Debug Console"));
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), EOutputColor::Green);
		SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), EOutputColor::Red);
		SetConsoleCtrlHandler(HandleConsoleEvent, TRUE);
	}
#endif
}

char const* GetBuildVersion()
{
	struct BuildVersionBuilder
	{
		char dayOfTheWeek[32];
		char mouth[32];
		int date;
		int hr;
		int min;
		int sec;
		int year;

		InlineString<> build()
		{
			sscanf(__TIMESTAMP__, "%s %s %d %d:%d:%d %d", dayOfTheWeek, mouth, &date, &hr, &min, &sec, &year);
			return InlineString<>::Make(
				"%d-%s-%d-%02d:%02d:%02d-%s",
				year, mouth, date , hr , min , sec , 
#if _DEBUG
				"debug"
#else
				"release"
#endif
			);
		}
	};

	static InlineString<> buildVersionCached = BuildVersionBuilder().build();
	return buildVersionCached;
}

struct FData
{
	FData(int index)
		:index(index)
	{
		LogMsg("Data %d Construction", index);
	}

	FData(FData&& InData)
	{
		index = InData.index;
		LogMsg("Data %d Move Construction", index);
	}

	FData(FData const& InData)
	{
		index = InData.index;
		LogMsg("Data %d Copy Construction", index);
	}

	~FData()
	{
		LogMsg("Data %d Destruction", index);
	}

	int index;
	std::function< void() > func;
	std::unordered_set<HashString> set;
};

BITWISE_RELLOCATABLE_FAIL(FData);

void RunArrayTest()
{
	TArray<FData> List;
	for (int i = 0; i < 64; ++i)
	{
		List.emplace_back(i);
	}

	for (int i = 0; i < 64; ++i)
	{
		for (auto const& v : List[i].set)
		{


		}
	}
}


bool TinyGameApp::initializeGame()
{
	TIME_SCOPE("Game Initialize");
	
	{
		TIME_SCOPE("Global Initialize");
		::Global::Initialize();
	}

	CreateConsole();

	gLogPrinter.addDefaultChannels();

#if SYS_PLATFORM_WIN && 0
	{
		InlineString<MAX_PATH + 1> moduleDir;
		::GetModuleFileNameA(NULL, moduleDir.data(), moduleDir.max_size());
		moduleDir = FFileUtility::GetDirectory(moduleDir);
		LogMsg("Exec Path = %s", moduleDir.c_str());
		::SetCurrentDirectoryA(moduleDir);
	}
#endif

	LogMsg("BuildVersion = %s", GetBuildVersion());

	{
		TIME_SCOPE("Engine Initialize");
		EngineInitialize();
	}

	LogMsg("OS Loc = %s", SystemPlatform::GetUserLocaleName().c_str());
	setlocale(LC_ALL, SystemPlatform::GetUserLocaleName().c_str());
	{
		TIME_SCOPE("Load GameConfig");
		if (!Global::GameConfig().loadFile(GAME_SETTING_PATH))
		{
			LogWarning(0, "Can't load config file : %s", GAME_SETTING_PATH);
		}

		importUserProfile();
		CVarLockFPS = ::Global::GameConfig().getIntValue("bLockFPS", nullptr, 0);
	}

#if TINY_WITH_EDITOR
	if (!FCString::StrStr(FCommandLine::Get(), "-NoEditor"))
	{
		bool bEnableEditor = FCString::StrStr(FCommandLine::Get(), "-Editor");
		if (!bEnableEditor)
		{
			bEnableEditor = ::Global::GameConfig().getBoolValue("Editor", CONFIG_SECTION, false);
		}
		if (bEnableEditor)
		{
			TIME_SCOPE("Editor Initialize");
			VERIFY_RETURN_FALSE(initializeEditor());
		}
	}
#endif

	std::unordered_set<VariableConsoleCommadBase*> cmdVarSet;
	{
		TIME_SCOPE("Console Initialize");
		ConsoleSystem::Get().initialize();
		ExecutionRegisterHelper::Manager = this;
		{
			TIME_SCOPE("Import Config");
			FConsoleConfigUtilities::Import(Global::GameConfig(), CONFIG_SECTION, cmdVarSet);
		}
	}

	{
		TIME_SCOPE("Create Window");
		if (!createWindowInternal(mGameWindow, gDefaultScreenWidth, gDefaultScreenHeight, TEXT("Tiny Game")))
			return false;
	}


	{

		char const* ignoreTypes =::Global::GameConfig().getStringValue("DataCache.IgnoreType", CONFIG_SECTION, "");
		StringView token;
		while (FStringParse::StringToken(ignoreTypes, " ,", token))
		{
			if (token.size() > 0)
			{
				::Global::DataCache().ignoreDataType(token.toCString());
			}
		}
	}

	{
		TIME_SCOPE("Asset Misc");
		Render::ShaderManager::Get().setDataCache(&::Global::DataCache());

		GameLoop::setUpdateTime(gDefaultTickTime);

		::Global::GetAssetManager().init();
		::Global::GetAssetManager().registerViewer(&gGameConfigAsset);

		Render::ShaderManager::Get().setAssetViewerRegister(&Global::GetAssetManager());

	}
	{
		TIME_SCOPE("Draw Initialize");
		::Global::GetDrawEngine().initialize(*this);
	}

	{
		TIME_SCOPE("GUI Initialize");
		::Global::GUI().initialize(*this);
	}
#if TINY_WITH_EDITOR
	if ( mEditor )
	{
		TIME_SCOPE("Editor InitializeRender");
		VERIFY_RETURN_FALSE(initializeEditorRender());
	}
#endif

	{
		TIME_SCOPE("Common Widget");
		mConsoleWidget = new ConsoleFrame(UI_ANY, Vec2i(10, 10), Vec2i(600, 500), nullptr);
		mConsoleWidget->setGlobal();
		mConsoleWidget->addDefaultChannels();
		::Global::GUI().addWidget(mConsoleWidget);
	}

	{
		TIME_SCOPE("Load Modules");
		loadModules();
		{
			TIME_SCOPE("Import Config");
			FConsoleConfigUtilities::Import(Global::GameConfig(), CONFIG_SECTION, cmdVarSet);
		}
	}

	{
		TIME_SCOPE("Stage Initialize");
		setupStage();

		setConsoleShowMode(ConsoleShowMode::None);

		bool havePlayGame = false;
		char const* gameName;
		if (::Global::GameConfig().tryGetStringValue("DefaultGame", nullptr, gameName))
		{
			IGameModule* game = ::Global::ModuleManager().changeGame(gameName);
			if (game)
			{
				game->beginPlay(*this, EGameStageMode::Single);
				havePlayGame = true;
			}
		}

		if (havePlayGame == false)
		{
			changeStage(STAGE_MAIN_MENU);
		}

		mFPSCalc.init(mClock.getTimeMilliseconds());
	}

	::ProfileSystem::Get().resetSample();
	return true;
}

void TinyGameApp::finalizeGame()
{
	cleanup();
}

void TinyGameApp::cleanup()
{
#if TINY_WITH_EDITOR
	finalizeEditor();
#endif

	FConsoleConfigUtilities::Export(Global::GameConfig(), CONFIG_SECTION);

	StageManager::cleanup();

	ExecutionRegisterCollection::Get().cleanup();

	closeNetwork();

	//cleanup widget before delete game instance
	Global::GUI().cleanupWidget(true , true);

	Global::ModuleManager().cleanupModuleInstances();

	Global::GUI().finalize();

	Global::GetDrawEngine().release();

	Global::GetAssetManager().cleanup();

	exportUserProfile();

	Global::GameConfig().saveFile(GAME_SETTING_PATH);

	ILocalization::Get().saveTranslateAsset("LocText.txt");

	Global::Finalize();

	EngineFinalize();
	
	Global::ModuleManager().cleanupModuleMemory();
}


long TinyGameApp::handleGameUpdate( long shouldTime )
{	
	PROFILE_FUNCTION();

	ProfileSystem::Get().incrementFrameCount();

	Tickable::Update(float(shouldTime) / 1000.0);

#if TINY_WITH_EDITOR
	if (mEditor)
	{
		PROFILE_ENTRY("Editor Update");
		mEditor->update();
	}
#endif

#if TINY_WITH_EDITOR
	if (mEditor)
	{
		PROFILE_ENTRY("Editor Render");
		GPU_PROFILE("Editor Render");
		mEditor->render();

	}
#endif
	int  numFrame = shouldTime / getUpdateTime();
	long updateTime = numFrame * getUpdateTime();

	::Global::GetAssetManager().tick(updateTime);

	::Global::GetDrawEngine().update(updateTime);

	for (int i = 0; i < numFrame; ++i)
	{
		if (mStageMode && mStageMode->getStage() == nullptr)
		{
			delete mStageMode;
			mStageMode = nullptr;
		}
		checkNewStage();

		if (mNetWorker)
		{
			mNetWorker->update(getUpdateTime());
		}

		{
			PROFILE_ENTRY("Task Update");
			runTask(getUpdateTime());
		}

		{
			PROFILE_ENTRY("Stage Update");
			getCurStage()->update(getUpdateTime());
		}
		{
			PROFILE_ENTRY("GUI Update");
			::Global::GUI().update();
		}


		IGameModule* game = Global::ModuleManager().getRunningGame();
		if (game)
		{
			game->getInputControl().clearFrameInput();
		}
	}

	gLogPrinter.update(updateTime);
	return updateTime;
}


void TinyGameApp::handleGameIdle(long time)
{
	PROFILE_ENTRY("GameIdle");
#if 0
	if ( CVarLockFPS )
		SystemPlatform::Sleep(time);
	else
		render( 0.0f );
#else
	SystemPlatform::Sleep(time / 2);
#endif
}

void TinyGameApp::handleGameRender()
{
	render( 0.0f );
}

void TinyGameApp::loadModules()
{
	//ensure game manager created
	Global::ModuleManager();

	InlineString<MAX_PATH + 1> moduleDir;
#if _DEBUG
	GetCurrentDirectoryA(
		moduleDir.max_size(),
		moduleDir.data()
	);
#else
	::GetModuleFileNameA(NULL, moduleDir.data(), moduleDir.max_size());
	moduleDir = FFileUtility::GetDirectory(moduleDir);
#endif

	if (!ModuleManager::Get().loadModulesFromFile("TinyGame.module"))
	{

	}

	FileIterator fileIter;
	if (FFileSystem::FindFiles(moduleDir, ".dll", fileIter))
	{
		for ( ; fileIter.haveMore() ; fileIter.goNext() )
		{
			char const* fileName = fileIter.getFileName();
			if ( FCString::CompareN(fileName, "Game", 4 ) != 0 &&
				 FCString::CompareN(fileName, "Test", 4) != 0 )
				continue;
#if _DEBUG
			if ( fileName[ FCString::Strlen( fileName ) - 5 ] != 'D' )
				continue;
#else 
			if ( fileName[ FCString::Strlen( fileName ) - 5 ] == 'D' )
				continue;
#endif

			ModuleManager::Get().loadModule( fileName );
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
			UI_ANY, LOCTEXT("Can't Create Server") , EMessageButton::Ok );
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
		::Global::GUI().showMessageBox( UI_ANY , LOCTEXT("Can't Create Client") ,  EMessageButton::Ok);
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
		mConsoleWidget->clearFocus();
	}
	
}

MsgReply TinyGameApp::handleMouseEvent(MouseMsg const& msg)
{
	MsgReply result;

	IGameModule* game = Global::ModuleManager().getRunningGame();
	if (game)
	{
		InputControl& inputControl = game->getInputControl();

		if (!inputControl.shouldLockMouse())
		{
			result = ::Global::GUI().procMouseMsg(msg);
		}

		if (result.isHandled())
			return result;

		inputControl.recvMouseMsg(msg);
	}
	else
	{
		result = ::Global::GUI().procMouseMsg(msg);
	}

	if (result.isHandled())
		return result;

	result = getCurStage()->onMouse(msg);
	InputManager::Get().procMouseEvent(msg);
	return result;
}


MsgReply TinyGameApp::handleKeyEvent(KeyMsg const& msg)
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

				struct Local
				{
					Local()
					{
						NextSystem[(int)ERenderSystem::OpenGL] = ERenderSystem::D3D11;
						NextSystem[(int)ERenderSystem::D3D11] = ERenderSystem::D3D12;
						NextSystem[(int)ERenderSystem::D3D12] = ERenderSystem::OpenGL;
					}
					ERenderSystem NextSystem[(int)RHISystemName::Count];
				};

				static Local SLocal;
				if (GRHISystem)
				{
					ERenderSystem systemName = []
					{
						switch (GRHISystem->getName())
						{
						case RHISystemName::OpenGL: return ERenderSystem::OpenGL;
						case RHISystemName::D3D11: return ERenderSystem::D3D11;
						case RHISystemName::D3D12: return ERenderSystem::D3D12;
						case RHISystemName::Vulkan: return ERenderSystem::Vulkan;
						}
						return ERenderSystem::OpenGL;
					}();
					for (;;)
					{
						systemName = SLocal.NextSystem[int(systemName)];
						if ( ::Global::GetDrawEngine().resetupSystem(systemName) )
							break;
					}
				}
			}
			break;
		}
	}
	MsgReply result = ::Global::GUI().procKeyMsg(msg);
	if( !result.isHandled() )
	{
		result = getCurStage()->onKey(msg);
	}
	return result;
}

MsgReply TinyGameApp::handleCharEvent( unsigned code )
{
	MsgReply result = ::Global::GUI().procCharMsg(code);
	if( !result.isHandled() )
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
#if 0
	if( !::Global::GetDrawEngine().isInitialized() )
		return;

	render(0.0f);
#endif
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
	PROFILE_ENTRY("Render");

	using namespace Render;

	if ( getNextStage() || mbInitializingStage )
		return;

	DrawEngine& drawEngine = Global::GetDrawEngine();

	if ( !drawEngine.beginFrame() )
		return;

	if ( CVarProfileGPU )
		GpuProfiler::Get().beginFrame();

	bool bDrawScene = ( mStageMode == nullptr ) || mStageMode->canRender();

	IGraphics2D& g = ::Global::GetIGraphics2D();
	{
		if (bDrawScene)
		{
			{
				PROFILE_ENTRY("StageRender");
				if (getCurStage())
				{
					getCurStage()->render(dframe);
				}
			}

			g.beginRender();

			{
				long dt = long(dframe * getUpdateTime());
				if (mRenderEffect)
				{
					mRenderEffect->onRender(dt);
				}
			}
			if ( GRHISystem == nullptr || GRHISystem->getName() != RHISystemName::D3D12 )
			{
				PROFILE_ENTRY("GUIRender");
				GPU_PROFILE("GUI");
				::Global::GUI().render();
			}

			g.endRender();
		}
	}

	g.beginRender();

	if( mConsoleShowMode == ConsoleShowMode::Screen )
	{
		gLogPrinter.render(Vec2i(5, 25));
	}

	{

		auto DrawBackgroundRect = [&g](Vec2i rectPos, Vec2i rectSize)
		{
			g.beginBlend(rectPos, rectSize, 0.3f);
			RenderUtility::SetPen(g, EColor::Null);
			RenderUtility::SetBrush(g, EColor::Black);
			g.drawRoundRect(rectPos, rectSize, Vector2(5, 5));
			g.endBlend();
		};

		mFPSCalc.increaseFrame(mClock.getTimeMilliseconds());
		if (CVarShowFPS)
		{
			Vec2i pos = Vec2i(5, 5);
			DrawBackgroundRect(pos - Vec2i(5, 5), Vec2i(60, 12) + Vec2i(10, 10));
			InlineString< 256 > str;
			RenderUtility::SetFont(g, FONT_S8);
			g.setTextColor(Color3ub(255, 255, 0));
			str.format("FPS = %3.1f", mFPSCalc.getFPS());
			g.drawText(Vec2i(5, 5), str);
			//g.drawText(Vec2i(5, 15), str.format("mode = %d", (int)mConsoleShowMode));
		}

		if (CVarDrawGPUUsage)
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

		if (CVarShowProifle)
		{
			::Global::GetDrawEngine().drawProfile(Vec2i(10, 10));
		}

		if (CVarShowGPUProifle)
		{
			{
				PROFILE_ENTRY("ProfileGPU.endFrame");
				if (CVarProfileGPU)
					GpuProfiler::Get().endFrame();
			}

			if (CVarProfileGPU && RHIIsInitialized() && 0)
			{
				SimpleTextLayout textlayout;
				textlayout.offset = 15;
				textlayout.posX = 500;
				textlayout.posY = 10;

				Vec2i rectPos;
				rectPos.x = textlayout.posX - 5;
				rectPos.y = textlayout.posY - 5;
				Vec2i rectSize;
				rectSize.x = 250;
				rectSize.y = GpuProfiler::Get().getSampleNum() * textlayout.offset + 10;
				DrawBackgroundRect(rectPos, rectSize);

				g.setTextColor(Color3ub(255, 0, 0));
				RenderUtility::SetFont(g, FONT_S10);

				InlineString< 512 > str;
				InlineString< 512 > temp;
				int curLevel = 0;
				for (int i = 0; i < GpuProfiler::Get().getSampleNum(); ++i)
				{
					GpuProfileSample* sample = GpuProfiler::Get().getSample(i);

					if (curLevel != sample->level)
					{
						if (sample->level > curLevel)
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
					textlayout.show(g, "%7.4lf %s--> %s", sample->time, temp.c_str(), sample->name.c_str());
				}
			}
		}
	}
	g.endRender();	
	drawEngine.endFrame();
}
///
void TinyGameApp::importUserProfile()
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
	{
		TIME_SCOPE("Localization Initialize");
		ILocalization::Get().initialize((Language)userPorfile.language);
	}

}

void TinyGameApp::exportUserProfile()
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


	IGameRenderSetup* renderSetup = dynamic_cast<IGameRenderSetup*>(stage);
	if (renderSetup)
	{
		if (!::Global::GetDrawEngine().setupSystem(renderSetup, true))
			return false;
	}

	if (gameStage)
	{
		mStageMode = gameStage->getStageMode();
		if (!mStageMode->prevStageInit())
		{
			LogWarning(0, "Can't PrevInit Stage");
			return false;
		}
	}

	if (!stage->onInit())
	{
		LogWarning(0, "Can't Initialize Stage");
		return false;
	}

	if (renderSetup)
	{
		if (!::Global::GetDrawEngine().setupRenderResource())
			return false;

		Render::ShaderManager::Get().cleanupLoadedSource();
	}

	if (gameStage)
	{
		if (!mStageMode->postStageInit())
		{
			LogWarning(0, "Can't PostInit Stage");
			return false;
		}

		setTickTime(gameStage->getTickTime());
	}

	stage->postInit();

	IGameModule* game = ::Global::ModuleManager().getRunningGame();
	if (game)
	{
		game->notifyStageInitialized(stage);
	}

	//::ProfileSystem::Get().resetSample();
	return true;
}

void TinyGameApp::finalizeStage(StageBase* stage)
{
	GameStageBase* gameStage = stage->getGameStage();

	stage->onEnd();

	if (gameStage)
	{
		setTickTime(gDefaultTickTime);
	}

	IGameRenderSetup* renderSetup = dynamic_cast<IGameRenderSetup*>(stage);
	if (renderSetup)
	{
		Global::GetDrawEngine().setupSystem(nullptr);
	}
}

void TinyGameApp::prevStageChange()
{
	DrawEngine& de = ::Global::GetDrawEngine();
	if ( de.beginFrame() )
	{
		Graphics2D& g = ::Global::GetGraphics2D();

		RenderUtility::SetBrush( g , EColor::Black );
		RenderUtility::SetPen( g , EColor::Black );
		g.drawRect( Vec2i(0,0) , ::Global::GetScreenSize() );
		de.endFrame();
	}

	IGameModule* game = ::Global::ModuleManager().getRunningGame();
	if (game)
	{
		game->notifyStagePreExit(getCurStage());
	}
}

void TinyGameApp::postStageChange( StageBase* stage )
{
	addTask( new FadeInEffect( EColor::Black , 1000 ) , this );
	if ( mShowErrorMsg )
	{
		//::Global::getGUI().showMessageBox( UI_ANY , mErrorMsg.c_str() , EMessageButton::Ok );
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
			TIME_SCOPE("Build Server");
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
	bool bShow = window.isShow();
	Vec2i pos = window.getPosition();
	TCHAR const* title = TEXT("Tiny Game");
	window.destroy();
	if( !createWindowInternal(window , width, height , title) )
	{
		return false;
	}

	window.setPosition(pos);
	window.show(bShow);
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
	IGraphics2D& g = Global::GetIGraphics2D();

	g.beginBlend( Vec2i(0,0) , size , float( getLifeTime() - dt ) / totalTime  ); 

	RenderUtility::SetBrush( g , color );
	g.drawRect( Vec2i(0,0) , size );

	g.endBlend();
}
