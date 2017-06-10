#include "TinyGamePCH.h"
#include "TinyGameApp.h"

#include "DrawEngine.h"
#include "GLGraphics2D.h"
#include "RenderUtility.h"

#include "PropertyKey.h"
#include "InputManager.h"
#include "FileSystem.h"

#include "GameGUISystem.h"
#include "GameStage.h"
#include "GameStageMode.h"
#include "GameGlobal.h"

#include "GameServer.h"
#include "GameClient.h"

#include "GameWidget.h"
#include "GameWidgetID.h"

#include "Localization.h"

#include "Stage/MainMenuStage.h"

#include "NetGameStage.h"

#include "SingleStageMode.h"
#include "ReplayStageMode.h"

#include "Thread.h"
#include "RenderGL/GLUtility.h"

#define GAME_SETTING_PATH "Game.ini"

int g_DevMsgLevel = 10;

GAME_API IGameNetInterface* gGameNetInterfaceImpl;
GAME_API uint32 gGameThreadId;

class GMsgListener : public ILogListener
{
public:

	GMsgListener()
	{

	}
	void init()
	{

	}
	static int const MaxLineNum = 60;

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

	void render( Vec2i const& a_pos )
	{
		IGraphics2D& g = Global::getIGraphics2D();

		Vec2i pos = a_pos;
		RenderUtility::setFont( g , FONT_S8 );
		g.setTextColor(255 , 255 , 0 );
		Mutex::Locker locker( mMutex );
		for( StringList::iterator iter = mMsgList.begin();
			iter != mMsgList.end() ; ++ iter )
		{
			g.drawText( pos , iter->c_str() );
			pos.y += 14;
		}
	}

	//bool     beInited;
	Mutex    mMutex;
	unsigned mTime;
	typedef  std::list< std::string > StringList;
	StringList mMsgList;



};

static GMsgListener gMsgListener;


TinyGameApp::TinyGameApp()
	:mRenderEffect( NULL )
	,mNetWorker( NULL )
	,mStageMode( nullptr )
{
	mShowErrorMsg = false;
	gGameNetInterfaceImpl = this;
	mbLockFPS = false;
}

TinyGameApp::~TinyGameApp()
{

}

bool TinyGameApp::onInit()
{
	gGameThreadId = PlatformThread::GetCurrentThreadId();

	GameLoop::setUpdateTime( gDefaultTickTime );

	if ( !Global::GameSetting().loadFile( GAME_SETTING_PATH ) )
	{

	}

	exportUserProfile();

	mbLockFPS = ::Global::GameSetting().getIntValue("bLockFPS", nullptr, 0);

	gMsgListener.addChannel( LOG_DEV );
	gMsgListener.addChannel( LOG_MSG );
	gMsgListener.addChannel( LOG_ERROR  );

	if ( !mGameWindow.create( TEXT("Tiny Game") , gDefaultScreenWidth , gDefaultScreenHeight , SysMsgHandler::MsgProc  ) )
		return false;

	::Global::getDrawEngine()->init( mGameWindow );

	::Global::GUI().init( *this );

	loadGamePackage();

	setupStage();


	bool havePlayGame = false;
	char const* gameName;
	if ( ::Global::GameSetting().tryGetStringValue( "DefaultGame" , nullptr , gameName ) )
	{
		IGameInstance* game = ::Global::GameManager().changeGame( gameName );
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
	Global::GUI().cleanupWidget(true);

	Global::GameManager().cleanup();

	Global::getDrawEngine()->release();

	importUserProfile();

	Global::GameSetting().saveFile(GAME_SETTING_PATH);

	extern void saveTranslateAsset(char const* path);
	saveTranslateAsset("tt.txt");
}

long TinyGameApp::onUpdate( long shouldTime )
{
	int  numFrame = shouldTime / getUpdateTime();
	long updateTime = numFrame * getUpdateTime();

	for( int i = 0 ; i < numFrame ; ++i )
	{
		if ( mNetWorker )
			mNetWorker->update( getUpdateTime() );

		checkNewStage();
		runTask( getUpdateTime() );

		getCurStage()->update( getUpdateTime() );

		::Global::GUI().update();

		IGameInstance* game = Global::GameManager().getRunningGame();
		if ( game )
			game->getController().clearFrameInput();
	}

	gMsgListener.update( updateTime );
	return updateTime;
}


void TinyGameApp::onIdle(long time)
{
	if ( mbLockFPS )
		::Sleep(time);
	else
		render( 0.0f );
}


void TinyGameApp::onRender()
{
	render( 0.0f );
}

void TinyGameApp::loadGamePackage()
{
	FileIterator fileIter;
	if ( FileSystem::findFile( "" , ".dll" , fileIter ) )
	{
		for ( ; fileIter.haveMore() ; fileIter.goNext() )
		{
			char const* fileName = fileIter.getFileName();
			if ( strncmp( fileName , "Game" , 4 ) != 0 )
				continue;
#if _DEBUG
			if ( fileName[ strlen( fileName ) - 5 ] != 'D' )
				continue;
#else 
			if ( fileName[ strlen( fileName ) - 5 ] == 'D' )
				continue;
#endif

			Global::GameManager().loadGame( fileName );
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
			UI_ANY , LAN("Can't Create Server") );
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
	if ( ::Global::GameSetting().getIntValue("SimNetLog", nullptr, 0) )
	{
		DelayClientWorker* delayWorker = new DelayClientWorker();
		delayWorker->setDelay(
			::Global::GameSetting().getIntValue("SimNetLagDelay", nullptr, 30),
			::Global::GameSetting().getIntValue("SimNetLagDelayRand", nullptr, 0));
		worker = delayWorker;
	}
	else
	{
		worker = new ClientWorker();
	}

	if ( !worker->startNetwork() )
	{
		delete worker;
		::Global::GUI().showMessageBox( UI_ANY , LAN("Can't Create Client") );
		return NULL;
	}

	mNetWorker = worker;
	return worker;
}

bool TinyGameApp::onMouse( MouseMsg const& msg )
{
	bool result = true;

	IGameInstance* game = Global::GameManager().getRunningGame();
	if ( game )
	{
		GameController& controller = game->getController();

		if ( !controller.haveLockMouse() )
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
		InputManager::getInstance().procMouseEvent( msg );
	}
	return result;
}

bool TinyGameApp::onKey( unsigned key , bool isDown )
{
	if ( isDown )
	{
		if ( key == VK_F1 )
		{
			mGameWindow.toggleFullscreen();
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

void TinyGameApp::onDestroy()
{
	setLoopOver(true);
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
	if ( getNextStage() )
		return;

	DrawEngine* de = Global::getDrawEngine();

	if ( !de->beginRender() )
		return;

	bool bDrawScene = ( mStageMode == nullptr ) || mStageMode->canRender();

	if( bDrawScene )
	{
		getCurStage()->render(dframe);

		long dt = long(dframe * getUpdateTime());

		if( mRenderEffect )
			mRenderEffect->onRender(dt);

		if( de->isOpenGLEnabled() )
			::Global::getGLGraphics2D().beginRender();

		Global::GUI().render();
	}
	else
	{
		if( de->isOpenGLEnabled() )
			::Global::getGLGraphics2D().beginRender();
	}

	gMsgListener.render(Vec2i(5, 25));

	mFPSCalc.increaseFrame(getMillionSecond());
	IGraphics2D& g = ::Global::getIGraphics2D();
	FixString< 256 > str;
	g.setTextColor(255, 255, 0);
	g.drawText(Vec2i(5, 5), str.format("FPS = %f", mFPSCalc.getFPS()));

	if( de->isOpenGLEnabled() )
		::Global::getGLGraphics2D().endRender();
		
	de->endRender();
}

void TinyGameApp::exportUserProfile()
{
	PropertyKey& setting = Global::GameSetting();
	UserProfile& userPorfile = Global::getUserProfile();

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

	initLanguage( ( Language )userPorfile.language );
}

void TinyGameApp::importUserProfile()
{
	PropertyKey& setting = Global::GameSetting();

	UserProfile& userPorfile = Global::getUserProfile();

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

	IGameInstance* curGame = Global::GameManager().getRunningGame();

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
			ErrorMsg( "Can't find Stage %d " , stageId );
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
	switch( reason )
	{
	case FailReason::InitFail:
	case FailReason::NoStage:
		//if ( !mErrorMsg.empty() )
		//	mShowErrorMsg = true;
		break;
	}
	return createStage( STAGE_MAIN_MENU );
}

bool TinyGameApp::initializeStage(StageBase* stage)
{
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
	DrawEngine* de = ::Global::getDrawEngine();
	Graphics2D& g = ::Global::getGraphics2D();
	if ( de->beginRender() )
	{
		RenderUtility::setBrush( g , Color::eBlack );
		RenderUtility::setPen( g , Color::eBlack );
		g.drawRect( Vec2i(0,0) , ::Global::getDrawEngine()->getScreenSize() );
		de->endRender();
	}
}

void TinyGameApp::postStageEnd()
{
	::Global::getDrawEngine()->cleanupGLContextDeferred();
}

void TinyGameApp::postStageChange( StageBase* stage )
{
	addTask( new FadeInEffect( Color::eBlack , 1000 ) , this );
	if ( mShowErrorMsg )
	{
		//::Global::getGUI().showMessageBox( UI_ANY , mErrorMsg.c_str() , GMB_OK );
		mShowErrorMsg = false;
	}
}

bool TinyGameApp::onActivate( bool beA )
{
	return true;
}

void TinyGameApp::onPaint(HDC hDC)
{
	if( !::Global::getDrawEngine()->isInitialized() )
		return;

	render(0.0f);
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
				UI_EXIT_GAME , LAN("Be Sure To Exit The Game?") );
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
				UI_MAIN_MENU , LAN("Be Sure Back To Main Menu?") );
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

			LocalWorker* worker = server->createLocalWorker(::Global::getUserProfile().name );

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

FadeInEffect::FadeInEffect( int _color , long time )
	:RenderEffect( time )
{
	color = _color;
	totalTime = time;
}

void FadeInEffect::onRender( long dt )
{
	DrawEngine* de = Global::getDrawEngine();

	Vec2i size = de->getScreenSize() + Vec2i(5,5); 
	Graphics2D& g = Global::getGraphics2D();

	g.beginBlend( Vec2i(0,0) , size , float( getLifeTime() - dt ) / totalTime  ); 

	RenderUtility::setBrush( g , color );
	g.drawRect( Vec2i(0,0) , size );

	g.endBlend();
}
