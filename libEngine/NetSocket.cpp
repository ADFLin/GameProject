#include "NetSocket.h"

#include <cstdlib>
#include <cassert>

WORD  gSockVersion = MAKEWORD(1,1);

void socketError(char* str){ }

NetSocket::NetSocket()
	:mHandle( INVALID_SOCKET )
	,mState( SKS_CLOSE )
{

}

NetSocket::~NetSocket()
{
	close();
}

bool NetSocket::connect( char const* addrName  , unsigned  port )
{
	assert( mHandle == INVALID_SOCKET );
	NetAddress addr;
	if ( !addr.setInternet( addrName , port ) )
		return false;

	return connect( addr );
}

bool NetSocket::connect( NetAddress const& addr )
{

	if ( !createTCP( true ) )
	{
		return false;
	}

	int result = ::connect( getHandle() , (sockaddr*)&addr.get() , sizeof( addr.get() ) );

	if ( result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK )
	{
		socketError("Failed connect()");
		return  false;
	}
	mState = SKS_CONNECTING;
	return true;
}

bool NetSocket::accept(NetSocket& clientSocket, NetAddress& address)
{
	return accept( clientSocket, (sockaddr*)&address.get(), sizeof(address.get()) );
}

bool NetSocket::listen( unsigned port , int maxWaitClient )
{
	if ( !createTCP( true ) )
		return false;

	if ( !bindPort( port ) )
		return false;

	int result = ::listen( getHandle(), maxWaitClient );
	if( result == SOCKET_ERROR)
	{
		return false;
	}

	mState = SKS_LISTING;
	return true;
}

int NetSocket::recvData( char* data , size_t maxNum )
{
	return ::recv( getHandle() , data , (int)maxNum , 0 );
}

int NetSocket::recvData( char* data , size_t maxNum , sockaddr* addrInfo , int addrLength )
{
	return ::recvfrom( getHandle() ,data , (int)maxNum , 0 , addrInfo , &addrLength );
}

int NetSocket::sendData( char const* data , size_t num )
{
	return ::send( getHandle() , data , (int)num , 0 );
}

int NetSocket::sendData( char const* data , size_t num , char const* addrName , unsigned port )
{
	NetAddress addr;
	if ( !addr.setInternet( addrName , port ) )
		return false;

	return sendData( data , num , addr );
}


int NetSocket::sendData( char const* data , size_t num , sockaddr* addrInfo , int addrLength )
{
	if ( mHandle == INVALID_SOCKET && ! createUDP( ) )
		return false;

	return ::sendto( getHandle() , data , (int)num , 0 , addrInfo , addrLength );
}

bool NetSocket::createTCP( bool beNB )
{
	close();

	mHandle = ::socket( PF_INET , SOCK_STREAM , 0 );
	if ( mHandle  == INVALID_SOCKET )
	{
		throw SocketException("Can't create socket");
	}

	setNonBlocking( beNB );
	return true;
}

bool NetSocket::createUDP()
{
	close();
	mHandle = ::socket( PF_INET , SOCK_DGRAM , IPPROTO_UDP );

	if ( mHandle == INVALID_SOCKET )
	{
		throw SocketException("Can't create socket");
	}

	setNonBlocking( true );

	mState = SKS_UDP;

	return true;
}

bool NetSocket::setBroadcast()
{
	int i = 1;
	if ( setsockopt( getHandle() , SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == SOCKET_ERROR )
		return false;
	return true;
}

bool NetSocket::bindPort( unsigned port )
{
	sockaddr_in  addr;
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl( INADDR_ANY );
	addr.sin_port        = htons(port);

	int result;
	//bind the socket
	result = ::bind(getHandle(), (sockaddr*)&addr, sizeof(addr));
	if( result == SOCKET_ERROR)
	{
		return false;
	}
	return true;
}


bool NetSocket::setNonBlocking( bool beNB )
{
	u_long  nonBlocking = beNB ? 1 : 0;
	int rVal = ioctlsocket( getHandle() , FIONBIO , &nonBlocking );
	if ( rVal == SOCKET_ERROR )
	{
		return false;
	}
	return true;
}

char const* NetSocket::getIPByName( char const* AddrName )
{
	hostent *pHost = NULL;
	pHost = gethostbyname( AddrName );

	if(!pHost)
	{
		return NULL;
	}
	return inet_ntoa(*(in_addr *)(*pHost->h_addr_list));
}

bool NetSocket::accept( NetSocket& clientSocket , sockaddr* addr , int addrLength )
{
	assert( clientSocket.mHandle == INVALID_SOCKET );
	clientSocket.mHandle = ::accept( mHandle , addr , &addrLength );

	if ( clientSocket.mHandle == INVALID_SOCKET )
		return false;

	clientSocket.mState  = SKS_CONNECT;
	return  true;
}

struct NetSelectSet 
{
	NetSelectSet()
	{
		clear();
	}
	void clear()
	{
		numFD = 0;
		FD_ZERO(&read);
		FD_ZERO(&write);
		FD_ZERO(&except);
	}
	void addSocket(NetSocket& socket)
	{
		SOCKET hSocket = socket.getHandle();
		FD_SET(hSocket, &read);
		FD_SET(hSocket, &write);
		FD_SET(hSocket, &except);
		++numFD;
	}

	int select(long sec, long usec)
	{
		timeval TimeOut;
		TimeOut.tv_sec = sec;
		TimeOut.tv_usec = usec;
		return ::select(numFD, &read, &write, &except, &TimeOut);
	}

	bool canRead(NetSocket& socket)
	{
		return FD_ISSET( socket.getHandle() , &read );
	}
	bool canWrite(NetSocket& socket)
	{
		return FD_ISSET(socket.getHandle(), &write);
	}
	bool haveExcept(NetSocket& socket)
	{
		return FD_ISSET(socket.getHandle(), &except);
	}

	int numFD;
	fd_set read;
	fd_set write;
	fd_set except;
};

bool NetSocket::detectTCP( SocketDetector& detector )
{
	if ( mState == SKS_CLOSE )
		return false;

	int rVal;

	NetSelectSet selectSet;
	selectSet.addSocket(*this);
	rVal = selectSet.select( 0 , 0 );
	if( rVal == SOCKET_ERROR)
	{
		return false;
	}


	SOCKET hSocket = getHandle();
	switch ( mState )
	{
	case  SKS_CONNECT:
		if( selectSet.canRead( *this ))
		{
			int length = 0;
			rVal = ioctlsocket( hSocket , FIONREAD ,(unsigned long *)&length); 
			// connection is be gracefully closed
			if( length == 0 )
			{ 
				detector.onClose( *this , true );
				close();
				return true;
				// connection is not be gracefully closeed
			}
			else if( recv( hSocket ,0,0,0) == SOCKET_ERROR )
			{ 
				detector.onClose( *this , false );
				close();
				return true;
			}
			else
			{
				detector.onReadable( *this , length );
			}
		}

		if( selectSet.canWrite(*this) )
		{
			detector.onSendable( *this );
		}

		if( selectSet.haveExcept( *this ) )
		{	
			detector.onExcept( *this );
		}
		break;
	case SKS_LISTING:
		if( selectSet.canRead(*this) )
		{
			detector.onAcceptable( *this );
		}
		break;
	case SKS_CONNECTING:
		if( selectSet.canWrite(*this) )
		{
			sockaddr_in Address;
			memset(&Address,0,sizeof(Address));
			mState = SKS_CONNECT;

			detector.onConnect( *this );

		}
		// connect failed
		if( selectSet.canRead(*this) )
		{	
			mState = SKS_CLOSE;
			detector.onConnectFailed(*this); 
		}
		break;
	}

	return true;
}

bool NetSocket::detectUDP( SocketDetector& detector )
{
	if ( mState == SKS_CLOSE )
		return false;

	assert( mState == SKS_UDP );

	int rVal;

	NetSelectSet selectSet;
	selectSet.addSocket(*this);
	rVal = selectSet.select(0, 0);

	if( rVal == SOCKET_ERROR)
	{
		return false;
	}

	SOCKET hSocket = getHandle();

	if ( selectSet.canRead( *this ) )
	{
		while ( 1 )
		{
			unsigned long length = 0;
			rVal = ioctlsocket( hSocket , FIONREAD ,(unsigned long *)&length);

			if ( rVal == SOCKET_ERROR || length == 0 )
				break;
			detector.onReadable( *this , length );
		}
	}
	if ( selectSet.canWrite( *this ) )
	{
		detector.onSendable( *this );
	}
	if ( selectSet.haveExcept( *this ) )
	{
		detector.onExcept( *this );
	}

	return true;
}

void NetSocket::close()
{
	if ( getHandle() != INVALID_SOCKET )
	{
		int rVal = ::closesocket( getHandle() );
		if ( rVal == SOCKET_ERROR )
		{

		}
	}
	mHandle =  INVALID_SOCKET;
	mState     =  SKS_CLOSE;

}

static bool sInitSystem = false;
bool NetSocket::initSystem()
{
	if ( sInitSystem )
		return true;

	WSADATA wsaData;
	if ( WSAStartup( gSockVersion , &wsaData ) != NET_INIT_OK )
		return false;


	sInitSystem  = true;
	return true;
}

void NetSocket::exitSystem()
{
	if ( sInitSystem )
	{
		WSACleanup();
		sInitSystem = false;
	}
}

int NetSocket::getLastError()
{
	return WSAGetLastError();
}

void NetSocket::move( NetSocket& socket )
{
	assert( mHandle == INVALID_SOCKET );

	mHandle =  socket.mHandle;
	mState = socket.mState;

	socket.mHandle = INVALID_SOCKET;
	socket.mState = SKS_CLOSE;
}


bool NetAddress::setFromSocket( NetSocket const& socket )
{
	int len = sizeof( mAddr );
	return ::getpeername( socket.getHandle() , (sockaddr*)&mAddr, &len) != 0;
}

bool NetAddress::setInternet( char const* addrName , unsigned port )
{
	hostent* host = gethostbyname( addrName );
	if ( !host )
		return false;
	mAddr.sin_family = AF_INET;
	mAddr.sin_addr.s_addr = *( ( unsigned long*) host->h_addr );
	mAddr.sin_port = htons( port );

	ZeroMemory( mAddr.sin_zero , 8 );
	return true;
}

void NetAddress::setBroadcast( unsigned port )
{
	mAddr.sin_family = AF_INET;
	mAddr.sin_addr.s_addr = INADDR_BROADCAST;
	mAddr.sin_port = htons( port );

	ZeroMemory( mAddr.sin_zero , 8 );
}
