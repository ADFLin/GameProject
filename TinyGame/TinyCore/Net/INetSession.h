#pragma once
#ifndef INetSession_H_INCLUDED
#define INetSession_H_INCLUDED

#include "GameShare.h"
#include "GamePlayer.h"
#include "ComPacket.h"
#include <functional>

class INetTransport;
class IPlayerManager;
class DataStreamBuffer;
struct GameLevelInfo;

/**
 * @brief 會話層介面 - 管理遊戲會話（Room/Level/玩家）
 * 
 * 此介面處理：
 * - 連線狀態機
 * - 玩家管理
 * - Room/Level 流程
 * - 事件分發
 * 
 * 不處理：
 * - 底層 Socket
 * - 具體遊戲規則
 */

// 會話狀態
enum class ENetSessionState : uint8
{
	Disconnected,   // 未連線
	Connecting,     // 連線中
	Connected,      // 已連線（尚未加入房間）
	InLobby,        // 在大廳/房間列表
	InRoom,         // 在房間中等待
	RoomReady,      // 在房間中已準備
	LevelLoading,   // 關卡載入中
	LevelRunning,   // 關卡進行中
	LevelPaused,    // 關卡暫停
};

// 玩家會話資訊
struct NetPlayerInfo
{
	PlayerId    id;
	char        name[MAX_PLAYER_NAME_LENGTH];
	int         slot;
	bool        isLocal;
	bool        isReady;
	int         playerType;  // 對應 EPlayerType
};

// 會話事件類型
enum class ENetSessionEvent : uint8
{
	// 連線相關
	ConnectionEstablished,
	ConnectionLost,
	LoginSuccess,
	LoginFailed,
	
	// 玩家相關
	PlayerJoined,
	PlayerLeft,
	PlayerReadyChanged,
	
	// Room 相關
	RoomCreated,
	RoomClosed,
	RoomSettingChanged,
	
	// Level 相關
	LevelStarting,
	LevelLoaded,
	LevelStarted,
	LevelPaused,
	LevelResumed,
	LevelEnded,
};

/**
 * @brief 會話層基礎介面
 */
class INetSession
{
public:
	virtual ~INetSession() = default;
	
	//========================================
	// 生命週期
	//========================================
	virtual bool initialize(INetTransport* transport) = 0;
	virtual void shutdown() = 0;
	virtual void update(long time) = 0;
	
	//========================================
	// 狀態管理
	//========================================
	virtual ENetSessionState getState() const = 0;
	virtual bool changeState(ENetSessionState newState) = 0;
	
	//========================================
	// 玩家管理
	//========================================
	virtual IPlayerManager* getPlayerManager() = 0;
	virtual PlayerId getLocalPlayerId() const = 0;
	
	// 取得所有玩家資訊
	virtual void getPlayerInfos(TArray<NetPlayerInfo>& outInfos) const = 0;
	virtual NetPlayerInfo const* getPlayerInfo(PlayerId id) const = 0;
	
	//========================================
	// 封包處理
	//========================================
	
	// 註冊封包處理器 (會話層處理的封包)
	using PacketHandler = std::function<void(PlayerId senderId, IComPacket* packet)>;
	virtual void registerPacketHandler(ComID packetId, PacketHandler handler) = 0;
	virtual void unregisterPacketHandler(ComID packetId) = 0;
	
	//========================================
	// 封包發送 (透過 Transport)
	//========================================
	virtual void sendReliable(IComPacket* cp) = 0;
	virtual void sendUnreliable(IComPacket* cp) = 0;
	virtual void sendReliableTo(PlayerId targetId, IComPacket* cp) = 0;
	
	//========================================
	// 事件回調
	//========================================
	using SessionEventHandler = std::function<void(ENetSessionEvent event, PlayerId playerId)>;
	void setEventHandler(SessionEventHandler handler) { mEventHandler = handler; }
	
protected:
	void fireEvent(ENetSessionEvent event, PlayerId playerId = ERROR_PLAYER_ID)
	{
		if (mEventHandler)
			mEventHandler(event, playerId);
	}
	
	SessionEventHandler mEventHandler;
};

/**
 * @brief Server 端會話介面 (Host)
 */
class INetSessionHost : public INetSession
{
public:
	//========================================
	// Room 管理
	//========================================
	
	// 建立 Room
	virtual bool createRoom(char const* gameName, int maxPlayers = MAX_PLAYER_NUM) = 0;
	
	// 關閉 Room
	virtual void closeRoom() = 0;
	
	// Room 是否開放
	virtual bool isRoomOpen() const = 0;
	
	// 設定是否允許中途加入
	virtual void setAllowLateJoin(bool allow) = 0;
	virtual bool isAllowLateJoin() const = 0;
	
	//========================================
	// 玩家管理 (Host 特有)
	//========================================
	
	// 踢出玩家
	virtual bool kickPlayer(PlayerId id) = 0;
	
	// 廣播給所有玩家
	virtual void broadcastReliable(IComPacket* cp) = 0;
	virtual void broadcastUnreliable(IComPacket* cp) = 0;
	
	//========================================
	// Level 控制
	//========================================
	
	// 開始關卡
	virtual bool startLevel(GameLevelInfo const& info) = 0;
	
	// 直接開始關卡（跳過 Room）
	virtual bool startLevelDirect(GameLevelInfo const& info) = 0;
	
	// 結束關卡
	virtual void endLevel() = 0;
	
	// 暫停/恢復
	virtual void pauseLevel() = 0;
	virtual void resumeLevel() = 0;
	
	// 重新開始關卡
	virtual void restartLevel() = 0;
	
	//========================================
	// Late Join 支援
	//========================================
	
	// 當有新玩家請求加入時的回調
	// 返回 false 拒絕加入
	using LateJoinRequestHandler = std::function<bool(PlayerId requesterId, NetPlayerInfo const& info)>;
	void setLateJoinRequestHandler(LateJoinRequestHandler handler) { mLateJoinHandler = handler; }
	
protected:
	LateJoinRequestHandler mLateJoinHandler;
};

/**
 * @brief Client 端會話介面
 */
class INetSessionClient : public INetSession
{
public:
	//========================================
	// 連線
	//========================================
	
	// 連線到伺服器並加入
	virtual void joinServer(char const* hostName, char const* playerName) = 0;
	
	// 離開伺服器
	virtual void leaveServer() = 0;
	
	// 直接加入正在進行的關卡
	virtual void joinLevelDirect(char const* hostName, char const* playerName) = 0;
	
	//========================================
	// Room
	//========================================
	
	// 設定準備狀態
	virtual void setReady(bool ready) = 0;
	virtual bool isReady() const = 0;
	
	//========================================
	// 搜尋伺服器
	//========================================
	
	struct ServerInfo
	{
		char name[64];
		char gameName[64];
		char hostAddress[64];
		int  playerCount;
		int  maxPlayers;
		bool allowLateJoin;
		ENetSessionState state;
	};
	
	// 搜尋區網伺服器
	virtual void searchLanServers() = 0;
	
	// 搜尋結果回調
	using ServerFoundHandler = std::function<void(ServerInfo const& info)>;
	void setServerFoundHandler(ServerFoundHandler handler) { mServerFoundHandler = handler; }
	
protected:
	ServerFoundHandler mServerFoundHandler;
};

#endif // INetSession_H_INCLUDED
