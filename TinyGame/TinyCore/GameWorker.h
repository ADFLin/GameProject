#pragma once
#ifndef GameWorker_H_414D91B7_E6B1_4BF0_9900_CF50D7AA77F2
#define GameWorker_H_414D91B7_E6B1_4BF0_9900_CF50D7AA77F2

#include "GameShare.h"
#include "ComPacket.h"
#include "PacketDispatcher.h"
#include "PacketFactory.h"
#include "PlatformThread.h"

#include "GameNetConnect.h"
#include "SocketBuffer.h"

#include <vector>
#include <functional>

class NetSocket;
class IFrameUpdater;
class NetAddress;
struct ActionParam;
class ComEvaluator;
class IActionInput;
class IComPacket;
class IPlayerManager;
class InputControl;


enum NetActionState
{
	NAS_DISSCONNECT ,
	NAS_LOGIN       ,
	NAS_ACCPET      , //Server
	NAS_CONNECT     ,
	NAS_RECONNECT   ,


	//#TODO# split
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
	LOGIN_SUCCESS          =  1 ,
	LOGIN_INVALID_PASSWORD = -1 ,
	LOGIN_BAN              = -2 ,
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

enum EWorkerSendFlag
{
	WSF_NONE = 0,
	WSF_IGNORE_LOCAL = BIT(0),

};

SUPPORT_ENUM_FLAGS_OPERATION(EWorkerSendFlag);


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
	
	// 新接口：直接访问组件
	virtual PacketDispatcher& getPacketDispatcher() = 0;
	
	// 辅助方法：模拟 ComEvaluator 接口（用于兼容现有代码）
	template<class GamePacketT, class T, class TFunc>
	bool setUserFunc(T* processer, TFunc func)
	{
		return getPacketDispatcher().setUserFunc<GamePacketT>(processer, func);
	}
	
	template<class GamePacketT, class T, class TFunc>
	bool setWorkerFunc(T* processer, TFunc func, void* dummy)
	{
		return getPacketDispatcher().setWorkerFunc<GamePacketT>(processer, func, dummy);
	}
	
	template<class GamePacketT, class T, class TFunc>
	bool setWorkerFunc(T* processer, TFunc func, TFunc funcSocket)
	{
		return getPacketDispatcher().setWorkerFunc<GamePacketT>(processer, func, funcSocket);
	}
	
	TINY_API void removeProcesserFunc(void* processer);

	virtual IPlayerManager*    getPlayerManager() = 0;
	virtual bool  sendCommand(int channel, IComPacket* cp, EWorkerSendFlag flag) { return false; }
	virtual long  getNetLatency(){ return 0; }

	bool  sendTcpCommand(IComPacket* cp, EWorkerSendFlag flag = WSF_NONE){ return sendCommand( CHANNEL_GAME_NET_TCP , cp , flag); }
	bool  sendUdpCommand(IComPacket* cp, EWorkerSendFlag flag = WSF_NONE){ return sendCommand( CHANNEL_GAME_NET_UDP_CHAIN , cp , flag); }

protected:
	virtual void  doUpdate( long time ){}
	virtual void  postChangeState( NetActionState oldState ){}

private:
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

class NetChannel
{
	enum Type
	{
		eUDP ,
		eUDP_CHAIN ,
		eTCP ,
	};
	Type getType();


	virtual void closeNet() = 0;
};

typedef std::function< void() > NetCommandDelegate;
BITWISE_RELLOCATABLE_FAIL(NetCommandDelegate);


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

	long  getNetRunningTime() const { return mNetRunningTimeSpan;  }

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
		mNetRunningTimeSpan = SystemPlatform::GetTickCount() - mNetStartTime;
		update_NetThread(time);
#endif

#if NETWORKER_PROCESS_COMMAND_DEFERRED
		processGameThreadCommnads();
#endif
	}
protected:
	typedef fastdelegate::FastDelegate< void ( )> SocketFunc;
	typedef TFunctionThread< SocketFunc > SocketThread;
	TINY_API void sendUdpCmd( NetSocket& socket );

	typedef TArray< INetStateListener* > NetMsgListenerVec;
	INetStateListener* mNetListener;

	template< class TFunc >
	void addGameThreadCommnad(TFunc&& func)
	{
#if NETWORKER_PROCESS_COMMAND_DEFERRED
		NET_MUTEX_LOCK(mMutexGameThreadCommands);
		mGameThreadCommands.push_back(std::forward<TFunc>(func));
#else
		func();
#endif
	}
	template< class TFunc >
	void addNetThreadCommnad(TFunc&& func)
	{
#if NETWORKER_PROCESS_COMMAND_DEFERRED
		NET_MUTEX_LOCK(mMutexNetThreadCommands);
		mNetThreadCommands.push_back(std::forward<TFunc>(func));
#else
		func();
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
	struct UdpCmd
	{
		NetAddress  addr;
		size_t      dataSize;
	};
	typedef TArray< UdpCmd > UdpCmdList;
	NET_MUTEX( mMutexUdpCmdList )
	UdpCmdList    mUdpCmdList;
	SocketBuffer  mUdpSendBuffer;
#if TINY_USE_NET_THREAD
	SocketThread  mSocketThread;
#endif
	int64         mNetRunningTimeSpan;
	int64         mNetStartTime;



#if NETWORKER_PROCESS_COMMAND_DEFERRED
	NET_MUTEX(mMutexNetThreadCommands)
	TArray< NetCommandDelegate > mNetThreadCommands;

	NET_MUTEX(mMutexGameThreadCommands)
	TArray< NetCommandDelegate > mGameThreadCommands;


	void processThreadCommandInternal(TArray< NetCommandDelegate >& commands
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
	// 旧接口 - 使用 ComEvaluator（逐步淘汰）
	static bool Eval(UdpChain& chain, ComEvaluator& evaluator, SocketBuffer& buffer, int group = -1, void* userData = nullptr);
	
	// 新接口 - 使用分离的组件
	static bool Eval(UdpChain& chain, PacketFactory& factory, PacketDispatcher& dispatcher, SocketBuffer& buffer, int group = -1, void* userData = nullptr);
	static bool EvalCommand(PacketFactory& factory, PacketDispatcher& dispatcher, SocketBuffer& buffer, int group = -1, void* userData = nullptr);
	
	static unsigned Write(NetBufferOperator& bufferCtrl, IComPacket* cp);
	static unsigned Write(SocketBuffer& buffer, IComPacket* cp);
};


#endif // GameWorker_H_414D91B7_E6B1_4BF0_9900_CF50D7AA77F2
