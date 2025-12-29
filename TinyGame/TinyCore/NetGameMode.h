#pragma once
#ifndef NetGameMode_H_4B9DCBFB_CFBC_479A_A103_2AF6CD8F1D81
#define NetGameMode_H_4B9DCBFB_CFBC_479A_A103_2AF6CD8F1D81

#include "GameMode.h"
#include "GameModule.h"
#include "GameWorker.h"
#include "GameServer.h"
#include "GameClient.h"
#include "INetEngine.h"
#include "ComPacket.h"

#include "Widget/GameRoomUI.h"


class CSPRawData;
class GameSettingPanel;
class ServerListPanel;
class NetRoomSettingHelper;
class DataStreamBuffer;

class NetGameMode;


class GameStartTask : public TaskBase
{
public:
	void release(){ delete this; }
	virtual void onStart();
	virtual bool onUpdate( long time );
	virtual void onEnd( bool beComplete ){}
	
	int  nextMsgSec;
	long SynTime;
	ServerWorker* server;
};

class NetRoomStage : public StageBase
	               , public SettingListener
	               , public ClientListener
{
	typedef StageBase BaseClass;

	NetGameMode* getMode();
	
	// Helper Redirects
	bool       haveServer(); 
	ComWorker* getWorker();
	ServerWorker* getServer();
	ClientWorker* getClientWorker();

	enum
	{
		UI_NET_ROOM_READY = BaseClass::NEXT_UI_ID ,
		UI_NET_ROOM_START ,
		UI_DISCONNECT_MSGBOX ,
	};
public:
	NetRoomStage();
	~NetRoomStage();

	virtual bool onInit();

	virtual void onEnd();
	virtual void onRender( float dFrame );
	virtual bool onWidgetEvent( int event , int id , GWidget* ui );

	virtual void onUpdate(GameTimeSpan deltaTime);
	virtual void onServerEvent( ClientListener::EventID event , unsigned msg );

	//SettingListener
	void onModify( GWidget* ui );


	void setupServerProcFunc(PacketDispatcher& dispatcher);
	void setupWorkerProcFunc(PacketDispatcher& dispatcher);

	bool  setupUI(bool bFullSetting);
protected:
	bool taskDestroyServerListPanelCL( long time );

public:
	void procPlayerStateSv(IComPacket* cp);
	void procPlayerState  (IComPacket* cp);
	void procMsg          (IComPacket* cp);
	void procPlayerStatus (IComPacket* cp);
	void procSlotState    (IComPacket* cp);
	void procRawData      (IComPacket* cp);


	ServerListPanel*    mConnectPanel = nullptr;
	int64               mLastSendSetting;
	GameSettingPanel*   mSettingPanel = nullptr;
	PlayerListPanel*    mPlayerPanel = nullptr;
	ComMsgPanel*        mMsgPanel = nullptr;
	GButton*            mReadyButton;
	GButton*            mExitButton;
	bool                mNeedSendSetting;


};

class NetGameMode : public GameLevelMode
	              , public IFrameUpdater
			      , public ServerEventResolver
				  , public ClientListener
				  , public ServerPlayerListener
{
	typedef GameLevelMode BaseClass;
public:

	enum
	{
		UI_UNPAUSE_GAME = BaseClass::NEXT_UI_ID,
		NEXT_UI_ID,
	};

	NetGameMode();
	~NetGameMode();

	bool initializeStage(GameStageBase* stage) override;
	void onEnd();
	
	void updateTime(GameTimeSpan deltaTime);
	bool canRender();

	bool onWidgetEvent(int event, int id, GWidget* ui);
	MsgReply onKey(KeyMsg const& msg);

	void   onRestart(uint64& seed);
	bool   prevChangeState(EGameState state);
	IPlayerManager* getPlayerManager();

	StageManager*  getManager() { return mStageManager; }

	//FrameUpdater
	void  updateFrame(int frame);
	void  tick();

	void setupServerProcFunc(PacketDispatcher& dispatcher);
	void setupWorkerProcFunc(PacketDispatcher& dispatcher);


	void procPlayerState  (IComPacket* cp);
	void procPlayerStatus (IComPacket* cp);
	void procSlotState    (IComPacket* cp);
	void procRawData      (IComPacket* cp);
	void procLevelInfo    (IComPacket* cp);
	void procMsg          (IComPacket* cp);
	void procNetControlRequest(IComPacket* cp);

	virtual void onServerEvent(ClientListener::EventID event, unsigned msg);

	// ServerPlayerListener interface
	void onAddPlayer(PlayerId id) override;
	void onRemovePlayer(PlayerInfo const& info) override;


	// Game setting management
	void setupGame(char const* gameName);
	void generateGameSetting(CSPRawData& com);
	void sendGameSetting(unsigned pID = SERVER_PLAYER_ID);
	NetRoomSettingHelper* getSettingHelper() { return mHelper.get(); }

	// Helper-UI binding context for game switching
	struct GameSwitchContext
	{
		PlayerListPanel*  playerPanel = nullptr;
		GameSettingPanel* settingPanel = nullptr;
		SettingListener*  listener = nullptr;
		DataStreamBuffer* importBuffer = nullptr;  // For client importing settings
		bool sendToClients = false;                // For server broadcasting settings
	};
	
	// Unified game switching interface - handles helper creation, UI binding, and settings sync
	void switchGame(char const* gameName, GameSwitchContext const& ctx);
	
	// Bind existing helper to UI (for initial setup when helper already exists)
	void bindHelperToUI(GameSwitchContext const& ctx);

	// Game startup
	bool startGame();

	// State transition
	void transitionToLevel();

	static int const SETTING_DATA_ID = 1;
	static int const PLAYER_LIST_DATA_ID = 2;

	bool buildNetEngine();
	bool loadLevel(GameLevelInfo const& info);

	bool             mbLevelInitialized;
	INetEngine*      mNetEngine;
	ComMsgPanel*     mMsgPanel;
	uint64           mNetSeed;
	bool             mbReconnectMode;
	IFrameUpdater*   mUpdater;
	StageManager*    mStageManager;

	//ServerEventResolver
	virtual PlayerConnetionClosedAction resolveConnectClosed_NetThread(ServerResolveContext& context, NetCloseReason reason) override;
	virtual void resolveReconnect_NetThread(ServerResolveContext& context) override;
	void resolveChangeActionState(NetActionState state) override;

	virtual NetGameMode* getNetLevelMode() override { return this; }

	// NetStageData Inlining
	void       initWorker(ComWorker* worker, ServerWorker* server = nullptr);
	
	void  registerNetEvent();
	void  unregisterNetEvent(void* processor);
	void  disconnect();

	bool       haveServer() { return mServer != NULL; }
	ComWorker* getWorker() { return mWorker; }
	ServerWorker* getServer() { return mServer; }

	ClientWorker* getClientWorker() { assert(!haveServer()); return static_cast<ClientWorker*>(mWorker); }
protected:

	ComWorker*    mWorker;
	ServerWorker* mServer;
	bool          bCloseNetWork;

	TPtrHolder<NetRoomSettingHelper> mHelper;
	void scheduleGameStart();
};


#endif // NetGameMode_H_4B9DCBFB_CFBC_479A_A103_2AF6CD8F1D81