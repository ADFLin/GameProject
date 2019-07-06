#include "TinyGamePCH.h"
#include "GameClient.h"

#include "GameGlobal.h"
#include "GameNetPacket.h"


ClientWorker::ClientWorker()
	:NetWorker()
	,mCalculator( 300 )
	,mClientListener(NULL)
{
	mSessoionId = 0;
	mUdpClient.setListener( this );
	mTcpClient.setListener( this );
	mPlayerManager.reset( new CLPlayerManager );
}

ClientWorker::~ClientWorker()
{

}

bool ClientWorker::doStartNetwork()
{
	mUdpClient.initialize();
	mNetSelect.addSocket(mUdpClient.getSocket());

#define COM_PACKET_SET( Class , Processer , Fun , Fun2 )\
	getEvaluator().setWorkerFun< Class >( Processer , Fun , Fun2 );

#define COM_THIS_PACKET_SET( Class , Fun )\
	COM_PACKET_SET( Class , this , &ClientWorker::Fun , NULL )

#define COM_THIS_PACKET_SET_2( Class , Fun , SocketFun )\
	COM_PACKET_SET( Class , this , &ClientWorker::Fun , &ClientWorker::SocketFun )

	COM_THIS_PACKET_SET ( SPConSetting , procConSetting )
	COM_THIS_PACKET_SET ( SPPlayerStatus , procPlayerStatus )
	COM_THIS_PACKET_SET ( CSPPlayerState , procPlayerState )
	COM_THIS_PACKET_SET_2( CSPClockSynd  , procClockSynd  , procClockSynd_NetThread )

#undef  COM_PACKET_SET
#undef  COM_THIS_PACKET_SET
#undef  COM_THIS_PACKET_SET_2
	return true;
}

void ClientWorker::doUpdate( long time )
{
	BaseClass::doUpdate( time ); 
	
	if( getActionState() == NAS_DISSCONNECT )
	{
		mUdpClient.updateSocket(getNetRunningTime());
	}
}

bool ClientWorker::update_NetThread( long time )
{
	if( !BaseClass::update_NetThread(time) )
		return false;
#if 0
	mNetSelect.clear();
	if ( mTcpClient.getSocket().getState() != SKS_CLOSE )
		mNetSelect.addSocket(mTcpClient.getSocket());
	mNetSelect.addSocket(mUdpClient.getSocket());
#endif

	if (mNetSelect.select(0, 0))
	{
		mTcpClient.updateSocket(time, &mNetSelect);
		mUdpClient.updateSocket(time, &mNetSelect);
	}

	return true;
}

void ClientWorker::postChangeState( NetActionState oldState )
{
	if ( getActionState() != NAS_DISSCONNECT )
	{
		CSPPlayerState com;
		com.playerId = getPlayerManager()->getUserID();
		com.state    = getActionState();
		sendTcpCommand( &com );
	}

	switch( getActionState() )
	{
	case NAS_ROOM_WAIT:
		break;
	}

	if ( mNetListener )
		mNetListener->onChangeActionState( getActionState() );
}

void ClientWorker::notifyConnectionOpen( NetConnection* con )
{
	assert( con == &mTcpClient );
	try 
	{
		NetAddress addr;
		addr.setFromSocket( mTcpClient.getSocket() );
		mUdpClient.setServerAddr( addr.getIP() , TG_UDP_PORT );
		mTcpClient.setListener( this );

		CPLogin com;
		com.id   = mSessoionId;
		com.name = mLoginName;
		sendTcpCommand( &com );

		addGameThreadCommnad([this]
		{
			changeState(NAS_LOGIN);
			if( mClientListener )
			{
				mClientListener->onServerEvent(ClientListener::eCON_RESULT, 1);
			}
		});
			
	}
	catch ( std::exception& e )
	{
		e.what();
	}
}

void ClientWorker::notifyConnectClose( NetConnection* con , NetCloseReason reason )
{
	assert( con == &mTcpClient );

	mSessoionId = 0;
	addGameThreadCommnad([this,reason]
	{	
		if( mClientListener )
		{
			mClientListener->onServerEvent(ClientListener::eCON_CLOSE, (unsigned)reason);
		}
	});
}

bool ClientWorker::notifyConnectionRecv( NetConnection* con , SocketBuffer& buffer , NetAddress* clientAddr  )
{
	if ( clientAddr )
	{
		assert(  con == &mUdpClient );
		if (  mUdpClient.getServerAddress() != *clientAddr  )
		{
			while( buffer.getAvailableSize() )
			{
				if ( !getEvaluator().evalCommand( buffer ) )
				{
					return false;
				}
			}
			return true;
		}
		else
		{
			return EvalCommand( mUdpClient , getEvaluator() , buffer );
		}
	}
	else
	{
		if (con == &mUdpClient)
		{
			return EvalCommand(mUdpClient, getEvaluator(), buffer);
		}
		else
		{
			while (buffer.getAvailableSize())
			{
				if (!getEvaluator().evalCommand(buffer))
				{
					return false;
				}
			}
			return true;
		}

	}
}

void ClientWorker::notifyConnectionFail( NetConnection* con )
{
	addGameThreadCommnad([this]
	{
		if( mClientListener )
			mClientListener->onServerEvent(ClientListener::eCON_RESULT, 0);
	});
}

void ClientWorker::procClockSynd_NetThread( IComPacket* cp)
{
	assert(IsInNetThread());

	CSPClockSynd* com = cp->cast< CSPClockSynd >();

	switch( com->code )
	{
	case CSPClockSynd::eSTART:
		{
			LogMsg("ClockSynd Start");
			mCalculator.clear();
			mNumSampleTest = com->numSample;
			mCalculator.markRequest();

			CSPClockSynd sendCom;
			sendCom.code    = CSPClockSynd::eREQUEST;
			sendTcpCommand( &sendCom );
		}
		break;
	case CSPClockSynd::eREPLY:
		{
			mCalculator.markReply();
			LogMsg("ClockSynd Reply %d", mCalculator.getSampleNum());
			CSPClockSynd sendCom;
			if ( mCalculator.getSampleNum() > mNumSampleTest )
			{
				mNetLatency = mCalculator.calcResult();

				sendCom.code    = CSPClockSynd::eDONE;
				sendCom.latency = mNetLatency;
				sendTcpCommand( &sendCom );
			}
			else
			{
				sendCom.code    = CSPClockSynd::eREQUEST;
				sendTcpCommand( &sendCom );
				mCalculator.markRequest();
			}
		}
		break;
	}
}

void ClientWorker::procClockSynd( IComPacket* cp)
{
	CSPClockSynd* com = cp->cast< CSPClockSynd >();

	switch( com->code )
	{
	case CSPClockSynd::eSTART:
		break;
	case CSPClockSynd::eREPLY:
		if ( mCalculator.getSampleNum() > mNumSampleTest &&
			 mNextState == NAS_TIME_SYNC )
		{
			changeState( NAS_TIME_SYNC );
		}
		break;
	}
}


void ClientWorker::procPlayerStatus( IComPacket* cp)
{
	SPPlayerStatus* com = cp->cast< SPPlayerStatus >();
	mPlayerManager->updatePlayer( com->info , com->numPlayer );
}

void ClientWorker::procPlayerState( IComPacket* cp)
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();

	if ( com->playerId == SERVER_PLAYER_ID )
	{
		mNextState = (NetActionState) com->state;
	}
	switch( com->state )
	{
	case NAS_ACCPET:
		getPlayerManager()->setUserID( com->playerId );
		if ( mClientListener )
			mClientListener->onServerEvent( ClientListener::eLOGIN_RESULT , 1 );
		break;
	case NAS_ROOM_ENTER:
		changeState( NAS_ROOM_WAIT );
		break;
	case NAS_DISSCONNECT:
		if ( com->playerId == getPlayerManager()->getUserID() || 
			 com->playerId == SERVER_PLAYER_ID )
		{
			if ( mClientListener )
				mClientListener->onServerEvent(ClientListener::eCON_CLOSE, (unsigned)NetCloseReason::ShutDown);
			closeNetwork();
		}
		break;
	case NAS_TIME_SYNC:
		break;
	case NAS_LEVEL_RUN:
		assert( com->playerId == SERVER_PLAYER_ID );
		changeState( NAS_LEVEL_RUN );
		break;
	}
}

void ClientWorker::procConSetting( IComPacket* cp)
{
	SPConSetting* com = cp->cast< SPConSetting >();
	if ( com->result == SPConSetting::eNEW_CON )
	{
		mSessoionId = com->id;
	}

	CPUdpCon udpCom;
	udpCom.id = mSessoionId;
	addUdpCom( &udpCom , mUdpClient.getServerAddress() );
}

void ClientWorker::doCloseNetwork()
{
	mTcpClient.close();
	mUdpClient.close();
}


void ClientWorker::sendCommand( int channel , IComPacket* cp , unsigned flag )
{
	switch( channel )
	{
	case CHANNEL_GAME_NET_TCP:
		WriteComToBuffer( mTcpClient.getSendCtrl() , cp );
		break;
	case CHANNEL_GAME_NET_UDP_CHAIN:
		WriteComToBuffer( mUdpClient.getSendCtrl() , cp );
		break;
	}	
}


void ClientWorker::sreachLanServer()
{
	CSPComMsg com;
	com.str =  "server_info";

	NetAddress addr;
	addr.setBroadcast( TG_UDP_PORT );
	addUdpCom( &com , addr );
}

void ClientWorker::notifyConnectionSend( NetConnection* con )
{
	assert( con == &mUdpClient );
	sendUdpCom( mUdpClient.getSocket() );
}

void ClientWorker::connect( char const* hostName , char const* loginName )
{
	if( loginName )
		mLoginName = loginName;
	else
		mLoginName = "Player";
	mTcpClient.connect( hostName , TG_TCP_PORT );
	mNetSelect.addSocket( mTcpClient.getSocket() );
}

void CLPlayerManager::updatePlayer( PlayerInfo* info[] , int num )
{
	unsigned bit = 0;

	for ( int i = 0 ; i < num ; ++i )
	{
		bit |= BIT( info[i]->playerId );

		CLocalPlayer* player = getPlayer( info[i]->playerId );

		if ( player == NULL )
			player = createPlayer( info[i]->playerId );

		player->setInfo( *info[i] );
	}

	for( PlayerTable::iterator iter = mPlayerTable.begin() ;
		iter != mPlayerTable.end() ; )
	{
		CLocalPlayer* player = iter->second;

		if ( bit & BIT(player->getId()) )
		{
			++iter;
		}
		else
		{
			delete player;
			iter = mPlayerTable.erase(  iter );
		}
	}
}



DelayClientWorker::DelayClientWorker() 
	:ClientWorker()
	,mSDCTcp( mTcpClient.getSendCtrl() )
	,mSDCUdp( mUdpClient.getSendCtrl() )
	,mRDCTcp( 10240 )
	,mRDCUdp( 10240 )
	,mRDCUdpCL( 10240 )
{

}


DelayClientWorker::~DelayClientWorker()
{

}

void DelayClientWorker::sendCommand( int channel , IComPacket* cp , unsigned flag )
{
	switch( channel )
	{
	case CHANNEL_GAME_NET_TCP:
		mSDCTcp.add( cp );
		break;
	case CHANNEL_GAME_NET_UDP_CHAIN:
		mSDCUdp.add( cp );
		break;
	}	
}

bool DelayClientWorker::notifyConnectionRecv( NetConnection* con , SocketBuffer& buffer , NetAddress* clientAddr )
{
	if ( clientAddr )
	{
		if ( mUdpClient.getServerAddress() == *clientAddr )
			mRDCUdpCL.add( buffer , true );
		else
			mRDCUdp.add( buffer , true );
	}
	else
	{
		mRDCTcp.add( buffer , false );
	}
	return true;
}

bool DelayClientWorker::update_NetThread( long time )
{
	mSDCTcp.update( time );
	mSDCUdp.update( time );

	if ( !BaseClass::update_NetThread( time ) )
		return false;

	mRDCTcp.update( time , mUdpClient , getEvaluator() );
	mRDCUdp.update( time , mUdpClient , getEvaluator() );
	mRDCUdpCL.update( time , mUdpClient , getEvaluator() );

	return true;
}

void DelayClientWorker::setDelay( long delay , long delayRand )
{
	mSDCTcp.setDelay( delay , delayRand );
	mSDCUdp.setDelay( delay, delayRand );
	mRDCTcp.setDelay(delay, delayRand);
	mRDCUdp.setDelay(delay, delayRand);
	mRDCUdpCL.setDelay(delay, delayRand);
}

DelayCtrlBase::DelayCtrlBase() 
	:mDelay(0)
	,mDelayRand(0)
	,mCurTime(0)
{

}

long DelayCtrlBase::makeDelayValue()
{
	return mDelay + ::Global::Random() % mDelayRand;
}

SendDelayCtrl::SendDelayCtrl( NetBufferOperator& bufferCtrl ) 
	:mBufferCtrl( bufferCtrl )
	,mBuffer( bufferCtrl.getBuffer().getMaxSize() )
{
}

void SendDelayCtrl::update( long time )
{
	MUTEX_LOCK( mMutexBuffer );

	mCurTime = time;
	
	unsigned size = 0;
	SendInfoList::iterator iter = mInfoList.begin();
	for( ; iter != mInfoList.end() ; ++iter )
	{
		if ( iter->time > time )
			break;
		size += iter->size;
	}

	if ( size )
	{
		mBufferCtrl.fillBuffer( mBuffer , size );
		mInfoList.erase( mInfoList.begin() , iter );
		mBuffer.removeUseData();
	}
}


bool SendDelayCtrl::add( IComPacket* cp )
{
	MUTEX_LOCK( mMutexBuffer );

	SendInfo info;

	info.size = WriteComToBuffer( mBuffer , cp );

	if ( info.size == 0 )
		return false;

	info.time = mCurTime + makeDelayValue();
	mInfoList.push_back( info );
	return true;
}

RecvDelayCtrl::RecvDelayCtrl( int size ) 
	:mBuffer( size )
{

}

void RecvDelayCtrl::update( long time , UdpClient& client , ComEvaluator& evaluator )
{
	mCurTime = time;

	RecvInfoList::iterator iter = mInfoList.begin();

	size_t size = 0;
	size_t fillSize = mBuffer.getFillSize();

	for( ; iter != mInfoList.end() ; ++iter )
	{
		if ( iter->time > time )
			break;

		size += iter->size;

		mBuffer.setFillSize( size );
		try
		{
			if ( iter->isUdpPacket )
			{
				EvalCommand( client , evaluator , mBuffer , -1 );
			}
			else
			{
				while ( mBuffer.getAvailableSize() )
				{
					if ( !evaluator.evalCommand( mBuffer , NULL ) )
					{
						break;
					}
				}
			}
		}
		catch ( ComException& /*e*/ )
		{
			mBuffer.setUseSize( size );
		}
	}

	mBuffer.setFillSize( fillSize );

	if ( size )
	{
		mInfoList.erase( mInfoList.begin() , iter );
		mBuffer.removeUseData();
	}
}

bool RecvDelayCtrl::add( SocketBuffer& buffer , bool isUdpPacket )
{
	RecvInfo info;

	info.size        = (unsigned)buffer.getAvailableSize();
	info.isUdpPacket = isUdpPacket;

	bool done = false;
	int  count = 1;
	while ( !done )
	{
		try
		{
			mBuffer.fill( buffer , buffer.getAvailableSize() );
			done = true;
		}
		catch ( BufferException& e )
		{
			mBuffer.grow( ( mBuffer.getMaxSize() * 3 ) / 2 );
			LogWarning(  0 , e.what() );
		}
		++count;
	}

	info.time  = mCurTime + makeDelayValue();
	mInfoList.push_back( info );
	return true;
}
