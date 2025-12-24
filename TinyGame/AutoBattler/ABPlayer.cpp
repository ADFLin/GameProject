#include "ABPCH.h"
#include "ABPlayer.h"
#include "ABWorld.h"
#include "RenderUtility.h"
#include "GameGraphics2D.h"
#include "ABDefine.h"

#include "ABGameRenderer.h"

namespace AutoBattler
{

	Player::Player()
	{
		mGold = 0;
		mHp = AB_INITIAL_HP;
		setLevel(1);
		mXp = 0;
	}

	void Player::init(World* world)
	{
		mWorld = world;
		LogMsg("Player::init[%d] Addr=%p", mIndex, this);
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
		setLevel(1);
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
			UnitDefinition const* def = mWorld->getUnitDataManager().getRandomUnit(mLevel, mWorld->getRand());
			if (def)
				id = def->id;
			else
				id = 0;
		}

		LogMsg("ShopSync: P%d L%d [%d %d %d %d %d]", getIndex(), mLevel, mShopList[0], mShopList[1], mShopList[2], mShopList[3], mShopList[4]);
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
		// Use Player-scoped ID allocation
		unit->setUnitId(allocUnitID());

		if (def)
		{
			unit->setStats(def->baseStats);
			unit->setTypeId(def->id);
		}
		
		LogMsg("Player[%d] BuyUnit: Type=%d InstanceID=%d Cost=%d", mIndex, (def?def->id:-1), unit->getUnitId(), cost);

		mBench[slotIndex] = unit;
		unit->setPos(getBenchSlotPos(slotIndex));

		UnitLocation loc;
		loc.type = ECoordType::Bench;
		loc.index = slotIndex;
		unit->setDeployLocation(loc);
		
		mShopList[index] = 0; // Empty slot
		return true;
	}
	void Player::render(IGraphics2D& g)
	{
		// Render Board
		mBoard.render(g);

		// Render Bench
		for (int i = 0; i < mBench.size(); ++i)
		{
			Vector2 centerPos = getBenchSlotPos(i);
			Vector2 slotPos = centerPos - Vector2(AB_BENCH_SLOT_SIZE / 2.0f, AB_BENCH_SLOT_SIZE / 2.0f);

			RenderUtility::SetBrush(g, EColor::Gray);
			RenderUtility::SetPen(g, EColor::Black);
			g.drawRect(slotPos, Vector2(AB_BENCH_SLOT_SIZE, AB_BENCH_SLOT_SIZE));

			if (mBench[i])
			{
				Unit* unit = mBench[i];
				if (!unit) continue;
				
				// Unit position is now authoritative
				ABGameRenderer::Draw(g, *unit, false); // Don't show state bars in storage
			}
		}
	}

	Vector2 Player::getBenchSlotPos(int index) const
	{
		Vector2 boardOffset = mBoard.getOffset();
		Vector2 benchStartPos = boardOffset + Vector2(AB_BENCH_OFFSET_X, PlayerBoard::MapSize.y * PlayerBoard::CellSize.y * 0.75f + AB_BENCH_OFFSET_Y);

		Vector2 slotPos = benchStartPos + Vector2(index * (AB_BENCH_SLOT_SIZE + AB_BENCH_GAP), 0);
		Vector2 centerPos = slotPos + Vector2(AB_BENCH_SLOT_SIZE / 2.0f, AB_BENCH_SLOT_SIZE / 2.0f);
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

	int Player::getMaxPopulation() const
	{
		return mMaxPopulation;
	}

	void Player::updateMaxPopulation()
	{
		constexpr int DelataPopLevelMap[] = { 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
		mMaxPopulation = 0;
		for (int i = 0; i < mLevel; ++i)
		{
			mMaxPopulation += DelataPopLevelMap[i];
		}
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
		CHECK(unit);

		if (!mBoard.isValid(destCoord)) 
			return false;

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

		// Check Unit Cap
		if (!bOnBoard && targetOccupier == nullptr)
		{
			if (mUnits.size() >= getMaxPopulation())
				return false;
		}

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
				targetOccupier->setPos(mBoard.getWorldPos(currentCoord.x, currentCoord.y));
				mBoard.setUnit(currentCoord.x, currentCoord.y, targetOccupier);

				UnitLocation loc;
				loc.type = ECoordType::Board;
				loc.pos = currentCoord;
				targetOccupier->setDeployLocation(loc);
			}
			else
			{
				// Bench -> Board Swap
				// targetOccupier moves to Bench[currentBenchIndex]
				// targetOccupier WAS on mUnits (Board), so remove from mUnits
				mUnits.remove(targetOccupier);
				targetOccupier->setInternalBoard(nullptr);

				mBench[currentBenchIndex] = targetOccupier;

				UnitLocation loc;
				loc.type = ECoordType::Bench;
				loc.index = currentBenchIndex;
				targetOccupier->setDeployLocation(loc);
			}
		}

		// Move Unit to Target
		if (!bOnBoard) // Came from Bench
		{
			mUnits.push_back(unit);
			unit->setInternalBoard(&mBoard);
		}

		unit->setPos(mBoard.getWorldPos(destCoord.x, destCoord.y));
		unit->stopMove();
		mBoard.setUnit(destCoord.x, destCoord.y, unit);

		UnitLocation loc;
		loc.type = ECoordType::Board;
		loc.pos = destCoord;
		unit->setDeployLocation(loc);
		
		return true;
	}

	bool Player::retractUnit(Unit* unit, int slotIndex, Vec2i sourceCoord)
	{
		if (!unit) 
			return false;

		if (!mBench.isValidIndex(slotIndex)) 
			return false;

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
				targetOccupier->setPos(mBoard.getWorldPos(currentCoord.x, currentCoord.y));
				mBoard.setUnit(currentCoord.x, currentCoord.y, targetOccupier);

				UnitLocation loc;
				loc.type = ECoordType::Board;
				loc.pos = currentCoord;
				targetOccupier->setDeployLocation(loc);
			}
			else
			{
				// Bench -> Bench Swap
				mBench[currentBenchIndex] = targetOccupier;
				targetOccupier->setPos(getBenchSlotPos(currentBenchIndex));
				targetOccupier->stopMove();

				UnitLocation loc;
				loc.type = ECoordType::Bench;
				loc.index = currentBenchIndex;
				targetOccupier->setDeployLocation(loc);
			}
		}
		
		// Move Unit to Bench
		mBench[slotIndex] = unit;
		unit->setPos(getBenchSlotPos(slotIndex));
		unit->stopMove();

		UnitLocation loc;
		loc.type = ECoordType::Bench;
		loc.index = slotIndex;
		unit->setDeployLocation(loc);
		
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
		if (spendGold(AB_BUY_XP_COST))
		{
			mXp += AB_XP_PER_BUY;
			while (mXp >= 100 && mLevel < AB_MAX_LEVEL)
			{
				mXp -= 100;
				setLevel(mLevel + 1);
			}
		}
	}

}
