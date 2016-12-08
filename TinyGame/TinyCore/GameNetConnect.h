#ifndef GameNetConnect_h__
#define GameNetConnect_h__

#include "Thread.h"
#include "NetSocket.h"
#include "SocketBuffer.h"
#include "IntegerType.h"
#include <deque>

class NetConnection;
class NetAddress;
class ComEvaluator;
class ComConnection;
class UdpChain;

typedef unsigned SessionId;

#define TG_UDP_PORT 666
#define TG_TCP_PORT 665

#define US_RECV_BUFSIZE  10240
#define USC_SEND_BUFSIZE 10240
#define UC_RECV_BUFSIZE  10240
#define UC_SEND_BUFSIZE  10240

#define TSC_RECV_BUFSZE  10240
#define TSC_SEND_BUFSZE  10240
#define TC_RECV_BUFSZE   10240
#define TC_SEND_BUFSZE   10240


enum ConCloseReason
{
	CCR_TIMEOUT  ,
	CCR_SHUTDOWN ,
	CCR_KICK     ,
};


enum ConBufferOperation
{
	CBO_DISCARD ,
	CBO_RESERVE ,
};


class ConListener
{
public:
	virtual void onConnectExcept( NetConnection* con ){}
	//TCP Server
	virtual void onConnectAccpet( NetConnection* con ){}
	//TCP Client
	virtual void onConnectFail( NetConnection* con ){}
	virtual void onConnectOpen( NetConnection* con ){}
	virtual void onConnectClose( NetConnection* con , ConCloseReason reason ){}

	//UDP Server/Client
	virtual void onSendData( NetConnection* con ){}
	//UDP & TCP
	virtual bool onRecvData( NetConnection* con , SocketBuffer& buffer , NetAddress* addr  = NULL ){ return true; }
};

class NetBufferCtrl 
{
public:
	NetBufferCtrl( int size ): mBuffer( size ){}

	SocketBuffer& getBuffer(){ return mBuffer; }
	void     clear();
	auto     lockBuffer(){ return MakeLockedObjectHandle( mBuffer , &mMutexBuffer ); }
	void     fillBuffer( SocketBuffer& buffer , unsigned num );

	bool     sendData( NetSocket& socket , NetAddress* addr = NULL );
	bool     recvData( NetSocket& socket , int len , NetAddress* addr = NULL );
private:
	SocketBuffer  mBuffer;
	DEFINE_MUTEX( mMutexBuffer )
};


class NetConnection : public SocketDetector
{
public:
	NetConnection ():mListener( NULL ),mLastRespondTime(0){}

	void          setListener( ConListener* listener ){ mListener = listener; }
	ConListener*  getListener(){ return mListener; }
	NetSocket&    getSocket()  { return mSocket; }

	void          recvData( NetBufferCtrl& bufCtrl , int len , NetAddress* addr );
	void          close();

	void          updateSocket( long time );
	
protected:

	bool checkConnect( long time );
	virtual void  doUpdateSocket( long time ){}

	//SocketDetector
	virtual void onReadable( NetSocket& socket , int len ){ assert(0); }
	virtual void onSendable( NetSocket& socket ){ assert(0); }

	void    resolveExcept();
	NetSocket     mSocket;
	ConListener*  mListener;
	long          mLastRespondTime;
};


class ClientBase
{
public:
	ClientBase( int sendSize ) :mSendCtrl( sendSize ){}
	NetBufferCtrl&  getSendCtrl(){ return mSendCtrl; }
protected:
	NetBufferCtrl   mSendCtrl;
};

class UdpChain
{
public:
	UdpChain();
	bool sendPacket( long time , NetSocket& socket , SocketBuffer& buffer , NetAddress& addr );
	bool readPacket( SocketBuffer& buffer , unsigned& readSize );

private:
	UdpChain( UdpChain const& );
	UdpChain& operator = ( UdpChain const& );

	void refrushReliableData( unsigned outgoing );
	
	struct DataInfo
	{
		uint32 size;
		uint32 sequence;
	};

	typedef std::deque< DataInfo > DataInfoList;
	DataInfoList mInfoList;

	long    mTimeLastUpdate;
	long    mTimeResendRel;

	SocketBuffer mBufferCache;
	SocketBuffer mBufferRel;
	uint32  mIncomingAck;
	uint32  mOutgoingSeq;
	uint32  mOutgoingAck;
	uint32  mOutgoingRel;
};

class ServerBase
{

};


class UdpConnection : public NetConnection 
{
public:
	UdpConnection( int recvSize );

	void doUpdateSocket( long time ){   mSocket.detectUDP( *this );  }
	void onReadable( NetSocket& socket , int len );

protected:
	NetBufferCtrl    mRecvCtrl;
};

class TcpConnection : public NetConnection 
{
public:
	TcpConnection(){}
	bool isConnected(){ return mSocket.getState() == SKS_CONNECT; }
	void doUpdateSocket( long time ){  mSocket.detectTCP( *this );  }
	void onExcept( NetSocket& socket ){ resolveExcept(); }
};


class TcpClient : public TcpConnection
	            , public ClientBase
{
public:
	TcpClient( int recvSize = TC_RECV_BUFSZE , 
		       int sendSize = TC_SEND_BUFSZE )
		:ClientBase( sendSize ) 
		,mRecvCtrl( recvSize ){}

	void connect( char const* addrName , unsigned port );
	void clearBuffer();

	virtual void onConnect(NetSocket& socket);
	virtual void onConnectFailed(NetSocket& socket);
	virtual void onClose(NetSocket& socket , bool beGraceful );

	virtual void onSendable( NetSocket& socket );
	virtual void onReadable( NetSocket& socket , int len );

	NetBufferCtrl& getRecvCtrl() { return mRecvCtrl;  }

private:
	NetBufferCtrl mRecvCtrl;
};

class UdpClient : public UdpConnection
	            , public ClientBase
{
public:
	UdpClient();
	void init();
	void setServerAddr( char const* addrName , unsigned port );
	void doUpdateSocket( long time )
	{
		mNetTime = time;
		UdpConnection::doUpdateSocket( time );
	}
	void onSendable( NetSocket& socket );
	void onReadable( NetSocket& socket , int len );
	NetAddress const& getServerAddress(){ return mServerAddr; }
	bool sendData( NetSocket& socket );
	void clearBuffer();

	operator UdpChain&(){ return mChain; } 

protected:
	long       mNetTime;
	NetAddress mServerAddr;
	UdpChain   mChain;
};

class TcpServer : public TcpConnection
	            , public ServerBase
{
public:
	void run( unsigned port )
	{
		if ( !mSocket.listen( port , 10 ) )
			throw SocketException( "Can't Listen Socket" );
	}

	virtual void onAcceptable(NetSocket& socket);

	class Client : public TcpClient
	{
	public:
		Client():TcpClient( TSC_RECV_BUFSZE , TSC_SEND_BUFSZE ){}
	};
};


class UdpServer : public UdpConnection 
	            , public ServerBase
{
public:
	UdpServer()
		:UdpConnection( US_RECV_BUFSIZE )
		,mPort( 0xff ){}

	void run( unsigned port );
	
	
	class Client
	{
	public:
		Client():mSendCtrl( USC_SEND_BUFSIZE){}
		NetBufferCtrl&  getSendCtrl(){ return mSendCtrl; }

		bool processSendData( long time , NetSocket& socket , NetAddress& addr )
		{
			TLockedObject< SocketBuffer > buffer = mSendCtrl.lockBuffer();
			return mChain.sendPacket( time , socket , *buffer , addr );
		}

		operator UdpChain&(){ return mChain; } 

		void clearBuffer()
		{
			TLockedObject< SocketBuffer > buffer = mSendCtrl.lockBuffer();
			buffer->clear();
		}

	private:
		NetBufferCtrl   mSendCtrl;
		UdpChain        mChain;
	};

	void sendPacket( long time , Client& client , NetAddress& addr )
	{
		client.processSendData( time , getSocket() ,  addr );
	}
protected:
	virtual void onSendable( NetSocket& socket );
	unsigned        mPort;
};


class LatencyCalculator
{
public:
	LatencyCalculator( int maxSampleNum )
	{
		mSample = new uint32[ maxSampleNum ];
		mMaxSampleNum = maxSampleNum;
		mCount = 0;
	}
	~LatencyCalculator()
	{
		delete [] mSample;
	}

	void clear(){ mCount = 0; }
	void markRequest()
	{
		mLastRequst = getSystemTime();
	}
	void markReply()
	{
		if ( mCount >= mMaxSampleNum )
			return;

		mSample[ mCount ] = ( getSystemTime() - mLastRequst )/2;
		++mCount;
	}
	uint32 getSystemTime(){ return ::GetTickCount(); }
	int    getSampleNum(){ return mCount; }

	uint32 calcResult()
	{
		uint32 total = 0;
		for ( int i = 0 ; i < mCount ; ++i )
			total += mSample[i];
		return total / mCount;
	}

	int     mMaxSampleNum;
	uint32  mLastRequst;
	int     mCount;
	uint32* mSample;
};

#endif // GameNetConnect_h__