#ifndef GameClient_h__
#define GameClient_h__

#include "GameWorker.h"
#include "GamePlayer.h"

#include "Holder.h"


class TsClientFrameManager;
struct UserProfile;

class CLocalPlayer : public GamePlayer
{
public:
	CLocalPlayer(): GamePlayer(){}

};

class CLPlayerManager : public SimplePlayerManagerT< CLocalPlayer >
{
public:
	void   updatePlayer( PlayerInfo* info[] , int num );
};

class  ClientWorker : public NetWorker, public INetConnectListener
{
	typedef NetWorker BaseClass;
public:
	TINY_API ClientWorker();
	TINY_API ~ClientWorker();

	TINY_API void connect(char const* hostName, char const* loginName);
	TINY_API void sreachLanServer();

	//NetWorker
	bool  isServer(){   return false;  }
	void  sendCommand( int channel , IComPacket* cp , unsigned flag );
	long  getNetLatency(){  return mNetLatency;  }

	void setClientListener( ClientListener* listener ){  mClientListener = listener;  }

	CLPlayerManager* getPlayerManager(){ return mPlayerManager; }

	bool  haveConnect(){ return mSessoionId != 0; }

protected:
	
	void  notifyConnectionOpen( NetConnection* con );
	void  notifyConnectionClose( NetConnection* con , NetCloseReason reason );
	void  notifyConnectionFail( NetConnection* con );
	void  notifyConnectionSend(NetConnection* con);
	bool  notifyConnectionRecv( NetConnection* con , SocketBuffer& buffer , NetAddress* clientAddr );

	//NetWorker
	bool  doStartNetwork();
	void  doCloseNetwork();
	virtual void  doUpdate( long time ) override;
	virtual bool  update_NetThread( long time ) override;

	void  postChangeState( NetActionState oldState );

	////////////////////////////////
	void  procConSetting  ( IComPacket* cp);
	void  procClockSynd   ( IComPacket* cp);
	void  procPlayerStatus( IComPacket* cp);
	void  procPlayerState ( IComPacket* cp);
	void  procClockSynd_NetThread( IComPacket* cp);
	

	/////////////////////////////////

	TPtrHolder< CLPlayerManager > mPlayerManager;

	FixString< MAX_PLAYER_NAME_LENGTH > mLoginName;
	SessionId           mSessoionId;
	TcpClient           mTcpClient;
	UdpClient           mUdpClient;
	NetSelectSet        mNetSelect;
	NetActionState      mNextState;
	ClientListener*     mClientListener;
	LatencyCalculator   mCalculator;
	int                 mNumSampleTest;
	long                mNetLatency;

};

class DelayCtrlBase
{
public:
	DelayCtrlBase();

	void setDelay( long delay , long delayRand = 0 ) 
	{ 
		mDelay = delay;
		mDelayRand = delayRand;
	}
	long makeDelayValue();
protected:
	long         mDelay;
	long         mDelayRand;
	long         mCurTime;
};
class SendDelayCtrl : public DelayCtrlBase
{
public:
	SendDelayCtrl( NetBufferOperator& bufferCtrl );

	void update( long time );
	bool add( IComPacket* cp );
private:
	struct SendInfo
	{
		long     time;
		unsigned size;
	};
	typedef std::list< SendInfo > SendInfoList;

	DEFINE_MUTEX( mMutexBuffer )
	SendInfoList mInfoList;
	SocketBuffer mBuffer;
	NetBufferOperator& mBufferCtrl;
};

class RecvDelayCtrl : public DelayCtrlBase
{
public:
	RecvDelayCtrl( int size );

	void update( long time , UdpClient& client , ComEvaluator& evaluator );
	bool add( SocketBuffer& buffer , bool isUdpPacket );
private:
	struct RecvInfo
	{
		bool        isUdpPacket;
		long        time;
		unsigned    size;
	};
	typedef std::list< RecvInfo > RecvInfoList;
	RecvInfoList mInfoList;
	SocketBuffer mBuffer;
};

class DelayClientWorker : public ClientWorker
{
	typedef ClientWorker BaseClass;
public:
	TINY_API DelayClientWorker();
	TINY_API ~DelayClientWorker();

	TINY_API void setDelay(long delay, long delayRand = 0);
	bool update_NetThread(long time);
	virtual bool  notifyConnectionRecv( NetConnection* connection , SocketBuffer& buffer , NetAddress* clientAddr );
	virtual void  sendCommand( int channel , IComPacket* cp , unsigned flag );
protected:
	SendDelayCtrl mSDCTcp;
	SendDelayCtrl mSDCUdp;
	RecvDelayCtrl mRDCTcp;
	RecvDelayCtrl mRDCUdp;
	RecvDelayCtrl mRDCUdpCL;
};


#endif // GameClient_h__
