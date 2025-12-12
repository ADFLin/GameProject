#include "ABPCH.h"
#include "ABPlayer.h"
#include "ABWorld.h"
#include "RenderUtility.h"
#include "GameGraphics2D.h"
#include "ABDefine.h"

namespace AutoBattler
{

	Player::Player()
	{
		mGold = 0;
		mHp = AB_INITIAL_HP;
		mLevel = 1;
		mXp = 0;
	}

	void Player::init(World* world)
	{
		mWorld = world;
		mGold = AB_INITIAL_GOLD;
		mHp = AB_INITIAL_HP;

		mBench.resize(AB_BENCH_SIZE);
		for (auto& slot : mBench) slot = nullptr;
		
		mBoard.setup();
		mBoard.setOwner(this);
	}

	void Player::resetGame()
	{
		mGold = AB_INITIAL_GOLD;
		mHp = AB_INITIAL_HP;
		mLevel = 1;
		mXp = 0;

		mUnits.clear();
		mEnemyUnits.clear();

		mBench.resize(AB_BENCH_SIZE);
		for (auto& slot : mBench) slot = nullptr;

		mBoard.setup();
	}



	// ... 

	void Player::refreshShop()
	{
		if (!mWorld) return;

		mShopList.resize(5);
		for (int& id : mShopList)
		{
			UnitDefinition const* def = mWorld->getUnitDataManager().getRandomUnit(mLevel);
			if (def)
				id = def->id;
			else
				id = 0;
		}
	}

	bool Player::buyUnit(int index)
	{
		if (!mShopList.isValidIndex(index))
			return false;

		int unitId = mShopList[index];
		if (unitId == 0)
			return false;

		// Find empty bench slot
		int slotIndex = -1;
		for (int i = 0; i < mBench.size(); ++i)
		{
			if (mBench[i] == nullptr)
			{
				slotIndex = i;
				break;
			}
		}

		if (slotIndex == -1) // Bench Full
			return false;

		int cost = 1;
		UnitDefinition const* def = nullptr;
		if (mWorld)
		{
			def = mWorld->getUnitDataManager().getUnit(unitId);
			if (def) cost = def->cost;
		}

		if (!spendGold(cost))
			return false;

		Unit* unit = new Unit();
		if (def)
		{
			unit->setStats(def->baseStats);
			unit->setUnitId(def->id);
		}
		
		mBench[slotIndex] = unit;
		
		mShopList[index] = 0; // Empty slot
		return true;
	}
	void Player::render(IGraphics2D& g)
	{
		// Render Board
		mBoard.render(g);

		// Render Bench
		int benchSlotSize = 40; // World space size

		for (int i = 0; i < mBench.size(); ++i)
		{
			Vector2 centerPos = getBenchSlotPos(i);
			Vector2 slotPos = centerPos - Vector2(benchSlotSize / 2, benchSlotSize / 2);

			RenderUtility::SetBrush(g, EColor::Gray);
			RenderUtility::SetPen(g, EColor::Black);
			g.drawRect(slotPos, Vector2(benchSlotSize, benchSlotSize));

			if (mBench[i])
			{
				Unit* unit = mBench[i];
				if (!unit) continue;
				
				// Unit position is now authoritative
				unit->render(g, false); // Don't show state bars in storage
			}
		}
	}

	Vector2 Player::getBenchSlotPos(int index) const
	{
		Vector2 boardOffset = mBoard.getOffset();
		Vector2 benchStartPos = boardOffset + Vector2(0, PlayerBoard::MapSize.y * PlayerBoard::CellSize.y * 0.75f + 20); // Below Board

		int benchSlotSize = 40; // World space size
		int benchGap = 5;

		Vector2 slotPos = benchStartPos + Vector2(index * (benchSlotSize + benchGap), 0);
		Vector2 centerPos = slotPos + Vector2(benchSlotSize / 2, benchSlotSize / 2);
		return centerPos;
	}

	bool Player::spendGold(int val)
	{
		if (mGold < val)
			return false;
		mGold -= val;
		return true;
	}

	void Player::takeDamage(int val)
	{
		mHp -= val;
		if (mHp < 0)
			mHp = 0;
	}



	bool Player::rerollShop()
	{
		if (!spendGold(AB_SHOP_REFRESH_COST))
			return false;

		refreshShop();
		return true;
	}

	bool Player::deployUnit(Unit* unit, Vec2i destCoord, Vec2i sourceCoord)
	{
		if (!unit) return false;
		if (!mBoard.isValid(destCoord)) return false;

		// Check if occupied by Enemy or Invalid
		Unit* targetOccupier = mBoard.getUnit(destCoord.x, destCoord.y);
		if (targetOccupier && targetOccupier->getTeam() != UnitTeam::Player)
			return false;

		// Find current location
		int currentBenchIndex = -1;
		bool bOnBoard = false;
		Vec2i currentCoord(-1, -1);

		// Check Bench
		for (int i = 0; i < mBench.size(); ++i)
		{
			if (mBench[i] == unit)
			{
				currentBenchIndex = i;
				break;
			}
		}

		// Check Board
		if (currentBenchIndex == -1)
		{
			// If sourceCoord was provided (valid), use it directly
			if (sourceCoord.x >= 0 && sourceCoord.y >= 0 && mBoard.isValid(sourceCoord))
			{
				if (mBoard.getUnit(sourceCoord.x, sourceCoord.y) == unit)
				{
					currentCoord = sourceCoord;
					bOnBoard = true;
				}
			}
			else if (unit->getInternalBoard() == &mBoard)
			{
				// Fallback: calculate from unit's current position
				currentCoord = mBoard.getCoord(unit->getPos());
				// Double check consistency
				if (mBoard.getUnit(currentCoord.x, currentCoord.y) == unit)
					bOnBoard = true;
			}
		}

		if (currentBenchIndex == -1 && !bOnBoard)
			return false; // Unit not owned or lost

		// Update Source
		if (bOnBoard)
		{
			mBoard.setUnit(currentCoord.x, currentCoord.y, nullptr);
		}
		else
		{
			mBench[currentBenchIndex] = nullptr;
		}

		// Handle Target Occupation (Swap)
		if (targetOccupier)
		{
			// Move targetOccupier to unit's OLD location
			if (bOnBoard)
			{
				// Board -> Board Swap
				targetOccupier->setPos(mBoard.getPos(currentCoord.x, currentCoord.y));
				mBoard.setUnit(currentCoord.x, currentCoord.y, targetOccupier);
			}
			else
			{
				// Bench -> Board Swap
				// targetOccupier moves to Bench[currentBenchIndex]
				// targetOccupier WAS on mUnits (Board), so remove from mUnits
				mUnits.remove(targetOccupier);
				targetOccupier->setInternalBoard(nullptr);

				mBench[currentBenchIndex] = targetOccupier;
			}
		}

		// Move Unit to Target
		if (!bOnBoard) // Came from Bench
		{
			mUnits.push_back(unit);
			unit->setInternalBoard(&mBoard);
		}

		unit->setPos(mBoard.getPos(destCoord.x, destCoord.y));
		unit->stopMove();
		mBoard.setUnit(destCoord.x, destCoord.y, unit);
		
		return true;
	}

	bool Player::retractUnit(Unit* unit, int slotIndex, Vec2i sourceCoord)
	{
		if (!unit) return false;
		if (!mBench.isValidIndex(slotIndex)) return false;

		Unit* targetOccupier = mBench[slotIndex];

		// Find current location
		int currentBenchIndex = -1;
		bool bOnBoard = false;
		Vec2i currentCoord(-1, -1);

		for (int i = 0; i < mBench.size(); ++i)
		{
			if (mBench[i] == unit)
			{
				currentBenchIndex = i;
				break;
			}
		}

		if (currentBenchIndex == -1)
		{
			// If sourceCoord provided
			if (sourceCoord.x >= 0 && sourceCoord.y >= 0 && mBoard.isValid(sourceCoord))
			{
				if (mBoard.getUnit(sourceCoord.x, sourceCoord.y) == unit)
				{
					currentCoord = sourceCoord;
					bOnBoard = true;
				}
			}
			else if (unit->getInternalBoard() == &mBoard)
			{
				currentCoord = mBoard.getCoord(unit->getPos());
				if (mBoard.getUnit(currentCoord.x, currentCoord.y) == unit)
					bOnBoard = true;
			}
		}

		if (currentBenchIndex == -1 && !bOnBoard) return false;

		// Clear Source
		if (bOnBoard)
		{
			mBoard.setUnit(currentCoord.x, currentCoord.y, nullptr);
			mUnits.remove(unit); // Will be on bench
			unit->setInternalBoard(nullptr);
		}
		else
		{
			mBench[currentBenchIndex] = nullptr;
		}

		// Handle Swap
		if (targetOccupier)
		{
			// Move targetOccupier to OLD location
			if (bOnBoard)
			{
				// Board -> Bench Swap (targetOccupier goes to Board)
				mBench[slotIndex] = nullptr; // Temporarily clear to avoid duplicate logic?
				
				mUnits.push_back(targetOccupier);
				targetOccupier->setInternalBoard(&mBoard);
				targetOccupier->setPos(mBoard.getPos(currentCoord.x, currentCoord.y));
				mBoard.setUnit(currentCoord.x, currentCoord.y, targetOccupier);
			}
			else
			{
				// Bench -> Bench Swap
				mBench[currentBenchIndex] = targetOccupier;
				targetOccupier->setPos(getBenchSlotPos(currentBenchIndex));
				targetOccupier->stopMove();
			}
		}
		
		// Move Unit to Bench
		mBench[slotIndex] = unit;
		unit->setPos(getBenchSlotPos(slotIndex));
		unit->stopMove();
		
		return true;
	}

	void Player::sellUnit(int slotIndex)
	{
		if (mBench.isValidIndex(slotIndex))
		{
			Unit* unit = mBench[slotIndex];
			if (unit)
			{
				int cost = 1;
				if (mWorld)
				{
					UnitDefinition const* def = mWorld->getUnitDataManager().getUnit(unit->getUnitId());
					if (def) cost = def->cost;
				}
				addGold(cost);
				delete unit;
				mBench[slotIndex] = nullptr;
			}
		}
	}

	void Player::buyExperience()
	{
		if (spendGold(4))
		{
			mXp += 4;
		}
	}

}
