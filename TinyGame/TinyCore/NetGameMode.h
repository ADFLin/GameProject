#ifndef NetGameStage_h__
#define NetGameStage_h__

#include "GameStageMode.h"
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

class NetStateController
{
	GameStageBase* mStage;
};

class NetStageData : public ClientListener
{
public:
	NetStageData();
	~NetStageData();
	void       initWorker( ComWorker* worker , ServerWorker* server = NULL );
	
	bool       haveServer(){ return mServer != NULL;  }
	ComWorker* getWorker(){ return mWorker; }

	ClientWorker* getClientWorker(){ assert( !haveServer() ); return static_cast< ClientWorker* >( mWorker ); }
protected:

	void  registerNetEvent();
	void  unregisterNetEvent( void* processor );
	void  disconnect();
	virtual void setupServerProcFun( ComEvaluator& evaluator ) = 0;
	virtual void setupWorkerProcFun( ComEvaluator& evaluator ) = 0;
	ComWorker*    mWorker;
	ServerWorker* mServer;
	bool          bCloseNetWork;
};

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
	               , public NetStageData
	               , public SettingListener
				   , public ServerPlayerListener
{
	typedef StageBase BaseClass;

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

	virtual void onUpdate( long time );
	virtual void onServerEvent( EventID event , unsigned msg );

	//SettingListener
	void onModify( GWidget* ui );
	//PlayerListener
	void onAddPlayer( PlayerId id );
	void onRemovePlayer( PlayerInfo const& info );

	void setupServerProcFun( ComEvaluator& evaluator );
	void setupWorkerProcFun( ComEvaluator& evaluator );

	void  setupGame( char const* name );
	bool  setupUI(bool bFullSetting);

	static int const SETTING_DATA_ID = 1;
	static int const PLAYER_LIST_DATA_ID = 2; 
	void generateSetting( CSPRawData& com );
protected:
	bool taskDestroyServerListPanelCL( long time );
	void sendGameSetting( unsigned pID = SERVER_PLAYER_ID);

	void procPlayerStateSv( IComPacket* cp);
	void procPlayerState  ( IComPacket* cp);
	void procMsg          ( IComPacket* cp);
	void procPlayerStatus ( IComPacket* cp);
	void procSlotState    ( IComPacket* cp);
	void procRawData      ( IComPacket* cp);
	
	TPtrHolder< NetRoomSettingHelper >  mHelper;


	ServerListPanel*    mConnectPanel = nullptr;
	int64               mLastSendSetting;
	GameSettingPanel*   mSettingPanel = nullptr;
	PlayerListPanel*    mPlayerPanel = nullptr;
	ComMsgPanel*        mMsgPanel = nullptr;
	GButton*            mReadyButton;
	GButton*            mExitButton;
	bool                mNeedSendSetting;


};

class NetLevelStageMode : public LevelStageMode
	                    , public NetStageData
	                    , public IFrameUpdater
						, public ServerEventResolver
{
	typedef LevelStageMode BaseClass;
public:

	enum
	{
		UI_UNPAUSE_GAME = BaseClass::NEXT_UI_ID,
		NEXT_UI_ID,
	};

	NetLevelStageMode();

	bool prevStageInit();
	bool postStageInit();
	void onEnd();
	
	void updateTime(long time);
	bool canRender();

	void restart(bool beInit);

	bool onWidgetEvent(int event, int id, GWidget* ui);
	bool onKey(KeyMsg const& msg);

	void   onRestart(uint64& seed);
	bool   tryChangeState(GameState state);
	IPlayerManager* getPlayerManager();

	//FrameUpdater
	void  updateFrame(int frame);
	void  tick();

	void setupServerProcFun(ComEvaluator& evaluator);
	void setupWorkerProcFun(ComEvaluator& evaluator);

	void procPlayerStateSv(IComPacket* cp);
	void procPlayerState  (IComPacket* cp);
	void procLevelInfo    (IComPacket* cp);
	void procMsg          (IComPacket* cp);
	void procNetControlRequest(IComPacket* cp);

	virtual void onServerEvent(EventID event, unsigned msg);

	bool buildNetEngine();
	bool loadLevel(GameLevelInfo const& info);

	bool             mbLevelInitialized;
	INetEngine*      mNetEngine;
	ComMsgPanel*     mMsgPanel;
	uint64           mNetSeed;
	bool             mbReconnectMode;

	//ServerEventResolver
	virtual PlayerConnetionClosedAction resolveConnectClosed_NetThread(ServerResolveContext& context, NetCloseReason reason) override;
	virtual void resolveReconnect_NetThread(ServerResolveContext& context) override;

	virtual NetLevelStageMode* getNetLevelMode() override { return this; }
};



#endif // NetGameStage_h__