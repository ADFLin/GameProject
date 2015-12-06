#include "TinyGamePCH.h"
#include "NetGameStage.h"

#include "GameWorker.h"
#include "GameClient.h"
#include "GamePackageManager.h"

#include "CSyncFrameManager.h"
#include "RenderUtility.h"
#include "PropertyKey.h"

#include "GameGUISystem.h"
#include "GameWidgetID.h"
#include "GameSettingPanel.h"
#include "ServerListPanel.h"

#include "GameNetPacket.h"
#include "GameAction.h"


#include "GameSettingHelper.h"

#include "EasingFun.h"
#include <cmath>

class EmptyNetEngine : public INetEngine
{
public:
	EmptyNetEngine()
	{
	}
	virtual bool build( BuildInfo& info )
	{
		return true;
	}
	virtual void update( IFrameUpdater& updater , long time )
	{

	}
	virtual void setupInputAI( IPlayerManager& manager )
	{

	}
	virtual void release()
	{
	}
};

class LocalNetEngine : public INetEngine
{
public:
	LocalNetEngine()
	{
		mTickTime = gDefaultTickTime;
	}
	virtual bool build( BuildInfo& info )
	{
		mTickTime = info.tickTime;
		return true;
	}
	virtual void update( IFrameUpdater& updater , long time )
	{
		int numFrame = time / mTickTime;
		for( int i = 0 ; i < numFrame ; ++i )
			updater.tick();
		updater.updateFrame( numFrame );
	}
	virtual void setupInputAI( IPlayerManager& manager )
	{

	}
	virtual void release()
	{
	}

	long mTickTime;
};

static LocalNetEngine gLocalNetEngine;
static EmptyNetEngine gEmptyNetEngine;

NetRoomStage::NetRoomStage()
{
	mConnectPanel     = NULL;
}

NetRoomStage::~NetRoomStage()
{

}

bool NetRoomStage::onInit()
{
	if ( !BaseClass::onInit() )
		return false;

	getManager()->setTickTime( gDefaultTickTime );

	mLastSendSetting = 0;
	

	::Global::getGUI().cleanupWidget();
	
	if ( haveServer() )
	{
		mServer->getPlayerManager()->setListener( this );

		if ( !setupUI() )
			return false;

		for( IPlayerManager::Iterator iter = mServer->getPlayerManager()->getIterator();
			iter.haveMore() ; iter.goNext() )
		{
			GamePlayer* player = iter.getElement();
			mPlayerPanel->addPlayer( player->getInfo() );
		}

		IGamePackage* curGame = Global::getGameManager().getCurGame();
		mSettingPanel->setGame( curGame ? curGame->getName() : NULL );

		mHelper->sendSlotStateSV();
	}
	else
	{
		ClientWorker* worker = static_cast< ClientWorker*>( mWorker );

		if ( worker->getActionState() == NAS_DISSCONNECT )
		{
			mConnectPanel  = new ServerListPanel( worker , Vec2i( 0 , 0 ) , NULL );
			mConnectPanel->setPos( ::Global::getGUI().calcScreenCenterPos( mConnectPanel ->getSize() ) );

			addTask( TaskUtility::createMemberFunTask( this , &NetRoomStage::taskDestroyServerListPanelCL ) );
			::Global::getGUI().addWidget( mConnectPanel  );

			mConnectPanel->refreshServerList();
			mConnectPanel->doModal();
		}
	}

	mWorker->changeState( NAS_ROOM_ENTER );
	return true;
}



void NetRoomStage::onEnd()
{
	if ( haveServer() )
	{
		mServer->getPlayerManager()->setListener( NULL );
	}
	BaseClass::onEnd();
}

bool NetRoomStage::setupUI()
{

	GamePackageVec gameVec;
	Global::getGameManager().classifyGame( ATTR_NET_SUPPORT , gameVec );
	if ( gameVec.empty() )
	{
		return false;
	}

	typedef Easing::OQuart EasingFun;

	long const timeUIAnim = 1000;

	Vec2i panelPos = Vec2i( 20 , 20 );
	{
		mPlayerPanel = new PlayerListPanel( 
			mWorker->getPlayerManager() ,UI_PLAYER_LIST_PANEL , panelPos , NULL );
		mPlayerPanel->init( haveServer() );
		::Global::getGUI().addWidget( mPlayerPanel );

		Vec2i from = Vec2i( -mPlayerPanel->getSize().x , panelPos.y );
		mPlayerPanel->setPos( Vec2i( -mPlayerPanel->getSize().x , panelPos.y ) );
		::Global::getGUI().addMotion< EasingFun >( mPlayerPanel , from , panelPos , timeUIAnim );
	}


	Vec2i sizeSettingPanel( 350 , 400 );
	{
		Vec2i pos = panelPos + Vec2i( PlayerListPanel::WidgetSize.x + 10 , 0 ); 
		GameSettingPanel* panel = new GameSettingPanel( 
			UI_GAME_SETTING_PANEL , pos , sizeSettingPanel , NULL );

		if ( !haveServer() )
		{
			panel->enable( false );
		}


		::Global::getGUI().addWidget( panel );
		mSettingPanel = panel;
		for( GamePackageVec::iterator iter = gameVec.begin();
			iter != gameVec.end() ; ++iter )
		{
			panel->addGame( *iter );
		}

		Vec2i from = Vec2i( Global::getDrawEngine()->getScreenWidth() , pos.y );
		::Global::getGUI().addMotion< EasingFun >( panel , from , pos , timeUIAnim );
	}

	{
		Vec2i pos = panelPos + Vec2i( 0 , PlayerListPanel::WidgetSize.y + 20 );
		ComMsgPanel* panel = new ComMsgPanel( UI_ANY , pos , Vec2i( PlayerListPanel::WidgetSize.x , 230 ) , NULL );
		panel->setWorker( mWorker );
		::Global::getGUI().addWidget( panel );
		mMsgPanel = panel;

		Vec2i from = Vec2i( -panel->getSize().x , pos.y );
		::Global::getGUI().addMotion< EasingFun >( panel , from , pos , timeUIAnim );
	}


	Vec2i btnSize( sizeSettingPanel.x , 30 );
	Vec2i btnPos( Vec2i( panelPos.x + PlayerListPanel::WidgetSize.x + 10 , panelPos.y + sizeSettingPanel.y + 10 )  );

	GButton* button;
	if ( haveServer() )
	{	
		button = new GButton( UI_NET_ROOM_START , btnPos , btnSize , NULL );
		button->setTitle( LAN("Start Game") );
		//button->enable( false );
	}
	else
	{
		button = new GButton( UI_NET_ROOM_READY , btnPos , btnSize , NULL );
		button->setTitle( LAN("Ready" ) );
	}

	{
		Vec2i from =  Vec2i( Global::getDrawEngine()->getScreenWidth() , btnPos.y );
		::Global::getGUI().addMotion< EasingFun >( button , from , btnPos , timeUIAnim );
	}

	button->setFontType( FONT_S10 );
	mReadyButton = button;
	::Global::getGUI().addWidget( button );

	btnPos += Vec2i( 0 , btnSize.y + 5 );
	button = new GButton( UI_MAIN_MENU , btnPos , btnSize , NULL );
	button->setFontType( FONT_S10 );
	button->setTitle( LAN("Exit") );
	::Global::getGUI().addWidget( button );
	mExitButton = button;
	{
		Vec2i from =  Vec2i( Global::getDrawEngine()->getScreenWidth() , btnPos.y );
		::Global::getGUI().addMotion< EasingFun >( button , from , btnPos , timeUIAnim );
	}

	return true;
}

void NetRoomStage::setupServerProcFun( ComEvaluator& evaluator )
{

#define DEFINE_CP_USER_FUN( Class , Func )\
	evaluator.setUserFun< Class >( this , &NetRoomStage::Func );

	DEFINE_CP_USER_FUN( CSPPlayerState , procPlayerStateSv )
#undef  DEFINE_CP_USER_FUN

}

void NetRoomStage::setupWorkerProcFun( ComEvaluator& evaluator )
{

#define DEFINE_CP_USER_FUN( Class , Func )\
	evaluator.setUserFun< Class >( this , &NetRoomStage::Func );

	DEFINE_CP_USER_FUN( CSPMsg         , procMsg )
	DEFINE_CP_USER_FUN( CSPPlayerState , procPlayerState )
	DEFINE_CP_USER_FUN( CSPRawData     , procRawData )
	DEFINE_CP_USER_FUN( SPPlayerStatus , procPlayerStatus )
	DEFINE_CP_USER_FUN( SPSlotState    , procSlotState )
#undef  DEFINE_CP_USER_FUN

}



void NetRoomStage::onRender( float dFrame )
{
	//Graphics2D& g = de->getGraphics2D();
	//Vec2i basePos = de->getScreenSize();
	//basePos /= 2;
	//g.setBrush( ColorKey3( 255 , 255 , 0 ) );
	//g.drawRect( basePos + 20 * mPos , Vec2i( 20 , 20 ) );

}

bool NetRoomStage::onWidgetEvent( int event , int id , GWidget* ui )
{
	switch( id )
	{
	case UI_GAME_CHOICE:
		setupGame( static_cast< GChoice* >( ui )->getSelectValue() );
		if ( haveServer() )
			sendGameSetting();
		return false;
	case UI_PLAYER_LIST_PANEL:
		switch( event )
		{
		case EVT_SLOT_CHANGE:
			if ( haveServer() )
			{
				mHelper->sendSlotStateSV();
			}
			return false;
		case EVT_PLAYER_TYPE_CHANGE:
			if ( haveServer() )
			{
				mHelper->sendPlayerStatusSV();
			}
			return false;
		}
		break;
	case UI_PLAYER_SLOT:

		switch( event )
		{
		case EVT_CHOICE_SELECT:
			if ( haveServer() )
			{
				GChoice* choice = GUI::castFast< GChoice*>( ui );
				PlayerListPanel::Slot* slot = (PlayerListPanel::Slot*) ui->getUserData();

				switch ( choice->getSelection() )
				{
				case PlayerListPanel::OpenSelection:
					mHelper->emptySlotSV( slot->id , SLOT_OPEN );
					break;
				case PlayerListPanel::CloseSelection:
					mHelper->emptySlotSV( slot->id , SLOT_CLOSE );
					break;
				}
			}
			return false;
		}
		break;
	case UI_NET_ROOM_START: // Server
		if ( mHelper->checkSettingVaildSV() )
		{
			assert( haveServer() );

			CSPPlayerState com;
			com.playerID = mServer->getPlayerManager()->getUserID();
			com.state    = NAS_ROOM_READY;
			mServer->sendTcpCommand( &com );

			mHelper->sendPlayerStatusSV();
			sendGameSetting();

			GameStartTask* task = new GameStartTask;
			task->server = mServer;
			addTask( task );
		}
		return false;

	case UI_NET_ROOM_READY: //Client
		{
			GButton* button = GUI::castFast< GButton* >( ui );
			assert( button == mReadyButton );
			if ( mWorker->getActionState() != NAS_ROOM_READY )
			{
				mWorker->changeState( NAS_ROOM_READY );
				button->setTitle( LAN("Cancel Ready") );
			}
			else
			{
				mWorker->changeState( NAS_ROOM_WAIT );
				button->setTitle( LAN("Ready") );
			}	
		}
		return false;
	case UI_MAIN_MENU:
		disconnect();
		getManager()->changeStage( STAGE_MAIN_MENU );
		return false;
	case UI_DISCONNECT_MSGBOX:			
		getManager()->changeStage( STAGE_MAIN_MENU );
		return false;
	}

	return BaseClass::onWidgetEvent( event , id , ui );
}

void NetRoomStage::onServerEvent( EventID event , unsigned msg )
{
	if ( mConnectPanel )
	{
		mConnectPanel->onServerEvent( event , msg );
	}

	switch( event )
	{
	case eCON_CLOSE:
		getManager()->changeStage( STAGE_MAIN_MENU );
		break;
	case eCON_RESULT:
		if ( msg )
			setupUI();
		break;
	}
	
}

void NetRoomStage::onUpdate( long time )
{
	BaseClass::onUpdate( time );
	::Global::getGUI().updateFrame( time / gDefaultTickTime , gDefaultTickTime );
}

void NetRoomStage::procPlayerState( IComPacket* cp )
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();

	switch( com->state )
	{
	case NAS_ROOM_ENTER:
	case NAS_ACCPET:
		mWorker->changeState( NAS_ROOM_WAIT );
		break;
	case NAS_LEVEL_SETUP:
		{
			mReadyButton->enable( false );
			mExitButton->enable( false );

			assert( Global::getGameManager().getCurGame()  );

			StageBase* nextStage = NULL;
			GameSubStage* subStage = Global::getGameManager().getCurGame()->createSubStage( STAGE_NET_GAME );
			if ( subStage )
			{
				GameNetLevelStage* stage = new GameNetLevelStage;
				stage->setSubStage( subStage );
				stage->initWorker( mWorker , mServer );

				nextStage = stage;
			}
			else
			{
				nextStage = getManager()->createStage( STAGE_NET_GAME );
			}

			if ( nextStage )
			{
				mHelper->setupGame( *getManager() , subStage );

				if ( haveServer() )
				{
					mHelper->sendPlayerStatusSV();
				}
			}
			else
			{
				Msg( "No NetLevel!" );
			}

			getManager()->setNextStage( nextStage );
			mWorker->changeState( NAS_LEVEL_SETUP );
		}
		break;
	case NAS_ROOM_READY:
		{
			GamePlayer* player = mWorker->getPlayerManager()->getPlayer( com->playerID );
			PlayerListPanel::Slot& slot = mPlayerPanel->getSlot( player->getInfo().slot );
			slot.choice->setColor( Color::eGreen );
		}
		break;
	case NAS_ROOM_WAIT:
		{
			GamePlayer* player = mWorker->getPlayerManager()->getPlayer( com->playerID );
			PlayerListPanel::Slot& slot = mPlayerPanel->getSlot( player->getInfo().slot );
			slot.choice->setColor( Color::eBlue );
		}
		break;
	case NAS_CONNECT:
		if ( haveServer() )
		{
			CSPRawData settingCom;
			generateSetting( settingCom );
			mServer->getPlayerManager()->getPlayer( com->playerID )->sendTcpCommand( &settingCom );
		}

		{
			GamePlayer* player = mWorker->getPlayerManager()->getPlayer( com->playerID );
			FixString< 64 > str;
			str.format( LAN("== %s Inter Room =="), player->getName() );
			mMsgPanel->addMessage( str , RGB( 255 , 125 , 0 ) );
		}
		break;
	case NAS_DISSCONNECT:
		if ( com->playerID == ERROR_PLAYER_ID )
		{
			::Global::getGUI().showMessageBox( 
				UI_DISCONNECT_MSGBOX , 
				LAN("Server shout down") , GMB_OK );
		}
		else if ( mWorker->getPlayerManager()->getUserID() == com->playerID )
		{
			::Global::getGUI().showMessageBox( 
				UI_DISCONNECT_MSGBOX , 
				LAN("You are kicked by Server") , GMB_OK );
		}
		else
		{
			GamePlayer* player = mWorker->getPlayerManager()->getPlayer( com->playerID );
			if ( player )
			{
				PlayerListPanel::Slot& slot = mPlayerPanel->getSlot( player->getInfo().slot );
				slot.choice->setColor( Color::eBlue );

				FixString< 64 > str;
				str.format( LAN("== %s Leave Room =="), player->getName() );
				mMsgPanel->addMessage( str , RGB( 255 , 125 , 0 ) );
			}
		}
		break;
	}

}

void NetRoomStage::procPlayerStateSv( IComPacket* cp )
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();
	SVPlayerManager* playerMgr = mServer->getPlayerManager();

	switch( com->state )
	{
	case NAS_ROOM_ENTER:
		{
			sendGameSetting( com->playerID );
		}
		break;
	case NAS_ROOM_WAIT:
		{
			mReadyButton->enable( false );
		}
		break;
	case NAS_ROOM_READY:
		if ( playerMgr->checkPlayerFlag( ServerPlayer::eReady , true )  )
		{
			mReadyButton->enable( true );
			mReadyButton->setTitle( LAN("Start Game") );
		}
		break;
	}
}

void NetRoomStage::procMsg( IComPacket* cp )
{
	CSPMsg* com = cp->cast< CSPMsg >();

	FixString< 128 > str;

	switch( com->type )
	{
	case  CSPMsg::eSERVER:
		str.format( "## %s ##" , com->content.c_str() );
		mMsgPanel->addMessage( str , RGB( 255 , 0 , 255 ) );
		//CFly::Msg(  "server :" , com->str );
		break;
	case  CSPMsg::ePLAYER:
		{
			GamePlayer* player = mWorker->getPlayerManager()->getPlayer( com->playerID );

			if ( !player )
				return;
			str.format( "%s : %s " , player->getName() , com->content.c_str() );
			mMsgPanel->addMessage( str , RGB( 255 , 255 , 0 ) );
			//CFly::Msg( "( ID = %d ):%s" , com->playerID , com->str );
		}
		break;
	}
}



void NetRoomStage::procPlayerStatus( IComPacket* cp )
{
	SPPlayerStatus* com = cp->cast< SPPlayerStatus >();
	mPlayerPanel->setupPlayerList( *com );
}

void NetRoomStage::procSlotState( IComPacket* cp )
{
	SPSlotState* com = cp->cast< SPSlotState >();
	for( int i = 0 ; i < MAX_PLAYER_NUM ; ++i )
	{
		if ( com->state[i] >= 0 )
		{
			GamePlayer* player = mWorker->getPlayerManager()->getPlayer( PlayerId( com->state[i] ) );
			if ( player )
				player->getInfo().slot = SlotId( i );
		}
	}
	mPlayerPanel->refreshPlayerList( com->idx , com->state );
}

void NetRoomStage::procRawData( IComPacket* cp )
{
	CSPRawData* com = cp->cast< CSPRawData >();

	switch( com->id )
	{
	case SETTING_DATA_ID:
		if ( !haveServer() )
		{
			try
			{
				char gameName[ 32 ];
				com->buffer.take( gameName , sizeof( gameName ) );
				mSettingPanel->setGame( gameName );
				mHelper->recvSetting( com->buffer );
			}
			catch ( std::exception& /*e*/ )
			{


			}
		}
		break;

	}
}

void NetRoomStage::generateSetting( CSPRawData& com )
{
	com.id = SETTING_DATA_ID;
	DataStreamBuffer& buffer = com.buffer;

	buffer.fill( Global::getGameManager().getCurGame()->getName() );
	mHelper->sendSetting( buffer );
}


void NetRoomStage::setupGame( char const* name )
{
	IGamePackage* game = Global::getGameManager().changeGame( name );

	SettingHepler* helper = game->createSettingHelper( SHT_NET_ROOM_HELPER );
	assert( dynamic_cast< NetRoomSettingHelper* >( helper ) );
	if ( helper )
	{
		if ( mHelper )
		{
			mHelper->clearUserUI();
			mSettingPanel->adjustGuiLocation();
		}

		mHelper.reset( static_cast< NetRoomSettingHelper* >( helper ) );

		mHelper->addGUIControl( mPlayerPanel );
		mHelper->addGUIControl( mSettingPanel );
		mHelper->setListener( this );
		mHelper->setupSetting( mServer );
		
	}
}

void NetRoomStage::onModify( GWidget* ui )
{
	if ( ui->getID() == UI_GAME_SETTING_PANEL )
	{
		long const MinSendSettingTime = 250;
		if ( haveServer() && mLastSendSetting - ::GetTickCount() > MinSendSettingTime )
		{
			sendGameSetting();
		}
	}
}

void NetRoomStage::sendGameSetting( unsigned pID )
{
	CSPRawData com;
	generateSetting( com );
	if ( pID == ERROR_PLAYER_ID )
	{
		mServer->sendTcpCommand( &com );
		mLastSendSetting = ::GetTickCount();
	}
	else
	{
		ServerPlayer* player = mServer->getPlayerManager()->getPlayer( pID );
		if ( player )
		{
			player->sendTcpCommand( &com );
		}
	}
}

void NetRoomStage::onAddPlayer( PlayerId id )
{
	mHelper->addPlayerSV( id );
}

void NetRoomStage::onRemovePlayer( PlayerInfo const& info )
{
	mHelper->emptySlotSV( info.slot , SLOT_OPEN );
}


bool NetRoomStage::taskDestroyServerListPanelCL( long time )
{
	if ( mWorker->getActionState() == NAS_ROOM_WAIT )
	{
		mConnectPanel->destroy();
		mConnectPanel = NULL;
		return false;
	}
	return true;
}

void GameStartTask::onStart()
{
	nextMsgSec = 0;
	SynTime    = 0;
}

bool GameStartTask::onUpdate( long time )
{
	int const MaxSec = 3;
	SynTime += time;
	if ( SynTime > MaxSec * 1000 )
	{
		CSPPlayerState com;
		com.playerID = ERROR_PLAYER_ID;
		com.state    = NAS_LEVEL_SETUP;
		server->sendTcpCommand( &com );
		return false;
	}
	else if ( SynTime > nextMsgSec * 1000 && nextMsgSec <= MaxSec )
	{
		CSPMsg com;
		com.type = CSPMsg::eSERVER;
		com.content.format( LAN("Game will start after %d sec..") , MaxSec - nextMsgSec );
		server->sendTcpCommand( &com );
		++nextMsgSec;
	}
	return true;
}

GameNetLevelStage::GameNetLevelStage() 
	:NetStageT< GameLevelStage >( GT_NET_GAME )
{
	mNetEngine = &gEmptyNetEngine;
}

bool GameNetLevelStage::onInit()
{
	if ( !BaseClass::onInit() )
		return false;

	::Global::getGUI().enableRender( false );
	::Global::getGUI().cleanupWidget();


	mMsgPanel = new ComMsgPanel( UI_ANY , Vec2i(0,0) , Vec2i( PlayerListPanel::WidgetSize.x , 230 ) , NULL );
	mMsgPanel->setWorker( mWorker );
	mMsgPanel->show( false );
	::Global::getGUI().addWidget( mMsgPanel );

	if ( haveServer() )
	{

	}
	else if ( !getClientWorker()->haveConnect() )
	{
		mNetEngine = &gEmptyNetEngine;

		ServerListPanel* panel = new ServerListPanel( getClientWorker() , Vec2i( 0 , 0 ) , NULL );

		panel->setPos( ::Global::getGUI().calcScreenCenterPos( panel ->getSize() ) );

		//addTask( TaskUtility::createMemberFunTask( this , &NetRoomStage::taskDestroyServerListPanelCL ) );
		::Global::getGUI().addWidget( panel );

		panel->refreshServerList();
		panel->doModal();
	}

	return true;
}


bool GameNetLevelStage::loadLevel( GameLevelInfo const& info )
{
	IPlayerManager* playerMgr = getPlayerManager();

	mSeed = info.seed;
	getSubStage()->setupLevel( info );

	getSubStage()->setupScene( *playerMgr );

	if ( haveServer() )
		mNetEngine->setupInputAI( *playerMgr );

	if ( !buildNetEngine() )
	{
		return false;
	}

	if ( !buildReplayRecorder() )
	{

	}

	return true;
}

bool GameNetLevelStage::buildNetEngine()
{
	NetWorker* netWorker = getManager()->getNetWorker();

	mNetEngine = &gLocalNetEngine;
	if ( !getSubStage()->setupNetwork( netWorker , &mNetEngine ) )
	{
		getManager()->setErrorMsg( "Can't setup Network" );
		return false;
	}

	INetEngine::BuildInfo info;
	info.netWorker = netWorker;
	info.worker    = mWorker;
	info.game      = getGame();
	info.processor = &getActionProcessor();
	info.tickTime  = mTickTime;

	if ( !mNetEngine->build( info ) )
	{
		getManager()->setErrorMsg( "Can't Build NetEngine" );
		return false;
	}
	return true;
}

void GameNetLevelStage::onRestart( uint64& seed )
{
	//FIXME
	seed = mSeed;
	BaseClass::onRestart( seed );
}


void GameNetLevelStage::onEnd()
{
	if ( mNetEngine )
	{
		mNetEngine->close();
		mNetEngine->release();
		mNetEngine = NULL;
	}
	BaseClass::onEnd();
}

void GameNetLevelStage::tick()
{
	ActionProcessor& ctrlManager = getActionProcessor();

	unsigned flag = 0;
	switch( getState() )
	{
	case GS_RUN:
		++mReplayFrame;
		break;
	default:
		flag |= CTF_FREEZE_FRAME;
	}
	getActionProcessor().beginAction( flag );
	getSubStage()->tick();
	getActionProcessor().endAction();
}

void GameNetLevelStage::updateFrame( int frame )
{
	getSubStage()->updateFrame( frame );
}

void GameNetLevelStage::onUpdate( long time )
{
	BaseClass::onUpdate( time );
	mNetEngine->update( *this , time );

	if ( getGame() )
		::Global::getGUI().scanHotkey( getGame()->getController() );
}

void GameNetLevelStage::onServerEvent( EventID event , unsigned msg )
{
	FixString< 256 > str;

	switch( event )
	{
	case eCON_CLOSE:
		str.format( "Lost Server %s" , "" );
		::Global::getGUI().showMessageBox( UI_ANY , str , GMB_OK );
		break;
	}
}

bool GameNetLevelStage::onKey( unsigned key , bool isDown )
{
	if ( isDown && key == VK_TAB )
	{
		bool beShow = !mMsgPanel->isShow();
		mMsgPanel->show( beShow );
		if ( beShow )
		{
			mMsgPanel->clearInputString();
			mMsgPanel->setFocus();
		}
		getGame()->getController().blockKeyEvent( beShow );
		return false;
	}

	if ( !BaseClass::onKey( key , isDown ) ) 
		return false;

	return true;
}

bool GameNetLevelStage::onWidgetEvent( int event , int id , GWidget* ui )
{
	if ( !BaseClass::onWidgetEvent( event , id , ui ) )
		return false;

	switch( id )
	{
	case UI_PAUSE_GAME:
		{
			CSPPlayerState com;
			com.playerID = mWorker->getPlayerManager()->getUserID();
			com.state    = NAS_LEVEL_PAUSE;
			mWorker->sendTcpCommand( &com );
		}
		return false;
	case UI_UNPAUSE_GAME:
		{
			CSPPlayerState com;
			com.playerID = mWorker->getPlayerManager()->getUserID();
			com.state    = NAS_LEVEL_RUN;
			mWorker->sendTcpCommand( &com );
		}
		return false;
	case UI_RESTART_GAME:
		if ( event == EVT_BOX_YES  )
		{
			if ( haveServer() )
				mServer->changeState( NAS_LEVEL_RESTART );
			return false;
		}
		else if ( event == EVT_BOX_NO )
		{
			if ( haveServer() && getState() == GS_END )
			{
				mServer->changeState( NAS_ROOM_ENTER );
			}
			return false;
		}
		else
		{
			if ( haveServer() )
			{
				if ( getState() == GS_END )
				{
					mServer->changeState( NAS_LEVEL_RESTART );
				}
				else
				{
					::Global::getGUI().showMessageBox( 
						UI_RESTART_GAME , LAN("Do you Want to Stop Current Game?") , GMB_YESNO );
				}
			}
			return false;

		}
		break;
	case UI_GAME_MENU:
	case UI_MAIN_MENU:
		if ( event == EVT_BOX_YES )
		{
			disconnect();
			getManager()->changeStage( ( id == UI_MAIN_MENU ) ? STAGE_MAIN_MENU : STAGE_GAME_MENU );
			return true;
		}
		else if ( event == EVT_BOX_NO )
		{


		}
		else
		{
			::Global::getGUI().showMessageBox( id , LAN("Be Sure Exit Game") , GMB_YESNO );
			return false;
		}
		break;

	}
	return true;
}

bool GameNetLevelStage::tryChangeState( GameState state )
{
	switch( getState() )
	{
	case GS_START:
		if ( mWorker->getActionState() != NAS_LEVEL_RUN )
			return false;
	}
	return true;
}

void GameNetLevelStage::setupWorkerProcFun( ComEvaluator& evaluator )
{
#define DEFINE_CP_USER_FUN( Class , Func )\
	evaluator.setUserFun< Class >( this , &GameNetLevelStage::Func );

	DEFINE_CP_USER_FUN( CSPPlayerState , procPlayerState );
	DEFINE_CP_USER_FUN( SPLevelInfo    , procLevelInfo );
	DEFINE_CP_USER_FUN( CSPMsg         , procMsg );


#undef  DEFINE_CP_USER_FUN
}

void GameNetLevelStage::setupServerProcFun( ComEvaluator& evaluator )
{

#define DEFINE_CP_USER_FUN( Class , Func )\
	evaluator.setUserFun< Class >( this , &GameNetLevelStage::Func );

	DEFINE_CP_USER_FUN( CSPPlayerState , procPlayerStateSv );

#undef  DEFINE_CP_USER_FUN

}

void GameNetLevelStage::procPlayerStateSv( IComPacket* cp )
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();


}

void GameNetLevelStage::procPlayerState( IComPacket* cp )
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();

	switch( com->state )
	{
	case NAS_ROOM_ENTER:
		{
			NetRoomStage* stage = static_cast< NetRoomStage* >( getManager()->changeStage( STAGE_NET_ROOM ) );
			stage->initWorker( mWorker , mServer );
		}
		break;
	case NAS_LEVEL_LOAD:
		{
			if ( haveServer() )
			{
				assert( com->playerID == ERROR_PLAYER_ID );
				SPLevelInfo info;
				getSubStage()->buildServerLevel( info );
				mServer->sendTcpCommand( &info );
			}
		}
		break;
	case NAS_LEVEL_PAUSE:
		if ( com->playerID != ERROR_PLAYER_ID )
		{
			if ( haveServer() ||  com->playerID == mWorker->getPlayerManager()->getUserID() )
			{
				::Global::getGUI().showMessageBox( 
					UI_UNPAUSE_GAME , LAN("Stop Game. Click OK to Continue Game.") , GMB_OK );
			}
			else
			{
				GamePlayer* player = mWorker->getPlayerManager()->getPlayer( com->playerID );
				FixString< 256 > str;
				str.format( LAN("%s Puase Game" ) , player->getName() );
				::Global::getGUI().showMessageBox( 
					UI_UNPAUSE_GAME , str , GMB_NONE );
			}
		}
		break;
	case NAS_LEVEL_RUN:
		{
			GWidget* ui = ::Global::getGUI().getManager().getModalUI();
			if ( ui && ui->getID() == UI_UNPAUSE_GAME )
			{
				ui->destroy();
			}
			mWorker->changeState( NAS_LEVEL_RUN );
		}
		break;
	case NAS_LEVEL_INIT:
		{
			restart( true );
			::Global::getGUI().enableRender( true );
			mWorker->changeState( NAS_LEVEL_INIT );
		}
		break;
	case NAS_LEVEL_RESTART:
		{
			restart( false );
			mWorker->changeState( NAS_LEVEL_RESTART );
		}
		break;
	case NAS_DISSCONNECT:
		if ( com->playerID == ERROR_PLAYER_ID ||
			 com->playerID == mWorker->getPlayerManager()->getUserID() )
		{
			getManager()->changeStage( STAGE_MAIN_MENU );
		}
		break;
	}
}

void GameNetLevelStage::procMsg( IComPacket* cp )
{
	CSPMsg* com = cp->cast< CSPMsg >();

	FixString< 128 > str;

	switch( com->type )
	{
	case CSPMsg::eSERVER:
		str.format( "## %s ##" , com->content.c_str() );
		mMsgPanel->addMessage( str , RGB( 255 , 0 , 255 ) );
		//::Msg(  "server :" , com->str );
		break;
	case CSPMsg::ePLAYER:
		{
			GamePlayer* player = mWorker->getPlayerManager()->getPlayer( com->playerID );

			if ( !player )
				return;
			str.format( "%s : %s " , player->getName() , com->content.c_str() );
			mMsgPanel->addMessage( str , RGB( 255 , 255 , 0 ) );
			//::Msg( "( ID = %d ):%s" , com->playerID , com->str );
		}
		break;
	}
}

IPlayerManager* GameNetLevelStage::getPlayerManager()
{
	return mWorker->getPlayerManager();
}

void GameNetLevelStage::onRender( float dFrame )
{
	if ( getWorker()->getActionState() == NAS_LEVEL_LOAD ||
		 getWorker()->getActionState() == NAS_LEVEL_SETUP ) 
		return;
	BaseClass::onRender( dFrame );
}

void GameNetLevelStage::procLevelInfo(IComPacket* cp)
{
	SPLevelInfo* com = cp->cast< SPLevelInfo >();
	if ( !loadLevel( *com ) )
	{
		mWorker->changeState( NAS_LEVEL_LOAD_FAIL );
		return;
	}
	mWorker->changeState( NAS_LEVEL_LOAD );
}

NetStageData::NetStageData()
{
	mWorker = NULL; 
	mServer = NULL;
}

void NetStageData::disconnect()
{
	if ( haveServer() )
	{
		mServer->changeState( NAS_DISSCONNECT );
	}	
	else
	{
		CSPPlayerState com;
		com.state    =  NAS_DISSCONNECT;
		com.playerID =  mWorker->getPlayerManager()->getUserID();
		mWorker->sendTcpCommand( &com );
	}
}

void NetStageData::initWorker( ComWorker* worker , ServerWorker* server /*= NULL */ )
{
	mWorker = worker;
	assert( mWorker );
	mServer = server;
	registerNetEvent();
}

void NetStageData::unregisterNetEvent( void* processor )
{
	mWorker->getEvaluator().removeProcesserFun( processor );
	if ( mServer )
	{
		mServer->getEvaluator().removeProcesserFun( processor );
	}
	else
	{
		static_cast< ClientWorker* >( mWorker )->setClientListener( NULL );
	}
}

void NetStageData::registerNetEvent()
{
	setupWorkerProcFun( mWorker->getEvaluator() );
	if ( haveServer() )
	{
		setupServerProcFun( mServer->getEvaluator() );
	}
	else
	{
		static_cast< ClientWorker* >( mWorker )->setClientListener( this );
	}
}


