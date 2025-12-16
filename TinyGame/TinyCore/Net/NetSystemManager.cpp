#include "TinyGamePCH.h"
#include "NetSystemManager.h"
#include "NetTransportImpl.h"
#include "NetSessionImpl.h"
#include "GameServer.h"
#include "GameClient.h"

//========================================
// NetSystemManager
//========================================

static NetSystemManager* sNetSystemInstance = nullptr;

NetSystemManager::NetSystemManager()
{
	if (sNetSystemInstance == nullptr)
	{
		sNetSystemInstance = this;
	}
}

NetSystemManager::~NetSystemManager()
{
	closeNetwork();
	
	if (sNetSystemInstance == this)
	{
		sNetSystemInstance = nullptr;
	}
}

NetSystemManager& NetSystemManager::Get()
{
	if (sNetSystemInstance == nullptr)
	{
		static NetSystemManager sDefaultInstance;
	}
	return *sNetSystemInstance;
}

bool NetSystemManager::createServerSession()
{
	// 清理現有會話
	closeNetwork();
	
	// 建立傳輸層
	mServerTransport = std::make_unique<ServerTransportImpl>();
	if (!mServerTransport->startNetwork())
	{
		LogError("Failed to start server transport");
		mServerTransport.reset();
		return false;
	}
	
	// 建立會話層
	mHostSession = std::make_unique<NetSessionHostImpl>();
	if (!mHostSession->initialize(mServerTransport.get()))
	{
		LogError("Failed to initialize host session");
		mServerTransport->closeNetwork();
		mServerTransport.reset();
		mHostSession.reset();
		return false;
	}
	
	LogMsg("Server session created");
	return true;
}

bool NetSystemManager::createClientSession()
{
	// 清理現有會話
	closeNetwork();
	
	// 建立傳輸層
	mClientTransport = std::make_unique<ClientTransportImpl>();
	if (!mClientTransport->startNetwork())
	{
		LogError("Failed to start client transport");
		mClientTransport.reset();
		return false;
	}
	
	// 建立會話層
	mClientSession = std::make_unique<NetSessionClientImpl>();
	if (!mClientSession->initialize(mClientTransport.get()))
	{
		LogError("Failed to initialize client session");
		mClientTransport->closeNetwork();
		mClientTransport.reset();
		mClientSession.reset();
		return false;
	}
	
	LogMsg("Client session created");
	return true;
}

void NetSystemManager::closeNetwork()
{
	// 關閉遊戲會話
	if (mGameSession)
	{
		mGameSession->shutdown();
		mGameSession = nullptr;
	}
	
	// 關閉會話層
	if (mHostSession)
	{
		mHostSession->shutdown();
		mHostSession.reset();
	}
	if (mClientSession)
	{
		mClientSession->shutdown();
		mClientSession.reset();
	}
	
	// 關閉傳輸層
	if (mServerTransport)
	{
		mServerTransport->closeNetwork();
		mServerTransport.reset();
	}
	if (mClientTransport)
	{
		mClientTransport->closeNetwork();
		mClientTransport.reset();
	}
	
	mCompatNetWorker = nullptr;
	
	LogMsg("Network closed");
}

void NetSystemManager::update(long time)
{
	// 更新傳輸層
	if (mServerTransport)
	{
		static_cast<ServerTransportImpl*>(mServerTransport.get())->update(time);
	}
	if (mClientTransport)
	{
		static_cast<ClientTransportImpl*>(mClientTransport.get())->update(time);
	}
	
	// 更新會話層
	if (mHostSession)
	{
		mHostSession->update(time);
	}
	if (mClientSession)
	{
		mClientSession->update(time);
	}
	
	// 更新遊戲層
	if (mGameSession)
	{
		mGameSession->update(time);
	}
}

void NetSystemManager::setGameSession(IGameNetSession* gameSession)
{
	mGameSession = gameSession;
	
	if (mGameSession)
	{
		INetSession* session = getSession();
		if (session)
		{
			mGameSession->initialize(session);
		}
	}
}

bool NetSystemManager::createGameSession(IGameNetSessionFactory* factory)
{
	if (!factory || !factory->supportsNetGame())
		return false;
	
	IGameNetSession* gameSession = factory->createGameNetSession();
	if (!gameSession)
		return false;
	
	setGameSession(gameSession);
	return true;
}

NetWorker* NetSystemManager::buildNetworkCompat(bool beServer)
{
	// 使用舊系統建立 NetWorker
	// 這是為了向後相容
	
	// TODO: 可以選擇：
	// 1. 真的建立舊的 NetWorker
	// 2. 建立新系統，然後包裝成 NetWorker 介面
	
	return nullptr;
}

NetWorker* NetSystemManager::getNetWorkerCompat()
{
	return mCompatNetWorker;
}

//========================================
// GameNetInterfaceEx
//========================================

GameNetInterfaceEx::GameNetInterfaceEx()
{
}

GameNetInterfaceEx::~GameNetInterfaceEx()
{
	closeNetwork();
}

bool GameNetInterfaceEx::haveServer()
{
	if (mUsingLegacySystem)
	{
		return mLegacyNetWorker && mLegacyNetWorker->isServer();
	}
	return mNetSystem.isServer();
}

NetWorker* GameNetInterfaceEx::getNetWorker()
{
	if (mUsingLegacySystem)
	{
		return mLegacyNetWorker.get();
	}
	return mNetSystem.getNetWorkerCompat();
}

NetWorker* GameNetInterfaceEx::buildNetwork(bool beServer)
{
	closeNetwork();
	
	// 使用舊系統
	mUsingLegacySystem = true;
	
	if (beServer)
	{
		mLegacyNetWorker = std::make_unique<ServerWorker>();
	}
	else
	{
		mLegacyNetWorker = std::make_unique<ClientWorker>();
	}
	
	if (!mLegacyNetWorker->startNetwork())
	{
		mLegacyNetWorker.reset();
		return nullptr;
	}
	
	return mLegacyNetWorker.get();
}

void GameNetInterfaceEx::closeNetwork()
{
	mNetSystem.closeNetwork();
	
	if (mLegacyNetWorker)
	{
		mLegacyNetWorker->closeNetwork();
		mLegacyNetWorker.reset();
	}
	
	mUsingLegacySystem = false;
}

bool GameNetInterfaceEx::createServer()
{
	closeNetwork();
	mUsingLegacySystem = false;
	return mNetSystem.createServerSession();
}

bool GameNetInterfaceEx::createClient()
{
	closeNetwork();
	mUsingLegacySystem = false;
	return mNetSystem.createClientSession();
}

//========================================
// GlobalEx
//========================================

static GameNetInterfaceEx* sGameNetEx = nullptr;

namespace GlobalEx
{
	NetSystemManager& NetSystem()
	{
		return GameNetEx().getNetSystem();
	}
	
	GameNetInterfaceEx& GameNetEx()
	{
		if (sGameNetEx == nullptr)
		{
			static GameNetInterfaceEx sInstance;
			sGameNetEx = &sInstance;
		}
		return *sGameNetEx;
	}
}
