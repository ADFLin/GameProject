#include "TinyGamePCH.h"
#include "NetSessionAdapter.h"
#include "GameNetPacket.h"

//========================================
// NetSessionAdapter
//========================================

NetSessionAdapter::NetSessionAdapter()
{
}

NetSessionAdapter::~NetSessionAdapter()
{
	shutdown();
}

void NetSessionAdapter::initFromWorker(ComWorker* worker, ServerWorker* server)
{
	mWorker = worker;
	mServer = server;
	mSession = nullptr;
	mIsHost = (server != nullptr);
}

void NetSessionAdapter::initFromSession(INetSession* session)
{
	mSession = session;
	mWorker = nullptr;
	mServer = nullptr;
	mIsHost = dynamic_cast<INetSessionHost*>(session) != nullptr;
}

void NetSessionAdapter::shutdown()
{
	mWorker = nullptr;
	mServer = nullptr;
	mSession = nullptr;
}

ENetSessionState NetSessionAdapter::getSessionState() const
{
	if (mSession)
	{
		return mSession->getState();
	}
	else if (mWorker)
	{
		return toSessionState(mWorker->getActionState());
	}
	return ENetSessionState::Disconnected;
}

void NetSessionAdapter::setSessionState(ENetSessionState state)
{
	if (mSession)
	{
		mSession->changeState(state);
	}
	else if (mWorker)
	{
		mWorker->changeState(toNetActionState(state));
	}
}

IPlayerManager* NetSessionAdapter::getPlayerManager()
{
	if (mSession)
	{
		return mSession->getPlayerManager();
	}
	else if (mWorker)
	{
		return mWorker->getPlayerManager();
	}
	return nullptr;
}

PlayerId NetSessionAdapter::getLocalPlayerId() const
{
	if (mSession)
	{
		return mSession->getUserPlayerId();
	}
	else if (mWorker)
	{
		return mWorker->getPlayerManager()->getUserID();
	}
	return ERROR_PLAYER_ID;
}

void NetSessionAdapter::sendReliable(IComPacket* cp)
{
	if (mSession)
	{
		mSession->sendReliable(cp);
	}
	else if (mServer)
	{
		mServer->sendTcpCommand(cp);
	}
	else if (mWorker)
	{
		mWorker->sendTcpCommand(cp);
	}
}

void NetSessionAdapter::sendUnreliable(IComPacket* cp)
{
	if (mSession)
	{
		mSession->sendUnreliable(cp);
	}
	else if (mServer)
	{
		mServer->sendUdpCommand(cp);
	}
	else if (mWorker)
	{
		mWorker->sendUdpCommand(cp);
	}
}

void NetSessionAdapter::sendReliableTo(PlayerId targetId, IComPacket* cp)
{
	if (mSession)
	{
		mSession->sendReliableTo(targetId, cp);
	}
	else if (mServer)
	{
		ServerPlayer* player = mServer->getPlayerManager()->getPlayer(targetId);
		if (player)
		{
			player->sendTcpCommand(cp);
		}
	}
}

void NetSessionAdapter::broadcastReliable(IComPacket* cp)
{
	if (mSession)
	{
		if (INetSessionHost* host = dynamic_cast<INetSessionHost*>(mSession))
		{
			host->broadcastReliable(cp);
		}
	}
	else if (mServer)
	{
		mServer->sendTcpCommand(cp);
	}
}

void NetSessionAdapter::registerPacketHandler(ComID packetId, PacketHandler handler)
{
	if (mSession)
	{
		mSession->registerPacketHandler(packetId, 
			[handler](PlayerId sender, IComPacket* cp) {
				handler(cp);
			});
	}
	else if (mWorker)
	{
		// 舊系統使用 ComEvaluator
		// 這裡需要額外的適配邏輯
		// TODO: 實作 ComEvaluator 適配
	}
}

void NetSessionAdapter::unregisterPacketHandler(ComID packetId)
{
	if (mSession)
	{
		mSession->unregisterPacketHandler(packetId);
	}
}

//========================================
// NetStageDataEx
//========================================

NetStageDataEx::NetStageDataEx()
{
}

NetStageDataEx::~NetStageDataEx()
{
	if (mCloseNetworkOnDestroy)
	{
		::Global::GameNet().closeNetwork();
	}
}

void NetStageDataEx::initWorker(ComWorker* worker, ServerWorker* server)
{
	mAdapter.initFromWorker(worker, server);
	
	// 設定 ClientListener（向後相容）
	if (!server && worker)
	{
		ClientWorker* client = static_cast<ClientWorker*>(worker);
		client->setClientListener(this);
	}
	
	// 註冊封包處理
	setupWorkerProcFunc();
	if (server)
	{
		setupServerProcFunc();
	}
}

void NetStageDataEx::initSession(INetSession* session)
{
	mAdapter.initFromSession(session);
	
	// 設定事件處理
	mAdapter.onSessionEvent = [this](ENetSessionEvent event, PlayerId playerId) {
		// 轉換為舊的事件格式
		switch (event)
		{
		case ENetSessionEvent::ConnectionLost:
			onServerEvent(ClientListener::eCON_CLOSE, 0);
			break;
		case ENetSessionEvent::LoginSuccess:
			onServerEvent(ClientListener::eLOGIN_RESULT, 1);
			break;
		case ENetSessionEvent::LoginFailed:
			onServerEvent(ClientListener::eLOGIN_RESULT, 0);
			break;
		case ENetSessionEvent::PlayerJoined:
			onServerEvent(ClientListener::ePLAYER_ADD, playerId);
			break;
		case ENetSessionEvent::PlayerLeft:
			onServerEvent(ClientListener::ePLAYER_REMOVE, playerId);
			break;
		}
	};
	
	// 註冊封包處理
	setupWorkerProcFunc();
	if (mAdapter.isHost())
	{
		setupServerProcFunc();
	}
}

ClientWorker* NetStageDataEx::getClientWorker()
{
	return mAdapter.getClient();
}

void NetStageDataEx::disconnect()
{
	if (mAdapter.isHost())
	{
		mAdapter.setSessionState(ENetSessionState::Disconnected);
		mCloseNetworkOnDestroy = true;
	}
	else
	{
		CSPPlayerState com;
		com.state = NAS_DISSCONNECT;
		com.playerId = mAdapter.getLocalPlayerId();
		sendReliable(&com);
	}
}

void NetStageDataEx::unregisterNetEvent()
{
	// 清理封包處理器
	// TODO: 記錄已註冊的封包 ID 並逐一移除
	
	// 清理 ClientListener
	if (ClientWorker* client = getClientWorker())
	{
		client->setClientListener(nullptr);
	}
}
