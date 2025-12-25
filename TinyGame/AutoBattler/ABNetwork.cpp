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
	REGISTER_GAME_PACKET(ABCombatStartPacket);
	REGISTER_GAME_PACKET(ABCombatEventPacket);
	REGISTER_GAME_PACKET(ABCombatEndPacket);
	REGISTER_GAME_PACKET(ABShopUpdatePacket);

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
		mWorker->getPacketDispatcher().setUserFunc<ABShopUpdatePacket>(this, &ABNetEngine::onShopUpdatePacket);

		// Check for Dedicated Server mode
		if (mStage == nullptr)
		{
			mIsDedicatedServer = true;
			mDedicatedWorld = std::make_unique<World>();
			
			// Initialize Dedicated World
			mDedicatedWorld->init();
			LogMsg("NetEngine: Dedicated Server Mode Enabled (No Stage)");
		}
		
		World& world = GetWorld();



		// Setup replay manager (for both server and client)
		mReplayManager.setWorld(&world);


		// Enable network mode in World to skip local unit teleportation
		// Both server and client should use network mode
		world.setNetworkMode(true);
		// Disable auto-resolve combat
		world.setAutoResolveCombat(false);
		LogMsg("NetEngine: Network mode enabled (AutoResolve=False)");

		if (mNetWorker->isServer())
		{
			mCombatManager.initialize(AB_MAX_PLAYER_NUM);
			mNetWorker->getPacketDispatcher().setUserFunc<ABActionPacket>(this, &ABNetEngine::onPacketSV);

			world.onPhaseChanged = [this](BattlePhase phase)
			{
				if (mNetWorker->isServer())
				{
					// Broadcast Phase Change via Sync Packet
					// Now we send ONE packet PER PLAYER to sync their specific State (Gold/HP) correctly.
					World& w = GetWorld();
					int numPlayers = w.getPlayerCount(); // Assuming helper exists or use vector size

					for (int i = 0; i < numPlayers; ++i)
					{
						Player& p = w.getPlayer(i);
						if (p.getHp() <= 0 && w.getRound() > 0) continue; // Optional: skip dead? Better to sync dead state too.

						ABSyncPacket packet;
						packet.frameID = mNextFrameID;
						packet.checksum = 0;
						packet.playerId = i; // Identify which player this data belongs to
						packet.status = 0;

						packet.round = w.getRound();
						packet.phase = (int)w.getPhase();
						packet.phaseTimer = w.getPhaseTimer();

						packet.playerGold = p.getGold();
						packet.playerHP = p.getHp();
						packet.shopHash = 0; // TODO: Sync shop hash?

						mNetWorker->sendTcpCommand(&packet);

						if (phase == BattlePhase::Preparation)
						{
							ABShopUpdatePacket shopPacket;
							shopPacket.playerId = i;
							shopPacket.shopList = p.mShopList;
							mNetWorker->sendTcpCommand(&shopPacket);
						}
					}
					LogMsg("NetEngine: Broadcast Phase Change to %d (Synced Players)", (int)phase);
				}

				if (phase == BattlePhase::Combat)
				{
					onCombatPhaseStart();
				}
			};
		}
		else
		{
			// Client: No longer needs to register combat streaming packet handlers here
			// Setup replay manager with World reference - moved outside
		}

		bReplayPlaying = false;

		mSimAccumulator = 0;
		mAccTime = 0;
		mNextFrameID = 0;
		mLastReceivedFrame = 0;
		mSyncTimer = 0;
		mConnectionTimeout = 0;

		std::fill(std::begin(mPlayerTimeouts), std::end(mPlayerTimeouts), 0);
		std::fill(std::begin(mPlayerTimeoutStatus), std::end(mPlayerTimeoutStatus), false);

		// Important: Explicitly set Authority.
		// Server matches NetWorker::isServer(). Client is NOT authority.
		bool isServer = mNetWorker->isServer();
		world.setAuthority(isServer);
		
		// Send initial Sync Packet immediately if Server
		if (isServer)
		{
			// Server broadcasts initial state (existing logic)
			ABSyncPacket packet;
			packet.frameID = mNextFrameID;
			packet.checksum = 0;
			packet.playerId = 0;
			packet.status = 0; 
			packet.round = world.getRound();
			packet.phase = (int)world.getPhase();
			packet.phaseTimer = world.getPhaseTimer();
			packet.playerGold = 0;
			packet.playerHP = 0;
			packet.shopHash = 0;

			mNetWorker->sendTcpCommand(&packet);
			LogMsg("NetEngine: Sent initial Sync Packet.");
		}

		return true;
	}

	void ABNetEngine::update(IFrameUpdater& updater, long time)
	{
		bool bIsPaused = false;

		if (mNetWorker->isServer())
		{
			// Server: Check Clients
			int localPlayerId = GetWorld().getLocalPlayerIndex();
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
					// Initialize validation data to 0 to avoid garbage
					packet.round = 0; packet.phase = 0; packet.phaseTimer = 0.0f;
					packet.playerGold = 0; packet.playerHP = 0; packet.shopHash = 0;
					
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
						// Initialize validation data to 0
						packet.round = 0; packet.phase = 0; packet.phaseTimer = 0.0f;
						packet.playerGold = 0; packet.playerHP = 0; packet.shopHash = 0;
						
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
					
					// Fill validation data correctly for Restore
					World& world = GetWorld();
					packet.round = world.getRound();
					packet.phase = (int)world.getPhase();
					packet.phaseTimer = world.getPhaseTimer();
					
					// Note: Player-specific data (Gold/HP) is hard to fill for "Broadcast".
					// Restore packet is broadcast? sendTcpCommand is broadcast.
					// If we fill 0, client validation might fail or sync to 0.
					// But Client only syncs Phase/Timer from this packet in current logic.
					packet.playerGold = 0; 
					packet.playerHP = 0; 
					packet.shopHash = 0;
					
					mNetWorker->sendTcpCommand(&packet);
				}


				// Periodic Sync Broadcast (Server -> Clients) to fix drift
				mSyncTimer += time;
				if (mSyncTimer >= SyncInterval)
				{
					mSyncTimer = 0;
					ABSyncPacket packet;
					packet.frameID = mNextFrameID;
					packet.checksum = 0;
					packet.playerId = 0;
					packet.status = 0;

					World& world = GetWorld();
					packet.round = world.getRound();
					packet.phase = (int)world.getPhase();
					packet.phaseTimer = world.getPhaseTimer();
					packet.playerGold = 0;
					packet.playerHP = 0;
					packet.shopHash = 0;

					mNetWorker->sendTcpCommand(&packet);
				}

				{
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
				packet.checksum = 0; 
				packet.playerId = mStage->getWorld().getLocalPlayerIndex();
				// Initialize status to 0 (Heartbeat) as it was previously uninitialized
				packet.status = 0; 
				
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
				packet.phaseTimer = world.getPhaseTimer();

				mWorker->sendTcpCommand(&packet);
			}
		}

		if (bReplayPlaying)
		{
			mReplayManager.update(float(time) / 1000.0f);
			applyCompletedReplayResults();
		}


		long deltaTime = mIsDedicatedServer ? TickRate : mStage->getTickTime();
		if (!bIsPaused)
			mLocalTimeAccumulator += time;

		int maxSteps = mLocalTimeAccumulator / deltaTime;
		int steps = 0;
		while (mSimAccumulator >= deltaTime && steps < maxSteps)
		{
			mSimAccumulator -= deltaTime;
			mLocalTimeAccumulator -= deltaTime;
			
			// ✅ Dedicated Server Support: Tick Independent World
			if (mIsDedicatedServer)
			{
				GetWorld().tick(float(deltaTime) / 1000.0f);
			}
			else
			{
				mStage->runLogic(float(deltaTime) / 1000.0f);
			}

			steps++;
		}
	}

	void ABNetEngine::broadcastFrame(int numFrame)
	{
		CHECK(mNetWorker->isServer());

		if (mStage)
		{
			// Determinism: Execute in the exact same order as sent to clients
			for (auto const& data : mPendingActions)
			{
				mStage->executeAction(data.port, data.item);
			}
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
		if (!GetWorld().validateAction(packet->port, packet->item, &error))
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
				World& world = GetWorld();
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

			LogMsg("Client: Recv SyncPacket (Status=%d, Ph=%d, R=%d)", packet->status, packet->phase, packet->round);

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
			else
			{
				// Sync Time Logic
				World& world = GetWorld();
				// Force Sync Phase if different
				if (world.getPhase() != (BattlePhase)packet->phase)
				{
					LogMsg("Client Sync: Phase Mismatch! Force Sync (Ph: %d -> %d)", (int)world.getPhase(), packet->phase);
					
					// IMPORTANT: Sync Round BEFORE changing phase!
					// changePhase() triggers setupRound(), which depends on the current Round number.
					if (world.getRound() != packet->round)
					{
						LogMsg("Client Sync: Round Mismatch! Force Sync (R: %d -> %d)", world.getRound(), packet->round);
						world.setRound(packet->round);
					}

					world.changePhase((BattlePhase)packet->phase);
					world.setPhaseTimer(packet->phaseTimer);
					
					// Force sync player state on phase change mismatch
					Player& player = world.getLocalPlayer();
					if (player.getGold() != packet->playerGold)
					{
						// Hacky set? Player::setGold?
						// Assume drift is small or handled by actions. 
						// But if phase is wrong, everything might be wrong.
					}
				}
				else
				{
					// Same Phase, adjust timer drift
					float timeDiff = std::abs(world.getPhaseTimer() - packet->phaseTimer);
					if (timeDiff > 0.5f)
					{
						world.setPhaseTimer(packet->phaseTimer);
						// LogMsg("Client: PhaseTimer corrected. Drift=%.2f", timeDiff);
					}
				}

				// Always Sync Player State (Gold, HP) from Server
				// Packet carries data for 'packet->playerId'
				if (world.isValidPlayer(packet->playerId))
				{
					Player& player = world.getPlayer(packet->playerId);
					
					if (player.getGold() != packet->playerGold)
					{
						// Only log for Local Player to reduce spam
						if (packet->playerId == world.getLocalPlayerIndex())
							LogMsg("Client Sync: Player %d Gold correction %d -> %d", packet->playerId, player.getGold(), packet->playerGold);
						
						player.setGold(packet->playerGold);
					}
					if (player.getHp() != packet->playerHP)
					{
						player.setHp(packet->playerHP);
					}
				}
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

	void ABNetEngine::onShopUpdatePacket(IComPacket* cp)
	{
		auto* packet = cp->cast<ABShopUpdatePacket>();
		World& world = GetWorld();

		if (world.isValidPlayer(packet->playerId))
		{
			Player& player = world.getPlayer(packet->playerId);
			player.mShopList = packet->shopList;

			if (packet->playerId == world.getLocalPlayerIndex())
				LogMsg("Client: Recv Shop Update [ %d %d %d %d %d ]",
					player.mShopList[0], player.mShopList[1], player.mShopList[2], player.mShopList[3], player.mShopList[4]);
		}
	}
	void ABNetEngine::onCombatPhaseStart()
	{
		if (!mNetWorker->isServer())
			return;

		LogMsg("ABNetEngine: Combat phase started, submitting combats...");
		
		World& world = GetWorld();

		// Get all match pairings
		TArray<World::MatchPair> const& matches = world.getMatches();
		
		// Check if this is PVE or PVP round
		bool isPVE = matches.empty();
		
		if (isPVE)
		{
			// PVE: Each player fights independently against their own enemies
			LogMsg("ABNetEngine: PVE Round - submitting individual combats for each player");
			
			for (int playerIndex = 0; playerIndex < AB_MAX_PLAYER_NUM; ++playerIndex)
			{
				Player const& player = world.getPlayer(playerIndex);
				
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
					playerIndex, INDEX_NONE,  // -1 indicates PVE (no away player)
					playerUnits, enemyUnits,
					[this](CombatResult result) {
						// This callback executes in worker thread
						// Should use thread-safe queue to pass to main thread
						LogMsg("PVE Combat %u completed in worker thread", result.combatID);
						std::lock_guard<std::mutex> lock(mResultMutex);
						mCombatResults.push_back(result);
					}
				);
				
				mPendingCombatIDs.push_back(combatID);
				LogMsg("ABNetEngine: PVE Combat %u submitted for Player %d", combatID, playerIndex);

				// Log Sent IDs
				std::string sHome = "Sent PVE HomeUnits: ";
				for(auto& u : playerUnits) sHome += std::to_string(u.unitID) + " ";
				LogMsg("%s", sHome.c_str());

				// Send Start Packet to Clients
				ABCombatStartPacket packet;
				packet.combatID = combatID;
				packet.homeIndex = playerIndex;
				packet.awayIndex = INDEX_NONE;
				packet.homeUnits = std::move(playerUnits);
				packet.awayUnits = std::move(enemyUnits);
				mNetWorker->sendTcpCommand(&packet);
			}
		}
		else
		{
			// PVP: Use match pairings to set up combats
			LogMsg("ABNetEngine: PVP Round - submitting %d match combats", matches.size());
			
			for (World::MatchPair const& match : matches)
			{
				Player const& homePlayer = world.getPlayer(match.home);
				Player const& awayPlayer = world.getPlayer(match.away);
				
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
				packet.homeUnits = std::move(homeUnits);
				packet.awayUnits = std::move(awayUnits);
				mNetWorker->sendTcpCommand(&packet);
			}
		}
		
		LogMsg("ABNetEngine: Total %d combats submitted", mPendingCombatIDs.size());
	}

	// Poll and broadcast combat events (called in update)
	void ABNetEngine::broadcastCombatEvents()
	{
		auto batches = mCombatManager.pollEventBatches(4);

		for (auto& batch : batches)
		{
			// LogMsg("Broadcasting %d combat events for combat %u (batch %u)",
			//	batch.events.size(), batch.combatID, batch.batchIndex);

			// Create and send ABCombatEventPacket
			ABCombatEventPacket packet;
			packet.combatID = batch.combatID;
			packet.batchIndex = batch.batchIndex;
			packet.events = std::move(batch.events);
			mNetWorker->sendTcpCommand(&packet);
		}
		
		// ✅ After broadcasting events, check if any completed combats can send their End packet
		sendPendingCombatEndPackets();
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

		// ✅ DON'T send CombatEnd immediately - cache it and wait for all events to be sent
		mCompletedCombats[result.combatID] = result;
		LogMsg("ABNetEngine: Combat %u result cached, waiting for all events to be sent", result.combatID);
		
		// ✅ NOTE: Do NOT apply results or change phase here directly!
		// For Host/Listen Server, we must wait for the local Replay to finish.
		// We rely on the NP_COMBAT_END packet being sent (even to self via loopback)
		// and processed by onCombatEndPacket -> applyCompletedReplayResults.
	}

	// ========================================================================
	// Client-side Combat Packet Handlers
	// ========================================================================

	void ABNetEngine::onCombatStartPacket(IComPacket* cp)
	{
		auto* packet = cp->cast<ABCombatStartPacket>();

		LogMsg("Client: Recv CombatStartPacket FrameID=%u", packet->combatID); // Using combatID as pseudo-frame tracker, need better seq?

		bReplayPlaying = true;

		// Sync: Ensure Client enters Combat Phase immediately when Server starts it.
		// This corrects any drift caused by prolonged Replays or Client lag.
		if (GetWorld().getPhase() != BattlePhase::Combat)
		{
			LogMsg("Client: Force switching to Combat Phase (Sync from Server CombatPacket)");
			GetWorld().changePhase(BattlePhase::Combat);
		}


		LogMsg("Client: Combat %u started (Home=%d vs Away=%d, %d vs %d units)",
			packet->combatID, packet->homeIndex, packet->awayIndex,
			packet->homeUnits.size(), packet->awayUnits.size());

		// Log Recv IDs
		std::string sHome = "Recv PVE HomeUnits: ";
		for(auto& u : packet->homeUnits) sHome += std::to_string(u.unitID) + " ";
		LogMsg("%s", sHome.c_str());

		// Setup combat replay
		// Dedicated Server skips replay recording to finish immediately
		if (!mIsDedicatedServer)
		{
			mReplayManager.setupReplay(
				packet->combatID,
				packet->homeIndex, packet->awayIndex,
				packet->homeUnits, packet->awayUnits
			);
		}
	}

	void ABNetEngine::onCombatEventPacket(IComPacket* cp)
	{
		auto* packet = cp->cast<ABCombatEventPacket>();

		LogMsg("Client: Combat %u received event batch %u (%d events)",
			packet->combatID, packet->batchIndex, packet->events.size());

		// Enqueue events for replay
		// Dedicated Server skips events
		if (!mIsDedicatedServer)
		{
			mReplayManager.enqueueEvents(packet->combatID, packet->events);
		}
	}

	void ABNetEngine::onCombatEndPacket(IComPacket* cp)
	{
		auto* packet = cp->cast<ABCombatEndPacket>();

		LogMsg("Client: Combat %u ended - Winner: Index=%d, Damage: Home=%d, Away=%d, Duration=%.2fs",
			packet->combatID, packet->winnerIndex,
			packet->homeDamage, packet->awayDamage, packet->duration);

		// Mark combat as ended
		if (!mIsDedicatedServer)
		{
			mReplayManager.markEnded(packet->combatID, packet->winnerIndex);
		}

		// DON'T apply result immediately - cache it and wait for replay to finish
		PendingCombatResult pending;
		pending.homePlayerIndex = packet->homePlayerIndex;
		pending.awayPlayerIndex = packet->awayPlayerIndex;
		pending.winnerIndex = packet->winnerIndex;
		pending.homeDamage = packet->homeDamage;
		pending.awayDamage = packet->awayDamage;
		pending.duration = packet->duration;
		
		mPendingCombatResults[packet->combatID] = pending;
		LogMsg("Client: Combat %u result cached, waiting for replay to finish", packet->combatID);
	}

	World& ABNetEngine::GetWorld()
	{
		if (mIsDedicatedServer)
			return *mDedicatedWorld;
		return mStage->getWorld();
	}

	// ========================================================================
	// Server: Send pending CombatEnd packets after all events delivered
	// ========================================================================
	void ABNetEngine::sendPendingCombatEndPackets()
	{
		for (auto it = mCompletedCombats.begin(); it != mCompletedCombats.end();)
		{
			uint32 combatID = it->first;
			
			// Check if this combat still has more events to send
			if (mCombatManager.hasMoreEvents(combatID))
			{
				++it;  // Still has events, wait for next call
				continue;
			}
			
			// All events sent, now send CombatEnd
			CombatResult& result = it->second;
			
			ABCombatEndPacket packet;
			packet.combatID = result.combatID;
			packet.homePlayerIndex = result.homePlayerIndex;
			packet.awayPlayerIndex = result.awayPlayerIndex;
			packet.winnerIndex = result.winnerIndex;
			packet.homeDamage = result.homeDamage;
			packet.awayDamage = result.awayDamage;
			packet.duration = result.duration;
			mNetWorker->sendTcpCommand(&packet);
			
			LogMsg("Sent CombatEnd for combat %u (all events delivered)", combatID);
			
			it = mCompletedCombats.erase(it);
		}
	}

	// ========================================================================
	// Client: Apply combat results after replay finishes
	// ========================================================================
	void ABNetEngine::applyCompletedReplayResults()
	{
		for (auto it = mPendingCombatResults.begin(); it != mPendingCombatResults.end();)
		{
			uint32 combatID = it->first;
			
			// Check if replay is still playing
			if (!mReplayManager.isReplayFinished(combatID))
			{
				++it;  // Still playing, wait
				continue;
			}

			auto& result = it->second;
			
			// Calculate aliveCount
			int homeAlive = 0, awayAlive = 0;
			if (result.winnerIndex == result.homePlayerIndex)
				homeAlive = (result.awayDamage >= 2) ? (result.awayDamage - 2) : 1;
			else if (result.winnerIndex == result.awayPlayerIndex)
				awayAlive = (result.homeDamage >= 2) ? (result.homeDamage - 2) : 1;
			
			bool isPVE = (result.awayPlayerIndex == -1);
			mStage->getWorld().applyCombatResult(
				result.homePlayerIndex, result.awayPlayerIndex,
				homeAlive, awayAlive, isPVE
			);
			
			LogMsg("Client: Applied combat result for combat %u after replay finished", combatID);
			
			it = mPendingCombatResults.erase(it);
		}
		
		// All replays finished, switch phase
		if (mPendingCombatResults.empty() && mReplayManager.allReplaysFinished())
		{
			LogMsg("Client: All replays finished, switching to Preparation phase");
			mStage->getWorld().changePhase(BattlePhase::Preparation);
			bReplayPlaying = false;
		}
	}

} // namespace AutoBattler
