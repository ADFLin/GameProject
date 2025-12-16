#include "TinyGamePCH.h"
#include "NetTransportImpl.h"

//========================================
// NetTransportBase
//========================================

NetTransportBase::NetTransportBase()
{
}

NetTransportBase::~NetTransportBase()
{
	if (mbRunning)
	{
		closeNetwork();
	}
}

bool NetTransportBase::startNetwork()
{
	if (mbRunning)
		return true;
	
	mNetStartTime = SystemPlatform::GetTickCount();
	mNetRunningTimeSpan = 0;
	
	if (!doStartNetwork())
		return false;
	
#if TINY_USE_NET_THREAD
	mbRequestExitNetThread = 0;
	mSocketThread.start(SocketFunc(this, &NetTransportBase::entryNetThread));
#endif
	
	mbRunning = true;
	return true;
}

void NetTransportBase::closeNetwork()
{
	if (!mbRunning)
		return;
	
#if TINY_USE_NET_THREAD
	mbRequestExitNetThread = 1;
	mSocketThread.join();
#endif
	
	doCloseNetwork();
	
	mNetThreadCommands.clear();
	mGameThreadCommands.clear();
	
	mbRunning = false;
}

void NetTransportBase::update(long time)
{
#if !TINY_USE_NET_THREAD
	mNetRunningTimeSpan = SystemPlatform::GetTickCount() - mNetStartTime;
	update_NetThread(time);
#endif
	
	processGameThreadCommnads();
	doUpdate(time);
}

bool NetTransportBase::update_NetThread(long time)
{
	processNetThreadCommnads();
	return true;
}

void NetTransportBase::processGameThreadCommnads()
{
	processThreadCommandInternal(mGameThreadCommands
#if TINY_USE_NET_THREAD
		, mMutexGameThreadCommands
#endif
	);
}

void NetTransportBase::processNetThreadCommnads()
{
#if TINY_USE_NET_THREAD
	assert(IsInNetThread());
#endif
	processThreadCommandInternal(mNetThreadCommands
#if TINY_USE_NET_THREAD
		, mMutexNetThreadCommands
#endif
	);
}

void NetTransportBase::processThreadCommandInternal(TArray<std::function<void()>>& commands
#if TINY_USE_NET_THREAD
	, Mutex& mutex
#endif
)
{
#if TINY_USE_NET_THREAD
	MutexLock lock(mutex);
#endif
	for (auto const& command : commands)
	{
		command();
	}
	commands.clear();
}

#if TINY_USE_NET_THREAD
void NetTransportBase::entryNetThread()
{
	while (!mbRequestExitNetThread)
	{
		long time = SystemPlatform::GetTickCount();
		mNetRunningTimeSpan = time - mNetStartTime;
		
		if (!update_NetThread(time))
			break;
		
		SystemPlatform::Sleep(1);
	}
}
#endif

//========================================
// ServerTransportImpl
//========================================

ServerTransportImpl::ServerTransportImpl()
{
	mBase.mOwner = this;
	mListener.mOwner = this;
	
	mTcpServer.setListener(&mListener);
	mUdpServer.setListener(&mListener);
}

ServerTransportImpl::~ServerTransportImpl()
{
	closeNetwork();
}

bool ServerTransportImpl::startNetwork()
{
	return mBase.startNetwork();
}

void ServerTransportImpl::closeNetwork()
{
	mBase.closeNetwork();
}

bool ServerTransportImpl::ServerTransportImpl::TransportBase::doStartNetwork()
{
	if (!mOwner->mTcpServer.run(TG_TCP_PORT))
	{
		LogError("Server: Failed to open TCP port %d", TG_TCP_PORT);
		return false;
	}
	
	if (!mOwner->mUdpServer.run(TG_UDP_PORT))
	{
		LogError("Server: Failed to open UDP port %d", TG_UDP_PORT);
		return false;
	}
	
	mOwner->mNetSelect.addSocket(mOwner->mTcpServer.getSocket());
	mOwner->mNetSelect.addSocket(mOwner->mUdpServer.getSocket());
	
	LogMsg("Server transport started on TCP:%d UDP:%d", TG_TCP_PORT, TG_UDP_PORT);
	return true;
}

void ServerTransportImpl::TransportBase::doCloseNetwork()
{
	mOwner->mNetSelect.clear();
	mOwner->mTcpServer.close();
	mOwner->mUdpServer.close();
	mOwner->mClients.clear();
}

void ServerTransportImpl::TransportBase::doUpdate(long time)
{
	// Game thread 更新
}

bool ServerTransportImpl::TransportBase::update_NetThread(long time)
{
	NetTransportBase::update_NetThread(time);
	
	// Socket 更新
	mOwner->mNetSelect.select(1);
	
	// 處理 TCP 連線
	mOwner->mTcpServer.updateSocket(time, mOwner->mNetSelect);
	
	// 處理 UDP 資料
	mOwner->mUdpServer.updateSocket(time, mOwner->mNetSelect);
	
	return true;
}

bool ServerTransportImpl::sendPacket(SessionId targetId, ENetChannelType channel, IComPacket* cp)
{
	// TODO: 根據 targetId 找到對應的 Client 並發送
	return false;
}

bool ServerTransportImpl::broadcastPacket(ENetChannelType channel, IComPacket* cp)
{
	// TODO: 廣播給所有 Client
	return false;
}

void ServerTransportImpl::kickConnection(SessionId id)
{
	// TODO: 踢出指定連線
}

void ServerTransportImpl::getConnectionIds(TArray<SessionId>& outIds) const
{
	outIds.clear();
	for (auto const& client : mClients)
	{
		outIds.push_back(client.id);
	}
}

int ServerTransportImpl::getConnectionCount() const
{
	return (int)mClients.size();
}

void ServerTransportImpl::update(long time)
{
	mBase.update(time);
}

void ServerTransportImpl::doPostToGameThread(std::function<void()> func)
{
	mBase.addGameThreadCommand(std::move(func));
}

void ServerTransportImpl::doPostToNetThread(std::function<void()> func)
{
	mBase.addNetThreadCommand(std::move(func));
}

// Connection Listener

void ServerTransportImpl::ConnectionListener::notifyConnectionSend(NetConnection* con)
{
	// 可以發送資料了
}

bool ServerTransportImpl::ConnectionListener::notifyConnectionRecv(NetConnection* con, SocketBuffer& buffer, NetAddress* clientAddr)
{
	// 收到資料
	// TODO: 解析封包並呼叫回調
	return true;
}

void ServerTransportImpl::ConnectionListener::notifyConnectionAccpet(NetConnection* con)
{
	// 新連線接受
	SessionId newId = mOwner->mNextSessionId++;
	
	// 通知會話層
	if (mOwner->mCallbacks.onConnectionEstablished)
	{
		mOwner->postToGameThread([owner = mOwner, newId]() {
			owner->mCallbacks.onConnectionEstablished(newId);
		});
	}
}

void ServerTransportImpl::ConnectionListener::notifyConnectionClose(NetConnection* con, NetCloseReason reason)
{
	// 連線關閉
	// TODO: 找到對應的 SessionId
	SessionId id = 0;
	ENetCloseReason closeReason = ENetCloseReason::RemoteClose;
	
	if (mOwner->mCallbacks.onConnectionClosed)
	{
		mOwner->postToGameThread([owner = mOwner, id, closeReason]() {
			owner->mCallbacks.onConnectionClosed(id, closeReason);
		});
	}
}

//========================================
// ClientTransportImpl
//========================================

ClientTransportImpl::ClientTransportImpl()
	: mLatencyCalculator(300)
{
	mBase.mOwner = this;
	mListener.mOwner = this;
	
	mTcpClient.setListener(&mListener);
	mUdpClient.setListener(&mListener);
}

ClientTransportImpl::~ClientTransportImpl()
{
	closeNetwork();
}

bool ClientTransportImpl::startNetwork()
{
	return mBase.startNetwork();
}

void ClientTransportImpl::closeNetwork()
{
	mBase.closeNetwork();
}

bool ClientTransportImpl::ClientTransportImpl::TransportBase::doStartNetwork()
{
	// Client 啟動時只準備資源，不建立連線
	LogMsg("Client transport ready");
	return true;
}

void ClientTransportImpl::TransportBase::doCloseNetwork()
{
	mOwner->mTcpClient.close();
	mOwner->mUdpClient.close();
	mOwner->mNetSelect.clear();
	mOwner->mServerSessionId = 0;
}

void ClientTransportImpl::TransportBase::doUpdate(long time)
{
	// Game thread 更新
}

bool ClientTransportImpl::TransportBase::update_NetThread(long time)
{
	NetTransportBase::update_NetThread(time);
	
	if (mOwner->mServerSessionId != 0)
	{
		mOwner->mNetSelect.select(1);
		mOwner->mTcpClient.updateSocket(mOwner->mNetSelect);
		mOwner->mUdpClient.updateSocket();
	}
	
	return true;
}

void ClientTransportImpl::connect(char const* hostName, int port)
{
	if (port == 0)
		port = TG_TCP_PORT;
	
	LogMsg("Client connecting to %s:%d", hostName, port);
	
	mTcpClient.connect(hostName, port);
	mNetSelect.addSocket(mTcpClient.getSocket());
}

void ClientTransportImpl::disconnect()
{
	mTcpClient.close();
	mUdpClient.close();
	mNetSelect.clear();
	mServerSessionId = 0;
	
	if (mCallbacks.onConnectionClosed)
	{
		mCallbacks.onConnectionClosed(0, ENetCloseReason::LocalClose);
	}
}

bool ClientTransportImpl::sendPacket(SessionId targetId, ENetChannelType channel, IComPacket* cp)
{
	// Client 只能發送給 Server
	// TODO: 實作發送
	return false;
}

bool ClientTransportImpl::broadcastPacket(ENetChannelType channel, IComPacket* cp)
{
	// Client 不支援廣播
	return false;
}

void ClientTransportImpl::update(long time)
{
	mBase.update(time);
}

void ClientTransportImpl::doPostToGameThread(std::function<void()> func)
{
	mBase.addGameThreadCommand(std::move(func));
}

void ClientTransportImpl::doPostToNetThread(std::function<void()> func)
{
	mBase.addNetThreadCommand(std::move(func));
}

// Connection Listener

void ClientTransportImpl::ConnectionListener::notifyConnectionOpen(NetConnection* con)
{
	mOwner->mServerSessionId = 1;  // Server 的 SessionId 固定為 1
	
	if (mOwner->mCallbacks.onConnectionEstablished)
	{
		mOwner->postToGameThread([owner = mOwner]() {
			owner->mCallbacks.onConnectionEstablished(1);
		});
	}
}

void ClientTransportImpl::ConnectionListener::notifyConnectionClose(NetConnection* con, NetCloseReason reason)
{
	SessionId oldServerId = mOwner->mServerSessionId;
	mOwner->mServerSessionId = 0;
	
	ENetCloseReason closeReason = ENetCloseReason::RemoteClose;
	
	if (mOwner->mCallbacks.onConnectionClosed)
	{
		mOwner->postToGameThread([owner = mOwner, oldServerId, closeReason]() {
			owner->mCallbacks.onConnectionClosed(oldServerId, closeReason);
		});
	}
}

void ClientTransportImpl::ConnectionListener::notifyConnectionFail(NetConnection* con)
{
	if (mOwner->mCallbacks.onConnectionFailed)
	{
		mOwner->postToGameThread([owner = mOwner]() {
			owner->mCallbacks.onConnectionFailed();
		});
	}
}

void ClientTransportImpl::ConnectionListener::notifyConnectionSend(NetConnection* con)
{
	// 可以發送資料了
}

bool ClientTransportImpl::ConnectionListener::notifyConnectionRecv(NetConnection* con, SocketBuffer& buffer, NetAddress* clientAddr)
{
	// 收到資料
	// TODO: 解析封包並呼叫回調
	return true;
}
