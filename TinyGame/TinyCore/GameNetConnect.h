#ifndef GameNetConnect_h__
#define GameNetConnect_h__

#include "PlatformThread.h"
#include "NetSocket.h"
#include "SocketBuffer.h"
#include "Core/IntegerType.h"
#include "SystemPlatform.h"

#include "Core/LockFreeList.h"
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

enum class NetCloseReason
{
	Timeout  ,
	ShutDown ,
	Kick     ,
};

enum NetBufferOperation
{
	CBO_DISCARD ,
	CBO_RESERVE ,
};


class INetConnectListener
{
public:
	virtual void notifyConnectionExcept( NetConnection* con ){}
	//TCP Server
	virtual void notifyConnectionAccpet( NetConnection* con ){}
	//TCP Client
	virtual void notifyConnectionFail( NetConnection* con ){}
	virtual void notifyConnectionOpen( NetConnection* con ){}
	virtual void notifyConnectionClose( NetConnection* con , NetCloseReason reason ){}

	//UDP Server/Client
	virtual void notifyConnectionSend( NetConnection* con ){}
	//UDP & TCP
	virtual bool notifyConnectionRecv( NetConnection* con , SocketBuffer& buffer , NetAddress* addr  = NULL ){ return true; }
};

class NetBufferOperator
{
public:
	NetBufferOperator(int bufferSize)
		: mBuffer(bufferSize){}

	SocketBuffer& getBuffer(){ return mBuffer; }
	void     clear();
	auto     lockBuffer()
	{ 
#if TINY_USE_NET_THREAD
		return MakeLockedObjectHandle( mBuffer , &mMutexBuffer ); 
#else
		return MakeLockedObjectHandle(mBuffer, nullptr);
#endif
	}
	void     fillBuffer( SocketBuffer& buffer , unsigned num );

	bool     sendData( NetSocket& socket , NetAddress* addr = NULL );
	bool     recvData( NetSocket& socket , int len , NetAddress* addr = NULL );
private:

	SocketBuffer   mBuffer;
	NET_MUTEX( mMutexBuffer )
};


class NetConnection : public SocketDetector
{
public:
	NetConnection ()
		:mListener( NULL )
		,mLastRespondTime(0)
	{}

	void          setListener( INetConnectListener* listener ){ mListener = listener; }
	INetConnectListener*  getListener(){ return mListener; }
	NetSocket&    getSocket()  { return mSocket; }

	void          recvData( NetBufferOperator& bufCtrl , int len , NetAddress* addr );
	void          close();

	void          updateSocket(long time);
	void          updateSocket(long time, NetSelectSet& netSelect);
protected:

	bool checkConnectStatus( long time );
	virtual bool  doUpdateSocket(long time) = 0;
	virtual bool  doUpdateSocket(long time, NetSelectSet& netSelect) = 0;
	//SocketDetector
	virtual void onReadable( NetSocket& socket , int len ){ assert(0); }
	virtual void onSendable( NetSocket& socket ){ assert(0); }

	void    resolveExcept();
	NetSocket     mSocket;
	INetConnectListener*  mListener;
	long          mLastRespondTime;
};


class ClientBase
{
public:
	ClientBase( int sendSize ) :mSendCtrl( sendSize ){}
	NetBufferOperator&  getSendCtrl(){ return mSendCtrl; }
protected:
	NetBufferOperator   mSendCtrl;
};

class UdpChain
{
public:
	UdpChain();
	bool sendPacket( long time , NetSocket& socket , SocketBuffer& buffer , NetAddress* csaddr );
	bool readPacket( SocketBuffer& buffer , uint32& readSize );


	UdpChain( UdpChain const& ) = delete;
	UdpChain& operator = ( UdpChain const& ) = delete;

private:
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

	bool    mbNeedSendAck;
	SocketBuffer mBufferCache;
	SocketBuffer mBufferRel;
	uint32  mIncomingAck;
	uint32  mOutgoingSeq;
	uint32  mOutgoingAck;
};

class ServerBase
{

};


class UdpConnection : public NetConnection 
{
public:
	UdpConnection( int recvSize );

	bool doUpdateSocket(long time){ return mSocket.detectUDP(*this);  }
	bool doUpdateSocket(long time, NetSelectSet& netSelect) { return mSocket.detectUDP(*this, netSelect); }
	void onReadable( NetSocket& socket , int len );

protected:
	NetBufferOperator    mRecvCtrl;
};

class TcpConnection : public NetConnection 
{
public:
	TcpConnection(){}
	bool isConnected(){ return mSocket.getState() == SKS_CONNECTED; }
	bool doUpdateSocket(long time) { return mSocket.detectTCP(*this); }
	bool doUpdateSocket(long time, NetSelectSet& netSelect) { return mSocket.detectTCP(*this, netSelect); }
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

	NetBufferOperator& getRecvCtrl() { return mRecvCtrl;  }

private:
	NetBufferOperator mRecvCtrl;
};

class UdpClient : public UdpConnection
	            , public ClientBase
{
public:
	UdpClient();
	void initialize();
	void setServerAddr( char const* addrName , unsigned port );
	bool doUpdateSocket( long time)
	{
		mNetTime = time;
		return UdpConnection::doUpdateSocket( time );
	}
	bool doUpdateSocket(long time, NetSelectSet& netSelect)
	{
		mNetTime = time;
		return UdpConnection::doUpdateSocket(time, netSelect);
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

		NetSocket& getSocket() { return mSocket; }
		NetBufferOperator&  getSendCtrl(){ return mSendCtrl; }

		bool processSendData( long time )
		{
			TLockedObject< SocketBuffer > buffer = mSendCtrl.lockBuffer();
			return mChain.sendPacket( time , mSocket, *buffer , nullptr );
		}

		bool processSendData(long time, NetSocket& socket, NetAddress& addr)
		{
			TLockedObject< SocketBuffer > buffer = mSendCtrl.lockBuffer();
			return mChain.sendPacket(time, socket, *buffer, &addr);
		}
		operator UdpChain&(){ return mChain; } 

		void clearBuffer()
		{
			TLockedObject< SocketBuffer > buffer = mSendCtrl.lockBuffer();
			buffer->clear();
		}

	private:
		NetSocket           mSocket;
		NetBufferOperator   mSendCtrl;
		UdpChain            mChain;
	};

	void sendPacket( long time , Client& client )
	{
		client.processSendData( time );
	}
	void sendPacket(long time, Client& client, NetAddress& addr)
	{
		client.processSendData(time, getSocket(), addr);
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
		mSample = new int64[ maxSampleNum ];
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

		mSample[ mCount ] = ( getSystemTime() - mLastRequst ) / 2;
		++mCount;
	}

	int64 getSystemTime()
	{
		return SystemPlatform::GetTickCount();
	}

	int    getSampleNum(){ return mCount; }

	int64 calcResult()
	{
		int64 total = 0;
		for ( int i = 0 ; i < mCount ; ++i )
			total += mSample[i];
		return total / mCount;
	}

	int     mMaxSampleNum;
	int64   mLastRequst;
	int     mCount;
	int64*  mSample;
};

#endif // GameNetConnect_h__