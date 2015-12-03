#ifndef GameServer_h__
#define GameServer_h__

#include "GameWorker.h"
#include "GamePlayer.h"

#include "Flag.h"
#include "THolder.h"

class LocalWorker;
class SNetPlayer;
class ServerWorker;

class SPPlayerStatus;

struct ClientInfo : public ComConnection
{
	typedef UdpServer::Client UdpClient;

	class TCPClient : public TcpServer::Client
	{
	public:
		TCPClient( TSocket& socket )
		{
			getSocket().move( socket );
		}
		ClientInfo* clientInfo;
	};


	ClientInfo( TSocket& socket )
		:tcpClient( socket )
	{
		tcpClient.clientInfo = this;
	}

	SessionId    id;
	TCPClient    tcpClient;
	UdpClient    udpClient;
	NetAddress   udpAddr;
	SNetPlayer*  player;
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
		eLevelSync   ,
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
	SNetPlayer( ServerWorker* server , ClientInfo* cInfo );
	ClientInfo& getClientInfo(){  return *mClientInfo;  }
	void  sendTcpCommand( IComPacket* cp );
	void  sendUdpCommand( IComPacket* cp );
	void  sendCommand( int channel , IComPacket* cp );
protected:
	ServerWorker* mServer;
	ClientInfo*   mClientInfo;
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

	void        updateNet( long time );
	void        sendUdpData( long time , UdpServer& server );
	ClientInfo* findClient( NetAddress const& addr );
	ClientInfo* findClient( SessionId id );
	ClientInfo* createClient( TSocket& socket );

	void        removeAllClient(){  cleanup();  }
	bool        removeClient( ClientInfo* info );
	void        sendTcpCommand( ComEvaluator& evaluator , IComPacket* cp );
	void        sendUdpCommand( ComEvaluator& evaluator , IComPacket* cp );

	void        setClientUdpAddr( SessionId id , NetAddress const& addr );

protected:

	SessionId   getNewSessionId();
	void        cleanup();
	bool        doRemoveClient( SessionId id );
	void        cleanupClient( ClientInfo* info );

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
	typedef std::map< NetAddress const* , ClientInfo* , AddrCmp >  AddrMap;
	typedef std::map< SessionId , ClientInfo* >              SessionMap;
	typedef std::list< ClientInfo* > ClientList;

	SessionId  mNextId;
	ClientList mRemoveList;
	DEFINE_MUTEX( mMutexClientMap )
	AddrMap    mAddrMap;
	SessionMap mSessionMap;
};


class PlayerListener
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

	size_t         getPlayerNum();
	ServerPlayer*  getPlayer( PlayerId id );
	PlayerId       getUserID(){  return mUserID;  }

	GAME_API SNetPlayer*    createNetPlayer( ServerWorker* server , char const* name , ClientInfo* client );
	GAME_API SUserPlayer*   createUserPlayer( LocalWorker* worker , UserProfile& profile );
	GAME_API SLocalPlayer*  createAIPlayer();
	SLocalPlayer*  swepNetPlayerToLocal( SNetPlayer* player );
	
	bool  removePlayer( PlayerId id );
	void  getPlayerInfo( PlayerInfo* info[] );
	void  removePlayerFlag( unsigned bitPos );
	GAME_API bool  checkPlayerFlag( unsigned bitPos , bool beNet );
	void  getPlayerFlag( int flag[] );
	void  cleanup();

	void sendTcpCommand( IComPacket* cp );
	void sendCommand( int channel , IComPacket* cp , unsigned flag );
	void sendUdpCommand( IComPacket* cp );
	void setListener( PlayerListener* listener ){ mListener = listener; }

protected:
	void insertPlayer( ServerPlayer* player , char const* name , PlayerType type );

	
	PlayerListener* mListener;
	DEFINE_MUTEX( mMutexPlayerTable );
	typedef TTable< ServerPlayer* > PlayerTable; 
	PlayerTable  mPlayerTable;
	unsigned     mUserID;
	PlayerInfo   mPlayerInfo[ MAX_PLAYER_NUM ];
};


class LocalWorker;

class  ServerWorker : public NetWorker
{
	typedef NetWorker BaseClass;
public:
	GAME_API ServerWorker();
	~ServerWorker();

	bool  isServer(){   return true;  }
	//NetWorker
	void  sendCommand( int channel , IComPacket* cp , unsigned flag )
	{
		mPlayerManager->sendCommand( channel , cp , flag );
	}

	GAME_API LocalWorker* createLocalWorker( UserProfile& profile );

	void sendClientTcpCommand( ClientInfo& client , IComPacket* cp );
protected:
	//NetWorker
	void  postChangeState( NetActionState oldState );
	bool  doStartNetwork();
	bool  updateSocket( long time );
	void  doCloseNetwork();
	bool  swapServer();
	void  doUpdate( long time );



	virtual void onSendData( Connection* con );
	virtual bool onRecvData( Connection* con , SBuffer& buffer ,NetAddress* clientAddr );
	virtual void onAccpetClient( Connection* con );
	virtual void onClose( Connection* con , ConCloseReason reason );

public:
	bool kickPlayer( unsigned id );
	void removeConnect( ClientInfo* info , bool bRMPlayer = true );
	SVPlayerManager* getPlayerManager(){ return mPlayerManager; }

	void  generatePlayerStatus( SPPlayerStatus& comPS );
	/////////////////////////////////////////////////

protected:
	void procLogin      ( IComPacket* cp );
	void procEcho       ( IComPacket* cp );
	void procClockSynd  ( IComPacket* cp );
	void procUdpAddress ( IComPacket* cp );
	void procMsg        ( IComPacket* cp );
	void procPlayerState( IComPacket* cp );
	void procComMsg     ( IComPacket* cp );
	void procUdpCon      ( IComPacket* cp );

	void procEchoNet     ( IComPacket* cp );
	void procClockSyndNet( IComPacket* cp );
	void procComMsgNet   ( IComPacket* cp );
	void procUdpConNet   ( IComPacket* cp );
	
	/////////////////////////////////////////

	TPtrHolder< SVPlayerManager >  mPlayerManager;
	TPtrHolder< LocalWorker >      mLocalWorker;

	bool                 mbEnableUDPChain;
	TcpServer            mTcpServer;
	UdpServer            mUdpServer;
	TcpClient            mGuideClient;
	ServerClientManager  mClientManager;
	NetAddress*          mSendAddr;
};


class LocalWorker : public ComWorker
{
public:
	IPlayerManager*  getPlayerManager()    {  return mPlayerMgr;  }

	void  sendCommand( int channel , IComPacket* cp , unsigned flag )
	{  
		::FillBufferByCom( mServer->getEvaluator() , mSendBuffer , cp );  
	}
	void  recvCommand( IComPacket* cp )
	{  
		::FillBufferByCom( getEvaluator() , mRecvBuffer , cp );  
	}
protected:

	void  doUpdate( long time );
	void  postChangeState( NetActionState oldState );
protected:
	LocalWorker( ServerWorker* worker );
	void procPlayerState( IComPacket* cp );
	void procClockSynd  ( IComPacket* cp );
	friend class ServerWorker;
protected:

	SBuffer           mSendBuffer;
	SBuffer           mRecvBuffer;
	SVPlayerManager*  mPlayerMgr;
	ServerWorker*     mServer;
};


#endif // GameServer_h__
