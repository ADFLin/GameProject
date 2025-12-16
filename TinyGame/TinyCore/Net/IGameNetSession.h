#pragma once
#ifndef IGameNetSession_H_INCLUDED
#define IGameNetSession_H_INCLUDED

#include "GameShare.h"
#include "DataStreamBuffer.h"
#include <functional>

class INetSession;
class IComPacket;

/**
 * @brief 遊戲網路會話介面 - 遊戲特定的網路邏輯
 * 
 * 此介面由具體遊戲實作，處理：
 * - 遊戲狀態同步
 * - Late Join 支援
 * - 遊戲特定封包
 * 
 * 不處理：
 * - 底層網路
 * - 通用會話邏輯（由 INetSession 處理）
 */

// Late Join 資訊
struct GameLateJoinInfo
{
	uint64 randomSeed;             // 隨機種子
	uint32 currentFrame;           // 當前遊戲幀
	DataStreamBuffer gameState;    // 遊戲狀態數據
};

// 遊戲同步模式
enum class EGameSyncMode : uint8
{
	LockStep,       // 鎖步同步（所有玩家同步執行）
	ClientPredict,  // 客戶端預測（Server 驗證）
	StateSync,      // 狀態同步（定期同步完整狀態）
	Hybrid,         // 混合模式
};

/**
 * @brief 遊戲網路會話介面
 * 
 * 每個需要網路功能的遊戲模組都應實作此介面
 */
class IGameNetSession
{
public:
	virtual ~IGameNetSession() = default;
	
	//========================================
	// 生命週期
	//========================================
	
	// 初始化（綁定到會話層）
	virtual bool initialize(INetSession* session) = 0;
	
	// 關閉
	virtual void shutdown() = 0;
	
	// 每幀更新
	virtual void update(long time) = 0;
	
	//========================================
	// 基本資訊
	//========================================
	
	// 取得遊戲名稱
	virtual char const* getGameName() const = 0;
	
	// 取得同步模式
	virtual EGameSyncMode getSyncMode() const = 0;
	
	//========================================
	// 遊戲狀態同步
	//========================================
	
	// 序列化當前遊戲狀態
	virtual void serializeGameState(DataStreamBuffer& buffer) = 0;
	
	// 反序列化遊戲狀態
	virtual void deserializeGameState(DataStreamBuffer& buffer) = 0;
	
	// 取得當前遊戲幀
	virtual uint32 getCurrentFrame() const = 0;
	
	// 取得隨機種子
	virtual uint64 getRandomSeed() const = 0;
	
	//========================================
	// Late Join 支援
	//========================================
	
	// 是否支援中途加入
	virtual bool supportsLateJoin() const { return false; }
	
	// 產生 Late Join 資訊（Server 端呼叫）
	virtual GameLateJoinInfo generateLateJoinInfo()
	{
		GameLateJoinInfo info;
		info.randomSeed = getRandomSeed();
		info.currentFrame = getCurrentFrame();
		serializeGameState(info.gameState);
		return info;
	}
	
	// 套用 Late Join 資訊（Client 端呼叫）
	virtual bool applyLateJoinInfo(GameLateJoinInfo const& info)
	{
		DataStreamBuffer buffer;
		buffer.copy(info.gameState);
		deserializeGameState(buffer);
		return true;
	}
	
	// 當新玩家即將加入時呼叫（Server 端）
	// 返回 false 拒絕加入
	virtual bool onPlayerJoinRequest(PlayerId playerId) { return true; }
	
	// 當新玩家加入後呼叫
	virtual void onPlayerJoined(PlayerId playerId, bool isLateJoin) {}
	
	// 當玩家離開時呼叫
	virtual void onPlayerLeft(PlayerId playerId) {}
	
	//========================================
	// 遊戲特定封包
	//========================================
	
	// 註冊遊戲特定的封包處理器
	virtual void registerGamePacketHandlers() = 0;
	
	// 取消註冊
	virtual void unregisterGamePacketHandlers() = 0;
	
	//========================================
	// 輔助方法
	//========================================
	
	INetSession* getSession() const { return mSession; }
	bool isHost() const { return mIsHost; }
	
protected:
	INetSession* mSession = nullptr;
	bool mIsHost = false;
};

/**
 * @brief 遊戲網路會話工廠介面
 * 
 * 在 IGameModule 中實作此介面來建立遊戲特定的網路會話
 */
class IGameNetSessionFactory
{
public:
	virtual ~IGameNetSessionFactory() = default;
	
	// 建立遊戲網路會話
	virtual IGameNetSession* createGameNetSession() = 0;
	
	// 是否支援網路遊戲
	virtual bool supportsNetGame() const = 0;
};

#endif // IGameNetSession_H_INCLUDED
