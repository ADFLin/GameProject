#pragma once
#ifndef TFWRGameLogic_H_8A3F2B1C_4D5E_4F7A_9B8C_1D2E3F4A5B6C
#define TFWRGameLogic_H_8A3F2B1C_4D5E_4F7A_9B8C_1D2E3F4A5B6C

#include "TFWRCore.h"
#include "DataStructure/Grid2D.h"

namespace TFWR
{
	class Entity;
	class BoxingValue;

	// Result of a game operation
	struct GameOperationResult
	{
		bool success = false;
		int tickCost = 0;
		
		GameOperationResult() = default;
		GameOperationResult(bool inSuccess, int inTickCost = 0)
			: success(inSuccess), tickCost(inTickCost)
		{
		}

		static GameOperationResult Success(int tickCost = 0)
		{
			return GameOperationResult(true, tickCost);
		}

		static GameOperationResult Failure(int tickCost = 0)
		{
			return GameOperationResult(false, tickCost);
		}
	};

	// Pure drone state without script execution
	struct DroneState
	{
		Vec2i pos;

		DroneState()
		{
			reset();
		}

		void reset()
		{
			pos = Vec2i(0, 0);
		}
	};

	// Core game logic without Lua dependencies
	class GameLogic
	{
	public:
		GameLogic();

		void init(int size);
		void resizeMap(int size);
		void reset();
		void clear();

		Vec2i getPos(MapTile const& tile) const;

		void update(float deltaTime);
		void update(MapTile& tile, UpdateArgs const& updateArgs);



		// Core game operations - return results instead of calling Wait()
		GameOperationResult doMove(DroneState& drone, EDirection direction);
		GameOperationResult doTill(DroneState& drone);
		GameOperationResult doHarvest(DroneState& drone);
		GameOperationResult doPlant(DroneState& drone, Entity& entity);
		GameOperationResult doUseItem(Vec2i const& pos, EItem item, int nTimes);
		GameOperationResult doSwapPlant(Vec2i const& pos, EDirection direction);

		// Query operations
		bool canMove(Vec2i const& pos, EDirection direction);
		bool canHarvest(DroneState& drone);
		Entity* getPlantEntity(Vec2i const& pos);
		EGround getGroundType(Vec2i const& pos);
		Entity* getCompanion(Vec2i const& pos, Vec2i& outPos);
		int measure(Vec2i const& pos, BoxingValue* outputs[]);

		int getUnlockLevel(EUnlock::Type unlock);
		int getUnlockLevel(EItem item);
		int getUnlockLevel(EGround ground);
		int getUnlockLevel(Entity& entity);

		// Map and tile access
		MapTile& getTile(Vec2i const& pos)
		{
			return mTiles(pos);
		}

		TGrid2D<MapTile> const& getTiles() const { return mTiles; }
		TGrid2D<MapTile>& getTiles() { return mTiles; }

		// Item management
		void addItem(EItem item, int num)
		{
			mItems[(int)item] += num;
		}

		int getItemNum(EItem item) const
		{
			return mItems[(int)item];
		}

		// Unlock system
		bool isUnlocked(EUnlock::Type unlock) const;
		TArray<int> const& getUnlockLevels() const { return mUnlockLevels; }
		TArray<int>& getUnlockLevels() { return mUnlockLevels; }

		// Plant operations
		void removePlant(MapTile& tile);
		void plantAndRipen(Vec2i const& pos, Entity& entity);

		// Maze operations
		void generateMaze(Vec2i const& pos, Vec2i const& size);
		int createMaze(Area const& area);
		void removeMaze(int id);

		// Area management
		int tryMergeArea(Vec2i const& pos, Entity* entity);
		int tryMergeArea(Area const& testArea, int size, Entity* entity, Vec2i& outBlockPos);
		int addArea(Area const& area);
		void removeArea(int id);

		// Power consumption
		void consumePowerForAction()
		{
			mActionCounter++;
			if (mActionCounter >= 30)
			{
				mActionCounter = 0;
				if (mItems[(int)EItem::Power] > 0)
				{
					mItems[(int)EItem::Power]--;
				}
			}
		}

		// Speed control
		float getMaxSpeed() const { return mMaxSpeed; }
		void setMaxSpeed(float speed) { mMaxSpeed = speed; }

		bool bForceRipen = false;

		struct LinkedArea : Area
		{
			LinkedArea(Area const& area)
				:Area(area)
			{
				link = INDEX_NONE;
			}

			int  link;
		};

		struct MazeInfo
		{
			int   areaID;
			Vec2i treasurePos;
			int   treasureValue;
			int   link;
		};

		struct Companion
		{
			Vec2i pos;
			EPlant::Type planet;
			int linkIndex;
		};

		// Public for backward compatibility with Entity interface
		TArray< LinkedArea > mAreas;
		int mFreeAreaIndex = INDEX_NONE;

		TArray< MazeInfo > mMazes;
		int mFreeMazeIndex = INDEX_NONE;

		TArray< Companion > mCompanions;
		int mFreeCompanionIndex = INDEX_NONE;

		TGrid2D<MapTile> mTiles;

	private:
		bool plantInternal(Vec2i const& pos, Entity& entity);
		void postPlant(Vec2i const& pos, MapTile& tile, Entity* oldPlant, int oldMeta);



		float mMaxSpeed = 1.0f;
		float mWaterTankFillProgress = 0.0f;
		float mFertilizerTankFillProgress = 0.0f;
		int mActionCounter = 0;


		TArray<int> mUnlockLevels;
		TArray<int> mItems;
	};

} // namespace TFWR

#endif // TFWRGameLogic_H_8A3F2B1C_4D5E_4F7A_9B8C_1D2E3F4A5B6C
