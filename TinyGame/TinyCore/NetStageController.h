#pragma once
#ifndef NetStageController_H_8A3B2C1D_4E5F_6789_ABCD_EF0123456789
#define NetStageController_H_8A3B2C1D_4E5F_6789_ABCD_EF0123456789

#include "GameWorker.h"
#include "GameServer.h"
#include "GameClient.h"
#include "ComPacket.h"

class ComWorker;
class ServerWorker;
class ClientWorker;
class ComEvaluator;
class IComPacket;

//=============================================================================
// NetStageController - Composition-based network stage management
//=============================================================================
// This class provides network functionality as a composable component,
// replacing the inheritance-based NetStageData approach.
//
// Usage:
//   class MyNetStage : public StageBase
//   {
//       NetStageController mNetController;
//   public:
//       bool onInit() override {
//           mNetController.init(worker, server);
//           return true;
//       }
//   };
//=============================================================================

class NetStageController : public ClientListener
{
public:
	NetStageController();
	~NetStageController();

	//-------------------------------------------------------------------------
	// Initialization
	//-------------------------------------------------------------------------
	
	// Initialize with worker and optional server
	void init(ComWorker* worker, ServerWorker* server = nullptr);
	
	// Cleanup network resources
	void cleanup();

	//-------------------------------------------------------------------------
	// State Queries
	//-------------------------------------------------------------------------
	
	bool isServer() const { return mServer != nullptr; }
	bool isClient() const { return mServer == nullptr && mWorker != nullptr; }
	bool isInitialized() const { return mWorker != nullptr; }
	
	ComWorker* getWorker() { return mWorker; }
	ServerWorker* getServer() { return mServer; }
	ClientWorker* getClientWorker();
	
	//-------------------------------------------------------------------------
	// Network Operations
	//-------------------------------------------------------------------------
	
	// Disconnect from server/clients
	void disconnect();
	
	// Send commands
	bool sendTcpCommand(IComPacket* cp);
	bool sendUdpCommand(IComPacket* cp);
	bool sendCommand(int channel, IComPacket* cp);

	//-------------------------------------------------------------------------
	// Event Callbacks (set by owner)
	//-------------------------------------------------------------------------
	
	// Called when a server event occurs (client-side)
	// Use this callback to receive events from the network
	std::function<void(ClientListener::EventID, unsigned)> onServerEventCallback;
	
	// Called when network state changes
	std::function<void(NetActionState)> onStateChanged;

protected:
	// ClientListener interface - forwards to callback
	virtual void onServerEvent(EventID event, unsigned msg) override;

private:
	void registerNetEvent();
	void unregisterNetEvent();

	ComWorker*    mWorker = nullptr;
	ServerWorker* mServer = nullptr;
	bool          mCloseNetworkOnDestroy = false;
	bool          mRegistered = false;
};

#endif // NetStageController_H_8A3B2C1D_4E5F_6789_ABCD_EF0123456789
