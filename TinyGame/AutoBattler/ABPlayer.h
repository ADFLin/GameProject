#ifndef ABPlayer_h__
#define ABPlayer_h__

#include "ABUnit.h"
#include "ABPlayerBoard.h"
#include "DataStructure/Array.h"

namespace AutoBattler
{
	class Player
	{
	public:
		Player();

		int getIndex()
		{
			return mIndex;
		}

		void init(class World* world);
		void resetGame();
		void render(IGraphics2D& g);

		class World* getWorld() const { return mWorld; }

		int getGold() const { return mGold; }
		void addGold(int val) { mGold += val; }
		void setGold(int val) { mGold = val; } // Added setter
		bool spendGold(int val);

		int getHp() const { return mHp; }
		void setHp(int val) { mHp = val; } // Added setter
		void takeDamage(int val);

		void setLevel(int level)
		{
			mLevel = level;
			updateMaxPopulation();
		}

		int getLevel() const { return mLevel; }
		int getXp() const { return mXp; }

		int getMaxPopulation() const;

	public:
		void updateMaxPopulation();

		int mIndex;

		TArray<Unit*> mUnits;
		TArray<Unit*> mEnemyUnits;
		TArray<Unit*> mBench;
		TArray<int>   mShopList;

		PlayerBoard mBoard;
		PlayerBoard& getBoard() { return mBoard; }
		PlayerBoard const& getBoard() const { return mBoard; }

		void refreshShop();
		bool buyUnit(int index);

		// Encode PlayerIndex into UnitID (High 16 bits = PlayerIdx, Low 16 bits = Counter)
		int allocUnitID() 
		{ 
			int id = (mIndex << 16) | (mNextUnitID++ & 0xFFFF);
			LogMsg("Player %d allocUnitID: %d (raw=%d)", mIndex, id, mNextUnitID-1);
			return id;
		}
		int mNextUnitID = 1;

		void sellUnit(int slotIndex);
		bool rerollShop();
		void buyExperience();

		// Unit Manipulation (Phase 9 Refactoring)
		bool deployUnit(Unit* unit, Vec2i destCoord, Vec2i sourceCoord = Vec2i(-1, -1));

		bool retractUnit(Unit* unit, int slotIndex, Vec2i sourceCoord = Vec2i(-1, -1));
		
		Vector2 getBenchSlotPos(int index) const;

	private:
		class World* mWorld;

		int mGold;
		int mHp;
		int mLevel;
		int mXp;
		int mMaxPopulation;
	};

}//namespace AutoBattler

#endif // ABPlayer_h__
