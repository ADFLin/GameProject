#include "TinyGamePCH.h"
#include "GameServer.h"



#include "GameNetPacket.h"
#include "ConsoleSystem.h"
int const CLIENT_GROUP = 1;
#define SERVER_USE_CONNECTED_UDP 1

TConsoleVariable<bool> CVarSvUseConnectedUDP(false, "sv.UseConnectedUDP");


#if !USE_NEW_NETWORK


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

void ServerWorker::sendClientTcpCommand( NetClientData& client , IComPacket* cp )
{
	TcpServerClientChannel channel(client.tcpChannel);
	channel.send(cp);
}

bool ServerWorker::doStartNetwork()
{
	try 
	{
		mTcpServer.run( TG_TCP_PORT );
		mUdpServer.run( TG_UDP_PORT );
		mNetSelect.addSocket(mTcpServer.getSocket());
		mNetSelect.addSocket(mUdpServer.getSocket());
	}
	catch ( ... )
	{
		return false;
	}


	typedef ServerWorker ThisClass;

#define COM_PACKET_SET( Class , Processer , Func , Func2 )\
	getEvaluator().setWorkerFunc< Class >( Processer , Func , Func2 );

#define COM_THIS_PACKET_SET( Class , Func )\
	COM_PACKET_SET( Class , this , &ThisClass::Func , NULL )


#define COM_THIS_PACKET_SET_2( Class , Func , SocketFunc )\
	COM_PACKET_SET( Class , this , &ThisClass::Func , &ThisClass::SocketFunc )

	COM_THIS_PACKET_SET( CPLogin       , procLogin )
	COM_THIS_PACKET_SET( CPEcho        , procEcho )
	COM_THIS_PACKET_SET( CSPMsg        , procMsg )

	COM_THIS_PACKET_SET_2( CPUdpCon       , procUdpCon , procUdpCon_NetThread )
	COM_THIS_PACKET_SET_2( CSPClockSynd   , procClockSynd , procClockSynd_NetThread )
	COM_THIS_PACKET_SET_2( CSPComMsg      , procComMsg    , procComMsg_NetThread )

	COM_THIS_PACKET_SET ( CSPPlayerState , procPlayerState )

	
#undef  COM_PACKET_SET
#undef  COM_THIS_PACKET_SET
#undef  COM_THIS_PACKET_SET_2

	changeState( NAS_CONNECT );
	return true;
}


void ServerWorker::doCloseNetwork()
{
	mPlayerManager->cleanup();
}

void ServerWorker::clenupNetResource()
{
	mClientManager.cleanup();
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
		}
		break;
	case NAS_TIME_SYNC:
		{
			mPlayerManager->removePlayerFlag( ServerPlayer::eSyndDone );

			CSPClockSynd com;
			com.code = CSPClockSynd::eSTART;
			com.numSample = 20;
			if (!sendTcpCommand(&com, WSF_IGNORE_LOCAL))
			{
				changeState(NAS_LEVEL_RUN);
			}
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


bool ServerWorker::notifyConnectionRecv( NetConnection* connection , SocketBuffer& buffer , NetAddress* clientAddr )
{
	if( clientAddr )
	{
		assert(  connection == &mUdpServer );
		NetClientData* client = mClientManager.findClient( *clientAddr );

		bool result = true;

		//maybe broadcast
		if( client == nullptr )
		{
			LogMsg("ip = %lu , port = %u", clientAddr->get().sin_addr.s_addr, clientAddr->get().sin_port);

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
			result = FNetCommand::Eval(client->udpChannel, getEvaluator(), buffer, CLIENT_GROUP , client);
		}

		return result;
	}
	else
	{
		if ( connection == &mTcpServer )
			return true;

		NetClientData::TCPClient* tcpChannel = static_cast< NetClientData::TCPClient*>( connection );
		NetClientData* client  = tcpChannel->client;

		while( buffer.getAvailableSize() )
		{		
			if ( !getEvaluator().evalCommand( buffer , CLIENT_GROUP, client ) )
			{
				return false;
			}
		}
	}
	return true;
}


bool ServerWorker::update_NetThread( long time )
{
	BaseClass::update_NetThread(time);

	if( mLocalWorker )
		mLocalWorker->update_NetThread(time);

	if ( mNetSelect.select(0))
	{
		mTcpServer.updateSocket(time, mNetSelect);
		mUdpServer.updateSocket(time, mNetSelect);
		mClientManager.updateSocket(time, mNetSelect);
	}

	return true;
}

void ServerWorker::notifyConnectionSend( NetConnection* con )
{
	assert( con == &mUdpServer );
	sendUdpCmd( mUdpServer.getSocket() );
#if SERVER_USE_CONNECTED_UDP
	if (!CVarSvUseConnectedUDP)
		mClientManager.sendUdpData( getNetRunningTime() , mUdpServer );
#endif
}


void ServerWorker::notifyConnectionAccpet( NetConnection* con )
{
	assert( con == &mTcpServer );

	NetSocket conSocket;
	NetAddress hostAddr;
	if ( !mTcpServer.getSocket().accept( conSocket , hostAddr ) )
		return;

	LogMsg( "Accpet ip = %lu port = %u" , hostAddr.get().sin_addr.s_addr , hostAddr.get().sin_port );

	NetClientData* client = mClientManager.findClient(hostAddr);
	if( client )
	{
		client->reconnect(conSocket);
		if( client->ownerId != ERROR_PLAYER_ID )
		{
			if( mEventResolver )
				mEventResolver->resolveReconnect_NetThread( ServerResolveContext( *this , client->ownerId) );

			CSPPlayerState com;
			com.playerId = client->ownerId;
			com.state = NAS_RECONNECT;
			sendTcpCommand(&com);
		}
	}
	else
	{
		client = mClientManager.createClient(hostAddr , conSocket);
		if( client )
		{
			client->tcpChannel.setListener(this);
			mNetSelect.addSocket(client->tcpChannel.getSocket());
			SPConnectMsg com;
			com.result = SPConnectMsg::eNEW_CON;
			com.id = client->id;
			sendClientTcpCommand(*client, &com);
		}
		else
		{



		}
	}
}

void ServerWorker::notifyConnectionClose( NetConnection* con , NetCloseReason reason )
{
	NetClientData* client  = static_cast< NetClientData::TCPClient* >( con )->client;
	PlayerId playerId = client->ownerId;

	PlayerConnetionClosedAction mode = PlayerConnetionClosedAction::Remove;

	if( mEventResolver && playerId != ERROR_PLAYER_ID )
		mode = mEventResolver->resolveConnectClosed_NetThread( ServerResolveContext( *this , playerId) , reason);

	switch( mode )
	{
	case PlayerConnetionClosedAction::Remove:
		{
			removeConnect_NetThread(client, true);
			addGameThreadCommnad([this, playerId]
			{
				mPlayerManager->removePlayer(playerId);
			});
		}
		break;
	case PlayerConnetionClosedAction::ChangeToLocal:
		{
			removeConnect_NetThread(client , false);
			addGameThreadCommnad([this, playerId]
			{
				auto player = static_cast<SNetPlayer*>( getPlayerManager()->getPlayer(playerId) );
				getPlayerManager()->swepNetPlayerToLocal(player);
				if( mNetListener )
					mNetListener->onPlayerStateMsg(playerId, PSM_CHANGE_TO_LOCAL);
			});
		}
		break;
	case PlayerConnetionClosedAction::WaitReconnect:
		{
			addGameThreadCommnad([this, playerId]
			{
				auto player = static_cast<SNetPlayer*>(getPlayerManager()->getPlayer(playerId));
				player->getStateFlag().add(ServerPlayer::eDissconnect);

				CSPPlayerState com;
				com.playerId = playerId;
				com.state = NAS_DISSCONNECT;
				sendTcpCommand(&com);
			});
		}
		break;
	}

	addGameThreadCommnad([this]
	{
		SPPlayerStatus infoCom;
		getPlayerManager()->getPlayerInfo(infoCom.info);
		infoCom.numPlayer = (uint8)getPlayerManager()->getPlayerNum();
		sendTcpCommand(&infoCom);
	});

}

void ServerWorker::procUdpCon_NetThread( IComPacket* cp )
{
	assert(IsInNetThread());

	if ( cp->getUserData() && cp->getGroup() == -1 )
	{
		NetAddress* sendAddr = (NetAddress*)cp->getUserData();
		CPUdpCon* com = cp->cast< CPUdpCon >();
		NetClientData* client = mClientManager.setClientUdpAddr(com->id, *sendAddr);
		if (client)
		{
			mNetSelect.addSocket(client->udpChannel.getSocket());
		}
	}
	//::Msg("procUdpConNet");
}

void ServerWorker::procUdpCon( IComPacket* cp)
{

}

void ServerWorker::procLogin( IComPacket* cp)
{
	CPLogin* com = cp->cast< CPLogin >();
	NetClientData* client = static_cast< NetClientData*>( cp->getUserData() );

	if ( !client )
	{
		return;
	}

	SNetPlayer* player = getPlayerManager()->createNetPlayer( this , com->name , client );
	if( player )
	{

		PlayerId playerId = player->getId();
		addNetThreadCommnad([client, playerId]
		{
			client->ownerId = playerId;
		});

		{
			//SPPlayerStatus PSCom;
			//generatePlayerStatus( PSCom );
			//sendTcpCommand( &PSCom );

			CSPPlayerState stateCom;
			stateCom.playerId = player->getId();
			stateCom.state = NAS_ACCPET;
			player->sendTcpCommand(&stateCom);

			stateCom.playerId = player->getId();
			stateCom.state = NAS_CONNECT;
			sendTcpCommand(&stateCom);
		}
	}
	else
	{
		addNetThreadCommnad([this,client]
		{
			removeClient(client);
		});
		return;
	}
}


void ServerWorker::procEcho( IComPacket* cp)
{
	CPEcho* com = cp->cast< CPEcho >();
	NetClientData* client = static_cast< NetClientData*>(cp->getUserData());
	assert( client );
	FNetCommand::Write( client->udpChannel.getSendCtrl() , cp );
}


void ServerWorker::procMsg( IComPacket* cp)
{
	sendTcpCommand( cp );
}


void ServerWorker::procClockSynd_NetThread( IComPacket* cp )
{
	NetClientData* client = static_cast< NetClientData*>( cp->getUserData() );
	if ( client == nullptr )
		return;

	CSPClockSynd* com = cp->cast< CSPClockSynd >();
	switch( com->code )
	{
	case CSPClockSynd::eREQUEST:
		{
			LogMsg("ClockSynd Request");
			CSPClockSynd packet;
			packet.code = CSPClockSynd::eREPLY;
			sendClientTcpCommand(*client, &packet);
		}
		break;
	case CSPClockSynd::eDONE:
		{
			LogMsg("ClockSynd Done");
			PlayerId playerId = client->ownerId;
			long latency = com->latency;
			addGameThreadCommnad([this,playerId, latency]
			{		
				LogMsg("Player %d synd done , ping = %u", playerId, latency);
				auto player = mPlayerManager->getPlayer(playerId);
				player->latency = latency;
			});
		}
		break;
	}
}

void ServerWorker::procClockSynd( IComPacket* cp )
{
	CSPClockSynd* com = cp->cast< CSPClockSynd >();


	if ( cp->getUserData() )
	{
		NetClientData* client = static_cast< NetClientData*>(cp->getUserData());
	}
	else
	{

	}

	switch( com->code )
	{
	case CSPClockSynd::eREQUEST:
		{

		}
		break;
	case CSPClockSynd::eDONE:
		{

		}
		break;
	}
}

void ServerWorker::procPlayerState( IComPacket* cp )
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();

	NetClientData* client = static_cast< NetClientData*>(cp->getUserData());

	if ( com->playerId == ERROR_PLAYER_ID || com->playerId == SERVER_PLAYER_ID )
	{
		return;
	}

	ServerPlayer* player = getPlayerManager()->getPlayer( com->playerId );
	
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
			LogDevMsg( 5, "Plyer %d not Ready" , player->getId() );
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
			LogDevMsg( 5, "Plyer %d Ready" , player->getId() );
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
			player->getStateFlag().add(ServerPlayer::eSyndDone);
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

void ServerWorker::procComMsg_NetThread( IComPacket* cp)
{
	if( cp->getUserData() == nullptr )
		return;

	CSPComMsg* com = cp->cast< CSPComMsg >();

	if ( com->str == "server_info" )
	{
		InlineString< 256 > hostname;
		
		NetAddress sendAddr = *(NetAddress*)com->getUserData();
		if( cp->getGroup() == CLIENT_GROUP )
		{
			sendAddr = static_cast<NetClientData*>(cp->getUserData())->udpAddr;
		}
		else
		{
			sendAddr = *(NetAddress*)com->getUserData();
		}

		if( gethostname(hostname, hostname.max_size()) == 0 )
		{
			hostent* hn = gethostbyname(hostname);
			InlineString<32> ip = inet_ntoa(*(struct in_addr *)hn->h_addr_list[0]);

			addGameThreadCommnad([this, ip, sendAddr]
			{
				SPServerInfo info;
				ServerPlayer* player = mPlayerManager->getPlayer(mPlayerManager->getUserID());
				if( player )
				{
					info.name.format("%s's Game", player->getName());
				}
				else
				{
					info.name = "server";
				}
				info.ip = ip;
				addUdpCom(&info, sendAddr);
			});

		}
	}
}

void ServerWorker::procComMsg( IComPacket* cp)
{

}


void ServerWorker::procUdpAddress( IComPacket* cp)
{
	

}

void ServerWorker::removeConnect_NetThread( NetClientData* client , bool bRMPlayer )
{
	assert(IsInNetThread());

	if( client->ownerId != ERROR_PLAYER_ID )
	{
		PlayerId playerId = client->ownerId;
		if( bRMPlayer )
		{
			client->ownerId = ERROR_PLAYER_ID;
		}

		removeClient(client);

		addGameThreadCommnad([this, playerId, bRMPlayer]
		{
			auto player = mPlayerManager->getPlayer(playerId);
			if ( player )
			{
				if( bRMPlayer )
				{
					mPlayerManager->removePlayer(playerId);
				}

				if( !player->getStateFlag().check(ServerPlayer::eDissconnect) )
				{
					CSPPlayerState com;
					com.playerId = playerId;
					com.state = NAS_DISSCONNECT;
					sendTcpCommand(&com);
				}
			}
		});
	}
}


void ServerWorker::doUpdate( long time )
{
	BaseClass::doUpdate( time );

	if ( mLocalWorker )
		mLocalWorker->update( time );
}

bool ServerWorker::kickPlayer( unsigned id )
{
	assert(IsInGameThead());

	if ( mPlayerManager->getUserID() == id )
		return false;

	ServerPlayer* player = mPlayerManager->getPlayer( id );
	if ( !player )
		return false;

	if ( player->isNetwork() )
	{
		SNetPlayer* netPlayer = static_cast< SNetPlayer*>( player );
		auto client = &netPlayer->getClient();
		addNetThreadCommnad([this,client]
		{
			removeConnect_NetThread(client, true);
		});
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
	//		netPlayer->getClient().udpAddr.getIP() );

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

bool ServerWorker::sendCommand(int channel , IComPacket* cp , EWorkerSendFlag flag)
{
	return mPlayerManager->sendCommand( channel , cp , flag );
}

#endif

SVPlayerManager::SVPlayerManager()
{
	mUserID   = ERROR_PLAYER_ID;
	mListener = NULL;
}

SVPlayerManager::~SVPlayerManager()
{
	cleanup();
}

SNetPlayer* SVPlayerManager::createNetPlayer( ServerWorker* server , char const* name , NetClientData* client  )
{
	NET_MUTEX_LOCK( mMutexPlayerTable );

	SNetPlayer* player =  new SNetPlayer( server , client );

	InlineString< 64 > nameUsed = name;

	int idx = 1;
	while( 1 )
	{
		bool done = true;

		for( PlayerTable::iterator iter = mPlayerTable.begin();
			iter != mPlayerTable.end(); ++iter )
		{
			ServerPlayer* player = *iter;
			if ( nameUsed != player->getName() )
				continue;

			nameUsed.format(  "%s(%d)" , name , idx++ );
			done = false;
			break;
		}
		if ( done )
			break;
	} 

	insertPlayer( player , nameUsed , PT_PLAYER );

	return player;
}

SUserPlayer* SVPlayerManager::createUserPlayer( LocalWorker* worker , char const* name )
{
	NET_MUTEX_LOCK( mMutexPlayerTable );

	if ( mUserID != ERROR_PLAYER_ID )
	{	
		LogMsg("Bug !! Can't Create 2 User Player" );
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
	for (ServerPlayer* player : mPlayerTable)
	{
		player->getStateFlag().remove( bitPos );
	}

}
bool SVPlayerManager::checkPlayerFlag( unsigned bitPos , bool beNet )
{
	NET_MUTEX_LOCK( mMutexPlayerTable );
	bool result = true;
	if ( beNet )
	{
		for (ServerPlayer* player : mPlayerTable)
		{
			if (  player->isNetwork() &&
				 !player->getStateFlag().check( bitPos )  )
				return false;
		}
	}
	else
	{
		for (ServerPlayer* player : mPlayerTable)
		{
			if ( !player->getStateFlag().check( bitPos ) )
				return false;
		}
	}
	return true;
}

void SVPlayerManager::getPlayerInfo( PlayerInfo* info[] )
{
	NET_MUTEX_LOCK( mMutexPlayerTable );
	int i = 0;
	for (ServerPlayer* player : mPlayerTable)
	{
		info[i++] = &player->getInfo();
	}
}

void SVPlayerManager::getPlayerFlag( int flags[] )
{
	NET_MUTEX_LOCK( mMutexPlayerTable );
	int i = 0;
	for (ServerPlayer* player : mPlayerTable)
	{
		flags[i++] = player->getStateFlag().getValue();
	}
}

void SVPlayerManager::cleanup()
{
	NET_MUTEX_LOCK( mMutexPlayerTable );
	for (ServerPlayer* player : mPlayerTable)
	{
		delete player;
	}
	mPlayerTable.clear();
}


bool SVPlayerManager::removePlayer( PlayerId id )
{
	NET_MUTEX_LOCK( mMutexPlayerTable );

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

bool SVPlayerManager::sendCommand(int channel, IComPacket* cp, EWorkerSendFlag flag)
{
	NET_MUTEX_LOCK( mMutexPlayerTable );
	bool result = false;
	for(ServerPlayer* player : mPlayerTable)
	{
		if ( ( flag & WSF_IGNORE_LOCAL ) && !player->isNetwork() )
			continue;
		player->sendCommand( channel , cp );
		result = true;
	}
	return result;
}

void SVPlayerManager::insertPlayer( ServerPlayer* player , char const* name , PlayerType type )
{
	PlayerId id;

	NET_MUTEX_LOCK( mMutexPlayerTable );
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
	NET_MUTEX_LOCK( mMutexPlayerTable );

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

	NET_MUTEX_LOCK( mMutexPlayerTable );
	if ( mPlayerTable.getItem( id ) )
		return  *mPlayerTable.getItem( id );
	return NULL;
}

size_t  SVPlayerManager::getPlayerNum()
{
	NET_MUTEX_LOCK( mMutexPlayerTable ); 
	return mPlayerTable.size();
}

ServerClientManager::ServerClientManager()
{
	mNextId = 1;
}

ServerClientManager::~ServerClientManager()
{
	if( IsInNetThread() )
	{
		cleanup();
	}
	else
	{
		assert(mSessionMap.empty() && mAddrMap.empty());
	}
}

void ServerClientManager::sendUdpData(long time, UdpServer& server)
{
	assert(IsInNetThread());

	for (SessionMap::iterator iter = mSessionMap.begin();
		iter != mSessionMap.end(); ++iter)
	{
		NetClientData* client = iter->second;
		server.sendPacket(time, client->udpChannel,client->udpAddr);
	}
}

NetClientData* ServerClientManager::createClient(NetAddress const& address, NetSocket& socket  )
{
	assert(IsInNetThread());

	NetClientData* client = new NetClientData( socket );

	client->id      = getNewSessionId();
	client->ownerId = ERROR_PLAYER_ID;
	client->udpAddr = address;
	client->udpAddr.setPort( TG_UDP_PORT );
	mSessionMap.insert( std::make_pair( client->id , client ) );

	return client;
}

bool ServerClientManager::doRemoveClient( SessionId id )
{
	assert(IsInNetThread());

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

bool ServerClientManager::removeClient( NetClientData* info )
{
	assert(IsInNetThread());
	mRemoveList.push_back( info );
	return true;
}

void ServerClientManager::cleanupClient( NetClientData* info )
{
	delete info;
}



NetClientData* ServerClientManager::findClient( NetAddress const& addr )
{
	assert(IsInNetThread());
	AddrMap::iterator iter = mAddrMap.find( &addr );

	if ( iter != mAddrMap.end() )
		return iter->second;

	return NULL;
}

NetClientData* ServerClientManager::findClient( SessionId id )
{
	assert(IsInNetThread());
	SessionMap::iterator iter = mSessionMap.find( id );

	if ( iter != mSessionMap.end() )
		return iter->second;

	return NULL;
}

void ServerClientManager::updateSocket( long time , NetSelectSet& netSelect)
{
	assert(IsInNetThread());
#if 1
#if SERVER_USE_CONNECTED_UDP
	if (CVarSvUseConnectedUDP)
	{
		for (auto& pair : mSessionMap)
		{
			NetClientData* client = pair.second;
			client->updateUdpSocket(time, netSelect);
		}
	}
#endif
	for (auto& pair : mSessionMap)
	{
		NetClientData* client = pair.second;
		client->tcpChannel.updateSocket( time , netSelect);
	}
#else

#if SERVER_USE_CONNECTED_UDP
	if (CVarSvUseConnectedUDP)
	{
		for (auto& pair : mSessionMap)
		{
			NetClientData* client = pair.second;
			client->updateUdpSocket(time);
		}
	}
#endif
	for (auto& pair : mSessionMap)
	{
		NetClientData* client = pair.second;
		client->tcpChannel.updateSocket(time);
	}
#endif

	for( ClientList::iterator iter = mRemoveList.begin() ;
		 iter != mRemoveList.end() ; ++iter )
	{
		NetClientData* client = *iter;
		doRemoveClient( client->id );
	}
	mRemoveList.clear();
}

void ServerClientManager::sendTcpCommand( ComEvaluator& evaluator , IComPacket* cp )
{
	assert(IsInNetThread());
	for( SessionMap::iterator iter = mSessionMap.begin();
		iter != mSessionMap.end(); ++iter )
	{
		NetClientData* client = iter->second;
		FNetCommand::Write( client->tcpChannel.getSendCtrl() , cp );
	}
}

void ServerClientManager::sendUdpCommand( ComEvaluator& evaluator , IComPacket* cp )
{
	assert(IsInNetThread());
	for( SessionMap::iterator iter = mSessionMap.begin();
		iter != mSessionMap.end(); ++iter )
	{
		NetClientData* client = iter->second;
		FNetCommand::Write( client->udpChannel.getSendCtrl() , cp );
	}
}

void ServerClientManager::cleanup()
{
	assert(IsInNetThread());
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

NetClientData* ServerClientManager::setClientUdpAddr( SessionId id , NetAddress const& addr )
{
	NetClientData* client = findClient( id );
	if ( !client )
		return nullptr;

	client->setUDPAddress(addr);

	bool isOk = mAddrMap.insert( std::make_pair( &client->udpAddr , client ) ).second;
	assert( isOk );
	return client;
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

#define COM_PACKET_SET( Class , Processer , Func , Fun2 )\
	getEvaluator().setWorkerFunc< Class >( Processer , Func , Fun2 );

#define COM_THIS_PACKET_SET( Class , Func )\
	COM_PACKET_SET( Class , this , &LocalWorker::Func , NULL )

#define COM_THIS_PACKET_SET_2( Class , Func , SocketFunc )\
	COM_PACKET_SET( Class , this , &LocalWorker::Func , &LocalWorker::SocketFunc )

	COM_THIS_PACKET_SET( CSPPlayerState , procPlayerState )
	COM_THIS_PACKET_SET( CSPClockSynd   , procClockSynd )

#undef  COM_PACKET_SET
#undef  COM_THIS_PACKET_SET
#undef  COM_THIS_PACKET_SET_2

}

void LocalWorker::procPlayerState( IComPacket* cp)
{
	CSPPlayerState* com = cp->cast< CSPPlayerState >();

	if ( com->playerId == SERVER_PLAYER_ID )
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
	com.playerId = mPlayerMgr->getUserID();
	com.state    = getActionState();
	sendTcpCommand( &com );
}

void LocalWorker::doUpdate(long time)
{
	//MUTEX_LOCK(mMutexBuffer);
	try
	{
		while( mSendBuffer.getAvailableSize() )
		{
			if( !mServer->getEvaluator().evalCommand(mSendBuffer) )
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
			if( !getEvaluator().evalCommand(mRecvBuffer) )
				break;
		}
	}
	catch( ComException& )
	{



	}
	mRecvBuffer.clear();
}


void LocalWorker::update_NetThread(long time)
{

}

bool LocalWorker::sendCommand(int channel , IComPacket* cp , EWorkerSendFlag flag)
{
	//MUTEX_LOCK(mMutexBuffer);
	FNetCommand::Write( mSendBuffer , cp );
	return true;
}

void LocalWorker::recvCommand(IComPacket* cp)
{
	//MUTEX_LOCK(mMutexBuffer);
	FNetCommand::Write( mRecvBuffer , cp );
}

void SNetPlayer::sendCommand( int channel , IComPacket* cp )
{
	switch( channel )
	{
	case CHANNEL_GAME_NET_TCP:
		if (mTcpChannel)
			mTcpChannel->send(cp);
		break;
	case CHANNEL_GAME_NET_UDP_CHAIN:
		if (mUdpChannel)
			mUdpChannel->send(cp);
		break;
	}
}

void SNetPlayer::sendTcpCommand( IComPacket* cp )
{
	if (mTcpChannel)
		mTcpChannel->send(cp);
}

void SNetPlayer::sendUdpCommand( IComPacket* cp )
{
	if (mUdpChannel)
		mUdpChannel->send(cp);
}

void SNetPlayer::initChannels()
{
	if (mClient)
	{
		mTcpChannel = std::make_unique<TcpServerClientChannel>(mClient->tcpChannel);
		mUdpChannel = std::make_unique<UdpChainChannel>(mClient->udpChannel, 
			mServer->getUdpSocket(), mClient->udpAddr);
	}
}

SNetPlayer::SNetPlayer( ServerWorker* server , NetClientData* client ) 
	:ServerPlayer( true )
{
	mServer     = server;
	mClient     = client;
	initChannels();
}


