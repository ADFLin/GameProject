#pragma once
#ifndef DedicatedServerMode_H_INCLUDED
#define DedicatedServerMode_H_INCLUDED

#include "NetGameMode.h"

/**
 * DedicatedServerMode - Headless Server Mode for Dedicated Server Execution
 * 
 * Inherits from NetGameMode to reuse network infrastructure.
 * Key differences from NetGameMode:
 *   1. No rendering - canRender() always returns false
 *   2. No UI - skips all UI-related code
 *   3. Internal lobby management - waits for players, countdown, auto-start
 *   4. Direct game control - calls INetEngine::onGameStart()
 * 
 * State Machine:
 *   WaitingForPlayers -> Countdown -> Running -> Finished
 */

class INetEngine;
class IGameModule;

class DedicatedServerMode : public NetGameMode
{
	typedef NetGameMode BaseClass;
public:
	
	// Server state machine
	enum class EServerState
	{
		Initializing,      // Setting up server
		WaitingForPlayers, // Waiting for minimum players to join
		Countdown,         // Countdown before game start
		Running,           // Game is running
		Finished,          // Game ended, can restart or shutdown
	};

	// Configuration
	struct Config
	{
		int minPlayers = 2;           // Minimum players to start
		int maxPlayers = 8;           // Maximum players allowed
		float countdownSeconds = 3.0f; // Countdown time before start
		bool autoRestart = true;       // Auto restart after game ends
		bool headless = true;          // No rendering
		char const* gameName = nullptr; // Game module name
	};

	DedicatedServerMode();
	~DedicatedServerMode();

	// Configuration
	void configure(Config const& config);
	Config const& getConfig() const { return mConfig; }

	// State
	EServerState getServerState() const { return mServerState; }
	float getCountdownRemaining() const { return mCountdownTimer; }
	int getCurrentPlayerCount();
	
	// Manual controls
	bool forceStartGame();
	void requestShutdown();

	// === GameModeBase overrides ===
	bool initialize() override;
	bool initializeStage(GameStageBase* stage) override;
	void onEnd() override;
	void updateTime(GameTimeSpan deltaTime) override;
	bool canRender() override { return true; }
	
	// No UI interaction needed
	MsgReply onKey(KeyMsg const& msg) override { return MsgReply::Unhandled(); }
	bool onWidgetEvent(int event, int id, GWidget* ui) override { return true; }

	// === NetGameMode overrides ===
	// Override to skip UI-related initialization
	void onServerEvent(ClientListener::EventID event, unsigned msg) override;

	bool initializeServer(GameStageBase* stage);
protected:
	// Internal state management
	void transitionTo(EServerState newState);
	void updateWaitingState(float dt);
	void updateCountdownState(float dt);
	void updateRunningState(float dt);
	void updateFinishedState(float dt);
	
	// Game control


	void startGameSession();
	void endGameSession();
	void broadcastServerState();

	// ServerPlayerListener overrides
	void onAddPlayer(PlayerId id) override;
	void onRemovePlayer(PlayerInfo const& info) override;
	
	SlotId findAvailableSlot();
	
	// Logging
	void logState(char const* format, ...);

	// UI
	void createServerStatusUI();
	void updateServerStatusUI();
	void destroyServerStatusUI();

private:
	Config mConfig;
	EServerState mServerState = EServerState::Initializing;
	float mCountdownTimer = 0.0f;
	bool mShutdownRequested = false;
	int64 mStateStartTime = 0;

	// UI (only used when headless=false)
	class ServerStatusPanel* mStatusPanel = nullptr;
};

#endif // DedicatedServerMode_H_INCLUDED

