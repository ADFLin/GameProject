#pragma once
#ifndef NetGameModeEx_H_INCLUDED
#define NetGameModeEx_H_INCLUDED

/**
 * @file NetGameModeEx.h
 * @brief NetRoomStage / NetLevelStageMode 的重構版本 - 使用新的網路分層架構
 * 
 * 主要變更：
 * 1. 使用 NetSessionAdapter 替代 NetStageData（組合 vs 繼承）
 * 2. 使用 INetSessionHost/INetSessionClient 替代直接存取 ServerWorker/ClientWorker
 * 3. 封包處理使用統一的註冊機制
 * 4. 支援 Direct Level 和 Late Join
 */

#include "StageBase.h"
#include "Net/NetSessionAdapter.h"
#include "Net/INetSession.h"
#include "Widget/GameRoomUI.h"
#include "GameSettingHelper.h"

class CSPRawData;
class GameSettingPanel;
class ServerListPanel;
class NetRoomSettingHelper;

/**
 * @brief 重構後的 NetRoomStage
 * 
 * 變更：
 * - 不再繼承 NetStageData，改用組合
 * - 支援新架構（INetSession）和舊架構（ComWorker）
 */
class NetRoomStageEx : public StageBase
                     , public SettingListener
{
	typedef StageBase BaseClass;
	
public:
	enum
	{
		UI_NET_ROOM_READY = BaseClass::NEXT_UI_ID,
		UI_NET_ROOM_START,
		UI_DISCONNECT_MSGBOX,
	};
	
	NetRoomStageEx();
	~NetRoomStageEx();
	
	//========================================
	// StageBase
	//========================================
	bool onInit() override;
	void onEnd() override;
	void onRender(float dFrame) override;
	bool onWidgetEvent(int event, int id, GWidget* ui) override;
	void onUpdate(GameTimeSpan deltaTime) override;
	
	//========================================
	// 初始化方式
	//========================================
	
	// 方式1：使用舊系統（向後相容）
	void initWithWorker(ComWorker* worker, ServerWorker* server = nullptr);
	
	// 方式2：使用新系統
	void initWithSession(INetSessionHost* hostSession);
	void initWithSession(INetSessionClient* clientSession);
	
	//========================================
	// SettingListener
	//========================================
	void onModify(GWidget* ui) override;
	
protected:
	// 網路事件處理
	void onSessionEvent(ENetSessionEvent event, PlayerId playerId);
	
	// UI
	void setupUI(bool bFullSetting);
	void updatePlayerPanel();
	
	// 封包處理
	void setupPacketHandlers();
	void procPlayerState(IComPacket* cp);
	void procPlayerStateSv(IComPacket* cp);
	void procMsg(IComPacket* cp);
	void procPlayerStatus(IComPacket* cp);
	void procSlotState(IComPacket* cp);
	void procRawData(IComPacket* cp);
	
	// 遊戲設定
	void setupGame(char const* name);
	void generateSetting(CSPRawData& com);
	void sendGameSetting(PlayerId targetId = SERVER_PLAYER_ID);
	
	// 開始遊戲
	void startGame();
	void onGameStarting();
	
private:
	//========================================
	// 網路層（使用適配器）
	//========================================
	NetSessionAdapter mNetAdapter;
	
	// 新架構直接存取（可選）
	INetSessionHost* mHostSession = nullptr;
	INetSessionClient* mClientSession = nullptr;
	
	//========================================
	// UI 元件
	//========================================
	ServerListPanel*  mConnectPanel = nullptr;
	GameSettingPanel* mSettingPanel = nullptr;
	PlayerListPanel*  mPlayerPanel = nullptr;
	ComMsgPanel*      mMsgPanel = nullptr;
	GButton*          mReadyButton = nullptr;
	GButton*          mExitButton = nullptr;
	
	//========================================
	// 狀態
	//========================================
	TPtrHolder<NetRoomSettingHelper> mHelper;
	int64 mLastSendSetting = 0;
	bool  mNeedSendSetting = false;
	
	//========================================
	// 輔助
	//========================================
	bool isHost() const { return mNetAdapter.isHost(); }
	IPlayerManager* getPlayerManager() { return mNetAdapter.getPlayerManager(); }
};

/**
 * @brief 重構後的 NetLevelStageMode
 * 
 * 變更：
 * - 不再繼承 NetStageData，改用組合
 * - 支援 Direct Level（跳過 Room）
 * - 支援 Late Join
 */
class NetLevelStageModeEx : public GameModeBase
                          , public IFrameUpdater
{
	typedef GameLevelMode BaseClass;
	
public:
	enum
	{
		UI_UNPAUSE_GAME = BaseClass::NEXT_UI_ID,
		NEXT_UI_ID,
	};
	
	NetLevelStageModeEx();
	~NetLevelStageModeEx();
	
	//========================================
	// GameModeBase
	//========================================
	bool prevStageInit() override;
	bool postStageInit() override;
	void onEnd() override;
	void updateTime(GameTimeSpan deltaTime) override;
	bool canRender() override;
	void restart(bool beInit) override;
	bool onWidgetEvent(int event, int id, GWidget* ui) override;
	MsgReply onKey(KeyMsg const& msg) override;
	void onRestart(uint64& seed) override;
	bool prevChangeState(EGameState state) override;
	IPlayerManager* getPlayerManager() override;
	
	//========================================
	// IFrameUpdater
	//========================================
	void updateFrame(int frame) override;
	void tick() override;
	
	//========================================
	// 初始化
	//========================================
	
	// 使用舊系統
	void initWithWorker(ComWorker* worker, ServerWorker* server = nullptr);
	
	// 使用新系統
	void initWithSession(INetSession* session);
	
	//========================================
	// Direct Level 模式
	//========================================
	
	// Server：直接開始關卡（不經過 Room）
	bool startDirectLevel(GameLevelInfo const& info);
	
	// Client：直接加入正在進行的關卡
	bool joinDirectLevel(char const* hostAddress);
	
	//========================================
	// Late Join 支援
	//========================================
	void setLateJoinEnabled(bool enabled);
	bool isLateJoinEnabled() const { return mAllowLateJoin; }
	
	// 當新玩家加入時呼叫
	void onPlayerLateJoin(PlayerId playerId);
	
protected:
	// 網路事件
	void onSessionEvent(ENetSessionEvent event, PlayerId playerId);
	
	// 封包處理
	void setupPacketHandlers();
	void procPlayerState(IComPacket* cp);
	void procPlayerStateSv(IComPacket* cp);
	void procLevelInfo(IComPacket* cp);
	void procMsg(IComPacket* cp);
	void procNetControlRequest(IComPacket* cp);
	
	// 關卡載入
	bool buildNetEngine();
	bool loadLevel(GameLevelInfo const& info);
	
private:
	//========================================
	// 網路層
	//========================================
	NetSessionAdapter mNetAdapter;
	INetSessionHost* mHostSession = nullptr;
	INetSessionClient* mClientSession = nullptr;
	
	//========================================
	// 遊戲層（可選）
	//========================================
	class IGameNetSession* mGameSession = nullptr;
	
	//========================================
	// 引擎
	//========================================
	class INetEngine* mNetEngine = nullptr;
	
	//========================================
	// UI
	//========================================
	ComMsgPanel* mMsgPanel = nullptr;
	
	//========================================
	// 狀態
	//========================================
	uint64 mNetSeed = 0;
	bool   mLevelInitialized = false;
	bool   mReconnectMode = false;
	bool   mAllowLateJoin = false;
	bool   mDirectLevelMode = false;
	
	//========================================
	// 輔助
	//========================================
	bool isHost() const { return mNetAdapter.isHost(); }
};

#endif // NetGameModeEx_H_INCLUDED
