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
	PlayerId mUserPlayerId = ERROR_PLAYER_ID;
	
	// 封包處理器
	using PacketHandler = std::function<void(PlayerId, IComPacket*)>;
	std::unordered_map<ComID, PacketHandler> mPacketHandlers;


	PacketDispatcher mPacketDispatcher;
	
	// 註冊核心封包處理
	virtual void registerCorePacketHandlers() = 0;
	
public:
	void procHandlerCommand(IComPacket* cp);
	
protected:
	
	// 狀態變更
	bool tryChangeState(ENetSessionState newState);
	virtual bool isValidStateTransition(ENetSessionState from, ENetSessionState to) const;


	void doUpdate(long time)
	{
		if (mComListener)
		{
			if (mComListener->prevProcCommand())
			{
				mPacketDispatcher.procCommand(*mComListener);
				mComListener->postProcCommand();
			}
		}
		else
		{
			mPacketDispatcher.procCommand();
		}
	}

	ComListener* mComListener = nullptr;
};

// 前置聲明
class NetSessionHostImpl;


/**
 * @brief Session 層專用的 ServerPlayer 實現
 * 
 * ✅ 合併了 PlayerSession 的所有數據，統一玩家管理
 * 通過 NetSessionHostImpl 發送數據，不需要 NetClientData
 */
class SSessionPlayer : public ServerPlayer
{
public:
	SSessionPlayer(NetSessionHostImpl* session, bool bNet = true);
	void sendCommand(int channel, IComPacket* cp) override;
	void sendTcpCommand(IComPacket* cp) override;
	void sendUdpCommand(IComPacket* cp) override;
	
	// ✅ Session 映射數據（來自 PlayerSession）
	PlayerId playerId = ERROR_PLAYER_ID;
	SessionId sessionId = 0;
	
	// ✅ 玩家狀態標誌使用 ServerPlayer::mFlag (繼承自 ServerPlayer)
	// 可用的標誌：eReady, eSyndDone, ePause, eLevelSetup, eLevelLoaded, eLevelReady, eDissconnect
	
	// 網絡延遲 (ms) - 繼承自 ServerPlayer::latency
	
private:
	NetSessionHostImpl* mSession;
};

class SUserSessionPlayer : public SSessionPlayer
{
public:
	SUserSessionPlayer(NetSessionHostImpl* session)
		:SSessionPlayer(session, false)
	{
	}

	void sendCommand(int channel, IComPacket* cp);
	void sendTcpCommand(IComPacket* cp);
	void sendUdpCommand(IComPacket* cp);

	LocalWorker* mLocalPlayer;
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

	PacketDispatcher& getPacketDispatcher() { return mPacketDispatcher; }

	
	ENetSessionState getState() const override { return mState; }
	bool changeState(ENetSessionState newState) override;
	
	// INetSession::getPlayerManager() - 返回 SVPlayerManager (向後兼容)
	IPlayerManager* getPlayerManager() override { return mPlayerManager.get(); }
	PlayerId getUserPlayerId() const override { return mUserPlayerId; }
	
	// ✅ 直接訪問 SVPlayerManager (兼容舊代碼)
	SVPlayerManager* getSVPlayerManager() { return mPlayerManager.get(); }
	
	void getPlayerInfos(TArray<NetPlayerInfo>& outInfos) const override;
	NetPlayerInfo const* getPlayerInfo(PlayerId id) const override;
	
	void registerPacketHandler(ComID packetId, INetSession::PacketHandler handler) override;
	void unregisterPacketHandler(ComID packetId) override;
	
	void sendReliable(IComPacket* cp) override;
	void sendUnreliable(IComPacket* cp) override;
	void sendReliableTo(PlayerId targetId, IComPacket* cp) override;
	

	void dispatchLocalPacket(IComPacket* packet);

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
	
	PlayerId createUserPlayer(char const* name); // ✅ 為 Server 創建 local player
	
	bool hasLocalPlayer() const { return mUserPlayerId != ERROR_PLAYER_ID; }
	
protected:
	void registerCorePacketHandlers() override;
	
	// Transport 回調
	void onConnectionEstablished(SessionId id);
	void onConnectionClosed(SessionId id, ENetCloseReason reason);
	void onPacketReceived(IComPacket* packet);
	void onUdpPacketReceived(IComPacket* packet, NetAddress const& clientAddr);
	
	// 房間搜尋處理（會話層功能）
	void handleServerInfoRequest(NetAddress const& clientAddr);
	
	// 獲取伺服器資訊（可被子類覆蓋以自定義）
	virtual void getServerInfo(class SPServerInfo& outInfo);

	
	// PacketDispatcher 處理器方法（用於 setUserFunc）
	void procLoginPacket(IComPacket* cp);
	void procPlayerStatePacket(IComPacket* cp);
	void procClockSyncPacket(IComPacket* cp);
	void procEchoPacket(IComPacket* cp);

	void procMsgPacket(IComPacket* cp)
	{
		broadcastReliable(cp);
	}


	template<typename Predicate>
	bool checkAllPlayersFlag(Predicate pred, bool networkOnly = false) const
	{
		if (!mPlayerManager->getPlayerNum() == 0)
			return false;
		
		// ✅ 遍历 SVPlayerManager 中的所有玩家
		for (size_t i = 0; i < mPlayerManager->getPlayerNum(); ++i)
		{
			ServerPlayer* player = mPlayerManager->getPlayer(i);
			if (player)
			{
				// ✅ 如果 networkOnly == true，跳过非网络玩家（local/user player）
				if (networkOnly && !player->isNetwork())
					continue;
					
				SSessionPlayer* sessionPlayer = static_cast<SSessionPlayer*>(player);
				if (!pred(*sessionPlayer))
					return false;
			}
		}
		return true;
	}

	PlayerId createPlayer(SessionId sessionId, char const* name);
	PlayerId createPlayerInternal(SessionId sessionId, char const* name, bool bUser);

	void removePlayer(PlayerId id);
	SSessionPlayer* findPlayerBySession(SessionId sessionId);
	SSessionPlayer* findPlayer(PlayerId id);
	SessionId getPlayerSession(PlayerId id);
	
private:
	// ✅ 完全使用 SVPlayerManager 管理玩家
	std::unique_ptr<SVPlayerManager> mPlayerManager;
	IServerTransport* mServerTransport = nullptr;
	
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
	PlayerId getUserPlayerId() const override { return mUserPlayerId; }
	
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
	void onPacketReceived(IComPacket* packet);
	
	// 封包處理
	void procLoginResponse(IComPacket* cp);
	void procPlayerStatus(IComPacket* cp);
	void procPlayerState(IComPacket* cp);
	void procLevelInfo(IComPacket* cp);
	
	// PacketDispatcher 處理器方法（用於 setWorkerFunc）
	void procLoginResponsePacket(IComPacket* cp);
	void procPlayerStatusPacket(IComPacket* cp);
	void procPlayerStatePacket(IComPacket* cp);
	void procLevelInfoPacket(IComPacket* cp);

	
private:
	std::unique_ptr<CLPlayerManager> mPlayerManager;
	IClientTransport* mClientTransport = nullptr;
	
	TArray<NetPlayerInfo> mPlayerInfos;
	
	char mPlayerName[MAX_PLAYER_NAME_LENGTH] = {0};
	bool mIsReady = false;
	bool mDirectJoin = false;
};

#endif // NetSessionImpl_H_INCLUDED
