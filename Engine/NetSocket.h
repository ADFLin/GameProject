#ifndef NetSocket_h__
#define NetSocket_h__

#include "PlatformConfig.h"
#include "CompilerConfig.h"

#include "DataStructure/Array.h"

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
	SKS_CONNECTED  ,
	SKS_UDP        ,
	SKS_CONNECTED_UDP ,
};


class FSocket
{
public:
	static int GetLastError()
	{
		return ::WSAGetLastError();
	}
	static SOCKET Create( int af, int type, int protocol )
	{
		return ::socket( af , type , protocol );
	}
	static void Close(SOCKET& handle)
	{
		if ( handle == INVALID_SOCKET )
			return;
		int rVal = ::closesocket( handle );
		if (rVal == SOCKET_ERROR)
		{


		}
		handle =  INVALID_SOCKET;
	}
	static bool SetOption(SOCKET handle, int option , int value )
	{
		if ( setsockopt( handle , SOL_SOCKET , option , (char *)&value, sizeof(value) ) == SOCKET_ERROR )
			return false;
		return true;
	}
	static bool GetOption(SOCKET handle, int option , int& value )
	{
		int len = sizeof(value);
		if ( getsockopt( handle , SOL_SOCKET , option , (char *)&value, &len ) == SOCKET_ERROR )
			return false;
		return true;
	}

	static bool Bind(SOCKET handle, sockaddr const *to, int tolen)
	{
		int result = ::bind(handle, to, tolen);
		if (result == SOCKET_ERROR)
		{
			return false;
		}
		return true;
	}

	static bool Connect(SOCKET handle, sockaddr const *to , int tolen )
	{  
		int ret = ::connect( handle , to , tolen ); 
		if ( ret == SOCKET_ERROR && GetLastError()  != WSAEWOULDBLOCK )
			return false;
		return true;
	}
	static int SendTo(SOCKET handle, char const* buf , int len, int flags, sockaddr const *to , int tolen )
	{  
		return ::sendto( handle , buf , len , flags , to , tolen );  
	}
	static int Recvfrom(SOCKET handle, char* buf , int len, int flags, sockaddr* addr , int lenAddr )
	{  
		return ::recvfrom( handle , buf , len , flags , addr , &lenAddr );  
	}
	static int Recv(SOCKET handle, char* buf , int len, int flags )
	{  
		return ::recv( handle , buf , len , flags );  
	}
	static int Select(SOCKET handle, fd_set* fRead , fd_set* fWrite , fd_set* fExcept , timeval const& timeout )
	{
		if ( fRead  ) FD_SET( handle , fRead );
		if ( fWrite ) FD_SET( handle , fRead );
		if ( fExcept ) FD_SET( handle , fRead );
		return ::select( 1, fRead, fWrite, fExcept, &timeout );
	}
};

class NetSocket;

struct NetSelectSet
{
	NetSelectSet()
	{
		clear();
	}
	void clear();
	void addSocket(NetSocket& socket);
	void removeSocket(NetSocket& socket);

	bool select(long sec, long usec);

	bool canRead(NetSocket& socket);
	bool canWrite(NetSocket& socket);
	bool haveExcept(NetSocket& socket);

	TArray< NetSocket* > mSockets;
	fd_set mRead;
	fd_set mWrite;
	fd_set mExcept;
};

class NetSocket
{
public:
	NetSocket();
	explicit NetSocket( SOCKET hSocket )
		:mHandle( hSocket ){}

	~NetSocket();

	SocketState getState(){ return mState; }

	static bool StartupSystem();
	static void ShutdownSystem();
	static bool IsInitialized();

	bool setBroadcast();

	bool setNonBlocking( bool beNB );
	bool bindPort( unsigned port );
	int  getLastError();

	void move( NetSocket& socket );
public:	// TCP
	bool createTCP( bool beNB );
	bool detectTCP( SocketDetector& detector);
	bool detectTCP(SocketDetector& detector, NetSelectSet& selectSet);

	bool listen(unsigned port, int maxWaitClient);
	bool connect( NetAddress const& addr );
	bool connect( char const* addrName , unsigned port );

	bool accept(NetSocket& clientSocket , NetAddress& address );
	bool accept(NetSocket& clientSocket, sockaddr* addr, int addrLength);

	int  recvData( char* data , size_t maxNum );
	int  sendData( char const* data , size_t num );

public: 	//UDP

	bool createUDP();
	bool detectUDP(SocketDetector& detector);
	bool detectUDP(SocketDetector& detector, NetSelectSet& selectSet);

	int  recvData(char* data, size_t maxNum, sockaddr* addrInfo, int addrLength);
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
	bool detectTCPInternal(SocketDetector& detector,NetSelectSet& selectSet);
	bool detectUDPInternal(SocketDetector& detector,NetSelectSet& selectSet);

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
