#ifndef CARExpansion_h__
#define CARExpansion_h__

#include "CARDefine.h"
#include "CARGameplaySetting.h"

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

		EXP_HALFLINGS_I ,                //?T
 		EXP_HALFLINGS_II ,               //?T

		EXP_BASIC,
		EXP_TEST,
		NUM_EXPANSIONS,
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

	struct ExpansionContent
	{
		Expansion   exp;
		TileDefine* defines;
		size_t      numDefines;
	};

	extern TArrayView< ExpansionContent const > gAllExpansionTileContents;

	class GameplaySetting;
	void AddExpansionRule(GameplaySetting& setting, Expansion exp);

}//namespace CAR

#endif // CARExpansion_h__
