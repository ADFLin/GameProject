#include "TinyGamePCH.h"
#include "GameWorker.h"


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
	long beforeTime = ::GetTickCount();

	while( 1 )
	{
		long intervalTime = ::GetTickCount() - beforeTime;

		mNetRunningTime += intervalTime;
		beforeTime += intervalTime;

		try
		{
			if ( !updateSocket( mNetRunningTime ) )
				break;

			::Sleep(0);
		}
		catch( ComException& e )
		{
			Msg( e.what() );
		}
		catch( SocketException& e )
		{
			Msg( e.what() );
		}
		catch( BufferException& e )
		{
			Msg( e.what() );
		}
		catch( std::exception& e )
		{
			Msg( e.what() );
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
		ErrorMsg( e.what() );
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
				::Msg("Udp can't send full Data");
				mUdpSendBuffer.shiftUseSize( -numSend );
				break;
			}
		}
		catch ( BufferException& /*e*/ )
		{
			::Msg("Send Udp Com Error" );
			mUdpSendBuffer.clear();
			mUdpComList.clear();
			return;
		}
	}

	mUdpComList.erase( mUdpComList.begin() , iter );
	mUdpSendBuffer.clearUseData();
}

bool NetWorker::addUdpCom( IComPacket* cp , NetAddress const& addr )
{
	try
	{
		MUTEX_LOCK( mMutexUdpComList );
		size_t fillSize = ComEvaluator::FillBuffer( mUdpSendBuffer , cp );
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
	unsigned size;
	while( chain.readPacket( buffer , size ) )
	{	
		size_t oldSize = buffer.getAvailableSize();

		if ( oldSize < size )
			throw ComException( "error UDP Packet" );

		do
		{
			if ( !evaluator.evalCommand( buffer , group , userData ) )
			{
				::Msg( "readPacket Error Need Fix" );
				return false;
			}
		}
		while( oldSize - buffer.getAvailableSize() < size );

		if ( oldSize - buffer.getAvailableSize() != size )
			throw ComException( "error UDP Packet" );
	}

	return true;
}

unsigned FillBufferFromCom(NetBufferCtrl& bufferCtrl , IComPacket* cp)
{
	TLockedObject< SocketBuffer > buffer = bufferCtrl.lockBuffer();
	return FillBufferFromCom( *buffer , cp );
}

unsigned FillBufferFromCom( SocketBuffer& buffer , IComPacket* cp )
{
	assert( cp );

	bool done = false;
	int  count = 1;
	unsigned result;
	while ( !done )
	{
		try
		{
			result = ComEvaluator::FillBuffer( buffer , cp );
			done = true;
		}
		catch ( BufferException& e )
		{
			buffer.grow( ( buffer.getMaxSize() * 3 ) / 2 );
			Msg( e.what() );
		}
		catch ( ComException& e )
		{
			Msg( e.what() );
			return 0;
		}
		++count;
	}
	return result;
}
