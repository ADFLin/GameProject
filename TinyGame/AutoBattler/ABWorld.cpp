#include "ABPCH.h"
#include "ABWorld.h"
#include "RenderUtility.h"
#include "GameGlobal.h"
#include "ABDefine.h"
#include "GameGraphics2D.h"



namespace AutoBattler
{
	World::World()
	{
		mPhase = BattlePhase::Preparation;
		mPhaseTimer = 0.0f;
		mLocalPlayerIndex = 0;
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
		int const GapX = 600;
		int const GapY = 500;

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

			if (i != mLocalPlayerIndex)
			{
			if (i != mLocalPlayerIndex)
			{
				// Bot logic moved to ABStage
			}
			}
		}
		
		// changePhase(BattlePhase::Preparation); // restart calls this
		restart(true);
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

	void World::restart(bool bInit)
	{
		cleanup(); // Clears units

		for (auto& player : mPlayers)
		{
			player.resetGame(); // Resets HP, Gold, Board
			player.refreshShop();

			// Give one starter unit on bench
			if (player.mBench.size() > 0)
			{
				Unit* unit = new Unit();
				player.mBench[0] = unit;
				unit->setPos(player.getBenchSlotPos(0));

				UnitLocation loc;
				loc.type = ECoordType::Bench;
				loc.index = 0;
				unit->setHoldLocation(loc);
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
		// Start mRound=0. Preparation -> mRound=1.
		// So we want config for mRound-1 if mRound is 1-based (UI).
		// Code logic: mRound initialized 0. In Prep: mRound++. Current Round 1.
		
		RoundConfig const& roundConfig = mRoundManager.getRoundConfig(mRound - 1);

		bool isPVE = (roundConfig.type == RoundType::PVE);

		// Spawn for ALL players
		for (int i = 0; i < mPlayers.size(); ++i)
		{
			if (isPVE)
			{
				spawnPVEModule(i, mRound);
			}
			else
			{
				// PVP Handled in spawnPVPModule called later
			}
		}
		
		if (!isPVE)
		{
			spawnPVPModule();
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

			Vector2 pos = board.getPos(enemyInfo.gridPos.x, enemyInfo.gridPos.y);
			enemy->setPos(pos);
			
			// Force set unit irrespective of occupation? PVE units shouldn't overlap if config is good.
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
				Vec2i gridPos = fromPlayer.getBoard().getCoord(unit->getPos());
				int targetX = PlayerBoard::MapSize.x - 1 - gridPos.x;
				int targetY = PlayerBoard::MapSize.y - 1 - gridPos.y;
				
				Vector2 worldPos = toBoard.getPos(targetX, targetY);
				unit->setPos(worldPos);
				unit->setInternalBoard(&toBoard);
				
				toBoard.setUnit(targetX, targetY, unit);
				unit->setTeam(UnitTeam::Enemy);

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
				ghost->setStats(unit->getStats());
				// ...
				
				Vec2i gridPos = fromPlayer.getBoard().getCoord(unit->getPos());
				int targetX = PlayerBoard::MapSize.x - 1 - gridPos.x;
				int targetY = PlayerBoard::MapSize.y - 1 - gridPos.y;

				Vector2 worldPos = toBoard.getPos(targetX, targetY);
				ghost->setPos(worldPos);
				ghost->setInternalBoard(&toBoard);
				ghost->setTeam(UnitTeam::Enemy);

				toBoard.setUnit(targetX, targetY, ghost);
				toPlayer.mEnemyUnits.push_back(ghost);
			}
		}
	}

	void World::spawnPVPModule()
	{
		// 1. Clear previous matches
		mMatches.clear();

		// 2. Identify eligible players
		TArray<int> activePlayers;
		for (int i = 0; i < mPlayers.size(); ++i)
		{
			if (mPlayers[i].getHp() > 0)
				activePlayers.push_back(i);
		}

		if (activePlayers.empty()) 
			return;

		// 3. Shuffle
		// Simple shuffle
		for (int i = 0; i < activePlayers.size(); ++i)
		{
			int swapIdx = ::Global::RandomNet() % activePlayers.size();
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

			// Teleport p2 units to p1 board
			teleportUnits(p2, p1, false);
		}

		// 5. Handle odd player (Ghost Match)
		if (!activePlayers.empty())
		{
			int oddPlayer = activePlayers.back(); activePlayers.pop_back();

			// Pick a random opponent (who is not self) from ALIVE players
			// If only 1 player alive, no PVP possible (handled in phase change maybe).
			// If we are here, there must be at least 1 player. 
			// But if total players = 1, we can't find opponent.
			
			int opponentIdx = -1;
			TArray<int> potentialOpponents;
			for (int i = 0; i < mPlayers.size(); ++i) 
			{
				if (i != oddPlayer && mPlayers[i].getHp() > 0) 
					potentialOpponents.push_back(i);
			}

			if (!potentialOpponents.empty())
			{
				opponentIdx = potentialOpponents[::Global::RandomNet() % potentialOpponents.size()];
				
				MatchPair match;
				match.home = oddPlayer;
				match.away = opponentIdx;
				match.isGhost = true;
				mMatches.push_back(match);

				teleportUnits(opponentIdx, oddPlayer, true);
			}
		}
	}


	void World::restoreUnits()
	{
		for (auto& player : mPlayers)
		{
			PlayerBoard& board = player.getBoard();
			for (auto unit : player.mUnits)
			{
				unit->restoreStartState();

				// Ensure unit is set to correct team
				unit->setTeam(UnitTeam::Player);

				// Re-register to grid on the OWNER's board (not current internal board)
				Vec2i coord = board.getCoord(unit->getPos());
				if (board.isValid(coord))
				{
					board.setUnit(coord.x, coord.y, unit);
				}
				// Set internal board to owner's board
				unit->setInternalBoard(&board);
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
				Player& awayP = mPlayers[match.away]; // If ghost, this is real player who was cloned, but irrelevant for outcome usually

				int homeAlive = 0;
				for (auto u : homeP.mUnits) if (!u->isDead()) homeAlive++;

				int awayAlive = 0;
				if (match.isGhost)
				{
					// For Ghost, enemies are in homeP.mEnemyUnits
					for (auto u : homeP.mEnemyUnits) if (!u->isDead()) awayAlive++;
				}
				else
				{
					// For normal, away units are awayP.mUnits (teleported)
					for (auto u : awayP.mUnits) if (!u->isDead()) awayAlive++;
				}

				// Resolution for HOME
				if (homeAlive > 0 && awayAlive == 0)
				{
					homeP.addGold(1); 
				}
				else if (awayAlive > 0 && homeAlive == 0)
				{
					homeP.takeDamage(2 + awayAlive);
				}

				// Resolution for AWAY (only if Not Ghost)
				if (!match.isGhost)
				{
					if (awayAlive > 0 && homeAlive == 0)
					{
						awayP.addGold(1);
					}
					else if (homeAlive > 0 && awayAlive == 0)
					{
						awayP.takeDamage(2 + homeAlive);
					}
				}
			}
		}
		// 2. PVE (No Matches)
		else
		{
			for (auto& player : mPlayers)
			{
				int playerAlive = 0;
				for (auto u : player.mUnits) if (!u->isDead()) playerAlive++;

				int enemyAlive = 0;
				for (auto u : player.mEnemyUnits) if (!u->isDead()) enemyAlive++;

				if (playerAlive > 0 && enemyAlive == 0)
				{
					player.addGold(1);
				}
				else if (enemyAlive > 0 && playerAlive == 0)
				{
					int damage = 2 + enemyAlive;
					player.takeDamage(damage);
				}
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
					mCombatEnded = true;
					resolveCombat();
					if (mPhaseTimer > 3.0f) mPhaseTimer = 3.0f;
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

		for (auto& player : mPlayers)
		{
			for (auto unit : player.mUnits)
				unit->update(dt, *this);
			for (auto unit : player.mEnemyUnits)
				unit->update(dt, *this);
		}
		
		updateProjectiles(dt);
	}

	void World::changePhase(BattlePhase phase)
	{
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
			mPhaseTimer = AB_PHASE_COMBAT_TIME;
			mCombatEnded = false;

			for (auto& player : mPlayers)
			{
				for (auto unit : player.mUnits)
					unit->saveStartState();
			}
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
					p.target->takeDamage(p.damage);
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

} // namespace AutoBattler
