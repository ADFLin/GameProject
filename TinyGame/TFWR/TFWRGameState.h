#pragma once
#ifndef TFWRGameState_H_0EE33DEF_183A_43E1_9642_178C1B29F742
#define TFWRGameState_H_0EE33DEF_183A_43E1_9642_178C1B29F742

#include "TFWRCore.h"
#include "TFWRScript.h"

#include "DataStructure/Grid2D.h"

namespace TFWR
{
	class GameState;
	class Drone : public ExecutableObject
	{
	public:

		Drone()
		{
			reset();
			clearExecution();
		}

		void reset()
		{
			pos = Vec2i(0, 0);
		}

		Vec2i pos;
	};

	struct UpdateArgs
	{
		float deltaTime;
		float fTicks;
		float speed;
	};

	struct MapTile
	{
		EGround ground;
		Entity* plant;

		float  growValue;
		float  growTime;
		float  waterValue;
		int    meta;

		void reset()
		{
			ground = EGround::Grassland;
			plant = nullptr;
			growValue = 0;
			waterValue = 0;
			meta = 0;
		}

		void swap(MapTile& target)
		{
			using std::swap;
			swap(plant, plant);
			swap(growValue, target.growValue);
			swap(growTime, target.growTime);
		}
	};

	class Entity
	{
	public:
		virtual ~Entity() = default;
		virtual EPlant::Type getPlantType() = 0;
		virtual bool plant(MapTile& tile, GameState& game) { return false; }
		virtual void grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs) {}
		virtual bool canHarvest(MapTile& tile, GameState& game) { return false; }
		virtual void harvest(MapTile& tile, GameState& game, bool bCompanionPreference) {}
		virtual void till(MapTile& tile, GameState& game) {}
		virtual std::string getDebugInfo(MapTile const& tile) = 0;
	};

	template< int N >
	constexpr uint32 MakeMask(EPlant::Type const (&PolyculturePlants)[N])
	{
		uint32 result = 0;
		for (int i = 0; i < N; ++i)
		{
			result |= BIT(PolyculturePlants[i]);
		}
		return result;
	}
	EPlant::Type constexpr PolyculturePlants[] = { EPlant::Grass, EPlant::Bush, EPlant::Tree, EPlant::Carrots };
	uint32 constexpr PolycultureMask = MakeMask(PolyculturePlants);



	class BasePlantEntity : public Entity
	{
	public:
		BasePlantEntity(EPlant::Type inPlant);

		EPlant::Type getPlantType() { return mPlant; }
		bool plant(MapTile& tile, GameState& game);
		void grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs);
		bool canHarvest(MapTile& tile, GameState& game)
		{
			return tile.growValue == 1.0;
		}
		void harvest(MapTile& tile, GameState& game, bool bCompanionPreference);

		std::string getDebugInfo(MapTile const& tile);

		void updateGrowValue(MapTile& tile, UpdateArgs const& updateArgs, float growTimeScale = 1.0f);

		EPlant::Type mPlant;
	};

	class SimplePlantEntity : public BasePlantEntity
	{
	public:
		using BasePlantEntity::BasePlantEntity;

	};

	class TreePlantEntity : public BasePlantEntity
	{
	public:
		using BasePlantEntity::BasePlantEntity;
		void grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs);
	};

	class AreaPlantEntity : public BasePlantEntity
	{
		using BaseClass = BasePlantEntity;
	public:
		AreaPlantEntity(EPlant::Type inPlant, int maxSize, float diePercent, EPlant::Type inDieType)
			:BaseClass(inPlant)
			, mMaxSize(maxSize)
			, mDiePercent(diePercent)
			, mDieType(inDieType)
		{

		}

		bool plant(MapTile& tile, GameState& game)
		{
			if (!BaseClass::plant(tile, game))
				return false;

			tile.meta = INDEX_NONE;
			return true;
		}

		void grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs);
		void harvest(MapTile& tile, GameState& game, bool bCompanionPreference);
		void till(MapTile& tile, GameState& game);

		int removeArea(MapTile& tile, GameState& game);

		int   mMaxSize;
		float mDiePercent;
		EPlant::Type mDieType;
	};

	class EmptyEntity : public Entity
	{
	public:
		EmptyEntity(EPlant::Type inPlant) : mPlant(inPlant) {}

		EPlant::Type getPlantType() { return mPlant; }
		bool plant(MapTile& tile, GameState& game) { return false; }
		void grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs) {}
		bool canHarvest(MapTile& tile, GameState& game) { return false; }
		void harvest(MapTile& tile, GameState& game, bool bCompanionPreference) {}
		void till(MapTile& tile, GameState& game) {}
		std::string getDebugInfo(MapTile const& tile);

		EPlant::Type mPlant;
	};

	class GameState
	{
	public:

		GameState();

		void init();

		void runExecution(Drone& drone, ScriptFile& codeFile);
		void stopExecution(Drone& drone);
		bool isExecutingCode();

		void release();

		Vec2i getPos(MapTile const& tile) const;

		void reset();
		void clear();

		Drone* mExecutingDrone = nullptr;

		void update(float deltaTime);
		void update(Drone& drone, UpdateArgs const& updateArgs);

		void update(MapTile& tile, UpdateArgs const& updateArgs);

		Drone* createDrone(ScriptFile& file);

		bool isUnlocked(EUnlock::Type unlock);
		void till(Drone& drone);
		bool harvest(Drone& drone);
		bool canHarvest(Drone& drone);
		bool swap(Drone& drone, EDirection direction);

		bool plant(Drone& drone, Entity& entity);
		bool move(Drone& drone, EDirection direction);

		Entity* getPlantEntity(Drone& drone)
		{
			auto& tile = getTile(drone.pos);
			return tile.plant;
		}

		EGround getGroundType(Drone& drone)
		{
			auto& tile = getTile(drone.pos);
			return tile.ground;
		}

		Entity* getCompanion(Drone& drone, Vec2i& outPos);

		MapTile& getTile(Vec2i const& pos)
		{
			return mTiles(pos);
		}

		void addItem(EItem item, int num)
		{
			mItems[(int)item] += num;
		}
		int getItemNum(EItem item)
		{
			return mItems[(int)item];
		}

		int tryMergeArea(Vec2i const& pos, Entity* entity);
		int tryMergeArea(Vec2i const& pos, int size, Entity* entity, Vec2i& outBlockPos);

		int addArea(Area const& area)
		{
			int result;
			if (mFreeAreaIndex != INDEX_NONE)
			{
				result = mFreeAreaIndex;
				mAreas[mFreeAreaIndex] = area;
				mFreeAreaIndex = mAreas[mFreeAreaIndex].link;
			}
			else
			{
				result = mAreas.size();
				mAreas.push_back(area);
			}
			return result;
		}

		bool plantInternal(Vec2i const& pos, Entity& entity);
		void removeArea(int id)
		{
			if (id == mFreeAreaIndex || mAreas[id].link != INDEX_NONE)
				return;

			mAreas[id].link = mFreeAreaIndex;
			mFreeAreaIndex = id;
		}


		struct LinkedArea : Area
		{
			LinkedArea(Area const& area)
				:Area(area)
			{
				link = INDEX_NONE;
			}

			int  link;
		};


		TArray< LinkedArea > mAreas;
		int mFreeAreaIndex = INDEX_NONE;
		struct Companion
		{
			Vec2i pos;
			EPlant::Type planet;
			int linkIndex;
		};
		TArray< Companion > mCompanions;
		int mFreeCompanionIndex = INDEX_NONE;

		float mMaxSpeed = 1.0f;
		lua_State* mMainL = nullptr;

		TArray<Drone*> mDrones;
		TGrid2D<MapTile> mTiles;

		TArray<int> mUnlockLevels;
		TArray<int> mItems;
	};

}//namespace TFWR

#endif // TFWRGameState_H_0EE33DEF_183A_43E1_9642_178C1B29F742