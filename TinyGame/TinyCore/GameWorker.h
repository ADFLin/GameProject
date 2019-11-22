#pragma once
#ifndef GameWorker_H_414D91B7_E6B1_4BF0_9900_CF50D7AA77F2
#define GameWorker_H_414D91B7_E6B1_4BF0_9900_CF50D7AA77F2

#include "GameShare.h"
#include "ComPacket.h"
#include "PlatformThread.h"

#include "GameNetConnect.h"
#include "SocketBuffer.h"

#include <vector>

class NetSocket;
class IFrameUpdater;
class NetAddress;
struct ActionParam;
class ComEvaluator;
class IActionInput;
class IComPacket;
class IPlayerManager;
class GameController;


enum NetActionState
{
	NAS_DISSCONNECT ,
	NAS_LOGIN       ,
	NAS_ACCPET      , //Server
	NAS_CONNECT     ,
	NAS_RECONNECT   ,


	//TODO# split
	NAS_TIME_SYNC   ,

	NAS_ROOM_ENTER  ,
	NAS_ROOM_READY  ,
	NAS_ROOM_WAIT   ,

	NAS_LEVEL_SETUP   ,
	NAS_LEVEL_LOAD    ,

	NAS_LEVEL_INIT    ,
	NAS_LEVEL_RESTART ,
	NAS_LEVEL_RUN     ,
	NAS_LEVEL_PAUSE   ,
	NAS_LEVEL_EXIT    ,

	NAS_LEVEL_LOAD_FAIL,
};

enum class NetControlAction
{
	LevelSetup ,
	LevelLoad ,
	LevelInit ,
	LevelRun ,
	LevelPause ,
	LevelRestart ,
	LevelExit ,
	LevelChange ,
};

enum LoginResult
{
	LOGIN_SUCCESS    =  1 ,
	LOGIN_IN_PW = -1 ,
	LOGIN_BAN        = -2 ,
	LOGIN_GAME_VERSION_ERROR = -3 ,
};


class ClientListener
{
public:
	enum EventID
	{
		eCON_RESULT    ,
		eCON_CLOSE     ,
		eLOGIN_RESULT  ,
		ePLAYER_ADD    ,
		ePLAYER_REMOVE ,
	};
	virtual void onServerEvent( EventID event , unsigned msg ){}
};

class ComListener : public ComVisitor
{
public:
	virtual bool prevProcCommand() { return true; }
	virtual void postProcCommand(){}
};

enum WorkerSendFlag
{
	WSF_IGNORE_LOCAL = BIT(0),

};

enum DefaultChannel
{
	CHANNEL_GAME_NET_TCP = 1,
	CHANNEL_GAME_NET_UDP_CHAIN   = 2,
	NEXT_CHANNEL ,
};


class  NetStateControl
{



};
class  ComWorker
{
public:
	TINY_API ComWorker();
	TINY_API virtual ~ComWorker(){}

	TINY_API void   update( long time );
	TINY_API void   changeState( NetActionState state );

	NetActionState  getActionState(){ return mNAState; }
	void            setComListener( ComListener* listener ){  mComListener = listener; }
	ComEvaluator&   getEvaluator(){ return mCPEvaluator; }

	virtual IPlayerManager*    getPlayerManager() = 0;
	virtual void  sendCommand( int channel , IComPacket* cp , unsigned flag = 0 ){}
	virtual long  getNetLatency(){ return 0; }

	void  sendTcpCommand( IComPacket* cp ){ sendCommand( CHANNEL_GAME_NET_TCP , cp , 0 ); }
	void  sendUdpCommand( IComPacket* cp ){ sendCommand( CHANNEL_GAME_NET_UDP_CHAIN , cp , 0 ); }

protected:
	virtual void  doUpdate( long time ){}
	virtual void  postChangeState( NetActionState oldState ){}
private:
	ComEvaluator     mCPEvaluator;
	NetActionState   mNAState;
	ComListener*     mComListener;
};

enum PlayerStateMsg
{
	PSM_ADD ,
	PSM_CHANGE_TO_LOCAL ,
	PSM_REMOVE ,
};

enum LevelStateMsg
{
	LSM_START ,
	LSM_END   ,
};

class INetStateListener
{
public:
	virtual void onPlayerStateMsg( unsigned pID , PlayerStateMsg msg ){}
	virtual void onChangeActionState( NetActionState state ){}
};

class Channel
{
	enum Type
	{
		eUDP ,
		eUDP_CHAIN ,
		eTCP ,
	};
	Type getType();
};

typedef std::function< void() > NetCommandDelegate;


#define NETWORKER_PROCESS_COMMAND_DEFERRED (TINY_USE_NET_THREAD  || 1 )

class  NetWorker : public ComWorker
{
public:
	TINY_API NetWorker();
	TINY_API ~NetWorker();

	TINY_API bool  startNetwork();
	TINY_API void  closeNetwork();
	TINY_API bool  addUdpCom( IComPacket* cp , NetAddress const& addr );

	void  setNetListener( INetStateListener* listener ){ mNetListener = listener;  }
	virtual bool  isServer() = 0;

	long  getNetRunningTime() const { return mNetRunningTime;  }

protected:

	virtual bool  doStartNetwork() = 0;
	virtual void  doCloseNetwork() = 0;
	virtual void  clenupNetResource(){}
	virtual bool  update_NetThread(long time)
	{
#if NETWORKER_PROCESS_COMMAND_DEFERRED
		processNetThreadCommnads();
#endif
		return true;
	}
	virtual void  doUpdate(long time) 
	{

#if !TINY_USE_NET_THREAD
		update_NetThread(time);
#endif

#if NETWORKER_PROCESS_COMMAND_DEFERRED
		processGameThreadCommnads();
#endif
	}
protected:
	typedef fastdelegate::FastDelegate< void ( )> SocketFun;
	typedef TFunctionThread< SocketFun > SocketThread;
	TINY_API void sendUdpCom( NetSocket& socket );

	typedef std::vector< INetStateListener* > NetMsgListenerVec;
	INetStateListener* mNetListener;

	template< class Fun >
	void addGameThreadCommnad(Fun&& fun)
	{
#if NETWORKER_PROCESS_COMMAND_DEFERRED
		NET_MUTEX_LOCK(mMutexGameThreadCommands);
		mGameThreadCommands.push_back(std::forward<Fun>(fun));
#else
		fun();
#endif
	}
	template< class Fun >
	void addNetThreadCommnad(Fun&& fun)
	{
#if NETWORKER_PROCESS_COMMAND_DEFERRED
		NET_MUTEX_LOCK(mMutexNetThreadCommands);
		mNetThreadCommands.push_back(std::forward<Fun>(fun));
#else
		fun();
#endif
	}

#if NETWORKER_PROCESS_COMMAND_DEFERRED
	void processGameThreadCommnads()
	{
		processThreadCommandInternal(mGameThreadCommands
#if TINY_USE_NET_THREAD
			, mMutexGameThreadCommands
#endif
		);
	}

	void processNetThreadCommnads()
	{
		assert(IsInNetThread());
		processThreadCommandInternal(mNetThreadCommands
#if TINY_USE_NET_THREAD
			, mMutexNetThreadCommands
#endif
		);
	}
#endif

private:
	struct UdpCom
	{
		NetAddress  addr;
		size_t      dataSize;
	};
	typedef std::vector< UdpCom > UdpComList;
	NET_MUTEX( mMutexUdpComList )
	UdpComList    mUdpComList;
	SocketBuffer  mUdpSendBuffer;
#if TINY_USE_NET_THREAD
	SocketThread  mSocketThread;
#endif
	long          mNetRunningTime;



#if NETWORKER_PROCESS_COMMAND_DEFERRED
	NET_MUTEX(mMutexNetThreadCommands)
	std::vector< NetCommandDelegate > mNetThreadCommands;

	NET_MUTEX(mMutexGameThreadCommands)
	std::vector< NetCommandDelegate > mGameThreadCommands;


	void processThreadCommandInternal(std::vector< NetCommandDelegate >& commands
#if TINY_USE_NET_THREAD
								,Mutex& MutexCommands
#endif
	)
	{
		NET_MUTEX_LOCK(MutexCommands);
		for( auto const& command : commands )
		{
			command();
		}
		commands.clear();
	}
#endif

#if TINY_USE_NET_THREAD
	volatile int32 mbRequestExitNetThread;
	void  entryNetThread();
#endif
};

class FNetCommand
{
public:
	static bool Eval(UdpChain& chain, ComEvaluator& evaluator, SocketBuffer& buffer, int group = -1, void* userData = nullptr);
	static unsigned Write(NetBufferOperator& bufferCtrl, IComPacket* cp);
	static unsigned Write(SocketBuffer& buffer, IComPacket* cp);
};


#endif // GameWorker_H_414D91B7_E6B1_4BF0_9900_CF50D7AA77F2
