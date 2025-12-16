#pragma once
#ifndef INetTransport_H_INCLUDED
#define INetTransport_H_INCLUDED

#include "GameShare.h"
#include "NetChannel.h"
#include <functional>

class IComPacket;
class NetAddress;

/**
 * @brief 傳輸層介面 - 負責底層網路通訊
 * 
 * 此介面僅處理：
 * - Socket 管理 (TCP/UDP)
 * - 連線管理 (Connect/Accept/Close)
 * - 封包發送/接收
 * - 執行緒管理 (NetThread/GameThread)
 * 
 * 不處理：
 * - 遊戲邏輯
 * - Room/Level 概念
 * - 玩家管理
 */

// 連線關閉原因
enum class ENetCloseReason : uint8
{
	Unknown,
	LocalClose,
	RemoteClose,
	Timeout,
	Error,
	Kicked,
};

// 傳輸層事件回調
struct NetTransportCallbacks
{
	// 當連線建立時呼叫 (Client 連上 Server，或 Server 接受 Client)
	std::function<void(SessionId id)> onConnectionEstablished;
	
	// 當連線關閉時呼叫
	std::function<void(SessionId id, ENetCloseReason reason)> onConnectionClosed;
	
	// 當連線失敗時呼叫 (Client 無法連上 Server)
	std::function<void()> onConnectionFailed;
	
	// 當收到封包時呼叫 (在適當的執行緒)
	std::function<void(SessionId id, IComPacket* packet)> onPacketReceived;
};

/**
 * @brief 傳輸層基礎介面
 */
class INetTransport
{
public:
	virtual ~INetTransport() = default;
	
	//========================================
	// 生命週期
	//========================================
	virtual bool startNetwork() = 0;
	virtual void closeNetwork() = 0;
	virtual bool isRunning() const = 0;
	
	//========================================
	// 封包發送
	//========================================
	virtual bool sendPacket(SessionId targetId, ENetChannelType channel, IComPacket* cp) = 0;
	virtual bool broadcastPacket(ENetChannelType channel, IComPacket* cp) = 0;
	
	//========================================
	// 狀態查詢
	//========================================
	virtual long getNetLatency() const = 0;
	virtual long getNetRunningTime() const = 0;
	
	//========================================
	// 回調設定
	//========================================
	void setCallbacks(NetTransportCallbacks const& callbacks) { mCallbacks = callbacks; }
	NetTransportCallbacks& getCallbacks() { return mCallbacks; }
	
	//========================================
	// 執行緒間通訊
	//========================================
	
	// 在 Game Thread 執行命令
	template<typename Func>
	void postToGameThread(Func&& func)
	{
		doPostToGameThread(std::function<void()>(std::forward<Func>(func)));
	}
	
	// 在 Net Thread 執行命令
	template<typename Func>
	void postToNetThread(Func&& func)
	{
		doPostToNetThread(std::function<void()>(std::forward<Func>(func)));
	}
	
protected:
	virtual void doPostToGameThread(std::function<void()> func) = 0;
	virtual void doPostToNetThread(std::function<void()> func) = 0;
	
	NetTransportCallbacks mCallbacks;
};

/**
 * @brief Server 端傳輸介面
 */
class IServerTransport : public INetTransport
{
public:
	//========================================
	// Server 特有功能
	//========================================
	
	// 踢出指定連線
	virtual void kickConnection(SessionId id) = 0;
	
	// 取得所有連線 ID
	virtual void getConnectionIds(TArray<SessionId>& outIds) const = 0;
	
	// 取得連線數量
	virtual int getConnectionCount() const = 0;
	
	// 發送給指定連線
	bool sendToConnection(SessionId id, ENetChannelType channel, IComPacket* cp)
	{
		return sendPacket(id, channel, cp);
	}
};

/**
 * @brief Client 端傳輸介面
 */
class IClientTransport : public INetTransport
{
public:
	//========================================
	// Client 特有功能
	//========================================
	
	// 連線到伺服器
	virtual void connect(char const* hostName, int port = 0) = 0;
	
	// 斷開連線
	virtual void disconnect() = 0;
	
	// 是否已連線
	virtual bool isConnected() const = 0;
	
	// 取得 Server 的 SessionId (Client 角度)
	virtual SessionId getServerSessionId() const = 0;
	
	// 發送給 Server
	bool sendToServer(ENetChannelType channel, IComPacket* cp)
	{
		return sendPacket(getServerSessionId(), channel, cp);
	}
};

#endif // INetTransport_H_INCLUDED
