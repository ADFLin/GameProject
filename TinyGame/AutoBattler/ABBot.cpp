#include "ABPCH.h"
#include "ABBot.h"
#include "ABPlayer.h"
#include "ABWorld.h"
#include "ABGameActionControl.h"

namespace AutoBattler
{
	BotController::BotController(Player& player, IGameActionControl& control)
		: mPlayer(player), mActionControl(control)
	{
		mThinkTimer = 0.0f;
	}

	void BotController::update(float dt)
	{
		if (mPlayer.getWorld()->getPhase() != BattlePhase::Preparation)
			return;

		mThinkTimer -= dt;
		if (mThinkTimer > 0)
			return;

		// Random Think Time 0.3 ~ 0.8s
		mThinkTimer = 0.3f + (::Global::RandomNet() % 500) / 1000.0f;

		PlayerBoard& board = mPlayer.getBoard();
		auto& unitMgr = mPlayer.getWorld()->getUnitDataManager();

		// 1. Deploy Logic (Prioritize filling board)
		{
			int maxPop = mPlayer.getMaxPopulation();
			
			// Simple Strategy: Tank Front, DPS Back
			// Front: Y=3, then 2. Back: Y=0, then 1.
			
			for(int i=0; i < mPlayer.mBench.size() && mPlayer.mUnits.size() < maxPop; ++i)
			{
				Unit* unit = mPlayer.mBench[i];
				if (unit)
				{
					// Classify Role
					bool isRanged = unit->getStats().range >= 200;
					bool isTank = !isRanged && unit->getStats().maxHp >= 750; // Simple threshold
					
					struct Pos { int x, y; };
					Pos const* targetList = nullptr;
					int listSize = 0;

					// Priority Lists (Hardcoded for now in local static)
					// Front Center (Relative Row 3)
					static const Pos TankPos[] = { 
						{3,3}, {4,3}, {2,3}, {5,3}, {1,3}, {6,3}, {0,3}, {7,3},
						{3,2}, {4,2}, {2,2}, {5,2} 
					};
					// Front/Mid Flank (Relative Row 3 and 2)
					static const Pos MeleePos[] = {
						{0,3}, {7,3}, {1,3}, {6,3}, 
						{0,2}, {7,2}, {1,2}, {6,2}, 
						{2,2}, {5,2}, {3,2}, {4,2}
					};
					// Back Corner (Left biased, Relative Row 0 and 1)
					static const Pos RangedPos[] = {
						{0,0}, {1,0}, {2,0}, {3,0},
						{0,1}, {1,1}, {2,1}, {3,1},
						{7,0}, {6,0}, {5,0}, {4,0}
					};

					if (isTank)
					{
						targetList = TankPos;
						listSize = sizeof(TankPos)/sizeof(Pos);
					}
					else if (!isRanged)
					{
						targetList = MeleePos;
						listSize = sizeof(MeleePos)/sizeof(Pos);
					}
					else
					{
						targetList = RangedPos;
						listSize = sizeof(RangedPos)/sizeof(Pos);
					}

					bool found = false;
					int const HalfHeight = PlayerBoard::MapSize.y / 2;
					int const FullHeight = PlayerBoard::MapSize.y;

					for(int k=0; k<listSize; ++k)
					{
						int tx = targetList[k].x;
						// Map logic Y (0=Back, 3=Front) to Board Y (7=Back, 4=Front)
						int ty = (FullHeight - 1) - targetList[k].y;
						
						if (!board.isOccupied(tx, ty))
						{
							mActionControl.deployUnit(mPlayer, 1, i, 0, tx, ty);
							goto Finish;
						}
					}
					// Fallback to Scan if priority full
					if (!found)
					{
						for(int y=HalfHeight; y<FullHeight; ++y)
						{
							for(int x=0; x<PlayerBoard::MapSize.x; ++x)
							{
								if (!board.isOccupied(x,y))
								{
									mActionControl.deployUnit(mPlayer, 1, i, 0, x, y);
									goto Finish;
								}
							}
						}
					}
				}
			}
		}

		// 2. Buy / Level / Reroll Strategy
		{
			int gold = mPlayer.getGold();
			int hp = mPlayer.getHp();
			int level = mPlayer.getLevel();

			// Count Occurrences for Shop Units
			int bestBuyIdx = -1;
			int bestBuyScore = -1;

			bool bBenchFull = true;
			for (auto unit : mPlayer.mBench)
				if (unit == nullptr) { bBenchFull = false; break; }

			if (!bBenchFull)
			{
				for (int i = 0; i < mPlayer.mShopList.size(); ++i)
				{
					int unitId = mPlayer.mShopList[i];
					if (unitId != 0)
					{
						UnitDefinition const* def = unitMgr.getUnit(unitId);
						if (def && gold >= def->cost)
						{
							// Calculate Score
							int score = 0;
							
							// Have copies?
							int copies = 0;
							for(auto u : mPlayer.mUnits) if(u->getUnitId() == unitId) copies++;
							for(auto u : mPlayer.mBench) if(u && u->getUnitId() == unitId) copies++;
							
							if (copies >= 2) score += 50; // Triple!
							else if (copies == 1) score += 20; // Pair!
							
							// Cost/Tier
							score += def->cost;

							// Role need? (Simple: Randomize slightly)
							score += (::Global::RandomNet() % 5);

							if (score > bestBuyScore)
							{
								bestBuyScore = score;
								bestBuyIdx = i;
							}
						}
					}
				}
			}

			// Decide Action
			bool bPanic = (hp < 30);
			int keepGold = bPanic ? 0 : 30; // Economy Threshold

			// Buy if high priority (Pair/Triple) regardless of economy (mostly)
			// Or if we have excess gold
			
			if (bestBuyIdx != -1)
			{
				// Always buy Pair/Triple
				if (bestBuyScore >= 20)
				{
					mActionControl.buyUnit(mPlayer, bestBuyIdx);
					goto Finish;
				}
				// Else buy if over threshold
				if (gold > keepGold + 2) // +2 padding
				{
					mActionControl.buyUnit(mPlayer, bestBuyIdx);
					goto Finish;
				}
			}

			// Level Up Strategy
			// XP needed logic (Optional, currently blind)
			// If Gold > 50, Level Up
			if (gold > 50)
			{
				mActionControl.buyExperience(mPlayer);
				goto Finish;
			}
			// If panic and high gold?
			if (bPanic && gold > 20)
			{
				// Maybe Reroll? 
				if (bestBuyIdx == -1)
				{
					mActionControl.refreshShop(mPlayer);
					goto Finish;
				}
			}
			
			// Reroll if very rich
			if (gold > 60)
			{
				mActionControl.refreshShop(mPlayer);
				goto Finish;
			}
		}

	Finish:
		;
	}

}//namespace AutoBattler
