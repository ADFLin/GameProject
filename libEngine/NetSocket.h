#ifndef NetSocket_h__
#define NetSocket_h__

#include "PlatformConfig.h"
#include "CompilerConfig.h"

#if SYS_PLATFORM_WIN
#define  NOMINMAX
#include <WinSock.h>
#pragma comment(lib, "wsock32.lib")
#include <Windows.h>
#else
#error "NetSockt not supported"
#endif
#include <exception>

#define NET_INIT_OK 0

extern WORD  gSockVersion;

class NetSocket;


class SocketDetector
{
public:
	virtual ~SocketDetector(){}


	virtual void onExcept(NetSocket& socket){}

	// server
	virtual void onAcceptable(NetSocket& socket){}
	// client
	virtual void onConnect(NetSocket& socket){}
	virtual void onConnectFailed(NetSocket& socket){}
	virtual void onClose(NetSocket& socket , bool beGraceful ){}
	virtual void onReadable(NetSocket& socket , int len ){}
	virtual void onSendable(NetSocket& socket){}
};


class NetAddress
{
public:
	NetAddress(){}
	explicit NetAddress( sockaddr_in const& addr ){ mAddr = addr; }

	bool setFromSocket( NetSocket const& socket );
	bool setInternet( char const* addrName , unsigned port );
	void setBroadcast( unsigned port );
	char const* getIP() const
	{ 
		return inet_ntoa( mAddr.sin_addr );
	}

	void setPort( unsigned port ){  mAddr.sin_port = htons( port ); }

	sockaddr_in&       get(){ return mAddr; }
	sockaddr_in const& get() const { return mAddr; }

	NetAddress& operator = ( sockaddr_in const& addr ){ mAddr = addr; return *this; }
	
	bool operator == ( sockaddr_in const& addr ) const
	{
		return mAddr.sin_family == addr.sin_family &&
			   mAddr.sin_addr.s_addr == addr.sin_addr.s_addr &&
			   mAddr.sin_port == addr.sin_port;
	}

	bool operator != ( sockaddr_in const& addr ) const
	{
		return !( *this == addr );
	}

	bool operator == ( NetAddress const& addr ) const
	{
		return *this == addr.mAddr;
	}

	bool operator != ( NetAddress const& addr ) const
	{
		return !( *this == addr.mAddr );
	}


private:
	friend class NetSocket;
	sockaddr_in mAddr;
};

enum SocketState
{
	SKS_CONNECTING ,
	SKS_CLOSE      ,
	SKS_LISTING    ,
	SKS_CONNECT    ,
	SKS_UDP        ,
};


class SocketObject
{
public:

	static int getLastError()
	{
		return ::WSAGetLastError();
	}
	bool init( int af,int type,int protocol )
	{
		mHandle = ::socket( af , type , protocol );
		if ( mHandle == INVALID_SOCKET )
			return false;
		return true;
	}
	void close()
	{
		if ( mHandle == INVALID_SOCKET )
			return;
		int rVal = ::closesocket( mHandle );
		if ( rVal != SOCKET_ERROR )
			mHandle =  INVALID_SOCKET;
	}
	bool setopt( int option , int value )
	{
		if ( setsockopt( mHandle , SOL_SOCKET , option , (char *)&value, sizeof(value) ) == SOCKET_ERROR )
			return false;
		return true;
	}
	bool getopt( int option , int& value )
	{
		int len = sizeof(value);
		if ( getsockopt( mHandle , SOL_SOCKET , option , (char *)&value, &len ) == SOCKET_ERROR )
			return false;
		return true;
	}
	bool connect( sockaddr const *to , int tolen )
	{  
		int ret = ::connect( mHandle , to , tolen ); 
		if ( ret == SOCKET_ERROR && getLastError()  != WSAEWOULDBLOCK )
			return false;
		return true;
	}
	int sendto( char const* buf , int len, int flags, sockaddr const *to , int tolen )
	{  return ::sendto( mHandle , buf , len , flags , to , tolen );  }
	int recvfrom( char* buf , int len, int flags, sockaddr* addr , int lenAddr )
	{  return ::recvfrom( mHandle , buf , len , flags , addr , &lenAddr );  }
	int recv( char* buf , int len, int flags )
	{  return ::recv( mHandle , buf , len , flags );  }
	int select( fd_set* fRead , fd_set* fWrite , fd_set* fExcept , timeval const& timeout )
	{
		if ( fRead  ) FD_SET( mHandle , fRead );
		if ( fWrite ) FD_SET( mHandle , fRead );
		if ( fExcept ) FD_SET( mHandle , fRead );
		return ::select( 1, fRead, fWrite, fExcept, &timeout );
	}

	SOCKET       mHandle;
};

class NetSocket
{
public:
	NetSocket();
	explicit NetSocket( SOCKET hSocket )
		:mHandle( hSocket ){}

	~NetSocket();

	SocketState getState(){ return mState; }

	static bool initSystem();
	static void exitSystem();

	bool setBroadcast();

	bool setNonBlocking( bool beNB );
	bool bindPort( unsigned port );
	int  getLastError();

	void move( NetSocket& socket );
public:	// TCP
	bool createTCP( bool beNB );
	bool detectTCP( SocketDetector& detector );


	bool listen( unsigned port , int maxWaitClient );
	bool connect( NetAddress const& addr );
	bool connect( char const* addrName , unsigned port );

	bool accept(NetSocket& clientSocket , NetAddress& address );
	bool accept(NetSocket& clientSocket, sockaddr* addr, int addrLength);

	int  recvData( char* data , size_t maxNum );
	int  sendData( char const* data , size_t num );

public: 	//UDP

	bool createUDP();
	bool detectUDP( SocketDetector& detector );

	int  recvData( char* data , size_t maxNum , sockaddr* addrInfo , int addrLength );
	int  recvData( char* data , size_t maxNum , NetAddress& addr  )
	{
		return recvData(  data ,  maxNum , (sockaddr*)&addr.mAddr , sizeof( addr.mAddr ) );
	}

	int  sendData( char const* data , size_t num , char const* addrName , unsigned port );
	int  sendData( char const* data , size_t num , sockaddr* addrInfo , int addrLength );
	int  sendData( char const* data , size_t num , NetAddress& addr  )
	{
		return sendData( data , num , (sockaddr*)&addr.mAddr , sizeof( addr.mAddr ) );
	}
	void   close();

	SOCKET getHandle() const { return mHandle; }
protected:

	friend class NetAddress;
	
	static char const* getIPByName( char const* AddrName );

private:
	SocketState  mState;
	SOCKET       mHandle;
};


class SocketException : public std::exception
{
public:
	STD_EXCEPTION_CONSTRUCTOR_WITH_WHAT( SocketException )
};

#endif // NetSocket_h__
