#include "ABPCH.h"
#include "ABCombat.h"
#include "ABDefine.h"
#include "LogSystem.h"

namespace AutoBattler
{
	//=======================================================================
	// UnitSnapshot
	//=======================================================================

	UnitSnapshot UnitSnapshot::fromUnit(Unit const& unit)
	{
		UnitSnapshot snapshot;
		snapshot.unitID = unit.getUnitId();
		snapshot.unitTypeID = unit.getTypeId();
		snapshot.location = unit.getDeployLocation();
		snapshot.stats = unit.getStats();
		snapshot.team = unit.getTeam();
		return snapshot;
	}

	Unit* UnitSnapshot::toUnit() const
	{
		Unit* unit = new Unit();
		unit->setUnitId(unitID);
		unit->setTypeId(unitTypeID);
		unit->setDeployLocation(location);
		unit->setStats(stats);
		unit->setTeam(team);
		return unit;
	}


	//=======================================================================
	// CombatEventQueue
	//=======================================================================

	void CombatEventQueue::push(CombatEventBatch batch)
	{
		Mutex::Locker lock(mMutex);
		mQueue.push(std::move(batch));
	}

	bool CombatEventQueue::tryPop(CombatEventBatch& outBatch)
	{
		Mutex::Locker lock(mMutex);
		if (mQueue.empty())
			return false;

		outBatch = std::move(mQueue.front());
		mQueue.pop();
		return true;
	}

	bool CombatEventQueue::empty() const
	{
		Mutex::Locker lock(mMutex);
		return mQueue.empty();
	}

	void CombatEventQueue::clear()
	{
		Mutex::Locker lock(mMutex);
		while (!mQueue.empty())
			mQueue.pop();
	}

	//=======================================================================
	// CombatWork
	//=======================================================================

	CombatWork::CombatWork(
		uint32 combatID,
		int homeIndex,
		int awayIndex,
		TArray<UnitSnapshot> const& homeUnits,
		TArray<UnitSnapshot> const& awayUnits,
		CombatEventQueue* eventQueue,
		std::function<void(CombatResult)> onComplete)
		: mCombatID(combatID)
		, mHomePlayerIndex(homeIndex)
		, mAwayPlayerIndex(awayIndex)
		, mHomeUnits(homeUnits)
		, mAwayUnits(awayUnits)
		, mEventQueue(eventQueue)
		, mOnComplete(onComplete)
	{
	}

	void CombatWork::executeWork()
	{
		LogMsg("CombatWork: Combat %u started (Home=%d vs Away=%d)", 
			   mCombatID, mHomePlayerIndex, mAwayPlayerIndex);

		simulateAndStream();

		LogMsg("CombatWork: Combat %u completed", mCombatID);
	}

	void CombatWork::simulateAndStream()
	{
		// Create temporary World for combat simulation (thread-local)
		World tempWorld;
		// Configure deterministic local RNG BEFORE init to prevent consuming Global RNG
		tempWorld.setUseLocalRandom(true);
		tempWorld.setRandomSeed(mCombatID * 214013 + 2531011); // Deterministic seed

		tempWorld.init();
		
		// Configure World for infinite combat time so phase doesn't switch mid-sim
		tempWorld.setPhaseTimer(3600.0f); 
		tempWorld.changePhase(BattlePhase::Combat);

		// Enable event recording BEFORE setting up units so we catch initial states if needed
		tempWorld.setEventRecording(true);
		tempWorld.setSimulationTime(0.0f);

		bool bPVP = mAwayPlayerIndex != INDEX_NONE;


		// Get player reference (we'll use HOME player for simulation)
		Player& simPlayer = tempWorld.getPlayer(mHomePlayerIndex);
		PlayerBoard& board = simPlayer.getBoard();

		// Create units from snapshots
		TArray<Unit*> homeUnits;
		TArray<Unit*> awayUnits;

		// Create home units (player team)
		int homeIndex = 0;
		for (auto const& snapshot : mHomeUnits)
		{
			Unit* unit = snapshot.toUnit();
			unit->setTeam(UnitTeam::Player);
			auto location = unit->getDeployLocation();
			unit->place(board, location.pos);

			homeUnits.push_back(unit);
			simPlayer.mUnits.push_back(unit);
			homeIndex++;
		}

		// Create away units (enemy team)
		int awayIndex = 0;
		for (auto const& snapshot : mAwayUnits)
		{
			Unit* unit = snapshot.toUnit();
			unit->setTeam(UnitTeam::Enemy);
			// Mark as ghost so it gets updated in World::tick via mEnemyUnits loop
			// (World::tick only updates mEnemyUnits if they are ghosts, assuming real units are updated by their owner)
			unit->setIsGhost(true);
			auto location = unit->getDeployLocation();

			Vec2i target = location.pos;
			
			// Detect if unit is a Ghost (Pre-Mirrored on Server) using Offset
			bool bIsGhost = UnitIdHelper::IsGhost(unit->getUnitId());

			if (bPVP && !bIsGhost)
			{
				target = PlayerBoard::Mirror(target);
			}

			unit->place(board, target);

			awayUnits.push_back(unit);
			simPlayer.mEnemyUnits.push_back(unit);
			awayIndex++;
		}

		// Log Unit Positions for Debugging
		LogMsg("CombatSim Start: HomeUnits=%d AwayUnits=%d", homeUnits.size(), awayUnits.size());
		for (auto u : homeUnits) LogMsg("  HomeUnit[%d] Pos=(%.1f, %.1f)", u->getUnitId(), u->getPos().x, u->getPos().y);
		for (auto u : awayUnits) LogMsg("  AwayUnit[%d] Pos=(%.1f, %.1f)", u->getUnitId(), u->getPos().x, u->getPos().y);

		// Combat simulation parameters
		const float dt = 1.0f / 60.0f;  // 60 FPS
		const float maxBattleTime = 60.0f;
		const float flushInterval = 0.1f;  // Revert to 100ms for responsiveness
		const size_t maxBatchSize = 100;

		float elapsedTime = 0;
		float lastFlushTime = 0;
		uint32 batchIndex = 0;

		TArray<CombatEvent> eventBuffer;
		TArray<CombatEvent> allEvents;

		int frameCount = 0;
		const int maxFrames = (int)(maxBattleTime / dt);
		bool combatEnded = false;

		// Run combat simulation
		while (frameCount < maxFrames && !combatEnded)
		{
			// Update simulation time
			tempWorld.setSimulationTime(elapsedTime);

			// Actually tick the world to simulate combat
			tempWorld.tick(dt);
			
			elapsedTime += dt;
			frameCount++;

			// Check combat end condition
			int homeAlive = 0;
			int awayAlive = 0;

			for (auto unit : homeUnits)
				if (!unit->isDead()) homeAlive++;

			for (auto unit : awayUnits)
				if (!unit->isDead()) awayAlive++;

			if (homeAlive == 0 || awayAlive == 0)
			{
				combatEnded = true;
			}

			// Capture actual combat events from World
			auto const& recordedEvents = tempWorld.getRecordedEvents();
			for (auto const& event : recordedEvents)
			{
				eventBuffer.push_back(event);
				allEvents.push_back(event);
			}
			tempWorld.clearRecordedEvents();

			// Check if need to send event batch
			bool shouldFlush = eventBuffer.size() >= maxBatchSize ||
							   (elapsedTime - lastFlushTime) >= flushInterval;

			if (shouldFlush && !eventBuffer.empty())
			{
				float startTime = eventBuffer.front().timestamp;
				float endTime = eventBuffer.back().timestamp;
				LogMsg("CombatWork: Flushing %u events (Batch %u) TimeRange=[%.2f, %.2f]", 
					(uint32)eventBuffer.size(), batchIndex, startTime, endTime);
				
				for (auto const& evt : eventBuffer)
				{
					LogMsg("  > SvrEvent: T=%.3f Type=%d Src=%d Tgt=%d", 
						evt.timestamp, (int)evt.type, evt.sourceUnitID, evt.targetUnitID);
				}

				// Stream event batch
				CombatEventBatch batch;
				batch.combatID = mCombatID;
				batch.batchIndex = batchIndex++;
				batch.events = eventBuffer;

				if (mEventQueue)
				{
					mEventQueue->push(std::move(batch));
				}

				eventBuffer.clear();
				lastFlushTime = elapsedTime;
			}
		}

		// Send remaining events
		if (!eventBuffer.empty())
		{
			LogMsg("CombatWork: Flushing FINAL batch %u with %u events", 
				batchIndex, (uint32)eventBuffer.size());
			
			for (auto const& evt : eventBuffer)
			{
				LogMsg("  > SvrEvent: T=%.3f Type=%d Src=%d Tgt=%d", 
					evt.timestamp, (int)evt.type, evt.sourceUnitID, evt.targetUnitID);
			}

			CombatEventBatch batch;
			batch.combatID = mCombatID;
			batch.batchIndex = batchIndex++;
			batch.events = std::move(eventBuffer);

			if (mEventQueue)
			{
				mEventQueue->push(std::move(batch));
			}
		}

		// Calculate final statistics
		int homeAlive = 0;
		int awayAlive = 0;

		for (auto unit : homeUnits)
			if (!unit->isDead()) homeAlive++;

		for (auto unit : awayUnits)
			if (!unit->isDead()) awayAlive++;

		// Prepare result
		CombatResult result;
		result.combatID = mCombatID;
		result.homePlayerIndex = mHomePlayerIndex;
		result.awayPlayerIndex = mAwayPlayerIndex;
		
		// Determine winner
		if (homeAlive > awayAlive)
		{
			result.winnerIndex = mHomePlayerIndex;
			result.homeDamage = 0;
			result.awayDamage = 2 + awayAlive;  // Standard damage formula
		}
		else if (awayAlive > homeAlive)
		{
			result.winnerIndex = mAwayPlayerIndex;
			result.homeDamage = 2 + homeAlive;
			result.awayDamage = 0;
		}
		else
		{
			// Draw
			result.winnerIndex = -1;
			result.homeDamage = 0;
			result.awayDamage = 0;
		}
		
		result.duration = elapsedTime;
		result.allEvents = std::move(allEvents);

		// Callback notification of completion (executes in worker thread)
		if (mOnComplete)
		{
			mOnComplete(result);
		}

		tempWorld.cleanup();
	}

	//=======================================================================
	// CombatManager
	//=======================================================================

	CombatManager::CombatManager()
	{
	}

	CombatManager::~CombatManager()
	{
		shutdown();
	}

	void CombatManager::initialize(int numThreads)
	{
		LogMsg("CombatManager: Initializing with %d threads", numThreads);
		mThreadPool.init(numThreads);
		mActiveCombats = 0;
	}

	void CombatManager::shutdown()
	{
		LogMsg("CombatManager: Shutting down...");
		mThreadPool.waitAllWorkComplete();
		mEventQueue.clear();
		mActiveCombats = 0;
	}

	uint32 CombatManager::submitCombat(
		int homeIndex,
		int awayIndex,
		TArray<UnitSnapshot> const& homeUnits,
		TArray<UnitSnapshot> const& awayUnits,
		std::function<void(CombatResult)> onComplete)
	{
		uint32 combatID = mNextCombatID++;

		// Create combat work
		CombatWork* work = new CombatWork(
			combatID,
			homeIndex, awayIndex,
			homeUnits, awayUnits,
			&mEventQueue,
			[this, onComplete](CombatResult result) {
				// Callback wrapper: decrement active combat count
				mActiveCombats--;
				if (onComplete)
					onComplete(result);
			}
		);

		mActiveCombats++;
		mThreadPool.addWork(work);

		LogMsg("CombatManager: Combat %u submitted (Home=%d vs Away=%d)", 
			   combatID, homeIndex, awayIndex);

		return combatID;
	}

	TArray<CombatEventBatch> CombatManager::pollEventBatches(int maxBatches)
	{
		TArray<CombatEventBatch> batches;

		CombatEventBatch batch;
		int count = 0;
		while ((maxBatches == -1 || count < maxBatches) && mEventQueue.tryPop(batch))
		{
			batches.push_back(std::move(batch));
			count++;
		}

		return batches;
	}

	bool CombatManager::hasPendingCombats() const
	{
		return mActiveCombats > 0 || !mEventQueue.empty();
	}

} // namespace AutoBattler
