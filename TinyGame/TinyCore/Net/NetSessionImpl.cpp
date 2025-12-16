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
		it->second(senderId, packet);
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
			mCachedPlayerInfos.push_back(ps.info);
		}
		mPlayerInfosDirty = false;
	}
	outInfos = mCachedPlayerInfos;
}

NetPlayerInfo const* NetSessionHostImpl::getPlayerInfo(PlayerId id) const
{
	for (auto const& ps : mPlayerSessions)
	{
		if (ps.playerId == id)
			return &ps.info;
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
	mServerTransport->broadcastPacket(ENetChannelType::TCP, cp);
}

void NetSessionHostImpl::sendUnreliable(IComPacket* cp)
{
	mServerTransport->broadcastPacket(ENetChannelType::UDP, cp);
}

void NetSessionHostImpl::sendReliableTo(PlayerId targetId, IComPacket* cp)
{
	SessionId sessionId = getPlayerSession(targetId);
	if (sessionId != 0)
	{
		mServerTransport->sendPacket(sessionId, ENetChannelType::TCP, cp);
	}
}

bool NetSessionHostImpl::createRoom(char const* gameName, int maxPlayers)
{
	if (mRoomOpen)
		return false;
	
	FCString::Copy(mGameName, ARRAY_SIZE(mGameName), gameName);
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
	// 註冊 Host 需要處理的核心封包
	// TODO: 添加封包處理
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
	dispatchPacket(senderId, packet);
}

PlayerId NetSessionHostImpl::createPlayer(SessionId sessionId, char const* name)
{
	if ((int)mPlayerSessions.size() >= mMaxPlayers)
		return ERROR_PLAYER_ID;
	
	PlayerId newId = mNextPlayerId++;
	
	PlayerSession ps;
	ps.playerId = newId;
	ps.sessionId = sessionId;
	ps.info.id = newId;
	FCString::Copy(ps.info.name, ARRAY_SIZE(ps.info.name), name);
	ps.info.slot = -1;
	ps.info.isLocal = false;
	ps.info.isReady = false;
	ps.info.playerType = (int)EPlayerType::Player;
	
	mPlayerSessions.push_back(ps);
	mPlayerInfosDirty = true;
	
	fireEvent(ENetSessionEvent::PlayerJoined, newId);
	
	return newId;
}

void NetSessionHostImpl::removePlayer(PlayerId id)
{
	for (auto it = mPlayerSessions.begin(); it != mPlayerSessions.end(); ++it)
	{
		if (it->playerId == id)
		{
			mPlayerSessions.erase(it);
			mPlayerInfosDirty = true;
			break;
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
	mClientTransport->sendToServer(ENetChannelType::TCP, cp);
}

void NetSessionClientImpl::sendUnreliable(IComPacket* cp)
{
	mClientTransport->sendToServer(ENetChannelType::UDP, cp);
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
	
	FCString::Copy(mPlayerName, ARRAY_SIZE(mPlayerName), playerName);
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
	
	FCString::Copy(mPlayerName, ARRAY_SIZE(mPlayerName), playerName);
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
	// TODO: 添加封包處理
}

void NetSessionClientImpl::onConnectionEstablished(SessionId id)
{
	LogMsg("Connected to server");
	
	changeState(ENetSessionState::Connected);
	fireEvent(ENetSessionEvent::ConnectionEstablished);
	
	// 發送登入請求
	CPSLogin loginPacket;
	FCString::Copy(loginPacket.name, ARRAY_SIZE(loginPacket.name), mPlayerName);
	// TODO: 設定其他登入資訊
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
}

void NetSessionClientImpl::procLoginResponse(IComPacket* cp)
{
	SPLogin* loginPacket = cp->cast<SPLogin>();
	
	if (loginPacket->result == LGR_SUCCESS)
	{
		mLocalPlayerId = loginPacket->id;
		
		fireEvent(ENetSessionEvent::LoginSuccess);
		
		if (mDirectJoin)
		{
			// 直接加入關卡
			changeState(ENetSessionState::LevelLoading);
		}
		else
		{
			// 進入房間
			changeState(ENetSessionState::InRoom);
		}
	}
	else
	{
		fireEvent(ENetSessionEvent::LoginFailed);
		mClientTransport->disconnect();
	}
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
		FCString::Copy(info.name, ARRAY_SIZE(info.name), statusPacket->info[i]->name);
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
