#pragma once
#ifndef CARExpansion_H_2FF2081E_ECAB_4679_AEA1_97373D6DAFB8
#define CARExpansion_H_2FF2081E_ECAB_4679_AEA1_97373D6DAFB8

#include "CARDefine.h"

#include "Template/ArrayView.h"

namespace CAR
{

	enum Expansion : uint8
	{
		// ?:process -:Done +:Tested T:Title Edit
		EXP_INNS_AND_CATHEDRALS,         //
		EXP_TRADERS_AND_BUILDERS,        //
		EXP_THE_PRINCESS_AND_THE_DRAGON, //+T +Dragon
		EXP_THE_TOWER,                   //-Tower
		EXP_ABBEY_AND_MAYOR,             //
		EXP_KING_AND_ROBBER,             //
		EXP_BRIDGES_CASTLES_AND_BAZAARS, //
		EXP_HILLS_AND_SHEEP,             //-T -Vineyards -Hill -shepherd -ST_HalfSeparate

		EXP_THE_RIVER,                   //+T +RiverRule
		EXP_THE_RIVER_II,                //+T 

		EXP_CASTLES,                     //?T ?CastleTile ?RoadCityScoring ?CastleFeature
		EXP_PHANTOM,
		EXP_HERETICS_AND_SHRINES,
		EXP_ABBOT ,
		EXP_CROP_CIRCLE_I,
		EXP_CROP_CIRCLE_II,
		EXP_THE_FLY_MACHINES,
		EXP_GOLDMINES,
		EXP_THE_SCHOOL ,
		EXP_THE_FESTIVAL ,

		EXP_THE_ROBBERS ,
		EXP_HALFLINGS_I ,                //?T
 		EXP_HALFLINGS_II ,               //?T


		EXP_THE_COUNT_OF_CARCASSONNE ,
		EXP_LA_PORXADA ,
		EXP_LITTLE_BUILDINGS ,
		EXP_MAGE_AND_WITCH ,
		EXP_THE_MESSSAGES ,
		EXP_MONASTERIES ,

		NUM_EXPANSIONS,

		EXP_NONE ,
		EXP_BASIC,
		EXP_TEST,
	};

	enum GameRule
	{
		eHardcore,

		eSmallCity,
		eDoubleTurnDrawImmediately,
		eCantMoveFairy,
		ePrincessTileMustRemoveKnightOrBuilder,
		eMoveDragonBeforeScoring,
		eTowerCaptureEverything,
	};

	enum class Rule
	{
		eInn,
		eCathedral,
		eBigMeeple,
		eBuilder,
		eTraders,
		ePig,
		eKingAndRobber,
		ePrinecess,
		eDragon,
		eFariy,
		eTower,
		eTeacher ,

		eWagon,
		eMayor,
		eBarn,
		ePhantom,
		eAbbot,
		eRobber ,

		eUseHill,
		eShepherdAndSheep,
		eBazaar,
		eBridge,
		eLaPorxada,
		eCastleToken,
		eUseVineyard,
		eLittleBuilding,
		eMageAndWitch,
		eMessage ,
		eGold ,
		eFlyMahine ,
		eCropCircle ,
		eShrine ,

		eFestival ,
		
		eHaveGermanCastleTile,
		eHaveAbbeyTile,
		eHaveRiverTile,
		eHaveHalflingTile,
		
		eHaveMonaster,
		eRemoveOriginalCloister ,

		//////////////
		eHardcore,
		eSmallCity,
		eDoubleTurnDrawImmediately,
		eCantMoveFairy,
		ePrincessTileMustRemoveKnightOrBuilder,
		eMoveDragonBeforeScoring,
		eTowerCaptureEverything,

		TotalNum,
	};

	struct TileDefine
	{
		uint8  numPiece;
		uint8  linkType[4];
		uint8  sideLink[2];
		uint16 roadLink[2];
		uint32 content;
		uint16 sideContent[4];
		uint8  centerFarmMask;
		uint8  farmLink[6];
		uint8  tag;
	};

	struct ExpansionContent
	{
		Expansion   exp;
		TileDefine* defines;
		size_t      numDefines;
	};

	extern TArrayView< ExpansionContent const > gAllExpansionTileContents;


}//namespace CAR

#endif // CARExpansion_H_2FF2081E_ECAB_4679_AEA1_97373D6DAFB8

