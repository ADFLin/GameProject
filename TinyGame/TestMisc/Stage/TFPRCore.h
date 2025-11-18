#include "DataStructure/Grid2D.h"
#include "Math/TVector2.h"
#include "Math/GeometryPrimitive.h"

struct lua_State;

namespace TFWR
{
	typedef TVector2<int> Vec2i;

	class CodeFile;

	struct ExecutableObject
	{
		enum EState
		{
			ePause = BIT(0),
			eExecuting = BIT(2),
			eStep = BIT(3),
		};

		void reset()
		{
			fTicksAcc = 0;
			line = 0;
			executeL = nullptr;
			stateFlags = 0;
			executionCode = nullptr;
		}

		CodeFile* executionCode;
		lua_State* executeL;
		float fTicksAcc;
		int line;
		uint32 stateFlags;
	};

	namespace EUnlock
	{
		enum Type
		{
			Auto_Unlock = -1,
			Cactus = 0,
			Carrots,
			Costs,
			Debug,
			Debug_2,
			Dictionaries,
			Dinosaurs,
			Expand,
			Fertilizer,
			Functions,
			Grass,
			Hats,
			Import,
			Leaderboard,
			Lists,
			Loops,
			Mazes,
			Megafarm,
			Operators,
			Plant,
			Polyculture,
			Pumpkins,
			Senses,
			Simulation,
			Speed,
			Sunflowers,
			The_Farmers_Remains,
			Timing,
			Top_Hat,
			Trees,
			Utilities,
			Variables,
			Watering,

			COUNT,
		};
	}

	enum EDirection
	{
		East,
		West,
		North,
		South,
	};

	class GameState;
	class Drone : public ExecutableObject
	{
	public:

		Drone()
		{
			reset();
		}

		void reset()
		{
			ExecutableObject::reset();
			pos = Vec2i(0, 0);
		}
		
		Vec2i pos;
	};

	struct MapTile;

	struct UpdateArgs
	{
		float deltaTime;
		float fTicks;
		float speed;
	};

	namespace EPlant
	{
		enum Type
		{
			Grass,
			Bush,
			Carrots,
			Tree,
			Pumpkin,
			Cactus,
			Sunflower,
			Dionsaur,

			COUNT,
		};
	}

	class Entity
	{
	public:
		virtual ~Entity() = default;
		virtual EPlant::Type getPlantType() = 0;
		virtual void plant(MapTile& tile, GameState& game){}
		virtual void grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs) {}
		virtual void harvest(MapTile& tile, GameState& game) {}
		virtual std::string getDebugInfo(MapTile const& tile) = 0;
	};


	enum EItem
	{
		Hay,
		Wood,
		Pumpkin,
		COUNT,
	};

	enum EGround
	{
		Grassland,
		Soil,
	};


	struct MapTile
	{
		EGround ground;
		Entity* plant;
		float  growValue;
		float  growTime;
		int    meta;

		void reset()
		{
			ground = EGround::Grassland;
			plant = nullptr;
			growValue = 0;
		}
	};

	class BasePlantEntity : public Entity
	{
	public:
		BasePlantEntity(EPlant::Type inPlant, EItem production);

		EPlant::Type getPlantType() { return mPlant; }

		void grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs);
		void harvest(MapTile& tile, GameState& game);

		std::string getDebugInfo(MapTile const& tile);

		void updateGrowValue(MapTile& tile, UpdateArgs const& updateArgs);

		EPlant::Type mPlant;
		EItem production;
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
	public:
		using BasePlantEntity::BasePlantEntity;
		void plant(MapTile& tile, GameState& game) 
		{
			tile.meta = INDEX_NONE;
		}
		void grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs);

		int mMaxSize = 5;
	};

	class GameState
	{
	public:

		GameState();

		void init();

		void runExecution(Drone& drone, CodeFile& codeFile);
		void stopExecution(Drone& drone);
		bool isExecutingCode();

		void release();

		Vec2i getPos(MapTile const& tile) const;

		void reset();

		void update(float deltaTime);
		void update(Drone& drone, UpdateArgs const& updateArgs);
		void update(MapTile& tile, UpdateArgs const& updateArgs);

		Drone* createDrone(CodeFile& file);

		bool isUnlocked(EUnlock::Type unlock);
		void till(Drone& drone);
		bool harvest(Drone& drone);
		bool canHarvest(Drone& drone);
		bool plant(Drone& drone, Entity& entity);
		bool move(Drone& drone, EDirection direction);

		Entity* getPlantEntity(Drone& drone)
		{
			auto& tile = getTile(drone.pos);
			return tile.plant;
		}

		MapTile& getTile(Vec2i const& pos)
		{
			return mTiles(pos);
		}

		using Area = Math::TAABBox<Vec2i>;

		void addItem(EItem item, int num)
		{
			mItems[item] += num;
		}

		int tryMergeArea(Vec2i const& pos, int maxSize, Entity* entity);

		int tryMergeArea(Area const& areaToMerge, int size, int dir, Entity* entity);

		int addArea(Area const& area)
		{
			int result;
			if (mFreeAreaIndex != INDEX_NONE)
			{
				result = mFreeAreaIndex;
				mFreeAreaIndex = mAreas[mFreeAreaIndex].link;
			}
			else
			{
				result = mAreas.size();
				mAreas.push_back(area);
			}
			return result;
		}

		void removeArea(int id)
		{
			if (mAreas[id].link != INDEX_NONE)
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
		int mFreeAreaIndex;

		float mMaxSpeed = 1.0f;
		lua_State* mMainL = nullptr;

		TArray<Drone*> mDrones;
		TGrid2D<MapTile> mTiles;

		TArray<int> mUnlockLevels;
		TArray<int> mItems;
	};


}//namespace TFWR