#pragma once
#ifndef NetSessionAdapter_H_INCLUDED
#define NetSessionAdapter_H_INCLUDED

#include "Net/INetSession.h"
#include "GameWorker.h"
#include "GameServer.h"
#include "GameClient.h"

class ComWorker;
class ServerWorker;
class ClientWorker;

/**
 * @brief 會話適配器 - 橋接舊系統與新架構
 * 
 * 讓現有的 NetRoomStage/NetLevelStageMode 能夠
 * 透過此適配器使用新的 INetSession 功能
 */
class NetSessionAdapter
{
public:
	NetSessionAdapter();
	~NetSessionAdapter();

	//========================================
	// 初始化
	//========================================
	
	// 從現有 Worker 初始化（向後相容）
	void initFromWorker(ComWorker* worker, ServerWorker* server = nullptr);
	
	// 從新架構初始化
	void initFromSession(INetSession* session);
	
	void shutdown();
	
	//========================================
	// 狀態查詢
	//========================================
	bool isHost() const { return mServer != nullptr || mIsHost; }
	bool isInitialized() const { return mWorker != nullptr || mSession != nullptr; }
	
	// 取得會話狀態
	ENetSessionState getSessionState() const;
	
	// 設定會話狀態（轉換為舊的 NetActionState）
	void setSessionState(ENetSessionState state);
	
	//========================================
	// 玩家管理
	//========================================
	IPlayerManager* getPlayerManager();
	PlayerId getLocalPlayerId() const;
	
	//========================================
	// 封包發送
	//========================================
	void sendReliable(IComPacket* cp);
	void sendUnreliable(IComPacket* cp);
	void sendReliableTo(PlayerId targetId, IComPacket* cp);
	void broadcastReliable(IComPacket* cp);
	
	//========================================
	// 封包處理註冊
	//========================================
	using PacketHandler = std::function<void(IComPacket*)>;
	void registerPacketHandler(ComID packetId, PacketHandler handler);
	void unregisterPacketHandler(ComID packetId);
	
	//========================================
	// 事件回調
	//========================================
	std::function<void(ENetSessionEvent, PlayerId)> onSessionEvent;
	
	//========================================
	// 直接存取（僅用於遷移期間）
	//========================================
	ComWorker* getWorker() { return mWorker; }
	ServerWorker* getServer() { return mServer; }
	ClientWorker* getClient();
	INetSession* getSession() { return mSession; }
	
protected:
	// 舊系統
	ComWorker* mWorker = nullptr;
	ServerWorker* mServer = nullptr;
	
	// 新系統
	INetSession* mSession = nullptr;
	bool mIsHost = false;
	
	// 狀態轉換
	static NetActionState toNetActionState(ENetSessionState state);
	static ENetSessionState toSessionState(NetActionState state);
};

/**
 * @brief 擴展的 NetStageData - 支援新架構
 * 
 * 繼承自現有的 NetStageData，添加對新架構的支援
 * 可作為 NetRoomStage/NetLevelStageMode 的替代基底類別
 */
class NetStageDataEx : public ClientListener
{
public:
	NetStageDataEx();
	virtual ~NetStageDataEx();
	
	//========================================
	// 初始化 (兩種方式皆可)
	//========================================
	
	// 方式1：使用舊系統
	void initWorker(ComWorker* worker, ServerWorker* server = nullptr);
	
	// 方式2：使用新系統
	void initSession(INetSession* session);
	
	//========================================
	// 統一介面
	//========================================
	bool haveServer() const { return mAdapter.isHost(); }
	IPlayerManager* getPlayerManager() { return mAdapter.getPlayerManager(); }
	
	// 封包發送
	void sendReliable(IComPacket* cp) { mAdapter.sendReliable(cp); }
	void sendUnreliable(IComPacket* cp) { mAdapter.sendUnreliable(cp); }
	
	// 封包處理
	template<typename PacketType>
	void registerHandler(void (NetStageDataEx::*handler)(IComPacket*))
	{
		mAdapter.registerPacketHandler(PacketType::StaticID(),
			[this, handler](IComPacket* cp) { (this->*handler)(cp); });
	}
	
protected:
	void disconnect();
	void unregisterNetEvent();
	
	// 子類實作
	virtual void setupServerProcFunc() = 0;
	virtual void setupWorkerProcFunc() = 0;
	
	// ClientListener（向後相容）
	virtual void onServerEvent(EventID event, unsigned msg) override {}
	
	NetSessionAdapter mAdapter;
	bool mCloseNetworkOnDestroy = false;
	
	// 向後相容存取
	ComWorker* getWorker() { return mAdapter.getWorker(); }
	ServerWorker* getServer() { return mAdapter.getServer(); }
	ClientWorker* getClientWorker();
};

//========================================
// 內聯實作
//========================================

inline ClientWorker* NetSessionAdapter::getClient()
{
	if (mServer)
		return nullptr;
	return static_cast<ClientWorker*>(mWorker);
}

inline NetActionState NetSessionAdapter::toNetActionState(ENetSessionState state)
{
	switch (state)
	{
	case ENetSessionState::Disconnected:   return NAS_DISSCONNECT;
	case ENetSessionState::Connecting:     return NAS_LOGIN;
	case ENetSessionState::Connected:      return NAS_CONNECT;
	case ENetSessionState::InRoom:         return NAS_ROOM_ENTER;
	case ENetSessionState::RoomReady:      return NAS_ROOM_READY;
	case ENetSessionState::LevelLoading:   return NAS_LEVEL_LOAD;
	case ENetSessionState::LevelRunning:   return NAS_LEVEL_RUN;
	case ENetSessionState::LevelPaused:    return NAS_LEVEL_PAUSE;
	default:                               return NAS_DISSCONNECT;
	}
}

inline ENetSessionState NetSessionAdapter::toSessionState(NetActionState state)
{
	switch (state)
	{
	case NAS_DISSCONNECT:   return ENetSessionState::Disconnected;
	case NAS_LOGIN:         return ENetSessionState::Connecting;
	case NAS_CONNECT:       return ENetSessionState::Connected;
	case NAS_ROOM_ENTER:
	case NAS_ROOM_WAIT:     return ENetSessionState::InRoom;
	case NAS_ROOM_READY:    return ENetSessionState::RoomReady;
	case NAS_LEVEL_SETUP:
	case NAS_LEVEL_LOAD:    return ENetSessionState::LevelLoading;
	case NAS_LEVEL_INIT:
	case NAS_LEVEL_RUN:     return ENetSessionState::LevelRunning;
	case NAS_LEVEL_PAUSE:   return ENetSessionState::LevelPaused;
	default:                return ENetSessionState::Disconnected;
	}
}

#endif // NetSessionAdapter_H_INCLUDED
