#include "TFWREntities.h"

#include "TFWRGameLogic.h"

#include "InlineString.h"
#include "RandomUtility.h"

namespace TFWR
{
	EntityLibrary GEntities;

	EntityLibrary::EntityLibrary()
		:Grass(EPlant::Grass)
		, Bush(EPlant::Bush)
		, Tree(EPlant::Tree)
		, Pumpkin(EPlant::Pumpkin, 5, 20, EPlant::DeadPumpkin)
		, Dead_Pumpkin(EPlant::DeadPumpkin)
		, Cactus(EPlant::Cactus)
		, Sunflower(EPlant::Sunflower)
		, Hedge(EPlant::Hedge)
		, Treasure(EPlant::Treasure)
	{

	}

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
				return &GEntities.Cactus;
			case EPlant::Sunflower:
				return &GEntities.Sunflower;
			case EPlant::Dionsaur:
				break;
			case EPlant::DeadPumpkin: return &GEntities.Dead_Pumpkin;
			case EPlant::Hedge: return &GEntities.Hedge;
			case EPlant::Treasure: return &GEntities.Treasure;
			case EPlant::COUNT:
				break;
			}
			return nullptr;
		}
	}

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
	bool BasePlantEntity::plant(MapTile& tile, GameLogic& game)
	{
		tile.plant = this;
		tile.growValue = 0.0;
		tile.growTime = 0.0;
		return true;
	}


	void BasePlantEntity::updateGrowValue(MapTile& tile, UpdateArgs const& updateArgs, float growTimeScale)
	{
		auto const&  growTimeRange = PlantGrowthTimeMap[mPlant];
		float growTime = growTimeScale * RandFloat(growTimeRange.min, growTimeRange.max);

		// Growth speed scales from 1x at water level 0 to 5x at water level 1
		float waterMultiplier = 1.0f + 4.0f * Math::Clamp(tile.waterValue, 0.0f, 1.0f);

		tile.growValue += waterMultiplier * updateArgs.speed * updateArgs.deltaTime / growTime;
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
	PLANT_INFO(EPlant::Sunflower, EGround::Any, EItem::Power, 1);

	namespace EPlant
	{
		Entity* GetEntity(EPlant::Type plant);
		PlantInfo& GetInfo(EPlant::Type plant)
		{
			return GPlantInfoMap[plant];
		}
	}

	void BasePlantEntity::harvest(MapTile& tile, GameLogic& game, bool bCompanionPreference)
	{
		auto const& production = EPlant::GetInfo(mPlant).production;
		if (tile.infected)
		{
			// Split yield: half normal item, half weird substance
			int normalYield = production.num / 2;
			int weirdYield = production.num - normalYield;  // Remainder goes to weird substance
			game.addItem(production.item, normalYield);
			game.addItem(EItem::Weird_Substance, weirdYield);
		}
		else
		{
			game.addItem(production.item, production.num);
		}
	}

	std::string BasePlantEntity::getDebugInfo(MapTile const& tile)
	{
		return InlineString<>::Make("%s\n%f", toString(mPlant), tile.growValue);
	}

	std::string EmptyEntity::getDebugInfo(MapTile const& tile)
	{
		return InlineString<>::Make("%s", toString(mPlant));
	}

	void TreePlantEntity::doGrow(MapTile& tile, GameLogic& game, UpdateArgs const& updateArgs)
	{
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
	}

	void AreaPlantEntity::onRipening(MapTile& tile, GameLogic& game)
	{
		if (mDiePercent > RandFloat() * 100.0 && !game.bForceRipen)
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

	void AreaPlantEntity::harvest(MapTile& tile, GameLogic& game, bool bCompanionPreference)
	{
		CHECK(tile.meta >= 0);
		int size = 1;
		if (tile.meta != 0)
		{
			Area area = game.mAreas[tile.meta - 1];
			size = area.getSize().x;
		}
		int sizeFactor = Math::Min(mMaxSize, size);
		int num = size * size * sizeFactor;
		auto const& production = EPlant::GetInfo(mPlant).production;
		int totalYield = production.num * num;

		if (tile.infected)
		{
			// Split yield: half normal item, half weird substance
			int normalYield = totalYield / 2;
			int weirdYield = totalYield - normalYield;  // Remainder goes to weird substance
			game.addItem(production.item, normalYield);
			game.addItem(EItem::Weird_Substance, weirdYield);
		}
		else
		{
			game.addItem(production.item, totalYield);
		}
	}

	void AreaPlantEntity::doRemove(MapTile& tile, GameLogic& game)
	{
		removeArea(tile, game);
	}

	void AreaPlantEntity::removeArea(MapTile& tile, GameLogic& game)
	{
		if (tile.meta <= 0)
			return;

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
				otherTile.growTime = 0;
			}
		}

		game.removeArea(tile.meta - 1);
	}

	bool SortableEntity::plant(MapTile& tile, GameLogic& game)
	{
		// Initialize cactus with random size 0-9
		if (!BasePlantEntity::plant(tile, game))
			return false;
		tile.meta = RandRange(0, 10);  // Size 0-9
		return true;
	}

	bool SortableEntity::isSorted(MapTile const& tile, GameLogic& game)
	{
		if (tile.growValue != 1.0f)
			return false;  // Must be fully grown

		int mySize = tile.meta;
		Vec2i myPos = game.getPos(tile);

		// Check North (should be >= mySize)
		Vec2i northPos = myPos + GDirectionOffset[2];  // North
		if (game.mTiles.checkRange(northPos))
		{
			auto& northTile = game.getTile(northPos);
			if (northTile.plant && northTile.plant->getPlantType() == EPlant::Cactus)
			{
				if (northTile.growValue != 1.0f || (int)northTile.meta < mySize)
					return false;
			}
		}

		// Check East (should be >= mySize)
		Vec2i eastPos = myPos + GDirectionOffset[0];  // East
		if (game.mTiles.checkRange(eastPos))
		{
			auto& eastTile = game.getTile(eastPos);
			if (eastTile.plant && eastTile.plant->getPlantType() == EPlant::Cactus)
			{
				if (eastTile.growValue != 1.0f || (int)eastTile.meta < mySize)
					return false;
			}
		}

		// Check South (should be <= mySize)
		Vec2i southPos = myPos + GDirectionOffset[3];  // South
		if (game.mTiles.checkRange(southPos))
		{
			auto& southTile = game.getTile(southPos);
			if (southTile.plant && southTile.plant->getPlantType() == EPlant::Cactus)
			{
				if (southTile.growValue != 1.0f || (int)southTile.meta > mySize)
					return false;
			}
		}

		// Check West (should be <= mySize)
		Vec2i westPos = myPos + GDirectionOffset[1];  // West
		if (game.mTiles.checkRange(westPos))
		{
			auto& westTile = game.getTile(westPos);
			if (westTile.plant && westTile.plant->getPlantType() == EPlant::Cactus)
			{
				if (westTile.growValue != 1.0f || (int)westTile.meta > mySize)
					return false;
			}
		}

		return true;
	}

	int SortableEntity::harvestRecursive(Vec2i const& pos, GameLogic& game)
	{
		if (!game.mTiles.checkRange(pos))
			return 0;

		auto& tile = game.getTile(pos);
		if (tile.plant == nullptr || tile.plant->getPlantType() != EPlant::Cactus)
			return 0;

		if (tile.growValue != 1.0f)
			return 0;  // Not fully grown

		int count = 1;
		game.removePlant(tile);

		// Check if sorted and recursively harvest neighbors
		// We check if neighbors can be harvested (they're sorted)
		for (int dir = 0; dir < 4; ++dir)
		{
			Vec2i neighborPos = pos + GDirectionOffset[dir];
			if (game.mTiles.checkRange(neighborPos))
			{
				auto& neighborTile = game.getTile(neighborPos);
				if (neighborTile.plant && neighborTile.plant->getPlantType() == EPlant::Cactus)
				{
					// Check if neighbor is sorted before harvesting
					if (isSorted(neighborTile, game))
					{
						count += harvestRecursive(neighborPos, game);
					}
				}
			}
		}

		return count;
	}

	void SortableEntity::harvest(MapTile& tile, GameLogic& game, bool bCompanionPreference)
	{
		if (tile.growValue != 1.0f)
			return;  // Can't harvest if not fully grown

		Vec2i pos = game.getPos(tile);

		// Check if this cactus and all neighbors are sorted
		bool allSorted = isSorted(tile, game);
		for (int dir = 0; dir < 4; ++dir)
		{
			Vec2i neighborPos = pos + GDirectionOffset[dir];
			if (game.mTiles.checkRange(neighborPos))
			{
				auto& neighborTile = game.getTile(neighborPos);
				if (neighborTile.plant && neighborTile.plant->getPlantType() == EPlant::Cactus)
				{
					if (!isSorted(neighborTile, game))
					{
						allSorted = false;
						break;
					}
				}
			}
		}

		int harvestedCount = 0;
		if (allSorted)
		{
			// Harvest recursively
			harvestedCount = harvestRecursive(pos, game);
		}
		else
		{
			// Just harvest this one
			game.removePlant(tile);
			harvestedCount = 1;
		}

		// Reward: n^2 cacti for harvesting n cacti
		int reward = harvestedCount * harvestedCount;
		if (tile.infected)
		{
			// Split yield: half normal item, half weird substance
			int normalYield = reward / 2;
			int weirdYield = reward - normalYield;  // Remainder goes to weird substance
			game.addItem(EItem::Cactus, normalYield);
			game.addItem(EItem::Weird_Substance, weirdYield);
		}
		else
		{
			game.addItem(EItem::Cactus, reward);
		}
	}

	bool SunflowerEntity::plant(MapTile& tile, GameLogic& game)
	{
		if (!BasePlantEntity::plant(tile, game))
			return false;

		// Generate random petals: 7-15
		tile.petals = RandRange(7, 16);  // 16 is exclusive, so max is 15
		return true;
	}

	void SunflowerEntity::harvest(MapTile& tile, GameLogic& game, bool bCompanionPreference)
	{
		auto const& production = EPlant::GetInfo(mPlant).production;

		// Count total sunflowers on the map
		int sunflowerCount = 0;
		int maxPetals = 0;
		for (auto& mapTile : game.mTiles)
		{
			if (mapTile.plant && mapTile.plant->getPlantType() == EPlant::Sunflower)
			{
				sunflowerCount++;
				maxPetals = Math::Max(maxPetals, mapTile.petals);
			}
		}

		// If harvesting max petals and >= 10 sunflowers, get 5x power
		int powerYield = production.num;
		if (sunflowerCount >= 10 && tile.petals == maxPetals)
		{
			powerYield *= 5;
		}

		if (tile.infected)
		{
			// Split yield: half normal item, half weird substance
			int normalYield = powerYield / 2;
			int weirdYield = powerYield - normalYield;
			game.addItem(production.item, normalYield);
			game.addItem(EItem::Weird_Substance, weirdYield);
		}
		else
		{
			game.addItem(production.item, powerYield);
		}
	}

} // namespace TFWR
