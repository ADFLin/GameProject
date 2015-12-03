#ifndef GameClient_h__
#define GameClient_h__

#include "GameWorker.h"
#include "GamePlayer.h"

#include "THolder.h"


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

class  ClientWorker : public NetWorker
{
	typedef NetWorker BaseClass;
public:
	GAME_API ClientWorker( UserProfile& profile );
	GAME_API ~ClientWorker();

	GAME_API void  connect( char const* hostName );
	GAME_API void  sreachLanServer();

	//NetWorker
	bool  isServer(){   return false;  }
	void  sendCommand( int channel , IComPacket* cp , unsigned flag );
	long  getNetLatency(){  return mNetLatency;  }

	void setClientListener( ClientListener* listener ){  mClientListener = listener;  }

	CLPlayerManager* getPlayerManager(){ return mPlayerManager; }

	bool  haveConnect(){ return mSessoionId != 0; }

protected:
	void  onSendData( Connection* con );
	void  onConnect( Connection* con );
	void  onClose( Connection* con , ConCloseReason reason );
	void  onConnectFailed( Connection* con );
	bool  onRecvData( Connection* con , SBuffer& buffer , NetAddress* clientAddr );

	//NetWorker
	bool  doStartNetwork();
	void  doCloseNetwork();
	void  doUpdate( long time );

	bool  updateSocket( long time );

	void  postChangeState( NetActionState oldState );

	////////////////////////////////
	void  procConSetting  ( IComPacket* cp );
	void  procClockSynd   ( IComPacket* cp );
	void  procPlayerStatus( IComPacket* cp );
	void  procPlayerState ( IComPacket* cp );
	void  procClockSyndNet( IComPacket* cp );
	

	/////////////////////////////////

	TPtrHolder< CLPlayerManager > mPlayerManager;

	SessionId           mSessoionId;
	TcpClient           mTcpClient;
	UdpClient           mUdpClient;
	UserProfile&        mUserProfile;
	NetActionState      mNextState;
	ClientListener*     mClientListener;
	LatencyCalculator   mCalculator;
	int                 mNumSampleTest;
	long                mNetLatency;

};


class SendDelayCtrl
{
public:
	SendDelayCtrl( NetBufferCtrl& bufferCtrl );

	void update( long time );
	bool add( ComEvaluator& evaluator , IComPacket* cp );
	void setDelay( long delay ){ mDelay = delay;  }

private:
	struct SendInfo
	{
		long     time;
		unsigned size;
	};
	typedef std::list< SendInfo > SendInfoList;

	DEFINE_MUTEX( mMutexBuffer )
	SendInfoList mInfoList;
	long         mDelay;
	long         mCurTime;
	SBuffer      mBuffer;
	NetBufferCtrl& mBufferCtrl;
};

class RecvDelayCtrl
{
public:
	RecvDelayCtrl( int size );

	void update( long time , UdpClient& client , ComEvaluator& evaluator );
	bool add( SBuffer& buffer , bool isUdpPacket );
	void setDelay( long delay ){ mDelay = delay;  }
private:
	struct RecvInfo
	{
		bool        isUdpPacket;
		long        time;
		unsigned    size;
	};
	typedef std::list< RecvInfo > RecvInfoList;
	RecvInfoList mInfoList;
	long         mDelay;
	long         mCurTime;
	SBuffer      mBuffer;
};

class DelayClientWorker : public ClientWorker
{
	typedef ClientWorker BaseClass;
public:
	DelayClientWorker( UserProfile& profile );
	void setDelay( long time );
	bool updateSocket( long time );
	virtual bool  onRecvData( Connection* connection , SBuffer& buffer , NetAddress* clientAddr );
	virtual void  sendCommand( int channel , IComPacket* cp , unsigned flag );
protected:
	SendDelayCtrl mSDCTcp;
	SendDelayCtrl mSDCUdp;
	RecvDelayCtrl mRDCTcp;
	RecvDelayCtrl mRDCUdp;
	RecvDelayCtrl mRDCUdpCL;
};


#endif // GameClient_h__
