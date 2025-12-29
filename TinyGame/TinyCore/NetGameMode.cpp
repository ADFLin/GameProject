#include "TinyGamePCH.h"
#include "NetGameMode.h"

#include "GameStage.h"
#include "GameWorker.h"
#include "GameClient.h"
#include "GameModuleManager.h"

#include "CSyncFrameManager.h"
#include "RenderUtility.h"
#include "PropertySet.h"

#include "GameGUISystem.h"
#include "GameWidgetID.h"
#include "GameSettingPanel.h"
#include "Widget/ServerListPanel.h"

#include "GameNetPacket.h"
#include "GameAction.h"


#include "GameSettingHelper.h"

#include "EasingFunction.h"
#include "InputManager.h"

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
	virtual void setupInputAI( IPlayerManager& manager , ActionProcessor& processor )
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
	virtual void setupInputAI( IPlayerManager& manager , ActionProcessor& processor )
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

NetGameMode* NetRoomStage::getMode()
{
	return static_cast<NetGameMode*>(getManager()->getActiveMode());
}
bool NetRoomStage::haveServer() { return getMode() && getMode()->haveServer(); }
ComWorker* NetRoomStage::getWorker() { return getMode() ? getMode()->getWorker() : nullptr; }
ServerWorker* NetRoomStage::getServer() { return getMode() ? getMode()->getServer() : nullptr; }
ClientWorker* NetRoomStage::getClientWorker() { return getMode() ? getMode()->getClientWorker() : nullptr; }


bool NetRoomStage::onInit()
{
	if ( !BaseClass::onInit() )
		return false;



	getManager()->setTickTime( gDefaultTickTime );

	mLastSendSetting = 0;
	

	::Global::GUI().cleanupWidget();
	
	VERIFY_RETURN_FALSE(setupUI(haveServer()));

	setupWorkerProcFunc(getWorker()->getPacketDispatcher());

	if ( haveServer() )
	{
		setupServerProcFunc(getServer()->getPacketDispatcher());

		// ✅ 先設置遊戲並初始化 helper，然後才能添加玩家
		IGameModule* curGame = Global::ModuleManager().getRunningGame();
		mSettingPanel->setGame( curGame ? curGame->getName() : NULL );

		// 現在 helper 已經創建並初始化，可以安全地添加玩家
		for( auto iter = getServer()->getPlayerManager()->createIterator(); iter; ++iter )
		{
			GamePlayer* player = iter.getElement();
			mPlayerPanel->addPlayer( player->getInfo() );
		}

		if (getMode()->getSettingHelper())
		{
			getMode()->getSettingHelper()->sendSlotStateSV();
		}
	}
	else
	{
		ClientWorker* worker = getClientWorker();

		worker->setClientListener(this);
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

	getWorker()->changeState( NAS_ROOM_ENTER );
	LogMsg("Net Room Init");
	return true;
}



void NetRoomStage::onEnd()
{
	if ( haveServer() )
	{
		getServer()->getPlayerManager()->setListener( NULL );
	}
	getMode()->unregisterNetEvent(this);
	BaseClass::onEnd();
}


bool NetRoomStage::setupUI(bool bFullSetting)
{

	GameModuleVec gameVec;
	Global::ModuleManager().classifyGame( ATTR_NET_SUPPORT , gameVec );
	if ( gameVec.empty() )
	{
		return false;
	}

	typedef Easing::OQuart EasingFunc;

	long const timeUIAnim = 1000;

	Vec2i panelPos = Vec2i( 20 , 20 );
	{
		if ( mPlayerPanel == nullptr )
		{
			mPlayerPanel = new PlayerListPanel(
				getWorker()->getPlayerManager(), UI_PLAYER_LIST_PANEL, panelPos, NULL);
			mPlayerPanel->init(haveServer());
			::Global::GUI().addWidget(mPlayerPanel);
		}

		if ( bFullSetting )
		{
			mPlayerPanel->show(true);
			Vec2i from = Vec2i(-mPlayerPanel->getSize().x, panelPos.y);
			mPlayerPanel->setPos(Vec2i(-mPlayerPanel->getSize().x, panelPos.y));
			::Global::GUI().addMotion< EasingFunc >(mPlayerPanel, from, panelPos, timeUIAnim);
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
			Vec2i from = Vec2i(Global::GetScreenSize().x, pos.y);
			::Global::GUI().addMotion< EasingFunc >(mSettingPanel, from, pos, timeUIAnim);
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
			mMsgPanel->setWorker(getWorker());
			::Global::GUI().addWidget(mMsgPanel);
		}

		if ( bFullSetting )
		{
			mMsgPanel->show(true);
			Vec2i from = Vec2i(-mMsgPanel->getSize().x, pos.y);
			::Global::GUI().addMotion< EasingFunc >(mMsgPanel, from, pos, timeUIAnim);
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
			Vec2i from = Vec2i(Global::GetScreenSize().x, btnPos.y);
			::Global::GUI().addMotion< EasingFunc >(button, from, btnPos, timeUIAnim);
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
			Vec2i from = Vec2i(Global::GetScreenSize().x, btnPos.y);
			::Global::GUI().addMotion< EasingFunc >(button, from, btnPos, timeUIAnim);
		}
	}

	// ✅ 連接 Helper 與 UI panels (從 Mode 獲取 Helper)
	if (bFullSetting)
	{
		NetGameMode::GameSwitchContext ctx;
		ctx.playerPanel = mPlayerPanel;
		ctx.settingPanel = mSettingPanel;
		ctx.listener = this;
		getMode()->bindHelperToUI(ctx);
	}

	return true;
}

void NetRoomStage::setupServerProcFunc(PacketDispatcher& dispatcher)
{

#define DEFINE_CP_USER_FUNC( Class , Func )\
	dispatcher.setUserFunc< Class >( this , &NetRoomStage::Func );

	DEFINE_CP_USER_FUNC( CSPPlayerState , procPlayerStateSv )
#undef  DEFINE_CP_USER_FUNC

}

void NetRoomStage::setupWorkerProcFunc(PacketDispatcher& dispatcher)
{
#define DEFINE_CP_USER_FUNC( Class , Func )\
	dispatcher.setUserFunc< Class >( this , &NetRoomStage::Func );

	DEFINE_CP_USER_FUNC( CSPMsg         , procMsg )
	DEFINE_CP_USER_FUNC( CSPPlayerState , procPlayerState )
	DEFINE_CP_USER_FUNC( CSPRawData     , procRawData )
	DEFINE_CP_USER_FUNC( SPPlayerStatus , procPlayerStatus )
	DEFINE_CP_USER_FUNC( SPSlotState    , procSlotState )
#undef  DEFINE_CP_USER_FUNC

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
		// ✅ Server 切換遊戲 - 使用統一的 switchGame 接口
		if (haveServer())
		{
			NetGameMode::GameSwitchContext ctx;
			ctx.playerPanel = mPlayerPanel;
			ctx.settingPanel = mSettingPanel;
			ctx.listener = this;
			ctx.sendToClients = true;
			
			getMode()->switchGame(static_cast<GChoice*>(ui)->getSelectValue(), ctx);
			mSettingPanel->adjustChildLayout();
		}
		return false;
	case UI_PLAYER_LIST_PANEL:
		switch( event )
		{
		case EVT_SLOT_CHANGE:
			if ( haveServer() && getMode()->getSettingHelper() )
			{
				getMode()->getSettingHelper()->sendSlotStateSV();
			}
			return false;
		case EVT_PLAYER_TYPE_CHANGE:
			if ( haveServer() && getMode()->getSettingHelper() )
			{
				getMode()->getSettingHelper()->sendPlayerStatusSV();
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

				if (getMode()->getSettingHelper())
				{
					switch ( choice->getSelection() )
					{
					case PlayerListPanel::OpenSelection:
						getMode()->getSettingHelper()->emptySlotSV( slot->id , SLOT_OPEN );
						break;
					case PlayerListPanel::CloseSelection:
						getMode()->getSettingHelper()->emptySlotSV( slot->id , SLOT_CLOSE );
						break;
					}
				}
			}
			return false;
		}
		break;
	case UI_NET_ROOM_START: // Server
		if (getMode()->startGame())
		{
			// UI 更新
			mReadyButton->enable(false);
			mExitButton->enable(false);
		}
		return false;

	case UI_NET_ROOM_READY: //Client
		{
			GButton* button = GUI::CastFast< GButton >( ui );
			assert( button == mReadyButton );
			if ( getWorker()->getActionState() != NAS_ROOM_READY )
			{
				getWorker()->changeState( NAS_ROOM_READY );
				button->setTitle( LOCTEXT("Cancel Ready") );
			}
			else
			{
				getWorker()->changeState( NAS_ROOM_WAIT );
				button->setTitle( LOCTEXT("Ready") );
			}	
		}
		return false;
	case UI_MAIN_MENU:
		getMode()->disconnect();
		getManager()->changeStage( STAGE_MAIN_MENU );
		return false;
	case UI_DISCONNECT_MSGBOX:
		getManager()->changeStage( STAGE_MAIN_MENU );
		return false;
	}

	return BaseClass::onWidgetEvent( event , id , ui );
}

void NetRoomStage::onServerEvent( ClientListener::EventID event , unsigned msg )
{
	if ( mConnectPanel )
	{
		mConnectPanel->onServerEvent( event , msg );
	}

	switch( event )
	{
	case ClientListener::eCON_CLOSE:
		getManager()->changeStage( STAGE_MAIN_MENU );
		break;
	case ClientListener::eLOGIN_RESULT:
		if ( msg )
		{
			getWorker()->changeState(NAS_ROOM_WAIT);
			setupUI(true);
		}
		break;
	}
	
}

void NetRoomStage::onUpdate(GameTimeSpan deltaTime)
{
	if (haveServer())
	{
		static bool bFired = false;
		if (InputManager::Get().isKeyDown(EKeyCode::X))
		{
			if ( !bFired )
			{
				bFired = true;
				CSPMsg com;
				com.type = CSPMsg::eSERVER;
				com.content = "UDP Send Test";
				getServer()->sendUdpCommand(&com);
			}
		}
		else
		{
			bFired = false;
		}
	}
	BaseClass::onUpdate(deltaTime);
	if ( mNeedSendSetting && SystemPlatform::GetTickCount() - mLastSendSetting > 500 )
	{
		getMode()->sendGameSetting();
		mNeedSendSetting = false;
	}
	::Global::GUI().updateFrame( long(deltaTime) / gDefaultTickTime , gDefaultTickTime );
}

void NetRoomStage::procPlayerState( IComPacket* cp )
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();

	switch( com->state )
	{
	case NAS_ROOM_ENTER:
		getWorker()->changeState( NAS_ROOM_WAIT );
		break;
	case NAS_LEVEL_SETUP:
		{
			// ✅ 只處理 UI 更新
			mReadyButton->enable( false );
			mExitButton->enable( false );
		}
		break;
	case NAS_ROOM_READY:
		{
			GamePlayer* player = getWorker()->getPlayerManager()->getPlayer( com->playerId );
			if (player)
			{
				PlayerListPanel::Slot& slot = mPlayerPanel->getSlot( player->getSlot() );
				slot.choice->setColorName( EColor::Green );
			}
		}
		break;
	case NAS_ROOM_WAIT:
		{
			GamePlayer* player = getWorker()->getPlayerManager()->getPlayer( com->playerId );
			if (player)
			{
				PlayerListPanel::Slot& slot = mPlayerPanel->getSlot( player->getSlot() );
				slot.choice->setColorName( EColor::Blue );
			}
		}
		break;
	case NAS_CONNECT:
		{
			GamePlayer* player = getWorker()->getPlayerManager()->getPlayer( com->playerId );
			if (player)
			{
				InlineString< 64 > str;
				str.format( LOCTEXT("== %s Inter Room =="), player->getName() );
				mMsgPanel->addMessage( str , Color3ub( 255 , 125 , 0 ) );
			}
		}
		break;
	case NAS_DISSCONNECT:
		if ( com->playerId == SERVER_PLAYER_ID )
		{
			::Global::GUI().showMessageBox( 
				UI_DISCONNECT_MSGBOX , 
				LOCTEXT("Server shout down") , EMessageButton::Ok );
		}
		else if ( getWorker()->getPlayerManager()->getUserID() == com->playerId )
		{
			::Global::GUI().showMessageBox( 
				UI_DISCONNECT_MSGBOX , 
				LOCTEXT("You are kicked by Server") , EMessageButton::Ok );
		}
		else
		{
			GamePlayer* player = getWorker()->getPlayerManager()->getPlayer( com->playerId );
			if ( player )
			{
				PlayerListPanel::Slot& slot = mPlayerPanel->getSlot(player->getSlot());
				slot.choice->setColorName(EColor::Blue);

				{
					InlineString< 64 > str;
					str.format(LOCTEXT("== %s Leave Room =="), player->getName());
					mMsgPanel->addMessage(str, Color3ub(255, 125, 0));
				}
			}
		}
		break;
	}

}

void NetRoomStage::procPlayerStateSv(IComPacket* cp)
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();
	SVPlayerManager* playerMgr = getServer()->getPlayerManager();

	switch( com->state )
	{
	case NAS_ROOM_ENTER:
		{
			getMode()->sendGameSetting( com->playerId );
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

void NetRoomStage::procMsg(IComPacket* cp)
{
	CSPMsg* com = cp->cast< CSPMsg >();

	InlineString< 128 > str;

	switch( com->type )
	{
	case  CSPMsg::eSERVER:
		str.format( "## %s ##" , com->content.c_str() );
		mMsgPanel->addMessage( str , Color3ub( 255 , 0 , 255 ) );
		//CFly::Msg(  "server :" , com->str );
		break;
	case  CSPMsg::ePLAYER:
		{
			GamePlayer* player = getWorker()->getPlayerManager()->getPlayer( com->playerID );

			if ( !player )
				return;
			str.format( "%s : %s " , player->getName() , com->content.c_str() );
			mMsgPanel->addMessage( str , Color3ub( 255 , 255 , 0 ) );
			//CFly::Msg( "( ID = %d ):%s" , com->playerID , com->str );
		}
		break;
	}
}


void NetRoomStage::procPlayerStatus(IComPacket* cp)
{
	SPPlayerStatus* com = cp->cast< SPPlayerStatus >();

	mPlayerPanel->setupPlayerList(*com);
}

void NetRoomStage::procSlotState(IComPacket* cp)
{
	SPSlotState* com = cp->cast< SPSlotState >();
	for( int i = 0 ; i < MAX_PLAYER_NUM ; ++i )
	{
		if ( com->state[i] >= 0 )
		{
			GamePlayer* player = getWorker()->getPlayerManager()->getPlayer( PlayerId( com->state[i] ) );
			if ( player )
				player->setSlot( i );
		}
	}

	mPlayerPanel->refreshPlayerList(com->idx, com->state);
}

void NetRoomStage::procRawData(IComPacket* cp)
{
	CSPRawData* com = cp->cast< CSPRawData >();

	switch( com->id )
	{
	case NetGameMode::SETTING_DATA_ID:
		// ✅ Client 接收遊戲設定 - 使用統一的 switchGame 接口
		if ( !haveServer() )
		{
			try
			{
				char gameName[128];
				com->buffer.take(gameName, sizeof(gameName));
				mSettingPanel->setGame(gameName);  // 更新 UI 選單

				NetGameMode::GameSwitchContext ctx;
				ctx.playerPanel = mPlayerPanel;
				ctx.settingPanel = mSettingPanel;
				ctx.listener = this;
				ctx.importBuffer = &com->buffer;  // 導入設定數據
				
				getMode()->switchGame(gameName, ctx);
				mSettingPanel->adjustChildLayout();
			}
			catch (std::exception& /*e*/)
			{
				LogWarning(0, "Failed to process game settings");
			}
		}
		break;

	}
}

// ✅ 舊方法已搬移到 NetGameMode
// generateSetting, setupGame, sendGameSetting, onAddPlayer, onRemovePlayer

void NetRoomStage::onModify( GWidget* ui )
{
	if ( ui->getID() == UI_GAME_SETTING_PANEL )
	{
		int64 const MinSendSettingTime = 250;
		if ( haveServer() && mLastSendSetting - SystemPlatform::GetTickCount() > MinSendSettingTime )
		{
			getMode()->sendGameSetting();
		}
		else
		{
			mNeedSendSetting = true;
		}
	}
}


bool NetRoomStage::taskDestroyServerListPanelCL( long time )
{
	if ( getWorker()->getActionState() == NAS_ROOM_WAIT )
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


void NetGameMode::initWorker( ComWorker* worker , ServerWorker* server /*= NULL */ )
{
	mWorker = worker;
	mServer = server;
	registerNetEvent();

	if (haveServer())
	{
		mServer->setEventResolver(this);
		mServer->getPlayerManager()->setListener(this);  // ✅ 註冊為 PlayerListener
	}

}

void NetGameMode::unregisterNetEvent( void* processor )
{
	mWorker->removeProcesserFunc( processor );
	if ( mServer )
	{
		mServer->removeProcesserFunc( processor );
	}
	else
	{
		static_cast< ClientWorker* >( mWorker )->setClientListener( NULL );
	}
}

void NetGameMode::registerNetEvent()
{
	if (mWorker)
	{
		setupWorkerProcFunc(mWorker->getPacketDispatcher());
	}

	if ( haveServer() )
	{
		setupServerProcFunc( mServer->getPacketDispatcher() );
	}
	else
	{
		static_cast< ClientWorker* >( mWorker )->setClientListener( this );
	}
}

void NetGameMode::disconnect()
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


NetGameMode::NetGameMode() 
	:GameLevelMode(EGameMode::Net)
{
	mWorker = nullptr;
	mServer = nullptr;
	bCloseNetWork = false;

	mNetEngine = &gEmptyNetEngine;
	mbReconnectMode = false;
	mbLevelInitialized = false;

	mMsgPanel = nullptr;
}

NetGameMode::~NetGameMode()
{
	if( bCloseNetWork )
	{
		::Global::GameNet().closeNetwork();
	}
}

bool NetGameMode::initializeStage(GameStageBase* stage)
{

	if (!stage->onInit())
		return false;

	mUpdater = stage;

	::Global::GUI().hideWidgets(true);
	::Global::GUI().cleanupWidget();

	mMsgPanel = new ComMsgPanel(UI_ANY, Vec2i(0, 0), Vec2i(PlayerListPanel::WidgetSize.x, 230), NULL);
	mMsgPanel->setWorker(mWorker);
	mMsgPanel->show(false);
	::Global::GUI().addWidget(mMsgPanel);

	if (haveServer())
	{
		// ✅ Helper 會在 NetRoomStage::onInit 中透過 mSettingPanel->setGame() 自動創建
		// 不需要在這裡預先創建，避免重複創建和初始化時機問題
	}
	else if (!getClientWorker()->haveConnect())
	{
		mNetEngine = &gEmptyNetEngine;

		getClientWorker()->setClientListener(this);

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


void NetGameMode::onEnd()
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
		mServer->getPlayerManager()->setListener(nullptr);  // ✅ 取消 PlayerListener 註冊
	}
	BaseClass::onEnd();
}

bool NetGameMode::onWidgetEvent(int event, int id, GWidget* ui)
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
			if( haveServer() && getGameState() == EGameState::End )
			{
				mServer->changeState(NAS_ROOM_ENTER);
			}
			return false;
		}
		else
		{
			if( haveServer() )
			{
				if( getGameState() == EGameState::End )
				{
					mServer->changeState(NAS_LEVEL_RESTART);
				}
				else
				{
					::Global::GUI().showMessageBox(
						UI_RESTART_GAME, LOCTEXT("Do you Want to Stop Current Game?"), EMessageButton::YesNo);
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
			::Global::GUI().showMessageBox(id, LOCTEXT("Be Sure Exit Game"), EMessageButton::YesNo);
			return false;
		}
		break;

	}
	return true;
}

MsgReply NetGameMode::onKey(KeyMsg const& msg)
{
	if(msg.isDown() && msg.getCode() == EKeyCode::Tab )
	{
		bool beShow = !mMsgPanel->isShow();
		mMsgPanel->show(beShow);
		if( beShow )
		{
			mMsgPanel->clearInputString();
			mMsgPanel->makeFocus();
		}
		getGame()->getInputControl().blockAllAction(beShow);
		return MsgReply::Handled();
	}

	return BaseClass::onKey(msg);
}

void NetGameMode::onRestart(uint64& seed)
{
	//FIXME
	seed = mNetSeed;
	BaseClass::onRestart(seed);
}

bool NetGameMode::prevChangeState(EGameState state)
{
	switch( getGameState() )
	{
	case EGameState::Start:
		if( mWorker->getActionState() != NAS_LEVEL_RUN )
			return false;
	}
	return true;
}

IPlayerManager* NetGameMode::getPlayerManager()
{
	return mWorker->getPlayerManager();
}

void NetGameMode::updateFrame(int frame)
{
	mUpdater->updateFrame(frame);
}

void NetGameMode::tick()
{
	ActionProcessor& actionProcessor = getStage()->getActionProcessor();

	unsigned flag = 0;
	switch( getGameState() )
	{
	case EGameState::Run:
		++mReplayFrame;
		break;
	default:
		flag |= CTF_FREEZE_FRAME;
	}
	actionProcessor.beginAction(flag);
	mUpdater->tick();
	actionProcessor.endAction();
}

void NetGameMode::setupServerProcFunc(PacketDispatcher& dispatcher)
{
#define DEFINE_CP_USER_FUNC( Class , Func )\
		dispatcher.setUserFunc< Class >( this , &NetGameMode::Func );


#undef  DEFINE_CP_USER_FUNC
}

void NetGameMode::setupWorkerProcFunc(PacketDispatcher& dispatcher)
{
#define DEFINE_CP_USER_FUNC( Class , Func )\
		dispatcher.setUserFunc< Class >( this , &NetGameMode::Func );

	DEFINE_CP_USER_FUNC(CSPPlayerState, procPlayerState);
	DEFINE_CP_USER_FUNC(CSPMsg, procMsg);
	DEFINE_CP_USER_FUNC(SPPlayerStatus, procPlayerStatus);
	DEFINE_CP_USER_FUNC(SPSlotState, procSlotState);
	DEFINE_CP_USER_FUNC(CSPRawData, procRawData);
	DEFINE_CP_USER_FUNC(SPLevelInfo, procLevelInfo);


#undef  DEFINE_CP_USER_FUNC
}

void NetGameMode::resolveChangeActionState(NetActionState state)
{
	switch (state)
	{
		
	case NAS_LEVEL_LOAD:
		{			
			SPLevelInfo info;
			if (getStage())
			{
				getStage()->configLevelSetting(info);
			}
			else if (mNetEngine)
			{
				// Dedicated server mode: get level info from NetEngine
				mNetEngine->configLevelSetting(info);
			}
			mServer->sendTcpCommand(&info);
		}
		break;
		
	default:
		break;
	}
}

void NetGameMode::procPlayerState(IComPacket* cp)
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();

	switch( com->state )
	{
	case NAS_ROOM_ENTER:
		{
			NetRoomStage* stage = static_cast<NetRoomStage*>( getManager()->changeStage(STAGE_NET_ROOM));
			// stage->initWorker(mWorker, mServer); // Worker initialization moved to NetGameMode
		}
		break;
	case NAS_LEVEL_SETUP:
		{
			// Reset level initialization flag when starting a new level session
			// This ensures that procLevelInfo will process the level loading
			LogMsg("[NetGameMode::procPlayerState] NAS_LEVEL_SETUP - Calling transitionToLevel()");
			transitionToLevel();
		}
		break;
	case NAS_CONNECT:
		// Server-side handling is in procPlayerStateSv
		break;
	case NAS_LEVEL_LOAD:
		// Server-side handling is in procPlayerStateSv
		// Client-side: nothing to do here, just wait for SPLevelInfo
		break;
	case NAS_LEVEL_PAUSE:
		if( com->playerId != SERVER_PLAYER_ID )
		{
			if( haveServer() || (mWorker && com->playerId == mWorker->getPlayerManager()->getUserID()) )
			{
				::Global::GUI().showMessageBox(
					UI_UNPAUSE_GAME, LOCTEXT("Stop Game. Click OK to Continue Game."), EMessageButton::Ok);
			}
			else
			{
				GamePlayer* player = mWorker->getPlayerManager()->getPlayer(com->playerId);
				InlineString< 256 > str;
				str.format(LOCTEXT("%s Puase Game"), player->getName());
				::Global::GUI().showMessageBox(
					UI_UNPAUSE_GAME, str, EMessageButton::None);
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
			if (mWorker)
			{
				mWorker->changeState(NAS_LEVEL_RUN);
			}
		}
		break;
	case NAS_LEVEL_INIT:
		{
			doRestart(true);
			::Global::GUI().hideWidgets(false);
			if (mWorker)
			{
				mWorker->changeState(NAS_LEVEL_INIT);
			}
		}
		break;
	case NAS_LEVEL_RESTART:
		{
			if( mNetEngine )
				mNetEngine->restart();
			doRestart(false);
			if (mWorker)
			{
				mWorker->changeState(NAS_LEVEL_RESTART);
			}
		}
		break;
	case NAS_DISSCONNECT:
		if( com->playerId == SERVER_PLAYER_ID ||
			(mWorker && com->playerId == mWorker->getPlayerManager()->getUserID()) )
		{
			if( getStage() )
			{
				getStage()->getManager()->changeStage(STAGE_MAIN_MENU);
			}
		}
		break;
	}
}

void NetGameMode::procLevelInfo(IComPacket* cp)
{
	SPLevelInfo* com = cp->cast< SPLevelInfo >();
	LogMsg("[NetGameMode::procLevelInfo] Entering, isServer=%d, mbLevelInitialized=%d", 
		haveServer() ? 1 : 0, mbLevelInitialized ? 1 : 0);
	
	// Guard: Only load level once. This can be called multiple times due to network state synchronization
	// but we should only initialize the level once per game session
	if (mbLevelInitialized)
	{
		LogMsg("[NetGameMode::procLevelInfo] Level already initialized, skipping duplicate call");
		return;
	}
	
	if( !loadLevel(*com) )
	{
		mWorker->changeState(NAS_LEVEL_LOAD_FAIL);
		return;
	}
	
	mbLevelInitialized = true;
	mWorker->changeState(NAS_LEVEL_LOAD);
}

void NetGameMode::updateTime(GameTimeSpan deltaTime)
{
	BaseClass::updateTime(deltaTime);
	mNetEngine->update(*this, long(deltaTime));
	if (getGame())
	{
		::Global::GUI().scanHotkey(getGame()->getInputControl());
	}
}

bool NetGameMode::canRender()
{
	if( getWorker()->getActionState() == NAS_LEVEL_LOAD ||
	    getWorker()->getActionState() == NAS_LEVEL_SETUP )
		return false;
	return true;
}

void NetGameMode::procMsg(IComPacket* cp)
{
	CSPMsg* com = cp->cast< CSPMsg >();

	InlineString< 128 > str;

	switch( com->type )
	{
	case CSPMsg::eSERVER:
		if (mMsgPanel)
		{
			str.format("## %s ##", com->content.c_str());
			mMsgPanel->addMessage(str, Color3ub(255, 0, 255));
		}
		//::Msg(  "server :" , com->str );
		break;
	case CSPMsg::ePLAYER:
		{
			GamePlayer* player = mWorker->getPlayerManager()->getPlayer(com->playerID);

			if( !player )
				return;

			if (mMsgPanel)
			{
				str.format("%s : %s ", player->getName(), com->content.c_str());
				mMsgPanel->addMessage(str, Color3ub(255, 255, 0));
			}
			//::Msg( "( ID = %d ):%s" , com->playerID , com->str );
		}
		break;
	}
}

void NetGameMode::onServerEvent(ClientListener::EventID event, unsigned msg)
{
	InlineString< 256 > str;

	switch( event )
	{
	case ClientListener::eCON_CLOSE:
		str.format("Lost Server %s", "");
		::Global::GUI().showMessageBox(UI_ANY, str, EMessageButton::Ok);
		break;
	case ClientListener::eLOGIN_RESULT:
		break;
	}
}

bool NetGameMode::buildNetEngine()
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

bool NetGameMode::loadLevel(GameLevelInfo const& info)
{
	IPlayerManager* playerMgr = getPlayerManager();

	mNetSeed = info.seed;
	getStage()->setupLevel(info);
	getStage()->setupScene(*playerMgr);

	if( haveServer() )
		mNetEngine->setupInputAI(*playerMgr, getActionProcessor());

	if( !buildNetEngine() )
	{
		return false;
	}

	if( !buildReplayRecorder() )
	{

	}

	return true;
}

PlayerConnetionClosedAction NetGameMode::resolveConnectClosed_NetThread(ServerResolveContext& context, NetCloseReason reason)
{
	assert(IsInNetThread());
	return PlayerConnetionClosedAction::Remove;
}

void NetGameMode::resolveReconnect_NetThread(ServerResolveContext& context)
{
	assert(IsInNetThread());
}

// === ServerPlayerListener 實作 ===
void NetGameMode::onAddPlayer(PlayerId id)
{
	sendGameSetting(id);
	if (mHelper)
	{
		if (!mHelper->addPlayerSV(id))
		{
			// Handle error
		}
	}
}

void NetGameMode::onRemovePlayer(PlayerInfo const& info)
{
	if (mHelper)
	{
		mHelper->emptySlotSV(info.slot, SLOT_OPEN);
	}
}


void NetGameMode::setupGame(char const* gameName)
{
	IGameModule* game = Global::ModuleManager().changeGame(gameName);

	if (game)
	{
		SettingHepler* helper = game->createSettingHelper(SHT_NET_ROOM_HELPER);
		assert(dynamic_cast<NetRoomSettingHelper*>(helper));
		if (helper)
		{
			mHelper.reset(static_cast<NetRoomSettingHelper*>(helper));
		}
	}
	else
	{
		LogWarning(0, "Can't Setup Game");
	}
}

void NetGameMode::generateGameSetting(CSPRawData& com)
{
	com.id = SETTING_DATA_ID;
	DataStreamBuffer& buffer = com.buffer;

	buffer.fill(Global::ModuleManager().getRunningGame()->getName());
	if (mHelper)
	{
		mHelper->exportSetting(buffer);
	}
}

void NetGameMode::sendGameSetting(unsigned pID)
{
	CSPRawData com;
	generateGameSetting(com);
	if (pID == SERVER_PLAYER_ID)
	{
		getServer()->sendTcpCommand(&com);
	}
	else
	{
		ServerPlayer* player = getServer()->getPlayerManager()->getPlayer(pID);
		if (player)
		{
			player->sendTcpCommand(&com);
		}
	}
}

void NetGameMode::switchGame(char const* gameName, GameSwitchContext const& ctx)
{
	// 1. Clear old Helper's UI
	if (mHelper)
	{
		mHelper->clearUserUI();
	}
	
	// 2. Create new Helper
	setupGame(gameName);
	
	// 3. Bind UI to new Helper
	bindHelperToUI(ctx);
	
	// 4. Import settings (Client side)
	if (ctx.importBuffer && mHelper)
	{
		mHelper->importSetting(*ctx.importBuffer);
	}
	
	// 5. Send settings to clients (Server side)
	if (ctx.sendToClients && haveServer())
	{
		sendGameSetting();
	}
}

void NetGameMode::bindHelperToUI(GameSwitchContext const& ctx)
{
	if (!mHelper)
		return;
		
	if (ctx.playerPanel)
		mHelper->addGUIControl(ctx.playerPanel);
	if (ctx.settingPanel)
		mHelper->addGUIControl(ctx.settingPanel);
	if (ctx.listener)
		mHelper->setListener(ctx.listener);
		
	mHelper->setupSetting(getServer());
}

// === 遊戲啟動 ===
bool NetGameMode::startGame()
{
	if (!mHelper || !mHelper->checkSettingSV())
		return false;

	assert(haveServer());

	// 發送 Server 準備好的狀態
	CSPPlayerState com;
	com.playerId = getServer()->getPlayerManager()->getUserID();
	com.state = NAS_ROOM_READY;
	getServer()->sendTcpCommand(&com);

	// 同步玩家狀態和設定
	mHelper->sendPlayerStatusSV();
	sendGameSetting();

	// 啟動倒數計時
	scheduleGameStart();
	return true;
}

void NetGameMode::scheduleGameStart()
{
	GameStartTask* task = new GameStartTask;
	task->server = getServer();
	getManager()->addTask(task);
}

void NetGameMode::transitionToLevel()
{
	assert(Global::ModuleManager().getRunningGame());

	if (getWorker())
	{
		Global::ModuleManager().getRunningGame()->beginPlay(*getManager(), EGameMode::Net);
		StageBase* nextStage = getManager()->getNextStage();
		if (nextStage)
		{
			if (mHelper)
			{
				mHelper->setupGame(*getManager(), nextStage);
			}
			if (haveServer() && mHelper)
			{
				mHelper->sendPlayerStatusSV();
			}

			getWorker()->changeState(NAS_LEVEL_SETUP);
			// Reset level initialization flag
			mbLevelInitialized = false;
		}
		else
		{
			LogMsg("No NetLevel!");
		}
	}
}

// === Lobby 封包處理 (業務邏輯) ===
void NetGameMode::procPlayerStatus(IComPacket* cp)
{
	// SPPlayerStatus 通常由 Server 發送給 Client，Mode 不需要處理
	// NetRoomStage 會處理 UI 更新
}

void NetGameMode::procSlotState(IComPacket* cp)
{
	SPSlotState* com = cp->cast<SPSlotState>();
	// 更新玩家 Slot 分配
	for (int i = 0; i < MAX_PLAYER_NUM; ++i)
	{
		if (com->state[i] >= 0)
		{
			GamePlayer* player = getWorker()->getPlayerManager()->getPlayer(PlayerId(com->state[i]));
			if (player)
				player->setSlot(i);
		}
	}
}

void NetGameMode::procRawData(IComPacket* cp)
{
	CSPRawData* com = cp->cast<CSPRawData>();

	switch (com->id)
	{
	case SETTING_DATA_ID:
		// ✅ Client 端的遊戲設定導入由 NetRoomStage::procRawData 處理
		// 因為需要連接 UI panels 後才能安全地調用 importSetting()
		// NetGameMode 這裡不處理 Client 端邏輯
		break;
	}
}

