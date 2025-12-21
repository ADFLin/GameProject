#include "ABPCH.h"
#include "ABWorld.h"
#include "ABAction.h"
#include "ABDefine.h"

#include "RenderUtility.h"
#include "GameGlobal.h"
#include "GameGraphics2D.h"

namespace AutoBattler
{
	World::World()
	{
		mPhase = BattlePhase::Preparation;
		mPhaseTimer = 0.0f;
		mLocalPlayerIndex = 0;
		mRound = 0;
		mCombatEnded = false;
	}

	World::~World()
	{
		cleanup();
	}

	void World::init()
	{
		mRoundManager.init();
		mUnitDataManager.init();
		
		mUnitDataManager.init();
		
		mPlayers.resize(AB_MAX_PLAYER_NUM);
		int const GridCols = 3;
		// Calculate board dimensions + bench area + padding
		int boardWidth = PlayerBoard::MapSize.x * PlayerBoard::CellSize.x;
		int boardHeight = (int)(PlayerBoard::MapSize.y * PlayerBoard::CellSize.y * 0.75f) + AB_BENCH_OFFSET_Y + AB_BENCH_SLOT_SIZE;
		int const GapX = boardWidth + AB_BOARD_PADDING;
		int const GapY = boardHeight + AB_BOARD_PADDING;

		for (int i = 0; i < mPlayers.size(); ++i)
		{
			Player& player = mPlayers[i];
			player.mIndex = i;
			player.init(this);
			
			// Map 0..7 to 3x3 grid skipping center (index 4)
			int gridIndex = (i >= 4) ? i + 1 : i;
			int row = gridIndex / GridCols;
			int col = gridIndex % GridCols;

			player.getBoard().setOffset(Vector2(col * GapX, row * GapY));
		}		
	}

	void World::cleanup()
	{
		for (auto& player : mPlayers)
		{
			for (auto unit : player.mUnits)
				delete unit;
			player.mUnits.clear();

			for (auto unit : player.mBench)
				if (unit) delete unit;
			player.mBench.clear();

			for (auto unit : player.mEnemyUnits)
				delete unit;
			player.mEnemyUnits.clear();
		}
		mProjectiles.clear();
	}

	void World::restart()
	{
		cleanup(); // Clears units

		for (auto& player : mPlayers)
		{
			player.resetGame(); // Resets HP, Gold, Board


			// Give one starter unit on bench
			if (player.mBench.size() > 0)
			{
				Unit* unit = new Unit();
				unit->setUnitId(player.allocUnitID());
				// Default Stats?
				unit->setStats(mUnitDataManager.getUnit(1)->baseStats); // Assuming ID 1 exists
				unit->setTypeId(1);
				// Put on Board directly for testing
				player.mBench[0] = nullptr; 
				// player.mBench[0] = unit;
				// unit->setPos(player.getBenchSlotPos(0));
				
				// Center Front

				Vec2i pos = Vec2i(0, 7);
				player.getBoard().setUnit(pos.x, pos.y, unit);
				unit->setPos(player.getBoard().getWorldPos(pos.x, pos.y));
				unit->setInternalBoard(&player.getBoard());

				UnitLocation loc;
				loc.type = ECoordType::Board;
				loc.pos = pos;

				unit->setHoldLocation(loc);

				// Add to active units list
				player.mUnits.push_back(unit);
			}
		}

		mRound = 0;
		changePhase(BattlePhase::Preparation);
	}

	void World::cleanupEnemies()
	{
		// Cleanup previous enemies for ALL players
		for (auto& player : mPlayers)
		{
			PlayerBoard& board = player.getBoard();
			for (auto unit : player.mEnemyUnits)
			{
				Vec2i coord = board.getCoord(unit->getPos());
				if (board.isValid(coord) && board.getUnit(coord.x, coord.y) == unit)
					board.setUnit(coord.x, coord.y, nullptr);
				
				// Only delete if this is NOT a real player unit (i.e., it's a PVE enemy or ghost clone)
				// Check if this unit belongs to any player's mUnits
				bool isRealPlayerUnit = false;
				for (auto& otherPlayer : mPlayers)
				{
					for (auto playerUnit : otherPlayer.mUnits)
					{
						if (playerUnit == unit)
						{
							isRealPlayerUnit = true;
							break;
						}
					}
					if (isRealPlayerUnit) break;
				}
				
				if (!isRealPlayerUnit)
				{
					delete unit;
				}
			}
			player.mEnemyUnits.clear();
		}
	}

	void World::setupRound()
	{
		// Round Logic
		// Round is 0-indexed in code? 
		// "mRound++" happens in changePhase::Preparation.
		// Start m Round=0. Preparation -> mRound=1.
		// So we want config for mRound-1 if mRound is 1-based (UI).
		// Code logic: mRound initialized 0. In Prep: mRound++. Current Round 1.
		
		RoundConfig const& roundConfig = mRoundManager.getRoundConfig(mRound - 1);

		bool isPVE = (roundConfig.type == RoundType::PVE);

		if (isPVE)
		{
			mMatches.clear();
			// PVE: Spawn enemies for each player
			for (int i = 0; i < mPlayers.size(); ++i)
			{
				spawnPVEModule(i, mRound);
			}
		}
		else
		{
			// PVP: Generate match pairings
			generateMatches();
			
			// Only teleport units in standalone mode
			// In network mode, combat simulation handles unit positioning
			if (!mbNetworkMode)
			{
				teleportMatchedUnits();
				LogMsg("setupRound: Standalone mode - units teleported locally");
			}
			else
			{
				// In network mode, we only spawn GHOSTS locally for visualization.
				// Real units are synced via network entity system.
				teleportNetworkGhosts();
				LogMsg("setupRound: Network mode - syncing ghosts if any");
			}
		}
	}

	void World::teleportNetworkGhosts()
	{
		for (auto const& match : mMatches)
		{
			if (match.isGhost)
			{
				teleportUnits(match.away, match.home, true);
			}
		}
	}

	void World::spawnPVEModule(int playerIndex, int round)
	{
		if (!mPlayers.isValidIndex(playerIndex)) return;
		Player& player = mPlayers[playerIndex];
		PlayerBoard& board = player.getBoard();

		RoundConfig const& roundConfig = mRoundManager.getRoundConfig(round - 1);

		for(auto const& enemyInfo : roundConfig.enemies)
		{
			Unit* enemy = new Unit();
			enemy->setTeam(UnitTeam::Enemy);
			enemy->setInternalBoard(&board);

			Vector2 pos = board.getWorldPos(enemyInfo.gridPos.x, enemyInfo.gridPos.y);
			enemy->setPos(pos);
			
			UnitLocation loc;
			loc.type = ECoordType::Board;
			loc.pos = enemyInfo.gridPos;
			enemy->setHoldLocation(loc);

			board.setUnit(enemyInfo.gridPos.x, enemyInfo.gridPos.y, enemy);
			
			player.mEnemyUnits.push_back(enemy);

			enemy->setStats(enemyInfo.stats);
			enemy->setUnitId(enemyInfo.unitId);
		}
	}

	void World::teleportUnits(int fromIdx, int toIdx, bool isGhost)
	{
		Player& fromPlayer = mPlayers[fromIdx];
		Player& toPlayer = mPlayers[toIdx];
		PlayerBoard& toBoard = toPlayer.getBoard();

		if (!isGhost)
		{
			// Move ACTUAL units
			for (auto unit : fromPlayer.mUnits)
			{
				Vec2i basePos = PlayerBoard::Mirror(unit->getHoldLocation().pos);
				Vec2i targetPos = basePos;

				unit->setTeam(UnitTeam::Enemy);
				unit->place(toBoard, targetPos);
				// Vital: Add to Home Player's Enemy List so they can be targeted
				toPlayer.mEnemyUnits.push_back(unit);
			}
		}
		else
		{
			// Ghost: Clone units as mEnemyUnits of Home Player
			for (auto unit : fromPlayer.mUnits)
			{
				Unit* ghost = new Unit();
				ghost->setIsGhost(true);
				ghost->setUnitId(unit->getUnitId());
				ghost->setStats(unit->getStats());
				Vec2i targetPos = PlayerBoard::Mirror(unit->getHoldLocation().pos);
				ghost->place(toBoard, targetPos);

				toPlayer.mEnemyUnits.push_back(ghost);
			}
		}
	}

	// =======================================================================
	// PVP Match Generation & Unit Teleportation (Refactored)
	// =======================================================================

	void World::generateMatches()
	{
		// 1. Clear previous matches
		mMatches.clear();

		// 2. Identify eligible players
		TArray<int> activePlayers;
		LogMsg("MatchGen: Checking Active Players:");
		for (int i = 0; i < mPlayers.size(); ++i)
		{
			if (mPlayers[i].getHp() > 0)
			{
				activePlayers.push_back(i);
				LogMsg("  Player %d (HP: %d)", i, mPlayers[i].getHp());
			}
		}

		if (activePlayers.empty()) 
			return;

		// 3. Shuffle
		for (int i = 0; i < activePlayers.size(); ++i)
		{
			int swapIdx = getRand() % activePlayers.size();
			std::swap(activePlayers[i], activePlayers[swapIdx]);
		}

		// 4. Pair them
		while (activePlayers.size() >= 2)
		{
			int p1 = activePlayers.back(); activePlayers.pop_back();
			int p2 = activePlayers.back(); activePlayers.pop_back();
			
			MatchPair match;
			match.home = p1;
			match.away = p2;
			match.isGhost = false;
			mMatches.push_back(match);

			LogMsg("MatchGen: Pair %d vs %d", p1, p2);
		}

		// 5. Handle odd player (Ghost Match)
		if (!activePlayers.empty())
		{
			int oddPlayer = activePlayers.back(); activePlayers.pop_back();

			// Pick a random opponent (who is not self) from ALIVE players
			TArray<int> potentialOpponents;
			for (int i = 0; i < mPlayers.size(); ++i) 
			{
				if (i != oddPlayer && mPlayers[i].getHp() > 0) 
					potentialOpponents.push_back(i);
			}

			if (!potentialOpponents.empty())
			{
				int opponentIdx = potentialOpponents[getRand() % potentialOpponents.size()];
				
				MatchPair match;
				match.home = oddPlayer;
				match.away = opponentIdx;
				match.isGhost = true;
				mMatches.push_back(match);

				LogMsg("MatchGen: Ghost match %d vs %d", oddPlayer, opponentIdx);
			}
		}
	}

	void World::teleportMatchedUnits()
	{
		LogMsg("Teleport Matched Units. Matches: %d", mMatches.size());

		// Teleport units for all generated matches
		for (auto const& match : mMatches)
		{
			teleportUnits(match.away, match.home, match.isGhost);
		}
	}

	void World::restoreUnits()
	{
		for (auto& player : mPlayers)
		{
			PlayerBoard& board = player.getBoard();
			for (auto unit : player.mUnits)
			{
				// Set internal board to owner's board
				unit->setInternalBoard(&board);

				unit->restoreStartState();

				// Ensure unit is set to correct team
				unit->setTeam(UnitTeam::Player);

				// Re-register to grid on the OWNER's board (not current internal board)
				Vec2i coord = board.getCoord(unit->getPos());
				if (board.isValid(coord))
				{
					board.setUnit(coord.x, coord.y, unit);
				}
			}
		}
	}



	void World::resolveCombat()
	{
		// 1. PVP Matches
		if (!mMatches.empty())
		{
			for (auto& match : mMatches)
			{
				Player& homeP = mPlayers[match.home];
				Player& awayP = mPlayers[match.away];

				// Count alive units
				int homeAlive = 0;
				for (auto u : homeP.mUnits) if (!u->isDead()) homeAlive++;

				int awayAlive = 0;
				if (match.isGhost)
				{
					for (auto u : homeP.mEnemyUnits) if (!u->isDead()) awayAlive++;
				}
				else
				{
					for (auto u : awayP.mUnits) if (!u->isDead()) awayAlive++;
				}

				// Apply combat result using shared method
				// For ghost matches, treat away as -1 (PVE-like for away player)
				int awayIndex = match.isGhost ? -1 : match.away;
				applyCombatResult(match.home, awayIndex, homeAlive, awayAlive, false);
			}
		}
		// 2. PVE (No Matches)
		else
		{
			for (int i = 0; i < AB_MAX_PLAYER_NUM; ++i)
			{
				Player& player = mPlayers[i];
				
				// Count alive units
				int playerAlive = 0;
				for (auto u : player.mUnits) if (!u->isDead()) playerAlive++;

				int enemyAlive = 0;
				for (auto u : player.mEnemyUnits) if (!u->isDead()) enemyAlive++;

				// Apply PVE result
				applyCombatResult(i, -1, playerAlive, enemyAlive, true);
			}
		}
	}

	void World::applyCombatResult(int homePlayerIndex, int awayPlayerIndex, 
	                               int homeAlive, int awayAlive, bool isPVE)
	{
		// Apply result for home player
		if (homePlayerIndex >= 0 && homePlayerIndex < AB_MAX_PLAYER_NUM)
		{
			Player& homePlayer = mPlayers[homePlayerIndex];
			
			if (homeAlive > 0 && awayAlive == 0)
			{
				// Home player won
				homePlayer.addGold(1);
				LogMsg("  Player %d won, +1 gold", homePlayerIndex);
			}
			else if (awayAlive > 0 && homeAlive == 0)
			{
				// Home player lost
				int damage = 2 + awayAlive;
				homePlayer.takeDamage(damage);
				LogMsg("  Player %d lost, -%d HP (HP: %d)", 
				       homePlayerIndex, damage, homePlayer.getHp());
			}
			// else: draw, no rewards or penalties
		}
		
		// Apply result for away player (only for PVP, not PVE)
		if (!isPVE && awayPlayerIndex >= 0 && awayPlayerIndex < AB_MAX_PLAYER_NUM)
		{
			Player& awayPlayer = mPlayers[awayPlayerIndex];
			
			if (awayAlive > 0 && homeAlive == 0)
			{
				// Away player won
				awayPlayer.addGold(1);
				LogMsg("  Player %d won, +1 gold", awayPlayerIndex);
			}
			else if (homeAlive > 0 && awayAlive == 0)
			{
				// Away player lost
				int damage = 2 + homeAlive;
				awayPlayer.takeDamage(damage);
				LogMsg("  Player %d lost, -%d HP (HP: %d)", 
				       awayPlayerIndex, damage, awayPlayer.getHp());
			}
		}
	}

	void World::tick(float dt)
	{
		if (mPhase != BattlePhase::End)
		{
			mPhaseTimer -= dt;

			if (mPhase == BattlePhase::Combat && !mCombatEnded)
			{
				bool allCombatsEnded = true;
				
				if (!mMatches.empty())
				{
					// Check Matches
					for (auto& match : mMatches)
					{
						Player& homeP = mPlayers[match.home];
						Player& awayP = mPlayers[match.away];

						int homeAlive = 0;
						for (auto u : homeP.mUnits) if (!u->isDead()) homeAlive++;

						int awayAlive = 0;
						if (match.isGhost)
						{
							for (auto u : homeP.mEnemyUnits) if (!u->isDead()) awayAlive++;
						}
						else
						{
							for (auto u : awayP.mUnits) if (!u->isDead()) awayAlive++;
						}

						if (homeAlive > 0 && awayAlive > 0)
						{
							allCombatsEnded = false;
							break;
						}
					}
				}
				else
				{
					// PVE Check
					for (auto& player : mPlayers)
					{
						if (player.getHp() <= 0) continue; 
						
						int playerAlive = 0;
						for (auto u : player.mUnits) if (!u->isDead()) playerAlive++;

						int enemyAlive = 0;
						for (auto u : player.mEnemyUnits) if (!u->isDead()) enemyAlive++;

						if (playerAlive > 0 && enemyAlive > 0)
						{
							allCombatsEnded = false;
							break;
						}
					}
				}

				if (allCombatsEnded)
				{
					if (mAutoResolveCombat)
					{
						mCombatEnded = true;
						resolveCombat();
						if (mPhaseTimer > 3.0f) mPhaseTimer = 3.0f;
					}
				}
			}

			if (mPhaseTimer <= 0)
			{
				switch (mPhase)
				{
				case BattlePhase::Preparation: changePhase(BattlePhase::Combat); break;
				case BattlePhase::Combat:      changePhase(BattlePhase::Preparation); break;
				}
			}
		}

		updateProjectiles(dt);

		static int updateFrame = 0;
		updateFrame++;

		int numPlayers = (int)mPlayers.size();
		for (int i = 0; i < numPlayers; ++i)
		{
			int idx = (updateFrame % 2 == 0) ? i : (numPlayers - 1 - i);
			Player& player = mPlayers[idx];

			for (auto unit : player.mUnits)
				unit->update(dt, *this);

			for (auto unit : player.mEnemyUnits)
			{
				if (unit->isGhost())
					unit->update(dt, *this);
			}
		}
	}

	void World::changePhase(BattlePhase phase)
{
	LogMsg("ChangePhase: %d Matches: %d", (int)phase, mMatches.size());
	mPhase = phase;
		switch (mPhase)
		{
		case BattlePhase::Preparation:
		{
			int alivePlayers = 0;
			for (auto& player : mPlayers)
			{
				if (player.getHp() > 0)
					alivePlayers++;
			}

			if (alivePlayers <= 1 && mRound > 0) // End game if only 1 standing (and strictly > 0 rounds to avoid instant end)
			{
				// mPhase = BattlePhase::End;
				// return;
				// Keep going for testing?
			}

			mRound++;
			mPhaseTimer = AB_PHASE_PREP_TIME;

			for (auto& player : mPlayers)
			{
				player.getBoard().setup(); // Clear occupation
				
				// Restore Units must happen after clearing occupation?
				// restoreUnits logic clears/overwrites occupation.
			}
			
			restoreUnits(); // Register units to board
			cleanupEnemies(); // Cleanup old enemies

			for (auto& player : mPlayers)
			{
				if (player.getHp() <= 0) continue;

				// Income
				int interest = Math::Min(player.getGold() / 10, 5);
				int income = 5 + interest;
				player.addGold(income);
				player.refreshShop();
		}
	}
	break;
	case BattlePhase::Combat:
			mPhaseTimer = 3600.0f; // Infinite time (wait for combat end)
			mCombatEnded = false;
			setupRound(); // Spawn/Teleport Enemies
			break;
		}
	}

	void World::render(IGraphics2D& g)
	{
		// Render All Players
		for (auto& player : mPlayers)
		{
			player.render(g);

			bool bShowState = (mPhase == BattlePhase::Combat);
			for (auto unit : player.mEnemyUnits)
				unit->render(g, bShowState);
            
            for (auto unit : player.mUnits)
                unit->render(g, bShowState);
			
			// Bench units are rendered inside player.render(g).
			// Player units (active on board) are rendered how?
			// Player::render DOES NOT render active units on board.
			// mBoard.render(g) draws the grid.
			// Logic in player.render():
			// 1. mBoard.render(g)
			// 2. Iterate mBench and render circles.
			// WHERE ARE mUnits rendered?
			// Oh, previously ABWorld.cpp loop:
			// for (auto unit : player.mUnits) unit->render(g);
			// This was removed in Step 1528.
			// BAD. Step 1528 replaced board.render() AND mUnits loop with player.render().
			// But player.render() implementation (Step 1522) only renders Board and Bench.
			// It does NOT render mUnits (Active units).
			
			// FIX: We must render active units here OR in player.render().
			// Rendering here is better for layering?
			// Let's add them back here.
			for (auto unit : player.mUnits)
				unit->render(g, bShowState);
		}
		renderProjectiles(g);
	}

	void World::addProjectile(Unit* owner, Unit* target, float speed, float damage)
	{
		Projectile p;
		p.owner = owner;
		p.pos = owner->getPos();
		p.target = target;
		p.targetPos = target->getPos();
		p.speed = speed;
		p.damage = damage;
		mProjectiles.push_back(p);
	}

	void World::updateProjectiles(float dt)
	{
		for (int i = 0; i < mProjectiles.size(); )
		{
			Projectile& p = mProjectiles[i];
			if (p.target && !p.target->isDead())
			{
				p.targetPos = p.target->getPos();
			}

			Vector2 dir = p.targetPos - p.pos;
			float dist = dir.length();
			float moveDist = p.speed * dt;

			if (dist <= moveDist)
			{
				// Hit
				if (p.target && !p.target->isDead())
				{
					// In Replay mode, damage is handled via UnitTakeDamage events
					if (!mPlayingReplay)
					{
						p.target->takeDamage(p.damage);
					}
					// Mana gain for attacker? Handled in attack() usually. 
					// But if we defer damage, maybe mana should be deferred? 
					// Simplest: Attacker mana gained on spawn (Visual Cast), Victim mana gained on hit.
				}
				mProjectiles.erase(mProjectiles.begin() + i);
			}
			else
			{
				dir /= dist;
				p.pos += dir * moveDist;
				++i;
			}
		}
	}

	void World::renderProjectiles(IGraphics2D& g)
	{
		RenderUtility::SetBrush(g, EColor::Orange);
		RenderUtility::SetPen(g, EColor::White);
		for (auto const& p : mProjectiles)
		{
			g.drawCircle(p.pos, 5);
		}
	}

	Unit* World::getNearestEnemy(Vector2 const& pos, UnitTeam team, PlayerBoard* board)
	{
		if (!board) return nullptr;
		Player* player = board->getOwner();
		if (!player) return nullptr;

		TArray<Unit*>* targetList = nullptr;
		if (team == UnitTeam::Player)
			targetList = &player->mEnemyUnits;
		else
			targetList = &player->mUnits;

		Unit* bestUnit = nullptr;
		float bestDistSq = FLT_MAX;

		for (Unit* unit : *targetList)
		{
			if (unit->isDead())
				continue;

			float distSq = (unit->getPos() - pos).length2();
			if (distSq < bestDistSq)
			{
				bestDistSq = distSq;
				bestUnit = unit;
			}
		}
		return bestUnit;
	}

	// =======================================================================
	// Server端Action驗證
	// =======================================================================

	bool World::validateAction(int playerIndex, ABActionItem const& action, ActionError* outError)
	{
		if (playerIndex < 0 || playerIndex >= (int)mPlayers.size())
		{
			if (outError)
			{
				outError->code = ActionError::INVALID_PLAYER;
				outError->message = "Invalid player index";
			}
			return false;
		}

		Player const& player = mPlayers[playerIndex];

		switch (action.type)
		{
		case ACT_BUY_UNIT:
			return validateBuyUnit(player, action.buy, outError);
		case ACT_SELL_UNIT:
			return validateSellUnit(player, action.sell, outError);
		case ACT_REFRESH_SHOP:
			return validateRefreshShop(player, outError);
		case ACT_LEVEL_UP:
			return validateLevelUp(player, outError);
		case ACT_DEPLOY_UNIT:
			return validateDeployUnit(player, action.deploy, outError);
		case ACT_RETRACT_UNIT:
			return validateRetractUnit(player, action.retract, outError);
		case ACT_SYNC_DRAG:
			return true; // Visual sync only
		default:
			if (outError)
			{
				outError->code = ActionError::INVALID_SOURCE;
				outError->message = "Unknown action type";
			}
			return false;
		}
	}

	bool World::validateBuyUnit(Player const& player, ActionBuyUnit const& action, ActionError* outError)
	{
		// 檢查階段
		if (mPhase != BattlePhase::Preparation)
		{
			if (outError)
			{
				outError->code = ActionError::PHASE_MISMATCH;
				outError->message = "Can only buy units in preparation phase";
			}
			return false;
		}

		// 檢查Shop slot
		if (action.slotIndex >= (int)player.mShopList.size())
		{
			if (outError)
			{
				outError->code = ActionError::INVALID_POSITION;
				outError->message = "Invalid shop slot index";
			}
			return false;
		}

		int unitId = player.mShopList[action.slotIndex];
		if (unitId < 0)
		{
			if (outError)
			{
				outError->code = ActionError::SHOP_SLOT_EMPTY;
				outError->message = "Shop slot is empty";
			}
			return false;
		}

		// TODO: 檢查金幣
		// 需要查詢UnitDataManager獲取單位價格

		// 檢查Bench空位
		bool hasBenchSpace = false;
		for (auto* unit : player.mBench)
		{
			if (unit == nullptr)
			{
				hasBenchSpace = true;
				break;
			}
		}

		if (!hasBenchSpace)
		{
			if (outError)
			{
				outError->code = ActionError::BENCH_FULL;
				outError->message = "Bench is full";
			}
			return false;
		}

		return true;
	}

	bool World::validateSellUnit(Player const& player, ActionSellUnit const& action, ActionError* outError)
	{
		if (mPhase != BattlePhase::Preparation)
		{
			if (outError)
			{
				outError->code = ActionError::PHASE_MISMATCH;
				outError->message = "Can only sell units in preparation phase";
			}
			return false;
		}

		// 檢查slotIndex是否有單位
		if (action.slotIndex >= (int)player.mBench.size())
		{
			if (outError)
			{
				outError->code = ActionError::INVALID_POSITION;
				outError->message = "Invalid bench slot";
			}
			return false;
		}

		if (player.mBench[action.slotIndex] == nullptr)
		{
			if (outError)
			{
				outError->code = ActionError::NO_UNIT;
				outError->message = "No unit in this slot";
			}
			return false;
		}

		return true;
	}

	bool World::validateRefreshShop(Player const& player, ActionError* outError)
	{
		if (mPhase != BattlePhase::Preparation)
		{
			if (outError)
			{
				outError->code = ActionError::PHASE_MISMATCH;
				outError->message = "Can only refresh shop in preparation phase";
			}
			return false;
		}

		const int REFRESH_COST = 2;
		if (player.getGold() < REFRESH_COST)
		{
			if (outError)
			{
				outError->code = ActionError::INSUFFICIENT_GOLD;
				outError->message = "Not enough gold";
			}
			return false;
		}

		return true;
	}

	bool World::validateLevelUp(Player const& player, ActionError* outError)
	{
		if (mPhase != BattlePhase::Preparation)
		{
			if (outError)
			{
				outError->code = ActionError::PHASE_MISMATCH;
				outError->message = "Can only buy XP in preparation phase";
			}
			return false;
		}

		const int XP_COST = 4;
		if (player.getGold() < XP_COST)
		{
			if (outError)
			{
				outError->code = ActionError::INSUFFICIENT_GOLD;
				outError->message = "Not enough gold";
			}
			return false;
		}

		return true;
	}

	bool World::validateDeployUnit(Player const& player, ActionDeployUnit const& action, ActionError* outError)
	{
		if (mPhase != BattlePhase::Preparation)
		{
			if (outError)
			{
				outError->code = ActionError::PHASE_MISMATCH;
				outError->message = "Can only deploy units in preparation phase";
			}
			return false;
		}

		// 檢查目標位置是否合法
		PlayerBoard const& board = player.getBoard();
		if (action.destX < 0 || action.destX >= PlayerBoard::MapSize.x ||
			action.destY < 0 || action.destY >= PlayerBoard::MapSize.y)
		{
			if (outError)
			{
				outError->code = ActionError::INVALID_DESTINATION;
				outError->message = "Invalid destination position";
			}
			return false;
		}

		// 檢查源位置（簡化檢查，實際執行時會更詳細）
		if (action.srcType == 1) // From Bench
		{
			if (action.srcX < 0 || action.srcX >= (int)player.mBench.size())
			{
				if (outError)
				{
					outError->code = ActionError::INVALID_SOURCE;
					outError->message = "Invalid bench slot";
				}
				return false;
			}

			if (player.mBench[action.srcX] == nullptr)
			{
				if (outError)
				{
					outError->code = ActionError::NO_UNIT;
					outError->message = "No unit in source position";
				}
				return false;
			}
		}

		return true;
	}

	bool World::validateRetractUnit(Player const& player, ActionRetractUnit const& action, ActionError* outError)
	{
		if (mPhase != BattlePhase::Preparation)
		{
			if (outError)
			{
				outError->code = ActionError::PHASE_MISMATCH;
				outError->message = "Can only retract units in preparation phase";
			}
			return false;
		}

		// 檢查Bench是否有空位
		bool hasBenchSpace = false;
		for (auto* unit : player.mBench)
		{
			if (unit == nullptr)
			{
				hasBenchSpace = true;
				break;
			}
		}

		if (!hasBenchSpace)
		{
			if (outError)
			{
				outError->code = ActionError::BENCH_FULL;
				outError->message = "Bench is full";
			}
			return false;
		}

		// 檢查源位置是否有單位（簡化）
		PlayerBoard const& board = player.getBoard();
		if (action.srcX < 0 || action.srcX >= PlayerBoard::MapSize.x ||
			action.srcY < 0 || action.srcY >= PlayerBoard::MapSize.y)
		{
			if (outError)
			{
				outError->code = ActionError::INVALID_SOURCE;
				outError->message = "Invalid source position";
			}
			return false;
		}

		return true;
	}

	void World::setReplayMode(bool enabled)
	{
		mPlayingReplay = enabled;
		LogMsg("World: Replay mode %s", enabled ? "ENABLED" : "DISABLED");
	}

	void World::recordEvent(struct CombatEvent const& event)
	{
		CHECK(mRecordingEvents);
		{
			// Add event to list
			mRecordedEvents.push_back(event);
		}
	}

	int World::getRand()
	{
		if (!mUseLocalRandom)
		{
			return ::Global::RandomNet();
		}
		
		// Simple LCG
		mRandomSeed = mRandomSeed * 1103515245 + 12345;
		return (unsigned int)(mRandomSeed / 65536) % 32768;
	}

	void World::applyUnitMove(Unit& unit, Vec2i const& moveTo, bool immediate)
	{
		PlayerBoard* board = unit.getInternalBoard();
		if (!board) return;

		// Record move event
		if (mRecordingEvents && !mPlayingReplay)
		{
			CombatEvent event;
			event.type = CombatEventType::UnitMove;
			event.timestamp = mSimulationTime;
			event.sourceUnitID = unit.getUnitId();
			event.targetUnitID = -1;
			Vector2 targetPos = board->getWorldPos(moveTo.x, moveTo.y);
			event.data.movePosition = targetPos;
			recordEvent(event);
		}

		// Logic: Update Board Occupation
	Vec2i currentGrid = board->getCoord(unit.getPos());
	if (board->isValid(currentGrid))
	{
		Unit* occupant = board->getUnit(currentGrid.x, currentGrid.y);
		if (occupant == &unit)
		{
			board->setUnit(currentGrid.x, currentGrid.y, nullptr);
		}
	}

	// Set new position
	board->setUnit(moveTo.x, moveTo.y, &unit);

	Vector2 targetPos = board->getWorldPos(moveTo.x, moveTo.y);
	if (immediate)
	{
		// Teleport instantly (used for replay start)
		unit.setPos(targetPos);
		// Need to update next cell too?
		// No, startVisualMove handles dynamic movement. 
		// If teleporting, we are "there".
		unit.stopMove(); // Stop any interpolation
	}
	else
	{
		// Start smooth movement
		unit.startVisualMove(targetPos);
	}
}

	void World::applyUnitAttack(Unit& attacker, Unit& target)
	{
		// Record attack event (skip if replaying)
		if (mRecordingEvents && !mPlayingReplay)
		{
			CombatEvent event;
			event.type = CombatEventType::UnitAttack;
			event.timestamp = mSimulationTime;
			event.sourceUnitID = attacker.getUnitId();
			event.targetUnitID = target.getUnitId();
			recordEvent(event);
		}

		// Gain mana on attack
		UnitStats stats = attacker.getStats();
		stats.mana = Math::Min(stats.mana + 10.0f, stats.maxMana);
		attacker.setStats(stats);

		if (stats.range > 100.0f)
		{
			// Ranged: spawn projectile
			addProjectile(&attacker, &target, 500.0f, stats.attackDamage);
		}
		else
		{
			// Melee: instant damage (always apply, recording handled inside applyUnitDamage)
			applyUnitDamage(target, stats.attackDamage);
		}
	}

	void World::applyUnitDamage(Unit& target, float damage)
	{
		// Record damage event (skip if replaying)
		if (mRecordingEvents && !mPlayingReplay)
		{
			CombatEvent event;
			event.type = CombatEventType::UnitTakeDamage;
			event.timestamp = mSimulationTime;
			event.sourceUnitID = -1;
			event.targetUnitID = target.getUnitId();
			event.data.damageAmount = damage;
			recordEvent(event);
		}

		UnitStats stats = target.getStats();
		stats.hp -= damage;

		if (stats.hp <= 0)
		{
			// Unit died
			stats.hp = 0;
			target.setStats(stats);

			// Record death event
			if (mRecordingEvents)
			{
				CombatEvent deathEvent;
				deathEvent.type = CombatEventType::UnitDeath;
				deathEvent.timestamp = mSimulationTime;
				deathEvent.sourceUnitID = target.getUnitId();
				deathEvent.targetUnitID = -1;
				recordEvent(deathEvent);
			}

			// Mark as dead (handled by Unit internally via setDead or state)
			// But we need to ensure death state is set
			// For simplicity, we rely on Unit::takeDamage logic
			// So let's just call it
			target.takeDamage(damage);
			return;
		}
		else
		{
			// Gain mana when taking damage
			stats.mana = Math::Min(stats.mana + 5.0f, stats.maxMana);
		}

		target.setStats(stats);
	}

	void World::applyUnitHeal(Unit& target, float healAmount)
	{
		// Record heal event
		if (mRecordingEvents)
		{
			CombatEvent event;
			event.type = CombatEventType::UnitHeal;
			event.timestamp = mSimulationTime;
			event.sourceUnitID = -1;
			event.targetUnitID = target.getUnitId();
			event.data.healAmount = healAmount;
			recordEvent(event);
		}

		UnitStats stats = target.getStats();
		stats.hp = Math::Min(stats.hp + healAmount, stats.maxHp);
		target.setStats(stats);
	}

	void World::applyUnitBuff(Unit& target, int buffType, float buffValue, float duration)
	{
		// Record buff event
		if (mRecordingEvents)
		{
			CombatEvent event;
			event.type = CombatEventType::UnitBuffApplied;
			event.timestamp = mSimulationTime;
			event.sourceUnitID = -1;
			event.targetUnitID = target.getUnitId();
			event.data.buff.type = buffType;
			event.data.buff.value = buffValue;
			event.data.buff.duration = duration;
			recordEvent(event);
		}

		UnitStats stats = target.getStats();

		switch (buffType)
		{
		case 1: // Attack buff
			stats.attackDamage *= (1.0f + buffValue);
			break;
		case 2: // Defense/Armor buff
			stats.armor *= (1.0f + buffValue);
			break;
		case 3: // Speed buff
			stats.attackSpeed *= (1.0f + buffValue);
			break;
		}

		target.setStats(stats);

		// TODO: Track buff duration and remove when expired
		// For now, buffs are permanent once applied
	}

	void World::applyUnitCastSkill(Unit& caster, int skillID, Unit* target)
	{
		// Record skill cast event
		if (mRecordingEvents)
		{
			CombatEvent event;
			event.type = CombatEventType::UnitCastSkill;
			event.timestamp = mSimulationTime;
			event.sourceUnitID = caster.getUnitId();
			event.targetUnitID = target ? target->getUnitId() : -1;
			event.data.skillID = skillID;
			recordEvent(event);
		}

		// Simple skill implementation
		if (target && !target->isDead())
		{
			// Always apply damage, recording is handled by applyUnitDamage
			applyUnitDamage(*target, 50.0f); // Big damage
		}

		// Reset mana
		UnitStats stats = caster.getStats();
		stats.mana = 0;
		caster.setStats(stats);
	}

	void World::applyUnitManaGain(Unit& unit, float manaAmount)
	{
		// Record mana gain event
		if (mRecordingEvents)
		{
			CombatEvent event;
			event.type = CombatEventType::UnitManaGain;
			event.timestamp = mSimulationTime;
			event.sourceUnitID = unit.getUnitId();
			event.targetUnitID = -1;
			event.data.manaGained = manaAmount;
			recordEvent(event);
		}

		UnitStats stats = unit.getStats();
		stats.mana = Math::Min(stats.mana + manaAmount, stats.maxMana);
		unit.setStats(stats);
	}
} // namespace AutoBattler
