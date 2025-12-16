#pragma once
#ifndef NetLayerFwd_H_INCLUDED
#define NetLayerFwd_H_INCLUDED

/**
 * @file NetLayerFwd.h
 * @brief 網路分層架構 - 前向宣告
 * 
 * 網路架構分為三層：
 * 
 * 1. Transport Layer (傳輸層) - INetTransport
 *    - Socket 管理
 *    - 連線管理
 *    - 封包收發
 *    - 執行緒管理
 * 
 * 2. Session Layer (會話層) - INetSession
 *    - 連線狀態機
 *    - 玩家管理
 *    - Room/Level 流程
 *    - 事件分發
 * 
 * 3. Game Layer (遊戲層) - IGameNetSession
 *    - 遊戲狀態同步
 *    - Late Join 支援
 *    - 遊戲特定封包
 */

// ========================================
// Transport Layer (傳輸層)
// ========================================
class INetTransport;
class IServerTransport;
class IClientTransport;

struct NetTransportCallbacks;

// ========================================
// Session Layer (會話層)
// ========================================
class INetSession;
class INetSessionHost;
class INetSessionClient;

struct NetPlayerInfo;

enum class ENetSessionState : uint8;
enum class ENetSessionEvent : uint8;
enum class ENetCloseReason : uint8;

// ========================================
// Game Layer (遊戲層)
// ========================================
class IGameNetSession;
class IGameNetSessionFactory;

struct GameLateJoinInfo;

enum class EGameSyncMode : uint8;

// ========================================
// 常用類型別名
// ========================================
// (在需要時可以添加 using 別名)

#endif // NetLayerFwd_H_INCLUDED
