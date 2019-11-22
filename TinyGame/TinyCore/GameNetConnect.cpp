#include "TinyGamePCH.h"
#include "GameNetConnect.h"
#include "ConsoleSystem.h"

#include <algorithm>

TConsoleVariable<bool> CVarClUseConnectedUDP(true, "cl.UseConnectedUDP");

void NetConnection::recvData( NetBufferOperator& bufCtrl , int len , NetAddress* addr )
{
	try 
	{
		if ( bufCtrl.recvData( getSocket() , len , addr ) )
		{
			if ( !mListener )
			{
				bufCtrl.clear();
				return;
			}

			if ( mListener->notifyConnectionRecv( this , bufCtrl.getBuffer() , addr ) )
			{
				bufCtrl.clear();
			}
		}
	}
	catch( std::exception& e )
	{
		bufCtrl.clear();
		LogError( e.what() );
		//cout << e.what() << endl;
	}
}

void NetConnection::close()
{
	mSocket.close();
}

void NetConnection::updateSocket( long time , NetSelectSet* pNetSelect )
{
	doUpdateSocket( time , pNetSelect);
}

bool NetConnection::checkConnectStatus( long time )
{
	long const TimeOut = 30 * 1000;
	if ( time - mLastRespondTime > TimeOut )
	{
		mListener->notifyConnectionClose( this , NetCloseReason::Timeout );
		return false;
	}
	return true;
}

void NetConnection::resolveExcept()
{
	mListener->notifyConnectionExcept( this );
}

UdpConnection::UdpConnection( int recvSize ) :mRecvCtrl( recvSize )
{

}

void UdpConnection::onReadable( NetSocket& socket , int len )
{
	NetAddress clientAddr;
	recvData( mRecvCtrl , len ,
#if 1
		&clientAddr
#else
		socket.getState() == SKS_UDP ? &clientAddr : nullptr 
#endif
	);
}

UdpClient::UdpClient() 
	:UdpConnection( UC_RECV_BUFSIZE ) 
	,ClientBase( UC_SEND_BUFSIZE )
{
	mNetTime = 0;
}


void UdpClient::initialize()
{
	if ( !mSocket.createUDP() )
		throw SocketException("Can't create UDP");

	if ( !mSocket.setBroadcast() )
	{
		LogWarning( 0 , "Socket can't broadcast");
	}
}
void UdpClient::setServerAddr( char const* addrName , unsigned port )
{
#if 0
	if ( !mSocket.createUDP() )
		throw SocketException("Can't create UDP");
#endif

	if ( !mServerAddr.setInternet( addrName , port ) )
		throw SocketException("Can't get server address");

	if (!mSocket.connect(mServerAddr))
	{
		throw SocketException("Can't connect to server");
	}

#if 0
	if ( !mSocket.setBroadcast() )
	{
		LogWarning( 0 , "Socket can't broadcast");
	}
#endif
}

void UdpClient::onSendable( NetSocket& socket )
{
	sendData( socket );
	mListener->notifyConnectionSend( this );
}

void UdpClient::onReadable( NetSocket& socket , int len )
{
	NetAddress clientAddr;
	if (CVarClUseConnectedUDP && socket.getState() == SKS_CONNECTED_UDP )
		recvData( mRecvCtrl, len, nullptr);
	else
		recvData( mRecvCtrl , len , &clientAddr );
}

bool UdpClient::sendData(NetSocket& socket)
{
	TLockedObject< SocketBuffer > buffer = mSendCtrl.lockBuffer();

	return mChain.sendPacket(mNetTime, socket, *buffer,
		CVarClUseConnectedUDP ? nullptr : &mServerAddr
	);
}

void UdpClient::clearBuffer()
{
	{
		TLockedObject< SocketBuffer > buffer = mSendCtrl.lockBuffer();
		buffer->clear();
	}
	{
		TLockedObject< SocketBuffer > buffer = mRecvCtrl.lockBuffer();
		buffer->clear();
	}
}

void UdpServer::run( unsigned port )
{
	mPort = port;

	if( !mSocket.createUDP() )
		throw SocketException("Can't Create UDP Socket");

	if ( !mSocket.bindPort( mPort ) )
		throw SocketException("Can't Bind Port");

	if ( !mSocket.setBroadcast() )
	{
		LogWarning(0, "Socket can't broadcast");
	}
}

void UdpServer::onSendable( NetSocket& socket )
{
	mListener->notifyConnectionSend( this );
}


void TcpClient::connect( char const* addrName , unsigned port )
{
	if ( !mSocket.createTCP(true) )
		throw SocketException("Can't create TCP");
	if ( !mSocket.connect( addrName , port ) )
		throw SocketException("Can't Connect Sever" );
}

void TcpClient::clearBuffer()
{
	{
		TLockedObject< SocketBuffer > buffer = getSendCtrl().lockBuffer();
		buffer->clear();
	}
	{
		TLockedObject< SocketBuffer > buffer = getRecvCtrl().lockBuffer();
		buffer->clear();
	}
}

void TcpClient::onReadable( NetSocket& socket , int len )
{
	recvData( mRecvCtrl , len , NULL );
}

void TcpClient::onSendable( NetSocket& socket )
{
	mSendCtrl.sendData( socket );
}


void TcpClient::onConnectFailed( NetSocket& socket )
{
	LogMsg( "Connect Failed" );
	mListener->notifyConnectionFail( this );
}

void TcpClient::onConnect( NetSocket& socket )
{
	LogMsg( "Connect success" );
	mListener->notifyConnectionOpen( this );
}

void TcpClient::onClose( NetSocket& socket , bool beGraceful )
{
	LogMsg( "Connection close" );
	mListener->notifyConnectionClose( this , NetCloseReason::ShutDown );
}

void TcpServer::onAcceptable( NetSocket& socket )
{
	LogMsg( "Client Connection" );
	mListener->notifyConnectionAccpet( this );
}

void NetBufferOperator::fillBuffer( SocketBuffer& buffer , unsigned num )
{
	NET_MUTEX_LOCK( mMutexBuffer );

	bool done = false;
	int  count = 1;

	while ( !done )
	{
		try
		{
			mBuffer.fill( buffer , num );
			done = true;
		}
		catch ( BufferException& e )
		{
			mBuffer.grow( ( mBuffer.getMaxSize() * 3 ) / 2 );
			LogMsg( "%s(%d)" , e.what() ,count );
		}
		++count;
	}
}

bool NetBufferOperator::sendData( NetSocket& socket , NetAddress* addr )
{
	NET_MUTEX_LOCK( mMutexBuffer );

	int count = 0;

	while ( mBuffer.getAvailableSize() )
	{
		int numSend;
		if ( addr )
		{
			numSend = mBuffer.take( socket , *addr ); //UDP
		}
		else
		{
			numSend = mBuffer.take( socket );         //TCP or Connected UDP
		}

		if ( !numSend )
			return false;


		if ( mBuffer.getAvailableSize() )
		{
			++count;
			if ( count > 10 )
			{
				LogMsg( "Send Buffer: %d data no send" , mBuffer.getAvailableSize() );
			}
			return false;
		}
		else
		{
			break;
		}
	}

	mBuffer.clear();
	return true;
}

bool NetBufferOperator::recvData(NetSocket& socket, int len, NetAddress* addr /*= NULL */)
{
	NET_MUTEX_LOCK(mMutexBuffer);

	int count = 0;
	while (count < 10)
	{
		try
		{
			int num = 0;

			if (addr)
			{
				num = mBuffer.fill(socket, len, *addr);

			}
			else
			{
				num = mBuffer.fill(socket, len);
			}
			return num != 0;
		}
		catch (std::exception& e)
		{
			++count;
			mBuffer.resize(std::min<int>(len, mBuffer.getMaxSize() * 3 / 2));
			LogMsg("%s(%d) : %s", __FILE__, __LINE__, e.what());
			//LogError(e.what());
			
			//cout << e.what() << endl;
		}
	}
	
	return false;
}

void NetBufferOperator::clear()
{
	NET_MUTEX_LOCK( mMutexBuffer )
	mBuffer.clear();
}

UdpChain::UdpChain() 
	:mBufferCache(10240)
	,mBufferRel(10240)
{
	mIncomingAck = 0;
	mOutgoingSeq = 0;
	mOutgoingSeq = 0;
	mOutgoingAck = 0;
	mTimeLastUpdate = 0;
	mTimeResendRel  = 5;
	mbNeedSendAck = false;
}

struct DBGData
{
	uint32   outgoing;
	uint32   incoming;
	unsigned size;
};
int checkBuffer( char const* buf , int num )
{
#ifdef _DEBUG
	DBGData data[ 64 ];
	int idx = 0;
	while( num > 0 )
	{
		data[ idx ].outgoing = *((uint32*)(buf));
		data[ idx ].incoming = *((uint32*)(buf + 4) );
		data[ idx ].size =     *((uint32*)(buf + 8) );

		int offset = 12 + data[ idx ].size; 
		num -= offset;
		buf += offset;
		++idx;
		if ( idx >= 64 )
			return 0;
	}

	assert( num == 0 );
#endif
	return num;
}

bool UdpChain::sendPacket( long time , NetSocket& socket , SocketBuffer& buffer , NetAddress* addr  )
{

	bool bNeedSendRelData = mBufferRel.getFillSize() && ( time - mTimeLastUpdate > mTimeResendRel );
	bool bNeedSend = buffer.getFillSize() != 0 || mbNeedSendAck || bNeedSendRelData;

	if (!bNeedSend)
		return false;

	uint32 bufSize = (uint32)buffer.getFillSize();
	if ( bufSize )
	{
		++mOutgoingSeq;

		if ( mBufferRel.getFreeSize() < bufSize )
		{
			mBufferRel.grow( mBufferRel.getMaxSize() * 2 );
			LogMsg( "UDPChain::sendPacket : ReliableBuffer too small" );
		}

		size_t oldSize = mBufferRel.getFillSize();

		mBufferRel.fill( mOutgoingSeq );
		mBufferRel.fill( mIncomingAck );
		mBufferRel.fill( bufSize );
		mBufferRel.append( buffer );

		checkBuffer( mBufferRel.getData() , (int)mBufferRel.getFillSize() );

		DataInfo info;
		info.size     = (uint32)( mBufferRel.getFillSize() - oldSize );
		info.sequence = mOutgoingSeq;
		mInfoList.push_back( info );

		buffer.clear();
	}


	mBufferCache.clear();
	try
	{
		if ( mOutgoingAck < mOutgoingSeq )
		{
			mBufferCache.append( mBufferRel );
		}
		else
		{
			mBufferCache.fill( mOutgoingSeq );
			mBufferCache.fill( mIncomingAck );
			mBufferCache.fill( 0 );
		}

		int count = 0;
		while( mBufferCache.getAvailableSize() )
		{
			int numSend;
			if (addr)
			{
				numSend = mBufferCache.take(socket, *addr);
			}
			else
			{
				numSend = mBufferCache.take(socket);
			}

			if( numSend )
			{
				//LogDevMsg(0, "Send UDP Data : size = %d", numSend);
			}
			else
			{
				LogDevMsg(0, "Can't send UDP Data");
			}

			++count;
			if ( count == 10 )
			{
				return false;
			}
		}
		mTimeLastUpdate = time;
	}
	catch ( BufferException& )
	{
		mBufferCache.resize( mBufferCache.getMaxSize() * 2 );
		return false;
	}

	//LogMsg( "sendPacket %u %u %u" , outgoing , incoming , bufSize );
	return true;

}

bool UdpChain::readPacket( SocketBuffer& buffer , uint32& readSize )
{
	if ( !buffer.getAvailableSize() )
		return false;

	uint32  incoming;
	uint32  outgoing;

	int count = 0;

	uint32 maxOutgoing = 0;
	while( buffer.getAvailableSize() )
	{
		buffer.take( incoming );
		buffer.take( outgoing );
		buffer.take( readSize );

		if ( maxOutgoing < outgoing )
			maxOutgoing = outgoing;

		if ( incoming <= mIncomingAck )
		{

			if ( readSize > buffer.getAvailableSize() )
			{
				LogMsg( "UdpChain::error incoming data" );
				return false;
			}

			{
				//::Msg( "UdpChain::old incoming data" );
				buffer.shiftUseSize( readSize );
			}
		}
		else if ( incoming > mIncomingAck )
		{
			mIncomingAck = incoming;
			mbNeedSendAck = true;
			if (readSize)
			{
				break;
			}
		}

		++count;
	}


	
	refrushReliableData( maxOutgoing );
	LogMsg( "readPacket %u %u %u" , outgoing , incoming , readSize );
	return buffer.getAvailableSize() != 0;
}

void UdpChain::refrushReliableData( unsigned outgoing )
{
	if ( mOutgoingAck == mOutgoingSeq )
		return;

	mOutgoingAck = outgoing;

	uint32 endPos = 0;
	DataInfoList::iterator iter = mInfoList.begin();
	for(  ; iter != mInfoList.end() ; ++iter )
	{
		DataInfo& info = *iter;

		if ( info.sequence > mOutgoingAck )
			break;

		endPos += info.size;
	}

	if ( endPos )
	{
		mInfoList.erase( mInfoList.begin() , iter );
		mBufferRel.shiftUseSize( endPos );
		mBufferRel.removeUseData();
		checkBuffer( mBufferRel.getData() , (int)mBufferRel.getFillSize() );
	}
}

