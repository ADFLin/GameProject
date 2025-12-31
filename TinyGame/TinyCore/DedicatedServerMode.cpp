#include "TinyGamePCH.h"
#include "DedicatedServerMode.h"

#include "GameStage.h"
#include "GameWorker.h"
#include "GameServer.h"
#include "GameModuleManager.h"
#include "INetEngine.h"
#include "GameSettingHelper.h"

#include "GameNetPacket.h"
#include "SystemPlatform.h"

#include "ServerStatusPanel.h"
#include "GameWidget.h"
#include "GameGUISystem.h"

#include <cstdarg>


DedicatedServerMode::DedicatedServerMode()
	: NetGameMode()
{
	LogMsg("[DedicatedServerMode] Created");
}

DedicatedServerMode::~DedicatedServerMode()
{
	LogMsg("[DedicatedServerMode] Destroyed");
}

void DedicatedServerMode::configure(Config const& config)
{
	mConfig = config;
	LogMsg("[DedicatedServerMode] Configured: minPlayers=%d, maxPlayers=%d, countdown=%.1f, autoRestart=%s",
		mConfig.minPlayers, mConfig.maxPlayers, mConfig.countdownSeconds,
		mConfig.autoRestart ? "true" : "false");
}

int DedicatedServerMode::getCurrentPlayerCount()
{
	if (!haveServer())
		return 0;
	
	int count = 0;
	SVPlayerManager* playerMgr = getServer()->getPlayerManager();
	for (auto iter = playerMgr->createIterator(); iter; ++iter)
	{
		++count;
	}
	return count;
}

bool DedicatedServerMode::forceStartGame()
{
	if (mServerState == EServerState::WaitingForPlayers || 
	    mServerState == EServerState::Countdown)
	{
		logState("Force starting game");
		transitionTo(EServerState::Running);
		return true;
	}
	return false;
}

void DedicatedServerMode::requestShutdown()
{
	mShutdownRequested = true;
	logState("Shutdown requested");
}


bool DedicatedServerMode::initializeServer(GameStageBase* stage)
{
	logState("Initializing Server Stage");

	// Game module and helper setup is now done in initialize()
	// Here we focus on stage-specific initialization

	CHECK(stage);

	// Stage-based mode: Stage provides IFrameUpdater and World
	logState("Using Stage-based mode");
		
	mUpdater = stage;
	stage->setupStageMode(this);

	// Initialize the stage
	if (!stage->onInit())
	{
		LogError("[DedicatedServerMode] Stage init failed");
		return false;
	}


	// Register server packet handlers
	setupServerProcFunc(getServer()->getPacketDispatcher());
	if (getWorker())
	{
		setupWorkerProcFunc(getWorker()->getPacketDispatcher());
	}

	// Create status UI (if not headless)
	createServerStatusUI();

	// Transition to waiting state
	transitionTo(EServerState::WaitingForPlayers);

	logState("Server initialized successfully");
	return true;
}

bool DedicatedServerMode::initialize()
{
	if (!NetGameMode::initialize())
		return false;

	logState("Initializing DedicatedServerMode");
	
	// Mode-specific initialization (independent of Stage)
	// Verify server worker is available
	if (!haveServer())
	{
		LogError("[DedicatedServerMode] No server worker available");
		return false;
	}

	// Setup server event resolver and player listener
	getServer()->setEventResolver(this);
	getServer()->getPlayerManager()->setListener(this);

	// Load game module if specified
	IGameModule* game = nullptr;
	if (mConfig.gameName)
	{
		game = Global::ModuleManager().changeGame(mConfig.gameName);
		if (!game)
		{
			LogError("[DedicatedServerMode] Failed to load game: %s", mConfig.gameName);
			return false;
		}

		// Create setting helper for the game (lobby settings)
		setupGame(mConfig.gameName);
		if (mHelper)
		{
			mHelper->setupSetting(getServer());
		}
	}
	else
	{
		game = Global::ModuleManager().getRunningGame();
		if (!game)
		{
			LogError("[DedicatedServerMode] No game module available");
			return false;
		}
	}
	
	return true;
}

bool DedicatedServerMode::initializeStage(GameStageBase* stage)
{
	return initializeServer(stage);
}


void DedicatedServerMode::onEnd()
{
	logState("Ending dedicated server mode");
	
	destroyServerStatusUI();
	
	if (haveServer())
	{
		getServer()->getPlayerManager()->setListener(nullptr);
		getServer()->setEventResolver(nullptr);
	}
	
	BaseClass::onEnd();
}

void DedicatedServerMode::updateTime(GameTimeSpan deltaTime)
{
	if (mShutdownRequested)
	{
		logState("Processing shutdown");
		disconnect();
		return;
	}
	
	float dt = deltaTime.value;
	
	// Update UI
	updateServerStatusUI();
	
	switch (mServerState)
	{
	case EServerState::Initializing:
		// Should not be in this state during update
		break;
		
	case EServerState::WaitingForPlayers:
		updateWaitingState(dt);
		break;
		
	case EServerState::Countdown:
		updateCountdownState(dt);
		break;
		
	case EServerState::Running:
		updateRunningState(dt);
		break;
		
	case EServerState::Finished:
		updateFinishedState(dt);
		break;
	}
	
	// Call parent update for network processing (but NOT game tick - that's in Running state)
	if (mServerState != EServerState::Running)
	{
		// Only update network in non-running states
		// In Running state, mNetEngine->update() handles everything
	}
}

void DedicatedServerMode::updateWaitingState(float dt)
{
	int playerCount = getCurrentPlayerCount();
	
	if (playerCount >= mConfig.minPlayers)
	{
		logState("Minimum players reached (%d/%d), starting countdown",
			playerCount, mConfig.minPlayers);
		transitionTo(EServerState::Countdown);
	}
}

void DedicatedServerMode::updateCountdownState(float dt)
{
	mCountdownTimer -= dt;
	
	// Log countdown every second
	static int lastSecond = -1;
	int currentSecond = (int)mCountdownTimer;
	if (currentSecond != lastSecond && currentSecond >= 0)
	{
		lastSecond = currentSecond;
		logState("Game starting in %d...", currentSecond + 1);
		
		// Broadcast countdown message to players
		if (haveServer())
		{
			CSPMsg msg;
			msg.type = CSPMsg::eSERVER;
			msg.content.format("Game starting in %d seconds...", currentSecond + 1);
			getServer()->sendTcpCommand(&msg);
		}
	}
	
	// Check if a player left during countdown
	int playerCount = getCurrentPlayerCount();
	if (playerCount < mConfig.minPlayers)
	{
		logState("Not enough players (%d/%d), returning to waiting",
			playerCount, mConfig.minPlayers);
		transitionTo(EServerState::WaitingForPlayers);
		return;
	}
	
	if (mCountdownTimer <= 0)
	{
		startGameSession();
		transitionTo(EServerState::Running);
	}
}

void DedicatedServerMode::updateRunningState(float dt)
{
	if (getGameState() != EGameState::Run)
	{



	}
	// Update the net engine which handles game simulation
	// In dedicated server mode, the engine provides its own IFrameUpdater
	if (mNetEngine)
	{
		mNetEngine->update(*this, long(dt * 1000));
	}
	
	// Check if game has ended
	if (getGameState() == EGameState::End)
	{
		logState("Game ended");
		transitionTo(EServerState::Finished);
	}
}

void DedicatedServerMode::updateFinishedState(float dt)
{
	if (mConfig.autoRestart)
	{
		endGameSession();
		transitionTo(EServerState::WaitingForPlayers);
	}
}

void DedicatedServerMode::transitionTo(EServerState newState)
{
	if (mServerState == newState)
		return;
	
	char const* stateNames[] = {
		"Initializing", "WaitingForPlayers", "Countdown", "Running", "Finished"
	};
	
	logState("State transition: %s -> %s",
		stateNames[(int)mServerState], stateNames[(int)newState]);
	
	EServerState oldState = mServerState;
	mServerState = newState;
	mStateStartTime = SystemPlatform::GetTickCount();
	
	// State-specific transitions
	switch (newState)
	{
	case EServerState::WaitingForPlayers:
		logState("Waiting for %d players to join...", mConfig.minPlayers);
		broadcastServerState();
		break;
		
	case EServerState::Countdown:
		mCountdownTimer = mConfig.countdownSeconds;
		broadcastServerState();
		break;
		
	case EServerState::Running:
		broadcastServerState();
		break;
		
	case EServerState::Finished:
		broadcastServerState();
		break;
		
	default:
		break;
	}
}

void DedicatedServerMode::startGameSession()
{
	logState("Starting game session");
	
	// Send player status to all clients
	mHelper->sendPlayerStatusSV();
	
	// Notify clients to transition to level (NAS_LEVEL_SETUP)
	// This triggers clients to call transitionToLevel()
	{
		CSPPlayerState com;
		com.setServerState(NAS_LEVEL_SETUP);
		getServer()->sendTcpCommand(&com);
		logState("Sent NAS_LEVEL_SETUP to clients");
	}
	
	// Server-side: start game directly (no stage, headless mode)
	Global::ModuleManager().getRunningGame()->beginPlay(*getManager(), EGameMode::DedicatedServer);
	
	// Change to run state
	changeState(EGameState::Run);
}

void DedicatedServerMode::endGameSession()
{
	logState("Ending game session");
	
	if (mNetEngine)
	{
		mNetEngine->restart();
	}
	
	// Reset game state
	changeState(EGameState::Start);
}

void DedicatedServerMode::broadcastServerState()
{
	if (!haveServer())
		return;
	
	// Broadcast server state message
	CSPMsg msg;
	msg.type = CSPMsg::eSERVER;
	
	switch (mServerState)
	{
	case EServerState::WaitingForPlayers:
		msg.content.format("Waiting for players (%d/%d)...",
			getCurrentPlayerCount(), mConfig.minPlayers);
		break;
	case EServerState::Countdown:
		msg.content.format("Game starting in %.0f seconds...", mCountdownTimer);
		break;
	case EServerState::Running:
		msg.content = "Game started!";
		break;
	case EServerState::Finished:
		msg.content = "Game finished.";
		break;
	default:
		return;
	}
	
	getServer()->sendTcpCommand(&msg);
}

void DedicatedServerMode::onServerEvent(ClientListener::EventID event, unsigned msg)
{
	// Minimal event handling for headless mode
	switch (event)
	{
	case ClientListener::eCON_CLOSE:
		logState("Connection closed");
		break;
	default:
		break;
	}
}

void DedicatedServerMode::logState(char const* format, ...)
{
	char buffer[512];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	
	int64 uptime = (SystemPlatform::GetTickCount() - mStateStartTime) / 1000;
	LogMsg("[DedicatedServer %llds] %s", uptime, buffer);
}

void DedicatedServerMode::onAddPlayer(PlayerId id)
{
	logState("Player added: %d", id);
	
	if (!haveServer())
		return;
	
	// Get the player and assign a slot
	GamePlayer* player = getServer()->getPlayerManager()->getPlayer(id);
	if (player)
	{
		// Find next available slot
		SlotId slotId = findAvailableSlot();
		if (slotId != ERROR_SLOT_ID)
		{
			player->setSlot(slotId);
			player->setActionPort(slotId);  // ActionPort typically matches slot
			logState("Player %d assigned to slot %d", id, slotId);
			
			// Send player status update to all clients
			SPPlayerStatus PSCom;
			getServer()->generatePlayerStatus(PSCom);
			getServer()->sendTcpCommand(&PSCom);

			sendGameSetting(id);
		}
		else
		{
			logState("No available slot for player %d", id);
		}
	}
	
	// Update UI
	updateServerStatusUI();
}

void DedicatedServerMode::onRemovePlayer(PlayerInfo const& info)
{
	logState("Player removed: %s (slot %d)", info.name, info.slot);
	
	// Update UI
	updateServerStatusUI();
}

void DedicatedServerMode::createServerStatusUI()
{
	// Create server status panel
	Vec2i panelSize(300, 400);
	Vec2i panelPos(10, 10);
	
	mStatusPanel = new ServerStatusPanel(panelPos, panelSize, nullptr);
	mStatusPanel->setGameName(mConfig.gameName ? mConfig.gameName : "Unknown");
	mStatusPanel->setServerState("Initializing");
	
	::Global::GUI().addWidget(mStatusPanel);
}

void DedicatedServerMode::updateServerStatusUI()
{
	if (!mStatusPanel)
		return;
	
	// Update state name
	char const* stateName = "Unknown";
	switch (mServerState)
	{
	case EServerState::Initializing:      stateName = "Initializing"; break;
	case EServerState::WaitingForPlayers: stateName = "Waiting for Players"; break;
	case EServerState::Countdown:         stateName = "Starting..."; break;
	case EServerState::Running:           stateName = "Running"; break;
	case EServerState::Finished:          stateName = "Finished"; break;
	}
	mStatusPanel->setServerState(stateName);
	
	// Update countdown
	mStatusPanel->setCountdown(mServerState == EServerState::Countdown ? mCountdownTimer : 0.0f);
	
	// Update player list
	if (haveServer())
	{
		mStatusPanel->updatePlayerList(getServer()->getPlayerManager());
	}
}

void DedicatedServerMode::destroyServerStatusUI()
{
	if (mStatusPanel)
	{
		mStatusPanel->destroy();
		mStatusPanel = nullptr;
	}
}

SlotId DedicatedServerMode::findAvailableSlot()
{
	if (!haveServer())
		return ERROR_SLOT_ID;

	SVPlayerManager* playerMgr = getServer()->getPlayerManager();
	
	// Assume max slot count is based on max players config
	int maxSlots = mConfig.maxPlayers;
	
	for (int i = 0; i < maxSlots; ++i)
	{
		bool isOccupied = false;
		for (auto iter = playerMgr->createIterator(); iter; ++iter)
		{
			GamePlayer* player = iter.getElement();
			if (player && player->getSlot() == i)
			{
				isOccupied = true;
				break;
			}
		}
		
		if (!isOccupied)
		{
			return (SlotId)i;
		}
	}
	
	return ERROR_SLOT_ID;
}

