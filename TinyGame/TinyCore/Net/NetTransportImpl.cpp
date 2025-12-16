#include "TinyGamePCH.h"
#include "NetTransportImpl.h"
#include "GameWorker.h"

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

bool ServerTransportImpl::TransportBase::doStartNetwork()
{
	mOwner->mTcpServer.run(TG_TCP_PORT);
	if (mOwner->mTcpServer.getSocket().getState() != SKS_LISTING)
	{
		LogError("Server: Failed to open TCP port %d", TG_TCP_PORT);
		return false;
	}
	
	mOwner->mUdpServer.run(TG_UDP_PORT);
	if (mOwner->mUdpServer.getSocket().getState() == SKS_CLOSE)
	{
		LogError("Server: Failed to open UDP port %d", TG_UDP_PORT);
		return false;
	}
	
	// ✅ 设置监听器！这是关键 - 没有这个就收不到数据回调
	mOwner->mTcpServer.setListener(&mOwner->mListener);
	mOwner->mUdpServer.setListener(&mOwner->mListener);
	
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
	// 在 Net Thread 中執行實際發送
	postToNetThread([this, targetId, channel, cp]() {
		// 找到對應的 Client
		for (auto& client : mClients)
		{
			if (client.id == targetId)
			{
				// 根據 channel 類型發送
				if (channel == ENetChannelType::Tcp)
				{
					TcpServerClientChannel tcpChannel(*client.tcpClient);
					tcpChannel.send(cp);
				}
				else // UdpChain
				{
					// TODO: 實作 UDP 發送
				}
				return;
			}
		}
	});
	return true;
}

bool ServerTransportImpl::broadcastPacket(ENetChannelType channel, IComPacket* cp)
{
	// 在 Net Thread 中執行廣播
	postToNetThread([this, channel, cp]() {
		for (auto& client : mClients)
		{
			if (channel == ENetChannelType::Tcp)
			{
				TcpServerClientChannel tcpChannel(*client.tcpClient);
				tcpChannel.send(cp);
			}
			else // UdpChain
			{
				// TODO: 實作 UDP 廣播
			}
		}
	});
	return true;
}

void ServerTransportImpl::kickConnection(SessionId id)
{
	// 在 Net Thread 中執行踢出操作
	postToNetThread([this, id]() {
		for (auto it = mClients.begin(); it != mClients.end(); ++it)
		{
			if (it->id == id)
			{
				// 關閉 TCP 連線
				it->tcpClient->close();
				
				// 從列表移除
				mClients.erase(it);
				break;
			}
		}
	});
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

bool ServerTransportImpl::sendUdpPacket(IComPacket* packet, NetAddress const& addr)
{
	if (!packet)
		return false;
	
	// 序列化封包
	SocketBuffer buffer(512);
	try
	{
		FNetCommand::Write(buffer, packet);
	}
	catch (...)
	{
		LogError("ServerTransportImpl::sendUdpPacket - Failed to serialize packet");
		return false;
	}
	
	// 發送 UDP 封包
	int bytesSent = buffer.take(mUdpServer.getSocket(), buffer.getAvailableSize(), const_cast<NetAddress&>(addr));

	if (bytesSent <= 0)
	{
		LogWarning(0, "ServerTransportImpl::sendUdpPacket - Failed to send UDP packet");
		return false;
	}
	
	return true;
}

// Connection Listener

void ServerTransportImpl::ConnectionListener::notifyConnectionSend(NetConnection* con)
{
	// 可以發送資料了
}

bool ServerTransportImpl::ConnectionListener::notifyConnectionRecv(NetConnection* con, SocketBuffer& buffer, NetAddress* clientAddr)
{
	// 判断是 TCP 还是 UDP
	bool isUdp = (clientAddr != nullptr);
	
	if (!isUdp)
	{
		LogMsg("Transport: TCP data received, buffer size=%d", buffer.getAvailableSize());
	}
	
	// 找到对应的 SessionId
	SessionId sessionId = 0;
	
	if (!isUdp)
	{
		// TCP: 必须找到对应的连线
		for (auto const& client : mOwner->mClients)
		{
			if (&client.tcpClient->getSocket() == &con->getSocket())
			{
				sessionId = client.id;
				break;
			}
		}
		
		if (sessionId == 0)
		{
			LogWarning(0, "Received TCP packet from unknown connection");
			return false;  // TCP 必须有 sessionId
		}
		
		LogMsg("Transport: Found SessionId=%d for TCP connection", sessionId);
	}
	else
	{
		// UDP: 嘗試找到對應的連線，但允許 sessionId == 0
		// （用於房間搜尋等無連線的 UDP 請求）
		if (clientAddr)
		{
			// TODO: 可以通過 clientAddr 查找對應的 client
			// 目前先設為 0，表示是無連線的 UDP 封包
			sessionId = 0;
		}
	}
	
	// 使用 ComEvaluator 解析封包
	try
	{
		while (buffer.getAvailableSize() > 0)
		{
			LogMsg("Transport: Parsing packet, available=%d", buffer.getAvailableSize());
			
			// UserData 设置为 sessionId（由 ComEvaluator 使用）
			IComPacket* packet = mOwner->mPacketEvaluator.readNewPacket(buffer, -1, reinterpret_cast<void*>(sessionId));
			if (packet == nullptr)
			{
				LogWarning(0, "readNewPacket returned null");
				break;
			}
			
			LogMsg("Transport: Parsed packet ID=%u", packet->getID());
			// ✅ 使用專用的 UDP callback，直接傳遞 NetAddress
			if (isUdp && clientAddr && mOwner->mCallbacks.onUdpPacketReceived)
			{
				// 捕獲 clientAddr 的副本
				NetAddress addrCopy = *clientAddr;
				mOwner->postToGameThread([owner = mOwner, sessionId, packet, addrCopy]() {
					owner->mCallbacks.onUdpPacketReceived(sessionId, packet, addrCopy);
					// Session 層會呼叫 dispatchPacket() 然後刪除 packet
				});
			}
			else if (mOwner->mCallbacks.onPacketReceived)
			{
				// TCP 或沒有 UDP callback 的情況，使用標準 callback
				mOwner->postToGameThread([owner = mOwner, sessionId, packet]() {
					owner->mCallbacks.onPacketReceived(sessionId, packet);
					// Session 層會呼叫 dispatchPacket() 然後刪除 packet
				});
			}
		}
	}
	catch (ComException& e)
	{
		LogError("Packet parsing error: %s", e.what());
		return false;
	}
	catch (...)
	{
		LogError("Unknown packet parsing error");
		return false;
	}
	
	return true;
}

void ServerTransportImpl::ConnectionListener::notifyConnectionAccpet(NetConnection* con)
{
	// 新连线接受
	TcpServer::Client* client = static_cast<TcpServer::Client*>(con);
	
	SessionId newId = mOwner->mNextSessionId++;
	
	// ✅ 保存客户端连接！
	ClientData clientData;
	clientData.id = newId;
	clientData.tcpClient = client;
	mOwner->mClients.push_back(clientData);
	
	// ✅ 设置监听器！这样才能收到 TCP 数据
	client->setListener(this);
	
	LogMsg("Client Connection");
	LogMsg("Client connected: SessionId=%d", newId);
	
	// 通知会话层
	if (mOwner->mCallbacks.onConnectionEstablished)
	{
		mOwner->postToGameThread([owner = mOwner, newId]() {
			owner->mCallbacks.onConnectionEstablished(newId);
		});
	}
}

void ServerTransportImpl::ConnectionListener::notifyConnectionClose(NetConnection* con, NetCloseReason reason)
{
	// 連線關閉 - 找到對應的 SessionId
	SessionId id = 0;
	for (auto const& client : mOwner->mClients)
	{
		if (&client.tcpClient->getSocket() == &con->getSocket())
		{
			id = client.id;
			break;
		}
	}
	
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
		mOwner->mTcpClient.updateSocket(time, mOwner->mNetSelect);
		mOwner->mUdpClient.updateSocket(time, mOwner->mNetSelect);
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
	postToNetThread([this, channel, cp]() {
		if (channel == ENetChannelType::Tcp)
		{
			TcpNetChannel tcpChannel(mTcpClient);
			tcpChannel.send(cp);
		}
		else // UdpChain
		{
			UdpChainChannel udpChannel(mUdpClient);
			udpChannel.send(cp);
		}
	});
	return true;
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
	// 收到資料 - 解析封包並通知回調
	if (!mOwner->mCallbacks.onPacketReceived)
		return true;
	
	// Client 的 SessionId 固定為 1 (Server)
	SessionId sessionId = 1;
	
	// 使用 ComEvaluator 解析封包
	try
	{
		while (buffer.getAvailableSize() > 0)
		{
			IComPacket* packet = mOwner->mPacketEvaluator.readNewPacket(buffer, -1, reinterpret_cast<void*>(sessionId));
			if (packet == nullptr)
			{
				break;
			}
			
			// 通過回調傳遞給會話層（在 Game Thread 執行）
			// ⚠️ Session 層負責分派到處理器，並在處理完後釋放 packet
			mOwner->postToGameThread([owner = mOwner, sessionId, packet]() {
				owner->mCallbacks.onPacketReceived(sessionId, packet);
				// Session 層會呼叫 dispatchPacket() 然後刪除 packet
			});
		}
	}
	catch (ComException& e)
	{
		LogError("Packet parsing error: %s", e.what());
		return false;
	}
	catch (...)
	{
		LogError("Unknown packet parsing error");
		return false;
	}
	
	return true;
}
