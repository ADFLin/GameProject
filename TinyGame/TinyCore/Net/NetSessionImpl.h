#pragma once
#ifndef NetSessionImpl_H_INCLUDED
#define NetSessionImpl_H_INCLUDED

#include "INetSession.h"
#include "INetTransport.h"
#include "GamePlayer.h"
#include "GameServer.h"
#include "GameClient.h"
#include "ComPacket.h"

#include <unordered_map>
#include <memory>

/**
 * @brief 會話層基礎實作
 */
class NetSessionBase
{
public:
	NetSessionBase();
	virtual ~NetSessionBase();
	
protected:
	INetTransport* mTransport = nullptr;
	ENetSessionState mState = ENetSessionState::Disconnected;
	PlayerId mLocalPlayerId = ERROR_PLAYER_ID;
	
	// 封包處理器
	using PacketHandler = std::function<void(PlayerId, IComPacket*)>;
	std::unordered_map<ComID, PacketHandler> mPacketHandlers;
	
	// 註冊核心封包處理
	virtual void registerCorePacketHandlers() = 0;
	
	// 處理收到的封包
	void dispatchPacket(PlayerId senderId, IComPacket* packet);
	
	// 狀態變更
	bool tryChangeState(ENetSessionState newState);
	virtual bool isValidStateTransition(ENetSessionState from, ENetSessionState to) const;
};

// 前置聲明
class NetSessionHostImpl;

/**
 * @brief Session 層專用的 ServerPlayer 實現
 * 
 * 通過 NetSessionHostImpl 發送數據，不需要 NetClientData
 */
class SSessionPlayer : public ServerPlayer
{
public:
	SSessionPlayer(NetSessionHostImpl* session);
	void sendCommand(int channel, IComPacket* cp) override;
	void sendTcpCommand(IComPacket* cp) override;
	void sendUdpCommand(IComPacket* cp) override;
private:
	NetSessionHostImpl* mSession;
};

/**
 * @brief Server 端會話實作
 * 
 * ✅ 直接實現 IPlayerManager 介面，統一玩家資料管理
 */
class NetSessionHostImpl : public INetSessionHost, protected NetSessionBase
{
public:
	TINY_API NetSessionHostImpl();
	TINY_API ~NetSessionHostImpl();
	
	//========================================
	// INetSession
	//========================================
	bool initialize(INetTransport* transport) override;
	void shutdown() override;
	void update(long time) override;
	
	ENetSessionState getState() const override { return mState; }
	bool changeState(ENetSessionState newState) override;
	
	// INetSession::getPlayerManager() - 返回 SVPlayerManager (向後兼容)
	IPlayerManager* getPlayerManager() override { return mPlayerManager.get(); }
	PlayerId getLocalPlayerId() const override { return mLocalPlayerId; }
	
	// ✅ 直接訪問 SVPlayerManager (兼容舊代碼)
	SVPlayerManager* getSVPlayerManager() { return mPlayerManager.get(); }
	
	void getPlayerInfos(TArray<NetPlayerInfo>& outInfos) const override;
	NetPlayerInfo const* getPlayerInfo(PlayerId id) const override;
	
	void registerPacketHandler(ComID packetId, INetSession::PacketHandler handler) override;
	void unregisterPacketHandler(ComID packetId) override;
	
	void sendReliable(IComPacket* cp) override;
	void sendUnreliable(IComPacket* cp) override;
	void sendReliableTo(PlayerId targetId, IComPacket* cp) override;
	
	//========================================
	// INetSessionHost
	//========================================
	bool createRoom(char const* gameName, int maxPlayers) override;
	void closeRoom() override;
	bool isRoomOpen() const override { return mRoomOpen; }
	
	void setAllowLateJoin(bool allow) override { mAllowLateJoin = allow; }
	bool isAllowLateJoin() const override { return mAllowLateJoin; }
	
	bool kickPlayer(PlayerId id) override;
	void broadcastReliable(IComPacket* cp) override;
	void broadcastUnreliable(IComPacket* cp) override;
	
	bool startLevel(GameLevelInfo const& info) override;
	bool startLevelDirect(GameLevelInfo const& info) override;
	void endLevel() override;
	void pauseLevel() override;
	void resumeLevel() override;
	void restartLevel() override;
	
	PlayerId createLocalPlayer(char const* name); // ✅ 為 Server 創建 local player
	
	// ✅ Packet Observer 機制
	// 允許多個層級（Game, System, UI等）監聽和處理 packet
	// Session 層處理完後，會通知所有註冊的 observers
	using PacketObserver = std::function<void(PlayerId senderId, IComPacket* packet)>;
	
	// 註冊 packet observer（觀察者可以有多個）
	void addPacketObserver(ComID packetId, PacketObserver observer);
	
	// 移除所有 observers
	void clearPacketObservers();
	
protected:
	void registerCorePacketHandlers() override;
	
	// Transport 回調
	void onConnectionEstablished(SessionId id);
	void onConnectionClosed(SessionId id, ENetCloseReason reason);
	void onPacketReceived(SessionId id, IComPacket* packet);
	void onUdpPacketReceived(SessionId id, IComPacket* packet, NetAddress const& clientAddr);
	
	// 房間搜尋處理（會話層功能）
	void handleServerInfoRequest(NetAddress const& clientAddr);
	
	// 獲取伺服器資訊（可被子類覆蓋以自定義）
	virtual void getServerInfo(class SPServerInfo& outInfo);
	
	// 核心封包處理（會話層功能）
	void handleLoginRequest(SessionId sessionId, class CPLogin* packet);
	void handlePlayerState(PlayerId id, class CSPPlayerState* packet);
	void handleClockSync(PlayerId id, class CSPClockSynd* packet);
	
	// 輔助方法
	bool isAllPlayersReady() const;
	void notifyPacketObservers(ComID packetId, PlayerId senderId, IComPacket* packet);
	
	// 玩家 Session 映射
	struct PlayerSession
	{
		PlayerId playerId;
		SessionId sessionId;
		
		// ✅ 使用 PlayerInfo 而不是 NetPlayerInfo，因為 GamePlayer::init() 需要 PlayerInfo&
		PlayerInfo info;  // 玩家信息 (PlayerInfo 包含所有 NetPlayerInfo 的字段 + actionPort, ping)
		
		// ⚠️ PlayerInfo 沒有 isReady 字段，需要額外追蹤
		bool isReady = false;
		
		// ✅ 為了實現 IPlayerManager，我們需要提供 GamePlayer 對象
		GamePlayer player;   // GamePlayer wrapper (通過 PlayerInfo 初始化)
		
		PlayerSession()
		{
			// GamePlayer 會透過 init() 關聯到 PlayerInfo
			player.init(info);
		}
	};
	
	PlayerId createPlayer(SessionId sessionId, char const* name);

	void removePlayer(PlayerId id);
	PlayerSession* findPlayerBySession(SessionId sessionId);
	PlayerSession* findPlayer(PlayerId id);
	SessionId getPlayerSession(PlayerId id);
	
private:
	// ✅ Session 層維護雙重 player 管理
	// 1. PlayerSessions: Session 層的 player 資料 (主要來源)
	// 2. SVPlayerManager: Game 層的 player 管理 (向後兼容)
	std::unique_ptr<SVPlayerManager> mPlayerManager;
	IServerTransport* mServerTransport = nullptr;
	
	TArray<PlayerSession> mPlayerSessions;
	PlayerId mNextPlayerId = 1;
	
	bool mRoomOpen = false;
	bool mAllowLateJoin = false;
	char mGameName[64] = {0};
	int mMaxPlayers = MAX_PLAYER_NUM;
	
	// 快取的玩家資訊
	mutable TArray<NetPlayerInfo> mCachedPlayerInfos;
	mutable bool mPlayerInfosDirty = true;
	
	// ✅ Packet Observers（支持多個觀察者）
	// Key: PacketID, Value: 觀察者列表
	std::unordered_map<ComID, std::vector<PacketObserver>> mPacketObservers;
};

/**
 * @brief Client 端會話實作
 */
class NetSessionClientImpl : public INetSessionClient, protected NetSessionBase
{
public:
	TINY_API NetSessionClientImpl();
	TINY_API ~NetSessionClientImpl();
	
	//========================================
	// INetSession
	//========================================
	bool initialize(INetTransport* transport) override;
	void shutdown() override;
	void update(long time) override;
	
	ENetSessionState getState() const override { return mState; }
	bool changeState(ENetSessionState newState) override;
	
	IPlayerManager* getPlayerManager() override { return mPlayerManager.get(); }
	PlayerId getLocalPlayerId() const override { return mLocalPlayerId; }
	
	void getPlayerInfos(TArray<NetPlayerInfo>& outInfos) const override;
	NetPlayerInfo const* getPlayerInfo(PlayerId id) const override;
	
	void registerPacketHandler(ComID packetId, INetSession::PacketHandler handler) override;
	void unregisterPacketHandler(ComID packetId) override;
	
	void sendReliable(IComPacket* cp) override;
	void sendUnreliable(IComPacket* cp) override;
	void sendReliableTo(PlayerId targetId, IComPacket* cp) override;
	
	//========================================
	// INetSessionClient
	//========================================
	void joinServer(char const* hostName, char const* playerName) override;
	void leaveServer() override;
	void joinLevelDirect(char const* hostName, char const* playerName) override;
	
	void setReady(bool ready) override;
	bool isReady() const override { return mIsReady; }
	
	void searchLanServers() override;
	
protected:
	void registerCorePacketHandlers() override;
	
	// Transport 回調
	void onConnectionEstablished(SessionId id);
	void onConnectionClosed(SessionId id, ENetCloseReason reason);
	void onConnectionFailed();
	void onPacketReceived(SessionId id, IComPacket* packet);
	
	// 封包處理
	void procLoginResponse(IComPacket* cp);
	void procPlayerStatus(IComPacket* cp);
	void procPlayerState(IComPacket* cp);
	void procLevelInfo(IComPacket* cp);
	
private:
	std::unique_ptr<CLPlayerManager> mPlayerManager;
	IClientTransport* mClientTransport = nullptr;
	
	TArray<NetPlayerInfo> mPlayerInfos;
	
	char mPlayerName[MAX_PLAYER_NAME_LENGTH] = {0};
	bool mIsReady = false;
	bool mDirectJoin = false;
};

#endif // NetSessionImpl_H_INCLUDED
