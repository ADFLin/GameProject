#include "TFWRCore.h"

#include "RandomUtility.h"
#include "InlineString.h"

namespace TFWR
{
	char const* toString(EPlant::Type type)
	{
		switch (type)
		{
		case EPlant::Grass: return "Grass";
		case EPlant::Bush: return "Bush";
		case EPlant::Carrots: return "Carrots";
		case EPlant::Tree: return "Tree";
		case EPlant::Pumpkin: return "Pumpkin";
		case EPlant::Cactus: return "Cactus";
		case EPlant::Sunflower: return "Sunflower";
		case EPlant::Dionsaur: return "Dionsaur";
		}
		return "Unknown";
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
		game.addItem(production.item,  production.num * num);
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


}

