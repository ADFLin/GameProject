#include "TSocket.h"

#include <cstdlib>
#include <cassert>

WORD  g_sockVersion = MAKEWORD(1,1);

void socketError(char* str){ }

TSocket::TSocket()
	:mSocketObj( INVALID_SOCKET )
	,mState( SKS_CLOSE )
{

}

TSocket::~TSocket()
{
	close();
}

bool TSocket::connect( char const* addrName  , unsigned  port )
{
	assert( mSocketObj == INVALID_SOCKET );
	NetAddress addr;
	if ( !addr.setInternet( addrName , port ) )
		return false;

	return connect( addr );
}

bool TSocket::connect( NetAddress const& addr )
{

	if ( !createTCP( true ) )
	{
		return false;
	}

	int result = ::connect( getSocketObject() , (sockaddr*)&addr.get() , sizeof( addr.get() ) );

	if ( result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK )
	{
		socketError("Failed connect()");
		return  false;
	}
	mState = SKS_CONNECTING;
	return true;
}

bool TSocket::listen( unsigned port , int maxWaitClient )
{
	if ( !createTCP( true ) )
		return false;

	if ( !bindPort( port ) )
		return false;

	int result = ::listen( getSocketObject(), maxWaitClient );
	if( result == SOCKET_ERROR)
	{
		return false;
	}

	mState = SKS_LISTING;
	return true;
}

int TSocket::recvData( char* data , size_t maxNum )
{
	return ::recv( getSocketObject() , data , (int)maxNum , 0 );
}

int TSocket::recvData( char* data , size_t maxNum , sockaddr* addrInfo , int addrLength )
{
	return ::recvfrom( getSocketObject() ,data , (int)maxNum , 0 , addrInfo , &addrLength );
}

int TSocket::sendData( char const* data , size_t num )
{
	return ::send( getSocketObject() , data , (int)num , 0 );
}

int TSocket::sendData( char const* data , size_t num , char const* addrName , unsigned port )
{
	NetAddress addr;
	if ( !addr.setInternet( addrName , port ) )
		return false;

	return sendData( data , num , addr );
}


int TSocket::sendData( char const* data , size_t num , sockaddr* addrInfo , int addrLength )
{
	if ( mSocketObj == INVALID_SOCKET && ! createUDP( ) )
		return false;

	return ::sendto( getSocketObject() , data , (int)num , 0 , addrInfo , addrLength );
}

bool TSocket::createTCP( bool beNB )
{
	close();

	mSocketObj = ::socket( PF_INET , SOCK_STREAM , 0 );
	if ( mSocketObj  == INVALID_SOCKET )
	{
		throw SocketException("Can't create socket");
	}

	setNonBlocking( beNB );
	return true;
}

bool TSocket::createUDP()
{
	close();
	mSocketObj = ::socket( PF_INET , SOCK_DGRAM , IPPROTO_UDP );

	if ( mSocketObj == INVALID_SOCKET )
	{
		throw SocketException("Can't create socket");
	}

	setNonBlocking( true );

	mState = SKS_UDP;

	return true;
}

bool TSocket::setBroadcast()
{
	int i = 1;
	if ( setsockopt( getSocketObject() , SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == SOCKET_ERROR )
		return false;
	return true;
}

bool TSocket::bindPort( unsigned port )
{
	sockaddr_in  addr;
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl( INADDR_ANY );
	addr.sin_port        = htons(port);

	int result;
	//bind the socket
	result = ::bind(getSocketObject(), (sockaddr*)&addr, sizeof(addr));
	if( result == SOCKET_ERROR)
	{
		return false;
	}
	return true;
}


bool TSocket::setNonBlocking( bool beNB )
{
	u_long  nonBlocking = beNB ? 1 : 0;
	int rVal = ioctlsocket( getSocketObject() , FIONBIO , &nonBlocking );
	if ( rVal == SOCKET_ERROR )
	{
		return false;
	}
	return true;
}

char const* TSocket::getIPByName( char const* AddrName )
{
	hostent *pHost = NULL;
	pHost = gethostbyname( AddrName );

	if(!pHost)
	{
		return NULL;
	}
	return inet_ntoa(*(in_addr *)(*pHost->h_addr_list));
}

bool TSocket::accept( TSocket& clinetSocket , sockaddr* addr , int addrLength ) const
{
	assert( clinetSocket.mSocketObj == INVALID_SOCKET );
	clinetSocket.mSocketObj = ::accept( mSocketObj , addr , &addrLength );

	if ( clinetSocket.mSocketObj == INVALID_SOCKET )
		return false;

	clinetSocket.mState  = SKS_CONNECT;
	return  true;
}

bool TSocket::detectTCP( SocketDetector& detector )
{
	if ( mState == SKS_CLOSE )
		return false;

	fd_set fRead;
	fd_set fWrite;
	fd_set fExcept;

	FD_ZERO(&fRead);
	FD_ZERO(&fWrite);
	FD_ZERO(&fExcept);


	SOCKET hSocket = getSocketObject();

	FD_SET( hSocket , &fRead );
	FD_SET( hSocket , &fWrite );
	FD_SET( hSocket , &fExcept);
	
	int rVal;

	timeval TimeOut;
	TimeOut.tv_sec	= 0;
	TimeOut.tv_usec	= 0;
	rVal = select( 1, &fRead, &fWrite, &fExcept, &TimeOut );
	if( rVal == SOCKET_ERROR)
	{
		return false;
	}

	switch ( mState )
	{
	case  SKS_CONNECT:
		if(FD_ISSET( hSocket , &fRead ))
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

		if( FD_ISSET( hSocket , &fWrite ) )
		{
			detector.onSendable( *this );
		}

		if( FD_ISSET( hSocket , &fExcept ) )
		{	
			detector.onExcept( *this );
		}
		break;
	case SKS_LISTING:
		if( FD_ISSET( hSocket ,&fRead) )
		{
			detector.onAcceptable( *this );
		}
		break;
	case SKS_CONNECTING:
		if( FD_ISSET( hSocket , &fWrite ) )
		{
			sockaddr_in Address;
			memset(&Address,0,sizeof(Address));
			mState = SKS_CONNECT;

			detector.onConnect( *this );

		}
		// connect failed
		if(FD_ISSET( hSocket ,&fExcept) )
		{	
			mState = SKS_CLOSE;
			detector.onConnectFailed(*this); 
		}
		break;
	}

	return true;
}

bool TSocket::detectUDP( SocketDetector& detector )
{
	if ( mState == SKS_CLOSE )
		return false;

	assert( mState == SKS_UDP );

	fd_set fRead;
	fd_set fWrite;
	fd_set fExcept;

	FD_ZERO(&fRead);
	FD_ZERO(&fWrite);
	FD_ZERO(&fExcept);

	int rVal;

	timeval TimeOut;
	TimeOut.tv_sec	= 0;
	TimeOut.tv_usec	= 0;

	SOCKET hSocket = getSocketObject();

	FD_SET( hSocket , &fRead  );
	FD_SET( hSocket , &fWrite );
	FD_SET( hSocket , &fExcept);

	rVal = select( 1, &fRead, &fWrite, &fExcept, &TimeOut );
	if( rVal == SOCKET_ERROR)
	{
		return false;
	}

	if ( FD_ISSET( hSocket , &fRead ) )
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
	if ( FD_ISSET( hSocket , &fWrite ) )
	{
		detector.onSendable( *this );
	}
	if ( FD_ISSET( hSocket , &fExcept ) )
	{
		detector.onExcept( *this );
	}

	return true;
}

void TSocket::close()
{
	if ( getSocketObject() != INVALID_SOCKET )
	{
		int rVal = ::closesocket( getSocketObject() );
		if ( rVal == SOCKET_ERROR )
		{

		}
	}
	mSocketObj =  INVALID_SOCKET;
	mState     =  SKS_CLOSE;

}

static bool sInitSystem = false;
void TSocket::exitSystem()
{
	if ( sInitSystem )
	{
		WSACleanup();
		sInitSystem = false;
	}
}

bool TSocket::initSystem()
{
	if ( sInitSystem )
		return true;

	WSADATA wsaData;
	if ( WSAStartup( g_sockVersion , &wsaData ) != NET_INIT_OK )
		return false;


	sInitSystem  = true;
	return true;
}

int TSocket::getLastError()
{
	return WSAGetLastError();
}

void TSocket::move( TSocket& socket )
{
	assert( mSocketObj == INVALID_SOCKET );

	mSocketObj =  socket.mSocketObj;
	mState = socket.mState;

	socket.mSocketObj = INVALID_SOCKET;
	socket.mState = SKS_CLOSE;
}


bool NetAddress::setFromSocket( TSocket const& socket )
{
	int len = sizeof( mAddr );
	return ::getpeername( socket.getSocketObject() , (sockaddr*)&mAddr, &len) != 0;
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
