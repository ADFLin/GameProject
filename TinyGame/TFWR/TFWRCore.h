#pragma once
#ifndef TFWRCore_H_32DFB864_39C4_41F9_9896_52191D6A1768
#define TFWRCore_H_32DFB864_39C4_41F9_9896_52191D6A1768

#include "Math/TVector2.h"
#include "Math/GeometryPrimitive.h"
#include "ReflectionCollect.h"

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

	struct MapTile;
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

	char const* toString(EPlant::Type type);

}//namespace TFWR

#endif // TFWRCore_H_32DFB864_39C4_41F9_9896_52191D6A1768