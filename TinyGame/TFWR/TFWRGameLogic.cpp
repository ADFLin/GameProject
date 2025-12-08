#include "TFWRGameLogic.h"

#include "TFWREntities.h"
#include "TFWRScript.h"


#include "InlineString.h"
#include "RandomUtility.h"

#if 1
#include "Async/Coroutines.h"
#include "Misc/DebugDraw.h"
#include "RenderUtility.h"
#define DEBUG_RUNNING() Coroutines::IsRunning()
#define DEBUG_YEILD() if (DEBUG_RUNNING()){ CO_YEILD(nullptr); }
#define DEBUG_POINT(V, C) DrawDebugPoint(V, C, 5);
#define DEBUG_LINE(V1, V2, C) DrawDebugLine(V1, V2, C, 2);
#define DEBUG_RECT(P, S, C) DrawDebugRect(P, S, C, 2);
#define DEBUG_TEXT(V, TEXT, C) DrawDebugText(V, TEXT, C);
#else
#define DEBUG_RUNNING() false
#define DEBUG_YEILD()
#define DEBUG_POINT(V, C)
#define DEBUG_LINE(V1, V2)
#define DEBUG_RECT(P, S, C)
#define DEBUG_TEXT(V, TEXT, C)
#endif
#include <random>

namespace TFWR
{
	constexpr int TicksPerSecond = 400;

	GameLogic::GameLogic()
	{
		mItems.resize((int)EItem::COUNT, 0);
		mUnlockLevels.resize(EUnlock::COUNT, 0);
	}

	void GameLogic::init(int size)
	{
		mTiles.resize(size, size);
		reset();
	}

	void GameLogic::resizeMap(int size)
	{
		clear();
		mTiles.resize(size, size);
		reset();
	}

	Vec2i GameLogic::getPos(MapTile const& tile) const
	{
		int index = mTiles.getIndex(tile);
		Vec2i pos;
		mTiles.toCoord(index, pos.x, pos.y);
		return pos;
	}

	void GameLogic::reset()
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

	void GameLogic::clear()
	{
		reset();
	}

	void GameLogic::update(float deltaTime)
	{
		UpdateArgs updateArgs;
		updateArgs.speed = 1.0f;
		
		// Apply power speed boost: 2x speed if power is available
		if (mItems[(int)EItem::Power] > 0)
		{
			updateArgs.speed *= 2.0f;
		}
		
		updateArgs.fTicks = updateArgs.speed * deltaTime * TicksPerSecond;
		updateArgs.deltaTime = updateArgs.speed * deltaTime;

		// Water generation: 1 tank every 10 seconds (doubled if Watering is unlocked)
		mWaterTankFillProgress += updateArgs.deltaTime;

		int wateringLevel = mUnlockLevels[EUnlock::Watering];
		float tankInterval = 10.0f;
		if (wateringLevel > 0)
		{
			tankInterval /= float(1 << wateringLevel);  // Double rate
		}

		while (mWaterTankFillProgress >= tankInterval)
		{
			mWaterTankFillProgress -= tankInterval;
			mItems[(int)EItem::Water]++;
		}

		// Fertilizer generation: 1 tank every 10 seconds (doubled if Fertilizer is unlocked)
		mFertilizerTankFillProgress += updateArgs.deltaTime;

		int fertilizerLevel = mUnlockLevels[EUnlock::Fertilizer];
		float fertilizerInterval = 10.0f;
		if (fertilizerLevel > 0)
		{
			fertilizerInterval /= float(1 << fertilizerLevel);  // Double rate
		}

		while (mFertilizerTankFillProgress >= fertilizerInterval)
		{
			mFertilizerTankFillProgress -= fertilizerInterval;
			mItems[(int)EItem::Fertilizer]++;
		}

		for (auto& tile : mTiles)
		{
			update(tile, updateArgs);
		}
	}

	void GameLogic::update(MapTile& tile, UpdateArgs const& updateArgs)
	{
		// Water evaporation: triggered randomly between 0.8 and 1.2 seconds
		// When triggered, water decreases by 1% of current amount
		if (tile.waterEvaporationTimer <= 0.0f && tile.waterValue > 0.0f)
		{
			// Trigger evaporation
			tile.waterValue *= 0.99f;  // Decrease by 1%
			
			// Schedule next evaporation: uniform random between 0.8 and 1.2 seconds
			tile.waterEvaporationTimer = 0.8f + RandFloat() * 0.4f;
		}
		
		tile.waterEvaporationTimer -= updateArgs.deltaTime;

		if (tile.plant == nullptr && tile.ground == EGround::Grassland)
		{
			plantInternal(getPos(tile), *EPlant::GetEntity(EPlant::Grass));
		}

		if (tile.plant && tile.growValue < 1.0)
		{
			tile.plant->doGrow(tile, *this, updateArgs);
			if (tile.growValue >= 1.0)
			{
				tile.growValue = 1.0;
				tile.plant->onRipening(tile, *this);
			}
		}
	}

	void GameLogic::removePlant(MapTile& tile)
	{
		CHECK(tile.plant);

		tile.plant->doRemove(tile, *this);
		if (BIT(tile.plant->getPlantType()) & PolycultureMask)
		{
			auto& companion = mCompanions[tile.meta];
			companion.linkIndex = mFreeCompanionIndex;
			mFreeCompanionIndex = tile.meta;
		}

		tile.plant = nullptr;
		tile.growTime = 0;
		tile.growValue = 0;
		tile.meta = -1;
	}

	bool GameLogic::isUnlocked(EUnlock::Type unlock) const
	{
		return mUnlockLevels[unlock] > 0;
	}

	// Mapping tables for getUnlockLevel
	static constexpr EUnlock::Type PlantToUnlock[] =
	{
		EUnlock::Grass,          // EPlant::Grass
		EUnlock::Trees,          // EPlant::Bush
		EUnlock::Carrots,        // EPlant::Carrots
		EUnlock::Trees,          // EPlant::Tree
		EUnlock::Pumpkins,       // EPlant::Pumpkin
		EUnlock::Cactus,         // EPlant::Cactus
		EUnlock::Sunflowers,     // EPlant::Sunflower
		EUnlock::Dinosaurs,      // EPlant::Dionsaur
		EUnlock::Mazes,          // EPlant::Hedge
		EUnlock::Mazes,          // EPlant::Treasure
		EUnlock::Pumpkins,       // EPlant::DeadPumpkin
	};
	static_assert(ARRAY_SIZE(PlantToUnlock) == EPlant::COUNT, "PlantToUnlock size mismatch");

	static constexpr EUnlock::Type GroundToUnlock[] =
	{
		EUnlock::Auto_Unlock,    // EGround::Any
		EUnlock::Grass,          // EGround::Grassland
		EUnlock::Plant,          // EGround::Soil
	};

	static constexpr EUnlock::Type ItemToUnlock[] =
	{
		EUnlock::Grass,          // EItem::Hay
		EUnlock::Trees,          // EItem::Wood
		EUnlock::Pumpkins,       // EItem::Pumpkin
		EUnlock::Watering,       // EItem::Water
		EUnlock::Cactus,         // EItem::Cactus
		EUnlock::Fertilizer,     // EItem::Fertilizer
		EUnlock::Mazes,          // EItem::Weird_Substance
		EUnlock::Speed,          // EItem::Power
		EUnlock::Mazes,          // EItem::Gold
	};
	static_assert(ARRAY_SIZE(ItemToUnlock) == (int)EItem::COUNT, "ItemToUnlock size mismatch");

	int GameLogic::getUnlockLevel(EUnlock::Type unlock)
	{
		return mUnlockLevels[unlock];
	}

	int GameLogic::getUnlockLevel(EItem item)
	{
		EUnlock::Type unlock = ItemToUnlock[(int)item];
		if (unlock != EUnlock::Auto_Unlock)
		{
			return mUnlockLevels[unlock];
		}
		return 0;
	}

	int GameLogic::getUnlockLevel(EGround ground)
	{
		EUnlock::Type unlock = GroundToUnlock[(int)ground];
		if (unlock != EUnlock::Auto_Unlock)
		{
			return mUnlockLevels[unlock];
		}
		return 0;
	}

	int GameLogic::getUnlockLevel(Entity& entity)
	{
		EPlant::Type plantType = entity.getPlantType();
		EUnlock::Type unlock = PlantToUnlock[plantType];
		if (unlock != EUnlock::Auto_Unlock)
		{
			return mUnlockLevels[unlock];
		}
		return 0;
	}

	GameOperationResult GameLogic::doTill(DroneState& drone)
	{
		auto& tile = getTile(drone.pos);
		if (tile.plant)
		{
			removePlant(tile);
		}

		if (tile.ground == EGround::Grassland)
		{
			tile.ground = EGround::Soil;
		}
		else
		{
			tile.ground == EGround::Grassland;
		}
		
		consumePowerForAction();
		return GameOperationResult::Success(200);
	}

	GameOperationResult GameLogic::doHarvest(DroneState& drone)
	{
		auto& tile = getTile(drone.pos);
		if (tile.plant == nullptr)
			return GameOperationResult::Failure(1);

		// Check if harvesting maze treasure
		if (tile.plant->getPlantType() == EPlant::Hedge && tile.mazeID < (int)mMazes.size())
		{
			auto& maze = mMazes[tile.mazeID];
			// Check if at treasure position
			if (drone.pos == maze.treasurePos)
			{
				// Award gold
				if (maze.treasureValue > 0)
				{
					addItem(EItem::Gold, maze.treasureValue);
				}
				// Remove the maze
				removeMaze(tile.mazeID);
				consumePowerForAction();
				return GameOperationResult::Success(200);
			}
			else
			{
				// Harvesting non-treasure hedge removes the maze
				removeMaze(tile.mazeID);
				consumePowerForAction();
				return GameOperationResult::Success(200);
			}
		}

		if (tile.growValue < 1.0)
			return GameOperationResult::Failure(1);

		bool bCompanionPreference = false;
		if (BIT(tile.plant->getPlantType()) & PolycultureMask)
		{
			auto& companion = mCompanions[tile.meta];

			auto const& compTile = getTile(companion.pos);
			if (compTile.plant && compTile.plant->getPlantType() == companion.planet)
			{
				bCompanionPreference = true;
			}
		}

		tile.plant->harvest(tile, *this, bCompanionPreference);

		removePlant(tile);
		consumePowerForAction();
		return GameOperationResult::Success(200);
	}

	bool GameLogic::canHarvest(DroneState& drone)
	{
		auto& tile = getTile(drone.pos);
		if (tile.plant == nullptr)
			return false;

		return tile.plant->canHarvest(tile, *this);
	}

	GameOperationResult GameLogic::doUseItem(Vec2i const& pos, EItem item, int nTimes)
	{
		auto& tile = getTile(pos);
		switch (item)
		{
		case EItem::Water:
			if (mItems[(int)EItem::Water] > nTimes)
			{
				mItems[(int)EItem::Water] -= nTimes;
				tile.waterValue = Math::Min(tile.waterValue + nTimes * 0.25f, 1.0f);
				return GameOperationResult::Success(200);
			}
			break;

		case EItem::Fertilizer:
			if (mItems[(int)EItem::Fertilizer] > nTimes && tile.plant != nullptr)
			{
				mItems[(int)EItem::Fertilizer] -= nTimes;
				tile.growTime = Math::Max(0.0f, tile.growTime - nTimes * 2.0f);
				tile.infected = true;
				return GameOperationResult::Success(200);
			}
			break;
		case EItem::Weird_Substance:
			if (mItems[(int)EItem::Weird_Substance] >= nTimes && tile.plant != nullptr)
			{
				// Check if using on a bush to create a maze
				if (tile.plant->getPlantType() == EPlant::Bush)
				{
					mItems[(int)EItem::Weird_Substance] -= nTimes;
					
					// Calculate maze size based on amount and unlock level
					int mazeLevel = mUnlockLevels[EUnlock::Mazes];
					int baseSize = nTimes;
					if (mazeLevel > 0)
					{
						// Each maze upgrade doubles treasure but also doubles substance needed
						baseSize = nTimes / (1 << (mazeLevel - 1));
					}
					
					// Generate maze at current position
					Vec2i mazeSize(baseSize, baseSize);
					generateMaze(pos, mazeSize);
					
					return GameOperationResult::Success(200);
				}
				// Check if using on treasure position to relocate it (maze reuse)
				else if (tile.plant->getPlantType() == EPlant::Hedge && tile.mazeID < (int)mMazes.size())
				{
					auto& maze = mMazes[tile.mazeID];
					// Check if at treasure position
					if (pos == maze.treasurePos)
					{
						auto& area = mAreas[maze.areaID];
						
						// Check if we can still relocate (max 300 times)
						int mazeArea = area.getSize().x * area.getSize().y;
						if (maze.treasureValue / mazeArea < 300)
						{
							mItems[(int)EItem::Weird_Substance] -= nTimes;
							
							// Increase treasure value by one full maze worth
							maze.treasureValue += mazeArea;
							
							// Relocate treasure to random position in maze
							Vec2i newTreasurePos;
							newTreasurePos.x = area.min.x + RandRange(0, area.getSize().x);
							newTreasurePos.y = area.min.y + RandRange(0, area.getSize().y);
							
							// Update treasure position in maze
							maze.treasurePos = newTreasurePos;
							
							return GameOperationResult::Success(200);
						}
					}
				}
				else if (mItems[(int)EItem::Weird_Substance] > nTimes)
				{
					// Original behavior: toggle infection
					mItems[(int)EItem::Weird_Substance] -= nTimes;
					if (nTimes % 2 == 1)
					{
						// Toggle infection for the tile and all adjacent plants
						tile.infected = !tile.infected;
						for (int dir = 0; dir < 4; ++dir)
						{
							Vec2i adjPos = pos + GDirectionOffset[dir];
							if (mTiles.checkRange(adjPos))
							{
								auto& adjTile = getTile(adjPos);
								if (adjTile.plant != nullptr)
								{
									adjTile.infected = !adjTile.infected;
								}
							}
						}
					}
					return GameOperationResult::Success(200);
				}
			}
			break;
		}

		return GameOperationResult::Failure(1);
	}

	GameOperationResult GameLogic::doSwapPlant(Vec2i const& pos, EDirection direction)
	{
		Vec2i targetPos = pos + GDirectionOffset[direction];
		if (!mTiles.checkRange(targetPos))
			return GameOperationResult::Failure(1);

		MapTile& tileTarget = getTile(targetPos);
		MapTile& tile = getTile(pos);
		if (tileTarget.plant)
		{
			auto plantType = tileTarget.plant->getPlantType();
			if (!EPlant::IsSwappable(plantType))
				return GameOperationResult::Failure(1);

			auto ground = EPlant::GetInfo(plantType).ground;
			if (ground != EGround::Any && ground != tile.ground)
				return GameOperationResult::Failure(1);
		}

		if (tile.plant)
		{
			auto plantType = tile.plant->getPlantType();
			if (!EPlant::IsSwappable(plantType))
				return GameOperationResult::Failure(1);

			auto ground = EPlant::GetInfo(plantType).ground;
			if (ground != EGround::Any && ground != tileTarget.ground)
				return GameOperationResult::Failure(1);
		}

		tile.swap(tileTarget);
		return GameOperationResult::Success(200);
	}

	bool GameLogic::canMove(Vec2i const& pos, EDirection direction)
	{
		auto const& tile = getTile(pos);

		// Check if in a hedge maze
		if (tile.plant && tile.plant->getPlantType() == EPlant::Hedge)
		{
			// Check if there's a wall in the given direction
			return (tile.linkMask & BIT(direction)) != 0;
		}

		// Not in a maze, can always move
		return true;
	}

	void GameLogic::plantAndRipen(Vec2i const& pos, Entity& entity)
	{
		auto& tile = getTile(pos);
		auto ground = EPlant::GetInfo(entity.getPlantType()).ground;
		if (ground != EGround::Any && ground != tile.ground)
		{
			tile.ground = ground;
		}

		auto oldPlant = tile.plant;
		auto oldMeta = tile.meta;
		if (!entity.plant(tile, *this))
		{
			return;
		}

		postPlant(pos, tile, oldPlant, oldMeta);

		tile.growValue = 1.0;
		bForceRipen = true;
		tile.plant->onRipening(tile, *this);
		bForceRipen = false;
	}

	bool GameLogic::plantInternal(Vec2i const& pos, Entity& entity)
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

		postPlant(pos, tile, oldPlant, oldMeta);
		return true;
	}

	void GameLogic::postPlant(Vec2i const& pos, MapTile& tile, Entity* oldPlant, int oldMeta)
	{
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
	}

	GameOperationResult GameLogic::doPlant(DroneState& drone, Entity& entity)
	{
		auto& tile = getTile(drone.pos);
		if (tile.plant)
		{
			if (tile.plant == &entity)
				return GameOperationResult::Failure(1);

			bool canReplace = tile.plant->getPlantType() == EPlant::Grass || tile.plant->getPlantType() == EPlant::DeadPumpkin;
			if (!canReplace)
				return GameOperationResult::Failure(1);
		}

		if (!plantInternal(drone.pos, entity))
			return GameOperationResult::Failure(1);

		consumePowerForAction();
		return GameOperationResult::Success(200);
	}

	GameOperationResult GameLogic::doMove(DroneState& drone, EDirection direction)
	{
		Vec2i pos = drone.pos + GDirectionOffset[direction];

		auto const& tile = getTile(drone.pos);

		if (tile.plant && tile.plant->getPlantType() == EPlant::Hedge)
		{
			if (!(tile.linkMask & BIT(direction)))
				return GameOperationResult::Failure(1);
		}

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
		consumePowerForAction();
		return GameOperationResult::Success(200);
	}

	Entity* GameLogic::getPlantEntity(Vec2i const& pos)
	{
		auto& tile = getTile(pos);

		// Check if in a maze and at treasure position
		if (tile.plant && tile.plant->getPlantType() == EPlant::Hedge)
		{
			if (tile.mazeID < (int)mMazes.size())
			{
				auto& maze = mMazes[tile.mazeID];
				if (pos == maze.treasurePos)
				{
					// At treasure position, return Treasure entity
					return EPlant::GetEntity(EPlant::Treasure);
				}
			}
		}

		return tile.plant;
	}

	EGround GameLogic::getGroundType(Vec2i const& pos)
	{
		auto& tile = getTile(pos);
		return tile.ground;
	}

	Entity* GameLogic::getCompanion(Vec2i const& pos, Vec2i& outPos)
	{
		auto& tile = getTile(pos);

		if (tile.plant == nullptr)
			return nullptr;
		if (!(PolycultureMask & BIT(tile.plant->getPlantType())))
			return nullptr;

		CHECK(tile.meta >= 0);
		auto const& companion = mCompanions[tile.meta];
		outPos = companion.pos;
		return EPlant::GetEntity(companion.planet);
	}

	int GameLogic::measure(Vec2i const& pos, BoxingValue* outputs[])
	{
		auto& tile = getTile(pos);
		if (tile.plant == nullptr)
			return 0;

		switch (tile.plant->getPlantType())
		{
		case EPlant::Sunflower:
			*outputs[0] = tile.petals;
			return 1;
		case EPlant::Hedge:
			{
				auto const& maze = mMazes[tile.mazeID];
				*outputs[0] = maze.treasurePos.x;
				*outputs[1] = maze.treasurePos.y;
			}
			break;
		case EPlant::Cactus:
			*outputs[0] = (int64)tile.meta;
			return 1;
		default:
			break;
		}
		return 0;
	}

	void GameLogic::generateMaze(Vec2i const& pos, Vec2i const& size)
	{
		Area area;
		area.min = pos;
		area.max = pos + size;

		int id = createMaze(area);

		auto& maze = mMazes[id];
		// Treasure position is relative to area min
		maze.treasurePos = pos + Vec2i(RandRange(0, size.x), RandRange(0, size.y));

		auto CheckArea = [&](Vec2i const& pos)
		{
			return area.min.x <= pos.x && pos.x < area.max.x &&
				   area.min.y <= pos.y && pos.y < area.max.y;
		};

		std::random_device rd;
		std::mt19937 g(rd());
		TArray<Vec2i> posStack;
		Vec2i curPos = pos + Vec2i(RandRange(0, size.x), RandRange(0, size.y));
		for (;;)
		{
			int index = 0;
			int dirs[] = { 0, 1, 2, 3 };
			std::shuffle(dirs, dirs + 4, g);
			for (; index < 4; ++index)
			{
				int dir = dirs[index];
				Vec2i linkPos = curPos + GDirectionOffset[dir];
				if (!CheckArea(linkPos))
					continue;

				auto& linkTile = getTile(linkPos);
				if (linkTile.linkMask != 0)
					continue;

				auto& tile = getTile(curPos);

				linkTile.linkMask |= BIT(InverseDir(dir));
				tile.linkMask |= BIT(dir);
				posStack.push_back(curPos);
				curPos = linkPos;
				break;
			}

			if (index >= 4)
			{
				if (posStack.empty())
					break;

				curPos = posStack.back();
				posStack.pop_back();
			}
		}
		
		// Set maze IDs on all tiles
		for (int j = area.min.y; j < area.max.y; ++j)
		{
			for (int i = area.min.x; i < area.max.x; ++i)
			{
				Vec2i tPos = Vec2i(i, j);
				auto& tile = getTile(tPos);
				tile.mazeID = id;
			}
		}
		
		// Store treasure value in MazeInfo
		maze.treasureValue = size.x * size.y;  // Gold equal to maze area
	}

	int GameLogic::createMaze(Area const& area)
	{
		for (int j = area.min.y; j < area.max.y; ++j)
		{
			for (int i = area.min.x; i < area.max.x; ++i)
			{
				Vec2i tPos = Vec2i(i, j);

				auto& tile = getTile(tPos);
				if (tile.plant)
				{
					removePlant(tile);
				}

				tile.plant = EPlant::GetEntity(EPlant::Hedge);
				tile.linkMask = 0;
			}
		}

		int id;
		if (mFreeAreaIndex != INDEX_NONE)
		{
			id = mFreeAreaIndex;
		}
		else
		{
			id = mMazes.size();
			mMazes.addDefault(1);
		}

		auto& maze = mMazes[id];
		maze.areaID = addArea(area);
		return id;
	}

	void GameLogic::removeMaze(int id)
	{
		auto& maze = mMazes[id];
		auto const& area = mAreas[maze.areaID];

		for (int j = area.min.y; j < area.max.y; ++j)
		{
			for (int i = area.min.x; i < area.max.x; ++i)
			{
				Vec2i tPos = Vec2i(i, j);
				auto& tile = getTile(tPos);
				CHECK(tile.plant == EPlant::GetEntity(EPlant::Hedge));
				tile.plant = nullptr;
				tile.meta = 0;
			}
		}

		removeArea(maze.areaID);
		maze.link = mFreeAreaIndex;
		mFreeAreaIndex = id;
	}

	int GameLogic::addArea(Area const& area)
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

	void GameLogic::removeArea(int id)
	{
		if (id == mFreeAreaIndex || mAreas[id].link != INDEX_NONE)
			return;

		mAreas[id].link = mFreeAreaIndex;
		mFreeAreaIndex = id;
	}

	// Note: tryMergeArea implementation continues in next part...
	// This is a complex function that I'll need to copy from TFWRGameState.cpp

	int GameLogic::tryMergeArea(Vec2i const& pos, Entity* entity)
	{
		int result = 0;

		Area validArea;
		validArea.min = Vec2i(0, 0);
		validArea.max = mTiles.getSize();

		auto UpdateCheckArea = [&](Area& checkArea, int size) -> bool
		{
			checkArea = checkArea.intersection(validArea);

			if (!checkArea.isValid())
				return false;

			Vec2i checkSize = checkArea.getSize();
			if (checkSize.x < size || checkSize.y < size)
				return false;

			return true;
		};

		constexpr int MaxBlockCheck = 5;
		Vec2i blocks[MaxBlockCheck];
		int numBlock = 0;

		auto CheckBlocks = [&](Area& area)->bool
		{
			for (int i = 0; i < numBlock; ++i)
			{
				auto const& block = blocks[i];
				if (area.min.x <= block.x && block.x < area.max.x &&
					area.min.y <= block.y && block.y < area.max.y)
					return false;
			}
			return true;
		};

		int maxSize = mTiles.getSize().x;

		// Build prefix-sum for entity occupancy to quickly reject SxS areas.
		// ps is (W+1) x (H+1) with standard integral-image layout.
		Vec2i worldSize = mTiles.getSize();
		int W = worldSize.x;
		int H = worldSize.y;
		std::vector<int> ps((W + 1) * (H + 1), 0);
		auto psIndex = [&](int x, int y) { return y * (W + 1) + x; };
		for (int y = 0; y < H; ++y)
		{
			for (int x = 0; x < W; ++x)
			{
				auto const& tile = getTile(Vec2i(x, y));
				int v = (tile.plant == entity && tile.meta != INDEX_NONE) ? 1 : 0;
				ps[psIndex(x + 1, y + 1)] = v + ps[psIndex(x, y + 1)] + ps[psIndex(x + 1, y)] - ps[psIndex(x, y)];
			}
		}

		// Fast mismatch finder: check edges first (likely failures) then interior.
		auto FindMismatch = [&](Area const& area) -> Vec2i 
		{
			int x1 = area.min.x;
			int y1 = area.min.y;
			int x2 = area.max.x - 1;
			int y2 = area.max.y - 1;

			// top/bottom rows
			for (int x = x1; x <= x2; ++x)
			{
				auto const& t = getTile(Vec2i(x, y1));
				if (!(t.plant == entity && t.meta != INDEX_NONE))
					return Vec2i(x, y1);
			}
			for (int x = x1; x <= x2; ++x)
			{
				auto const& t = getTile(Vec2i(x, y2));
				if (!(t.plant == entity && t.meta != INDEX_NONE))
					return Vec2i(x, y2);
			}
			// left/right cols
			for (int y = y1; y <= y2; ++y)
			{
				auto const& t = getTile(Vec2i(x1, y));
				if (!(t.plant == entity && t.meta != INDEX_NONE))
					return Vec2i(x1, y);
			}
			for (int y = y1; y <= y2; ++y)
			{
				auto const& t = getTile(Vec2i(x2, y));
				if (!(t.plant == entity && t.meta != INDEX_NONE))
					return Vec2i(x2, y);
			}
			// interior (rare)
			for (int y = y1; y <= y2; ++y)
			{
				for (int x = x1; x <= x2; ++x)
				{
					auto const& t = getTile(Vec2i(x, y));
					if (!(t.plant == entity && t.meta != INDEX_NONE))
						return Vec2i(x, y);
				}
			}
			return Vec2i(x1, y1);
		};

		auto AddBlock = [&](Vec2i const &blockPos)
		{
			if (blockPos.y == pos.y)
			{
				if (blockPos.x > pos.x)
					validArea.max.x = Math::Min(blockPos.x, validArea.max.x);
				else if (blockPos.x < pos.x)
					validArea.min.x = Math::Max(blockPos.x, validArea.min.x);
			}
			else if (blockPos.x == pos.x)
			{
				if (blockPos.y > pos.y)
					validArea.max.y = Math::Min(blockPos.y, validArea.max.y);
				else if (blockPos.y < pos.y)
					validArea.min.y = Math::Max(blockPos.y, validArea.min.y);
			}
			else
			{
				if (numBlock < MaxBlockCheck)
				{
					blocks[numBlock] = blockPos;
					++numBlock;
				}
			}
		};

		for (int size = 2; size <= maxSize; ++size)
		{
			Area checkArea;
			checkArea.min = pos - Vec2i(size - 1, size - 1);
			checkArea.max = pos + Vec2i(size, size);

			Area debugCheckArea = checkArea;

			if (!UpdateCheckArea(checkArea, size))
				continue;

			if (DEBUG_RUNNING())
			{
				DEBUG_POINT(Vector2(pos) + Vector2(0.5, 0.5), Color3f(1, 1, 0));
				DEBUG_RECT(debugCheckArea.min, debugCheckArea.getSize(), Color3f(1, 0, 0));
				DEBUG_RECT(checkArea.min, checkArea.getSize(), Color3f(0, 0, 1));
				DEBUG_RECT(validArea.min, validArea.getSize(), Color3f(0, 1, 0));
				DEBUG_YEILD();
			}

			checkArea.max -= Vec2i(size, size);

			Vec2i tPos;
			for (tPos.y = checkArea.min.y; tPos.y <= checkArea.max.y; ++tPos.y)
			{
				for (tPos.x = checkArea.min.x; tPos.x <= checkArea.max.x; ++tPos.x)
				{
					Area testArea;
					testArea.min = tPos;
					testArea.max = tPos + Vec2i(size, size);

					if (!CheckBlocks(testArea))
						continue;

					int x1 = tPos.x;
					int y1 = tPos.y;
					int x2 = x1 + size - 1;
					int y2 = y1 + size - 1;
					int areaSum = ps[psIndex(x2 + 1, y2 + 1)] - ps[psIndex(x1, y2 + 1)] - ps[psIndex(x2 + 1, y1)] + ps[psIndex(x1, y1)];

					if (areaSum != size * size)
					{
						Vec2i blockPos = FindMismatch(testArea);

						AddBlock(blockPos);
						continue;
					}

					Vec2i blockPos;
					int id = tryMergeArea(testArea, size, entity, blockPos);
					if (id > 0)
					{
						result = id;
						goto End;
					}
					else if (id == INDEX_NONE)
					{
						AddBlock(blockPos);
					}
				}
			}
		End:
			;
		}

		return result;
	}

	int GameLogic::tryMergeArea(Area const& testArea, int size, Entity* entity, Vec2i& outBlockPos)
	{
		for (int oy = 0; oy < size; ++oy)
		{
			for (int ox = 0; ox < size; ++ox)
			{
				Vec2i tPos = testArea.min + Vec2i(ox, oy);

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

		int meta = addArea(testArea) + 1;
		for (int oy = 0; oy < size; ++oy)
		{
			for (int ox = 0; ox < size; ++ox)
			{
				Vec2i tPos = testArea.min + Vec2i(ox, oy);
				auto& tile = getTile(tPos);
				if (tile.meta > 0 && tile.meta != meta)
				{
					removeArea(tile.meta - 1);
				}
				tile.meta = meta;
			}
		}
		return meta;
	}

} // namespace TFWR
