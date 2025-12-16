#include "TinyGamePCH.h"
#include "NetStageController.h"
#include "GameNetPacket.h"

//=============================================================================
// NetStageController Implementation
//=============================================================================

NetStageController::NetStageController()
{
}

NetStageController::~NetStageController()
{
	cleanup();
}

void NetStageController::init(ComWorker* worker, ServerWorker* server)
{
	mWorker = worker;
	mServer = server;
	mCloseNetworkOnDestroy = true;
	
	registerNetEvent();
}

void NetStageController::cleanup()
{
	if (mRegistered)
	{
		unregisterNetEvent();
	}
	
	if (mCloseNetworkOnDestroy && mWorker)
	{
		if (NetWorker* netWorker = dynamic_cast<NetWorker*>(mWorker))
		{
			netWorker->closeNetwork();
		}
	}
	
	mWorker = nullptr;
	mServer = nullptr;
	mCloseNetworkOnDestroy = false;
}

ClientWorker* NetStageController::getClientWorker()
{
	if (mServer != nullptr)
		return nullptr;
	return static_cast<ClientWorker*>(mWorker);
}

void NetStageController::disconnect()
{
	if (mServer)
	{
		mServer->changeState(NAS_DISSCONNECT);
	}
	else if (mWorker)
	{
		CSPPlayerState com;
		com.state = NAS_DISSCONNECT;
		com.playerId = mWorker->getPlayerManager()->getUserID();
		mWorker->sendTcpCommand(&com);
	}
	mCloseNetworkOnDestroy = false;
}

bool NetStageController::sendTcpCommand(IComPacket* cp)
{
	if (!mWorker)
		return false;
	return mWorker->sendTcpCommand(cp);
}

bool NetStageController::sendUdpCommand(IComPacket* cp)
{
	if (!mWorker)
		return false;
	return mWorker->sendUdpCommand(cp);
}

bool NetStageController::sendCommand(int channel, IComPacket* cp)
{
	if (!mWorker)
		return false;
	return mWorker->sendCommand(channel, cp, WSF_NONE);
}

void NetStageController::registerNetEvent()
{
	if (mRegistered)
		return;
		
	ClientWorker* clientWorker = getClientWorker();
	if (clientWorker)
	{
		clientWorker->setClientListener(this);
	}
	
	mRegistered = true;
}

void NetStageController::unregisterNetEvent()
{
	if (!mRegistered)
		return;
		
	ClientWorker* clientWorker = getClientWorker();
	if (clientWorker)
	{
		clientWorker->setClientListener(nullptr);
	}
	
	// Unregister packet handlers
	if (mWorker)
	{
		mWorker->getEvaluator().removeProcesserFunc(this);
	}
	if (mServer)
	{
		mServer->getEvaluator().removeProcesserFunc(this);
	}
	
	mRegistered = false;
}

void NetStageController::onServerEvent(EventID event, unsigned msg)
{
	// Forward to callback if set
	if (onServerEventCallback)
	{
		onServerEventCallback(event, msg);
	}
}
