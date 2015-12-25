#include "TinyGamePCH.h"
#include "GameClient.h"

#include "GameGlobal.h"
#include "GameNetPacket.h"


ClientWorker::ClientWorker( UserProfile& profile )
	:NetWorker()
	,mUserProfile( profile )
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
	mUdpClient.init();

#define COM_PACKET_SET( Class , Processer , Fun , Fun2 )\
	getEvaluator().setWorkerFun< Class >( Processer , Fun , Fun2 );

#define COM_THIS_PACKET_SET( Class , Fun )\
	COM_PACKET_SET( Class , this , &ClientWorker::##Fun , NULL )

#define COM_THIS_PACKET_SET2( Class , Fun , SocketFun )\
	COM_PACKET_SET( Class , this , &ClientWorker::##Fun , &ClientWorker::##SocketFun )

	COM_THIS_PACKET_SET ( SPConSetting , procConSetting )
	COM_THIS_PACKET_SET ( SPPlayerStatus , procPlayerStatus )
	COM_THIS_PACKET_SET ( CSPPlayerState , procPlayerState )
	COM_THIS_PACKET_SET2( CSPClockSynd  , procClockSynd  , procClockSyndNet )

#undef  COM_PACKET_SET
#undef  COM_THIS_PACKET_SET
#undef  COM_THIS_PACKET_SET2
	return true;
}

bool ClientWorker::updateSocket( long time )
{
	mTcpClient.updateSocket( time );
	mUdpClient.updateSocket( time );

	return true;
}

void ClientWorker::postChangeState( NetActionState oldState )
{
	if ( getActionState() != NAS_DISSCONNECT )
	{
		CSPPlayerState com;
		com.playerID = getPlayerManager()->getUserID();
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

void ClientWorker::onConnect( NetConnection* con )
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
		com.name = mUserProfile.name;
		
		sendTcpCommand( &com );

		changeState( NAS_LOGIN );

		if ( mClientListener )
			mClientListener->onServerEvent( ClientListener::eCON_RESULT , 1 );
	}
	catch ( std::exception& e )
	{
		e.what();
	}
}

void ClientWorker::onClose( NetConnection* con , ConCloseReason reason )
{
	assert( con == &mTcpClient );

	mSessoionId = 0;
	if ( mClientListener )
	{
		mClientListener->onServerEvent( ClientListener::eCON_CLOSE , reason );
	}
}

bool ClientWorker::onRecvData( NetConnection* con , SBuffer& buffer , NetAddress* clientAddr  )
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
		while( buffer.getAvailableSize() )
		{
			if ( !getEvaluator().evalCommand( buffer ) )
			{
				return false;
			}
		}
		return true;
	}
}

void ClientWorker::onConnectFailed( NetConnection* con )
{
	if ( mClientListener )
		mClientListener->onServerEvent( ClientListener::eCON_RESULT , 0 );
}

void ClientWorker::procClockSyndNet( IComPacket* cp )
{
	CSPClockSynd* com = cp->cast< CSPClockSynd >();

	switch( com->code )
	{
	case CSPClockSynd::eSTART:
		{
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

void ClientWorker::procClockSynd( IComPacket* cp )
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


void ClientWorker::procPlayerStatus( IComPacket* cp )
{
	SPPlayerStatus* com = cp->cast< SPPlayerStatus >();
	mPlayerManager->updatePlayer( com->info , com->numPlayer );
}

void ClientWorker::procPlayerState( IComPacket* cp )
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();

	if ( com->playerID == ERROR_PLAYER_ID )
	{
		mNextState = (NetActionState) com->state;
	}
	switch( com->state )
	{
	case NAS_ACCPET:
		getPlayerManager()->setUserID( com->playerID );
		if ( mClientListener )
			mClientListener->onServerEvent( ClientListener::eLOGIN_RESULT , 1 );
		break;
	case NAS_ROOM_ENTER:
		changeState( NAS_ROOM_WAIT );
		break;
	case NAS_DISSCONNECT:
		if ( com->playerID == getPlayerManager()->getUserID() || 
			 com->playerID == ERROR_PLAYER_ID )
		{
			closeNetwork();
		}
		break;
	case NAS_TIME_SYNC:
		break;
	case NAS_LEVEL_RUN:
		assert( com->playerID == ERROR_PLAYER_ID );
		changeState( NAS_LEVEL_RUN );
		break;
	}
}

void ClientWorker::procConSetting( IComPacket* cp )
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
	case CHANNEL_TCP_CONNECT:
		FillBufferByCom( mTcpClient.getSendCtrl() , cp );
		break;
	case CHANNEL_UDP_CHAIN:
		FillBufferByCom( mUdpClient.getSendCtrl() , cp );
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

void ClientWorker::onSendData( NetConnection* con )
{
	assert( con == &mUdpClient );
	sendUdpCom( mUdpClient.getSocket() );
}

void ClientWorker::doUpdate( long time )
{
	if ( getActionState() == NAS_DISSCONNECT )
	{
		mUdpClient.updateSocket( getNetRunningTime() );
	}
	BaseClass::doUpdate( time ); 
}

void ClientWorker::connect( char const* hostName )
{
	mTcpClient.connect( hostName , TG_TCP_PORT );
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



DelayClientWorker::DelayClientWorker( UserProfile& profile ) 
	:ClientWorker( profile )
	,mSDCTcp( mTcpClient.getSendCtrl() )
	,mSDCUdp( mUdpClient.getSendCtrl() )
	,mRDCTcp( 10240 )
	,mRDCUdp( 10240 )
	,mRDCUdpCL( 10240 )
{

}


void DelayClientWorker::sendCommand( int channel , IComPacket* cp , unsigned flag )
{
	switch( channel )
	{
	case CHANNEL_TCP_CONNECT:
		mSDCTcp.add( cp );
		break;
	case CHANNEL_UDP_CHAIN:
		mSDCUdp.add( cp );
		break;
	}	
}

bool DelayClientWorker::onRecvData( NetConnection* con , SBuffer& buffer , NetAddress* clientAddr )
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

bool DelayClientWorker::updateSocket( long time )
{
	mSDCTcp.update( time );
	mSDCUdp.update( time );

	if ( !BaseClass::updateSocket( time ) )
		return false;

	mRDCTcp.update( time , mUdpClient , getEvaluator() );
	mRDCUdp.update( time , mUdpClient , getEvaluator() );
	mRDCUdpCL.update( time , mUdpClient , getEvaluator() );

	return true;
}

void DelayClientWorker::setDelay( long time )
{
	mSDCTcp.setDelay( time );
	mSDCUdp.setDelay( time );
	mRDCTcp.setDelay( time );
	mRDCUdp.setDelay( time );
	mRDCUdpCL.setDelay( time );
}

SendDelayCtrl::SendDelayCtrl( NetBufferCtrl& bufferCtrl ) 
	:mBufferCtrl( bufferCtrl )
	,mBuffer( bufferCtrl.getBuffer().getMaxSize() )
{
	mDelay = 0;
	mCurTime = 0;
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
		mBuffer.clearUseData();
	}
}


bool SendDelayCtrl::add( IComPacket* cp )
{
	MUTEX_LOCK( mMutexBuffer );

	SendInfo info;

	info.size = FillBufferByCom( mBuffer , cp );

	if ( info.size == 0 )
		return false;

	info.time = mCurTime + mDelay;
	mInfoList.push_back( info );
	return true;
}

RecvDelayCtrl::RecvDelayCtrl( int size ) 
	:mBuffer( size )
	,mCurTime(0)
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
				EvalCommand( client , evaluator , mBuffer );
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
		mBuffer.clearUseData();
	}
}

bool RecvDelayCtrl::add( SBuffer& buffer , bool isUdpPacket )
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
			Msg( e.what() );
		}
		++count;
	}

	info.time  = mCurTime + mDelay;
	mInfoList.push_back( info );
	return true;
}
