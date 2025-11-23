#include "DataStructure/Grid2D.h"
#include "Math/TVector2.h"
#include "Math/GeometryPrimitive.h"
#include "ReflectionCollect.h"

struct lua_State;

namespace TFWR
{
	typedef TVector2<int> Vec2i;
	Vec2i constexpr GDirectionOffset[] =
	{
		Vec2i(1,0),
		Vec2i(-1,0),
		Vec2i(0,1),
		Vec2i(0,-1),
	};


	using Area = Math::TAABBox<Vec2i>;


	class CodeFile;

	struct ExecutableObject
	{
		enum EState
		{
			ePause = BIT(0),
			eExecuting = BIT(2),
			eStep = BIT(3),
		};

		void clearExecution()
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

	REF_ENUM_BEGIN(EDirection)
		REF_ENUM(East)
		REF_ENUM(West)
		REF_ENUM(North)
		REF_ENUM(South)
	REF_ENUM_END()

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

	struct MapTile;

	struct UpdateArgs
	{
		float deltaTime;
		float fTicks;
		float speed;
	};

	class Entity;

	enum class EItem
	{
		Hay,
		Wood,
		Pumpkin,
		COUNT,
	};
	

	enum class EGround
	{
		Grassland,
		Soil,

		Any = -1,
	};

	REF_ENUM_BEGIN(EGround)
		REF_ENUM(Grassland)
		REF_ENUM(Soil)
	REF_ENUM_END()
	
	struct ProductionInfo
	{
		EItem item;
		int   num;
	};

	struct PlantInfo
	{
		EGround ground;
		ProductionInfo production;
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


			DeadPumpkin,

			COUNT,

			None = -1,
		};

		bool IsSwappable(EPlant::Type plant);
		Entity* GetEntity(EPlant::Type plant);
		PlantInfo& GetInfo(EPlant::Type plant);
	}

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
			,mMaxSize(maxSize)
			,mDiePercent(diePercent)
			,mDieType(inDieType)
		{

		}

		bool plant(MapTile& tile, GameState& game) 
		{
			if (!BaseClass::plant(tile, game))
				return false;
			
			tile.meta = INDEX_NONE;
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
		EmptyEntity(EPlant::Type inPlant): mPlant(inPlant){}

		EPlant::Type getPlantType() { return mPlant; }
		bool plant(MapTile& tile, GameState& game) { return false; }
		void grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs){}
		bool canHarvest(MapTile& tile, GameState& game) { return false; }
		void harvest(MapTile& tile, GameState& game, bool bCompanionPreference){}
		void till(MapTile& tile, GameState& game) {}
		std::string getDebugInfo(MapTile const& tile);

		EPlant::Type mPlant;
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
		void clear();

		Drone* mExecutingDrone = nullptr;

		void update(float deltaTime);
		void update(Drone& drone, UpdateArgs const& updateArgs);

		void update(MapTile& tile, UpdateArgs const& updateArgs);

		Drone* createDrone(CodeFile& file);

		bool isUnlocked(EUnlock::Type unlock);
		void till(Drone& drone);
		bool harvest(Drone& drone);
		bool canHarvest(Drone& drone);
		bool swap(Drone& drone, EDirection direction)
		{
			Vec2i targetPos = drone.pos + GDirectionOffset[direction];
			if (!mTiles.checkRange(targetPos))
				return false;

			MapTile& tileTarget = getTile(targetPos);
			MapTile& tile = getTile(drone.pos);
			if (tileTarget.plant)
			{
				auto plantType = tileTarget.plant->getPlantType();
				if (!EPlant::IsSwappable(plantType))
					return false;

				auto ground = EPlant::GetInfo(plantType).ground;
				if (ground != EGround::Any && ground != tile.ground)
					return false;
			}

			if (tile.plant)
			{
				auto plantType = tile.plant->getPlantType();
				if (!EPlant::IsSwappable(plantType))
					return false;

				auto ground = EPlant::GetInfo(plantType).ground;
				if (ground != EGround::Any && ground != tileTarget.ground)
					return false;
			}


			tile.swap(tileTarget);
			return true;
		}

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

		Entity* getCompanion(Drone& drone, Vec2i& outPos)
		{
			auto& tile = getTile(drone.pos);

			if (tile.plant == nullptr)
				return nullptr;
			if (!(PolycultureMask & BIT(tile.plant->getPlantType())))
				return nullptr;

			CHECK(tile.meta >= 0);
			auto const& companion = mCompanions[tile.meta];
			outPos = companion.pos;
			return EPlant::GetEntity(companion.planet);
		}

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