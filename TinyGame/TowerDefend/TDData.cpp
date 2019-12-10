#include "TDPCH.h"

#include "TDDefine.h"
#include "TDLevel.h"

namespace TowerDefend
{
	namespace
	{
		template< class T >
		class CActorFactory : public IActorFactory
		{
		public:
			Actor* create( ActorId aID )
			{
				return new T( aID );
			}
		};

		template< class T >
		static IActorFactory* getFactory()
		{
			static CActorFactory< T > sFactory;
			return &sFactory;
		}
	}


	enum __ComMapID
	{
		CMID_NONE        = COM_MAP_NULL  ,
		CMID_CANCEL      = COM_MAP_CANCEL ,
		CMID_BASE_UNIT     ,
		CMID_BASE_BUILDING ,
		CMID_BASE_BUILDER  ,
		CMID_BASE_TOWER    ,
		CMID_BARRACK ,

		CMID_BLG_BUILDER_1 ,
	};



#define START_DEF_UNITS()\
	UnitInfo const gUnitInfo[] = {\

#define UNIT_B( UTYPE , CLASS ,  GOLD , WOOD  ,POPU , MAX_HEAL , AT , AT_SPEED , DEF , MV_SPEED , COL_R , PROD_TIME , BASE_CID , EX_CID1 , EX_CID2 )\
	{ UTYPE , getFactory< CLASS >() , BASE_CID  , GOLD , WOOD  ,MAX_HEAL , POPU , AT , AT_SPEED , DEF , MV_SPEED , COL_R , PROD_TIME , EX_CID1 , EX_CID2 , CMID_NONE , CMID_NONE },

#define UNIT_A( UTYPE , CLASS , GOLD , WOOD  , POPU ,MAX_HEAL , AT , AT_SPEED , DEF , MV_SPEED , COL_R , PROD_TIME )\
	UNIT_B( UTYPE , CLASS , GOLD , WOOD , POPU , MAX_HEAL , AT , AT_SPEED , DEF , MV_SPEED , COL_R , PROD_TIME , CMID_BASE_UNIT , CMID_NONE , CMID_NONE )


#define END_DEF_UNITS() };


	START_DEF_UNITS()
		    //|   Unit ID       |   Entity   |Gold|Wood|Popu| MAX| AT    | AT    | DEF | MVS | COL|Produce| ComMapID         |  ComMapID         |    ComMapID   |
		    //|                 |    Type    |cast|cast|    | HP | power | Speed |     |     |  R |  Time |  Base            |    EX0            |      EX1      |
		UNIT_B( AID_UT_BUILDER_1 , Builder ,  10,   0,  1 ,  5  ,     1 ,    1 ,  5  ,  1  , 20 ,    500 , CMID_BASE_BUILDER , CMID_BLG_BUILDER_1 , CMID_NONE   )
		UNIT_A( AID_UT_MOUSTER_1 , Unit    ,  10,   0,  1 , 20  ,     1 ,    1 ,  5  ,  1  ,  9 ,    500 )

	END_DEF_UNITS()

#undef  START_DEF_UNITS
#undef  UNIT_A
#undef  UNIT_B
#undef  END_DEF_UNITS

	UnitInfo const& Unit::getUnitInfo( ActorId type )
	{
		assert( isUnit( type ) );
		int index = type - ( AID_UNIT_TYPE_START + 1 );
		assert( gUnitInfo[ index ].actorID == type );
		return gUnitInfo[ index ];
	}


#define START_DEF_BUILDING()\
	BuildingInfo const gBuildingInfo[] = {\

#define BUILDING( AID , CLASS ,  SX , SY , GOLD , WOOD  ,BLD_TIME , LIFE , BASE_CID )\
	{ AID , getFactory< CLASS >() , BASE_CID , GOLD , WOOD  , LIFE , Vec2i( SX , SY ) ,  BLD_TIME },

#define END_DEF_BUILDING() };


START_DEF_BUILDING()
//           |     Building  ID    | Entity      | size | gold |wood | build| life |  ComMapID       |
//           |                     |       Type  | x  y | cast |cast |  time| value|    Base         |
	BUILDING( AID_BT_TOWER_CUBE     , Tower    , 2 , 2 ,  10 ,   0 , 1000 ,   10 , CMID_BASE_TOWER )
	BUILDING( AID_BT_TOWER_CIRCLE   , Tower    , 2 , 2 ,  10 ,   0 , 1000 ,   10 , CMID_BASE_TOWER )
	BUILDING( AID_BT_TOWER_DIAMON   , Tower    , 2 , 2 ,  10 ,   0 , 1000 ,   10 , CMID_BASE_TOWER )
	BUILDING( AID_BT_TOWER_TRIANGLE , Tower    , 2 , 2 ,  10 ,   0 , 1000 ,   10 , CMID_BASE_TOWER )
	BUILDING( AID_BT_BARRACK        , Building , 3 , 3 ,  50 ,  10 , 5000 ,  200 , CMID_BARRACK    )

END_DEF_BUILDING()

#undef  START_DEF_BUILDING
#undef  BUILDING
#undef  END_DEF_BUILDING

	BuildingInfo const& Building::getBuildingInfo( ActorId type )
	{
		assert( isBuilding( type ) );
		int index = type - ( AID_BUILDING_TYPE_START + 1 );
		assert( gBuildingInfo[ index ].actorID == type );
		return gBuildingInfo[ index ];
	}


#define DEF_COM_MAP( ID , PARENT_ID )\
	{  ID , PARENT_ID , {

#define COM( CID , KEY ) { CID , KEY } ,
#define COM_NULL() COM( CID_NULL , 0 )
#define END_COM_MAP() }  },

	ComMap gComMapInfo[] = 
	{
		DEF_COM_MAP( CMID_NONE , CMID_NONE )
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		END_COM_MAP()

		DEF_COM_MAP( CMID_CANCEL , CMID_NONE )
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM( CID_CANCEL , VK_ESCAPE )
		END_COM_MAP()

		DEF_COM_MAP( CMID_BASE_UNIT , CMID_NONE )
		COM( CID_ATTACK , EKeyCode::A ) 
		COM( CID_MOVE   , EKeyCode::M )
		COM( CID_HOLD   , EKeyCode::H )
		COM_NULL() COM_NULL()

		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		END_COM_MAP()

		DEF_COM_MAP( CMID_BASE_UNIT , CMID_NONE )
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM( CID_RALLY_POINT , EKeyCode::R )
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		END_COM_MAP()

		DEF_COM_MAP( CMID_BASE_BUILDER , CMID_BASE_UNIT )
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()

		COM( CID_CHOICE_BLG , EKeyCode::B ) COM( CID_CHOICE_BLG_ADV , EKeyCode::V )
		COM_NULL() COM_NULL() COM_NULL()
		END_COM_MAP()

		DEF_COM_MAP( CMID_BASE_TOWER , CMID_NONE )
		COM( CID_ATTACK , EKeyCode::A ) COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		END_COM_MAP()

		DEF_COM_MAP( CMID_BARRACK , CMID_BASE_BUILDING )
		COM( AID_UT_MOUSTER_1 , EKeyCode::M )
		COM( AID_UT_BUILDER_1 , EKeyCode::B )
		COM_NULL() COM_NULL() COM_NULL()

		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		END_COM_MAP()

		DEF_COM_MAP( CMID_BLG_BUILDER_1 , CMID_CANCEL )
		COM( AID_BT_TOWER_CUBE     , EKeyCode::C )
		COM( AID_BT_TOWER_CIRCLE   , EKeyCode::R )
		COM( AID_BT_TOWER_DIAMON   , EKeyCode::D )
		COM( AID_BT_TOWER_TRIANGLE , EKeyCode::T )
		COM( AID_BT_BARRACK        , EKeyCode::B )

		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		COM_NULL() COM_NULL() COM_NULL() COM_NULL() COM_NULL()
		END_COM_MAP()
	};


#undef DEF_COM_MAP
#undef COM
#undef COM_NULL
#undef END_COM_MAP

	ComMap& Actor::getComMap( ComMapID id )
	{
		return gComMapInfo[id];
	}

}//namespace TowerDefend
