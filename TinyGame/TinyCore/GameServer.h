#ifndef GameServer_h__
#define GameServer_h__

#include "GameWorker.h"
#include "GamePlayer.h"

#include "Flag.h"
#include "Holder.h"

class LocalWorker;
class SNetPlayer;
class ServerWorker;

class SPPlayerStatus;


struct LocalClientData
{



};

struct NetClientData
{
	typedef UdpServer::Client UdpClient;

	class TCPClient : public TcpServer::Client
	{
	public:
		TCPClient( NetSocket& socket )
		{
			getSocket().move( socket );
		}
		NetClientData* client;
	};


	NetClientData( NetSocket& socket )
		:tcpChannel( socket )
	{
		tcpChannel.client = this;
		ownerId = ERROR_PLAYER_ID;
	}

	void reconnect(NetSocket& socket)
	{
		tcpChannel.getSocket().move(socket);
		tcpChannel.clearBuffer();
		udpChannel.clearBuffer();
	}
	volatile int32 locked;
	SessionId    id;
	TCPClient    tcpChannel;
	UdpClient    udpChannel;
	NetAddress   udpAddr;
	PlayerId     ownerId;
};


class ServerPlayer : public GamePlayer
{
public:
	ServerPlayer( bool bNetWork )
		:GamePlayer() 
		,mbNetWork( bNetWork )
	{
		mFlag.clear();
	}
	enum
	{
		eReady    ,
		eSyndDone ,
		ePause    ,
		eLevelSetup  ,
		eLevelLoaded ,
		eLevelReady  ,
		eDissconnect ,

		NumFlag       ,
	};
   
	bool    isNetwork(){ return mbNetWork; }
	typedef FlagBits< NumFlag > StateFlag;
	StateFlag& getStateFlag(){ return mFlag; }

	//server to player
	virtual void sendCommand( int channel , IComPacket* cp ) = 0;
	virtual void sendTcpCommand( IComPacket* cp ) = 0;
	virtual void sendUdpCommand( IComPacket* cp ) = 0;

	long      lastUpdateFrame;
	long      latency;
	long      lastRecvFrame;
	long      lastSendFrame;
private:
	bool      mbNetWork;
	StateFlag mFlag;
};

class SNetPlayer : public ServerPlayer
{
public:
	SNetPlayer( ServerWorker* server , NetClientData* client );
	NetClientData& getClient(){  return *mClient;  }
	void  sendTcpCommand( IComPacket* cp );
	void  sendUdpCommand( IComPacket* cp );
	void  sendCommand( int channel , IComPacket* cp );
protected:
	ServerWorker*    mServer;
	NetClientData*   mClient;
};

class SLocalPlayer : public ServerPlayer
{
public:
	SLocalPlayer( ):ServerPlayer( false ){}
	void sendCommand( int channel , IComPacket* cp ){}
	void sendTcpCommand( IComPacket* cp ){}
	void sendUdpCommand( IComPacket* cp ){}
};

class SUserPlayer : public SLocalPlayer
{
public:
	SUserPlayer( LocalWorker* worker );
	void sendTcpCommand( IComPacket* cp );
	void sendUdpCommand( IComPacket* cp );
	void sendCommand( int channel , IComPacket* cp );
private:
	
	LocalWorker*  mWorker;
};


class ServerClientManager
{
public:
	ServerClientManager();
	~ServerClientManager();

	void        update( long time );
	void        sendUdpData( long time , UdpServer& server );
	NetClientData* findClient( NetAddress const& addr );
	NetClientData* findClient( SessionId id );
	NetClientData* createClient( NetSocket& socket );

	void        cleanup();

	bool        removeClient( NetClientData* client );
	void        sendTcpCommand( ComEvaluator& evaluator , IComPacket* cp );
	void        sendUdpCommand( ComEvaluator& evaluator , IComPacket* cp );

	void        setClientUdpAddr( SessionId id , NetAddress const& addr );

protected:

	SessionId   getNewSessionId();
	
	bool        doRemoveClient( SessionId id );
	void        cleanupClient( NetClientData* client );

	struct AddrCmp
	{
		bool operator()( NetAddress const* lhs , NetAddress const* rhs ) const
		{
			if ( lhs->get().sin_addr.s_addr < rhs->get().sin_addr.s_addr )
				return true;
			else if ( lhs->get().sin_addr.s_addr == rhs->get().sin_addr.s_addr &&
				      lhs->get().sin_port < rhs->get().sin_port )
				return true;

			return false;
		}
	};
	typedef std::map< NetAddress const* , NetClientData* , AddrCmp >  AddrMap;
	typedef std::map< SessionId , NetClientData* >              SessionMap;
	typedef std::vector< NetClientData* > ClientList;

	SessionId  mNextId;
	ClientList mRemoveList;
	AddrMap    mAddrMap;
	SessionMap mSessionMap;
};


class ServerPlayerListener
{
public:
	virtual void onAddPlayer( PlayerId id ) = 0;
	virtual void onRemovePlayer( PlayerInfo const& info ) = 0;
};

class SVPlayerManager : public IPlayerManager
{
public:
	SVPlayerManager();
	~SVPlayerManager();

	TINY_API size_t          getPlayerNum();
	TINY_API ServerPlayer*   getPlayer( PlayerId id );
	PlayerId       getUserID(){  return mUserID;  }

	TINY_API SNetPlayer*    createNetPlayer( ServerWorker* server , char const* name , NetClientData* client );
	TINY_API SUserPlayer*   createUserPlayer( LocalWorker* worker , char const* name );
	TINY_API SLocalPlayer*  createAIPlayer();
	SLocalPlayer*  swepNetPlayerToLocal( SNetPlayer* player );
	
	TINY_API bool  removePlayer( PlayerId id );
	TINY_API void  getPlayerInfo( PlayerInfo* info[] );
	TINY_API void  removePlayerFlag( unsigned bitPos );
	TINY_API bool  checkPlayerFlag( unsigned bitPos , bool beNet );
	TINY_API void  getPlayerFlag( int flags[] );
	TINY_API void  cleanup();

	TINY_API void sendTcpCommand( IComPacket* cp );
	TINY_API void sendCommand( int channel , IComPacket* cp , unsigned flag );
	TINY_API void sendUdpCommand( IComPacket* cp );
	void setListener( ServerPlayerListener* listener ){ mListener = listener; }

	//auto lock() { return MakeLockedObjectHandle(*this, &mMutexPlayerTable); }

protected:
	void insertPlayer( ServerPlayer* player , char const* name , PlayerType type );

	
	ServerPlayerListener* mListener;
	DEFINE_MUTEX( mMutexPlayerTable );
	typedef TTable< ServerPlayer* > PlayerTable; 
	PlayerTable  mPlayerTable;
	unsigned     mUserID;
	PlayerInfo   mPlayerInfo[ MAX_PLAYER_NUM ];
};


class LocalWorker;

enum class PlayerConnetionClosedAction
{
	WaitReconnect ,
	ChangeToLocal ,
	Remove ,
};

struct ServerResolveContext
{
	ServerResolveContext( ServerWorker& inServer , PlayerId inId )
		:server(inServer),id(inId){ }

	PlayerId id;
	template< class Func >
	void addGameCommand(Func&& func)
	{
		server.addGameThreadCommnad(std::forward<Func>(func));
	}

	ServerWorker& server;
};
class ServerEventResolver
{
public:
	//Socket Thread
	virtual PlayerConnetionClosedAction resolveConnectClosed_NetThread( ServerResolveContext& context , NetCloseReason reason) { return PlayerConnetionClosedAction::Remove;  }
	virtual void                 resolveReconnect_NetThread( ServerResolveContext& context ){}
};

class  ServerWorker : public NetWorker, public INetConnectListener
{
	typedef NetWorker BaseClass;
public:
	TINY_API ServerWorker();
	TINY_API ~ServerWorker();

	bool  isServer(){   return true;  }
	//NetWorker
	void  sendCommand( int channel , IComPacket* cp , unsigned flag );

	TINY_API LocalWorker* createLocalWorker(char const* userName);

	void sendClientTcpCommand( NetClientData& client , IComPacket* cp );
	void setEventResolver(ServerEventResolver* resolver) { mEventResolver = resolver;  }

protected:
	//NetWorker
	void  postChangeState( NetActionState oldState );
	bool  doStartNetwork();
	bool  update_NetThread( long time );
	void  doCloseNetwork();
	void  clenupNetResource();
	bool  swapServer();
	void  doUpdate( long time );


	// INetConnectListener
	virtual void notifyConnectionSend( NetConnection* con );
	virtual bool notifyConnectionRecv( NetConnection* con , SocketBuffer& buffer ,NetAddress* clientAddr );
	virtual void notifyConnectionAccpet( NetConnection* con );
	virtual void notifyConnectClose( NetConnection* con , NetCloseReason reason );

public:
	TINY_API bool kickPlayer( unsigned id );
	void removeConnect_NetThread( NetClientData* client, bool bRMPlayer = true );

	SVPlayerManager* getPlayerManager(){ return mPlayerManager; }

	void  generatePlayerStatus( SPPlayerStatus& comPS );
	/////////////////////////////////////////////////

protected:
	void procLogin      ( IComPacket* cp);
	void procEcho       ( IComPacket* cp);
	void procClockSynd  ( IComPacket* cp);
	void procUdpAddress ( IComPacket* cp);
	void procMsg        ( IComPacket* cp);
	void procPlayerState( IComPacket* cp);
	void procComMsg     ( IComPacket* cp);
	void procUdpCon     ( IComPacket* cp);

	void procClockSynd_NetThread( IComPacket* cp);
	void procComMsg_NetThread   ( IComPacket* cp);
	void procUdpCon_NetThread   ( IComPacket* cp);
	
	/////////////////////////////////////////

	TPtrHolder< SVPlayerManager >  mPlayerManager;
	TPtrHolder< LocalWorker >      mLocalWorker;

	bool                 mbEnableUDPChain;
	TcpServer            mTcpServer;
	UdpServer            mUdpServer;
	TcpClient            mGuideClient;
	ServerClientManager  mClientManager;
	ServerEventResolver* mEventResolver;
};


class LocalWorker : public ComWorker
{
public:
	IPlayerManager*  getPlayerManager()    {  return mPlayerMgr;  }

	void  sendCommand( int channel , IComPacket* cp , unsigned flag );
	void  recvCommand( IComPacket* cp );
protected:

	void  doUpdate(long time);
	void  postChangeState( NetActionState oldState );
	void  update_NetThread(long time);
protected:
	LocalWorker( ServerWorker* worker );
	void procPlayerState( IComPacket* cp);
	void procClockSynd  ( IComPacket* cp);
	friend class ServerWorker;
protected:
	DEFINE_MUTEX(mMutexBuffer);
	SocketBuffer      mSendBuffer;
	SocketBuffer      mRecvBuffer;
	SVPlayerManager*  mPlayerMgr;
	ServerWorker*     mServer;
};


#endif // GameServer_h__
