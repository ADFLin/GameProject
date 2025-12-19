#include "TinyGamePCH.h"
#include "NetSessionImpl.h"
#include "GameNetPacket.h"
#include "GameServer.h"
#include "GameClient.h"
#include "GameWorker.h"

//========================================
// NetSessionBase
//========================================

NetSessionBase::NetSessionBase()
{
}

NetSessionBase::~NetSessionBase()
{
}

void NetSessionBase::procHandlerCommand(IComPacket* cp)
{
	auto it = mPacketHandlers.find(cp->getID());
	if (it != mPacketHandlers.end())
	{
		return;
	}
}

bool NetSessionBase::tryChangeState(ENetSessionState newState)
{
	if (!isValidStateTransition(mState, newState))
	{
		LogWarning(0, "Invalid session state transition: %d -> %d", (int)mState, (int)newState);
		return false;
	}
	
	ENetSessionState oldState = mState;
	mState = newState;
	LogMsg("Session state changed: %d -> %d", (int)oldState, (int)newState);
	return true;
}

bool NetSessionBase::isValidStateTransition(ENetSessionState from, ENetSessionState to) const
{
	// 基本狀態轉換驗證
	switch (from)
	{
	case ENetSessionState::Disconnected:
		return to == ENetSessionState::Connecting;
		
	case ENetSessionState::Connecting:
		return to == ENetSessionState::Connected || 
		       to == ENetSessionState::Disconnected;
		
	case ENetSessionState::Connected:
		return to == ENetSessionState::InLobby ||
		       to == ENetSessionState::InRoom ||
		       to == ENetSessionState::LevelLoading ||  // Direct join
		       to == ENetSessionState::Disconnected;
		
	case ENetSessionState::InLobby:
		return to == ENetSessionState::InRoom ||
		       to == ENetSessionState::Disconnected;
		
	case ENetSessionState::InRoom:
		return to == ENetSessionState::RoomReady ||
		       to == ENetSessionState::LevelLoading ||
		       to == ENetSessionState::InLobby ||
		       to == ENetSessionState::Disconnected;
		
	case ENetSessionState::RoomReady:
		return to == ENetSessionState::InRoom ||
		       to == ENetSessionState::LevelLoading ||
		       to == ENetSessionState::Disconnected;
		
	case ENetSessionState::LevelLoading:
		return to == ENetSessionState::LevelRunning ||
		       to == ENetSessionState::InRoom ||
		       to == ENetSessionState::Disconnected;
		
	case ENetSessionState::LevelRunning:
		return to == ENetSessionState::LevelPaused ||
		       to == ENetSessionState::InRoom ||
		       to == ENetSessionState::LevelLoading ||  // Restart
		       to == ENetSessionState::Disconnected;
		
	case ENetSessionState::LevelPaused:
		return to == ENetSessionState::LevelRunning ||
		       to == ENetSessionState::InRoom ||
		       to == ENetSessionState::Disconnected;
	}
	
	return false;
}

//========================================
// SSessionPlayer - Session 層專用 ServerPlayer
//========================================

SSessionPlayer::SSessionPlayer(NetSessionHostImpl* session, bool bNet)
	: ServerPlayer(bNet)
	, mSession(session)
{
	// ✅ 初始化 ServerPlayer 的 PlayerInfo（必須調用，否則 getInfo() 會返回無效引用）
	PlayerInfo info;  // 臨時 PlayerInfo
	init(info);  // ServerPlayer::init() 會複製這個 info 到內部
}

void SSessionPlayer::sendCommand(int channel, IComPacket* cp)
{
	if (channel == CHANNEL_GAME_NET_TCP)
		sendTcpCommand(cp);
	else
		sendUdpCommand(cp);
}

void SSessionPlayer::sendTcpCommand(IComPacket* cp)
{
	if (mSession)
	{
		// ✅ 通過 Session 層發送給特定 player
		mSession->sendReliableTo(getId(), cp);
	}
}

void SSessionPlayer::sendUdpCommand(IComPacket* cp)
{
	if (mSession)
	{
		// TODO: Session 層目前沒有 sendUnreliableTo 方法
		// 需要添加或使用其他方式
		LogWarning(0, "SSessionPlayer::sendUdpCommand not implemented yet");
	}
}

//========================================
// NetSessionHostImpl
//========================================

NetSessionHostImpl::NetSessionHostImpl()
{
	mPlayerManager = std::make_unique<SVPlayerManager>();
}

NetSessionHostImpl::~NetSessionHostImpl()
{
	shutdown();
}

bool NetSessionHostImpl::initialize(INetTransport* transport)
{
	mTransport = transport;
	mServerTransport = dynamic_cast<IServerTransport*>(transport);
	
	if (!mServerTransport)
	{
		LogError("NetSessionHostImpl requires IServerTransport");
		return false;
	}
	
	LogMsg("Server: Registered %d packet types", 7);
	
	// 設定 Transport 回調
	NetTransportCallbacks callbacks;
	callbacks.onConnectionEstablished = [this](SessionId id) {
		onConnectionEstablished(id);
	};
	callbacks.onConnectionClosed = [this](SessionId id, ENetCloseReason reason) {
		onConnectionClosed(id, reason);
	};
	callbacks.onPacketReceivedNet = [this](IComPacket* packet) {
		onPacketReceived(packet);
	};
	callbacks.onUdpPacketReceivedNet = [this](IComPacket* packet, NetAddress const& clientAddr) {
		onUdpPacketReceived(packet, clientAddr);
	};
	
	mTransport->setCallbacks(callbacks);
	
	// Host 玩家
	mUserPlayerId = ERROR_PLAYER_ID;
	
	// 註冊核心封包
	registerCorePacketHandlers();
	
	mState = ENetSessionState::Connected;
	
	LogMsg("NetSessionHost initialized");
	return true;
}

void NetSessionHostImpl::shutdown()
{
	closeRoom();
	mPacketHandlers.clear();
	
	// ✅ SVPlayerManager 會負責清理玩家
	
	mTransport = nullptr;
	mServerTransport = nullptr;
	mState = ENetSessionState::Disconnected;
}

void NetSessionHostImpl::update(long time)
{
	NetSessionBase::doUpdate(time);
}

bool NetSessionHostImpl::changeState(ENetSessionState newState)
{
	if (!tryChangeState(newState))
		return false;
	
	// 廣播狀態變更
	// TODO: 發送狀態同步封包
	
	return true;
}

void NetSessionHostImpl::getPlayerInfos(TArray<NetPlayerInfo>& outInfos) const
{
	if (mPlayerInfosDirty)
	{
		mCachedPlayerInfos.clear();
		for (size_t i = 0; i < mPlayerManager->getPlayerNum(); ++i)
		{
			ServerPlayer* player = mPlayerManager->getPlayer(i);
			if (player)
			{
				SSessionPlayer* sessionPlayer = static_cast<SSessionPlayer*>(player);
					
				// ✅ 將 PlayerInfo 轉換為 NetPlayerInfo
				NetPlayerInfo netInfo;
				netInfo.id = sessionPlayer->getInfo().playerId;
				FCString::CopyN(netInfo.name, sessionPlayer->getInfo().name.c_str(), MAX_PLAYER_NAME_LENGTH);
				netInfo.slot = sessionPlayer->getInfo().slot;
				netInfo.isLocal = false; // Server 上的 client 都不是 local
				netInfo.isReady = sessionPlayer->getStateFlag().check(ServerPlayer::eReady);
				netInfo.playerType = sessionPlayer->getInfo().type;
				mCachedPlayerInfos.push_back(netInfo);
			}
		}
		mPlayerInfosDirty = false;
	}
	outInfos = mCachedPlayerInfos;
}

NetPlayerInfo const* NetSessionHostImpl::getPlayerInfo(PlayerId id) const
{
	// ⚠️ 注意：這個方法返回 const NetPlayerInfo*，但我們儲存的是 PlayerInfo
	// 我們需要從 cached 中返回，因為不能返回臨時變量的指標
	
	// 強制更新 cache
	if (mPlayerInfosDirty)
	{
		TArray<NetPlayerInfo> temp;
		getPlayerInfos(temp); // 這會更新 mCachedPlayerInfos
	}
	
	// 從 cache 中查找
	for (auto const& info : mCachedPlayerInfos)
	{
		if (info.id == id)
			return &info;
	}
	return nullptr;
}

void NetSessionHostImpl::registerPacketHandler(ComID packetId, INetSession::PacketHandler handler)
{
	mPacketDispatcher.setUserFunc(packetId, static_cast<NetSessionBase*>(this), &NetSessionBase::procHandlerCommand);
	mPacketHandlers[packetId] = handler;
}

void NetSessionHostImpl::unregisterPacketHandler(ComID packetId)
{
	mPacketHandlers.erase(packetId);
}

void NetSessionHostImpl::sendReliable(IComPacket* cp)
{
	// 發送給網路上的 clients
	mServerTransport->broadcastPacket(ENetChannelType::Tcp, cp);
}

void NetSessionHostImpl::sendUnreliable(IComPacket* cp)
{
	// 發送給網路上的 clients
	mServerTransport->broadcastPacket(ENetChannelType::UdpChain, cp);
}

void NetSessionHostImpl::sendReliableTo(PlayerId targetId, IComPacket* cp)
{
	// ✅ 如果是發送給 local player
	if (targetId == mUserPlayerId && mUserPlayerId != ERROR_PLAYER_ID)
	{
		dispatchLocalPacket(cp);
		return;
	}
	
	// 發送給網路 player
	SessionId sessionId = getPlayerSession(targetId);
	if (sessionId != 0)
	{
		mServerTransport->sendPacket(sessionId, ENetChannelType::Tcp, cp);
	}
}

void NetSessionHostImpl::dispatchLocalPacket(IComPacket* packet)
{
	if (mUserPlayerId == ERROR_PLAYER_ID)
	{
		return;
	}

	auto player = static_cast<SUserSessionPlayer*>(mPlayerManager->getPlayer(mUserPlayerId));
	player->mLocalPlayer->recvCommand(packet);
}

bool NetSessionHostImpl::createRoom(char const* gameName, int maxPlayers)
{
	if (mRoomOpen)
		return false;
	
	FCString::Copy(mGameName, gameName);
	mMaxPlayers = maxPlayers;
	mRoomOpen = true;
	
	changeState(ENetSessionState::InRoom);
	fireEvent(ENetSessionEvent::RoomCreated);
	
	LogMsg("Room created: %s (max %d players)", mGameName, mMaxPlayers);
	return true;
}

void NetSessionHostImpl::closeRoom()
{
	if (!mRoomOpen)
		return;
	
	mRoomOpen = false;
	fireEvent(ENetSessionEvent::RoomClosed);
	
	LogMsg("Room closed");
}

bool NetSessionHostImpl::kickPlayer(PlayerId id)
{
	SSessionPlayer* ps = findPlayer(id);
	if (!ps)
		return false;
	
	mServerTransport->kickConnection(ps->sessionId);
	return true;
}

void NetSessionHostImpl::broadcastReliable(IComPacket* cp)
{
	sendReliable(cp);
	dispatchLocalPacket(cp);
}

void NetSessionHostImpl::broadcastUnreliable(IComPacket* cp)
{
	sendUnreliable(cp);
	dispatchLocalPacket(cp);
}

bool NetSessionHostImpl::startLevel(GameLevelInfo const& info)
{
	if (mState != ENetSessionState::InRoom && mState != ENetSessionState::RoomReady)
	{
		LogWarning(0, "Cannot start level: not in room state");
		return false;
	}
	
	// 通知所有玩家開始載入
	changeState(ENetSessionState::LevelLoading);
	fireEvent(ENetSessionEvent::LevelStarting);
	
	// 發送關卡資訊
	SPLevelInfo levelInfoPacket;
	levelInfoPacket.seed = info.seed;
	// TODO: 填充更多關卡資訊
	sendReliable(&levelInfoPacket);
	
	return true;
}

bool NetSessionHostImpl::startLevelDirect(GameLevelInfo const& info)
{
	// 直接開始關卡，不經過 Room
	mAllowLateJoin = true;
	
	changeState(ENetSessionState::LevelLoading);
	fireEvent(ENetSessionEvent::LevelStarting);
	
	// 發送關卡資訊
	SPLevelInfo levelInfoPacket;
	levelInfoPacket.seed = info.seed;
	sendReliable(&levelInfoPacket);
	
	return true;
}

void NetSessionHostImpl::endLevel()
{
	changeState(ENetSessionState::InRoom);
	fireEvent(ENetSessionEvent::LevelEnded);
}

void NetSessionHostImpl::pauseLevel()
{
	if (mState == ENetSessionState::LevelRunning)
	{
		changeState(ENetSessionState::LevelPaused);
		fireEvent(ENetSessionEvent::LevelPaused);
	}
}

void NetSessionHostImpl::resumeLevel()
{
	if (mState == ENetSessionState::LevelPaused)
	{
		changeState(ENetSessionState::LevelRunning);
		fireEvent(ENetSessionEvent::LevelResumed);
	}
}

void NetSessionHostImpl::restartLevel()
{
	changeState(ENetSessionState::LevelLoading);
	// TODO: 重新載入關卡
}

void NetSessionHostImpl::registerCorePacketHandlers()
{	
	// ✅ 使用 setWorkerFunc 註冊封包處理器
	// GameThread 處理：遊戲邏輯相關
	// NetThread 處理：網絡 I/O、立即回應
	
	mPacketDispatcher.setWorkerFunc<CPLogin>(this, &NetSessionHostImpl::procLoginPacket, nullptr);
	mPacketDispatcher.setWorkerFunc<CSPPlayerState>(this, &NetSessionHostImpl::procPlayerStatePacket, nullptr);
	mPacketDispatcher.setWorkerFunc<CSPClockSynd>(this, &NetSessionHostImpl::procClockSyncPacket, nullptr);
	mPacketDispatcher.setWorkerFunc<CPEcho>(this, &NetSessionHostImpl::procEchoPacket, nullptr);
	mPacketDispatcher.setWorkerFunc<CSPMsg>(this, &NetSessionHostImpl::procMsgPacket, nullptr);

	LogMsg("Server: Registered %d core packet handlers via PacketDispatcher", 4);
}

// ========================================
// Packet Handler 方法實現（GameThread）
// ========================================

void NetSessionHostImpl::procLoginPacket(IComPacket* cp)
{
	// ⚠️ 在 GameThread 執行
	//assert(IsInGameThread());
	
	CPLogin* packet = cp->cast<CPLogin>();
	// Group 存儲的是 SessionId（由 Transport 層設置）
	SessionId sessionId = static_cast<SessionId>(packet->getGroup());
	
	if (sessionId == 0)
	{
		LogWarning(0, "Invalid sessionId for login request");
		return;
	}
	
	// 檢查是否已經有玩家與此 session 關聯
	SSessionPlayer* existingPs = findPlayerBySession(sessionId);
	if (existingPs)
	{
		LogWarning(0, "SessionId %d already has a player: %d", sessionId, existingPs->playerId);
		return;
	}
	
	// 創建新玩家
	PlayerId playerId = createPlayer(sessionId, packet->name);
	if (playerId == ERROR_PLAYER_ID)
	{
		LogError("Failed to create player for session %d, name=%s", sessionId, packet->name);
		// TODO: 通知 client 登入失敗
		return;
	}
	
	LogMsg("Player login: PlayerId=%d, SessionId=%d, Name=%s", playerId, sessionId, packet->name);
	
	// 發送登入成功回應給該玩家
	CSPPlayerState acceptState;
	acceptState.playerId = playerId;
	acceptState.state = NAS_ACCPET;
	sendReliableTo(playerId, &acceptState);
	
	// 廣播新玩家已連接
	CSPPlayerState connectState;
	connectState.playerId = playerId;
	connectState.state = NAS_CONNECT;
	broadcastReliable(&connectState);
	
	// 觸發事件
	fireEvent(ENetSessionEvent::PlayerJoined, playerId);
}

void NetSessionHostImpl::procPlayerStatePacket(IComPacket* cp)
{
	//assert(IsInGameThread());
	
	CSPPlayerState* packet = cp->cast<CSPPlayerState>();
	PlayerId playerId = packet->playerId;
	
	// 忽略無效或 Server 的狀態變更
	if (playerId == ERROR_PLAYER_ID || playerId == SERVER_PLAYER_ID)
	{
		return;
	}
	
	// 查找玩家
	SSessionPlayer* ps = findPlayer(playerId);
	if (!ps)
	{
		LogWarning(0, "Player %d not found for state change", playerId);
		return;
	}
	
	LogMsg("Player %d state -> %d", playerId, packet->state);
	
	// 處理不同的狀態變更
	switch (packet->state)
	{
	case NAS_ROOM_ENTER:
		// 玩家進入房間
		LogMsg("Player %d entering room", playerId);
		break;
		
	case NAS_ROOM_WAIT:
		// 玩家取消 Ready
		LogMsg("Player %d not ready", playerId);
		ps->getStateFlag().remove(ServerPlayer::eReady);
		mPlayerInfosDirty = true;
		broadcastReliable(cp);
		fireEvent(ENetSessionEvent::PlayerReadyChanged, playerId);
		break;
		
	case NAS_ROOM_READY:
		// 玩家 Ready
		LogMsg("Player %d ready", playerId);
		ps->getStateFlag().add(ServerPlayer::eReady);
		mPlayerInfosDirty = true;
		broadcastReliable(cp);
		fireEvent(ENetSessionEvent::PlayerReadyChanged, playerId);
		break;
		
	case NAS_DISSCONNECT:
		// 玩家斷線
		LogMsg("Player %d disconnecting", playerId);
		ps->getStateFlag().add(ServerPlayer::eDissconnect);
		broadcastReliable(cp);
		break;
		
	case NAS_LEVEL_SETUP:
		// Level 設置完成
		ps->getStateFlag().add(ServerPlayer::eLevelSetup);
		LogMsg("Player %d level setup done", playerId);
		
		// 檢查所有玩家是否都完成設置
		if (checkAllPlayersFlag([](SSessionPlayer const& p) { return p.getStateFlag().check(ServerPlayer::eLevelSetup); }, true))
		{
			LogMsg("All players level setup done, transitioning to LEVEL_LOAD");
			changeState(ENetSessionState::LevelLoading);
			
			// 廣播狀態變更
			CSPPlayerState loadState;
			loadState.playerId = SERVER_PLAYER_ID;
			loadState.state = NAS_LEVEL_LOAD;
			broadcastReliable(&loadState);
		}
		break;
		
	case NAS_LEVEL_LOAD:
		// Level 載入完成
		ps->getStateFlag().add(ServerPlayer::eLevelLoaded);
		LogMsg("Player %d level loaded", playerId);
		
		// 檢查所有玩家是否都完成載入
		if (checkAllPlayersFlag([](SSessionPlayer const& p) { return p.getStateFlag().check(ServerPlayer::eLevelLoaded); }, true))
		{
			LogMsg("All players level loaded, transitioning to LEVEL_INIT");
			
			// 廣播初始化狀態
			CSPPlayerState initState;
			initState.playerId = SERVER_PLAYER_ID;
			initState.state = NAS_LEVEL_INIT;
			broadcastReliable(&initState);
		}
		break;
		
	case NAS_LEVEL_INIT:
		// Level  初始化完成
		ps->getStateFlag().add(ServerPlayer::eLevelReady);
		LogMsg("Player %d level ready", playerId);
		
		// 檢查所有玩家是否都準備好
		if (checkAllPlayersFlag([](SSessionPlayer const& p) { return p.getStateFlag().check(ServerPlayer::eLevelReady); }, true))
		{
			LogMsg("All players level ready, starting time sync");
			
			// 開始時鐘同步
			CSPPlayerState syncState;
			syncState.playerId = SERVER_PLAYER_ID;
			syncState.state = NAS_TIME_SYNC;
			broadcastReliable(&syncState);
		}
		break;
		
	case NAS_TIME_SYNC:
		// 時鐘同步完成
		ps->getStateFlag().add(ServerPlayer::eSyndDone);
		LogMsg("Player %d time sync done", playerId);
		
		// 檢查所有玩家是否都完成同步
		if (checkAllPlayersFlag([](SSessionPlayer const& p) { return p.getStateFlag().check(ServerPlayer::eSyndDone); }, true))
		{
			LogMsg("All players time sync done, starting level");
			changeState(ENetSessionState::LevelRunning);
			
			// 廣播開始運行
			CSPPlayerState runState;
			runState.playerId = SERVER_PLAYER_ID;
			runState.state = NAS_LEVEL_RUN;
			broadcastReliable(&runState);
		}
		break;
		
	case NAS_LEVEL_PAUSE:
		// 暫停請求
		if (mState == ENetSessionState::LevelRunning)
		{
			LogMsg("Player %d requests pause", playerId);
			ps->getStateFlag().add(ServerPlayer::ePause);
			changeState(ENetSessionState::LevelPaused);
			broadcastReliable(cp);
			fireEvent(ENetSessionEvent::LevelPaused, playerId);
		}
		break;
		
	case NAS_LEVEL_RUN:
		// 恢復運行
		if (mState == ENetSessionState::LevelPaused)
		{
			LogMsg("Player %d requests resume", playerId);
			
			if (playerId == mUserPlayerId)
			{
				// 移除所有玩家的暫停標誌
				for (size_t i = 0; i < mPlayerManager->getPlayerNum(); ++i)
				{
					ServerPlayer* player = mPlayerManager->getPlayer(i);
					if (player)
					{
						player->getStateFlag().remove(ServerPlayer::ePause);
					}
				}
				changeState(ENetSessionState::LevelRunning);
				broadcastReliable(cp);
				fireEvent(ENetSessionEvent::LevelResumed, playerId);
			}
			else
			{
				// 其他玩家：移除該玩家的暫停標誌，檢查是否所有玩家都不再暫停
				ps->getStateFlag().remove(ServerPlayer::ePause);
				
				if (checkAllPlayersFlag([](SSessionPlayer const& p) { return !p.getStateFlag().check(ServerPlayer::ePause); }, true))
				{
					changeState(ENetSessionState::LevelRunning);
					broadcastReliable(cp);
					fireEvent(ENetSessionEvent::LevelResumed, playerId);
				}
			}
		}
		break;
		
	default:
		LogWarning(0, "Unknown player state: %d", packet->state);
		break;
	}
}


void NetSessionHostImpl::procEchoPacket(IComPacket* cp)
{
	sendReliable(cp);
}

// ========================================
// NetThread Socket 處理器
// ========================================

void NetSessionHostImpl::procClockSyncPacket(IComPacket* cp)
{
	//assert(IsInGameThread());
	
	CSPClockSynd* packet = cp->cast<CSPClockSynd>();
	
	// GameThread 處理 eDONE 消息（更新玩家延遲）
	if (packet->code == CSPClockSynd::eDONE)
	{
		// ✅ 從 packet group 獲取 SessionId，然後查找對應的 PlayerId
		SessionId sessionId = static_cast<SessionId>(packet->getGroup());
		SSessionPlayer* ps = findPlayerBySession(sessionId);
		
		if (ps && ps->playerId != SERVER_PLAYER_ID)
		{
			// 更新玩家延遲
			ps->latency = packet->latency;
			LogMsg("Player %d clock sync done, latency=%d", ps->playerId, packet->latency);
		}
	}
}



void NetSessionHostImpl::onConnectionEstablished(SessionId id)
{
	LogMsg("Client connected: SessionId=%d", id);
	
	// 新連線建立，等待登入封包
}

void NetSessionHostImpl::onConnectionClosed(SessionId id, ENetCloseReason reason)
{
	LogMsg("Client disconnected: SessionId=%d, reason=%d", id, (int)reason);
	
	SSessionPlayer* ps = findPlayerBySession(id);
	if (!ps)
		return;
		
	PlayerId playerId = ps->playerId;
	
	// ✅ 詢問如何處理斷線（參考 ServerWorker::notifyConnectionClose）
	EPlayerConnectionCloseAction action = EPlayerConnectionCloseAction::Remove;
	
	if (mConnectionCloseHandler)
	{
		action = mConnectionCloseHandler(playerId, reason);
	}
	
	switch (action)
	{
	case EPlayerConnectionCloseAction::Remove:
		{
			// 直接移除玩家
			removePlayer(playerId);
			fireEvent(ENetSessionEvent::PlayerLeft, playerId);
		}
		break;
		
	case EPlayerConnectionCloseAction::ChangeToLocal:
		{
			// TODO: 實作轉為本地玩家的邏輯
			// 需要 SVPlayerManager::swapNetPlayerToLocal 支持
			LogWarning(0, "ChangeToLocal not yet implemented for PlayerId=%d", playerId);
			
			// 暫時仍然移除
			removePlayer(playerId);
			fireEvent(ENetSessionEvent::PlayerLeft, playerId);
		}
		break;
		
	case EPlayerConnectionCloseAction::WaitReconnect:
		{
			// 標記為斷線，但不移除玩家
			ps->getStateFlag().add(ServerPlayer::eDissconnect);
			
			// 廣播斷線狀態給其他玩家
			CSPPlayerState com;
			com.playerId = playerId;
			com.state = NAS_DISSCONNECT;
			broadcastReliable(&com);
			
			LogMsg("Player %d marked as disconnected, waiting for reconnect", playerId);
		}
		break;
	}
}

void NetSessionHostImpl::onPacketReceived(IComPacket* packet)
{
#if _DEBUG
	// 從 packet->getGroup() 獲取 sessionId
	SessionId id = static_cast<SessionId>(packet->getGroup());
	SSessionPlayer* ps = findPlayerBySession(id);
	PlayerId senderId = ps ? ps->playerId : ERROR_PLAYER_ID;

	LogMsg("TCP packet received: SessionId=%d, PacketID=%u, PlayerId=%d", id, packet->getID(), senderId);
#endif
	if (!mPacketDispatcher.recvCommand(packet))
	{
		delete packet;
	}
}

void NetSessionHostImpl::onUdpPacketReceived(IComPacket* packet, NetAddress const& clientAddr)
{
	LogMsg("UDP packet received: PacketID=%u", packet->getID());
	
	// ✅ 自动处理房间搜寻请求（Session 层功能）
	if (packet->getID() == CSPComMsg::PID)
	{
		CSPComMsg* com = packet->cast<CSPComMsg>();
		if (com && com->str == "server_info")
		{
			LogMsg("Handling server_info request");
			// Room 搜寻是 Session 层的职责，在这里直接处理
			handleServerInfoRequest(clientAddr);
			delete packet;
			return;
		}
	}
}

void NetSessionHostImpl::handleServerInfoRequest(NetAddress const& clientAddr)
{
	// 构建服务器信息
	SPServerInfo info;
	getServerInfo(info);
	
	LogMsg("Sending server info: name='%s', ip='%s'", info.name.c_str(), info.ip.c_str());
	
	// 通过 Transport 层发送 UDP 回应
	if (mServerTransport)
	{
		bool sent = mServerTransport->sendUdpPacket(&info, clientAddr);
		LogMsg("sendUdpPacket returned: %s", sent ? "true" : "false");
	}
	else
	{
		LogWarning(0, "mServerTransport is null, cannot send UDP packet");
	}
}

void NetSessionHostImpl::getServerInfo(SPServerInfo& outInfo)
{
	// 基础实现：使用 Session 层已有的信息
	outInfo.name = mGameName;
	
	// 如果没有设置房间名称，提供默认值
	if (outInfo.name.empty() || outInfo.name[0] == '\0')
	{
		outInfo.name = "Game Server";
	}
	
	// 获取本地 IP 地址
	InlineString<256> hostname;
	if (gethostname(hostname, hostname.max_size()) == 0)
	{
		hostent* hn = gethostbyname(hostname);
		if (hn && hn->h_addr_list[0])
		{
			outInfo.ip = inet_ntoa(*(struct in_addr*)hn->h_addr_list[0]);
		}
	}
	
	// 子类可以覆盖此方法来提供更详细的信息
	// 例如：玩家名称、房間設置等
}


PlayerId NetSessionHostImpl::createPlayer(SessionId sessionId, char const* name)
{
	return createPlayerInternal(sessionId, name, false);
}


PlayerId NetSessionHostImpl::createPlayerInternal(SessionId sessionId, char const* name, bool bUser)
{
	if (mPlayerManager->getPlayerNum() >= mMaxPlayers)
		return ERROR_PLAYER_ID;

	PlayerId newId = mNextPlayerId++;

	SlotId assignedSlot = -1;
	for (SlotId slot = 0; slot < MAX_PLAYER_NUM; ++slot)
	{
		bool slotUsed = false;
		for (size_t i = 0; i < mPlayerManager->getPlayerNum(); ++i)
		{
			ServerPlayer* player = mPlayerManager->getPlayer(i);
			if (player && player->getInfo().slot == slot)
			{
				slotUsed = true;
				break;
			}
		}
		if (!slotUsed)
		{
			assignedSlot = slot;
			break;
		}
	}

	SSessionPlayer* sessionPlayer;
	if (bUser)
	{
		sessionPlayer = new SUserSessionPlayer(this);
	}
	else
	{
		sessionPlayer = new SSessionPlayer(this);
	}

	PlayerId playerId = mPlayerManager->insertPlayer(sessionPlayer, name, PT_PLAYER);
	sessionPlayer->sessionId = sessionId;
	sessionPlayer->getInfo().slot = assignedSlot;
	sessionPlayer->getInfo().actionPort = ERROR_ACTION_PORT;
	sessionPlayer->getInfo().ping = 0;

	mPlayerInfosDirty = true;
	fireEvent(ENetSessionEvent::PlayerJoined, playerId);

	return playerId;
}

// ✅ 為 Server 創建 local player
PlayerId NetSessionHostImpl::createUserPlayer(char const* name)
{
	// Local player 不關聯 SessionId，使用特殊值 0
	PlayerId playerId = createPlayerInternal(0, name, true);
	
	// ✅ 設置 local player ID，這樣廣播時會包含 local player
	if (playerId != ERROR_PLAYER_ID)
	{
		mUserPlayerId = playerId;
		mPlayerManager->mUserID = playerId;
		LogMsg("Local player created: PlayerId=%d", playerId);
	}
	return playerId;
}

void NetSessionHostImpl::removePlayer(PlayerId id)
{
	if (mPlayerManager)
	{
		mPlayerManager->removePlayer(id);
	}
	
	fireEvent(ENetSessionEvent::PlayerLeft, id);
	mPlayerInfosDirty = true;
}

SSessionPlayer* NetSessionHostImpl::findPlayerBySession(SessionId sessionId)
{	
	// ✅ 遍歷 SVPlayerManager 查找匹配的 sessionId
	for (size_t i = 0; i < mPlayerManager->getPlayerNum(); ++i)
	{
		ServerPlayer* player = mPlayerManager->getPlayer(i);
		if (player)
		{
			SSessionPlayer* sessionPlayer = static_cast<SSessionPlayer*>(player);
			if (sessionPlayer->sessionId == sessionId)
				return sessionPlayer;
		}
	}
	return nullptr;
}

SSessionPlayer* NetSessionHostImpl::findPlayer(PlayerId id)
{
	ServerPlayer* player = mPlayerManager->getPlayer(id);
	return player ? static_cast<SSessionPlayer*>(player) : nullptr;
}

SessionId NetSessionHostImpl::getPlayerSession(PlayerId id)
{
	SSessionPlayer* ps = findPlayer(id);
	return ps ? ps->sessionId : 0;
}


//========================================
// NetSessionClientImpl
//========================================

NetSessionClientImpl::NetSessionClientImpl()
{
	mPlayerManager = std::make_unique<CLPlayerManager>();
}

NetSessionClientImpl::~NetSessionClientImpl()
{
	shutdown();
}

bool NetSessionClientImpl::initialize(INetTransport* transport)
{
	mTransport = transport;
	mClientTransport = dynamic_cast<IClientTransport*>(transport);
	
	if (!mClientTransport)
	{
		LogError("NetSessionClientImpl requires IClientTransport");
		return false;
	}
	
	LogMsg("Client: Registered %d packet types", 7);
	
	// 設定 Transport 回調
	NetTransportCallbacks callbacks;
	callbacks.onConnectionEstablished = [this](SessionId id) {
		onConnectionEstablished(id);
	};
	callbacks.onConnectionClosed = [this](SessionId id, ENetCloseReason reason) {
		onConnectionClosed(id, reason);
	};
	callbacks.onConnectionFailed = [this]() {
		onConnectionFailed();
	};
	callbacks.onPacketReceivedNet = [this](IComPacket* packet) {
		onPacketReceived(packet);
	};
	
	mTransport->setCallbacks(callbacks);
	
	// 註冊核心封包
	registerCorePacketHandlers();
	
	mState = ENetSessionState::Disconnected;
	
	LogMsg("NetSessionClient initialized");
	return true;
}

void NetSessionClientImpl::shutdown()
{
	leaveServer();
	mPacketHandlers.clear();
	mPlayerInfos.clear();
	mTransport = nullptr;
	mClientTransport = nullptr;
	mState = ENetSessionState::Disconnected;
}

void NetSessionClientImpl::update(long time)
{
	NetSessionBase::doUpdate(time);
}

bool NetSessionClientImpl::changeState(ENetSessionState newState)
{
	return tryChangeState(newState);
}

void NetSessionClientImpl::getPlayerInfos(TArray<NetPlayerInfo>& outInfos) const
{
	outInfos = mPlayerInfos;
}

NetPlayerInfo const* NetSessionClientImpl::getPlayerInfo(PlayerId id) const
{
	for (auto const& info : mPlayerInfos)
	{
		if (info.id == id)
			return &info;
	}
	return nullptr;
}

void NetSessionClientImpl::registerPacketHandler(ComID packetId, INetSession::PacketHandler handler)
{
	mPacketDispatcher.setUserFunc(packetId, static_cast<NetSessionBase*>(this), &NetSessionBase::procHandlerCommand);
	mPacketHandlers[packetId] = handler;
}

void NetSessionClientImpl::unregisterPacketHandler(ComID packetId)
{
	mPacketHandlers.erase(packetId);
}

void NetSessionClientImpl::sendReliable(IComPacket* cp)
{
	mClientTransport->sendToServer(ENetChannelType::Tcp, cp);
}

void NetSessionClientImpl::sendUnreliable(IComPacket* cp)
{
	mClientTransport->sendToServer(ENetChannelType::UdpChain, cp);
}

void NetSessionClientImpl::sendReliableTo(PlayerId targetId, IComPacket* cp)
{
	// Client 只能發送給 Server，Server 會轉發
	sendReliable(cp);
}

void NetSessionClientImpl::joinServer(char const* hostName, char const* playerName)
{
	if (mState != ENetSessionState::Disconnected)
	{
		LogWarning(0, "Cannot join: not disconnected");
		return;
	}
	
	FCString::Copy(mPlayerName, playerName);
	mDirectJoin = false;
	
	changeState(ENetSessionState::Connecting);
	mClientTransport->connect(hostName, 0);
}

void NetSessionClientImpl::leaveServer()
{
	if (mState == ENetSessionState::Disconnected)
		return;
	
	// 發送離開通知
	CSPPlayerState statePacket;
	statePacket.state = NAS_DISSCONNECT;
	statePacket.playerId = mUserPlayerId;
	sendReliable(&statePacket);
	
	mClientTransport->disconnect();
	mState = ENetSessionState::Disconnected;
	mUserPlayerId = ERROR_PLAYER_ID;
}

void NetSessionClientImpl::joinLevelDirect(char const* hostName, char const* playerName)
{
	if (mState != ENetSessionState::Disconnected)
	{
		LogWarning(0, "Cannot join: not disconnected");
		return;
	}
	
	FCString::Copy(mPlayerName, playerName);
	mDirectJoin = true;
	
	changeState(ENetSessionState::Connecting);
	mClientTransport->connect(hostName, 0);
}

void NetSessionClientImpl::setReady(bool ready)
{
	if (mState != ENetSessionState::InRoom && mState != ENetSessionState::RoomReady)
		return;
	
	mIsReady = ready;
	
	CSPPlayerState statePacket;
	statePacket.state = ready ? NAS_ROOM_READY : NAS_ROOM_WAIT;
	statePacket.playerId = mUserPlayerId;
	sendReliable(&statePacket);
	
	if (ready)
		changeState(ENetSessionState::RoomReady);
	else
		changeState(ENetSessionState::InRoom);
}

void NetSessionClientImpl::searchLanServers()
{
	// TODO: 實作區網伺服器搜尋
}

void NetSessionClientImpl::registerCorePacketHandlers()
{
	// 註冊 Client 需要處理的核心封包（直接使用 PacketDispatcher）
	
	mPacketDispatcher.setWorkerFunc<CPLogin>(this, &NetSessionClientImpl::procLoginResponsePacket);
	mPacketDispatcher.setWorkerFunc<SPPlayerStatus>(this, &NetSessionClientImpl::procPlayerStatusPacket);
	mPacketDispatcher.setWorkerFunc<SPLevelInfo>(this, &NetSessionClientImpl::procLevelInfoPacket);
	mPacketDispatcher.setWorkerFunc<CSPPlayerState>(this, &NetSessionClientImpl::procPlayerStatePacket);
	
	LogMsg("Client: Registered %d packet handlers via PacketDispatcher", 4);
}

void NetSessionClientImpl::onConnectionEstablished(SessionId id)
{
	LogMsg("Connected to server");
	
	changeState(ENetSessionState::Connected);
	fireEvent(ENetSessionEvent::ConnectionEstablished);
	
	// ✅ 发送登入请求
	CPLogin loginPacket;
	loginPacket.id = 0;  // Client 还没有 ID
	loginPacket.name = mPlayerName;  // TInlineString 直接赋值
	
	LogMsg("Sending CPLogin: name='%s'", mPlayerName);
	sendReliable(&loginPacket);
}

void NetSessionClientImpl::onConnectionClosed(SessionId id, ENetCloseReason reason)
{
	LogMsg("Disconnected from server, reason=%d", (int)reason);
	
	mState = ENetSessionState::Disconnected;
	mUserPlayerId = ERROR_PLAYER_ID;
	fireEvent(ENetSessionEvent::ConnectionLost);
}

void NetSessionClientImpl::onConnectionFailed()
{
	LogMsg("Failed to connect to server");
	
	mState = ENetSessionState::Disconnected;
	fireEvent(ENetSessionEvent::LoginFailed);
}

void NetSessionClientImpl::onPacketReceived(IComPacket* packet)
{
	if (!mPacketDispatcher.recvCommand(packet))
	{
		delete packet;
	}
}

// ========================================
// Packet Handler 包裝方法（用於 PacketDispatcher）
// ========================================

void NetSessionClientImpl::procLoginResponsePacket(IComPacket* cp)
{
	procLoginResponse(cp);
}

void NetSessionClientImpl::procPlayerStatusPacket(IComPacket* cp)
{
	procPlayerStatus(cp);
}

void NetSessionClientImpl::procPlayerStatePacket(IComPacket* cp)
{
	procPlayerState(cp);
}

void NetSessionClientImpl::procLevelInfoPacket(IComPacket* cp)
{
	// TODO: 實作關卡資訊處理
	LogMsg("Received SPLevelInfo");
}


void NetSessionClientImpl::procLoginResponse(IComPacket* cp)
{
	// TODO: 處理登入回應 (需要定義新的 NetSessionPacket)
	// SPLogin* loginPacket = cp->cast<SPLogin>();
	// if (loginPacket->result == LGR_SUCCESS) { ... }
}

void NetSessionClientImpl::procPlayerStatus(IComPacket* cp)
{
	SPPlayerStatus* statusPacket = cp->cast<SPPlayerStatus>();
	
	// 更新玩家列表
	mPlayerInfos.clear();
	for (int i = 0; i < statusPacket->numPlayer; i++)
	{
		NetPlayerInfo info;
		info.id = statusPacket->info[i]->playerId;
		FCString::Copy(info.name, statusPacket->info[i]->name);
		info.slot = statusPacket->info[i]->slot;
		info.isLocal = (info.id == mUserPlayerId);
		info.playerType = (int)statusPacket->info[i]->type;
		mPlayerInfos.push_back(info);
	}
}

void NetSessionClientImpl::procPlayerState(IComPacket* cp)
{
	CSPPlayerState* statePacket = cp->cast<CSPPlayerState>();
	
	// 處理玩家狀態變更
	switch (statePacket->state)
	{
	case NAS_LEVEL_SETUP:
		changeState(ENetSessionState::LevelLoading);
		fireEvent(ENetSessionEvent::LevelStarting);
		break;
		
	case NAS_LEVEL_RUN:
		changeState(ENetSessionState::LevelRunning);
		fireEvent(ENetSessionEvent::LevelStarted);
		break;
		
	case NAS_LEVEL_PAUSE:
		changeState(ENetSessionState::LevelPaused);
		fireEvent(ENetSessionEvent::LevelPaused);
		break;
		
	case NAS_ROOM_ENTER:
		changeState(ENetSessionState::InRoom);
		break;
	}
}

void NetSessionClientImpl::procLevelInfo(IComPacket* cp)
{
	SPLevelInfo* levelPacket = cp->cast<SPLevelInfo>();
	
	// 收到關卡資訊，準備載入
	changeState(ENetSessionState::LevelLoading);
	fireEvent(ENetSessionEvent::LevelLoaded);
}

void SUserSessionPlayer::sendCommand(int channel, IComPacket* cp)
{
	mLocalPlayer->recvCommand(cp);
}

void SUserSessionPlayer::sendTcpCommand(IComPacket* cp)
{
	mLocalPlayer->recvCommand(cp);
}

void SUserSessionPlayer::sendUdpCommand(IComPacket* cp)
{
	mLocalPlayer->recvCommand(cp);
}
