#ifndef TDDefine_h__
#define TDDefine_h__

#include "CppVersion.h"
#include "FastDelegate/FastDelegate.h"

#include "Math/TVector2.h"
#include "Math/Vector2.h"

#include <cmath>
#include <cassert>

namespace TowerDefend
{
	typedef TVector2< int >   Vec2i;
	typedef ::Math::Vector2   Vector2;

	typedef unsigned char uint8;
	float const gTowerDefendFPS = 30;

	inline float normalize( Vector2& vec )
	{
		float len = sqrt( vec.length2() );
		if ( len > FLOAT_DIV_ZERO_EPSILON)
		{
			vec *= ( 1.0f / len );
		}
		return len;
	}

#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif 

	class Renderer;
	class Entity;
	class Unit;
	class Building;
	class Actor;
	class World;

	enum ComID
	{ 
		CID_NULL            =  0  ,

		CID_MOVE            = -1  ,
		CID_STOP            = -2  ,
		CID_HOLD            = -3  ,
		CID_PATROL          = -4  ,
		CID_ATTACK          = -5  ,

		CID_BUILD           = -7  ,
		CID_RALLY_POINT     = -8  ,
		CID_CHOICE_BLG      = -9  ,
		CID_CHOICE_BLG_ADV  = -10 ,
		CID_CANCEL          = -11 ,

		CID_SPECIAL_0       = -12 ,
		CID_SPECIAL_1       = -13 ,
		CID_SPECIAL_2       = -14 ,
		CID_SPECIAL_3       = -15 ,
		CID_SPECIAL_4       = -16 ,
		CID_SPECIAL_5       = -17 ,
	};

	struct PlayerInfo
	{
		unsigned id;
		int gold;
		int wood;
		int team;
		int curPopulation;
		int maxPopulation;
	};


	enum ActorId
	{
		AID_NONE ,
		AID_BUILDING_TYPE_START ,
		AID_BT_TOWER_CUBE     ,
		AID_BT_TOWER_CIRCLE   ,
		AID_BT_TOWER_DIAMON   ,
		AID_BT_TOWER_TRIANGLE ,
		AID_BT_BARRACK        ,

		AID_UNIT_TYPE_START ,
		AID_UT_BUILDER_1 ,
		AID_UT_MOUSTER_1 ,
	};

	inline bool isBuilding( ActorId id ){ return id < AID_UNIT_TYPE_START;  }
	inline bool isUnit    ( ActorId id ){ return id > AID_UNIT_TYPE_START;  }


	typedef unsigned ComMapID;

#define  COM_MAP_NULL   ComMapID( 0 )
#define  COM_MAP_CANCEL ComMapID( 1 )
	int const COM_MAP_ELEMENT_NUN = 15;

	struct ComMap
	{
		ComMapID mapID;
		ComMapID baseID;
		struct KeyValue
		{
			int  comID;
			unsigned key;
		} value[ COM_MAP_ELEMENT_NUN ];
	};


	struct DamageInfo
	{
		Entity* attacker;
		float   power;

	};


	class IActorFactory;

	struct UnitInfo
	{
		ActorId        actorID;
		IActorFactory* factory;
		unsigned       baseComMapID;
		int            goldCast;
		int            woodCast;

		float          maxHealthValue;
		int            population;
		float          attackPower;
		float          attackSpeed;
		float          defence;
		float          moveSpeed;
		float          colRadius;
		long           produceTime;
		unsigned       extendComMapID[4];
	};


	struct BuildingInfo
	{
		ActorId        actorID;
		IActorFactory* factory;
		unsigned       baseComMapID;
		int            goldCast;
		int            woodCast;

		float          maxHealthValue;
		Vec2i          blgSize;
		long           buildTime;
	};

}//namespace TowerDefend

#endif // TDDefine_h__
