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
{
	mSocketThread.init( this , &NetWorker::procSocketThread );
	mNetListener = NULL;
}


NetWorker::~NetWorker()
{
	{
		MUTEX_LOCK( mMutexUdpComList );
		mUdpComList.clear();
	}
	mSocketThread.kill();
	TSocket::exitSystem();
}

unsigned NetWorker::procSocketThread()
{
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

	return 0;
}

bool NetWorker::startNetwork()
{
	try 
	{
		if ( !TSocket::initSystem() )
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

	mSocketThread.kill();
	doCloseNetwork();

	TSocket::exitSystem();
}

void NetWorker::sendUdpCom( TSocket& socket )
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
		size_t fSize = ComEvaluator::fillBuffer( cp , mUdpSendBuffer );

		{
			MUTEX_LOCK( mMutexUdpComList );
			UdpCom uc;
			uc.addr     = addr;
			uc.dataSize = fSize;
			mUdpComList.push_back( uc );
		}

	}
	catch ( ... )
	{
		return false;
	}

	return true;
}

