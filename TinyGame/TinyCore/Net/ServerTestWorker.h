#pragma once
#ifndef ServerTestWorker_H_INCLUDED
#define ServerTestWorker_H_INCLUDED

#include "GameWorker.h"
#include "NetTransportImpl.h"
#include "NetSessionImpl.h"
#include <memory>

// 前置聲明
class SVPlayerManager;
class SPPlayerStatus;

class ServerTransportImpl;
class NetSessionHostImpl;
class ServerEventResolver;


/**
 * @brief 伺服器測試 Worker - 驗證新架構
 * 
 * 此類別用於驗證重構後的網路架構是否能完整取代舊的 ServerWorker。
 * 它繼承自 NetWorker（與 ServerWorker 相同），但內部使用新的三層架構：
 * - ServerTransportImpl (傳輸層)
 * - NetSessionHostImpl (會話層)
 * - ServerTestWorker (橋接層)
 * 
 * ⚠️ 重要：為了能直接替代 ServerWorker，此類別必須：
 * 1. 繼承自 NetWorker（而非 ComWorker）
 * 2. 實作所有 ServerWorker 的公開介面
 * 3. 保持與 ServerWorker 相同的使用方式
 */
class ServerTestWorker : public NetWorker
{
	typedef NetWorker BaseClass;
public:
	TINY_API ServerTestWorker();
	TINY_API ~ServerTestWorker();

	//========================================
	// NetWorker/ComWorker 介面實作
	//========================================
	bool sendCommand(int channel, IComPacket* cp, EWorkerSendFlag flag) override;
	long getNetLatency() override { return mTransport ? mTransport->getNetLatency() : 0; }
	bool isServer() override { return true; }
	
	// 協變返回類型 (SVPlayerManager 繼承自 IPlayerManager)
	SVPlayerManager* getPlayerManager() override;

	//========================================
	// 網路生命週期
	//========================================
	TINY_API bool isRunning() const;

	//========================================
	// ServerWorker 兼容介面
	//========================================
	
	// getPlayerManager() 已在上面定義（協變返回類型）
	
	// 建立本地 Worker（模擬 ServerWorker 行為）
	TINY_API LocalWorker* createLocalWorker(char const* userName);
	
	// 踢出玩家
	TINY_API bool kickPlayer(unsigned id);
	
	// 生成玩家狀態 (ServerWorker 兼容方法)
	TINY_API void generatePlayerStatus(SPPlayerStatus& comPS);
	
	// 獲取 UDP Socket (用於 SNetPlayer)
	TINY_API NetSocket& getUdpSocket();
	
	// 房間管理
	TINY_API bool createRoom(char const* gameName, int maxPlayers);
	TINY_API void closeRoom();
	TINY_API bool isRoomOpen() const;
	
	// 關卡管理
	TINY_API bool startLevel(GameLevelInfo const& info);
	TINY_API void endLevel();
	TINY_API void pauseLevel();
	TINY_API void resumeLevel();


	PacketDispatcher& getPacketDispatcher();

	//========================================
	// 事件處理
	//========================================
	
	// 註冊封包處理器（新架構方式）
	void registerPacketHandler(ComID packetId, std::function<void(PlayerId, IComPacket*)> handler);
	void unregisterPacketHandler(ComID packetId);
	
	// 設定事件監聽器（兼容舊架構）
	void setEventListener(std::function<void(ENetSessionEvent, PlayerId)> listener);
	
	// 設定網路狀態監聽器（兼容 NetWorker）
	void setNetListener(INetStateListener* listener) { mNetListener = listener; }
	INetStateListener* getNetListener() const { return mNetListener; }
	
	// 設定伺服器事件解析器 (用於處理連線關閉等事件)
	void setEventResolver(ServerEventResolver* resolver) { mEventResolver = resolver; }
	ServerEventResolver* getEventResolver() const { return mEventResolver; }
	
	//========================================
	// UDP 通訊 (用於房間搜尋等)
	//========================================
	TINY_API bool addUdpCom(IComPacket* cp, NetAddress const& addr);

	//========================================
	// 直接存取新架構（用於測試和驗證）
	//========================================
	ServerTransportImpl* getTransport() { return mTransport.get(); }
	NetSessionHostImpl* getSession() { return mSession.get(); }

	//========================================
	// 狀態查詢
	//========================================
	ENetSessionState getSessionState() const;
	PlayerId getLocalPlayerId() const;

protected:
	//========================================
	// NetWorker 虛擬方法覆蓋
	//========================================
	bool doStartNetwork() override;
	void doCloseNetwork() override;
	void doUpdate(long time) override;
	bool update_NetThread(long time) override;
	void postChangeState(NetActionState oldState) override;
	void clenupNetResource() override;

private:
	//========================================
	// 新架構組件
	//========================================
	std::unique_ptr<ServerTransportImpl> mTransport;
	std::unique_ptr<NetSessionHostImpl> mSession;

	//========================================
	// 向後兼容
	//========================================
	TPtrHolder<LocalWorker> mLocalWorker;

	//========================================
	// 狀態轉換輔助
	//========================================
	
	// 將新的 ENetSessionState 轉換為舊的 NetActionState
	static NetActionState toNetActionState(ENetSessionState state);
	
	// 將舊的 NetActionState 轉換為新的 ENetSessionState
	static ENetSessionState toSessionState(NetActionState state);
	
	// 同步新舊狀態
	void syncStateFromSession();
	void syncStateToSession();

	//========================================
	// Session 事件橋接
	// (Transport → Session 直接連接，不經過 ServerTestWorker)
	//========================================
	void onSessionEvent(ENetSessionEvent event, PlayerId playerId);
	
	// ✅ handleServerInfoRequest 已移至 NetSessionHostImpl
	
	//========================================
	// 事件監聽器
	//========================================
	std::function<void(ENetSessionEvent, PlayerId)> mEventListener;
	INetStateListener* mNetListener = nullptr;
	ServerEventResolver* mEventResolver = nullptr;
};

//========================================
// 內聯實作
//========================================



#endif // ServerTestWorker_H_INCLUDED
