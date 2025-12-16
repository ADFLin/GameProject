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

void NetSessionBase::dispatchPacket(PlayerId senderId, IComPacket* packet)
{
	auto it = mPacketHandlers.find(packet->getID());
	if (it != mPacketHandlers.end())
	{
		LogMsg("Dispatching packet %u to handler", packet->getID());
		it->second(senderId, packet);
	}
	else
	{
		LogWarning(0, "No handler registered for PacketID=%u", packet->getID());
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

SSessionPlayer::SSessionPlayer(NetSessionHostImpl* session)
	: ServerPlayer(true)  // true = network player
	, mSession(session)
{
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
	// ✅ 初始化 SVPlayerManager (向後兼容)
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
	
	// 註冊封包工廠到 PacketEvaluator
	auto& evaluator = mTransport->getPacketEvaluator();
	evaluator.addFactory<CPLogin>();
	evaluator.addFactory<CSPPlayerState>();
	evaluator.addFactory<CSPClockSynd>();
	evaluator.addFactory<CSPComMsg>();
	evaluator.addFactory<CPUdpCon>();
	evaluator.addFactory<CPEcho>();
	evaluator.addFactory<CSPMsg>();
	LogMsg("Server: Registered %d packet types", 7);
	
	// 設定 Transport 回調
	NetTransportCallbacks callbacks;
	callbacks.onConnectionEstablished = [this](SessionId id) {
		onConnectionEstablished(id);
	};
	callbacks.onConnectionClosed = [this](SessionId id, ENetCloseReason reason) {
		onConnectionClosed(id, reason);
	};
	callbacks.onPacketReceived = [this](SessionId id, IComPacket* packet) {
		onPacketReceived(id, packet);
	};
	callbacks.onUdpPacketReceived = [this](SessionId id, IComPacket* packet, NetAddress const& clientAddr) {
		onUdpPacketReceived(id, packet, clientAddr);
	};
	
	mTransport->setCallbacks(callbacks);
	
	// Host 玩家
	mLocalPlayerId = SERVER_PLAYER_ID;
	
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
	mPlayerSessions.clear();
	mTransport = nullptr;
	mServerTransport = nullptr;
	mState = ENetSessionState::Disconnected;
}

void NetSessionHostImpl::update(long time)
{
	// 會話層更新邏輯
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
		for (auto const& ps : mPlayerSessions)
		{
			// ✅ 將 PlayerInfo 轉換為 NetPlayerInfo
			NetPlayerInfo netInfo;
			netInfo.id = ps.info.playerId;
			FCString::CopyN(netInfo.name, ps.info.name, MAX_PLAYER_NAME_LENGTH);
			netInfo.slot = ps.info.slot;
			netInfo.isLocal = false; // Server 上的 client 都不是 local
			netInfo.isReady = ps.isReady; // ✅ 使用 PlayerSession::isReady
			netInfo.playerType = ps.info.type;
			mCachedPlayerInfos.push_back(netInfo);
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
	mPacketHandlers[packetId] = handler;
}

void NetSessionHostImpl::unregisterPacketHandler(ComID packetId)
{
	mPacketHandlers.erase(packetId);
}

void NetSessionHostImpl::sendReliable(IComPacket* cp)
{
	mServerTransport->broadcastPacket(ENetChannelType::Tcp, cp);
}

void NetSessionHostImpl::sendUnreliable(IComPacket* cp)
{
	mServerTransport->broadcastPacket(ENetChannelType::UdpChain, cp);
}

void NetSessionHostImpl::sendReliableTo(PlayerId targetId, IComPacket* cp)
{
	SessionId sessionId = getPlayerSession(targetId);
	if (sessionId != 0)
	{
		mServerTransport->sendPacket(sessionId, ENetChannelType::Tcp, cp);
	}
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
	PlayerSession* ps = findPlayer(id);
	if (!ps)
		return false;
	
	mServerTransport->kickConnection(ps->sessionId);
	return true;
}

void NetSessionHostImpl::broadcastReliable(IComPacket* cp)
{
	sendReliable(cp);
}

void NetSessionHostImpl::broadcastUnreliable(IComPacket* cp)
{
	sendUnreliable(cp);
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
	// ✅ 登入封包 - 注意：對於登入封包，需要從 packet 的 UserData 獲取 SessionId
	// 因為此時玩家尚未創建，id 會是 ERROR_PLAYER_ID
	registerPacketHandler(CPLogin::PID, [this](PlayerId id, IComPacket* packet) {
		// UserData 存儲的是 SessionId（由 Transport 層設置）
		SessionId sessionId = reinterpret_cast<SessionId>(packet->getUserData());
		handleLoginRequest(sessionId, packet->cast<CPLogin>());
	});
	
	// ✅ 玩家狀態 (包括 Ready/Wait/Disconnect 等)
	registerPacketHandler(CSPPlayerState::PID, [this](PlayerId id, IComPacket* packet) {
		handlePlayerState(id, packet->cast<CSPPlayerState>());
	});
	
	// ✅ 时钟同步
	registerPacketHandler(CSPClockSynd::PID, [this](PlayerId id, IComPacket* packet) {
		handleClockSync(id, packet->cast<CSPClockSynd>());
	});
	
	// ✅ Echo 测试
	registerPacketHandler(CPEcho::PID, [this](PlayerId id, IComPacket* packet) {
		sendReliable(packet);
	});
	
	// ✅ Game 層封包 - Session 層不處理，只轉發給 observers
	// 這些封包由 Game 層（NetRoomStage 等）處理
	auto passThrough = [this](ComID packetId) {
		registerPacketHandler(packetId, [this, packetId](PlayerId id, IComPacket* packet) {
			// Session 層不處理，直接通知 observers
			notifyPacketObservers(packetId, id, packet);
		});
	};
	
	passThrough(CSPMsg::PID);
	passThrough(CSPRawData::PID);
	passThrough(SPPlayerStatus::PID);
	passThrough(SPSlotState::PID);
	passThrough(SPLevelInfo::PID);
	passThrough(SPNetControlRequest::PID);
	
	LogMsg("Server: Registered %d core packet handlers + %d game layer pass-through handlers", 4, 6);
}

void NetSessionHostImpl::onConnectionEstablished(SessionId id)
{
	LogMsg("Client connected: SessionId=%d", id);
	
	// 新連線建立，等待登入封包
}

void NetSessionHostImpl::onConnectionClosed(SessionId id, ENetCloseReason reason)
{
	LogMsg("Client disconnected: SessionId=%d, reason=%d", id, (int)reason);
	
	PlayerSession* ps = findPlayerBySession(id);
	if (ps)
	{
		PlayerId playerId = ps->playerId;
		removePlayer(playerId);
		fireEvent(ENetSessionEvent::PlayerLeft, playerId);
	}
}

void NetSessionHostImpl::onPacketReceived(SessionId id, IComPacket* packet)
{
	PlayerSession* ps = findPlayerBySession(id);
	PlayerId senderId = ps ? ps->playerId : ERROR_PLAYER_ID;
	
	LogMsg("TCP packet received: SessionId=%d, PacketID=%u, PlayerId=%d", id, packet->getID(), senderId);
	
	// TCP 封包或已連線的 UDP 封包
	// packet->getUserData() = sessionId (來自 readNewPacket)
	
	dispatchPacket(senderId, packet);
	
	// ⚠️ Session 層負責刪除 packet
	delete packet;
}

void NetSessionHostImpl::onUdpPacketReceived(SessionId id, IComPacket* packet, NetAddress const& clientAddr)
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
	
	// 其他 UDP 无连线封包 - 分派到注册的处理器
	// sessionId 通常为 0，senderId 为 ERROR_PLAYER_ID
	
	PlayerSession* ps = findPlayerBySession(id);
	PlayerId senderId = ps ? ps->playerId : ERROR_PLAYER_ID;
	
	// 将 NetAddress 临时附加到 UserData，供自定义处理器使用
	void* originalUserData = packet->getUserData();
	NetAddress* addrCopy = new NetAddress(clientAddr);
	packet->setUserData(addrCopy);
	
	// 分派到处理器
	dispatchPacket(senderId, packet);
	
	// 恢复原始 UserData 并清理
	packet->setUserData(originalUserData);
	delete addrCopy;
	delete packet;
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
	// 例如：玩家名称、房间设置等
}

void NetSessionHostImpl::handleLoginRequest(SessionId sessionId, CPLogin* packet)
{
	if (!packet)
		return;
	
	LogMsg("Login request from SessionId=%d, Name='%s'", sessionId, packet->name);
	
	// 检查是否已经创建了玩家
	PlayerSession* ps = findPlayerBySession(sessionId);
	if (ps)
	{
		LogMsg("Player already exists: PlayerId=%u", ps->playerId);
		return;
	}
	
	// 创建玩家会话（使用 sessionId 來關聯 TCP 連線）
	PlayerId playerId = createPlayer(sessionId, packet->name);
	if (playerId == ERROR_PLAYER_ID)
	{
		LogWarning(0, "Failed to create player - room full");
		// TODO: 发送登入失败回应
		return;
	}
	
	ps = findPlayer(playerId);
	
	// ✅ 发送登入成功回应 (按照旧的 ServerWorker 逻辑)
	// 1. 发送 NAS_ACCPET 给该玩家
	CSPPlayerState acceptState;
	acceptState.playerId = playerId;
	acceptState.state = NAS_ACCPET;
	sendReliableTo(playerId, &acceptState);
	
	// 2. 广播 NAS_CONNECT 给所有人
	CSPPlayerState connectState;
	connectState.playerId = playerId;
	connectState.state = NAS_CONNECT;
	broadcastReliable(&connectState);
	
	// 通知事件
	fireEvent(ENetSessionEvent::PlayerJoined, playerId);
	
	LogMsg("Player '%s' logged in as PlayerId=%u, sent NAS_ACCPET and NAS_CONNECT", packet->name, playerId);
}

void NetSessionHostImpl::handlePlayerState(PlayerId id, CSPPlayerState* packet)
{
	if (!packet)
		return;
	
	if (id == ERROR_PLAYER_ID || id == SERVER_PLAYER_ID)
		return;
	
	PlayerSession* ps = findPlayer(id);
	if (!ps)
		return;
	
	LogMsg("Player %u state packet: state=%d", id, packet->state);
	
	// ✅ 參考舊的 ServerWorker::procPlayerState，根據 state 字段處理不同狀態
	switch (packet->state)
	{
	case NAS_DISSCONNECT:
		// Player 斷線
		LogMsg("DEBUG: Broadcasting NAS_DISSCONNECT");
		broadcastReliable(packet);
		LogMsg("DEBUG: Broadcast completed");
		break;
		
	case NAS_LOGIN:
	case NAS_ACCPET:
	case NAS_CONNECT:
	case NAS_RECONNECT:
		// 連線相關狀態，通常在登入階段處理，這裡只轉發
		LogMsg("DEBUG: Broadcasting connection state %d", packet->state);
		broadcastReliable(packet);
		LogMsg("DEBUG: Broadcast completed");
		break;
		
	case NAS_ROOM_ENTER:
		// Player 進入房間
		LogMsg("DEBUG: NAS_ROOM_ENTER - no action");
		break;
		
	case NAS_ROOM_WAIT:
		// Player 取消準備
		LogMsg("Player %u not ready", id);
		ps->isReady = false; // ✅ 使用 PlayerSession::isReady
		mPlayerInfosDirty = true;
		LogMsg("DEBUG: Broadcasting NAS_ROOM_WAIT");
		broadcastReliable(packet);
		LogMsg("DEBUG: Broadcast completed");
		fireEvent(ENetSessionEvent::PlayerReadyChanged, id);
		break;
		
	case NAS_ROOM_READY:
		// Player 準備就緒
		LogMsg("Player %u ready", id);
		ps->isReady = true; // ✅ 使用 PlayerSession::isReady
		mPlayerInfosDirty = true;
		LogMsg("DEBUG: Broadcasting NAS_ROOM_READY");
		broadcastReliable(packet);
		LogMsg("DEBUG: Broadcast completed");
		fireEvent(ENetSessionEvent::PlayerReadyChanged, id);
		
		// 檢查是否所有人都準備好
		if (isAllPlayersReady())
		{
			LogMsg("All players ready!");
			// 可以觸發遊戲開始等邏輯
		}
		break;
		
	case NAS_TIME_SYNC:
	case NAS_LEVEL_SETUP:
	case NAS_LEVEL_LOAD:
	case NAS_LEVEL_INIT:
	case NAS_LEVEL_RESTART:
	case NAS_LEVEL_RUN:
	case NAS_LEVEL_PAUSE:
	case NAS_LEVEL_EXIT:
		// Level 相關狀態，暫時只轉發
		LogMsg("DEBUG: Broadcasting level state %d", packet->state);
		broadcastReliable(packet);
		LogMsg("DEBUG: Broadcast completed");
		break;
		
	default:
		LogWarning(0, "Unknown player state: %d", packet->state);
		break;
	}
	
	// ✅ 通知所有該 packet 的 observers
	notifyPacketObservers(CSPPlayerState::PID, id, packet);
	
	LogMsg("DEBUG: handlePlayerState returning");
}

void NetSessionHostImpl::addPacketObserver(ComID packetId, PacketObserver observer)
{
	mPacketObservers[packetId].push_back(observer);
}

void NetSessionHostImpl::clearPacketObservers()
{
	mPacketObservers.clear();
}

void NetSessionHostImpl::notifyPacketObservers(ComID packetId, PlayerId senderId, IComPacket* packet)
{
	auto it = mPacketObservers.find(packetId);
	if (it != mPacketObservers.end())
	{
		for (auto& observer : it->second)
		{
			observer(senderId, packet);
		}
	}
}

void NetSessionHostImpl::handleClockSync(PlayerId id, CSPClockSynd* packet)
{
	if (!packet)
		return;
	
	// 简单实现：直接回应
	// 实际的时钟同步逻辑较复杂，在这里简化处理
	LogMsg("Clock sync request from player %u, code=%d", id, (int)packet->code);
	
	// 可以回传同样的封包或根据 code 字段处理
	// 这里暂时只记录，完整实现需要根据具体的同步协议
}

bool NetSessionHostImpl::isAllPlayersReady() const
{
	if (mPlayerSessions.empty())
		return false;
	
	for (auto const& ps : mPlayerSessions)
	{
		if (!ps.isReady) // ✅ 使用 PlayerSession::isReady
			return false;
	}
	return true;
}

PlayerId NetSessionHostImpl::createPlayer(SessionId sessionId, char const* name)
{
	if ((int)mPlayerSessions.size() >= mMaxPlayers)
		return ERROR_PLAYER_ID;
	
	PlayerId newId = mNextPlayerId++;
	
	// ✅ 自動分配一個唯一的 slot
	SlotId assignedSlot = -1;
	for (SlotId slot = 0; slot < MAX_PLAYER_NUM; ++slot)
	{
		bool slotUsed = false;
		for (auto const& existingPlayer : mPlayerSessions)
		{
			if (existingPlayer.info.slot == slot)
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
	
	PlayerSession ps;
	ps.playerId = newId;
	ps.sessionId = sessionId;
	// ✅ PlayerInfo 的字段名
	ps.info.playerId = newId;
	ps.info.name = name;
	ps.info.slot = assignedSlot;  // ✅ 自動分配的 slot
	ps.info.type = PT_PLAYER;
	ps.info.actionPort = ERROR_ACTION_PORT; // PlayerInfo 特有字段
	ps.info.ping = 0; // PlayerInfo 特有字段
	
	mPlayerSessions.push_back(ps);
	mPlayerInfosDirty = true;
	
	// ✅ 同步到 SVPlayerManager
	if (mPlayerManager)
	{
		// ✅ 使用 SSessionPlayer（不需要 NetClientData）
		auto* sessionPlayer = new SSessionPlayer(this);
		
		// 使用 SVPlayerManager::insertPlayer 來添加 player
		mPlayerManager->insertPlayer(sessionPlayer, name, PT_PLAYER);
		
		// insertPlayer 會設置 playerId，確保匹配
		sessionPlayer->getInfo().playerId = newId;
		sessionPlayer->getInfo().slot = assignedSlot;  // ✅ 同步 slot
	}
	
	fireEvent(ENetSessionEvent::PlayerJoined, newId);
	
	return newId;
}

// ✅ 為 Server 創建 local player
PlayerId NetSessionHostImpl::createLocalPlayer(char const* name)
{
	// Local player 不關聯 SessionId，使用特殊值 0
	return createPlayer(0, name);
}

void NetSessionHostImpl::removePlayer(PlayerId id)
{
	for (auto it = mPlayerSessions.begin(); it != mPlayerSessions.end(); ++it)
	{
		if (it->playerId == id)
		{
			// ✅ 同步到 SVPlayerManager
			if (mPlayerManager)
			{
				mPlayerManager->removePlayer(id);
			}
			
			fireEvent(ENetSessionEvent::PlayerLeft, id);
			
			mPlayerSessions.erase(it);
			mPlayerInfosDirty = true;
			return;
		}
	}
}

NetSessionHostImpl::PlayerSession* NetSessionHostImpl::findPlayerBySession(SessionId sessionId)
{
	for (auto& ps : mPlayerSessions)
	{
		if (ps.sessionId == sessionId)
			return &ps;
	}
	return nullptr;
}

NetSessionHostImpl::PlayerSession* NetSessionHostImpl::findPlayer(PlayerId id)
{
	for (auto& ps : mPlayerSessions)
	{
		if (ps.playerId == id)
			return &ps;
	}
	return nullptr;
}

SessionId NetSessionHostImpl::getPlayerSession(PlayerId id)
{
	PlayerSession* ps = findPlayer(id);
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
	
	// 註冊封包工廠到 PacketEvaluator  
	auto& evaluator = mTransport->getPacketEvaluator();
	evaluator.addFactory<CPLogin>();
	evaluator.addFactory<SPPlayerStatus>();
	evaluator.addFactory<SPLevelInfo>();
	evaluator.addFactory<CSPPlayerState>();
	evaluator.addFactory<CSPClockSynd>();
	evaluator.addFactory<CSPComMsg>();
	evaluator.addFactory<SPServerInfo>();
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
	callbacks.onPacketReceived = [this](SessionId id, IComPacket* packet) {
		onPacketReceived(id, packet);
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
	// 會話層更新邏輯
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
	statePacket.playerId = mLocalPlayerId;
	sendReliable(&statePacket);
	
	mClientTransport->disconnect();
	mState = ENetSessionState::Disconnected;
	mLocalPlayerId = ERROR_PLAYER_ID;
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
	statePacket.playerId = mLocalPlayerId;
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
	// 註冊 Client 需要處理的核心封包
	
	// 登入回應封包
	registerPacketHandler(CPLogin::PID, [this](PlayerId id, IComPacket* packet) {
		procLoginResponse(packet);
	});
	
	// 玩家狀態列表封包
	registerPacketHandler(SPPlayerStatus::PID, [this](PlayerId id, IComPacket* packet) {
		procPlayerStatus(packet);
	});
	
	// 關卡資訊封包
	registerPacketHandler(SPLevelInfo::PID, [this](PlayerId id, IComPacket* packet) {
		// TODO: 實作關卡資訊處理
		LogMsg("Received SPLevelInfo");
	});
	
	// 玩家狀態變更封包
	registerPacketHandler(CSPPlayerState::PID, [this](PlayerId id, IComPacket* packet) {
		// TODO: 實作狀態變更處理
		LogMsg("Received CSPPlayerState");
	});
	
	LogMsg("Client: Registered %d packet handlers", 4);
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
	mLocalPlayerId = ERROR_PLAYER_ID;
	fireEvent(ENetSessionEvent::ConnectionLost);
}

void NetSessionClientImpl::onConnectionFailed()
{
	LogMsg("Failed to connect to server");
	
	mState = ENetSessionState::Disconnected;
	fireEvent(ENetSessionEvent::LoginFailed);
}

void NetSessionClientImpl::onPacketReceived(SessionId id, IComPacket* packet)
{
	dispatchPacket(SERVER_PLAYER_ID, packet);
	
	// ⚠️ Session 層負責在分派後刪除 packet
	delete packet;
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
		info.isLocal = (info.id == mLocalPlayerId);
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
