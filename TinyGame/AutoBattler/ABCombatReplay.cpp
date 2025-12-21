#include "ABPCH.h"
#include "ABCombatReplay.h"
#include "ABWorld.h"
#include "ABUnit.h"
#include "ABPlayer.h"
#include "LogSystem.h"

#include <unordered_map>

namespace AutoBattler
{
	CombatReplayManager::~CombatReplayManager()
	{
		clearAll();
	}

	void CombatReplayManager::setupReplay(
		uint32 combatID,
		int homeIndex, int awayIndex,
		TArray<UnitSnapshot> const& homeUnits,
		TArray<UnitSnapshot> const& awayUnits)
	{
		CombatReplay replay;
		replay.combatID = combatID;
		replay.homePlayerIndex = homeIndex;
		replay.awayPlayerIndex = awayIndex;
		replay.playbackTime = 0;
		replay.ended = false;
		replay.winnerIndex = -1;
		replay.playbackSpeed = 1.0f;

		mActiveReplays[combatID] = std::move(replay);
		LogMsg("CombatReplay: Setup combat %u (P%d vs P%d)", combatID, homeIndex, awayIndex);

		// Apply initial positions for replay
		// In network mode, away units don't exist in the main World yet
		// We need to create them from snapshots and place them at mirrored positions
		if (mWorld)
		{
			Player& homePlayer = mWorld->getPlayer(homeIndex);
			PlayerBoard& homeBoard = homePlayer.getBoard();

			// Create and place away units from snapshots
			for (auto const& snapshot : awayUnits)
			{
				// No-Copy Strategy:
				// Instead of creating a copy, we fetch the original unit and TELEPORT it to the mirror position.
				// This simplifies ID logic (no need to track copy IDs vs real IDs).
				Unit* awayUnit = findUnitByID(snapshot.unitID);

				if (awayUnit)
				{
					// Calculate mirrored position on Home Board
					int targetX = PlayerBoard::MapSize.x - 1 - snapshot.location.pos.x;
					int targetY = PlayerBoard::MapSize.y - 1 - snapshot.location.pos.y;
					
					// Remove from current board (wherever it is - usually Away Player's board)
					if (awayUnit->getInternalBoard())
					{
						Vec2i oldCoord = awayUnit->getInternalBoard()->getCoord(awayUnit->getPos());
						if (awayUnit->getInternalBoard()->isValid(oldCoord))
							awayUnit->getInternalBoard()->setUnit(oldCoord.x, oldCoord.y, nullptr);
					}

					// Place on Home Board at mirrored position
					// Use setUnit to update grid occupation
					homePlayer.getBoard().setUnit(targetX, targetY, awayUnit);
					awayUnit->setInternalBoard(&homePlayer.getBoard());

					// Update Transform
					Vector2 worldPos = homePlayer.getBoard().getWorldPos(targetX, targetY);
					// LogMsg("Teleport OrigUnit[%d] to (%d,%d) WorldPos=(%.1f, %.1f)", awayUnit->getUnitId(), targetX, targetY, worldPos.x, worldPos.y);
					awayUnit->setPos(worldPos);
					awayUnit->stopMove(); // Ensure stationary

					// Add to Enemy List reference (for easy access/cleanup logic)
					// Note: cleanupEnemies won't delete it because it's a real player unit.
					homePlayer.mEnemyUnits.push_back(awayUnit);

					// Sync Stats from Snapshot (optional but good for safety)
					// awayUnit->setStats(snapshot.stats); 
				}
				else
				{
					LogMsg("Error: SetupReplay could not find AwayUnit ID=%d", snapshot.unitID);
				}
			}

			// Activate Replay Mode immediately!
			// This prevents Unit::update() from running AI logic in the first frame
			// before CombatReplayManager::update() gets a chance to set it.
			mWorld->setReplayMode(true);
		}
	}

	void CombatReplayManager::enqueueEvents(uint32 combatID, TArray<CombatEvent> const& events)
	{
		auto it = mActiveReplays.find(combatID);
		if (it != mActiveReplays.end())
		{
			if (!events.empty())
			{
				float startTime = events[0].timestamp;
				float endTime = events.back().timestamp;
				LogMsg("Client Replay: Enqueue %d events for Combat %u (TimeRange=[%.2f, %.2f])", 
					events.size(), combatID, startTime, endTime);
			}
			for (auto const& event : events)
			{
				LogMsg("  < CliEvent: T=%.3f Type=%d Src=%d Tgt=%d", 
					event.timestamp, (int)event.type, event.sourceUnitID, event.targetUnitID);
				it->second.eventQueue.push(event);
			}
		}
		else
		{
			LogMsg("Client Replay Warning: Events for unknown Combat %u received", combatID);
		}
	}

	void CombatReplayManager::markEnded(uint32 combatID, int winnerIndex)
	{
		auto it = mActiveReplays.find(combatID);
		if (it != mActiveReplays.end())
		{
			it->second.ended = true;
			it->second.winnerIndex = winnerIndex;
			LogMsg("CombatReplay: Combat %u ended (winner=%d)", combatID, winnerIndex);
		}
	}

	void CombatReplayManager::update(float dt)
	{
		if (!mWorld)
			return;

		// Enable replay mode if we have active replays
		if (!mActiveReplays.empty() && !mWorld->isPlayingReplay())
		{
			mWorld->setReplayMode(true);
		}

		for (auto it = mActiveReplays.begin(); it != mActiveReplays.end();)
		{
			auto& replay = it->second;
			replay.playbackTime += dt * replay.playbackSpeed;

			while (!replay.eventQueue.empty())
			{
				auto const& event = replay.eventQueue.front();
				if (event.timestamp > replay.playbackTime)
					break;

				// Apply event through World
				applyCombatEvent(event);

				replay.eventQueue.pop();
			}

			if (replay.ended && replay.eventQueue.empty())
			{
				LogMsg("CombatReplay: Combat %u completed", replay.combatID);
				it = mActiveReplays.erase(it);
			}
			else
			{
				++it;
			}
		}

		// Disable replay mode if all replays finished
		if (mActiveReplays.empty() && mWorld->isPlayingReplay())
		{
			mWorld->setReplayMode(false);
		}
	}

	CombatReplay* CombatReplayManager::getReplay(uint32 combatID)
	{
		auto it = mActiveReplays.find(combatID);
		return (it != mActiveReplays.end()) ? &it->second : nullptr;
	}

	void CombatReplayManager::clearAll()
	{
		if (mWorld && !mActiveReplays.empty())
		{
			mWorld->setReplayMode(false);
		}
		mActiveReplays.clear();
		LogMsg("CombatReplay: Cleared all");
	}

	void CombatReplayManager::applyCombatEvent(CombatEvent const& event)
{
	if (!mWorld) return;

	switch (event.type)
	{
	case CombatEventType::UnitAttack:
	{
		Unit* source = findUnitByID(event.sourceUnitID);
		Unit* target = findUnitByID(event.targetUnitID);
		if (source && target)
		{
			mWorld->applyUnitAttack(*source, *target);
		}
	}
	break;
	case CombatEventType::UnitMove:
	{
		Unit* unit = findUnitByID(event.sourceUnitID);
		if (unit && unit->getInternalBoard())
		{
			Vec2i gridPos = unit->getInternalBoard()->getCoord(event.data.movePosition);
			mWorld->applyUnitMove(*unit, gridPos, false);
		}
	}
	break;
	case CombatEventType::UnitTakeDamage:
	{
		Unit* target = findUnitByID(event.targetUnitID);
		if (target)
		{
			mWorld->applyUnitDamage(*target, event.data.damageAmount);
		}
	}
	break;
	case CombatEventType::UnitDeath:
	{
		Unit* target = findUnitByID(event.sourceUnitID);
		if (target)
		{
			target->takeDamage(target->getStats().hp + 1.0f); // Fast kill
		}
	}
	break;
	case CombatEventType::UnitHeal:
	{
		Unit* target = findUnitByID(event.targetUnitID);
		if (target)
		{
			mWorld->applyUnitHeal(*target, event.data.healAmount);
		}
	}
	break;
	case CombatEventType::UnitCastSkill:
	{
		Unit* source = findUnitByID(event.sourceUnitID);
		Unit* target = findUnitByID(event.targetUnitID);
		if (source)
		{
			mWorld->applyUnitCastSkill(*source, event.data.skillID, target);
		}
	}
	break;
	case CombatEventType::UnitManaGain:
	{
		Unit* target = findUnitByID(event.sourceUnitID);
		if (target)
		{
			mWorld->applyUnitManaGain(*target, event.data.manaGained);
		}
	}
	break;
	}
}

Unit* CombatReplayManager::findUnitByID(int unitID)
{
	if (!mWorld)
		return nullptr;

	// Optimization: Decode PlayerIndex from UnitID (High 16 bits)
	int playerIndex = (unitID >> 16) & 0xFFFF;

		if (playerIndex >= 0 && playerIndex < AB_MAX_PLAYER_NUM)
		{
			Player& player = mWorld->getPlayer(playerIndex);
			for (Unit* unit : player.mUnits)
			{
				if (unit->getUnitId() == unitID)
					return unit;
			}
			// Just in case, try enemy list
			for (Unit* unit : player.mEnemyUnits)
			{
				if (unit->getUnitId() == unitID)
					return unit;
			}
		}

		// Fallback for legacy IDs
		for (int i = 0; i < AB_MAX_PLAYER_NUM; ++i)
		{
			Player& player = mWorld->getPlayer(i);

			for (Unit* unit : player.mUnits)
			{
				if (unit->getUnitId() == unitID)
					return unit;
			}
			for (Unit* unit : player.mEnemyUnits)
			{
				if (unit->getUnitId() == unitID)
					return unit;
			}
		}
		return nullptr;
	}

} // namespace AutoBattler
