#include "ServerTestWorker.h"

#include "GamePlayer.h"
#include "LogSystem.h"
#include "GameServer.h"
#include "GameNetPacket.h"

//========================================
// 建構/解構
//========================================

ServerTestWorker::ServerTestWorker()
	: NetWorker()  // 繼承自 NetWorker 而非 ComWorker
{
	LogMsg("ServerTestWorker: Initializing new architecture test worker");
}

ServerTestWorker::~ServerTestWorker()
{
	closeNetwork();
	LogMsg("ServerTestWorker: Destroyed");
}

//========================================
// NetWorker 虛擬方法實作
//========================================

bool ServerTestWorker::doStartNetwork()
{
	LogMsg("ServerTestWorker: Starting network (doStartNetwork)...");

	// 1. 建立傳輸層
	mTransport = std::make_unique<ServerTransportImpl>();
	if (!mTransport->startNetwork())
	{
		LogError("ServerTestWorker: Failed to start transport layer");
		mTransport.reset();
		return false;
	}

	// 2. 建立會話層
	mSession = std::make_unique<NetSessionHostImpl>();
	if (!mSession->initialize(mTransport.get()))
	{
		LogError("ServerTestWorker: Failed to initialize session layer");
		mTransport->closeNetwork();
		mTransport.reset();
		mSession.reset();
		return false;
	}


	// 3. Session 層已經在 initialize() 中設定了 Transport 回調
	// 不需要在這裡重複設定，否則會覆蓋掉 Session 層的回調！
	
	// 4. 設定 Session 事件處理器
	mSession->setEventHandler([this](ENetSessionEvent event, PlayerId playerId) {
		onSessionEvent(event, playerId);
	});
	
	// 5. ✅ 創建 local player (Server 自己)
	// 舊的 SVPlayerManager 會自動創建 SUserPlayer
	// 新架構需要明確創建
	PlayerId localPlayerId = mSession->createLocalPlayer("Server");
	if (localPlayerId == ERROR_PLAYER_ID)
	{
		LogWarning(0, "ServerTestWorker: Failed to create local player");
	}
	else
	{
		LogMsg("ServerTestWorker: Local player created, PlayerId=%d", localPlayerId);
	}
	
	// ✅ server_info 请求现在由 NetSession 层自动处理
	// 不需要在这里注册处理器
	
	// 6. ✅ 註冊 Game 層 packet handlers
	// Session 層處理完 packet 後，會調用這些 handlers
	// 這樣 NetRoomStage/NetLevelStageMode 等才能收到 packet
	
	// NetRoomStage + NetLevelStageMode 使用的封包
	mSession->registerPacketHandler(CSPPlayerState::PID, [this](PlayerId id, IComPacket* cp) {
		getPacketDispatcher().procCommand(cp);
	});
	
	mSession->registerPacketHandler(CSPMsg::PID, [this](PlayerId id, IComPacket* cp) {
		getPacketDispatcher().procCommand(cp);
	});
	
	mSession->registerPacketHandler(CSPRawData::PID, [this](PlayerId id, IComPacket* cp) {
		getPacketDispatcher().procCommand(cp);
	});
	
	mSession->registerPacketHandler(SPPlayerStatus::PID, [this](PlayerId id, IComPacket* cp) {
		getPacketDispatcher().procCommand(cp);
	});
	
	mSession->registerPacketHandler(SPSlotState::PID, [this](PlayerId id, IComPacket* cp) {
		getPacketDispatcher().procCommand(cp);
	});
	
	mSession->registerPacketHandler(SPLevelInfo::PID, [this](PlayerId id, IComPacket* cp) {
		getPacketDispatcher().procCommand(cp);
	});
	
	mSession->registerPacketHandler(SPNetControlRequest::PID, [this](PlayerId id, IComPacket* cp) {
		getPacketDispatcher().procCommand(cp);
	});
	
	// 可以添加更多層級的 observers，例如：
	// - System layer observer（記錄、統計）
	// - UI layer observer（通知更新）
	// - Debug observer（日誌）
	
	LogMsg("ServerTestWorker: Registered %d packet observers for Game layer", 7);

	LogMsg("ServerTestWorker: Network started successfully");
	return true;
}

void ServerTestWorker::doCloseNetwork()
{
	LogMsg("ServerTestWorker: Closing network (doCloseNetwork)...");

	if (mSession)
	{
		mSession->shutdown();
		mSession.reset();
	}

	if (mTransport)
	{
		mTransport->closeNetwork();
		mTransport.reset();
	}

	LogMsg("ServerTestWorker: Network closed");
}

void ServerTestWorker::clenupNetResource()
{
	// NetWorker 要求實作此方法
	// 清理網路資源（如果有額外的）
}

bool ServerTestWorker::update_NetThread(long time)
{
	// NetWorker 的網路執行緒更新
	// 由於新架構的 Transport 層自己處理執行緒，這裡不需要額外處理
	return BaseClass::update_NetThread(time);
}

void ServerTestWorker::doUpdate(long time)
{
	BaseClass::doUpdate(time);

	// 更新新架構組件
	if (mTransport)
	{
		mTransport->update(time);
	}

	if (mSession)
	{
		mSession->update(time);
	}

	// 更新本地 Worker
	if (mLocalWorker)
	{
		mLocalWorker->update(time);
	}

	// 同步狀態
	syncStateFromSession();
}

void ServerTestWorker::postChangeState(NetActionState oldState)
{
	BaseClass::postChangeState(oldState);
	
	// 同步狀態到新架構
	syncStateToSession();
}

//========================================
// ComWorker 介面實作
//========================================

bool ServerTestWorker::sendCommand(int channel, IComPacket* cp, EWorkerSendFlag flag)
{
	if (!mSession)
		return false;

	// 將 channel 轉換為 ENetChannelType
	ENetChannelType channelType = ENetChannelType::Tcp;
	if (channel == CHANNEL_GAME_NET_UDP_CHAIN)
	{
		channelType = ENetChannelType::UdpChain;
	}

	// 檢查是否要忽略本地玩家
	bool ignoreLocal = (flag & WSF_IGNORE_LOCAL) != 0;

	bool sentToAny = false;

	// 如果有本地玩家且不忽略，發送給本地 Worker
	if (mLocalWorker && !ignoreLocal)
	{
		// 將封包直接發送給本地 Worker
		// LocalWorker::recvCommand 會將封包放入接收緩衝區
		mLocalWorker->recvCommand(cp);
		sentToAny = true;
	}

	// 廣播給所有遠端玩家
	if (channelType == ENetChannelType::Tcp)
	{
		mSession->broadcastReliable(cp);
		sentToAny = true;
	}
	else
	{
		mSession->broadcastUnreliable(cp);
		sentToAny = true;
	}

	return sentToAny;
}

SVPlayerManager* ServerTestWorker::getPlayerManager()
{
	// ✅ 直接返回 Session 層的 SVPlayerManager
	return mSession ? mSession->getSVPlayerManager() : nullptr;
}

void ServerTestWorker::generatePlayerStatus(SPPlayerStatus& comPS)
{
	if (auto* playerMgr = getPlayerManager())
	{
		playerMgr->getPlayerInfo(comPS.info);
		playerMgr->getPlayerFlag(comPS.flags);
		comPS.numPlayer = (uint8)playerMgr->getPlayerNum();
	}
}

NetSocket& ServerTestWorker::getUdpSocket()
{
	// 從 Transport 層獲取 UDP Server 的 Socket
	if (mTransport)
	{
		return mTransport->getUdpServer().getSocket();
	}
	
	// 錯誤情況：沒有 Transport 層
	// 這不應該發生，但為了安全起見返回一個靜態的無效 socket
	static NetSocket invalidSocket;
	LogError("ServerTestWorker::getUdpSocket() called without valid transport!");
	return invalidSocket;
}

bool ServerTestWorker::addUdpCom(IComPacket* cp, NetAddress const& addr)
{
	if (!mTransport)
		return false;
	
	// 委託給 Transport 層處理 UDP 發送
	return mTransport->sendUdpPacket(cp, addr);
}

// ✅ handleServerInfoRequest 已移至 NetSessionHostImpl
// 如果需要自定义服务器信息，可以继承 NetSessionHostImpl 并覆盖 getServerInfo()

//========================================
// ServerWorker 兼容介面
//========================================

LocalWorker* ServerTestWorker::createLocalWorker(char const* userName)
{
	LogMsg("ServerTestWorker: Creating local worker for %s", userName);

	// 如果已經有 LocalWorker，直接返回
	if (mLocalWorker)
	{
		LogWarning(0, "ServerTestWorker: LocalWorker already exists");
		return mLocalWorker;
	}

	// 創建 LocalWorker（需要傳入 this 指針）
	// 注意：LocalWorker 的構造函數需要 ServerWorker*，但我們是 ServerTestWorker
	// 由於我們繼承自 NetWorker（ServerWorker 也繼承自 NetWorker），
	// 我們可以安全地將 this 轉換為所需類型
	// 但這裡有個問題：LocalWorker 的構造函數接受 ServerWorker*
	// 我們需要修改 LocalWorker 或者使用不同的方法

	// 方案：創建 LocalWorker 並傳入 this（作為兼容的 NetWorker）
	// LocalWorker 內部只使用 NetWorker 的功能，所以應該沒問題
	mLocalWorker.reset(new LocalWorker(reinterpret_cast<ServerWorker*>(this)));
	
	// 通過玩家管理器創建用戶玩家
	if (auto* playerMgr = static_cast<SVPlayerManager*>(getPlayerManager()))
	{
		playerMgr->createUserPlayer(mLocalWorker, userName);
	}
	else
	{
		LogError("ServerTestWorker: Failed to get player manager for local worker");
		mLocalWorker.release();
		return nullptr;
	}

	return mLocalWorker;
}

bool ServerTestWorker::kickPlayer(unsigned id)
{
	if (!mSession)
		return false;

	return mSession->kickPlayer(id);
}

bool ServerTestWorker::createRoom(char const* gameName, int maxPlayers)
{
	if (!mSession)
		return false;

	LogMsg("ServerTestWorker: Creating room '%s' with max %d players", gameName, maxPlayers);
	return mSession->createRoom(gameName, maxPlayers);
}

void ServerTestWorker::closeRoom()
{
	if (mSession)
	{
		mSession->closeRoom();
	}
}

bool ServerTestWorker::startLevel(GameLevelInfo const& info)
{
	if (!mSession)
		return false;

	LogMsg("ServerTestWorker: Starting level");
	return mSession->startLevel(info);
}

void ServerTestWorker::endLevel()
{
	if (mSession)
	{
		mSession->endLevel();
	}
}

void ServerTestWorker::pauseLevel()
{
	if (mSession)
	{
		mSession->pauseLevel();
	}
}

void ServerTestWorker::resumeLevel()
{
	if (mSession)
	{
		mSession->resumeLevel();
	}
}

//========================================
// 事件處理
//========================================

void ServerTestWorker::registerPacketHandler(ComID packetId, std::function<void(PlayerId, IComPacket*)> handler)
{
	if (mSession)
	{
		mSession->registerPacketHandler(packetId, handler);
	}
}

void ServerTestWorker::unregisterPacketHandler(ComID packetId)
{
	if (mSession)
	{
		mSession->unregisterPacketHandler(packetId);
	}
}

void ServerTestWorker::setEventListener(std::function<void(ENetSessionEvent, PlayerId)> listener)
{
	mEventListener = listener;
}

//========================================
// Session 事件橋接
// 
// ✅ 正確的架構：
// Transport → Session (directly) → ServerTestWorker (via event handler)
// 
// ServerTestWorker 不再作為 Transport 的回調橋接，
// 而是通過 Session 事件來接收通知
//========================================

void ServerTestWorker::onSessionEvent(ENetSessionEvent event, PlayerId playerId)
{
	LogMsg("ServerTestWorker: Session event %d for PlayerId=%d", (int)event, playerId);

	// ✅ Session 層已經實現了 IPlayerManager，不需要額外的 player 管理
	// Player 的創建和刪除都在 NetSessionHostImpl 中處理
	
	// 橋接到舊的 INetStateListener (for NetRoomStage etc.)
	if (mNetListener)
	{
		switch (event)
		{
		case ENetSessionEvent::PlayerJoined:
			mNetListener->onPlayerStateMsg(playerId, PSM_ADD);
			break;

		case ENetSessionEvent::PlayerLeft:
			mNetListener->onPlayerStateMsg(playerId, PSM_REMOVE);
			break;

		case ENetSessionEvent::LevelStarting:
			mNetListener->onChangeActionState(NAS_LEVEL_LOAD);
			break;

		case ENetSessionEvent::LevelStarted:
			mNetListener->onChangeActionState(NAS_LEVEL_RUN);
			break;

		case ENetSessionEvent::LevelEnded:
			mNetListener->onChangeActionState(NAS_LEVEL_EXIT);
			break;

		case ENetSessionEvent::LevelPaused:
			mNetListener->onChangeActionState(NAS_LEVEL_PAUSE);
			break;

		default:
			break;
		}
	}

	// 調用新的事件監聽器
	if (mEventListener)
	{
		mEventListener(event, playerId);
	}
}

//========================================
// 狀態轉換輔助
//========================================

NetActionState ServerTestWorker::toNetActionState(ENetSessionState state)
{
	switch (state)
	{
	case ENetSessionState::Disconnected:   return NAS_DISSCONNECT;
	case ENetSessionState::Connecting:     return NAS_LOGIN;
	case ENetSessionState::Connected:      return NAS_CONNECT;
	case ENetSessionState::InRoom:         return NAS_ROOM_ENTER;
	case ENetSessionState::RoomReady:      return NAS_ROOM_READY;
	case ENetSessionState::LevelLoading:   return NAS_LEVEL_LOAD;
	case ENetSessionState::LevelRunning:   return NAS_LEVEL_RUN;
	case ENetSessionState::LevelPaused:    return NAS_LEVEL_PAUSE;
	default:                                return NAS_DISSCONNECT;
	}
}

ENetSessionState ServerTestWorker::toSessionState(NetActionState state)
{
	switch (state)
	{
	case NAS_DISSCONNECT:   return ENetSessionState::Disconnected;
	case NAS_LOGIN:         return ENetSessionState::Connecting;
	case NAS_CONNECT:       return ENetSessionState::Connected;
	case NAS_ROOM_ENTER:
	case NAS_ROOM_WAIT:     return ENetSessionState::InRoom;
	case NAS_ROOM_READY:    return ENetSessionState::RoomReady;
	case NAS_LEVEL_SETUP:
	case NAS_LEVEL_LOAD:    return ENetSessionState::LevelLoading;
	case NAS_LEVEL_INIT:
	case NAS_LEVEL_RUN:     return ENetSessionState::LevelRunning;
	case NAS_LEVEL_PAUSE:   return ENetSessionState::LevelPaused;
	default:                return ENetSessionState::Disconnected;
	}
}

void ServerTestWorker::syncStateFromSession()
{
	if (!mSession)
		return;

	NetActionState newState = toNetActionState(mSession->getState());
	if (newState != getActionState())
	{
		changeState(newState);
	}
}

void ServerTestWorker::syncStateToSession()
{
	if (!mSession)
		return;

	ENetSessionState newState = toSessionState(getActionState());
	if (newState != mSession->getState())
	{
		mSession->changeState(newState);
	}
}

bool ServerTestWorker::isRunning() const
{
	return mTransport && mTransport->isRunning();
}

bool ServerTestWorker::isRoomOpen() const
{
	return mSession && mSession->isRoomOpen();
}

ENetSessionState ServerTestWorker::getSessionState() const
{
	return mSession ? mSession->getState() : ENetSessionState::Disconnected;
}

PlayerId ServerTestWorker::getLocalPlayerId() const
{
	return mSession ? mSession->getLocalPlayerId() : ERROR_PLAYER_ID;
}