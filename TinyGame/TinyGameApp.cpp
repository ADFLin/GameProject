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
#include "GameGlobal.h"

#include "GameServer.h"
#include "GameClient.h"

#include "GameWidget.h"
#include "GameWidgetID.h"

#include "Localization.h"

#include "MainMenuStage.h"
#include "GameSingleStage.h"
#include "NetGameStage.h"
#include "ReplayStage.h"

#include "GLUtility.h"

#define GAME_SETTING_PATH "Game.ini"

int g_DevMsgLevel = 10;

class GMsgListener : public IMsgListener
{
public:

	GMsgListener()
	{

	}
	void init()
	{

	}

	virtual void receive( MsgChannel channel , char const* str )
	{
		Mutex::Locker locker( mMutex );
		if ( mMsgList.size() > 15 )
			mMsgList.pop_front();

		mMsgList.push_back( str );
		if ( mMsgList.size() == 1 )
			mTime = 0;
	}

	void update( long time )
	{
		mTime += time;
		if ( mTime > 10000 )
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
{
	mShowErrorMsg = false;
}

TinyGameApp::~TinyGameApp()
{

}

bool TinyGameApp::onInit()
{
	GameLoop::setUpdateTime( gDefaultTickTime );

	if ( !Global::getSetting().loadFile( GAME_SETTING_PATH ) )
	{

	}

	exportUserProfile();

	gMsgListener.setLevel( g_DevMsgLevel );
	gMsgListener.addChannel( MSG_DEV );
	gMsgListener.addChannel( MSG_NORMAL );
	gMsgListener.addChannel( MSG_ERROR  );

	if ( !mGameWindow.create( TEXT("Tiny Game") , gDefaultScreenWidth , gDefaultScreenHeight , SysMsgHandler::MsgProc  ) )
		return false;

	::Global::getDrawEngine()->init( mGameWindow );

	::Global::getGUI().init( this );

	loadGamePackage();

	setupStage();
	changeStage( STAGE_MAIN_MENU );

	mFPSCalc.init( getMillionSecond() );

	return true;
}

void TinyGameApp::onEnd()
{
	StageManager::cleanup();

	closeNetwork();

	Global::getGameManager().cleanup();

	importUserProfile();
	Global::getSetting().saveFile( GAME_SETTING_PATH );

	extern void saveTranslateAsset( char const* path );
	saveTranslateAsset( "tt.txt" );

	Global::getDrawEngine()->release();
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

		::Global::getGUI().update();

		IGamePackage* game = Global::getGameManager().getCurGame();
		if ( game )
			game->getController().clearFrameInput();
	}

	gMsgListener.update( updateTime );
	return updateTime;
}


void TinyGameApp::onIdle(long time)
{
	//render( 0.0f );
	::Sleep( time );
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

			Global::getGameManager().loadGame( fileName );
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
		::Global::getGUI().showMessageBox( 
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
#if 0
	DelayClientWorker* worker = new DelayClientWorker( mUserProfile );
	worker->setDelay( 50 );
#else
	ClientWorker* worker = new ClientWorker( getUserProfile() );
#endif
	if ( !worker->startNetwork() )
	{
		delete worker;
		::Global::getGUI().showMessageBox( 
			UI_ANY , LAN("Can't Create Client") );
		return NULL;
	}
	mNetWorker = worker;

	return worker;
}

bool TinyGameApp::onWidgetEvent( int event , int id , GWidget* ui )
{

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
			::Global::getGUI().showMessageBox( 
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
			::Global::getGUI().showMessageBox( 
				UI_MAIN_MENU , LAN("Be Sure Back To Main Menu?") );
		}

		break;
	case UI_BUILD_CLIENT:
		{
			closeNetwork();

			ClientWorker* worker = createClinet();
			if ( !worker )
				return false;
			NetRoomStage* stage = static_cast< NetRoomStage* >( changeStage( STAGE_NET_ROOM ) );
			stage->initWorker( worker );
		}
		break;
	case UI_CREATE_SERVER:
		{
			closeNetwork();

			ServerWorker* server = createServer();
			if ( !server )
				return false;

			LocalWorker* worker = server->createLocalWorker( getUserProfile() );

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

	return true;
}



bool TinyGameApp::onMouse( MouseMsg const& msg )
{
	bool result = true;

	IGamePackage* game = Global::getGameManager().getCurGame();
	if ( game )
	{
		GameController& controller = game->getController();

		if ( !controller.haveLockMouse() )
		{
			result = ::Global::getGUI().getManager().procMouseMsg( msg );
		}

		if ( result )
			controller.recvMouseMsg( msg );
	}
	else
	{
		result = ::Global::getGUI().getManager().procMouseMsg( msg );
	}

	if ( result )
	{
		getCurStage()->onMouse( msg );
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
	return getCurStage()->onKey( key , isDown );
}

bool TinyGameApp::onChar( unsigned code )
{
	return getCurStage()->onChar( code );
}

void TinyGameApp::onDestroy()
{
	Global::getDrawEngine()->release();
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

	getCurStage()->render( dframe );

	long dt = long( dframe * getUpdateTime() );


	if ( mRenderEffect )
		mRenderEffect->onRender( dt );

	if ( ::Global::getDrawEngine()->isEnableOpenGL() )
		::Global::getGLGraphics2D().beginRender();

	Global::getGUI().render();
	gMsgListener.render( Vec2i( 5 , 25 ) );

	mFPSCalc.increaseFrame( getMillionSecond() );
	IGraphics2D& g = ::Global::getIGraphics2D();
	FixString< 256 > str;
	g.setTextColor( 255 , 255 , 0 );
	g.drawText( Vec2i(5,5) , str.format( "FPS = %f" , mFPSCalc.getFPS() ) );

	if ( ::Global::getDrawEngine()->isEnableOpenGL() )
		::Global::getGLGraphics2D().endRender();

	de->endRender();
}

void TinyGameApp::exportUserProfile()
{
	PropertyKey& setting = Global::getSetting();
	UserProfile& userPorfile = getUserProfile();

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
	PropertyKey& setting = Global::getSetting();

	UserProfile& userPorfile = getUserProfile();

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
	StageBase* chStage = NULL;

	IGamePackage* curGame = Global::getGameManager().getCurGame();

	if ( curGame )
	{
		GameSubStage* subStage = curGame->createSubStage( stageId );
		if ( subStage )
		{
			GameStage* gameStage = createGameStage( stageId );

			if ( gameStage )
			{
				gameStage->setSubStage( subStage );
			}
			else
			{
				delete subStage;
				ErrorMsg( "Can't Find GameStage" );
			}
			chStage = gameStage;
		}

		if ( !chStage )
			chStage = curGame->createStage( stageId );
	}


	if ( !chStage )
	{
		switch ( stageId )
		{

#define CASE_STAGE( id , Class )\
		case id : chStage = new Class ; break;

		CASE_STAGE( STAGE_MAIN_MENU   , MainMenuStage )
		CASE_STAGE( STAGE_NET_ROOM    , NetRoomStage )
		CASE_STAGE( STAGE_REPLAY_EDIT , ReplayEditStage )
#undef CASE_STAGE
		default:
			ErrorMsg( "Can't find Stage %d " , stageId );
			return NULL;
		}
	}

	return chStage;
}



GameStage* TinyGameApp::createGameStage( StageID stageId )
{
	GameStage* gameStage = NULL;

	switch( stageId )
	{
#define CASE_STAGE( idx , Class )\
			case idx : gameStage = new Class; break;
		CASE_STAGE( STAGE_SINGLE_GAME , GameSingleStage )
		CASE_STAGE( STAGE_NET_GAME    , GameNetLevelStage )
		CASE_STAGE( STAGE_REPLAY_GAME , GameReplayStage )
#undef CASE_STAGE
	}
	return gameStage;
}

StageBase* TinyGameApp::onStageChangeFail( FailReason reason )
{
	switch( reason )
	{
	case FR_INIT_FAIL:
		//if ( !mErrorMsg.empty() )
		//	mShowErrorMsg = true;
		break;
	}
	return createStage( STAGE_MAIN_MENU );
}

void TinyGameApp::prevChangeStage()
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

NetWorker* TinyGameApp::buildNetwork( bool beServer )
{
	if ( beServer )
		return createServer();
	return createClinet();
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
