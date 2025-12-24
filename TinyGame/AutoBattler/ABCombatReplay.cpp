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
		replay.playbackTime = -0.2f;
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

			// Reset Home Units (Important to clear stale targets from previous rounds)
			for (auto const& snapshot : homeUnits)
			{
				Unit* homeUnit = findUnitByID(snapshot.unitID);
				if (homeUnit)
				{
					homeUnit->resetRound();

					// Fix: Ensure home unit is attached to the board. 
					// Logic quirks in PVE/ListenServer might leave it detached.
					if (homeUnit->getInternalBoard() == nullptr)
					{
						LogWarning(0, "SetupReplay: HomeUnit %d was detached. Re-placing at (%d,%d)", 
							homeUnit->getUnitId(), snapshot.location.pos.x, snapshot.location.pos.y);
						
						if (homeBoard.isValid(snapshot.location.pos))
						{
							// Force clear target cell if needed (though it should be empty if unit is detached)
							// Actually, safely place it.
							homeUnit->place(homeBoard, snapshot.location.pos);
							homeUnit->stopMove();
						}
					}
					// Also sync position directly to ensure visual start matches snapshot
					else
					{
						// Optional: Teleport to snapshotted position if drifted?
						// homeUnit->setPos(homeBoard.getWorldPos(snapshot.location.pos.x, snapshot.location.pos.y));
					}
				}

			}

			// Create and place away units from snapshots
			for (auto const& snapshot : awayUnits)
			{
				// No-Copy Strategy:
				// Instead of creating a copy, we fetch the original unit and TELEPORT it to the mirror position.
				// This simplifies ID logic (no need to track copy IDs vs real IDs).
				Unit* awayUnit = findUnitByID(snapshot.unitID);

				if (!awayUnit)
				{
					// If unit not found (e.g. Ghost Unit with unique ID, or PVE unit not synced),
					// Create a visual clone for replay purposes.
					// LogMsg("SetupReplay: Creating Visual Clone for ID=%d", snapshot.unitID);
					awayUnit = new Unit();
					awayUnit->setUnitId(snapshot.unitID);
					awayUnit->setTypeId(snapshot.unitTypeID);
					awayUnit->setStats(snapshot.stats);
					awayUnit->setTeam(UnitTeam::Enemy); // Visual clones are enemies
					awayUnit->setIsGhost(true); // Always ghost/visual

					// Place it
					// Ghost Units (ID Offset) are pre-mirrored on Server. Do not mirror again.
					bool bIsGhost = UnitIdHelper::IsGhost(snapshot.unitID);
					Vec2i target = (awayIndex == -1 || bIsGhost) ? snapshot.location.pos : PlayerBoard::Mirror(snapshot.location.pos);
					
					if (homeBoard.isValid(target))
					{
						Unit* existingUnit = homeBoard.getUnit(target.x, target.y);
						if (existingUnit)
						{
							// LogWarning(0, "SetupReplay: Clone Collision at (%d,%d). Clearing.", target.x, target.y);
							homeBoard.setUnit(target.x, target.y, nullptr);
						}
					}

					awayUnit->place(homeBoard, target);
					homePlayer.mEnemyUnits.push_back(awayUnit);

					// Continue loop as we are done with this unit
					continue;
				}

				if (awayUnit)
				{
					// PVE units (Index -1) are already on the Home Board's coordinate system.
					// PVP units come from the Opponent's Board and need mirroring.
					// Ghosts (ID Offset) are pre-mirrored.
					bool bIsGhost = UnitIdHelper::IsGhost(snapshot.unitID);
					Vec2i target;
					
					if (awayIndex == -1 || bIsGhost)
					{
						target = snapshot.location.pos;
					}
					else
					{
						target = PlayerBoard::Mirror(snapshot.location.pos);
					}
					
					// Force Team to Enemy for Replay Visuals (so they look Red/Enemy color)
					awayUnit->setTeam(UnitTeam::Enemy);
					
					// Safety check: Ensure target cell is free before placing
					if (homeBoard.isValid(target))
					{
						Unit* existingUnit = homeBoard.getUnit(target.x, target.y);
						if (existingUnit && existingUnit != awayUnit)
						{
							LogWarning(0, "SetupReplay: Collision at (%d,%d). Removing existing unit ID=%d for ReplayUnit ID=%d", 
								target.x, target.y, existingUnit->getUnitId(), awayUnit->getUnitId());
							// Check if we should delete or just detach? 
							// Replay logic implies we are overriding state.
							// Detach simply:
							homeBoard.setUnit(target.x, target.y, nullptr);
						}
					}

					awayUnit->place(homeBoard, target);
					LogMsg("Teleport OrigUnit[%d] to (%d,%d)", awayUnit->getUnitId(), target.x, target.y);
					awayUnit->stopMove(); // Ensure stationary

					// Add to Enemy List reference (for easy access/cleanup logic)
					// Note: PVE units are already in this list from spawnPVEModule.
					bool bAlreadyInList = false;
					for (Unit* u : homePlayer.mEnemyUnits)
					{
						if (u == awayUnit)
						{
							bAlreadyInList = true;
							break;
						}
					}
					if (!bAlreadyInList)
					{
						homePlayer.mEnemyUnits.push_back(awayUnit);
					}

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
				// LogMsg("Client Replay: Enqueue %d events for Combat %u (TimeRange=[%.2f, %.2f])", 
				// 	events.size(), combatID, startTime, endTime);
			}
			for (auto const& event : events)
			{
				// LogMsg("  < CliEvent: T=%.3f Type=%d Src=%d Tgt=%d", 
				// 	event.timestamp, (int)event.type, event.sourceUnitID, event.targetUnitID);
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
				// Wait for effects to finish
				replay.finishTimer += dt;
				if (replay.finishTimer >= CombatReplay::FinishDelay)
				{
					LogMsg("CombatReplay: Combat %u completed (after delay)", replay.combatID);
					it = mActiveReplays.erase(it);
				}
				else
				{
					++it;
				}
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

	bool CombatReplayManager::isReplayFinished(uint32 combatID) const
	{
		auto it = mActiveReplays.find(combatID);
		if (it == mActiveReplays.end())
			return true;  // No replay = finished
		
		// Finished if marked ended AND no events left
		return it->second.ended && it->second.eventQueue.empty();
	}

	bool CombatReplayManager::allReplaysFinished() const
	{
		for (auto const& pair : mActiveReplays)
		{
			if (!pair.second.ended || !pair.second.eventQueue.empty())
				return false;
		}
		return true;
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
				Vector2 currentPos = unit->getPos();
				Vector2 targetPos = unit->getInternalBoard()->getWorldPos(event.data.moveTo.x, event.data.moveTo.y);
				LogMsg("Replay Move Unit %d: Cur=(%.1f, %.1f) Tgt=(%.1f, %.1f) Dist=%.1f", 
					unit->getUnitId(), currentPos.x, currentPos.y, targetPos.x, targetPos.y, Math::Distance(currentPos, targetPos));

				mWorld->applyUnitMove(*unit, event.data.moveTo, false);
			}
			else
			{
				LogMsg("Replay Move Failed: Unit %d not found or no board", event.sourceUnitID);
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
	int playerIndex = UnitIdHelper::GetPlayer(unitID);

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

	// Fallback: Search all players' mEnemyUnits (Ghost Clones might be stored in Opponent's list)
	for (int i = 0; i < AB_MAX_PLAYER_NUM; ++i)
	{
		Player& player = mWorld->getPlayer(i);
		for (Unit* unit : player.mEnemyUnits)
		{
			if (unit->getUnitId() == unitID)
				return unit;
		}
	}

	LogWarning(0,"Replay : Can't find unit %d", unitID);
	return nullptr;
}

} // namespace AutoBattler
