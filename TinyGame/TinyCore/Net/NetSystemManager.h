#pragma once
#ifndef NetSystemManager_H_INCLUDED
#define NetSystemManager_H_INCLUDED

#include "INetTransport.h"
#include "INetSession.h"
#include "IGameNetSession.h"
#include "GameGlobal.h"
#include <memory>

/**
 * @brief 網路系統管理器 - 管理所有網路層物件的生命週期
 * 
 * 類似於 Global::GameNet()，但支援新的分層架構。
 * 
 * 物件存放層級：
 * - Transport Layer (傳輸層) - 底層網路
 * - Session Layer (會話層) - 玩家/Room/Level 管理
 * - Game Layer (遊戲層) - 遊戲特定邏輯
 * 
 * 使用方式：
 * ```cpp
 * // 取得管理器
 * NetSystemManager& netMgr = Global::NetSystem();
 * 
 * // 建立 Server
 * netMgr.createServerSession();
 * 
 * // 建立 Client
 * netMgr.createClientSession();
 * 
 * // 取得會話
 * INetSessionHost* host = netMgr.getHostSession();
 * INetSessionClient* client = netMgr.getClientSession();
 * ```
 */
class NetSystemManager
{
public:
	NetSystemManager();
	~NetSystemManager();
	
	//========================================
	// 單例存取
	//========================================
	static NetSystemManager& Get();
	
	//========================================
	// Server 模式
	//========================================
	
	// 建立 Server 會話
	bool createServerSession();
	
	// 取得 Host 會話
	INetSessionHost* getHostSession() { return mHostSession.get(); }
	IServerTransport* getServerTransport() { return mServerTransport.get(); }
	
	// 是否為 Server
	bool isServer() const { return mHostSession != nullptr; }
	
	//========================================
	// Client 模式
	//========================================
	
	// 建立 Client 會話
	bool createClientSession();
	
	// 取得 Client 會話
	INetSessionClient* getClientSession() { return mClientSession.get(); }
	IClientTransport* getClientTransport() { return mClientTransport.get(); }
	
	// 是否為 Client
	bool isClient() const { return mClientSession != nullptr; }
	
	//========================================
	// 通用
	//========================================
	
	// 取得當前會話（不論 Server 或 Client）
	INetSession* getSession()
	{
		if (mHostSession) return mHostSession.get();
		if (mClientSession) return mClientSession.get();
		return nullptr;
	}
	
	// 是否有活躍的網路會話
	bool hasActiveSession() const { return mHostSession || mClientSession; }
	
	// 關閉所有網路
	void closeNetwork();
	
	// 每幀更新
	void update(long time);
	
	//========================================
	// 遊戲層
	//========================================
	
	// 設定遊戲網路會話
	void setGameSession(IGameNetSession* gameSession);
	IGameNetSession* getGameSession() { return mGameSession; }
	
	// 建立並設定遊戲會話（從工廠）
	bool createGameSession(IGameNetSessionFactory* factory);
	
	//========================================
	// 向後相容
	//========================================
	
	// 建立舊式 NetWorker（內部會橋接到新系統）
	NetWorker* buildNetworkCompat(bool beServer);
	
	// 取得舊式 NetWorker
	NetWorker* getNetWorkerCompat();
	
	// 是否有 Server（相容舊 API）
	bool haveServer() const { return isServer(); }
	
private:
	// Transport Layer
	std::unique_ptr<IServerTransport> mServerTransport;
	std::unique_ptr<IClientTransport> mClientTransport;
	
	// Session Layer
	std::unique_ptr<INetSessionHost> mHostSession;
	std::unique_ptr<INetSessionClient> mClientSession;
	
	// Game Layer (不擁有，由遊戲模組管理)
	IGameNetSession* mGameSession = nullptr;
	
	// 向後相容
	NetWorker* mCompatNetWorker = nullptr;
};

/**
 * @brief 擴展的 IGameNetInterface - 支援新舊兩套系統
 */
class GameNetInterfaceEx : public IGameNetInterface
{
public:
	GameNetInterfaceEx();
	~GameNetInterfaceEx();
	
	//========================================
	// IGameNetInterface (舊 API)
	//========================================
	bool haveServer() override;
	NetWorker* getNetWorker() override;
	NetWorker* buildNetwork(bool beServer) override;
	void closeNetwork() override;
	
	//========================================
	// 新 API
	//========================================
	NetSystemManager& getNetSystem() { return mNetSystem; }
	
	// 建立新架構的 Server
	bool createServer();
	
	// 建立新架構的 Client  
	bool createClient();
	
	// 取得會話（新架構）
	INetSession* getSession() { return mNetSystem.getSession(); }
	INetSessionHost* getHostSession() { return mNetSystem.getHostSession(); }
	INetSessionClient* getClientSession() { return mNetSystem.getClientSession(); }
	
private:
	NetSystemManager mNetSystem;
	
	// 舊系統（仍保留以支援現有程式碼）
	std::unique_ptr<NetWorker> mLegacyNetWorker;
	bool mUsingLegacySystem = false;
};

//========================================
// Global 擴展
//========================================

namespace GlobalEx
{
	// 取得網路系統管理器
	TINY_API NetSystemManager& NetSystem();
	
	// 取得擴展的網路介面
	TINY_API GameNetInterfaceEx& GameNetEx();
}

#endif // NetSystemManager_H_INCLUDED
