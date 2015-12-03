#ifndef NetGameStage_h__
#define NetGameStage_h__

#include "GameStage.h"
#include "GamePackage.h"
#include "GameWorker.h"
#include "GameServer.h"
#include "GameClient.h"
#include "INetEngine.h"
#include "ComPacket.h"

#include "GameRoomUI.h"

class CSPRawData;
class GameSettingPanel;
class ServerListPanel;
class NetRoomSettingHelper;

class NetStageData : public ClientListener
{
public:
	NetStageData();
	void       initWorker( ComWorker* worker , ServerWorker* server = NULL );
	bool       haveServer(){ return mServer != NULL;  }
	ComWorker* getWorker(){ return mWorker; }

	ClientWorker* getClientWorker(){ assert( !haveServer() ); return static_cast< ClientWorker* >( mWorker ); }
protected:
	void  disconnect();
	virtual void setupServerProcFun( ComEvaluator& evaluator ) = 0;
	virtual void setupWorkerProcFun( ComEvaluator& evaluator ) = 0;
	ComWorker*    mWorker;
	ServerWorker* mServer;
};

template< class Stage >
class NetStageT : public Stage
	            , public NetStageData
{
	typedef Stage BaseClass;
public:
	template< class T1 >
	NetStageT( T1 t1 ):Stage(t1){}
	NetStageT(){} 

	bool onInit()
	{
		if ( !BaseClass::onInit() )
			return false;

		setupWorkerProcFun( mWorker->getEvaluator() );
		if ( haveServer() )
		{
			setupServerProcFun( mServer->getEvaluator() );
		}
		else
		{
			static_cast< ClientWorker* >( mWorker )->setClientListener( this );
		}
		return true;
	}
	void onEnd()
	{
		mWorker->getEvaluator().removeProcesserFun( this );
		if ( mServer )
		{
			mServer->getEvaluator().removeProcesserFun( this );
		}
		else
		{
			static_cast< ClientWorker* >( mWorker )->setClientListener( NULL );
		}
		BaseClass::onEnd();
	}

	void onUpdate( long time )
	{
		BaseClass::onUpdate( time );
		if ( haveServer() )
		{
			//update local worker
			mWorker->update( time );
		}
	}
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


class NetRoomStage : public NetStageT< StageBase >
	               , public SettingListener
				   , public PlayerListener
{
	typedef NetStageT< StageBase > BaseClass;

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
	virtual bool onEvent( int event , int id , GWidget* ui );

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
	bool  setupUI();

	static int const SETTING_DATA_ID = 1;
	static int const PLAYER_LIST_DATA_ID = 2; 
	void generateSetting( CSPRawData& com );
protected:
	bool taskDestroyServerListPanelCL( long time );
	void sendGameSetting( unsigned pID = ERROR_PLAYER_ID );

	void procPlayerStateSv( IComPacket* cp );
	void procPlayerState  ( IComPacket* cp );
	void procMsg          ( IComPacket* cp );
	void procPlayerStatus ( IComPacket* cp );
	void procSlotState    ( IComPacket* cp );
	void procRawData      ( IComPacket* cp );
	
	TPtrHolder< NetRoomSettingHelper >  mHelper;


	ServerListPanel*    mConnectPanel;
	long                mLastSendSetting;
	GameSettingPanel*   mSettingPanel;
	PlayerListPanel*    mPlayerPanel;
	ComMsgPanel*        mMsgPanel;
	GButton*            mReadyButton;
	GButton*            mExitButton;
	StageBase*          mChangeLevelStage;

};

class  GameNetLevelStage : public NetStageT< GameLevelStage >
	                     , public IFrameUpdater
{
	typedef NetStageT< GameLevelStage > BaseClass;
public:

	enum
	{
		UI_UNPAUSE_GAME = BaseClass::NEXT_UI_ID ,
		NEXT_UI_ID ,
	};

	GameNetLevelStage();

	void onRender( float dFrame );
	bool onInit();



	void onEnd();
	bool onEvent( int event , int id , GWidget* ui );
	void onUpdate( long time );
	bool onKey( unsigned key , bool isDown );

	void   onRestart( uint64& seed );
	bool   tryChangeState(GameState state );
	IPlayerManager* getPlayerManager(); 

	//FrameUpdater
	void  updateFrame( int frame );
	void  tick();

	void setupServerProcFun( ComEvaluator& evaluator );
	void setupWorkerProcFun( ComEvaluator& evaluator );

	void procPlayerStateSv( IComPacket* cp );
	void procPlayerState  ( IComPacket* cp );
	void procMsg( IComPacket* cp );

	virtual void onServerEvent( EventID event , unsigned msg );

	bool buildNetEngine();
	bool loadLevel();

	INetEngine*      mNetEngine;
	ComMsgPanel*     mMsgPanel;
};









#endif // NetGameStage_h__