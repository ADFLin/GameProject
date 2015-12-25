#include "TinyGamePCH.h"
#include "GameNetConnect.h"

void NetConnection::recvData( NetBufferCtrl& bufCtrl , int len , NetAddress* addr )
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

			if ( mListener->onRecvData( this , bufCtrl.getBuffer() , addr ) )
			{
				bufCtrl.clear();
			}
		}
	}
	catch( std::exception& e )
	{
		bufCtrl.clear();
		ErrorMsg( e.what() );
		//cout << e.what() << endl;
	}
}

void NetConnection::close()
{
	mSocket.close();
}

void NetConnection::updateSocket( long time )
{
	doUpdateSocket( time );
}

bool NetConnection::checkConnect( long time )
{
	long const TimeOut = 1 * 1000;
	if ( time - mLastRespondTime > TimeOut )
	{
		mListener->onClose( this , CCR_TIMEOUT );
		return false;
	}
	return true;
}

void NetConnection::resolveExcept()
{
	mListener->onExcept( this );
}

UdpConnection::UdpConnection( int recvSize ) :mRecvCtrl( recvSize )
{

}

void UdpConnection::onReadable( TSocket& socket , int len )
{
	NetAddress clientAddr;
	recvData( mRecvCtrl , len , &clientAddr );
}

UdpClient::UdpClient() 
	:UdpConnection( UC_RECV_BUFSIZE ) 
	,ClientBase( UC_SEND_BUFSIZE )
{
	mNetTime = 0;
}


void UdpClient::init()
{
	if ( !mSocket.createUDP() )
		throw SocketException("Can't Create UDP");

	if ( !mSocket.setBroadcast() )
	{
		Msg( "Socket can't broadcast");
	}
}
void UdpClient::setServerAddr( char const* addrName , unsigned port )
{
	if ( !mSocket.createUDP() )
		throw SocketException("Can't Create UDP");

	if ( !mServerAddr.setInternet( addrName , port ) )
		throw SocketException("Can't Get Server Address");

	if ( !mSocket.setBroadcast() )
	{
		Msg( "Socket can't broadcast");
	}
}

void UdpClient::onSendable( TSocket& socket )
{
	sendData( socket );
	mListener->onSendData( this );
}

void UdpClient::onReadable( TSocket& socket , int len )
{
	NetAddress clientAddr;
	recvData( mRecvCtrl , len , &clientAddr );
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
		Msg( "Socket can't broadcast");
	}
}

void UdpServer::onSendable( TSocket& socket )
{
	mListener->onSendData( this );
}


void TcpClient::connect( char const* addrName , unsigned port )
{
	if ( !mSocket.connect( addrName , port ) )
		throw SocketException("Can't Connect Sever" );
}

void TcpClient::onReadable( TSocket& socket , int len )
{
	recvData( mRecvCtrl , len , NULL );
}

void TcpClient::onSendable( TSocket& socket )
{
	mSendCtrl.sendData( socket );
}


void TcpClient::onConnectFailed( TSocket& socket )
{
	Msg( "Connect Failed" );
	mListener->onConnectFailed( this );
}

void TcpClient::onConnect( TSocket& socket )
{
	Msg( "Connect success" );
	mListener->onConnect( this );
}

void TcpClient::onClose( TSocket& socket , bool beGraceful )
{
	Msg( "Connection close" );
	mListener->onClose( this , CCR_SHUTDOWN );
}

void TcpServer::onAcceptable( TSocket& socket )
{
	Msg( "Client Connection" );
	mListener->onAccpetClient( this );
}

void NetBufferCtrl::fillBuffer( SBuffer& buffer , unsigned num )
{
	MUTEX_LOCK( mMutexBuffer );

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
			Msg( "%s(%d)" , e.what() ,count );
		}
		++count;
	}
}

bool NetBufferCtrl::sendData( TSocket& socket , NetAddress* addr )
{
	MUTEX_LOCK( mMutexBuffer );

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
			numSend = mBuffer.take( socket );         //TCP
		}

		if ( !numSend )
			return false;


		if ( mBuffer.getAvailableSize() )
		{
			++count;
			if ( count > 10 )
			{
				Msg( "Send Buffer: %d data no send" , mBuffer.getAvailableSize() );
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

bool NetBufferCtrl::recvData( TSocket& socket , int len , NetAddress* addr /*= NULL */ )
{
	try 
	{
		MUTEX_LOCK( mMutexBuffer );
		int num = 0;

		if ( addr )
		{
			num = mBuffer.fill( socket , len , *addr  );

		}
		else
		{
			num = mBuffer.fill( socket , len );	
		}
		return num != 0;
	}
	catch( std::exception& e )
	{
		mBuffer.clear();
		Msg( "%s(%d) : %s" , __FILE__ , __LINE__ ,e.what() );
		ErrorMsg( e.what() );
		return false;
		//cout << e.what() << endl;
	}
	
	return true;
}

void NetBufferCtrl::clear()
{
	MUTEX_LOCK( mMutexBuffer )
	mBuffer.clear();
}

UdpChain::UdpChain() 
	:mBufferCache(10240)
	,mBufferRel(10240)
{
	mIncomingAck = 0;
	mOutgoingSeq = 0;
	mOutgoingAck = 0;
	mOutgoingRel = 0;
	mTimeLastUpdate = 0;
	mTimeResendRel  = 15;
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

bool UdpChain::sendPacket( long time , TSocket& socket , SBuffer& buffer , NetAddress& addr  )
{
	if ( buffer.getFillSize() == 0 && time - mTimeLastUpdate > mTimeResendRel )
	{
		if ( mBufferRel.getFillSize() == 0 )
			return false;
	}

	uint32  outgoing = ++mOutgoingSeq;
	uint32  incoming = mIncomingAck;

	uint32 bufSize = (uint32)buffer.getFillSize();

	if ( bufSize )
	{
		if ( mBufferRel.getFreeSize() < bufSize )
		{
			mBufferRel.grow( mBufferRel.getMaxSize() * 2 );
			Msg( "UDPChain::sendPacket : ReliableBuffer too small" );
		}

		size_t oldSize = mBufferRel.getFillSize();

		mBufferRel.fill( outgoing );
		mBufferRel.fill( incoming );
		mBufferRel.fill( bufSize );
		mBufferRel.append( buffer );

		checkBuffer( mBufferRel.getData() , (int)mBufferRel.getFillSize() );

		DataInfo info;
		info.size     = (uint32)( mBufferRel.getFillSize() - oldSize );
		info.sequence = outgoing;
		mInfoList.push_back( info );

		buffer.clear();
		mOutgoingAck = outgoing;
	}


	mBufferCache.clear();
	try
	{
		if ( mOutgoingRel < mOutgoingAck )
		{
			mBufferCache.append( mBufferRel );
		}
		else
		{
			mBufferCache.fill( outgoing );
			mBufferCache.fill( incoming );
			mBufferCache.fill( 0 );
		}

		int count = 0;
		while( mBufferCache.getAvailableSize() )
		{
			int numSend = mBufferCache.take( socket , addr );

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

	//Msg( "sendPacket %u %u %u" , outgoing , incoming , bufSize );
	return true;

}

bool UdpChain::readPacket( SBuffer& buffer , unsigned& readSize )
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
				::Msg( "UdpChain::error incoming data" );
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

			if ( readSize )
				break;
		}

		++count;
	}

	refrushReliableData( maxOutgoing );
	//Msg( "readPacket %u %u %u" , outgoing , incoming , readSize );
	return buffer.getAvailableSize() != 0;
}

void UdpChain::refrushReliableData( unsigned outgoing )
{
	if ( mOutgoingRel == mOutgoingAck )
		return;

	mOutgoingRel = outgoing;

	uint32 endPos = 0;
	DataInfoList::iterator iter = mInfoList.begin();
	for(  ; iter != mInfoList.end() ; ++iter )
	{
		DataInfo& info = *iter;

		if ( info.sequence > mOutgoingRel )
			break;

		endPos += info.size;
	}

	if ( endPos )
	{
		mInfoList.erase( mInfoList.begin() , iter );
		mBufferRel.shiftUseSize( endPos );
		mBufferRel.clearUseData();
		checkBuffer( mBufferRel.getData() , (int)mBufferRel.getFillSize() );
	}
}

