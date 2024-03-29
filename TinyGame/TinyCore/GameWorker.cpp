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
		if( mComListener->prevProcCommand() )
		{
			mCPEvaluator.procCommand(*mComListener);
			mComListener->postProcCommand();
		}
	}
	else
	{
		mCPEvaluator.procCommand();
	}

	doUpdate( time );
}

char const* NetActionStateStrings[] =
{
	"NAS_DISSCONNECT" ,
	"NAS_LOGIN"      ,
	"NAS_ACCPET"      , //Server
	"NAS_CONNECT"     ,
	"NAS_RECONNECT"   ,


	//#TODO split
	"NAS_TIME_SYNC"   ,

	"NAS_ROOM_ENTER"  ,
	"NAS_ROOM_READY"  ,
	"NAS_ROOM_WAIT"   ,

	"NAS_LEVEL_SETUP"   ,
	"NAS_LEVEL_LOAD"    ,

	"NAS_LEVEL_INIT"    ,
	"NAS_LEVEL_RESTART" ,
	"NAS_LEVEL_RUN"     ,
	"NAS_LEVEL_PAUSE"   ,
	"NAS_LEVEL_EXIT"    ,

	"NAS_LEVEL_LOAD_FAIL",
};

void ComWorker::changeState( NetActionState state )
{
	if ( state == mNAState )
		return;

	LogMsg("Change Net State : %s", NetActionStateStrings[state]);
	NetActionState oldState = mNAState;
	mNAState = state;
	postChangeState( oldState );
}

NetWorker::NetWorker() 
	:mUdpSendBuffer( 1024 )
#if TINY_USE_NET_THREAD
	,mSocketThread( SocketFunc( this , &NetWorker::entryNetThread ) )
#endif
{
	mNetListener = NULL;
}

NetWorker::~NetWorker()
{
	{
		NET_MUTEX_LOCK( mMutexUdpCmdList );
		mUdpCmdList.clear();
	}
#if TINY_USE_NET_THREAD
	mSocketThread.stop();
#endif

	NetSocket::ShutdownSystem();
}

#if TINY_USE_NET_THREAD
uint32 GNetThreadId = 0;
bool IsInNetThread()
{
	return GNetThreadId == PlatformThread::GetCurrentThreadId();
}
#endif


#if TINY_USE_NET_THREAD
void NetWorker::entryNetThread()
{
	GNetThreadId = PlatformThread::GetCurrentThreadId();

	SystemPlatform::AtomExchange(&mbRequestExitNetThread, 0);

	mNetRunningTime = 0;
	int64 beforeTime = SystemPlatform::GetTickCount();

	while( mbRequestExitNetThread == 0 )
	{
		int64 intervalTime = SystemPlatform::GetTickCount() - beforeTime;

		mNetRunningTime += intervalTime;
		beforeTime += intervalTime;

		try
		{
			if ( !update_NetThread( mNetRunningTime ) )
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

	clenupNetResource();
}
#endif

bool NetWorker::startNetwork()
{
	try 
	{
		if ( !NetSocket::StartupSystem() )
			return false;

		if ( !doStartNetwork() )
			return false;
#if TINY_USE_NET_THREAD
		if ( !mSocketThread.start() )
			return false;
#endif
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
	{
		NET_MUTEX_LOCK(mMutexUdpCmdList);
		mUdpCmdList.clear();
	}

#if TINY_USE_NET_THREAD
	SystemPlatform::AtomExchange(&mbRequestExitNetThread, 1);
	mSocketThread.join();
#endif

	doCloseNetwork();

	NetSocket::ShutdownSystem();
}

void NetWorker::sendUdpCmd( NetSocket& socket )
{
	NET_MUTEX_LOCK( mMutexUdpCmdList );
	if (mUdpCmdList.empty())
		return;
	
	UdpCmdList::iterator iter = mUdpCmdList.begin();
	for( ; iter != mUdpCmdList.end() ; ++iter )
	{
		UdpCmd& uc = *iter;
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
			mUdpCmdList.clear();
			return;
		}
	}
	
	mUdpCmdList.erase(mUdpCmdList.begin(), iter);
	mUdpSendBuffer.removeUsedData();
}

bool NetWorker::addUdpCom( IComPacket* cp , NetAddress const& addr )
{
	try
	{
		NET_MUTEX_LOCK( mMutexUdpCmdList );
		size_t fillSize = ComEvaluator::WriteBuffer( mUdpSendBuffer , cp );
		UdpCmd uc;
		uc.addr     = addr;
		uc.dataSize = fillSize;
		mUdpCmdList.push_back( uc );
	}
	catch ( ... )
	{
		return false;
	}

	return true;
}



bool FNetCommand::Eval( UdpChain& chain , ComEvaluator& evaluator , SocketBuffer& buffer , int group , void* userData )
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
				LogMsg( "readPacket Error Need Fix" );
				return false;
			}
		}
		while( oldSize - buffer.getAvailableSize() < readSize );

		if ( oldSize - buffer.getAvailableSize() != readSize )
			throw ComException( "error UDP Packet" );
	}

	return true;
}

unsigned FNetCommand::Write(NetBufferOperator& bufferCtrl , IComPacket* cp)
{
	TLockedObject< SocketBuffer > buffer = bufferCtrl.lockBuffer();
	return Write( *buffer , cp );
}

unsigned FNetCommand::Write( SocketBuffer& buffer , IComPacket* cp )
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
			LogMsg( e.what() );
		}
		catch ( ComException& e )
		{
			LogMsg( e.what() );
			return 0;
		}
		++count;
	}
	return result;
}
