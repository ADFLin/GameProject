#include "TFWRGameState.h"

#include "InlineString.h"

#include "RandomUtility.h"

#include "Lua/lauxlib.h"
#include "Lua/lualib.h"

namespace TFWR
{
	constexpr int TicksPerSecond = 400;

	struct WorldExecuteContext;

	struct WorldExecuteContext : ExecutionContext
	{
		WorldExecuteContext()
		{
		}

		~WorldExecuteContext()
		{
		}

		GameState* game;
		Drone* drone;
	};

	struct EntityLibrary
	{
		EntityLibrary()
			:Grass(EPlant::Grass)
			, Bush(EPlant::Bush)
			, Tree(EPlant::Tree)
			, Pumpkin(EPlant::Pumpkin, 5, 20, EPlant::DeadPumpkin)
			, Dead_Pumpkin(EPlant::DeadPumpkin)
		{

		}

		void registerScript(ScriptHandle L)
		{
			lua_newtable(L);
#define REGISTER(NAME)\
	lua_pushlightuserdata(L, &NAME);\
	lua_setfield(L, -2, #NAME)

			REGISTER(Grass);
			REGISTER(Bush);
			REGISTER(Tree);
			REGISTER(Pumpkin);
			REGISTER(Dead_Pumpkin);
			lua_setglobal(L, "Entities");

#undef REGISTER
		}
		SimplePlantEntity Grass;
		SimplePlantEntity Bush;
		TreePlantEntity Tree;
		AreaPlantEntity Pumpkin;
		EmptyEntity Dead_Pumpkin;
	};

	EntityLibrary GEntities;

	namespace EPlant
	{
		bool IsSwappable(EPlant::Type plant)
		{
			return plant == EPlant::Cactus;

		}

		Entity* GetEntity(EPlant::Type plant)
		{
			switch (plant)
			{
			case EPlant::Grass: return &GEntities.Grass;
			case EPlant::Bush:  return &GEntities.Bush;
			case EPlant::Carrots:
				break;
			case EPlant::Tree:  return &GEntities.Tree;
			case EPlant::Pumpkin:  return &GEntities.Pumpkin;
			case EPlant::Cactus:
				break;
			case EPlant::Sunflower:
				break;
			case EPlant::Dionsaur:
				break;
			case EPlant::DeadPumpkin: return &GEntities.Dead_Pumpkin;
			case EPlant::COUNT:
				break;
			}
			return nullptr;
		}
	}

	struct FGameAPI
	{
		static WorldExecuteContext& GetContext()
		{
			return *static_cast<WorldExecuteContext*>(IExecuteManager::GetCurrentContext());
		}

		static GameState& GetGame()
		{
			return *GetContext().game;
		}
		static Drone& GetDrone()
		{
			return *GetContext().drone;
		}

		static void Wait(int waitTicks)
		{
			GetContext().object->fTicksAcc -= waitTicks;
		}

		static int SetTickEnabled(lua_State* L)
		{
			IExecuteManager::Get().bTickEnabled = lua_toboolean(L, 1);
			return 0;
		}

		static void Error()
		{
			luaL_error(GetContext().object->executeL, "Exec Error");
		}

		static void till()
		{
			GetGame().till(GetDrone());
			Wait(200);
		}
		static bool harvest()
		{
			bool bSuccess = GetGame().harvest(GetDrone());
			Wait(bSuccess ? 200 : 1);
			return bSuccess;
		}

		static bool can_harvest()
		{
			Wait(1);
			return GetGame().canHarvest(GetDrone());
		}

		static bool plant(Entity* entity)
		{
			bool bSuccess = false;
			if (entity)
			{
				bSuccess = GetGame().plant(GetDrone(), *entity);
			}

			Wait(bSuccess ? 200 : 1);
			return bSuccess;
		}

		static int get_pos_x()
		{
			Wait(1);
			return GetDrone().pos.x;
		}

		static int get_pos_y()
		{
			Wait(1);
			return GetDrone().pos.y;
		}

		static int  get_world_size()
		{
			Wait(1);
			return GetGame().mTiles.getSizeX();
		}

		static Entity* get_entity_type()
		{
			Wait(1);
			return GetGame().getPlantEntity(GetDrone());
		}

		static void clear()
		{
			GetGame().clear();
			Wait(200);
		}

		static void set_execution_speed(float speed)
		{
			GetGame().mMaxSpeed = speed;
			Wait(1);
		}

		static bool move(EDirection direction)
		{
			bool bSuccess = GetGame().move(GetDrone(), direction);
			Wait(bSuccess ? 200 : 1);
			return bSuccess;
		}

		static EGround get_ground_type()
		{
			Wait(1);
			return GetGame().getGroundType(GetDrone());
		}

		static float num_items(EItem item)
		{
			Wait(1);
			return GetGame().getItemNum(item);
		}

		static bool swap(EDirection direction)
		{
			bool bSuccess = GetGame().swap(GetDrone(), direction);
			Wait(bSuccess ? 200 : 1);
			return bSuccess;
		}

		static Entity* __Internal_get_companion(int& x, int& y)
		{
			Wait(1);
			Vec2i pos;
			auto entity = GetGame().getCompanion(GetDrone(), pos);
			if (entity)
			{
				x = pos.x;
				y = pos.y;
			}
			return entity;
		}

		template< typename TEnum >
		static void RegisterEnum(ScriptHandle L, char const* space)
		{
			lua_newtable(L);
			for (ReflectEnumValueInfo const& value : REF_GET_ENUM_VALUES(TEnum))
			{
				lua_pushinteger(L, value.value);
				lua_setfield(L, -2, value.text);
			}
			lua_setglobal(L, space);
		}

		template< typename TEnum >
		static void RegisterEnum(ScriptHandle L)
		{
			lua_getglobal(L, "_G");
			for (ReflectEnumValueInfo const& value : REF_GET_ENUM_VALUES(TEnum))
			{
				lua_pushinteger(L, value.value);
				lua_setfield(L, -2, value.text);
			}
			lua_pop(L, 1);
		}

		static void Register(lua_State* L)
		{
#define REGISTER(FUNC_NAME)\
	FLuaBinding::Register(L, #FUNC_NAME, FUNC_NAME)
			REGISTER(__Internal_get_companion);
			REGISTER(move);
			REGISTER(till);
			REGISTER(harvest);
			REGISTER(can_harvest);
			REGISTER(plant);
			REGISTER(get_pos_x);
			REGISTER(get_pos_y);
			REGISTER(get_world_size);
			REGISTER(get_entity_type);
			REGISTER(get_ground_type);
			REGISTER(clear);
#undef REGISTER
		}

		static void LoadLib(lua_State* L)
		{
			luaL_Reg CustomLib[] =
			{
				{"print", Print},
				{"_SetTickEnabled", SetTickEnabled},
				{NULL, NULL} /* end of array */
			};

			lua_getglobal(L, "_G");
			luaL_setfuncs(L, CustomLib, 0);
			lua_pop(L, 1);
		}

		static lua_State* CreateGameState()
		{
			lua_State* L = luaL_newstate();
			luaL_openlibs(L);
			RegisterEnum< EDirection >(L);
			RegisterEnum< EGround >(L, "Grounds");
			Register(L);
			LoadLib(L);
			GEntities.registerScript(L);
			luaL_dofile(L, TFWR_SCRIPT_DIR"/Main.lua");
			return L;
		}
	};

	struct TimeRange
	{
		float min;
		float max;
	};
	TimeRange PlantGrowthTimeMap[EPlant::COUNT] =
	{
		{ 0.5 , 0.5 },
		{ 3.2 , 4.8 },
		{ 4.8 , 7.2 },
		{ 5.6 , 8.4 },
		{ 0.2 , 3.8 },
		{ 0.9 , 1.1 },
		{ 4.0 , 6.0 },
		{ 0.18 , 0.22 },
	};

	BasePlantEntity::BasePlantEntity(EPlant::Type plant)
		:mPlant(plant)
	{

	}
	bool BasePlantEntity::plant(MapTile& tile, GameState& game)
	{
		tile.plant = this;
		tile.growValue = 0.0;
		tile.growTime = 0.0;
		return true;
	}

	void BasePlantEntity::grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs)
	{
		if (tile.growValue == 1.0)
			return;

		updateGrowValue(tile, updateArgs);
		if (tile.growValue >= 1.0)
		{
			tile.growValue = 1.0;
		}
	}

	void BasePlantEntity::updateGrowValue(MapTile& tile, UpdateArgs const& updateArgs, float growTimeScale)
	{
		auto const&  growTimeRange = PlantGrowthTimeMap[mPlant];
		float growTime = growTimeScale * RandFloat(growTimeRange.min, growTimeRange.max);
		tile.growValue += updateArgs.speed * updateArgs.deltaTime / growTime;
		tile.growTime += updateArgs.speed * updateArgs.deltaTime;
	}


	PlantInfo GPlantInfoMap[EPlant::COUNT];
	struct PlantInfoInit
	{
		PlantInfoInit(EPlant::Type plant, EGround ground, EItem item, int count)
		{
			GPlantInfoMap[plant] = { ground, { item , count } };
		}
	};

#define PLANT_INFO(PLANT, GROUND, ITEM, COUNT)\
	PlantInfoInit ANONYMOUS_VARIABLE(GProductionInfoInit)(PLANT, GROUND, ITEM, COUNT)

	PLANT_INFO(EPlant::Grass, EGround::Any, EItem::Hay, 4);
	PLANT_INFO(EPlant::Bush, EGround::Any, EItem::Wood, 1);
	PLANT_INFO(EPlant::Tree, EGround::Any, EItem::Wood, 5);
	PLANT_INFO(EPlant::Pumpkin, EGround::Soil, EItem::Pumpkin, 1);

	namespace EPlant
	{
		Entity* GetEntity(EPlant::Type plant);
		PlantInfo& GetInfo(EPlant::Type plant)
		{
			return GPlantInfoMap[plant];
		}
	}

	void BasePlantEntity::harvest(MapTile& tile, GameState& game, bool bCompanionPreference)
	{
		if (tile.growValue < 1.0)
			return;

		auto const& production = EPlant::GetInfo(mPlant).production;
		game.addItem(production.item, production.num);
	}

	std::string BasePlantEntity::getDebugInfo(MapTile const& tile)
	{
		return InlineString<>::Make("%s\n%f", toString(mPlant), tile.growValue);
	}

	std::string EmptyEntity::getDebugInfo(MapTile const& tile)
	{
		return InlineString<>::Make("%s", toString(mPlant));
	}

	void TreePlantEntity::grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs)
	{
		if (tile.growValue == 1.0)
			return;

		int numNeighborTree = 0;
		for (int dir = 0; dir < 4; ++dir)
		{
			Vec2i nPos = game.getPos(tile) + GDirectionOffset[dir];
			if (game.mTiles.checkRange(nPos))
			{
				auto const& nTile = game.getTile(nPos);
				if (nTile.plant == tile.plant)
				{
					++numNeighborTree;
				}
			}
		}

		updateGrowValue(tile, updateArgs, 1 << numNeighborTree);
		if (tile.growValue >= 1.0)
		{
			tile.growValue = 1.0;
		}
	}

	void AreaPlantEntity::grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs)
	{
		if (tile.growValue == 1.0)
			return;

		updateGrowValue(tile, updateArgs);
		if (tile.growValue >= 1.0)
		{
			tile.growValue = 1.0;
			if (mDiePercent > RandFloat() * 100.0)
			{
				tile.plant = EPlant::GetEntity(mDieType);
				tile.meta = INDEX_NONE;
			}
			else
			{
				tile.meta = 0;
				game.tryMergeArea(game.getPos(tile), this);
			}
		}
	}

	void AreaPlantEntity::harvest(MapTile& tile, GameState& game, bool bCompanionPreference)
	{
		int size = removeArea(tile, game);
		int sizeFactor = Math::Min(mMaxSize, size);
		int num = size * size * sizeFactor;

		auto const& production = EPlant::GetInfo(mPlant).production;
		game.addItem(production.item, production.num * num);
	}

	void AreaPlantEntity::till(MapTile& tile, GameState& game)
	{
		if (tile.meta == INDEX_NONE)
			return;

		removeArea(tile, game);
	}

	int AreaPlantEntity::removeArea(MapTile& tile, GameState& game)
	{
		CHECK(tile.meta >= 0);
		if (tile.meta == 0)
			return 1;

		Area area = game.mAreas[tile.meta - 1];
		Vec2i pos = game.getPos(tile);

		for (int j = area.min.y; j < area.max.y; ++j)
		{
			for (int i = area.min.x; i < area.max.x; ++i)
			{
				if (i == pos.x || j == pos.y)
					continue;

				auto& otherTile = game.getTile(Vec2i(i, j));
				otherTile.plant = nullptr;
				otherTile.growValue = 0;
				otherTile.meta = 0;
			}
		}

		game.removeArea(tile.meta - 1);
		return area.getSize().x;
	}

	GameState::GameState()
	{
		mItems.resize((int)EItem::COUNT, 0);
		mUnlockLevels.resize(EUnlock::COUNT, 0);
	}

	void GameState::init()
	{
		mMainL = FGameAPI::CreateGameState();
		mTiles.resize(3, 3);
		reset();
	}

	void GameState::runExecution(Drone& drone, ScriptFile& codeFile)
	{
		if (drone.executeL)
		{
			stopExecution(drone);
		}

		drone.executeL = IExecuteManager::Get().createThread(mMainL, codeFile.name.c_str());
		drone.executionCode = &codeFile;
	}

	void GameState::stopExecution(Drone& drone)
	{
		if (drone.executeL == nullptr)
			return;

		ExecutionContext context;
		context.mainL = mMainL;
		context.object = &drone;
		IExecuteManager::Get().stop(context);
	}

	Drone* GameState::createDrone(ScriptFile& file)
	{
		Drone* drone = new Drone;
		drone->reset();
		mDrones.push_back(drone);
		return drone;
	}

	bool GameState::isExecutingCode()
	{
		bool bHadExecution = false;
		for (Drone* drone : mDrones)
		{
			if (drone->executeL)
			{
				if (drone->stateFlags & ExecutableObject::ePause)
					return false;

				bHadExecution = true;
			}
		}
		return bHadExecution;
	}

	void GameState::release()
	{
		if (mMainL)
		{
			lua_close(mMainL);
		}

		for (auto drone : mDrones)
		{
			delete drone;
		}
		mDrones.clear();
	}

	Vec2i GameState::getPos(MapTile const& tile) const
	{
		int index = &tile - mTiles.getRawData();
		Vec2i pos;
		mTiles.toCoord(index, pos.x, pos.y);
		return pos;
	}

	void GameState::reset()
	{
		for (auto& tile : mTiles)
		{
			tile.reset();
		}


		mAreas.clear();
		mFreeAreaIndex = INDEX_NONE;
		mCompanions.clear();
		mFreeCompanionIndex = INDEX_NONE;
	}

	void GameState::clear()
	{
		reset();
		for (auto drone : mDrones)
		{
			drone->reset();
			if (mExecutingDrone != drone)
			{
				stopExecution(*drone);
			}
		}

		if (mDrones[0] != mExecutingDrone)
		{
			mExecutingDrone = nullptr;
		}
	}

	void GameState::update(float deltaTime)
	{
		if (!isExecutingCode())
			return;

		UpdateArgs updateArgs;
		updateArgs.speed = 1.0f;
		updateArgs.fTicks = updateArgs.speed * deltaTime * TicksPerSecond;
		updateArgs.deltaTime = deltaTime;

		for (Drone* drone : mDrones)
		{
			update(*drone, updateArgs);
		}
		for (auto& tile : mTiles)
		{
			update(tile, updateArgs);
		}
	}

	void GameState::update(Drone& drone, UpdateArgs const& updateArgs)
	{

		if (drone.executeL == nullptr)
		{
			return;
		}

		drone.fTicksAcc += updateArgs.fTicks;
		if (drone.fTicksAcc <= 1.0)
			return;

		WorldExecuteContext context;
		context.drone = &drone;
		context.game = this;
		context.mainL = mMainL;
		context.object = &drone;

		mExecutingDrone = &drone;
		IExecuteManager::Get().execute(context);
		if (mExecutingDrone == nullptr)
		{
			stopExecution(drone);
		}
		else
		{
			mExecutingDrone = nullptr;
		}
	}

	void GameState::update(MapTile& tile, UpdateArgs const& updateArgs)
	{
		if (tile.plant == nullptr && tile.ground == EGround::Grassland)
		{
			plantInternal(getPos(tile), GEntities.Grass);
		}

		if (tile.plant)
		{
			tile.plant->grow(tile, *this, updateArgs);
		}
	}

	bool GameState::isUnlocked(EUnlock::Type unlock)
	{
		return mUnlockLevels[unlock] > 0;
	}

	void GameState::till(Drone& drone)
	{
		auto& tile = getTile(drone.pos);
		if (tile.plant)
		{
			tile.plant->till(tile, *this);
		}

		if (tile.ground == EGround::Grassland)
		{
			tile.ground = EGround::Soil;
		}
		else
		{
			tile.ground == EGround::Grassland;
		}
		tile.plant = nullptr;
		tile.growTime = 0;
		tile.growValue = 0;
		tile.meta = -1;
	}

	bool GameState::harvest(Drone& drone)
	{
		auto& tile = getTile(drone.pos);
		if (tile.plant == nullptr)
			return false;

		bool bCompanionPreference = false;
		if (BIT(tile.plant->getPlantType()) & PolycultureMask)
		{
			auto& companion = mCompanions[tile.meta];

			auto const& compTile = getTile(companion.pos);
			if (compTile.plant && compTile.plant->getPlantType() == companion.planet)
			{
				bCompanionPreference = true;
			}
			companion.linkIndex = mFreeCompanionIndex;
			mFreeCompanionIndex = tile.meta;
		}

		tile.plant->harvest(tile, *this, bCompanionPreference);
		tile.growValue = 0.0;
		tile.plant = nullptr;
		return true;
	}

	bool GameState::canHarvest(Drone& drone)
	{
		auto& tile = getTile(drone.pos);
		if (tile.plant == nullptr)
			return false;

		return tile.plant->canHarvest(tile, *this);
	}

	bool GameState::swap(Drone& drone, EDirection direction)
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

	bool GameState::plantInternal(Vec2i const& pos, Entity& entity)
	{
		auto& tile = getTile(pos);

		auto ground = EPlant::GetInfo(entity.getPlantType()).ground;
		if (ground != EGround::Any && ground != tile.ground)
			return false;

		auto oldPlant = tile.plant;
		auto oldMeta = tile.meta;
		if (!entity.plant(tile, *this))
		{
			return false;
		}

		if (oldPlant && (BIT(oldPlant->getPlantType()) & PolycultureMask))
		{
			auto& companion = mCompanions[oldMeta];
			companion.linkIndex = mFreeCompanionIndex;
			mFreeCompanionIndex = oldMeta;
		}

		if (BIT(tile.plant->getPlantType()) & PolycultureMask)
		{
			int index = mFreeCompanionIndex;
			if (index == INDEX_NONE)
			{
				index = mCompanions.size();
				mCompanions.push_back(Companion());
			}
			else
			{
				mFreeCompanionIndex = mCompanions[index].linkIndex;
			}

			auto& companion = mCompanions[index];
			do
			{
				companion.planet = PolyculturePlants[RandRange(0, ARRAY_SIZE(PolyculturePlants))];
			} while (companion.planet != tile.plant->getPlantType());


			int const MaxMoves = 3;
			Vec2i posList[2 * MaxMoves * (MaxMoves + 1)];
			int numPos = 0;
			auto AddPos = [&](Vec2i const& tPos)
			{
				if (!mTiles.checkRange(tPos))
					return;

				posList[numPos] = tPos;
				++numPos;
			};

			for (int moves = 1; moves <= MaxMoves; ++moves)
			{
				for (int xMoves = 0; xMoves < moves; ++xMoves)
				{
					int yMoves = moves - xMoves;
					AddPos(pos + Vec2i(xMoves, yMoves));
					AddPos(pos + Vec2i(xMoves, -yMoves));
					AddPos(pos + Vec2i(-xMoves, yMoves));
					AddPos(pos + Vec2i(-xMoves, -yMoves));
				}
			}
			companion.pos = posList[RandRange(0, numPos)];
			tile.meta = index;
		}
		return true;
	}

	bool GameState::plant(Drone& drone, Entity& entity)
	{
		auto& tile = getTile(drone.pos);
		if (tile.plant)
		{
			if (tile.plant == &entity)
				return false;

			bool canReplace = tile.plant->getPlantType() == EPlant::Grass || tile.plant->getPlantType() == EPlant::DeadPumpkin;
			if (!canReplace)
				return false;
		}

		return plantInternal(drone.pos, entity);
	}

	bool GameState::move(Drone& drone, EDirection direction)
	{
		static Vec2i const moveOffset[] =
		{
			Vec2i(1,0),
			Vec2i(-1,0),
			Vec2i(0,1),
			Vec2i(0,-1),
		};

		Vec2i pos = drone.pos + moveOffset[direction];
		switch (direction)
		{
		case East:
			if (pos.x >= mTiles.getSizeX())
				pos.x = 0;
			break;
		case West:
			if (pos.x < 0)
				pos.x = mTiles.getSizeX() - 1;
			break;
		case North:
			if (pos.y >= mTiles.getSizeY())
				pos.y = 0;
			break;
		case South:
			if (pos.y < 0)
				pos.y = mTiles.getSizeY() - 1;
			break;
		}
		drone.pos = pos;
		return true;
	}

	Entity* GameState::getCompanion(Drone& drone, Vec2i& outPos)
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

	int GameState::tryMergeArea(Vec2i const& pos, Entity* entity)
	{
		int result = 0;

		if (pos == Vec2i(2, 2))
		{
			int aa = 1;
		}

		Area validArea;
		validArea.min = Vec2i(0, 0);
		validArea.max = mTiles.getSize();

		int maxSize = mTiles.getSize().x;
		for (int size = 2; size <= maxSize; ++size)
		{
			Area checkArea;
			checkArea.min = pos - Vec2i(size - 1, size - 1);
			checkArea.max = pos + Vec2i(size, size);

			checkArea = checkArea.intersection(validArea);

			if (!checkArea.isValid())
				continue;

			Vec2i checkSize = checkArea.getSize();
			if (checkSize.x < size || checkSize.y < size)
				continue;

			Vec2i tPos;
			for (tPos.y = checkArea.min.y; tPos.y <= checkArea.max.y - size; ++tPos.y)
			{
				for (tPos.x = checkArea.min.x; tPos.x <= checkArea.max.x - size; ++tPos.x)
				{
					Vec2i blockPos;
					int id = tryMergeArea(tPos, size, entity, blockPos);
					if (id > 0)
					{
						result = id;
						goto Merged;
					}
					else if (id == INDEX_NONE)
					{
						if (blockPos.x > pos.x)
						{
							validArea.max.x = Math::Min(blockPos.x, validArea.max.x);
						}
						else if (blockPos.x < pos.x)
						{
							validArea.min.x = Math::Max(blockPos.x, validArea.min.x);
						}
						if (blockPos.y > pos.y)
						{
							validArea.max.y = Math::Min(blockPos.y, validArea.max.y);
						}
						else if (blockPos.y < pos.y)
						{
							validArea.min.y = Math::Max(blockPos.y, validArea.min.y);
						}
					}
				}
			}
		Merged:
			;
		}

		return result;
	}

	int GameState::tryMergeArea(Vec2i const& pos, int size, Entity* entity, Vec2i& outBlockPos)
	{
		Area testArea;
		testArea.min = pos;
		testArea.max = pos + Vec2i(size, size);

		for (int oy = 0; oy < size; ++oy)
		{
			for (int ox = 0; ox < size; ++ox)
			{
				Vec2i tPos = pos + Vec2i(ox, oy);

				auto const& tile = getTile(tPos);
				if (tile.plant != entity || tile.meta == INDEX_NONE)
				{
					outBlockPos = tPos;
					return INDEX_NONE;
				}

				if (tile.meta > 0)
				{
					auto const& area = mAreas[tile.meta - 1];
					if (!testArea.contain(area))
					{
						return 0;
					}
				}
			}
		}

		int id = addArea(testArea);
		for (int oy = 0; oy < size; ++oy)
		{
			for (int ox = 0; ox < size; ++ox)
			{
				Vec2i tPos = pos + Vec2i(ox, oy);
				auto& tile = getTile(tPos);
				if (tile.meta > 0 && tile.meta != id + 1)
				{
					removeArea(tile.meta - 1);
				}
				tile.meta = id + 1;
			}
		}
		return id + 1;
	}

}//namespace TFWR