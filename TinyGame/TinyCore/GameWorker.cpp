#include "TinyGamePCH.h"
#include "GameWorker.h"
#include "SystemPlatform.h"

ComWorker::ComWorker() 
	:mNAState( NAS_DISSCONNECT )
	,mComListener( NULL )
{

}


void ComWorker::update( long time )
{
	if ( mComListener )
	{
		mComListener->prevProcCommand();
		mCPEvaluator.procCommand( *mComListener );
		mComListener->postProcCommand();
	}
	else
	{
		mCPEvaluator.procCommand();
	}

	doUpdate( time );
}

void ComWorker::changeState( NetActionState state )
{
	if ( state == mNAState )
		return;

	NetActionState oldState = mNAState;
	mNAState = state;
	postChangeState( oldState );
}

NetWorker::NetWorker() 
	:mUdpSendBuffer( 1024 )
	,mSocketThread( SocketFun( this , &NetWorker::procSocketThread ) )
{
	mNetListener = NULL;
}

NetWorker::~NetWorker()
{
	{
		MUTEX_LOCK( mMutexUdpComList );
		mUdpComList.clear();
	}
	mSocketThread.stop();
	NetSocket::exitSystem();
}

uint32 gSocketThreadId = 0;
bool IsInSocketThread()
{
	return gSocketThreadId == PlatformThread::GetCurrentThreadId();
}

void NetWorker::procSocketThread()
{
	gSocketThreadId = PlatformThread::GetCurrentThreadId();

	mNetRunningTime = 0;
	int64 beforeTime = SystemPlatform::GetTickCount();

	while( 1 )
	{
		int64 intervalTime = SystemPlatform::GetTickCount() - beforeTime;

		mNetRunningTime += intervalTime;
		beforeTime += intervalTime;

		try
		{
			if ( !updateSocket( mNetRunningTime ) )
				break;

			SystemPlatform::Sleep(0);
		}
		catch( ComException& e )
		{
			LogMsg( e.what() );
		}
		catch( SocketException& e )
		{
			LogMsg( e.what() );
		}
		catch( BufferException& e )
		{
			LogMsg( e.what() );
		}
		catch( std::exception& e )
		{
			LogMsg( e.what() );
		}
	}
}

bool NetWorker::startNetwork()
{
	try 
	{
		if ( !NetSocket::initSystem() )
			return false;

		if ( !doStartNetwork() )
			return false;

		if ( !mSocketThread.start() )
			return false;

	}
	catch ( std::exception& e )
	{
		LogError( e.what() );
		return false;
	}

	return true;
}

void NetWorker::closeNetwork()
{
	MUTEX_LOCK( mMutexUdpComList );
	mUdpComList.clear();

	mSocketThread.stop();
	doCloseNetwork();

	NetSocket::exitSystem();
}

void NetWorker::sendUdpCom( NetSocket& socket )
{
	MUTEX_LOCK( mMutexUdpComList );
	UdpComList::iterator iter = mUdpComList.begin();
	for( ; iter != mUdpComList.end() ; ++iter )
	{
		UdpCom& uc = *iter;
		try
		{
			int numSend = mUdpSendBuffer.take( socket , uc.dataSize , uc.addr );

			if ( numSend != uc.dataSize )
			{
				LogWarning(0 , "Udp can't send full Data");
				mUdpSendBuffer.shiftUseSize( -numSend );
				break;
			}
		}
		catch ( BufferException& /*e*/ )
		{
			LogError("Send Udp Com Error" );
			mUdpSendBuffer.clear();
			mUdpComList.clear();
			return;
		}
	}

	mUdpComList.erase( mUdpComList.begin() , iter );
	mUdpSendBuffer.removeUseData();
}

bool NetWorker::addUdpCom( IComPacket* cp , NetAddress const& addr )
{
	try
	{
		MUTEX_LOCK( mMutexUdpComList );
		size_t fillSize = ComEvaluator::WriteBuffer( mUdpSendBuffer , cp );
		UdpCom uc;
		uc.addr     = addr;
		uc.dataSize = fillSize;
		mUdpComList.push_back( uc );
	}
	catch ( ... )
	{
		return false;
	}

	return true;
}



bool EvalCommand( UdpChain& chain , ComEvaluator& evaluator , SocketBuffer& buffer , int group , void* userData )
{
	uint32 readSize;
	while( chain.readPacket( buffer , readSize ) )
	{	
		size_t oldSize = buffer.getAvailableSize();

		if ( oldSize < readSize )
			throw ComException( "error UDP Packet" );

		do
		{
			if ( !evaluator.evalCommand( buffer , group , userData ) )
			{
				LogMsgF( "readPacket Error Need Fix" );
				return false;
			}
		}
		while( oldSize - buffer.getAvailableSize() < readSize );

		if ( oldSize - buffer.getAvailableSize() != readSize )
			throw ComException( "error UDP Packet" );
	}

	return true;
}

unsigned WriteComToBuffer(NetBufferOperator& bufferCtrl , IComPacket* cp)
{
	TLockedObject< SocketBuffer > buffer = bufferCtrl.lockBuffer();
	return WriteComToBuffer( *buffer , cp );
}

unsigned WriteComToBuffer( SocketBuffer& buffer , IComPacket* cp )
{
	assert( cp );

	bool done = false;
	int  count = 1;
	unsigned result;
	while ( !done )
	{
		try
		{
			result = ComEvaluator::WriteBuffer( buffer , cp );
			done = true;
		}
		catch ( BufferException& e )
		{
			buffer.grow( ( buffer.getMaxSize() * 3 ) / 2 );
			LogMsgF( e.what() );
		}
		catch ( ComException& e )
		{
			LogMsgF( e.what() );
			return 0;
		}
		++count;
	}
	return result;
}
