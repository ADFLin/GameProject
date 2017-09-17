#ifndef CARDefine_h__201a14a3_46e7_42a6_b6ac_29fbc7d227ba
#define CARDefine_h__201a14a3_46e7_42a6_b6ac_29fbc7d227ba

#include "IntegerType.h"

#include <cassert>

#ifndef BIT
#	define BIT( n ) ( 1 << (n) )
#endif

#ifndef ARRAY_SIZE
#	define ARRAY_SIZE( array ) ( sizeof(array) / sizeof( array[0]) )
#endif

namespace CAR
{
	class MapTile;
	class LevelActor;

	typedef uint32 TileId;
	TileId const FAIL_TILE_ID = TileId(-1);
	TileId const TEMP_TILE_ID = TileId(-2);

	int const FAIL_PLAYER_ID = 31;

	int const ERROR_GROUP_ID = -1;
	int const ABBEY_GROUP_ID = -2;

	enum Expansion : uint8;

	enum SideType : uint8
	{
		eField  = 0,
		eRoad  ,
		eRiver ,
		eCity  ,
		eAbbey , //EXP_ABBEY_AND_MAYOR
		eGermanCastle ,

		eEmptySide ,
	};

	struct TileContent
	{
		enum Enum
		{
			eCloister        = BIT(0) ,
			eCathedral       = BIT(1) , //EXP_INNS_AND_CATHEDRALS
			eVolcano         = BIT(2) , //EXP_THE_PRINCESS_AND_THE_DRAGON 
			eTheDragon       = BIT(3) , //EXP_THE_PRINCESS_AND_THE_DRAGON 
			eMagicPortal     = BIT(4) , //EXP_THE_PRINCESS_AND_THE_DRAGON
			eTowerFoundation = BIT(7) , //EXP_THE_TOWER
			eBazaar          = BIT(8) , //EXP_BRIDGES_CASTLES_AND_BAZAARS
			eHill            = BIT(9) , //EXP_HILLS_AND_SHEEP
			eVineyard        = BIT(10), //EXP_HILLS_AND_SHEEP
			eHalfling        = BIT(11),

			//runtime
			eTemp            = BIT(12) ,
		};

		static unsigned const FeatureMask = eCloister;
	};

	struct  SideContent
	{
		enum Enum
		{
			ePennant    = BIT(0) ,
			eInn        = BIT(1) , //EXP_INNS_AND_CATHEDRALS
			eWineHouse  = BIT(2) , //9 //EXP_TRADEERS_AND_BUILDERS
			eGrainHouse = BIT(3) , //6 //EXP_TRADEERS_AND_BUILDERS
			eClothHouse = BIT(4) , //5 //EXP_TRADEERS_AND_BUILDERS
			ePrincess   = BIT(5) , //EXP_THE_PRINCESS_AND_THE_DRAGON
			eNotSemiCircularCity = BIT(6) , //EXP_BRIDGES_CASTLES_AND_BAZAARS
			eSheep      = BIT(7) ,  //EXP_HILLS_AND_SHEEP use
			eHalfSeparate = BIT(8) , //EXP_HILLS_AND_SHEEP
		};
	};


	struct ActorPos
	{
		enum Enum
		{
			eSideNode ,
			eFarmNode ,
			eTile     ,
			eTower    ,
			eTileCorner ,
		};
		ActorPos( Enum aType , int aMeta )
			:type( aType ),meta( aMeta ){}
		ActorPos(){}

		Enum type;
		int  meta;

		bool operator == (ActorPos const& rhs) const
		{
			if( type != rhs.type )
				return false;
			if( type != ActorPos::eTile && meta != rhs.meta )
				return false;
			return true;
		}
	};


	enum ActorType
	{
		//follower
		eMeeple ,
		eBigMeeple , //EXP_INNS_AND_CATHEDRALS 
		eMayor , //EXP_ABBEY_AND_MAYOR
		eWagon , //EXP_ABBEY_AND_MAYOR
		eBarn  , //EXP_ABBEY_AND_MAYOR
		eShepherd , //EXP_HILLS_AND_SHEEP
		ePhantom , //EXP_PHANTOM
		eAbbot , //CII
		eMage ,
		eWitch ,

		eBuilder ,  //EXP_TRADEERS_AND_BUILDERS
		ePig ,     //EXP_TRADEERS_AND_BUILDERS

		//neutral
		eDragon , //EXP_THE_PRINCESS_AND_THE_DRAGON
		eFariy  , //EXP_THE_PRINCESS_AND_THE_DRAGON

		NUM_ACTOR_TYPE  ,
		NUM_PLAYER_ACTOR_TYPE = eDragon ,
		eNone,
	};

	struct FieldType
	{
		enum Enum
		{
			eActorStart = 0 ,
			eActorEnd   = eActorStart + NUM_PLAYER_ACTOR_TYPE - 1 ,

			//EXP_TRADEERS_AND_BUILDERS
			eGain , 
			eWine ,
			eCloth ,
			//EXP_THE_TOWER
			eTowerPices ,
			//EXP_ABBEY_AND_MAYOR
			eAbbeyPices ,
			//EXP_BRIDGES_CASTLES_AND_BAZAARS
			eBridgePices ,
			eCastleTokens ,
			eTileIdAuctioned ,
			//EXP_CASTLES
			eGermanCastleTiles ,
			//EXP_HALFLINGS_I && EXP_HALFLINGS_II
			eHalflingTiles ,

			NUM,
		};
	};

	enum TileTag
	{
		TILE_NO_TAG = 0,
		TILE_START_TAG ,
		TILE_FRIST_PLAY_TAG ,
		TILE_END_TAG ,
		TILE_ABBEY_TAG , //EXP_ABBEY_AND_MAYOR
		TILE_GERMAN_CASTLE_TAG ,
		TILE_HALFLING_TAG ,
	};

	enum SheepToken
	{
		eWolf = 0,
		eOne ,
		eTwo ,
		eThree ,
		eFour ,
		
		Num ,
	};

	struct ActorInfo
	{
		int       playerId;
		ActorType type;

		bool operator == ( ActorInfo const& rhs ) const
		{
			return playerId == rhs.playerId &&
				type == rhs.type;
		}
	};


}//namespace CAR

#endif // CARDefine_h__201a14a3_46e7_42a6_b6ac_29fbc7d227ba
