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

class NetSessionBase
{
public:
	NetSessionBase();
	virtual ~NetSessionBase();

protected:
	INetTransport* mTransport = nullptr;
	ENetSessionState mState = ENetSessionState::Disconnected;
	PlayerId mUserPlayerId = ERROR_PLAYER_ID;
	
	using PacketHandler = std::function<void(PlayerId, IComPacket*)>;
	std::unordered_map<ComID, PacketHandler> mPacketHandlers;


	PacketDispatcher mPacketDispatcher;
	
	virtual void registerCorePacketHandlers() = 0;
	
public:
	void procHandlerCommand(IComPacket* cp);
	
protected:
	
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

class NetSessionHostImpl;


class SSessionPlayer : public ServerPlayer
{
public:
	SSessionPlayer(NetSessionHostImpl* session, bool bNet = true);
	void sendCommand(int channel, IComPacket* cp) override;
	void sendTcpCommand(IComPacket* cp) override;
	void sendUdpCommand(IComPacket* cp) override;
	
	PlayerId playerId = ERROR_PLAYER_ID;
	SessionId sessionId = 0;
	
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
	
	IPlayerManager* getPlayerManager() override { return mPlayerManager.get(); }
	PlayerId getUserPlayerId() const override { return mUserPlayerId; }
	
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
	
	void onConnectionEstablished(SessionId id);
	void onConnectionClosed(SessionId id, ENetCloseReason reason);
	void onPacketReceived(IComPacket* packet);
	void onUdpPacketReceived(IComPacket* packet, NetAddress const& clientAddr);
	
	void handleServerInfoRequest(NetAddress const& clientAddr);
	
	virtual void getServerInfo(class SPServerInfo& outInfo);

	
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
		
		for (size_t i = 0; i < mPlayerManager->getPlayerNum(); ++i)
		{
			ServerPlayer* player = mPlayerManager->getPlayer(i);
			if (player)
			{
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
	std::unique_ptr<SVPlayerManager> mPlayerManager;
	IServerTransport* mServerTransport = nullptr;
	
	PlayerId mNextPlayerId = 1;
	
	bool mRoomOpen = false;
	bool mAllowLateJoin = false;
	char mGameName[64] = {0};
	int mMaxPlayers = MAX_PLAYER_NUM;
	
	mutable TArray<NetPlayerInfo> mCachedPlayerInfos;
	mutable bool mPlayerInfosDirty = true;

};

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
	
	void onConnectionEstablished(SessionId id);
	void onConnectionClosed(SessionId id, ENetCloseReason reason);
	void onConnectionFailed();
	void onPacketReceived(IComPacket* packet);
	
	void procLoginResponse(IComPacket* cp);
	void procPlayerStatus(IComPacket* cp);
	void procPlayerState(IComPacket* cp);
	void procLevelInfo(IComPacket* cp);
	
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
