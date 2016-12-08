#include "TinyGamePCH.h"

#include "GameServer.h"
#include "GameNetPacket.h"

int const CLIENT_GROUP = 1;

ServerWorker::ServerWorker()
	:NetWorker()
	,mEventResolver( nullptr )
{
	mPlayerManager.reset( new SVPlayerManager );

	mTcpServer.setListener( this );
	mUdpServer.setListener( this );

	//LocalPlayer::sProcesser = this;
}

ServerWorker::~ServerWorker()
{
	

}


LocalWorker* ServerWorker::createLocalWorker( char const* userName )
{
	if ( mLocalWorker == NULL )
	{
		mLocalWorker.reset( new LocalWorker( this ) );
	}
	getPlayerManager()->createUserPlayer( mLocalWorker , userName );
	return mLocalWorker;
}

void ServerWorker::sendClientTcpCommand( ClientInfo& client , IComPacket* cp )
{
	FillBufferFromCom( client.tcpClient.getSendCtrl() , cp );
}

bool ServerWorker::doStartNetwork()
{
	try 
	{
		mTcpServer.run( TG_TCP_PORT );
		mUdpServer.run( TG_UDP_PORT );
	}
	catch ( ... )
	{
		return false;
	}


	typedef ServerWorker ThisClass;

#define COM_PACKET_SET( Class , Processer , Fun , Fun2 )\
	getEvaluator().setWorkerFun< Class >( Processer , Fun , Fun2 );

#define COM_THIS_PACKET_SET( Class , Fun )\
	COM_PACKET_SET( Class , this , &ThisClass::##Fun , NULL )


#define COM_THIS_PACKET_SET_2( Class , Fun , SocketFun )\
	COM_PACKET_SET( Class , this , &ThisClass::##Fun , &ThisClass::##SocketFun )

	COM_THIS_PACKET_SET( CPLogin       , procLogin )
	COM_THIS_PACKET_SET( CPEcho        , procEcho )
	COM_THIS_PACKET_SET( CSPMsg        , procMsg )

	COM_THIS_PACKET_SET_2( CPUdpCon       , procUdpCon , procUdpConNet )
	COM_THIS_PACKET_SET_2( CSPClockSynd   , procClockSynd , procClockSyndNet )
	COM_THIS_PACKET_SET_2( CSPComMsg      , procComMsg    , procComMsgNet )

	COM_THIS_PACKET_SET ( CSPPlayerState , procPlayerState )

	
#undef  COM_PACKET_SET
#undef  COM_THIS_PACKET_SET
#undef  COM_THIS_PACKET_SET_2

	changeState( NAS_CONNECT );
	return true;
}


void ServerWorker::doCloseNetwork()
{
	mClientManager.removeAllClient();
	mPlayerManager->cleanup();
	mTcpServer.close();
	mUdpServer.close();
}

void ServerWorker::postChangeState( NetActionState oldState )
{
	if ( oldState == NAS_DISSCONNECT )
		return;

	switch( getActionState() )
	{
	case NAS_ROOM_ENTER:
		{
			mPlayerManager->removePlayerFlag( ServerPlayer::eReady );
			//mPlayerManager->removePlayerFlag( ServerPlayer::eReady );
		}
		break;
	case NAS_TIME_SYNC:
		{
			mPlayerManager->removePlayerFlag( ServerPlayer::eSyndDone );

			CSPClockSynd com;
			com.code = CSPClockSynd::eSTART;
			com.numSample = 20;
			sendTcpCommand( &com );
		}
		break;
	case NAS_LEVEL_SETUP:
		{
			mPlayerManager->removePlayerFlag( ServerPlayer::eLevelSetup );
		}
		break;
	case NAS_LEVEL_RESTART:
		{
			mPlayerManager->removePlayerFlag( ServerPlayer::eLevelReady );
		}
		break;
	case NAS_DISSCONNECT:
		{
			mPlayerManager->removePlayer( mPlayerManager->getUserID() );
		}
		break;
	}

	CSPPlayerState com;
	com.setServerState(getActionState());
	sendTcpCommand( &com );

	if ( mNetListener )
		mNetListener->onChangeActionState( getActionState() );
}


bool ServerWorker::onRecvData( NetConnection* connection , SocketBuffer& buffer , NetAddress* clientAddr )
{
	if( clientAddr )
	{
		assert(  connection == &mUdpServer );
		ClientInfo* info = mClientManager.findClient( *clientAddr );

		bool result = true;

		//maybe broadcast
		if( info == NULL )
		{
			::Msg("ip = %lu , port = %u", clientAddr->get().sin_addr.s_addr, clientAddr->get().sin_port);

			while( buffer.getAvailableSize() )
			{
				if( !getEvaluator().evalCommand(buffer, -1, clientAddr) )
				{
					result = false;
					break;
				}
			}
		}
		else
		{
			result = EvalCommand(info->udpClient, getEvaluator(), buffer, CLIENT_GROUP , info);
		}

		return result;
	}
	else
	{
		if ( connection == &mTcpServer )
			return true;

		ClientInfo::TCPClient* tcpClient = static_cast< ClientInfo::TCPClient*>( connection );
		ClientInfo* info  = tcpClient->clientInfo;

		while( buffer.getAvailableSize() )
		{		
			if ( !getEvaluator().evalCommand( buffer , CLIENT_GROUP, info ) )
			{
				return false;
			}
		}
	}
	return true;
}


bool ServerWorker::updateSocket( long time )
{
	mTcpServer.updateSocket( time );
	mUdpServer.updateSocket( time );
	mClientManager.updateNet( time );

	return true;
}

void ServerWorker::onSendData( NetConnection* con )
{
	assert( con == &mUdpServer );
	sendUdpCom( mUdpServer.getSocket() );
	mClientManager.sendUdpData( getNetRunningTime() , mUdpServer );
}


void ServerWorker::onConnectAccpet( NetConnection* con )
{
	assert( con == &mTcpServer );

	NetSocket conSocket;
	NetAddress hostAddr;
	if ( !mTcpServer.getSocket().accept( conSocket , hostAddr ) )
		return;

	::Msg( "Accpet ip = %lu port = %u" , hostAddr.get().sin_addr.s_addr , hostAddr.get().sin_port );

	ClientInfo* info = mClientManager.findClient(hostAddr);
	if( info )
	{
		info->reconnect(conSocket);
		if( info->player )
		{
			if( mEventResolver )
				mEventResolver->resolvePlayerReconnect(info->player->getId());

			CSPPlayerState com;
			com.playerID = info->player->getId();
			com.state = NAS_RECONNECT;
			sendTcpCommand(&com);
		}
	}
	else
	{
		ClientInfo* client = mClientManager.createClient(conSocket);
		if( client )
		{
			client->tcpClient.setListener(this);

			SPConSetting com;
			com.result = SPConSetting::eNEW_CON;
			com.id = client->id;
			sendClientTcpCommand(*client, &com);
		}
		else
		{



		}
	}
}

void ServerWorker::onConnectClose( NetConnection* con , ConCloseReason reason )
{
	ClientInfo* clientInfo = static_cast< ClientInfo::TCPClient* >( con )->clientInfo;
	SNetPlayer* netPlayer = clientInfo->player; 

	PlayerDisconnectMode mode = PlayerDisconnectMode::Remove;

	if( mEventResolver && netPlayer )
		mode = mEventResolver->resolvePlayerClose( netPlayer->getId() , reason);

	switch( mode )
	{
	case PlayerDisconnectMode::Remove:
		removeConnect(clientInfo, true);
		break;
	case PlayerDisconnectMode::ChangeToLocal:
		{
			unsigned id = netPlayer->getId();
			removeConnect(clientInfo, false);
			{
				MUTEX_LOCK(mMutexPlayerChangeInfoVec);
				PlayerChangeInfo changeInfo;
				changeInfo.changeType = PlayerChangeInfo::eSweepToLocal;
				changeInfo.player = netPlayer;
				mPlayerChangeInfoVec.push_back(changeInfo);
			}
		}
		break;
	case PlayerDisconnectMode::WaitReconnect:
		{
			netPlayer->getStateFlag().add(ServerPlayer::eDissconnect);
			CSPPlayerState com;
			com.playerID = netPlayer->getId();
			com.state = NAS_DISSCONNECT;
			sendTcpCommand(&com);
		}
		break;
	}

	SPPlayerStatus infoCom;
	getPlayerManager()->getPlayerInfo( infoCom.info );
	infoCom.numPlayer = (uint8) getPlayerManager()->getPlayerNum();
	sendTcpCommand( &infoCom );
}

void ServerWorker::procUdpConNet( IComPacket* cp )
{
	assert(IsInSocketThread());

	if ( cp->getUserData() && cp->getGroup() == -1 )
	{
		NetAddress* sendAddr = (NetAddress*)cp->getUserData();
		CPUdpCon* com = cp->cast< CPUdpCon >();
		mClientManager.setClientUdpAddr(com->id, *sendAddr);
	}
	//::Msg("procUdpConNet");
}

void ServerWorker::procUdpCon( IComPacket* cp)
{

}

void ServerWorker::procLogin( IComPacket* cp)
{
	CPLogin* com = cp->cast< CPLogin >();
	ClientInfo* info = static_cast< ClientInfo*>( cp->getUserData() );

	if ( !info )
	{
		return;
	}

	SNetPlayer* player = getPlayerManager()->createNetPlayer( this , com->name , info );

	if ( player == NULL )
	{
		mClientManager.removeClient( info );
		return;
	}

	{
		//SPPlayerStatus PSCom;
		//generatePlayerStatus( PSCom );
		//sendTcpCommand( &PSCom );

		CSPPlayerState stateCom;
		stateCom.playerID = player->getId();
		stateCom.state    = NAS_ACCPET;
		player->sendTcpCommand( &stateCom );

		stateCom.playerID = player->getId();
		stateCom.state    = NAS_CONNECT;
		sendTcpCommand( &stateCom );
	}

}


void ServerWorker::procEcho( IComPacket* cp)
{
	CPEcho* com = cp->cast< CPEcho >();
	ClientInfo* info = static_cast< ClientInfo*>(cp->getUserData());
	assert( info );
	FillBufferFromCom( info->udpClient.getSendCtrl() , cp );
}


void ServerWorker::procMsg( IComPacket* cp)
{
	sendTcpCommand( cp );
}


void ServerWorker::procClockSyndNet( IComPacket* cp )
{
	ClientInfo* info = static_cast< ClientInfo*>( cp->getUserData() );
	if ( info == nullptr )
		return;

	CSPClockSynd* com = cp->cast< CSPClockSynd >();
	SNetPlayer*   player = info->player;

	switch( com->code )
	{
	case CSPClockSynd::eREQUEST:
		{
			CSPClockSynd packet;
			packet.code = CSPClockSynd::eREPLY;
			player->sendTcpCommand( &packet );
		}
		break;
	case CSPClockSynd::eDONE:
		break;
	}
}

void ServerWorker::procClockSynd( IComPacket* cp )
{
	CSPClockSynd* com = cp->cast< CSPClockSynd >();

	ServerPlayer* player;
	if ( cp->getUserData() )
	{
		ClientInfo* info = static_cast< ClientInfo*>(cp->getUserData());
		player = info->player;
	}
	else
	{
		unsigned id = getPlayerManager()->getUserID();
		player = getPlayerManager()->getPlayer( id );
	}

	switch( com->code )
	{
	case CSPClockSynd::eREQUEST:
		{

		}
		break;
	case CSPClockSynd::eDONE:
		{
			Msg( "Player %d synd done , ping = %u" , player->getId() , com->latency );
			player->getStateFlag().add( ServerPlayer::eSyndDone );
			player->latency = com->latency;
		}
		break;
	}
}

void ServerWorker::procPlayerState( IComPacket* cp )
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();

	ClientInfo* info = static_cast< ClientInfo*>(cp->getUserData());

	if ( com->playerID == ERROR_PLAYER_ID )
	{
		return;
	}

	ServerPlayer* player = getPlayerManager()->getPlayer( com->playerID );
	
	if ( !player )
		return;

	switch( com->state )
	{
	case NAS_ROOM_ENTER:
		{
			//SPPlayerStatus PSCom;
			//generatePlayerStatus( PSCom );
			//player->sendTcpCommand( &PSCom );
		}
		break;
	case NAS_ROOM_WAIT:
		{
			DevMsg( 5, "Plyer %d not Ready" , player->getId() );
			player->getStateFlag().remove( ServerPlayer::eReady );
			sendTcpCommand( cp );
		}
		break;
	case NAS_DISSCONNECT:
		{
			player->getStateFlag().add( ServerPlayer::eDissconnect );
			sendTcpCommand( cp );
		}
		break;
	case NAS_ROOM_READY:
		{
			DevMsg( 5, "Plyer %d Ready" , player->getId() );
			player->getStateFlag().add( ServerPlayer::eReady );
			sendTcpCommand( cp );
		}
		break;
	case NAS_LEVEL_SETUP:
		{
			player->getStateFlag().add( ServerPlayer::eLevelSetup );
			if ( mPlayerManager->checkPlayerFlag( ServerPlayer::eLevelSetup , true ) )
			{
				changeState( NAS_LEVEL_LOAD );
			}
		}
		break;
	case NAS_LEVEL_LOAD:
		{
			player->getStateFlag().add( ServerPlayer::eLevelLoaded );
			if ( mPlayerManager->checkPlayerFlag( ServerPlayer::eLevelLoaded , true ) )
			{
				changeState( NAS_LEVEL_INIT );
			}
		}
		break;
	case NAS_LEVEL_RESTART:
	case NAS_LEVEL_INIT:
		{
			player->getStateFlag().add( ServerPlayer::eLevelReady );
			if ( mPlayerManager->checkPlayerFlag( ServerPlayer::eLevelReady , true ) )
			{
				changeState( NAS_TIME_SYNC );
			}
		}
		break;
	case NAS_TIME_SYNC:
		{
			if ( mPlayerManager->checkPlayerFlag( ServerPlayer::eSyndDone , true ) )
			{
				changeState( NAS_LEVEL_RUN );
			}
		}
		break;
	case NAS_LEVEL_PAUSE:
		if ( getActionState() == NAS_LEVEL_RUN )
		{
			player->getStateFlag().add( ServerPlayer::ePause );
			changeState( NAS_LEVEL_PAUSE );
			sendTcpCommand( cp );
		}
		break;
	case NAS_LEVEL_RUN:
		if ( getActionState() == NAS_LEVEL_PAUSE )
		{
			if ( player->getId() == mPlayerManager->getUserID() )
			{
				mPlayerManager->removePlayerFlag( ServerPlayer::ePause );
				changeState( NAS_LEVEL_RUN );
			}
			else
			{
				player->getStateFlag().remove( ServerPlayer::ePause );
				if ( !mPlayerManager->checkPlayerFlag( ServerPlayer::ePause , true ) )
					changeState( NAS_LEVEL_RUN );
			}
		}
		break;
	}
}

void ServerWorker::procComMsgNet( IComPacket* cp)
{
	if( cp->getUserData() == nullptr )
		return;

	CSPComMsg* com = cp->cast< CSPComMsg >();

	if ( com->str == "server_info" )
	{
		FixString< 256 > hostname;
		
		NetAddress* sendAddr = (NetAddress*)com->getUserData();
		if( cp->getGroup() == CLIENT_GROUP )
		{
			sendAddr = &static_cast<ClientInfo*>(cp->getUserData())->udpAddr;
		}
		else
		{
			sendAddr = (NetAddress*)com->getUserData();
		}

		if( gethostname(hostname, 256) == 0 )
		{
			//TODO
			TLockedObject< SVPlayerManager > playerManager = mPlayerManager->Lock();
			SPServerInfo info;
			ServerPlayer* player = playerManager->getPlayer(playerManager->getUserID());
			if( player )
			{
				info.name.format("%s's Game", player->getName());
			}
			else
			{
				info.name = "server";
			}

			hostent* hn = gethostbyname(hostname);
			info.ip = inet_ntoa(*(struct in_addr *)hn->h_addr_list[0]);
			addUdpCom(&info, *sendAddr);
		}
	}
}

void ServerWorker::procComMsg( IComPacket* cp)
{

}


void ServerWorker::procUdpAddress( IComPacket* cp)
{
	

}

void ServerWorker::removeConnect( ClientInfo* info , bool bRMPlayer )
{
	if ( info->player )
	{
		if ( !info->player->getStateFlag().check( ServerPlayer::eDissconnect ) )
		{
			CSPPlayerState com;
			com.playerID = info->player->getId();
			com.state    = NAS_DISSCONNECT;
			sendTcpCommand( &com );
		}

		if ( bRMPlayer )
		{
			MUTEX_LOCK( mMutexPlayerChangeInfoVec );
			PlayerChangeInfo changeInfo;
			changeInfo.changeType = PlayerChangeInfo::eRemove;
			changeInfo.player = info->player;
			mPlayerChangeInfoVec.push_back( changeInfo );
			info->player = NULL;
		}
	}
	mClientManager.removeClient( info );
}


void ServerWorker::doUpdate( long time )
{
	BaseClass::doUpdate( time );
	if ( mLocalWorker )
		mLocalWorker->update( time );

	{
		MUTEX_LOCK( mMutexPlayerChangeInfoVec );
		for( PlayerChangeInfoVec::iterator iter = mPlayerChangeInfoVec.begin() , itEnd = mPlayerChangeInfoVec.end();
			iter != itEnd ; ++iter )
		{
			switch( iter->changeType )
			{
			case PlayerChangeInfo::eRemove:
				mPlayerManager->removePlayer( iter->player->getId() );
				break;
			case PlayerChangeInfo::eSweepToLocal:
				{
					PlayerId id = iter->player->getId();
					getPlayerManager()->swepNetPlayerToLocal( iter->player );
					if ( mNetListener )
						mNetListener->onPlayerStateMsg( id , PSM_CHANGE_TO_LOCAL );
				}
				break;
			}
		}
		mPlayerChangeInfoVec.clear();
	}
}

bool ServerWorker::kickPlayer( unsigned id )
{
	if ( mPlayerManager->getUserID() == id )
		return false;

	ServerPlayer* player = mPlayerManager->getPlayer( id );
	if ( !player )
		return false;

	if ( player->isNetwork() )
	{
		SNetPlayer* netPlayer = static_cast< SNetPlayer*>( player );
		removeConnect( &netPlayer->getClientInfo() , true );
	}
	else
	{
		if ( id != mPlayerManager->getUserID() )
		{
			mPlayerManager->removePlayer( id );
		}
	}
	return true;
}

bool ServerWorker::swapServer()
{
	//SNetPlayer* netPlayer = NULL;
	//for( IPlayerManager::Iterator iter = mServer->getPlayerManager()->createIterator();
	//	iter.haveMore() ; iter.goNext() )
	//{
	//	ServerPlayer* player = static_cast< ServerPlayer*>( iter.getElement() );

	//	if ( player->isNetWork() )
	//	{
	//		netPlayer = static_cast< SNetPlayer* >( player );
	//		break;
	//	}
	//}
	//if ( netPlayer )
	//{
	//	CSPComMsg comMsg;
	//	
	//	sprintf_s( comMsg.str , "serverchange %u %s" ,
	//		netPlayer->getID() ,
	//		netPlayer->getClientInfo().udpAddr.getIP() );

	//	netPlayer->sendTcpCommand( &comMsg );
	//	mWorker->sendTcpCommand( &comMsg );
	//}
	return false;
}



void ServerWorker::generatePlayerStatus( SPPlayerStatus& comPS )
{
	getPlayerManager()->getPlayerInfo( comPS.info );
	getPlayerManager()->getPlayerFlag( comPS.flags );
	comPS.numPlayer = (uint8)getPlayerManager()->getPlayerNum();
}

void ServerWorker::sendCommand(int channel , IComPacket* cp , unsigned flag)
{
	mPlayerManager->sendCommand( channel , cp , flag );
}

SVPlayerManager::SVPlayerManager()
{
	mUserID   = ERROR_PLAYER_ID;
	mListener = NULL;
}

SVPlayerManager::~SVPlayerManager()
{
	cleanup();
}

SNetPlayer* SVPlayerManager::createNetPlayer( ServerWorker* server , char const* name , ClientInfo* client  )
{
	MUTEX_LOCK( mMutexPlayerTable );

	SNetPlayer* player = new SNetPlayer( server , client );

	FixString< 64 > useName = name;

	int idx = 1;
	while( 1 )
	{
		bool done = true;

		for( PlayerTable::iterator iter = mPlayerTable.begin();
			iter != mPlayerTable.end(); ++iter )
		{
			ServerPlayer* player = *iter;
			if ( useName != player->getName() )
				continue;

			useName.format(  "%s(%d)" , name , idx++ );
			done = false;
			break;
		}
		if ( done )
			break;
	} 

	insertPlayer( player , useName , PT_PLAYER );

	return player;
}

SUserPlayer* SVPlayerManager::createUserPlayer( LocalWorker* worker , char const* name )
{
	MUTEX_LOCK( mMutexPlayerTable );

	if ( mUserID != ERROR_PLAYER_ID )
	{	
		Msg("Bug !! Can't Create 2 User Player" );
		return NULL;
	}

	SUserPlayer* player = new SUserPlayer( worker );

	insertPlayer( player , name , PT_PLAYER );

	mUserID = player->getId();

	return player;
}

SLocalPlayer* SVPlayerManager::createAIPlayer()
{
	SLocalPlayer* player = new SLocalPlayer;
	insertPlayer( player , "PC" , PT_COMPUTER );
	return player;
}

void SVPlayerManager::removePlayerFlag( unsigned bitPos )
{
	for( PlayerTable::iterator iter = mPlayerTable.begin();
		iter != mPlayerTable.end() ; ++iter )
	{
		ServerPlayer* player = *iter;
		player->getStateFlag().remove( bitPos );
	}

}
bool SVPlayerManager::checkPlayerFlag( unsigned bitPos , bool beNet )
{
	MUTEX_LOCK( mMutexPlayerTable );
	bool result = true;
	if ( beNet )
	{
		for( PlayerTable::iterator iter = mPlayerTable.begin();
			iter != mPlayerTable.end() ; ++iter )
		{
			ServerPlayer* player = *iter;
			if (  player->isNetwork() &&
				 !player->getStateFlag().check( bitPos )  )
				return false;
		}
	}
	else
	{
		for( PlayerTable::iterator iter = mPlayerTable.begin();
			iter != mPlayerTable.end() ; ++iter )
		{
			ServerPlayer* player = *iter;
			if ( !player->getStateFlag().check( bitPos ) )
				return false;
		}
	}
	return true;
}

void SVPlayerManager::getPlayerInfo( PlayerInfo* info[] )
{
	MUTEX_LOCK( mMutexPlayerTable );
	int i = 0;
	for ( PlayerTable::iterator iter = mPlayerTable.begin();
		iter != mPlayerTable.end() ; ++iter )
	{
		ServerPlayer* player = *iter;
		info[i++] = &player->getInfo();
	}
}

void SVPlayerManager::getPlayerFlag( int flags[] )
{
	MUTEX_LOCK( mMutexPlayerTable );
	int i = 0;
	for ( PlayerTable::iterator iter = mPlayerTable.begin();
		iter != mPlayerTable.end() ; ++iter )
	{
		ServerPlayer* player = *iter;
		flags[i++] = player->getStateFlag().getValue();
	}
}

void SVPlayerManager::cleanup()
{
	MUTEX_LOCK( mMutexPlayerTable );
	for( PlayerTable::iterator iter = mPlayerTable.begin() ; 
		iter != mPlayerTable.end() ; ++iter )
	{
		delete *iter;
	}
	mPlayerTable.clear();
}


bool SVPlayerManager::removePlayer( PlayerId id )
{
	MUTEX_LOCK( mMutexPlayerTable );

	ServerPlayer* player = getPlayer(id);

	if ( player == NULL )
		return false;

	PlayerInfo info = player->getInfo();

	delete player;
	mPlayerTable.remove( id );

	if ( id == mUserID )
		mUserID = ERROR_PLAYER_ID;

	if ( mListener )
		mListener->onRemovePlayer( info );

	return true;
}

void SVPlayerManager::sendCommand( int channel , IComPacket* cp , unsigned flag )
{
	MUTEX_LOCK( mMutexPlayerTable );
	for( PlayerTable::iterator iter = mPlayerTable.begin();
		iter != mPlayerTable.end(); ++iter )
	{
		ServerPlayer* player = *iter;
		if ( ( flag & WSF_IGNORE_LOCAL ) && !player->isNetwork() )
			continue;
		player->sendCommand( channel , cp );
	}
}

void SVPlayerManager::sendTcpCommand( IComPacket* cp )
{
	MUTEX_LOCK( mMutexPlayerTable );
	for( PlayerTable::iterator iter = mPlayerTable.begin();
		 iter != mPlayerTable.end(); ++iter )
	{
		ServerPlayer* player = *iter;
		player->sendTcpCommand( cp );
	}
}

void SVPlayerManager::sendUdpCommand( IComPacket* cp )
{
	MUTEX_LOCK( mMutexPlayerTable );
	for( PlayerTable::iterator iter = mPlayerTable.begin();
		iter != mPlayerTable.end(); ++iter )
	{
		ServerPlayer* player = *iter;
		player->sendUdpCommand( cp );
	}
}

void SVPlayerManager::insertPlayer( ServerPlayer* player , char const* name , PlayerType type )
{
	PlayerId id;

	MUTEX_LOCK( mMutexPlayerTable );
	id = (PlayerId)mPlayerTable.insert( player );

	player->init( mPlayerInfo[ id ] );
	player->getInfo().playerId = id;
	player->getInfo().name     = name;
	player->getInfo().type     = type;

	if ( mListener )
		mListener->onAddPlayer( id );
}

SLocalPlayer* SVPlayerManager::swepNetPlayerToLocal( SNetPlayer* player )
{
	MUTEX_LOCK( mMutexPlayerTable );

	SLocalPlayer* sPlayer = new SLocalPlayer();
	sPlayer->init( player->getInfo() );

	mPlayerTable.replace( player->getId() , sPlayer );
	delete player;

	return sPlayer;
}


ServerPlayer* SVPlayerManager::getPlayer( PlayerId id )
{
	if ( id == ERROR_PLAYER_ID )
		return NULL;

	MUTEX_LOCK( mMutexPlayerTable );
	if ( mPlayerTable.getItem( id ) )
		return  *mPlayerTable.getItem( id );
	return NULL;
}

size_t  SVPlayerManager::getPlayerNum()
{
	MUTEX_LOCK( mMutexPlayerTable ); 
	return mPlayerTable.size();
}

ServerClientManager::ServerClientManager()
{
	mNextId = 1;
}

ServerClientManager::~ServerClientManager()
{
	cleanup();
}

void ServerClientManager::sendUdpData( long time , UdpServer& server )
{
	MUTEX_LOCK( mMutexClientMap );

	for( SessionMap::iterator iter = mSessionMap.begin();
		iter != mSessionMap.end(); ++iter )
	{
		ClientInfo* client = iter->second;
		server.sendPacket( time , client->udpClient , client->udpAddr );
	}
}

ClientInfo* ServerClientManager::createClient( NetSocket& socket  )
{
	MUTEX_LOCK( mMutexClientMap );

	ClientInfo* client = new ClientInfo( socket );

	client->id      = getNewSessionId();
	//client->udpAddr.setPort( TG_UDP_PORT );
	client->player = NULL;
	mSessionMap.insert( std::make_pair( client->id , client ) );

	return client;
}

bool ServerClientManager::doRemoveClient( SessionId id )
{
	MUTEX_LOCK( mMutexClientMap );
	SessionMap::iterator iter = mSessionMap.find( id );

	if ( iter == mSessionMap.end() )
		return false;

	AddrMap::iterator addrIter = mAddrMap.find( &iter->second->udpAddr );
	if ( addrIter != mAddrMap.end() )
		mAddrMap.erase( addrIter );

	cleanupClient( iter->second );
	mSessionMap.erase( iter );


	return true;
}

bool ServerClientManager::removeClient( ClientInfo* info )
{
	MUTEX_LOCK( mMutexClientMap );
	mRemoveList.push_back( info );
	return true;
}

void ServerClientManager::cleanupClient( ClientInfo* info )
{
	delete info;
}



ClientInfo* ServerClientManager::findClient( NetAddress const& addr )
{
	MUTEX_LOCK( mMutexClientMap );
	AddrMap::iterator iter = mAddrMap.find( &addr );

	if ( iter != mAddrMap.end() )
		return iter->second;

	return NULL;
}

ClientInfo* ServerClientManager::findClient( SessionId id )
{
	MUTEX_LOCK( mMutexClientMap );
	SessionMap::iterator iter = mSessionMap.find( id );

	if ( iter != mSessionMap.end() )
		return iter->second;

	return NULL;
}

void ServerClientManager::updateNet( long time )
{
	MUTEX_LOCK( mMutexClientMap );
	for( SessionMap::iterator iter = mSessionMap.begin();
		iter != mSessionMap.end(); ++iter )
	{
		ClientInfo* client = iter->second;
		client->tcpClient.updateSocket( time );
	}

	for( ClientList::iterator iter = mRemoveList.begin() ;
		 iter != mRemoveList.end() ; ++iter )
	{
		ClientInfo* info = *iter;
		doRemoveClient( info->id );
	}
	mRemoveList.clear();
}

void ServerClientManager::sendTcpCommand( ComEvaluator& evaluator , IComPacket* cp )
{
	MUTEX_LOCK( mMutexClientMap );
	for( SessionMap::iterator iter = mSessionMap.begin();
		iter != mSessionMap.end(); ++iter )
	{
		ClientInfo* client = iter->second;
		FillBufferFromCom( client->tcpClient.getSendCtrl() , cp );
	}
}

void ServerClientManager::sendUdpCommand( ComEvaluator& evaluator , IComPacket* cp )
{
	MUTEX_LOCK( mMutexClientMap );
	for( SessionMap::iterator iter = mSessionMap.begin();
		iter != mSessionMap.end(); ++iter )
	{
		ClientInfo* client = iter->second;
		FillBufferFromCom( client->udpClient.getSendCtrl() , cp );
	}
}

void ServerClientManager::cleanup()
{
	MUTEX_LOCK( mMutexClientMap );
	for( SessionMap::iterator iter = mSessionMap.begin() ; 
		iter != mSessionMap.end() ; ++iter )
	{
		cleanupClient( iter->second );
	}
	mSessionMap.clear();
	mAddrMap.clear();
}

SessionId ServerClientManager::getNewSessionId()
{
	SessionId id = mNextId;
	mNextId += 1;
	return id;
}

void ServerClientManager::setClientUdpAddr( SessionId id , NetAddress const& addr )
{
	ClientInfo* info = findClient( id );
	if ( !info )
		return;
	info->udpAddr = addr;
	bool isOk = mAddrMap.insert( std::make_pair( &info->udpAddr , info ) ).second;
	assert( isOk );
}

SUserPlayer::SUserPlayer( LocalWorker* worker ) 
	:SLocalPlayer()
	,mWorker( worker )
{

}

void SUserPlayer::sendCommand( int channel , IComPacket* cp )
{
	mWorker->recvCommand( cp );
	//mWorker->getEvaluator().execCommand( cp );
}

void SUserPlayer::sendTcpCommand( IComPacket* cp )
{
	mWorker->recvCommand( cp );
	//mWorker->getEvaluator().execCommand( cp );
}

void SUserPlayer::sendUdpCommand( IComPacket* cp )
{
	mWorker->recvCommand( cp );
	//mWorker->getEvaluator().execCommand( cp );
}

LocalWorker::LocalWorker( ServerWorker* worker )
	:mSendBuffer( 1024 )
	,mRecvBuffer( 1024 )
{
	mServer = worker;
	mPlayerMgr = worker->getPlayerManager();

#define COM_PACKET_SET( Class , Processer , Fun , Fun2 )\
	getEvaluator().setWorkerFun< Class >( Processer , Fun , Fun2 );

#define COM_THIS_PACKET_SET( Class , Fun )\
	COM_PACKET_SET( Class , this , &LocalWorker::##Fun , NULL )

#define COM_THIS_PACKET_SET_2( Class , Fun , SocketFun )\
	COM_PACKET_SET( Class , this , &LocalWorker::##Fun , &LocalWorker::##SocketFun )

	COM_THIS_PACKET_SET( CSPPlayerState , procPlayerState )
	COM_THIS_PACKET_SET( CSPClockSynd   , procClockSynd )

#undef  COM_PACKET_SET
#undef  COM_THIS_PACKET_SET
#undef  COM_THIS_PACKET_SET_2

}

void LocalWorker::procPlayerState( IComPacket* cp)
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();

	if ( com->playerID == ERROR_PLAYER_ID )
	{
		//changeState( ( NetActionState )com->state );
	}
}

void LocalWorker::procClockSynd( IComPacket* cp)
{
	CSPClockSynd* com = cp->cast< CSPClockSynd >();
	if ( com->code == CSPClockSynd::eSTART )
	{
		CSPClockSynd synd;
		synd.code    = CSPClockSynd::eDONE;
		synd.latency = 0;
		sendTcpCommand( &synd );

		changeState( NAS_TIME_SYNC );
	}
}

void LocalWorker::postChangeState( NetActionState oldState )
{

	CSPPlayerState com;
	com.playerID = mPlayerMgr->getUserID();
	com.state    = getActionState();
	sendTcpCommand( &com );

}

void LocalWorker::doUpdate( long time )
{
	try
	{
		while( mSendBuffer.getAvailableSize() )
		{		
			if ( !mServer->getEvaluator().evalCommand( mSendBuffer ) )
				break;
		}

	}
	catch( ComException& )
	{



	}
	mSendBuffer.clear();

	try
	{
		while( mRecvBuffer.getAvailableSize() )
		{
			if ( !getEvaluator().evalCommand( mRecvBuffer ) )
				break;
		}
	}
	catch( ComException& )
	{



	}
	mRecvBuffer.clear();

}

void LocalWorker::sendCommand(int channel , IComPacket* cp , unsigned flag)
{
	::FillBufferFromCom( mSendBuffer , cp );
}

void LocalWorker::recvCommand(IComPacket* cp)
{
	::FillBufferFromCom( mRecvBuffer , cp );
}

void SNetPlayer::sendCommand( int channel , IComPacket* cp )
{
	SocketBuffer buffer( 1000 );
	int num = FillBufferFromCom( buffer , cp );
	switch( channel )
	{
	case CHANNEL_GAME_NET_TCP:
		FillBufferFromCom( mClientInfo->tcpClient.getSendCtrl() , cp );
		break;
	case CHANNEL_GAME_NET_UDP_CHAIN:
		FillBufferFromCom( mClientInfo->udpClient.getSendCtrl() , cp );
		break;
	}
}

void SNetPlayer::sendTcpCommand( IComPacket* cp )
{
	FillBufferFromCom( mClientInfo->tcpClient.getSendCtrl() , cp );
}

void SNetPlayer::sendUdpCommand( IComPacket* cp )
{
	FillBufferFromCom( mClientInfo->udpClient.getSendCtrl() , cp );
}

SNetPlayer::SNetPlayer( ServerWorker* server , ClientInfo* cInfo ) 
	:ServerPlayer( true )
{
	mServer     = server;
	mClientInfo = cInfo;
	cInfo->player = this;
}
