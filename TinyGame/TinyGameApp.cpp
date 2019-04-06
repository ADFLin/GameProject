#include "TinyGamePCH.h"
#include "TinyGameApp.h"

#include "DrawEngine.h"
#include "GLGraphics2D.h"
#include "RenderUtility.h"

#include "PropertyKey.h"
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


#include "RHI/MeshUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/ShaderCompiler.h"

#define GAME_SETTING_PATH "Game.ini"

int g_DevMsgLevel = -1;

TINY_API IGameNetInterface* gGameNetInterfaceImpl;
TINY_API IDebugInterface*   gDebugInterfaceImpl;

TConsoleVariable< bool > gbShowFPS(false, "ShowFPS");

class GameLogPrinter : public LogOutput
	                 , public IDebugInterface
{
public:

	GameLogPrinter()
	{

	}
	void init()
	{

	}
	static int const MaxLineNum = 20;

	virtual void receiveLog( LogChannel channel , char const* str ) override
	{
		Mutex::Locker locker( mMutex );
		if ( mMsgList.size() > MaxLineNum )
			mMsgList.pop_front();

		mMsgList.push_back( str );
		if ( mMsgList.size() == 1 )
			mTime = 0;
	}

	virtual bool filterLog(LogChannel channel, int level) override
	{
		if( channel == LOG_DEV )
		{
			return g_DevMsgLevel < level;
		}
		return true;
	}

	void update( long time )
	{
		mTime += time;
		if ( mTime > 2000 )
		{
			Mutex::Locker locker( mMutex );
			if ( !mMsgList.empty() )
				mMsgList.pop_front();
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
		for( StringList::iterator iter = mMsgList.begin();
			iter != mMsgList.end() ; ++ iter )
		{
			g.drawText( pos , iter->c_str() );
			pos.y += 14;
		}
	}

	void clearDebugMsg()
	{
		Mutex::Locker locker(mMutex);
		mMsgList.clear();
		mTime = 0;
	}

	//bool     beInited;
	Mutex    mMutex;
	unsigned mTime;
	typedef  std::list< std::string > StringList;
	StringList mMsgList;
};


class GameConfigAsset : public IAssetViewer
{
public:
	virtual void getDependentFilePaths(std::vector< std::wstring >& paths) 
	{
		paths.push_back(FCString::CharToWChar(GAME_SETTING_PATH));
	}
	virtual void postFileModify(FileAction action) 
	{
		if( action == FileAction::Modify )
			Global::GameConfig().loadFile(GAME_SETTING_PATH);
	}

};

static GameLogPrinter gLogPrinter;
static GameConfigAsset gGameConfigAsset;

TinyGameApp::TinyGameApp()
	:mRenderEffect( NULL )
	,mNetWorker( NULL )
	,mStageMode( nullptr )
{
	mShowErrorMsg = false;
	mbLockFPS = false;

	gGameNetInterfaceImpl = this;
	gDebugInterfaceImpl = &gLogPrinter;
}

TinyGameApp::~TinyGameApp()
{

}

bool TinyGameApp::onInit()
{
	ConsoleSystem::Get().initialize();

	::Global::Initialize();


	Render::ShaderManager::Get().setDataCache(&::Global::DataCache());

	GameLoop::setUpdateTime( gDefaultTickTime );

	if ( !Global::GameConfig().loadFile( GAME_SETTING_PATH ) )
	{

	}


	::Global::GetAssetManager().init();

	::Global::GetAssetManager().registerViewer(&gGameConfigAsset);

	exportUserProfile();

	mbLockFPS = ::Global::GameConfig().getIntValue("bLockFPS", nullptr, 0);

	gLogPrinter.addChannel( LOG_DEV );
	gLogPrinter.addChannel( LOG_MSG );
	gLogPrinter.addChannel( LOG_ERROR  );

	if ( !createWindowInternal( mGameWindow, gDefaultScreenWidth , gDefaultScreenHeight, TEXT("Tiny Game") ) )
		return false;

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


	setConsoleShowMode(ConsoleShowMode::Screen);

	bool havePlayGame = false;
	char const* gameName;
	if ( ::Global::GameConfig().tryGetStringValue( "DefaultGame" , nullptr , gameName ) )
	{
		IGameModule* game = ::Global::GameManager().changeGame( gameName );
		if ( game )
		{
			game->beginPlay( SMT_SINGLE_GAME , *this );
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

void TinyGameApp::onEnd()
{
	cleanup();

}

void TinyGameApp::cleanup()
{
	StageManager::cleanup();

	closeNetwork();

	//cleanup widget before delete game instance
	Global::GUI().cleanupWidget(true , true);

	MiscTestRegister::GetList().clear();
	Global::GameManager().cleanup();

	Global::GUI().finalize();

	Global::GetDrawEngine().release();

	Global::GetAssetManager().cleanup();

	importUserProfile();

	Global::GameConfig().saveFile(GAME_SETTING_PATH);

	ILocalization::Get().saveTranslateAsset("LocText.txt");

	Global::Finalize();
}

long TinyGameApp::onUpdate( long shouldTime )
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

		IGameModule* game = Global::GameManager().getRunningGame();
		if ( game )
			game->getController().clearFrameInput();
	}

	gLogPrinter.update( updateTime );
	return updateTime;
}


void TinyGameApp::onIdle(long time)
{
	if ( mbLockFPS )
		SystemPlatform::Sleep(time);
	else
		render( 0.0f );
}


void TinyGameApp::onRender()
{
	render( 0.0f );
}

void TinyGameApp::loadModules()
{
	FileIterator fileIter;
	FixString<MAX_PATH + 1> dir;
	::GetModuleFileName(NULL, dir.data(), dir.max_size());
	
	if ( FileSystem::FindFiles( FileUtility::GetDirectory(dir).toCString() , ".dll" , fileIter ) )
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

			Global::GameManager().loadModule( fileName );
		}
	}
}

void TinyGameApp::closeNetwork()
{
	if ( mNetWorker )
	{
		mNetWorker->closeNetwork();
		delete mNetWorker;
		mNetWorker = NULL;
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
		return NULL;
	}
	mNetWorker = server;

	return server;
}

ClientWorker* TinyGameApp::createClinet()
{
	closeNetwork();

	ClientWorker* worker;
	if ( ::Global::GameConfig().getIntValue("SimNetLog", nullptr, 0) )
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
		return NULL;
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

bool TinyGameApp::onMouse( MouseMsg const& msg )
{
	bool result = true;

	IGameModule* game = Global::GameManager().getRunningGame();
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

bool TinyGameApp::onKey( unsigned key , bool isDown )
{
	if ( isDown )
	{
		if ( key == Keyboard::eF1 )
		{
			mGameWindow.toggleFullscreen();
		}
		else if( key == Keyboard::eOEM3 )
		{
			setConsoleShowMode( ConsoleShowMode(( int(mConsoleShowMode) + 1 ) % int(ConsoleShowMode::Count) ) );
		}
		if( key == Keyboard::eX )
		{
			loadModules();
		}
	}
	bool result = ::Global::GUI().procKeyMsg( key , isDown );
	if( result )
	{
		result = getCurStage()->onKey( key , isDown );
	}
	return result;
}

bool TinyGameApp::onChar( unsigned code )
{
	bool result = ::Global::GUI().procCharMsg(code);
	if( result )
	{
		result = getCurStage()->onChar(code);
	}
	return result;
}

bool TinyGameApp::onDestroy(HWND hWnd )
{
	if( mGameWindow.getHWnd() == hWnd )
	{
		setLoopOver(true);
		return true;
	}	
	return false;
}

void TinyGameApp::onPaint(HDC hDC)
{
	if( !::Global::GetDrawEngine().isInitialized() )
		return;

	render(0.0f);
}

bool TinyGameApp::onActivate( bool beA )
{
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
			mRenderEffect = NULL;
	}
}

void TinyGameApp::render( float dframe )
{
	using namespace Render;

	if ( getNextStage() || mbInitializingStage )
		return;

	DrawEngine& de = Global::GetDrawEngine();

	if ( !de.beginRender() )
		return;

	GpuProfiler::Get().beginFrame();

	bool bDrawScene = ( mStageMode == nullptr ) || mStageMode->canRender();

	if( bDrawScene )
	{
		getCurStage()->render(dframe);

		long dt = long(dframe * getUpdateTime());

		if( mRenderEffect )
			mRenderEffect->onRender(dt);

		if( de.isRHIEnabled() )
			::Global::GetRHIGraphics2D().beginRender();


		{
			GPU_PROFILE("GUI");
			::Global::GUI().render();
		}
	}
	else
	{
		if( de.isRHIEnabled() )
			::Global::GetRHIGraphics2D().beginRender();
	}

	if( mConsoleShowMode == ConsoleShowMode::Screen )
	{
		gLogPrinter.render(Vec2i(5, 25));
	}


	GpuProfiler::Get().endFrame();
	
	mFPSCalc.increaseFrame(getMillionSecond());
	IGraphics2D& g = ::Global::GetIGraphics2D();
	if ( gbShowFPS )
	{
		FixString< 256 > str;
		RenderUtility::SetFont(g, FONT_S8);
		g.setTextColor(Color3ub(255, 255, 0));
		g.drawText(Vec2i(5, 5), str.format("FPS = %3.1f", mFPSCalc.getFPS()));
		//g.drawText(Vec2i(5, 15), str.format("mode = %d", (int)mConsoleShowMode));
	}

	{

		g.setTextColor(Color3ub(255, 0, 0));
		RenderUtility::SetFont(g, FONT_S10);

		SimpleTextLayout textlayout;
		textlayout.offset = 15;
		textlayout.posX = 500;
		textlayout.posY = 25;
		FixString< 512 > str;
		FixString< 512 > temp;
		int curLevel = 0;
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

	if( de.isRHIEnabled() )
		::Global::GetRHIGraphics2D().endRender();
		
	de.endRender();
}

void TinyGameApp::exportUserProfile()
{
	PropertyKey& setting = Global::GameConfig();
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
	PropertyKey& setting = Global::GameConfig();

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
	StageBase* newStage = NULL;

	IGameModule* curGame = Global::GameManager().getRunningGame();

	if ( curGame )
	{
		newStage = curGame->createStage( stageId );

		if( GameStageBase* gameStage = dynamic_cast<GameStageBase*>(newStage) )
		{
			GameStageMode* stageMode = createGameStageMode(stageId);
			gameStage->setupStageMode(stageMode);
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
			return NULL;
		}
	}

	return newStage;
}


GameStageMode* TinyGameApp::createGameStageMode(StageID stageId)
{
	GameStageMode* stageMode = NULL;

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
		if( ::Global::GetDrawEngine().isOpenGLEnabled() )
		{
			::Global::GetDrawEngine().stopOpenGL();
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

	if( auto gameStage = dynamic_cast<GameStageBase*>(stage) )
	{
		mStageMode = gameStage->getStageMode();
		if( !mStageMode->prevStageInit() )
			return false;

		if( !stage->onInit() )
			return false;

		if( !mStageMode->postStageInit() )
			return false;
	}
	else
	{
		if( !stage->onInit() )
			return false;
	}

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
		g.drawRect( Vec2i(0,0) , ::Global::GetDrawEngine().getScreenSize() );
		de.endRender();
	}
}

void TinyGameApp::postStageEnd()
{

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

	return;
}

GameWindow& TinyGameApp::getMainWindow()
{
	return mGameWindow;
}

bool TinyGameApp::reconstructWindow( GameWindow& window )
{
	int width = window.getWidth();
	int height = window.getHeight();
	TCHAR const* title = TEXT("Tiny Game");
	window.destroy();
	if( !createWindowInternal(window , width, height , title) )
	{
		return false;
	}

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
