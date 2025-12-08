#pragma once
#ifndef TFWREntities_H
#define TFWREntities_H

#include "TFWRCore.h"
#include <string>

namespace TFWR
{
	class GameLogic;

	class Entity
	{
	public:
		virtual ~Entity() = default;
		virtual EPlant::Type getPlantType() = 0;
		virtual const char* TypeName() const = 0;
		virtual bool plant(MapTile& tile, GameLogic& game) { return false; }
		virtual void doGrow(MapTile& tile, GameLogic& game, UpdateArgs const& updateArgs){}
		virtual void onRipening(MapTile& tile, GameLogic& game){}
		virtual bool canHarvest(MapTile& tile, GameLogic& game) { return false; }
		virtual void harvest(MapTile& tile, GameLogic& game, bool bCompanionPreference) {}
		virtual void doRemove(MapTile& tile, GameLogic& game) {}
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
		const char* TypeName() const override { return "BasePlantEntity"; }
		bool plant(MapTile& tile, GameLogic& game);

		virtual void doGrow(MapTile& tile, GameLogic& game, UpdateArgs const& updateArgs)
		{
			updateGrowValue(tile, updateArgs);
		}
		virtual void onRipening(MapTile& tile, GameLogic& game)
		{

		}
		bool canHarvest(MapTile& tile, GameLogic& game)
		{
			return tile.growValue == 1.0;
		}
		void harvest(MapTile& tile, GameLogic& game, bool bCompanionPreference);


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
		void doGrow(MapTile& tile, GameLogic& game, UpdateArgs const& updateArgs);
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

		bool plant(MapTile& tile, GameLogic& game)
		{
			if (!BaseClass::plant(tile, game))
				return false;

			tile.meta = INDEX_NONE;
			return true;
		}

		void onRipening(MapTile& tile, GameLogic& game);
		void harvest(MapTile& tile, GameLogic& game, bool bCompanionPreference);
		void doRemove(MapTile& tile, GameLogic& game);
		void removeArea(MapTile& tile, GameLogic& game);

		int   mMaxSize;
		float mDiePercent;
		EPlant::Type mDieType;
	};

	class SortableEntity : public BasePlantEntity
	{
	public:
		using BasePlantEntity::BasePlantEntity;
		
		bool plant(MapTile& tile, GameLogic& game);
		void harvest(MapTile& tile, GameLogic& game, bool bCompanionPreference);

	private:
		bool isSorted(MapTile const& tile, GameLogic& game);
		int harvestRecursive(Vec2i const& pos, GameLogic& game);
	};

	class SunflowerEntity : public BasePlantEntity
	{
	public:
		using BasePlantEntity::BasePlantEntity;

		bool plant(MapTile& tile, GameLogic& game);
		void harvest(MapTile& tile, GameLogic& game, bool bCompanionPreference);
	};

	class EmptyEntity : public Entity
	{
	public:
		EmptyEntity(EPlant::Type inPlant) : mPlant(inPlant) {}

		EPlant::Type getPlantType() { return mPlant; }
		const char* TypeName() const override { return "EmptyEntity"; }
		bool plant(MapTile& tile, GameLogic& game) { return false; }
		void doGrow(MapTile& tile, GameLogic& game, UpdateArgs const& updateArgs) {}
		void onRipening(MapTile& tile, GameLogic& game){}
		bool canHarvest(MapTile& tile, GameLogic& game) { return false; }
		void harvest(MapTile& tile, GameLogic& game, bool bCompanionPreference) {}
		void doRemove(MapTile& tile, GameLogic& game) {}
	std::string getDebugInfo(MapTile const& tile);

		EPlant::Type mPlant;
	};

	struct EntityLibrary
	{
		EntityLibrary();

		SimplePlantEntity Grass;
		SimplePlantEntity Bush;
		TreePlantEntity Tree;
		AreaPlantEntity Pumpkin;
		EmptyEntity Dead_Pumpkin;
		SortableEntity Cactus;
		SunflowerEntity Sunflower;
		EmptyEntity Hedge;
		EmptyEntity Treasure;


		REFLECT_STRUCT_BEGIN(EntityLibrary)
			REF_PROPERTY(Grass)
			REF_PROPERTY(Bush)
			REF_PROPERTY(Tree)
			REF_PROPERTY(Pumpkin)
			REF_PROPERTY(Dead_Pumpkin)
			REF_PROPERTY(Cactus)
			REF_PROPERTY(Sunflower)
			REF_PROPERTY(Hedge)
			REF_PROPERTY(Treasure)
		REFLECT_STRUCT_END()
	};

	extern EntityLibrary GEntities;

} // namespace TFWR

#endif // TFWREntities_H
