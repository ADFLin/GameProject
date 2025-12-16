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

/**
 * @brief Server 端會話實作
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
	void handleLoginRequest(PlayerId id, class CPLogin* packet);
	void handlePlayerReady(PlayerId id, class CSPPlayerState* packet);
	void handleClockSync(PlayerId id, class CSPClockSynd* packet);
	
	// 輔助方法
	bool isAllPlayersReady() const;
	
	// 玩家 Session 映射
	struct PlayerSession
	{
		PlayerId playerId;
		SessionId sessionId;
		NetPlayerInfo info;  // 玩家信息
	};
	
	PlayerId createPlayer(SessionId sessionId, char const* name);
	void removePlayer(PlayerId id);
	PlayerSession* findPlayerBySession(SessionId sessionId);
	PlayerSession* findPlayer(PlayerId id);
	SessionId getPlayerSession(PlayerId id);
	
private:
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
