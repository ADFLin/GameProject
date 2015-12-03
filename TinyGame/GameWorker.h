#ifndef NetWorker_h__
#define NetWorker_h__

#include "ComPacket.h"
#include "Thread.h"

#include "GameNetConnect.h"
#include "SocketBuffer.h"

#include <vector>

class TSocket;
class IFrameUpdater;
class NetAddress;
struct ActionParam;
class ComEvaluator;
class ActionInput;
class IComPacket;
class IPlayerManager;
class GameController;


enum NetActionState
{
	NAS_DISSCONNECT ,
	NAS_LOGIN       ,
	NAS_ACCPET      , //Server
	NAS_CONNECT     ,

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
	NAS_LEVEL_STOP    ,
	NAS_LEVEL_EXIT    ,
};

enum LoginResult
{
	LOGIN_SUCCESS    =  1 ,
	LOGIN_INVAILD_PW = -1 ,
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
	virtual void prevProcCommand(){}
	virtual void postProcCommand(){}
};

enum WorkerSendFlag
{
	WSF_IGNORE_LOCAL = BIT(0),

};

enum DefaultChannel
{
	CHANNEL_TCP_CONNECT = 1,
	CHANNEL_UDP_CHAIN   = 2,
	NEXT_CHANNEL ,
};

class  ComWorker
{
public:
	GAME_API ComWorker();
	virtual ~ComWorker(){}

	GAME_API void   update( long time );
	GAME_API void   changeState( NetActionState state );

	NetActionState  getActionState(){ return mNAState; }
	void            setComListener( ComListener* listener ){  mComListener = listener; }
	ComEvaluator&   getEvaluator(){ return mCPEvaluator; }

	virtual IPlayerManager*    getPlayerManager() = 0;
	virtual void  sendCommand( int channel , IComPacket* cp , unsigned flag = 0 ){}
	virtual long  getNetLatency(){ return 0; }

	void  sendTcpCommand( IComPacket* cp ){ sendCommand( CHANNEL_TCP_CONNECT , cp , 0 ); }
	void  sendUdpCommand( IComPacket* cp ){ sendCommand( CHANNEL_UDP_CHAIN   , cp , 0 ); }

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

class NetMessageListener
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
class  NetWorker : public ComWorker
	             , public ConListener
{
public:
	GAME_API NetWorker();
	GAME_API ~NetWorker();

	GAME_API bool  startNetwork();
	GAME_API void  closeNetwork();
	GAME_API bool  addUdpCom( IComPacket* cp , NetAddress const& addr );

	void  setNetListener( NetMessageListener* listener ){ mNetListener = listener;  }
	virtual bool  isServer() = 0;

	long  getNetRunningTime() const { return mNetRunningTime;  }

protected:

	virtual bool  doStartNetwork() = 0;
	virtual void  doCloseNetwork() = 0;
	virtual bool  updateSocket( long time ) = 0;

protected:
	typedef MemberFunThread< NetWorker > SocketThread;
	GAME_API void sendUdpCom( TSocket& socket );

	typedef std::vector< NetMessageListener* > NetMsgListenerVec;
	NetMessageListener* mNetListener;

private:
	struct UdpCom
	{
		NetAddress  addr;
		size_t      dataSize;
	};
	typedef std::vector< UdpCom > UdpComList;
	DEFINE_MUTEX( mMutexUdpComList )
	UdpComList    mUdpComList;
	SBuffer       mUdpSendBuffer;
	SocketThread  mSocketThread;
	long          mNetRunningTime;

	unsigned  procSocketThread();
};



#endif // NetWorker_h__