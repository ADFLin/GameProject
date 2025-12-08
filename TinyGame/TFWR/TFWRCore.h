#pragma once
#ifndef TFWRCore_H_32DFB864_39C4_41F9_9896_52191D6A1768
#define TFWRCore_H_32DFB864_39C4_41F9_9896_52191D6A1768

#include "Math/TVector2.h"
#include "Math/GeometryPrimitive.h"
#include "ReflectionCollect.h"

namespace TFWR
{

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

	typedef TVector2<int> Vec2i;
	Vec2i constexpr GDirectionOffset[] =
	{
		Vec2i(1,0),
		Vec2i(-1,0),
		Vec2i(0,1),
		Vec2i(0,-1),
	};

	FORCEINLINE int InverseDir(int dir) { return 2 * (dir / 2) + (dir + 1) % 2; }


	class Entity;

	using Area = Math::TAABBox<Vec2i>;

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

	REF_ENUM_BEGIN(EUnlock::Type)
		REF_ENUM(Cactus)
		REF_ENUM(Carrots)
		REF_ENUM(Costs)
		REF_ENUM(Debug)
		REF_ENUM(Debug_2)
		REF_ENUM(Dictionaries)
		REF_ENUM(Dinosaurs)
		REF_ENUM(Expand)
		REF_ENUM(Fertilizer)
		REF_ENUM(Functions)
		REF_ENUM(Grass)
		REF_ENUM(Hats)
		REF_ENUM(Import)
		REF_ENUM(Leaderboard)
		REF_ENUM(Lists)
		REF_ENUM(Loops)
		REF_ENUM(Mazes)
		REF_ENUM(Megafarm)
		REF_ENUM(Operators)
		REF_ENUM(Plant)
		REF_ENUM(Polyculture)
		REF_ENUM(Pumpkins)
		REF_ENUM(Senses)
		REF_ENUM(Simulation)
		REF_ENUM(Speed)
		REF_ENUM(Sunflowers)
		REF_ENUM(The_Farmers_Remains)
		REF_ENUM(Timing)
		REF_ENUM(Top_Hat)
		REF_ENUM(Trees)
		REF_ENUM(Utilities)
		REF_ENUM(Variables)
		REF_ENUM(Watering)
	REF_ENUM_END()

	enum class EGround
	{
		Grassland,
		Soil,

		COUNT,
		Any = -1,
	};

	REF_ENUM_BEGIN(EGround)
		REF_ENUM(Grassland)
		REF_ENUM(Soil)
	REF_ENUM_END()

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
		float  waterEvaporationTimer;
		bool   infected = false;

		union
		{
			int32  meta;
			int    petals;
			struct
			{
				uint32 linkMask : 4;
				uint32 mazeID : 28;
			};
		};


		void reset()
		{
			ground = EGround::Grassland;
			plant = nullptr;
			growValue = 0;
			waterValue = 0;
			waterEvaporationTimer = 0;
			infected = false;
			meta = 0;
		}

		void swap(MapTile& target)
		{
			using std::swap;
			swap(plant, target.plant);
			swap(growValue, target.growValue);
			swap(growTime, target.growTime);
			swap(meta, target.meta);
		}
	};
	class Entity;

	enum class EItem
	{
		Hay,
		Wood,
		Pumpkin,
		Water,
		Cactus,
		Fertilizer,
		Weird_Substance,
		Power,
		Gold,
		COUNT,
	};
	

	
	REF_ENUM_BEGIN(EItem)
		REF_ENUM(Hay)
		REF_ENUM(Wood)
		REF_ENUM(Pumpkin)
		REF_ENUM(Water)
		REF_ENUM(Cactus)
		REF_ENUM(Fertilizer)
		REF_ENUM(Weird_Substance)
		REF_ENUM(Power)
		REF_ENUM(Gold)
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
			Hedge,
			Treasure,

			DeadPumpkin,

			COUNT,

			None = -1,
		};

		bool IsSwappable(EPlant::Type plant);
		Entity* GetEntity(EPlant::Type plant);
		PlantInfo& GetInfo(EPlant::Type plant);
	}

	char const* toString(EPlant::Type type);

}//namespace TFWR

#endif // TFWRCore_H_32DFB864_39C4_41F9_9896_52191D6A1768