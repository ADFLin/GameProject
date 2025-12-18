#pragma once
#ifndef NetTransportImpl_H_INCLUDED
#define NetTransportImpl_H_INCLUDED

#include "INetTransport.h"
#include "GameNetConnect.h"
#include "SocketBuffer.h"
#include "PlatformThread.h"
#include "PacketFactory.h"


/**
 * @brief 傳輸層基礎實作
 * 
 * 從舊的 NetWorker 核心功能衍生
 * 只包含純傳輸功能，不包含遊戲/會話邏輯
 */
class NetTransportBase
{
public:
	NetTransportBase();
	virtual ~NetTransportBase();
	
	//========================================
	// 生命週期
	//========================================
	bool startNetwork();
	void closeNetwork();
	bool isRunning() const { return mbRunning; }
	
	//========================================
	// 狀態
	//========================================
	long getNetRunningTime() const { return mNetRunningTimeSpan; }
	
	//========================================
	// 更新
	//========================================
	void update(long time);
	
protected:
	// 子類必須實作
	virtual bool doStartNetwork() = 0;
	virtual void doCloseNetwork() = 0;
	virtual void doUpdate(long time) {}
	virtual bool update_NetThread(long time);
	
	// 執行緒相關
	void processGameThreadCommnads();
	void processNetThreadCommnads();
	
public:
	// 執行緒命令添加（公開給子類和外部使用）
	template<typename Func>
	void addGameThreadCommand(Func&& func);
	
	template<typename Func>
	void addNetThreadCommand(Func&& func);
	
protected:
	bool mbRunning = false;
	int64 mNetRunningTimeSpan = 0;
	int64 mNetStartTime = 0;
	
	// 執行緒命令隊列
#if TINY_USE_NET_THREAD
	typedef fastdelegate::FastDelegate<void()> SocketFunc;
	typedef TFunctionThread<SocketFunc> SocketThread;
	SocketThread mSocketThread;
	volatile int32 mbRequestExitNetThread = 0;
	
	Mutex mMutexNetThreadCommands;
	Mutex mMutexGameThreadCommands;
	
	void entryNetThread();
#endif
	
	TArray<std::function<void()>> mNetThreadCommands;
	TArray<std::function<void()>> mGameThreadCommands;
	
	void processThreadCommandInternal(TArray<std::function<void()>>& commands
#if TINY_USE_NET_THREAD
		, Mutex& mutex
#endif
	);
};

/**
 * @brief Server 傳輸層實作
 */
class ServerTransportImpl : public IServerTransport
{
public:
	TINY_API ServerTransportImpl();
	TINY_API ~ServerTransportImpl();
	
	//========================================
	// INetTransport
	//========================================
	bool startNetwork() override;
	void closeNetwork() override;
	bool isRunning() const override { return mBase.isRunning(); }
	
	bool sendPacket(SessionId targetId, ENetChannelType channel, IComPacket* cp) override;
	bool broadcastPacket(ENetChannelType channel, IComPacket* cp) override;
	
	long getNetLatency() const override { return 0; /* Server 無延遲 */ }
	long getNetRunningTime() const override { return mBase.getNetRunningTime(); }
	
	// PacketFactory 管理
	void setPacketFactory(PacketFactory* factory) override;
	PacketFactory* getPacketFactory() override;
	
	//========================================
	// IServerTransport
	//========================================
	void kickConnection(SessionId id) override;
	void getConnectionIds(TArray<SessionId>& outIds) const override;
	int getConnectionCount() const override;
	
	//========================================
	// 更新
	//========================================
	TINY_API void update(long time);
	
	//========================================
	// 內部存取
	//========================================
	TcpServer& getTcpServer() { return mTcpServer; }
	UdpServer& getUdpServer() { return mUdpServer; }
	NetSelectSet& getNetSelect() { return mNetSelect; }
	
	// UDP 封包發送 (用於房間搜尋等功能)
	TINY_API bool sendUdpPacket(IComPacket* packet, NetAddress const& addr);
	
protected:
	void doPostToGameThread(std::function<void()> func) override;
	void doPostToNetThread(std::function<void()> func) override;
	
private:
	class TransportBase : public NetTransportBase
	{
	public:
		ServerTransportImpl* mOwner;
		bool doStartNetwork() override;
		void doCloseNetwork() override;
		void doUpdate(long time) override;
		bool update_NetThread(long time) override;
	};
	
	class ConnectionListener : public INetConnectListener
	{
	public:
		ServerTransportImpl* mOwner;
		void notifyConnectionSend(NetConnection* con) override;
		bool notifyConnectionRecv(NetConnection* con, SocketBuffer& buffer, NetAddress* clientAddr) override;
		void notifyConnectionAccpet(NetConnection* con) override;
		void notifyConnectionClose(NetConnection* con, NetCloseReason reason) override;
	};
	
	TransportBase mBase;
	ConnectionListener mListener;
	
	TcpServer mTcpServer;
	UdpServer mUdpServer;
	NetSelectSet mNetSelect;
	
	// Client 管理
	struct ClientData
	{
		SessionId id;
		TcpServer::Client* tcpClient;
		NetAddress udpAddr;
	};
	TArray<ClientData> mClients;
	SessionId mNextSessionId = 1;
	
	// 封包解析器
	PacketFactory* mPacketFactory = nullptr; // 外部設定的 factory
	PacketFactory mDefaultPacketFactory;      // 內部默認 factory
};

/**
 * @brief Client 傳輸層實作
 */
class ClientTransportImpl : public IClientTransport
{
public:
	TINY_API ClientTransportImpl();
	TINY_API ~ClientTransportImpl();
	
	//========================================
	// INetTransport
	//========================================
	bool startNetwork() override;
	void closeNetwork() override;
	bool isRunning() const override { return mBase.isRunning(); }
	
	bool sendPacket(SessionId targetId, ENetChannelType channel, IComPacket* cp) override;
	bool broadcastPacket(ENetChannelType channel, IComPacket* cp) override;
	
	long getNetLatency() const override { return mNetLatency; }
	long getNetRunningTime() const override { return mBase.getNetRunningTime(); }
	
	// PacketFactory 管理
	void setPacketFactory(PacketFactory* factory) override;
	PacketFactory* getPacketFactory() override;
	
	//========================================
	// IClientTransport
	//========================================
	void connect(char const* hostName, int port) override;
	void disconnect() override;
	bool isConnected() const override { return mServerSessionId != 0; }
	SessionId getServerSessionId() const override { return mServerSessionId; }
	
	//========================================
	// 更新
	//========================================
	TINY_API void update(long time);
	
protected:
	void doPostToGameThread(std::function<void()> func) override;
	void doPostToNetThread(std::function<void()> func) override;
	
private:
	class TransportBase : public NetTransportBase
	{
	public:
		ClientTransportImpl* mOwner;
		bool doStartNetwork() override;
		void doCloseNetwork() override;
		void doUpdate(long time) override;
		bool update_NetThread(long time) override;
	};
	
	class ConnectionListener : public INetConnectListener
	{
	public:
		ClientTransportImpl* mOwner;
		void notifyConnectionOpen(NetConnection* con) override;
		void notifyConnectionClose(NetConnection* con, NetCloseReason reason) override;
		void notifyConnectionFail(NetConnection* con) override;
		void notifyConnectionSend(NetConnection* con) override;
		bool notifyConnectionRecv(NetConnection* con, SocketBuffer& buffer, NetAddress* clientAddr) override;
	};
	
	TransportBase mBase;
	ConnectionListener mListener;
	
	TcpClient mTcpClient;
	UdpClient mUdpClient;
	NetSelectSet mNetSelect;
	
	SessionId mServerSessionId = 0;
	long mNetLatency = 0;
	LatencyCalculator mLatencyCalculator;
	
	// 封包解析器
	PacketFactory* mPacketFactory = nullptr; // 外部設定的 factory
	PacketFactory mDefaultPacketFactory;      // 內部默認 factory
};

//========================================
// 內聯實作
//========================================

template<typename Func>
inline void NetTransportBase::addGameThreadCommand(Func&& func)
{
#if TINY_USE_NET_THREAD
	MutexLock lock(mMutexGameThreadCommands);
#endif
	mGameThreadCommands.push_back(std::forward<Func>(func));
}

template<typename Func>
inline void NetTransportBase::addNetThreadCommand(Func&& func)
{
#if TINY_USE_NET_THREAD
	MutexLock lock(mMutexNetThreadCommands);
#endif
	mNetThreadCommands.push_back(std::forward<Func>(func));
}

#endif // NetTransportImpl_H_INCLUDED
