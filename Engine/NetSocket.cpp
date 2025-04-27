#include "NetSocket.h"

#include "LogSystem.h"

#include <cstdlib>
#include <cassert>
#include <corecrt_io.h>
#include "Core/Memory.h"

WORD  gSockVersion = MAKEWORD(1,1);

void SocketError(char* str){ }

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
	NetAddress addr;
	if ( !addr.setInternet( addrName , port ) )
		return false;

	return connect( addr );
}

bool NetSocket::connect( NetAddress const& addr )
{
	assert(mHandle != INVALID_SOCKET);

	if ( !FSocket::Connect( getHandle(), (sockaddr*)&addr.get(), sizeof(addr.get())) )
	{
		SocketError("Failed connect()");
		return  false;
	}
	if (mState == SKS_UDP)
	{
		mState = SKS_CONNECTED_UDP;
	}
	else
	{
		mState = SKS_CONNECTING;
	}

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
	return FSocket::Recv( getHandle() , data , (int)maxNum , 0 );
}

int NetSocket::recvData( char* data , size_t maxNum , sockaddr* addrInfo , int addrLength )
{
	return FSocket::Recvfrom( getHandle(), data, (int)maxNum, 0, addrInfo, addrLength );
}

int NetSocket::sendData( char const* data , size_t num )
{
	return ::send( getHandle() , data , (int)num , 0 );
}

int NetSocket::sendData( char const* data , size_t num , char const* addrName , unsigned port )
{
	NetAddress addr;
	if ( !addr.setInternet( addrName , port ) )
		return SOCKET_ERROR;

	return sendData( data , num , addr );
}


int NetSocket::sendData( char const* data , size_t num , sockaddr* addrInfo , int addrLength )
{
	if ( mHandle == INVALID_SOCKET && ! createUDP( ) )
		return SOCKET_ERROR;

	return FSocket::SendTo( getHandle() , data , (int)num , 0 , addrInfo , addrLength );
}

bool NetSocket::createTCP( bool beNB )
{
	close();

	mHandle = FSocket::Create( PF_INET , SOCK_STREAM , 0 );
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
	mHandle = FSocket::Create( PF_INET , SOCK_DGRAM , IPPROTO_UDP );

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
	return FSocket::SetOption(getHandle(), SO_BROADCAST, 1);
}

bool NetSocket::bindPort( unsigned port )
{
	sockaddr_in  addr;
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl( INADDR_ANY );
	addr.sin_port        = htons(port);
	return FSocket::Bind(getHandle(), (sockaddr*)&addr, sizeof(addr));
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

	clientSocket.mState  = SKS_CONNECTED;
	return  true;
}


bool NetSocket::detectTCP( SocketDetector& detector)
{
	if ( mState == SKS_CLOSE )
		return false;

	NetSelectSet selectSet;
	selectSet.addSocket(*this);
	if (!selectSet.select(0))
		return false;

	return detectTCPInternal(detector , selectSet);
}

bool NetSocket::detectTCP(SocketDetector& detector, NetSelectSet& selectSet)
{
	if (mState == SKS_CLOSE)
		return false;

	return detectTCPInternal(detector, selectSet);
}

bool NetSocket::detectTCPInternal(SocketDetector& detector, NetSelectSet& selectSet)
{
	assert(mState != SKS_CLOSE);

	SOCKET hSocket = getHandle();
	switch (mState)
	{
	case  SKS_CONNECTED:
		if (selectSet.canRead(*this))
		{
			int length = 0;
			int rVal = ioctlsocket(hSocket, FIONREAD, (unsigned long *)&length);
			// connection is be gracefully closed
			if (length == 0)
			{
				detector.onClose(*this, true);
				close();
				return true;
				// connection is not be gracefully closeed
			}
			else if (recv(hSocket, 0, 0, 0) == SOCKET_ERROR)
			{
				detector.onClose(*this, false);
				close();
				return true;
			}
			else
			{
				detector.onReadable(*this, length);
			}
		}

		if (selectSet.canWrite(*this))
		{
			detector.onSendable(*this);
		}

		if (selectSet.haveExcept(*this))
		{
			detector.onExcept(*this);
		}
		break;
	case SKS_LISTING:
		if (selectSet.canRead(*this))
		{
			detector.onAcceptable(*this);
		}
		break;
	case SKS_CONNECTING:
		if (selectSet.canWrite(*this))
		{
			mState = SKS_CONNECTED;
			detector.onConnect(*this);
		}
		// connect failed
		if (selectSet.canRead(*this))
		{
			mState = SKS_CLOSE;
			detector.onConnectFailed(*this);
		}
		break;
	}

	return true;
}

bool NetSocket::detectUDP( SocketDetector& detector)
{
	if ( mState == SKS_CLOSE )
		return false;

	NetSelectSet selectSet;
	selectSet.addSocket(*this);
	if (!selectSet.select(0))
		return false;

	return detectUDPInternal(detector, selectSet);

}

bool NetSocket::detectUDP(SocketDetector& detector, NetSelectSet& selectSet)
{
	if (mState == SKS_CLOSE)
		return false;

	return detectUDPInternal(detector, selectSet);
}

bool NetSocket::detectUDPInternal(SocketDetector& detector , NetSelectSet& selectSet)
{
	assert(mState == SKS_UDP || mState == SKS_CONNECTED_UDP);

	SOCKET hSocket = getHandle();

	if (selectSet.canRead(*this))
	{
		while (1)
		{
			unsigned long length = 0;
			int rVal = ioctlsocket(hSocket, FIONREAD, (unsigned long *)&length);

			if (rVal == SOCKET_ERROR || length == 0)
				break;
			detector.onReadable(*this, length);
		}
	}
	if (selectSet.canWrite(*this))
	{
		detector.onSendable(*this);
	}
	if (selectSet.haveExcept(*this))
	{
		detector.onExcept(*this);
	}

	return true;
}

void NetSocket::close()
{
	FSocket::Close(mHandle);
	mState     =  SKS_CLOSE;
}

static bool bNetSocketSystemInitialized = false;
bool NetSocket::StartupSystem()
{
	if ( bNetSocketSystemInitialized )
		return true;

#if SYS_PLATFORM_WIN
	WSADATA wsaData;
	if ( WSAStartup( gSockVersion , &wsaData ) != NET_INIT_OK )
		return false;
#endif


	bNetSocketSystemInitialized  = true;
	return true;
}

void NetSocket::ShutdownSystem()
{
	if ( bNetSocketSystemInitialized )
	{
#if SYS_PLATFORM_WIN
		WSACleanup();
#endif
		bNetSocketSystemInitialized = false;
	}
}

bool NetSocket::IsInitialized()
{
	return bNetSocketSystemInitialized;
}

int NetSocket::getLastError()
{
#if SYS_PLATFORM_WIN
	return WSAGetLastError();
#endif
}

void NetSocket::move( NetSocket& socket )
{
	assert( mHandle == INVALID_SOCKET );

	mHandle = socket.mHandle;
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

	FMemory::Zero( mAddr.sin_zero , 8 );
	return true;
}

void NetAddress::setBroadcast( unsigned port )
{
	mAddr.sin_family = AF_INET;
	mAddr.sin_addr.s_addr = INADDR_BROADCAST;
	mAddr.sin_port = htons( port );

	FMemory::Zero( mAddr.sin_zero , 8 );
}

void NetSelectSet::clear()
{
	mSockets.clear();
}

void NetSelectSet::addSocket(NetSocket& socket)
{
#if _DEBUG
	auto iter = std::find(mSockets.begin(), mSockets.end(), &socket);
	if (iter != mSockets.end())
	{
		LogWarning(0, "Socket object added select set twice");
		return;
	}
#endif
	mSockets.push_back(&socket);
}

void NetSelectSet::removeSocket(NetSocket& socket)
{
	auto iter = std::find(mSockets.begin(), mSockets.end(), &socket);
	if (iter != mSockets.end())
	{
		mSockets.erase(iter);
	}
}

bool NetSelectSet::select(uint64 usec)
{
	timeval TimeOut;
	TimeOut.tv_sec = usec / 1000000;
	TimeOut.tv_usec = usec % 1000000;

	FD_ZERO(&mRead);
	FD_ZERO(&mWrite);
	FD_ZERO(&mExcept);

	int numSockets = 0;

	for( auto pSocket : mSockets )
	{
		SOCKET hSocket = pSocket->getHandle();
		FD_SET(hSocket, &mRead);
		FD_SET(hSocket, &mWrite);
		FD_SET(hSocket, &mExcept);
		++numSockets;
	}

	int num = ::select(numSockets, &mRead, &mWrite, &mExcept, &TimeOut);
	if (num < 0)
	{
		return false;
	}
	if (num == 0)
		return false;

	return true;
}

bool NetSelectSet::canRead(NetSocket& socket)
{
	return !!FD_ISSET(socket.getHandle(), &mRead);
}

bool NetSelectSet::canWrite(NetSocket& socket)
{
	return !!FD_ISSET(socket.getHandle(), &mWrite);
}

bool NetSelectSet::haveExcept(NetSocket& socket)
{
	return !!FD_ISSET(socket.getHandle(), &mExcept);
}
