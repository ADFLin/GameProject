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

		void init(class World* world);
		void resetGame();
		void render(IGraphics2D& g);

		class World* getWorld() const { return mWorld; }

		int getGold() const { return mGold; }
		void addGold(int val) { mGold += val; }
		bool spendGold(int val);

		int getHp() const { return mHp; }
		void takeDamage(int val);

		int getLevel() const { return mLevel; }
		int getXp() const { return mXp; }

	public:
		TArray<Unit*> mUnits;
		TArray<Unit*> mEnemyUnits;
		TArray<Unit*> mBench;
		TArray<int>   mShopList;

		PlayerBoard mBoard;
		PlayerBoard& getBoard() { return mBoard; }

		void refreshShop();
		bool buyUnit(int index);

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
	};

}//namespace AutoBattler

#endif // ABPlayer_h__
