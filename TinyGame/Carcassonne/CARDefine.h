#ifndef CARDefine_h__201a14a3_46e7_42a6_b6ac_29fbc7d227ba
#define CARDefine_h__201a14a3_46e7_42a6_b6ac_29fbc7d227ba

#include "IntegerType.h"

#include <cassert>

#ifndef BIT
#	define BIT( n ) ( 1 << (n) )
#endif

#define ARRAY_SIZE( array ) ( sizeof(array) / sizeof( array[0]) )

namespace CAR
{
	typedef uint32 TileId;
	TileId const FAIL_TILE_ID = TileId(-1);

	enum Expansion
	{
		EXP_INNS_AND_CATHEDRALS ,
		EXP_TRADEERS_AND_BUILDERS ,
		EXP_THE_PRINCESS_AND_THE_DRAGON ,
		EXP_THE_TOWER ,
		EXP_ABBEY_AND_MAYOR ,
		EXP_KING_AND_ROBBER ,

		EXP_THE_RIVER ,
		EXP_THE_RIVER_II ,

		EXP_BASIC ,
		EXP_TEST ,
		NUM_EXPANSIONS  ,
		EXP_NULL ,
	};

	enum SideType
	{
		eField  = 0,
		eRoad  ,
		eRiver ,
		eCity  ,
		eAbbey , //EXP_ABBEY_AND_MAYOR
	};

	struct TileContent
	{
		enum Enum
		{
			eCloister     = BIT(0) ,
			eCathedral    = BIT(1) , //EXP_INNS_AND_CATHEDRALS
			eVolcano      = BIT(2) , //EXP_THE_PRINCESS_AND_THE_DRAGON 
			eTheDragon    = BIT(3) , //EXP_THE_PRINCESS_AND_THE_DRAGON 
			eMagicPortal  = BIT(4) , //EXP_THE_PRINCESS_AND_THE_DRAGON
			eCityCloister = BIT(6) , //EXP_THE_PRINCESS_AND_THE_DRAGON  Start //TODO
			eTowerFoundation = BIT(7), //EXP_THE_TOWER

		};

		static unsigned const FeatureMask = eCloister | eCityCloister;
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
	};


	enum ActorType
	{
		//follower
		eMeeple ,
		eBigMeeple , //EXP_INNS_AND_CATHEDRALS 
		eMayor , //EXP_ABBEY_AND_MAYOR
		eWagon , //EXP_ABBEY_AND_MAYOR
		eBarn  , //EXP_ABBEY_AND_MAYOR

		eBuilder ,  //EXP_TRADEERS_AND_BUILDERS
		ePig ,     //EXP_TRADEERS_AND_BUILDERS

		//neutral
		eDragon , //EXP_THE_PRINCESS_AND_THE_DRAGON
		eFariy  , //EXP_THE_PRINCESS_AND_THE_DRAGON

		NUM_ACTOR_TYPE  ,
		NUM_PLAYER_ACTOR_TYPE = ePig ,
		eNone,
	};

	struct FieldType
	{
		enum Enum
		{
			eActorStart = 0 ,
			eActorEnd   = eActorStart + NUM_PLAYER_ACTOR_TYPE  ,

			//EXP_TRADEERS_AND_BUILDERS
			eGain , 
			eWine ,
			eCloth ,
			//EXP_THE_TOWER
			eTowerPices ,
			//EXP_ABBEY_AND_MAYOR
			eAbbeyPices ,

			NUM,
		};
	};

	enum TileTag
	{
		//Must > 0
		TILE_START_TAG = 1,
		TILE_END_TAG ,
		TILE_ABBEY , //EXP_ABBEY_AND_MAYOR
	};

	
	struct TileDefine
	{
		uint8  numPiece;
		uint8  linkType[4];
		uint8  sideLink[2];
		uint16 roadLink[2];
		uint16 content;
		uint16 sideContent[4];
		uint8  centerFarmMask;
		uint8  farmLink[6];
		uint8  tag;
		
	};

	struct ExpansionTileContent
	{
		Expansion  exp;
		TileDefine* defines;
		int         numDefines;
	};

	extern ExpansionTileContent gAllExpansionTileContents[]; 


}//namespace CAR

#endif // CARDefine_h__201a14a3_46e7_42a6_b6ac_29fbc7d227ba
