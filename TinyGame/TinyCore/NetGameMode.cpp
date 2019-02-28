#include "TinyGamePCH.h"
#include "NetGameMode.h"

#include "GameStage.h"
#include "GameWorker.h"
#include "GameClient.h"
#include "GameModuleManager.h"

#include "CSyncFrameManager.h"
#include "RenderUtility.h"
#include "PropertyKey.h"

#include "GameGUISystem.h"
#include "GameWidgetID.h"
#include "GameSettingPanel.h"
#include "Widget/ServerListPanel.h"

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
	virtual bool build( BuildParam& buildParam )
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
	virtual bool build( BuildParam& buildParam )
	{
		mTickTime = buildParam.tickTime;
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
	mNeedSendSetting = false;
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
	

	::Global::GUI().cleanupWidget();
	
	VERIFY_RETURN_FALSE(setupUI(haveServer()));

	if ( haveServer() )
	{
		mServer->getPlayerManager()->setListener( this );

		for( auto iter = mServer->getPlayerManager()->createIterator(); iter; ++iter )
		{
			GamePlayer* player = iter.getElement();
			mPlayerPanel->addPlayer( player->getInfo() );
		}

		IGameModule* curGame = Global::GameManager().getRunningGame();
		mSettingPanel->setGame( curGame ? curGame->getName() : NULL );

		mHelper->sendSlotStateSV();
	}
	else
	{
		ClientWorker* worker = static_cast< ClientWorker*>( mWorker );

		if ( worker->getActionState() == NAS_DISSCONNECT )
		{
			mConnectPanel  = new ServerListPanel( worker , Vec2i( 0 , 0 ) , NULL );
			mConnectPanel->setPos( ::Global::GUI().calcScreenCenterPos( mConnectPanel ->getSize() ) );

			addTask( TaskUtility::MemberFun( this , &NetRoomStage::taskDestroyServerListPanelCL ) );
			::Global::GUI().addWidget( mConnectPanel  );

			mConnectPanel->refreshServerList();
			mConnectPanel->doModal();
		}
	}

	mWorker->changeState( NAS_ROOM_ENTER );
	LogMsg("Net Room Init");
	return true;
}



void NetRoomStage::onEnd()
{
	if ( haveServer() )
	{
		mServer->getPlayerManager()->setListener( NULL );
	}
	unregisterNetEvent(this);
	BaseClass::onEnd();
}


bool NetRoomStage::setupUI(bool bFullSetting)
{

	GameModuleVec gameVec;
	Global::GameManager().classifyGame( ATTR_NET_SUPPORT , gameVec );
	if ( gameVec.empty() )
	{
		return false;
	}

	typedef Easing::OQuart EasingFun;

	long const timeUIAnim = 1000;

	Vec2i panelPos = Vec2i( 20 , 20 );
	{
		if ( mPlayerPanel == nullptr )
		{
			mPlayerPanel = new PlayerListPanel(
				mWorker->getPlayerManager(), UI_PLAYER_LIST_PANEL, panelPos, NULL);
			mPlayerPanel->init(haveServer());
			::Global::GUI().addWidget(mPlayerPanel);
		}

		if ( bFullSetting )
		{
			mPlayerPanel->show(true);
			Vec2i from = Vec2i(-mPlayerPanel->getSize().x, panelPos.y);
			mPlayerPanel->setPos(Vec2i(-mPlayerPanel->getSize().x, panelPos.y));
			::Global::GUI().addMotion< EasingFun >(mPlayerPanel, from, panelPos, timeUIAnim);
		}
		else
		{
			mPlayerPanel->show(false);
		}
	}


	Vec2i sizeSettingPanel( 350 , 400 );
	{
		Vec2i pos = panelPos + Vec2i( PlayerListPanel::WidgetSize.x + 10 , 0 ); 
		if( mSettingPanel == nullptr )
		{
			mSettingPanel = new GameSettingPanel(
				UI_GAME_SETTING_PANEL, pos, sizeSettingPanel, NULL);

			if( !haveServer() )
			{
				mSettingPanel->enable(false);
			}
			::Global::GUI().addWidget(mSettingPanel);

			for( GameModuleVec::iterator iter = gameVec.begin();
				iter != gameVec.end(); ++iter )
			{
				mSettingPanel->addGame(*iter);
			}
		}

		if ( bFullSetting )
		{
			mSettingPanel->show(true);
			Vec2i from = Vec2i(Global::GetDrawEngine()->getScreenWidth(), pos.y);
			::Global::GUI().addMotion< EasingFun >(mSettingPanel, from, pos, timeUIAnim);
		}
		else
		{
			mSettingPanel->show(false);
		}
	}

	{
		Vec2i pos = panelPos + Vec2i( 0 , PlayerListPanel::WidgetSize.y + 20 );
		if ( mMsgPanel == nullptr )
		{
			mMsgPanel = new ComMsgPanel(UI_ANY, pos, Vec2i(PlayerListPanel::WidgetSize.x, 230), NULL);
			mMsgPanel->setWorker(mWorker);
			::Global::GUI().addWidget(mMsgPanel);
		}

		if ( bFullSetting )
		{
			mMsgPanel->show(true);
			Vec2i from = Vec2i(-mMsgPanel->getSize().x, pos.y);
			::Global::GUI().addMotion< EasingFun >(mMsgPanel, from, pos, timeUIAnim);
		}
		else
		{
			mMsgPanel->show(false);
		}
	}

	if ( bFullSetting )
	{
		Vec2i btnSize(sizeSettingPanel.x, 30);
		Vec2i btnPos(Vec2i(panelPos.x + PlayerListPanel::WidgetSize.x + 10, panelPos.y + sizeSettingPanel.y + 10));

		GButton* button;
		if( haveServer() )
		{
			button = new GButton(UI_NET_ROOM_START, btnPos, btnSize, NULL);
			button->setTitle(LOCTEXT("Start Game"));
			//button->enable( false );
		}
		else
		{
			button = new GButton(UI_NET_ROOM_READY, btnPos, btnSize, NULL);
			button->setTitle(LOCTEXT("Ready"));
		}

		{
			Vec2i from = Vec2i(Global::GetDrawEngine()->getScreenWidth(), btnPos.y);
			::Global::GUI().addMotion< EasingFun >(button, from, btnPos, timeUIAnim);
		}

		button->setFontType(FONT_S10);
		mReadyButton = button;
		::Global::GUI().addWidget(button);

		btnPos += Vec2i(0, btnSize.y + 5);
		button = new GButton(UI_MAIN_MENU, btnPos, btnSize, NULL);
		button->setFontType(FONT_S10);
		button->setTitle(LOCTEXT("Exit"));
		::Global::GUI().addWidget(button);
		mExitButton = button;
		{
			Vec2i from = Vec2i(Global::GetDrawEngine()->getScreenWidth(), btnPos.y);
			::Global::GUI().addMotion< EasingFun >(button, from, btnPos, timeUIAnim);
		}
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
				GChoice* choice = GUI::CastFast< GChoice >( ui );
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
		if ( mHelper->checkSettingSV() )
		{
			assert( haveServer() );

			CSPPlayerState com;
			com.playerId = mServer->getPlayerManager()->getUserID();
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
			GButton* button = GUI::CastFast< GButton >( ui );
			assert( button == mReadyButton );
			if ( mWorker->getActionState() != NAS_ROOM_READY )
			{
				mWorker->changeState( NAS_ROOM_READY );
				button->setTitle( LOCTEXT("Cancel Ready") );
			}
			else
			{
				mWorker->changeState( NAS_ROOM_WAIT );
				button->setTitle( LOCTEXT("Ready") );
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
			setupUI(true);
		break;
	}
	
}

void NetRoomStage::onUpdate( long time )
{
	BaseClass::onUpdate( time );
	if ( mNeedSendSetting && SystemPlatform::GetTickCount() - mLastSendSetting > 500 )
	{
		sendGameSetting();
		mNeedSendSetting = false;
	}
	::Global::GUI().updateFrame( time / gDefaultTickTime , gDefaultTickTime );
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

			assert( Global::GameManager().getRunningGame() );

			Global::GameManager().getRunningGame()->beginPlay( SMT_NET_GAME , *getManager() );

			StageBase* nextStage = getManager()->getNextStage();
			if( nextStage )
			{
				//#TODO
				if( auto gameStage = dynamic_cast<GameStageBase*>(nextStage) )
				{
					auto netStageMode = dynamic_cast<NetLevelStageMode*>(gameStage->getStageMode());
					if( netStageMode )
					{
						netStageMode->initWorker(mWorker, mServer);
					}
				}

				mHelper->setupGame(*getManager(), nextStage);
				if( haveServer() )
				{
					mHelper->sendPlayerStatusSV();
				}

				mWorker->changeState(NAS_LEVEL_SETUP);
			}
			else
			{
				LogMsg("No NetLevel!");
			}
		}
		break;
	case NAS_ROOM_READY:
		{
			GamePlayer* player = mWorker->getPlayerManager()->getPlayer( com->playerId );
			PlayerListPanel::Slot& slot = mPlayerPanel->getSlot( player->getInfo().slot );
			slot.choice->setColorName( EColor::Green );
		}
		break;
	case NAS_ROOM_WAIT:
		{
			GamePlayer* player = mWorker->getPlayerManager()->getPlayer( com->playerId );
			PlayerListPanel::Slot& slot = mPlayerPanel->getSlot( player->getInfo().slot );
			slot.choice->setColorName( EColor::Blue );
		}
		break;
	case NAS_CONNECT:
		if ( haveServer() )
		{
			CSPRawData settingCom;
			generateSetting( settingCom );
			mServer->getPlayerManager()->getPlayer( com->playerId )->sendTcpCommand( &settingCom );
		}

		{
			GamePlayer* player = mWorker->getPlayerManager()->getPlayer( com->playerId );
			FixString< 64 > str;
			str.format( LOCTEXT("== %s Inter Room =="), player->getName() );
			mMsgPanel->addMessage( str , Color3ub( 255 , 125 , 0 ) );
		}
		break;
	case NAS_DISSCONNECT:
		if ( com->playerId == SERVER_PLAYER_ID )
		{
			::Global::GUI().showMessageBox( 
				UI_DISCONNECT_MSGBOX , 
				LOCTEXT("Server shout down") , GMB_OK );
		}
		else if ( mWorker->getPlayerManager()->getUserID() == com->playerId )
		{
			::Global::GUI().showMessageBox( 
				UI_DISCONNECT_MSGBOX , 
				LOCTEXT("You are kicked by Server") , GMB_OK );
		}
		else
		{
			GamePlayer* player = mWorker->getPlayerManager()->getPlayer( com->playerId );
			if ( player )
			{
				PlayerListPanel::Slot& slot = mPlayerPanel->getSlot(player->getInfo().slot);
				slot.choice->setColorName(EColor::Blue);

				{
					FixString< 64 > str;
					str.format(LOCTEXT("== %s Leave Room =="), player->getName());
					mMsgPanel->addMessage(str, Color3ub(255, 125, 0));
				}
			}
		}
		break;
	}

}

void NetRoomStage::procPlayerStateSv( IComPacket* cp)
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();
	SVPlayerManager* playerMgr = mServer->getPlayerManager();

	switch( com->state )
	{
	case NAS_ROOM_ENTER:
		{
			sendGameSetting( com->playerId );
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
			mReadyButton->setTitle( LOCTEXT("Start Game") );
		}
		break;
	}
}

void NetRoomStage::procMsg( IComPacket* cp)
{
	CSPMsg* com = cp->cast< CSPMsg >();

	FixString< 128 > str;

	switch( com->type )
	{
	case  CSPMsg::eSERVER:
		str.format( "## %s ##" , com->content.c_str() );
		mMsgPanel->addMessage( str , Color3ub( 255 , 0 , 255 ) );
		//CFly::Msg(  "server :" , com->str );
		break;
	case  CSPMsg::ePLAYER:
		{
			GamePlayer* player = mWorker->getPlayerManager()->getPlayer( com->playerID );

			if ( !player )
				return;
			str.format( "%s : %s " , player->getName() , com->content.c_str() );
			mMsgPanel->addMessage( str , Color3ub( 255 , 255 , 0 ) );
			//CFly::Msg( "( ID = %d ):%s" , com->playerID , com->str );
		}
		break;
	}
}


void NetRoomStage::procPlayerStatus( IComPacket* cp)
{
	SPPlayerStatus* com = cp->cast< SPPlayerStatus >();

	mPlayerPanel->setupPlayerList(*com);
}

void NetRoomStage::procSlotState( IComPacket* cp)
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

	mPlayerPanel->refreshPlayerList(com->idx, com->state);
}

void NetRoomStage::procRawData( IComPacket* cp)
{
	CSPRawData* com = cp->cast< CSPRawData >();

	switch( com->id )
	{
	case SETTING_DATA_ID:
		if ( !haveServer() )
		{
			try
			{
				char gameName[ 128 ];
				com->buffer.take( gameName , sizeof( gameName ) );
				mSettingPanel->setGame( gameName );
				mHelper->importSetting( com->buffer );
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
	DataSteamBuffer& buffer = com.buffer;

	buffer.fill( Global::GameManager().getRunningGame()->getName() );
	mHelper->exportSetting( buffer );
}


void NetRoomStage::setupGame( char const* name )
{
	IGameModule* game = Global::GameManager().changeGame( name );

	if( game )
	{
		SettingHepler* helper = game->createSettingHelper(SHT_NET_ROOM_HELPER);
		assert(dynamic_cast<NetRoomSettingHelper*>(helper));
		if( helper )
		{
			if( mHelper )
			{
				mHelper->clearUserUI();
				mSettingPanel->adjustChildLayout();
			}

			mHelper.reset(static_cast<NetRoomSettingHelper*>(helper));

			mHelper->addGUIControl(mPlayerPanel);
			mHelper->addGUIControl(mSettingPanel);
			mHelper->setListener(this);
			mHelper->setupSetting(mServer);

		}
	}
	else
	{
		LogWarning(0, "Can't Setup Game");
	}

}

void NetRoomStage::onModify( GWidget* ui )
{
	if ( ui->getID() == UI_GAME_SETTING_PANEL )
	{
		int64 const MinSendSettingTime = 250;
		if ( haveServer() && mLastSendSetting - SystemPlatform::GetTickCount() > MinSendSettingTime )
		{
			sendGameSetting();
		}
		else
		{
			mNeedSendSetting = true;
		}
	}
}

void NetRoomStage::sendGameSetting( unsigned pID )
{
	CSPRawData com;
	generateSetting( com );
	if ( pID == SERVER_PLAYER_ID )
	{
		mServer->sendTcpCommand( &com );
		mLastSendSetting = SystemPlatform::GetTickCount();
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
	if( !mHelper->addPlayerSV(id) )
	{


	}
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
		com.setServerState(NAS_LEVEL_SETUP);
		server->sendTcpCommand( &com );
		return false;
	}
	else if ( SynTime > nextMsgSec * 1000 && nextMsgSec <= MaxSec )
	{
		CSPMsg com;
		com.type = CSPMsg::eSERVER;
		com.content.format( LOCTEXT("Game will start after %d sec..") , MaxSec - nextMsgSec );
		server->sendTcpCommand( &com );
		++nextMsgSec;
	}
	return true;
}

NetStageData::NetStageData()
{
	mWorker = NULL; 
	mServer = NULL;
	bCloseNetWork = false;
}

NetStageData::~NetStageData()
{
	if( bCloseNetWork )
	{
		::Global::GameNet().closeNetwork();
	}
}

void NetStageData::disconnect()
{
	if ( haveServer() )
	{
		mServer->changeState( NAS_DISSCONNECT );
		bCloseNetWork = true;
	}	
	else
	{
		CSPPlayerState com;
		com.state    =  NAS_DISSCONNECT;
		com.playerId =  mWorker->getPlayerManager()->getUserID();
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


NetLevelStageMode::NetLevelStageMode() 
	:LevelStageMode(SMT_NET_GAME)
{
	mNetEngine = &gEmptyNetEngine;
	mbReconnectMode = false;
}

bool NetLevelStageMode::prevStageInit()
{
	if( !BaseClass::prevStageInit() )
		return false;

	if( haveServer() )
	{
		mServer->setEventResolver(this);
	}
	return true;
}

bool NetLevelStageMode::postStageInit()
{
	if( !BaseClass::postStageInit() )
		return false;

	::Global::GUI().hideWidgets(true);
	::Global::GUI().cleanupWidget();

	mMsgPanel = new ComMsgPanel(UI_ANY, Vec2i(0, 0), Vec2i(PlayerListPanel::WidgetSize.x, 230), NULL);
	mMsgPanel->setWorker(mWorker);
	mMsgPanel->show(false);
	::Global::GUI().addWidget(mMsgPanel);

	if( haveServer() )
	{

	}
	else if( !getClientWorker()->haveConnect() )
	{
		mNetEngine = &gEmptyNetEngine;

		ServerListPanel* panel = new ServerListPanel(getClientWorker(), Vec2i(0, 0), NULL);

		panel->setPos(::Global::GUI().calcScreenCenterPos(panel->getSize()));

		//addTask( TaskUtility::createMemberFunTask( this , &NetRoomStage::taskDestroyServerListPanelCL ) );
		::Global::GUI().addWidget(panel);

		panel->refreshServerList();
		panel->doModal();

		::Global::GUI().hideWidgets(false);
	}
	return true;
}

void NetLevelStageMode::onEnd()
{
	if( mNetEngine )
	{
		mNetEngine->close();
		mNetEngine->release();
		mNetEngine = NULL;
	}
	unregisterNetEvent(this);

	if( haveServer() )
	{
		mServer->setEventResolver(nullptr);
	}
	BaseClass::onEnd();
}

bool NetLevelStageMode::onWidgetEvent(int event, int id, GWidget* ui)
{
	if( !BaseClass::onWidgetEvent(event, id, ui) )
		return false;

	switch( id )
	{
	case UI_PAUSE_GAME:
		{
			CSPPlayerState com;
			com.playerId = mWorker->getPlayerManager()->getUserID();
			com.state = NAS_LEVEL_PAUSE;
			mWorker->sendTcpCommand(&com);
		}
		return false;
	case UI_UNPAUSE_GAME:
		{
			CSPPlayerState com;
			com.playerId = mWorker->getPlayerManager()->getUserID();
			com.state = NAS_LEVEL_RUN;
			mWorker->sendTcpCommand(&com);
		}
		return false;
	case UI_RESTART_GAME:
		if( event == EVT_BOX_YES )
		{
			if( haveServer() )
				mServer->changeState(NAS_LEVEL_RESTART);
			return false;
		}
		else if( event == EVT_BOX_NO )
		{
			if( haveServer() && getGameState() == GS_END )
			{
				mServer->changeState(NAS_ROOM_ENTER);
			}
			return false;
		}
		else
		{
			if( haveServer() )
			{
				if( getGameState() == GS_END )
				{
					mServer->changeState(NAS_LEVEL_RESTART);
				}
				else
				{
					::Global::GUI().showMessageBox(
						UI_RESTART_GAME, LOCTEXT("Do you Want to Stop Current Game?"), GMB_YESNO);
				}
			}
			return false;

		}
		break;
	case UI_GAME_MENU:
	case UI_MAIN_MENU:
		if( event == EVT_BOX_YES )
		{
			getStage()->getManager()->changeStage((id == UI_MAIN_MENU) ? STAGE_MAIN_MENU : STAGE_GAME_MENU);
			disconnect();
			return true;
		}
		else if( event == EVT_BOX_NO )
		{


		}
		else
		{
			::Global::GUI().showMessageBox(id, LOCTEXT("Be Sure Exit Game"), GMB_YESNO);
			return false;
		}
		break;

	}
	return true;
}

bool NetLevelStageMode::onKey(unsigned key, bool isDown)
{
	if( isDown && key == Keyboard::eTAB )
	{
		bool beShow = !mMsgPanel->isShow();
		mMsgPanel->show(beShow);
		if( beShow )
		{
			mMsgPanel->clearInputString();
			mMsgPanel->makeFocus();
		}
		getGame()->getController().blockAllAction(beShow);
		return false;
	}

	if( !BaseClass::onKey(key, isDown) )
		return false;

	return true;
}

void NetLevelStageMode::onRestart(uint64& seed)
{
	//FIXME
	seed = mNetSeed;
	BaseClass::onRestart(seed);
}

bool NetLevelStageMode::tryChangeState(GameState state)
{
	switch( getGameState() )
	{
	case GS_START:
		if( mWorker->getActionState() != NAS_LEVEL_RUN )
			return false;
	}
	return true;
}

IPlayerManager* NetLevelStageMode::getPlayerManager()
{
	return mWorker->getPlayerManager();
}

void NetLevelStageMode::updateFrame(int frame)
{
	getStage()->updateFrame(frame);
}

void NetLevelStageMode::tick()
{
	ActionProcessor& actionProcessor = getStage()->getActionProcessor();

	unsigned flag = 0;
	switch( getGameState() )
	{
	case GS_RUN:
		++mReplayFrame;
		break;
	default:
		flag |= CTF_FREEZE_FRAME;
	}
	actionProcessor.beginAction(flag);
	getStage()->tick();
	actionProcessor.endAction();
}

void NetLevelStageMode::setupServerProcFun(ComEvaluator& evaluator)
{
#define DEFINE_CP_USER_FUN( Class , Func )\
		evaluator.setUserFun< Class >( this , &NetLevelStageMode::Func );

	DEFINE_CP_USER_FUN(CSPPlayerState, procPlayerStateSv);

#undef  DEFINE_CP_USER_FUN
}

void NetLevelStageMode::setupWorkerProcFun(ComEvaluator& evaluator)
{
#define DEFINE_CP_USER_FUN( Class , Func )\
		evaluator.setUserFun< Class >( this , &NetLevelStageMode::Func );

	DEFINE_CP_USER_FUN(CSPPlayerState, procPlayerState);
	DEFINE_CP_USER_FUN(SPLevelInfo, procLevelInfo);
	DEFINE_CP_USER_FUN(CSPMsg, procMsg);
	DEFINE_CP_USER_FUN(SPNetControlRequest, procNetControlRequest);


#undef  DEFINE_CP_USER_FUN
}

void NetLevelStageMode::procPlayerStateSv(IComPacket* cp)
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();
}

void NetLevelStageMode::procNetControlRequest(IComPacket* cp)
{
	SPNetControlRequest* com = cp->cast<SPNetControlRequest>();

	NetControlAction action = NetControlAction(com->action);
	switch( action )
	{
	case NetControlAction::LevelSetup:
		break;
	case NetControlAction::LevelLoad:
		{
			if( haveServer() )
			{
				SPLevelInfo info;
				getStage()->buildServerLevel(info);
				mServer->sendTcpCommand(&info);
			}
		}
		break;

	case NetControlAction::LevelInit:
		{
			doRestart(true);
			::Global::GUI().hideWidgets(false);
			mWorker->changeState(NAS_LEVEL_INIT);
		}
		break;
	case NetControlAction::LevelRun:
		{
			GWidget* ui = ::Global::GUI().getManager().getModalWidget();
			if( ui && ui->getID() == UI_UNPAUSE_GAME )
			{
				ui->destroy();
			}
			mWorker->changeState(NAS_LEVEL_RUN);
		}
		break;
	case NetControlAction::LevelPause:
		{
			if( haveServer() )
			{
				::Global::GUI().showMessageBox(
					UI_UNPAUSE_GAME, LOCTEXT("Stop Game. Click OK to Continue Game."), GMB_OK);
			}
			else
			{
#if 0
				GamePlayer* player = mWorker->getPlayerManager()->getPlayer(com->playerId);
				FixString< 256 > str;
				str.format(LOCTEXT("%s Puase Game"), player->getName());
				::Global::GUI().showMessageBox(
					UI_UNPAUSE_GAME, str, GMB_NONE);
#endif
			}
		}
		break;

	case NetControlAction::LevelRestart:
		{
			if( mNetEngine )
				mNetEngine->restart();
			doRestart(false);
			mWorker->changeState(NAS_LEVEL_RESTART);
		}
		break;
	case NetControlAction::LevelExit:
		break;
	case NetControlAction::LevelChange:
		break;
	}
}

void NetLevelStageMode::procPlayerState(IComPacket* cp)
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();

	switch( com->state )
	{
	case NAS_ROOM_ENTER:
		{
			NetRoomStage* stage = static_cast<NetRoomStage*>( getManager()->changeStage(STAGE_NET_ROOM));
			stage->initWorker(mWorker, mServer);
		}
		break;
	case NAS_LEVEL_LOAD:
		{
			if( haveServer() )
			{
				assert(com->playerId == SERVER_PLAYER_ID);
				SPLevelInfo info;
				getStage()->buildServerLevel(info);
				mServer->sendTcpCommand(&info);
			}
		}
		break;
	case NAS_LEVEL_PAUSE:
		if( com->playerId != SERVER_PLAYER_ID )
		{
			if( haveServer() || com->playerId == mWorker->getPlayerManager()->getUserID() )
			{
				::Global::GUI().showMessageBox(
					UI_UNPAUSE_GAME, LOCTEXT("Stop Game. Click OK to Continue Game."), GMB_OK);
			}
			else
			{
				GamePlayer* player = mWorker->getPlayerManager()->getPlayer(com->playerId);
				FixString< 256 > str;
				str.format(LOCTEXT("%s Puase Game"), player->getName());
				::Global::GUI().showMessageBox(
					UI_UNPAUSE_GAME, str, GMB_NONE);
			}
		}
		break;
	case NAS_LEVEL_RUN:
		{
			GWidget* ui = ::Global::GUI().getManager().getModalWidget();
			if( ui && ui->getID() == UI_UNPAUSE_GAME )
			{
				ui->destroy();
			}
			mWorker->changeState(NAS_LEVEL_RUN);
		}
		break;
	case NAS_LEVEL_INIT:
		{
			doRestart(true);
			::Global::GUI().hideWidgets(false);
			mWorker->changeState(NAS_LEVEL_INIT);
		}
		break;
	case NAS_LEVEL_RESTART:
		{
			if( mNetEngine )
				mNetEngine->restart();
			doRestart(false);
			mWorker->changeState(NAS_LEVEL_RESTART);
		}
		break;
	case NAS_DISSCONNECT:
		if( com->playerId == SERVER_PLAYER_ID ||
		    com->playerId == mWorker->getPlayerManager()->getUserID() )
		{
			if( getStage() )
			{
				getStage()->getManager()->changeStage(STAGE_MAIN_MENU);
			}
		}
		break;
	}
}

void NetLevelStageMode::procLevelInfo(IComPacket* cp)
{
	SPLevelInfo* com = cp->cast< SPLevelInfo >();
	if( !loadLevel(*com) )
	{
		mWorker->changeState(NAS_LEVEL_LOAD_FAIL);
		return;
	}
	mWorker->changeState(NAS_LEVEL_LOAD);
}

void NetLevelStageMode::updateTime(long time)
{
	BaseClass::updateTime(time);
	mNetEngine->update(*this, time);
	if( getGame() )
		::Global::GUI().scanHotkey(getGame()->getController());
}

bool NetLevelStageMode::canRender()
{
	if( getWorker()->getActionState() == NAS_LEVEL_LOAD ||
	    getWorker()->getActionState() == NAS_LEVEL_SETUP )
		return false;
	return true;
}

void NetLevelStageMode::restart(bool beInit)
{
	//TODO
	if( mServer )
	{
		mServer->changeState(NAS_LEVEL_RESTART);
	}
}

void NetLevelStageMode::procMsg(IComPacket* cp)
{
	CSPMsg* com = cp->cast< CSPMsg >();

	FixString< 128 > str;

	switch( com->type )
	{
	case CSPMsg::eSERVER:
		str.format("## %s ##", com->content.c_str());
		mMsgPanel->addMessage(str, Color3ub(255, 0, 255));
		//::Msg(  "server :" , com->str );
		break;
	case CSPMsg::ePLAYER:
		{
			GamePlayer* player = mWorker->getPlayerManager()->getPlayer(com->playerID);

			if( !player )
				return;
			str.format("%s : %s ", player->getName(), com->content.c_str());
			mMsgPanel->addMessage(str, Color3ub(255, 255, 0));
			//::Msg( "( ID = %d ):%s" , com->playerID , com->str );
		}
		break;
	}
}



void NetLevelStageMode::onServerEvent(EventID event, unsigned msg)
{
	FixString< 256 > str;

	switch( event )
	{
	case eCON_CLOSE:
		str.format("Lost Server %s", "");
		::Global::GUI().showMessageBox(UI_ANY, str, GMB_OK);
		break;
	}
}

bool NetLevelStageMode::buildNetEngine()
{
	NetWorker* netWorker = ::Global::GameNet().getNetWorker();

	mNetEngine = &gLocalNetEngine;
	if( !getStage()->setupNetwork(netWorker, &mNetEngine) )
	{
		getStage()->getManager()->setErrorMsg("Can't setup Network");
		return false;
	}

	INetEngine::BuildParam buildParam;
	buildParam.netWorker = netWorker;
	buildParam.worker = mWorker;
	buildParam.game = getGame();
	buildParam.processor = &getStage()->getActionProcessor();
	buildParam.tickTime = getStage()->getTickTime();

	if( !mNetEngine->build(buildParam) )
	{
		getStage()->getManager()->setErrorMsg("Can't Build NetEngine");
		return false;
	}
	return true;
}

bool NetLevelStageMode::loadLevel(GameLevelInfo const& info)
{
	IPlayerManager* playerMgr = getPlayerManager();

	mNetSeed = info.seed;
	getStage()->setupLevel(info);
	getStage()->setupScene(*playerMgr);

	if( haveServer() )
		mNetEngine->setupInputAI(*playerMgr);

	if( !buildNetEngine() )
	{
		return false;
	}

	if( !buildReplayRecorder() )
	{

	}

	return true;
}

PlayerConnetionClosedAction NetLevelStageMode::resolveConnectClosed_NetThread(ServerResolveContext& context, NetCloseReason reason)
{
	assert(IsInNetThread());
	return PlayerConnetionClosedAction::Remove;
}

void NetLevelStageMode::resolveReconnect_NetThread(ServerResolveContext& context)
{
	assert(IsInNetThread());
}
