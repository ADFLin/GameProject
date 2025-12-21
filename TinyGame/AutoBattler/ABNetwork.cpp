#include "ABPCH.h"
#include "ABNetwork.h"
#include "ABStage.h"
#include "ABBot.h"
#include "ABCombatReplay.h"

namespace AutoBattler
{


	REGISTER_GAME_PACKET(ABActionPacket);
	REGISTER_GAME_PACKET(ABFramePacket);
	REGISTER_GAME_PACKET(ABSyncPacket);
	REGISTER_GAME_PACKET(ABActionRejectPacket);
	REGISTER_GAME_PACKET(ABCombatStartPacket);
	REGISTER_GAME_PACKET(ABCombatEventPacket);
	REGISTER_GAME_PACKET(ABCombatEndPacket);

	// Client-side replay manager moved to member variable

	ABNetEngine::ABNetEngine(LevelStage* stage)
		: mStage(stage)
	{
	}

	bool ABNetEngine::build(BuildParam& param)
	{
		LogMsg("NetEngine Build: Worker=%p", param.worker);
		mWorker = param.worker;
		mNetWorker = param.netWorker;

		mNetWorker->getPacketDispatcher().setUserFunc<ABFramePacket>(this, &ABNetEngine::onFramePacket);
		mNetWorker->getPacketDispatcher().setUserFunc<ABSyncPacket>(this, &ABNetEngine::onSyncPacket); // Register for both

		// Register combat streaming packet handlers (for both server and client)
		// Server needs these to receive its own broadcasts for host player replay
		mWorker->getPacketDispatcher().setUserFunc<ABCombatStartPacket>(this, &ABNetEngine::onCombatStartPacket);
		mWorker->getPacketDispatcher().setUserFunc<ABCombatEventPacket>(this, &ABNetEngine::onCombatEventPacket);
		mWorker->getPacketDispatcher().setUserFunc<ABCombatEndPacket>(this, &ABNetEngine::onCombatEndPacket);

		// Setup replay manager (for both server and client)
		mReplayManager.setWorld(&mStage->getWorld());

		// Enable network mode in World to skip local unit teleportation
		// Both server and client should use network mode
		mStage->getWorld().setNetworkMode(true);
		// Disable auto-resolve combat in Main World for network mode
		// Combat will be simulated asynchronously (tempWorld) and replayed via events
		mStage->getWorld().setAutoResolveCombat(false);
		LogMsg("NetEngine: Network mode enabled (AutoResolve=False) - combat will be handled asynchronously");

		if (mNetWorker->isServer())
		{
			mCombatManager.initialize(AB_MAX_PLAYER_NUM);
			mNetWorker->getPacketDispatcher().setUserFunc<ABActionPacket>(this, &ABNetEngine::onPacketSV);
		}
		else
		{
			// Client: No longer needs to register combat streaming packet handlers here
			// Setup replay manager with World reference - moved outside
		}

		mSimAccumulator = 0;
		mAccTime = 0;
		mNextFrameID = 0;
		mLastReceivedFrame = 0;
		mSyncTimer = 0;
		mConnectionTimeout = 0;

		std::fill(std::begin(mPlayerTimeouts), std::end(mPlayerTimeouts), 0);
		std::fill(std::begin(mPlayerTimeoutStatus), std::end(mPlayerTimeoutStatus), false);

		return true;
	}

	void ABNetEngine::update(IFrameUpdater& updater, long time)
	{
		bool bIsPaused = false;

		if (mNetWorker->isServer())
		{
			// Server: Check Clients
			int localPlayerId = mStage->getWorld().getLocalPlayerIndex();
			for (auto it = mNetWorker->getPlayerManager()->createIterator(); it; ++it)
			{
				GamePlayer* player = it.getElement();
				if (player->getType() != PT_PLAYER)
					continue;

				int id = player->getActionPort();
				if (id == localPlayerId)
					continue;

				mPlayerTimeouts[id] += time;
				if (mPlayerTimeouts[id] > TimeoutSpan)
				{
					if (!mPlayerTimeoutStatus[id])
					{
						mPlayerTimeoutStatus[id] = true;
						if (OnTimeout)
							OnTimeout(id, true);
					}
				}
				else
				{
					if (mPlayerTimeoutStatus[id])
					{
						mPlayerTimeoutStatus[id] = false;
						if (OnTimeout)
							OnTimeout(id, false);
					}
				}
			}

			bool bAnyClientTimeout = false;
			for (auto it = mNetWorker->getPlayerManager()->createIterator(); it; ++it)
			{
				GamePlayer* player = it.getElement();
				if (player->getType() != PT_PLAYER)
					continue;

				int id = player->getActionPort();
				if (id == localPlayerId)
					continue;

				if (mPlayerTimeoutStatus[id])
				{
					bAnyClientTimeout = true;
					break;
				}
			}

			if (bAnyClientTimeout)
			{
				bIsPaused = true;

				// Send Timeout Packet
				if (!bServerPaused)
				{
					bServerPaused = true;
					ABSyncPacket packet;
					packet.frameID = mNextFrameID;
					packet.checksum = 0;
					packet.playerId = 0; // TODO: Find who timed out?
					packet.status = 1;
					mNetWorker->sendTcpCommand(&packet);
					mSyncTimer = 0;
				}
				else
				{
					mSyncTimer += time;
					if (mSyncTimer >= SyncInterval)
					{
						mSyncTimer = 0;
						ABSyncPacket packet;
						packet.frameID = mNextFrameID;
						packet.checksum = 0;
						packet.playerId = 0;
						packet.status = 1;
						mNetWorker->sendTcpCommand(&packet);
					}
				}
			}
			else
			{
				if (bServerPaused)
				{
					bServerPaused = false;
					// Send Restore Packet
					ABSyncPacket packet;
					packet.frameID = mNextFrameID;
					packet.checksum = 0;
					packet.playerId = 0;
					packet.status = 2;
					mNetWorker->sendTcpCommand(&packet);
				}

				// Poll and broadcast combat events
				broadcastCombatEvents();

				// Process combat results from worker threads (thread-safe)
				{
					std::lock_guard<std::mutex> lock(mResultMutex);
					for (auto& result : mCombatResults)
					{
						onCombatComplete(result);
					}
					mCombatResults.clear();
				}

				mAccTime += time;
				if (mAccTime >= TickRate)
				{
					int numFrame = mAccTime / TickRate;
					mAccTime -= numFrame * TickRate;
					broadcastFrame(numFrame);
				}
			}
		}
		else
		{

			// Client: Check Server
			mConnectionTimeout += time;
			if (mConnectionTimeout > TimeoutSpan)
			{
				if (!bIsTimeout)
				{
					bIsTimeout = true;
					if (OnTimeout)
						OnTimeout(mStage->getWorld().getLocalPlayerIndex(), true);
				}
				// Client returns? Existing code returned. But maybe we shouldn't return if we want to try sending heartbeat?
				// But if connection lost, sending is futile.
				// Existing code: return;
			}

			if (bIsTimeout && mConnectionTimeout <= TimeoutSpan)
			{
				bIsTimeout = false;
				if (OnTimeout)
					OnTimeout(mStage->getWorld().getLocalPlayerIndex(), false);
			}

			if (bIsTimeout)
				return;


			// Client: Send Checksum/Heartbeat
			mSyncTimer += time;
			if (mSyncTimer >= SyncInterval)
			{
				mSyncTimer = 0;

				ABSyncPacket packet;
				packet.frameID = mLastReceivedFrame;
				packet.checksum = 0; // TODO: World Hash
				packet.playerId = mStage->getWorld().getLocalPlayerIndex();
				
				// Fill Validation Data
				World& world = mStage->getWorld();
				packet.round = world.getRound();
				packet.phase = (int)world.getPhase();
				
				Player const& player = world.getLocalPlayer();
				packet.playerGold = player.getGold();
				packet.playerHP = player.getHp();
				
				// Calculate simple Shop Hash
				int sh = 0;
				for (int val : player.mShopList) sh = sh * 31 + val;
				packet.shopHash = sh;

				mWorker->sendTcpCommand(&packet);
			}
		}

		// Update combat replay playback (both server and client)
		mReplayManager.update(float(time) / 1000.0f);  // Convert ms to seconds

		long deltaTime = mStage->getTickTime();
		if (!bIsPaused)
			mLocalTimeAccumulator += time;

		int maxSteps = mLocalTimeAccumulator / deltaTime;
		int steps = 0;
		while (mSimAccumulator >= deltaTime && steps < maxSteps)
		{
			mSimAccumulator -= deltaTime;
			mLocalTimeAccumulator -= deltaTime;

			BattlePhase oldPhase = mStage->getPhase();
			mStage->runLogic(float(deltaTime) / 1000.0f);

			if (mNetWorker->isServer() && oldPhase != BattlePhase::Combat && mStage->getPhase() == BattlePhase::Combat)
			{
				onCombatPhaseStart();
			}

			steps++;
		}
	}

	void ABNetEngine::broadcastFrame(int numFrame)
	{
		CHECK(mNetWorker->isServer());

		// Determinism: Execute in the exact same order as sent to clients
		for (auto const& data : mPendingActions)
		{
			mStage->executeAction(data.port, data.item);
		}

		addSimuateTime(TickRate * numFrame);

		ABFramePacket packet;
		packet.frameID = mNextFrameID++;
		packet.actions = std::move(mPendingActions);
		packet.numFrame = numFrame;
		mNetWorker->sendTcpCommand(&packet, WSF_IGNORE_LOCAL);
	}

	void ABNetEngine::onFramePacket(IComPacket* cp)
	{
		mConnectionTimeout = 0;

		ABFramePacket* packet = cp->cast<ABFramePacket>();
		mLastReceivedFrame = packet->frameID;

		long deltaTime = mStage->getTickTime();
		while (mSimAccumulator >= deltaTime)
		{
			mSimAccumulator -= deltaTime;
			mLocalTimeAccumulator -= deltaTime;
			mStage->runLogic(float(deltaTime) / 1000.0f);
		}

		// LogMsg("Client Run Frame %u : %d", packet->frameID, packet->actions.size());
		for (auto const& action : packet->actions)
		{
			mStage->executeAction(action.port, action.item);
		}

		// Step Simulation (Client)
		addSimuateTime(TickRate * packet->numFrame);
	}

	void ABNetEngine::onPacketSV(IComPacket* cp)
	{
		ABActionPacket* packet = cp->cast<ABActionPacket>();

		if (packet->port < AB_MAX_PLAYER_NUM)
			mPlayerTimeouts[packet->port] = 0;

		// === Server-side Action validation ===
		ActionError error;
		if (!mStage->getWorld().validateAction(packet->port, packet->item, &error))
		{
			LogWarning(0, "Rejected action from player %d: type=%d, error=%d (%s)", 
					   packet->port, packet->item.type, error.code, error.message);

			// Send rejection notification
			ABActionRejectPacket rejectPacket;
			rejectPacket.port = packet->port;
			rejectPacket.actionType = packet->item.type;
			rejectPacket.errorCode = error.code;

			// Send to the client that initiated the action
			auto player = static_cast<ServerWorker*>(mNetWorker)->getPlayerManager()->getPlayerByActionPort(packet->port);
			if (player)
			{
				player->sendTcpCommand(&rejectPacket);
			}

			return;  // Reject this action, don't add to pending
		}

		// Validation passed, buffer action for broadcast
		mPendingActions.push_back({ packet->port, packet->item });
	}

	void ABNetEngine::onSyncPacket(IComPacket* cp)
	{
		ABSyncPacket* packet = cp->cast<ABSyncPacket>();

		if (mNetWorker->isServer())
		{
			// Server Logic: Heartbeat from Client
			if (packet->playerId < AB_MAX_PLAYER_NUM)
			{
				mPlayerTimeouts[packet->playerId] = 0;

				// Compare State
				World& world = mStage->getWorld();
				Player const& svPlayer = world.getPlayer(packet->playerId);

				// Calc Server Shop Hash
				int svShopHash = 0;
				for (int val : svPlayer.mShopList) svShopHash = svShopHash * 31 + val;

				if (packet->round != world.getRound() ||
					packet->phase != (int)world.getPhase() ||
					packet->playerGold != svPlayer.getGold() ||
					packet->playerHP != svPlayer.getHp() ||
					packet->shopHash != svShopHash)
				{
					LogMsg("SYNC ERROR Player %d: [CL] R%d P%d G%d HP%d SH%d != [SV] R%d P%d G%d HP%d SH%d",
						packet->playerId,
						packet->round, packet->phase, packet->playerGold, packet->playerHP, packet->shopHash,
						world.getRound(), (int)world.getPhase(), svPlayer.getGold(), svPlayer.getHp(), svShopHash);
				}
			}
		}
		else
		{
			// Client Logic: Status from Server
			mConnectionTimeout = 0; // Server is alive

			if (packet->status == 1) // Timeout
			{
				if (OnTimeout)
					OnTimeout(packet->playerId, true);
			}
			else if (packet->status == 2) // Restore
			{
				if (OnTimeout)
					OnTimeout(packet->playerId, false);
			}
		}

		//LogMsg("Client Sync: Frame %u Hash %u", packet->frameID, packet->checksum);

		// TODO: Check for divergence
	}

	void ABNetEngine::sendAction(ActionPort port, ABActionItem const& item)
	{
		if (mNetWorker->isServer())
		{
			// Host Action: Do NOT Exec Immediate. Buffer for Frame.
			// mStage->executeAction(port, item);
			mPendingActions.push_back({ port, item });
		}
		else
		{
			// Client: Send Only
			ABActionPacket packet;
			packet.port = port;
			packet.item = item;
			mWorker->sendTcpCommand(&packet);
		}
	}

	void ABNetEngine::onCombatPhaseStart()
	{
		if (!mNetWorker->isServer())
			return;

		LogMsg("ABNetEngine: Combat phase started, submitting combats...");

		// Get all match pairings
		TArray<World::MatchPair> const& matches = mStage->getWorld().getMatches();
		
		// Check if this is PVE or PVP round
		bool isPVE = matches.empty();
		
		if (isPVE)
		{
			// PVE: Each player fights independently against their own enemies
			LogMsg("ABNetEngine: PVE Round - submitting individual combats for each player");
			
			for (int playerIndex = 0; playerIndex < AB_MAX_PLAYER_NUM; ++playerIndex)
			{
				Player const& player = mStage->getWorld().getPlayer(playerIndex);
				
				// Skip dead players
				if (player.getHp() <= 0)
					continue;
				
				// Skip if no enemies to fight
				if (player.mEnemyUnits.empty())
					continue;
				
				// Collect player unit snapshots
				TArray<UnitSnapshot> playerUnits;
				for (Unit* unit : player.mUnits)
				{
					if (unit)
						playerUnits.push_back(UnitSnapshot::fromUnit(*unit));
				}
				
				// Collect enemy unit snapshots
				TArray<UnitSnapshot> enemyUnits;
				for (Unit* unit : player.mEnemyUnits)
				{
					if (unit)
						enemyUnits.push_back(UnitSnapshot::fromUnit(*unit));
				}
				
				// Submit PVE combat (player vs environment)
				uint32 combatID = mCombatManager.submitCombat(
					playerIndex, -1,  // -1 indicates PVE (no away player)
					playerUnits, enemyUnits,
					[this](CombatResult result) {
						// This callback executes in worker thread
						// Should use thread-safe queue to pass to main thread
						LogMsg("PVE Combat %u completed in worker thread", result.combatID);
					}
				);
				
				mPendingCombatIDs.push_back(combatID);
				LogMsg("ABNetEngine: PVE Combat %u submitted for Player %d", combatID, playerIndex);

				// Send Start Packet to Clients
				ABCombatStartPacket packet;
				packet.combatID = combatID;
				packet.homeIndex = playerIndex;
				packet.awayIndex = -1;
				packet.homeUnits = playerUnits;
				packet.awayUnits = enemyUnits;
				mNetWorker->sendTcpCommand(&packet);
			}
		}
		else
		{
			// PVP: Use match pairings to set up combats
			LogMsg("ABNetEngine: PVP Round - submitting %d match combats", matches.size());
			
			for (World::MatchPair const& match : matches)
			{
				Player const& homePlayer = mStage->getWorld().getPlayer(match.home);
				Player const& awayPlayer = mStage->getWorld().getPlayer(match.away);
				
				// Collect home player units
				TArray<UnitSnapshot> homeUnits;
				for (Unit* unit : homePlayer.mUnits)
				{
					if (unit)
						homeUnits.push_back(UnitSnapshot::fromUnit(*unit));
				}
				
				// Collect away units (real units or ghosts from enemy list)
				TArray<UnitSnapshot> awayUnits;
				if (match.isGhost)
				{
					// Ghost match: away units are in home player's enemy list
					for (Unit* unit : homePlayer.mEnemyUnits)
					{
						if (unit)
							awayUnits.push_back(UnitSnapshot::fromUnit(*unit));
					}
				}
				else
				{
					// Real PVP: collect away player's units
					for (Unit* unit : awayPlayer.mUnits)
					{
						if (unit)
							awayUnits.push_back(UnitSnapshot::fromUnit(*unit));
					}
				}
				
				// Submit PVP combat
				uint32 combatID = mCombatManager.submitCombat(
					match.home, match.away,
					homeUnits, awayUnits,
					[this](CombatResult result) {
						// This callback executes in worker thread
						// Push to thread-safe queue for main thread processing
						LogMsg("PVP Combat %u completed in worker thread", result.combatID);
						std::lock_guard<std::mutex> lock(mResultMutex);
						mCombatResults.push_back(result);
					}
				);
				
				mPendingCombatIDs.push_back(combatID);
				LogMsg("ABNetEngine: PVP Combat %u submitted (Home=%d vs Away=%d%s)", 
					combatID, match.home, match.away, match.isGhost ? " [Ghost]" : "");


				// Send Start Packet to Clients
				ABCombatStartPacket packet;
				packet.combatID = combatID;
				packet.homeIndex = match.home;
				packet.awayIndex = match.away;
				packet.homeUnits = homeUnits;
				packet.awayUnits = awayUnits;
				mNetWorker->sendTcpCommand(&packet);
			}
		}
		
		LogMsg("ABNetEngine: Total %d combats submitted", mPendingCombatIDs.size());
	}

	// Poll and broadcast combat events (called in update)
	void ABNetEngine::broadcastCombatEvents()
	{
		auto batches = mCombatManager.pollEventBatches();

		for (auto& batch : batches)
		{
			LogMsg("Broadcasting %d combat events for combat %u (batch %u)",
				batch.events.size(), batch.combatID, batch.batchIndex);

			// Feed events to Local Server Replay (Host Player)
			mReplayManager.enqueueEvents(batch.combatID, batch.events);

			// Create and send ABCombatEventPacket
			ABCombatEventPacket packet;
			packet.combatID = batch.combatID;
			packet.batchIndex = batch.batchIndex;
			packet.events = std::move(batch.events);
			mNetWorker->sendTcpCommand(&packet);
		}
	}

	// Combat completion callback
	void ABNetEngine::onCombatComplete(CombatResult result)
	{
		LogMsg("ABNetEngine: Combat %u completed - Winner: %d, Duration: %.2fs",
			result.combatID, result.winnerIndex, result.duration);

		// Remove pending combat
		mPendingCombatIDs.erase(
			std::remove(mPendingCombatIDs.begin(), mPendingCombatIDs.end(), result.combatID),
			mPendingCombatIDs.end()
		);

		// Send ABCombatEndPacket
		ABCombatEndPacket packet;
		packet.combatID = result.combatID;
		packet.homePlayerIndex = result.homePlayerIndex;
		packet.awayPlayerIndex = result.awayPlayerIndex;
		packet.winnerIndex = result.winnerIndex;
		packet.homeDamage = result.homeDamage;
		packet.awayDamage = result.awayDamage;
		packet.duration = result.duration;
		mNetWorker->sendTcpCommand(&packet);

		// Apply combat result to World using shared method
		World& world = mStage->getWorld();
		
		// Calculate alive units from result
		// If damage > 0, player lost (0 alive)
		// Otherwise use damage formula inverse: damage = 2 + aliveCount
		int homeAlive = 0;
		int awayAlive = 0;
		
		if (result.homeDamage > 0)
		{
			// Home lost
			homeAlive = 0;
			awayAlive = result.homeDamage - 2;  // Reverse: damage = 2 + awayAlive
		}
		else if (result.awayDamage > 0)
		{
			// Away lost
			awayAlive = 0;
			homeAlive = result.awayDamage - 2;  // Reverse: damage = 2 + homeAlive
		}
		else
		{
			// Draw or both alive - use winner to determine
			if (result.winnerIndex == result.homePlayerIndex)
			{
				homeAlive = 1;  // At least one survived
				awayAlive = 0;
			}
			else if (result.winnerIndex == result.awayPlayerIndex)
			{
				homeAlive = 0;
				awayAlive = 1;
			}
		}
		
		// Mark replay as ended for server-side replay manager
		mReplayManager.markEnded(result.combatID, result.winnerIndex);
		
		bool isPVE = (result.awayPlayerIndex == -1);
		world.applyCombatResult(result.homePlayerIndex, result.awayPlayerIndex,
		                        homeAlive, awayAlive, isPVE);

		// If all combats completed, switch back to preparation phase
		if (mPendingCombatIDs.empty())
		{
			LogMsg("ABNetEngine: All combats completed, switching to preparation phase");
			world.changePhase(BattlePhase::Preparation);
		}
	}

	// ========================================================================
	// Client-side Combat Packet Handlers
	// ========================================================================

	void ABNetEngine::onCombatStartPacket(IComPacket* cp)
	{
		auto* packet = cp->cast<ABCombatStartPacket>();

		LogMsg("Client: Combat %u started (Home=%d vs Away=%d, %d vs %d units)",
			packet->combatID, packet->homeIndex, packet->awayIndex,
			packet->homeUnits.size(), packet->awayUnits.size());

		// Setup combat replay
		mReplayManager.setupReplay(
			packet->combatID,
			packet->homeIndex, packet->awayIndex,
			packet->homeUnits, packet->awayUnits
		);
	}

	void ABNetEngine::onCombatEventPacket(IComPacket* cp)
	{
		auto* packet = cp->cast<ABCombatEventPacket>();

		LogMsg("Client: Combat %u received event batch %u (%d events)",
			packet->combatID, packet->batchIndex, packet->events.size());

		// Enqueue events for replay
		mReplayManager.enqueueEvents(packet->combatID, packet->events);
	}

	void ABNetEngine::onCombatEndPacket(IComPacket* cp)
	{
		auto* packet = cp->cast<ABCombatEndPacket>();

		LogMsg("Client: Combat %u ended - Winner: Index=%d, Damage: Home=%d, Away=%d, Duration=%.2fs",
			packet->combatID, packet->winnerIndex,
			packet->homeDamage, packet->awayDamage, packet->duration);

		// Mark combat as ended
		mReplayManager.markEnded(packet->combatID, packet->winnerIndex);

		// Apply result to Client World to sync HP/Gold/Wins
		int homeAlive = 0;
		int awayAlive = 0;


		if (packet->winnerIndex == packet->homePlayerIndex)
		{
			// Home won, calculate approximate alive count from damage dealt
			homeAlive = (packet->awayDamage >= 2) ? (packet->awayDamage - 2) : 1;
		}
		else if (packet->winnerIndex == packet->awayPlayerIndex)
		{
			// Away won (or PVE Environment if index is -1)
			awayAlive = (packet->homeDamage >= 2) ? (packet->homeDamage - 2) : 1;
		}

		bool isPVE = (packet->awayPlayerIndex == -1);
		mStage->getWorld().applyCombatResult(
			packet->homePlayerIndex, packet->awayPlayerIndex,
			homeAlive, awayAlive, isPVE
		);

		// Switch to Preparation phase after combat ends (sync with Server)
		// Note: In future with multiple concurrent combats, this should wait for all combats
		mStage->getWorld().changePhase(BattlePhase::Preparation);
	}

} // namespace AutoBattler
