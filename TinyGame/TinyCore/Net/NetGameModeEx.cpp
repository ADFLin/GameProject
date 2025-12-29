#include "TinyGamePCH.h"
#include "NetGameModeEx.h"

#include "GameModuleManager.h"
#include "GameNetPacket.h"
#include "GameStage.h"

#include "GameGUISystem.h"
#include "GameWidgetID.h"
#include "GameSettingPanel.h"
#include "Widget/ServerListPanel.h"
#include "GameSettingHelper.h"

#include "Net/NetSystemManager.h"
#include "Net/IGameNetSession.h"

#include "INetEngine.h"
#include "EasingFunction.h"

//========================================
// NetRoomStageEx
//========================================

NetRoomStageEx::NetRoomStageEx()
{
}

NetRoomStageEx::~NetRoomStageEx()
{
}

void NetRoomStageEx::initWithWorker(ComWorker* worker, ServerWorker* server)
{
	mNetAdapter.initFromWorker(worker, server);
	mHostSession = nullptr;
	mClientSession = nullptr;
}

void NetRoomStageEx::initWithSession(INetSessionHost* hostSession)
{
	mHostSession = hostSession;
	mClientSession = nullptr;
	mNetAdapter.initFromSession(hostSession);
}

void NetRoomStageEx::initWithSession(INetSessionClient* clientSession)
{
	mHostSession = nullptr;
	mClientSession = clientSession;
	mNetAdapter.initFromSession(clientSession);
}

bool NetRoomStageEx::onInit()
{
	if (!BaseClass::onInit())
		return false;
	
	getManager()->setTickTime(gDefaultTickTime);
	
	mLastSendSetting = 0;
	
	::Global::GUI().cleanupWidget();
	
	setupUI(isHost());
	setupPacketHandlers();
	
	if (isHost())
	{
		// Server 端：更新玩家列表
		updatePlayerPanel();
		
		IGameModule* curGame = Global::ModuleManager().getRunningGame();
		if (mSettingPanel)
		{
			mSettingPanel->setGame(curGame ? curGame->getName() : nullptr);
		}
	}
	else
	{
		// Client 端：如果未連線，顯示伺服器列表
		if (mClientSession)
		{
			if (mClientSession->getState() == ENetSessionState::Disconnected)
			{
				// 顯示連線面板
				// mConnectPanel = new ServerListPanel(...);
			}
		}
	}
	
	mNetAdapter.setSessionState(ENetSessionState::InRoom);
	
	LogMsg("NetRoomStageEx initialized");
	return true;
}

void NetRoomStageEx::onEnd()
{
	// 清理封包處理器
	// TODO: unregister packet handlers
	
	if (isHost() && mHostSession)
	{
		// mHostSession->setEventHandler(nullptr);
	}
	
	BaseClass::onEnd();
}

void NetRoomStageEx::onRender(float dFrame)
{
	// 渲染房間 UI
}

bool NetRoomStageEx::onWidgetEvent(int event, int id, GWidget* ui)
{
	switch (id)
	{
	case UI_GAME_CHOICE:
		setupGame(static_cast<GChoice*>(ui)->getSelectValue());
		if (isHost())
		{
			sendGameSetting();
		}
		return false;
		
	case UI_NET_ROOM_START:  // Server 開始遊戲
		if (isHost())
		{
			startGame();
		}
		return false;
		
	case UI_NET_ROOM_READY:  // Client 準備
		if (mClientSession)
		{
			bool isReady = mClientSession->isReady();
			mClientSession->setReady(!isReady);
			
			if (mReadyButton)
			{
				mReadyButton->setTitle(isReady ? LOCTEXT("Ready") : LOCTEXT("Cancel Ready"));
			}
		}
		return false;
		
	case UI_MAIN_MENU:
		// 離開房間
		if (mHostSession)
		{
			mHostSession->closeRoom();
		}
		else if (mClientSession)
		{
			mClientSession->leaveServer();
		}
		getManager()->changeStage(STAGE_MAIN_MENU);
		return false;
		
	case UI_DISCONNECT_MSGBOX:
		getManager()->changeStage(STAGE_MAIN_MENU);
		return false;
	}
	
	return BaseClass::onWidgetEvent(event, id, ui);
}

void NetRoomStageEx::onUpdate(GameTimeSpan deltaTime)
{
	BaseClass::onUpdate(deltaTime);
	
	// 定期發送設定
	if (mNeedSendSetting && SystemPlatform::GetTickCount() - mLastSendSetting > 500)
	{
		sendGameSetting();
		mNeedSendSetting = false;
	}
	
	::Global::GUI().updateFrame(long(deltaTime) / gDefaultTickTime, gDefaultTickTime);
}

void NetRoomStageEx::onModify(GWidget* ui)
{
	if (ui->getID() == UI_GAME_SETTING_PANEL)
	{
		int64 const MinSendSettingTime = 250;
		if (isHost() && SystemPlatform::GetTickCount() - mLastSendSetting > MinSendSettingTime)
		{
			sendGameSetting();
		}
		else
		{
			mNeedSendSetting = true;
		}
	}
}

void NetRoomStageEx::onSessionEvent(ENetSessionEvent event, PlayerId playerId)
{
	switch (event)
	{
	case ENetSessionEvent::ConnectionLost:
		::Global::GUI().showMessageBox(
			UI_DISCONNECT_MSGBOX,
			LOCTEXT("Lost connection to server"),
			EMessageButton::Ok);
		break;
		
	case ENetSessionEvent::PlayerJoined:
		updatePlayerPanel();
		if (mMsgPanel)
		{
			NetPlayerInfo const* info = mNetAdapter.getPlayerManager() 
				? nullptr : nullptr;  // TODO: get player info
			InlineString<64> str;
			str.format(LOCTEXT("== Player joined =="));
			mMsgPanel->addMessage(str, Color3ub(255, 125, 0));
		}
		break;
		
	case ENetSessionEvent::PlayerLeft:
		updatePlayerPanel();
		break;
		
	case ENetSessionEvent::LevelStarting:
		onGameStarting();
		break;
	}
}

void NetRoomStageEx::setupUI(bool bFullSetting)
{
	typedef Easing::OQuart EasingFunc;
	long const timeUIAnim = 1000;
	Vec2i panelPos = Vec2i(20, 20);
	
	// 玩家列表面板
	if (mPlayerPanel == nullptr)
	{
		mPlayerPanel = new PlayerListPanel(
			getPlayerManager(), UI_PLAYER_LIST_PANEL, panelPos, nullptr);
		mPlayerPanel->init(isHost());
		::Global::GUI().addWidget(mPlayerPanel);
	}
	
	if (bFullSetting)
	{
		mPlayerPanel->show(true);
		Vec2i from = Vec2i(-mPlayerPanel->getSize().x, panelPos.y);
		mPlayerPanel->setPos(from);
		::Global::GUI().addMotion<EasingFunc>(mPlayerPanel, from, panelPos, timeUIAnim);
	}
	else
	{
		mPlayerPanel->show(false);
	}
	
	// 設定面板
	Vec2i sizeSettingPanel(350, 400);
	Vec2i settingPos = panelPos + Vec2i(PlayerListPanel::WidgetSize.x + 10, 0);
	
	if (mSettingPanel == nullptr)
	{
		mSettingPanel = new GameSettingPanel(
			UI_GAME_SETTING_PANEL, settingPos, sizeSettingPanel, nullptr);
		
		if (!isHost())
		{
			mSettingPanel->enable(false);
		}
		::Global::GUI().addWidget(mSettingPanel);
		
		// 添加遊戲列表
		GameModuleVec gameVec;
		Global::ModuleManager().classifyGame(ATTR_NET_SUPPORT, gameVec);
		for (IGameModule* game : gameVec)
		{
			mSettingPanel->addGame(game);
		}
	}
	
	if (bFullSetting)
	{
		mSettingPanel->show(true);
		Vec2i from = Vec2i(Global::GetScreenSize().x, settingPos.y);
		::Global::GUI().addMotion<EasingFunc>(mSettingPanel, from, settingPos, timeUIAnim);
	}
	else
	{
		mSettingPanel->show(false);
	}
	
	// 訊息面板
	Vec2i msgPos = panelPos + Vec2i(0, PlayerListPanel::WidgetSize.y + 20);
	if (mMsgPanel == nullptr)
	{
		mMsgPanel = new ComMsgPanel(UI_ANY, msgPos, Vec2i(PlayerListPanel::WidgetSize.x, 230), nullptr);
		mMsgPanel->setWorker(mNetAdapter.getWorker());
		::Global::GUI().addWidget(mMsgPanel);
	}
	
	if (bFullSetting)
	{
		mMsgPanel->show(true);
		Vec2i from = Vec2i(-mMsgPanel->getSize().x, msgPos.y);
		::Global::GUI().addMotion<EasingFunc>(mMsgPanel, from, msgPos, timeUIAnim);
	}
	else
	{
		mMsgPanel->show(false);
	}
	
	// 按鈕
	if (bFullSetting)
	{
		Vec2i btnSize(sizeSettingPanel.x, 30);
		Vec2i btnPos(panelPos.x + PlayerListPanel::WidgetSize.x + 10, 
		             panelPos.y + sizeSettingPanel.y + 10);
		
		GButton* button;
		if (isHost())
		{
			button = new GButton(UI_NET_ROOM_START, btnPos, btnSize, nullptr);
			button->setTitle(LOCTEXT("Start Game"));
		}
		else
		{
			button = new GButton(UI_NET_ROOM_READY, btnPos, btnSize, nullptr);
			button->setTitle(LOCTEXT("Ready"));
		}
		
		Vec2i from = Vec2i(Global::GetScreenSize().x, btnPos.y);
		::Global::GUI().addMotion<EasingFunc>(button, from, btnPos, timeUIAnim);
		
		button->setFontType(FONT_S10);
		mReadyButton = button;
		::Global::GUI().addWidget(button);
		
		// 離開按鈕
		btnPos += Vec2i(0, btnSize.y + 5);
		button = new GButton(UI_MAIN_MENU, btnPos, btnSize, nullptr);
		button->setFontType(FONT_S10);
		button->setTitle(LOCTEXT("Exit"));
		::Global::GUI().addWidget(button);
		mExitButton = button;
		
		from = Vec2i(Global::GetScreenSize().x, btnPos.y);
		::Global::GUI().addMotion<EasingFunc>(button, from, btnPos, timeUIAnim);
	}
}

void NetRoomStageEx::updatePlayerPanel()
{
	if (!mPlayerPanel)
		return;
	
	// 從會話層取得玩家列表並更新 UI
	// TODO: 實作玩家列表更新
}

void NetRoomStageEx::setupPacketHandlers()
{
	// 註冊封包處理器
	// TODO: 使用新架構的封包註冊
}

void NetRoomStageEx::procPlayerState(IComPacket* cp)
{
	CSPPlayerState* com = cp->cast<CSPPlayerState>();
	
	switch (com->state)
	{
	case NAS_LEVEL_SETUP:
		onGameStarting();
		break;
		
	case NAS_ROOM_READY:
	case NAS_ROOM_WAIT:
		updatePlayerPanel();
		break;
	}
}

void NetRoomStageEx::procPlayerStateSv(IComPacket* cp)
{
	// Server 端處理玩家狀態
}

void NetRoomStageEx::procMsg(IComPacket* cp)
{
	CSPMsg* com = cp->cast<CSPMsg>();
	
	if (mMsgPanel)
	{
		InlineString<128> str;
		switch (com->type)
		{
		case CSPMsg::eSERVER:
			str.format("## %s ##", com->content.c_str());
			mMsgPanel->addMessage(str, Color3ub(255, 0, 255));
			break;
		case CSPMsg::ePLAYER:
			{
				GamePlayer* player = getPlayerManager()->getPlayer(com->playerID);
				if (player)
				{
					str.format("%s : %s", player->getName(), com->content.c_str());
					mMsgPanel->addMessage(str, Color3ub(255, 255, 0));
				}
			}
			break;
		}
	}
}

void NetRoomStageEx::procPlayerStatus(IComPacket* cp)
{
	SPPlayerStatus* com = cp->cast<SPPlayerStatus>();
	if (mPlayerPanel)
	{
		mPlayerPanel->setupPlayerList(*com);
	}
}

void NetRoomStageEx::procSlotState(IComPacket* cp)
{
	// 處理槽位狀態
}

void NetRoomStageEx::procRawData(IComPacket* cp)
{
	// 處理原始數據（遊戲設定等）
}

void NetRoomStageEx::setupGame(char const* name)
{
	IGameModule* game = Global::ModuleManager().changeGame(name);
	
	if (game)
	{
		SettingHepler* helper = game->createSettingHelper(SHT_NET_ROOM_HELPER);
		if (helper)
		{
			if (mHelper)
			{
				mHelper->clearUserUI();
				if (mSettingPanel)
				{
					mSettingPanel->adjustChildLayout();
				}
			}
			
			mHelper.reset(static_cast<NetRoomSettingHelper*>(helper));
			
			mHelper->addGUIControl(mPlayerPanel);
			mHelper->addGUIControl(mSettingPanel);
			mHelper->setListener(this);
			mHelper->setupSetting(mNetAdapter.getServer());
		}
	}
	else
	{
		LogWarning(0, "Can't setup game: %s", name);
	}
}

void NetRoomStageEx::generateSetting(CSPRawData& com)
{
	// 生成遊戲設定數據
}

void NetRoomStageEx::sendGameSetting(PlayerId targetId)
{
	CSPRawData com;
	generateSetting(com);
	
	if (targetId == SERVER_PLAYER_ID)
	{
		mNetAdapter.broadcastReliable(&com);
	}
	else
	{
		mNetAdapter.sendReliableTo(targetId, &com);
	}
	
	mLastSendSetting = SystemPlatform::GetTickCount();
}

void NetRoomStageEx::startGame()
{
	if (!isHost())
		return;
	
	// 檢查設定
	if (mHelper && !mHelper->checkSettingSV())
		return;
	
	// 廣播開始遊戲
	CSPPlayerState com;
	com.setServerState(NAS_LEVEL_SETUP);
	mNetAdapter.broadcastReliable(&com);
	
	onGameStarting();
}

void NetRoomStageEx::onGameStarting()
{
	if (mReadyButton)
		mReadyButton->enable(false);
	if (mExitButton)
		mExitButton->enable(false);
	
	IGameModule* runningGame = Global::ModuleManager().getRunningGame();
	if (!runningGame)
	{
		LogError("No running game");
		return;
	}
	
	// 開始遊戲
	runningGame->beginPlay(*getManager(), EGameMode::Net);
	
	StageBase* nextStage = getManager()->getNextStage();
	if (nextStage)
	{
		if (auto gameStage = nextStage->getGameStage())
		{
			// 設定下一個階段的網路模式
			// TODO: 傳遞 Session 到 NetLevelStageModeEx
		}
	}
	
	mNetAdapter.setSessionState(ENetSessionState::LevelLoading);
}

//========================================
// NetLevelStageModeEx
//========================================

NetLevelStageModeEx::NetLevelStageModeEx()
	: GameLevelMode(EGameStageMode::Net)
{
}

NetLevelStageModeEx::~NetLevelStageModeEx()
{
}

void NetLevelStageModeEx::initWithWorker(ComWorker* worker, ServerWorker* server)
{
	mNetAdapter.initFromWorker(worker, server);
	mHostSession = nullptr;
	mClientSession = nullptr;
}

void NetLevelStageModeEx::initWithSession(INetSession* session)
{
	mNetAdapter.initFromSession(session);
	mHostSession = dynamic_cast<INetSessionHost*>(session);
	mClientSession = dynamic_cast<INetSessionClient*>(session);
}

bool NetLevelStageModeEx::prevStageInit()
{
	if (!BaseClass::prevStageInit())
		return false;
	
	setupPacketHandlers();
	
	return true;
}

bool NetLevelStageModeEx::postStageInit()
{
	if (!BaseClass::postStageInit())
		return false;
	
	::Global::GUI().hideWidgets(true);
	::Global::GUI().cleanupWidget();
	
	// 訊息面板
	mMsgPanel = new ComMsgPanel(UI_ANY, Vec2i(0, 0), 
	                            Vec2i(PlayerListPanel::WidgetSize.x, 230), nullptr);
	mMsgPanel->setWorker(mNetAdapter.getWorker());
	mMsgPanel->show(false);
	::Global::GUI().addWidget(mMsgPanel);
	
	return true;
}

void NetLevelStageModeEx::onEnd()
{
	if (mNetEngine)
	{
		mNetEngine->close();
		mNetEngine->release();
		mNetEngine = nullptr;
	}
	
	if (mGameSession)
	{
		mGameSession->shutdown();
		mGameSession = nullptr;
	}
	
	BaseClass::onEnd();
}

void NetLevelStageModeEx::updateTime(GameTimeSpan deltaTime)
{
	BaseClass::updateTime(deltaTime);
	
	if (mNetEngine)
	{
		mNetEngine->update(*this, long(deltaTime));
	}
	
	if (mGameSession)
	{
		mGameSession->update(long(deltaTime));
	}
	
	if (getGame())
	{
		::Global::GUI().scanHotkey(getGame()->getInputControl());
	}
}

bool NetLevelStageModeEx::canRender()
{
	ENetSessionState state = mNetAdapter.getSessionState();
	if (state == ENetSessionState::LevelLoading)
		return false;
	return true;
}

void NetLevelStageModeEx::restart(bool beInit)
{
	if (isHost() && mHostSession)
	{
		mHostSession->restartLevel();
	}
}

bool NetLevelStageModeEx::onWidgetEvent(int event, int id, GWidget* ui)
{
	if (!BaseClass::onWidgetEvent(event, id, ui))
		return false;
	
	switch (id)
	{
	case UI_PAUSE_GAME:
		if (isHost() && mHostSession)
		{
			mHostSession->pauseLevel();
		}
		return false;
		
	case UI_UNPAUSE_GAME:
		if (isHost() && mHostSession)
		{
			mHostSession->resumeLevel();
		}
		return false;
		
	case UI_RESTART_GAME:
		if (event == EVT_BOX_YES)
		{
			restart(false);
			return false;
		}
		break;
		
	case UI_MAIN_MENU:
		if (event == EVT_BOX_YES)
		{
			getStage()->getManager()->changeStage(STAGE_MAIN_MENU);
			GlobalEx::NetSystem().closeNetwork();
			return true;
		}
		else if (event != EVT_BOX_NO)
		{
			::Global::GUI().showMessageBox(id, LOCTEXT("Exit game?"), EMessageButton::YesNo);
			return false;
		}
		break;
	}
	
	return true;
}

MsgReply NetLevelStageModeEx::onKey(KeyMsg const& msg)
{
	if (msg.isDown() && msg.getCode() == EKeyCode::Tab)
	{
		if (mMsgPanel)
		{
			bool beShow = !mMsgPanel->isShow();
			mMsgPanel->show(beShow);
			if (beShow)
			{
				mMsgPanel->clearInputString();
				mMsgPanel->makeFocus();
			}
			getGame()->getInputControl().blockAllAction(beShow);
		}
		return MsgReply::Handled();
	}
	
	return BaseClass::onKey(msg);
}

void NetLevelStageModeEx::onRestart(uint64& seed)
{
	seed = mNetSeed;
	BaseClass::onRestart(seed);
}

bool NetLevelStageModeEx::prevChangeState(EGameState state)
{
	// 只有在 LevelRunning 狀態才能開始遊戲
	if (getGameState() == EGameState::Start)
	{
		if (mNetAdapter.getSessionState() != ENetSessionState::LevelRunning)
			return false;
	}
	return true;
}

IPlayerManager* NetLevelStageModeEx::getPlayerManager()
{
	return mNetAdapter.getPlayerManager();
}

void NetLevelStageModeEx::updateFrame(int frame)
{
	getStage()->updateFrame(frame);
}

void NetLevelStageModeEx::tick()
{
	ActionProcessor& actionProcessor = getStage()->getActionProcessor();
	
	unsigned flag = 0;
	switch (getGameState())
	{
	case EGameState::Run:
		++mReplayFrame;
		break;
	default:
		flag |= CTF_FREEZE_FRAME;
	}
	
	actionProcessor.beginAction(flag);
	getStage()->tick();
	actionProcessor.endAction();
}

bool NetLevelStageModeEx::startDirectLevel(GameLevelInfo const& info)
{
	if (!isHost() || !mHostSession)
		return false;
	
	mDirectLevelMode = true;
	mAllowLateJoin = true;
	
	return mHostSession->startLevelDirect(info);
}

bool NetLevelStageModeEx::joinDirectLevel(char const* hostAddress)
{
	if (!mClientSession)
		return false;
	
	mDirectLevelMode = true;
	mClientSession->joinLevelDirect(hostAddress, "Player");
	return true;
}

void NetLevelStageModeEx::setLateJoinEnabled(bool enabled)
{
	mAllowLateJoin = enabled;
	
	if (isHost() && mHostSession)
	{
		mHostSession->setAllowLateJoin(enabled);
	}
}

void NetLevelStageModeEx::onPlayerLateJoin(PlayerId playerId)
{
	LogMsg("Player %d late-joined", playerId);
	
	if (mGameSession)
	{
		mGameSession->onPlayerJoined(playerId, true);
	}
}

void NetLevelStageModeEx::onSessionEvent(ENetSessionEvent event, PlayerId playerId)
{
	switch (event)
	{
	case ENetSessionEvent::PlayerJoined:
		if (mAllowLateJoin && 
		    mNetAdapter.getSessionState() == ENetSessionState::LevelRunning)
		{
			onPlayerLateJoin(playerId);
		}
		break;
		
	case ENetSessionEvent::PlayerLeft:
		if (mGameSession)
		{
			mGameSession->onPlayerLeft(playerId);
		}
		break;
		
	case ENetSessionEvent::LevelPaused:
		mNetAdapter.setSessionState(ENetSessionState::LevelPaused);
		if (isHost())
		{
			::Global::GUI().showMessageBox(
				UI_UNPAUSE_GAME, 
				LOCTEXT("Game paused. Click OK to continue."),
				EMessageButton::Ok);
		}
		break;
		
	case ENetSessionEvent::LevelResumed:
		mNetAdapter.setSessionState(ENetSessionState::LevelRunning);
		{
			GWidget* ui = ::Global::GUI().getManager().getModalWidget();
			if (ui && ui->getID() == UI_UNPAUSE_GAME)
			{
				ui->destroy();
			}
		}
		break;
	}
}

void NetLevelStageModeEx::setupPacketHandlers()
{
	// 註冊封包處理器
}

void NetLevelStageModeEx::procPlayerState(IComPacket* cp)
{
	CSPPlayerState* com = cp->cast<CSPPlayerState>();
	
	switch (com->state)
	{
	case NAS_LEVEL_RUN:
		mNetAdapter.setSessionState(ENetSessionState::LevelRunning);
		break;
		
	case NAS_LEVEL_PAUSE:
		mNetAdapter.setSessionState(ENetSessionState::LevelPaused);
		break;
		
	case NAS_ROOM_ENTER:
		// 返回房間
		mNetAdapter.setSessionState(ENetSessionState::InRoom);
		break;
	}
}

void NetLevelStageModeEx::procPlayerStateSv(IComPacket* cp)
{
	// Server 端處理
}

void NetLevelStageModeEx::procLevelInfo(IComPacket* cp)
{
	SPLevelInfo* com = cp->cast<SPLevelInfo>();
	
	if (!loadLevel(*com))
	{
		LogError("Failed to load level");
		return;
	}
	
	mNetAdapter.setSessionState(ENetSessionState::LevelLoading);
}

void NetLevelStageModeEx::procMsg(IComPacket* cp)
{
	CSPMsg* com = cp->cast<CSPMsg>();
	
	if (mMsgPanel)
	{
		InlineString<128> str;
		switch (com->type)
		{
		case CSPMsg::eSERVER:
			str.format("## %s ##", com->content.c_str());
			mMsgPanel->addMessage(str, Color3ub(255, 0, 255));
			break;
		case CSPMsg::ePLAYER:
			{
				GamePlayer* player = getPlayerManager()->getPlayer(com->playerID);
				if (player)
				{
					str.format("%s : %s", player->getName(), com->content.c_str());
					mMsgPanel->addMessage(str, Color3ub(255, 255, 0));
				}
			}
			break;
		}
	}
}

void NetLevelStageModeEx::procNetControlRequest(IComPacket* cp)
{
	// 處理網路控制請求
}

bool NetLevelStageModeEx::buildNetEngine()
{
	// TODO: 建立網路引擎
	return true;
}

bool NetLevelStageModeEx::loadLevel(GameLevelInfo const& info)
{
	IPlayerManager* playerMgr = getPlayerManager();
	
	mNetSeed = info.seed;
	getStage()->setupLevel(info);
	getStage()->setupScene(*playerMgr);
	
	if (!buildNetEngine())
	{
		return false;
	}
	
	// 建立遊戲網路會話（如果遊戲支援）
	IGameModule* game = getGame();
	if (game)
	{
		if (auto factory = dynamic_cast<IGameNetSessionFactory*>(game))
		{
			mGameSession = factory->createGameNetSession();
			if (mGameSession)
			{
				mGameSession->initialize(mNetAdapter.getSession());
			}
		}
	}
	
	return true;
}
