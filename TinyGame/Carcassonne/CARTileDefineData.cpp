#include "CARDefine.h"

#pragma warning( disable : 4482 ; error : 4002 )
namespace CAR
{

#define LF SideType::eField
#define LC SideType::eCity
#define LR SideType::eRoad
#define LL SideType::eRiver
#define LA SideType::eAbbey

#define SPE SideContent::ePennant
#define SIN SideContent::eInn
#define SWH SideContent::eWineHouse
#define SGH SideContent::eGrainHouse
#define SCH SideContent::eClothHouse
#define SPR SideContent::ePrincess

#define TCL TileContent::eCloister
#define TCA TileContent::eCathedral
#define TCC TileContent::eCityCloister
#define TMP TileContent::eMagicPortal
#define TDR TileContent::eTheDragon
#define TVO TileContent::eVolcano
#define TTF TileContent::eTowerFoundation

#define BIT2( A , B )     ( BIT(A)|BIT(B) )
#define BIT3( A , B , C ) ( BIT(A)|BIT(B)|BIT(C) )
#define BIT4( A , B , C , D ) ( BIT(A)|BIT(B)|BIT(C)|BIT(D) )

#define SL2( A , B )     { BIT2(A,B) , 0  }
#define SL3( A , B , C ) { BIT3(A,B,C) , 0  } 
#define SL22( A , B , C , D ) { BIT2(A,B) , BIT2(C,D)  }
#define SL_ALL  { 0xf }
#define SL_NONE { 0x0 }

#define FL_RE 0xff

	static TileDefine DataBasic[] =
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*00*/  { 5, { LF,LC,LF,LF }, SL3(0,2,3)   , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*01*/ 	{ 2, { LC,LC,LF,LF }, SL2(2,3)     , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*02*/ 	{ 3, { LC,LF,LC,LF }, SL2(1,3)     , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*03*/ 	{ 3, { LR,LC,LF,LR }, SL2(0,3)     , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(0,7) , FL_RE } , 0 } ,
/*04*/ 	{ 3, { LF,LC,LR,LR }, SL2(2,3)     , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*05*/ 	{ 3, { LR,LC,LR,LR }, SL_NONE      , SL3(0,2,3)   , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , BIT2(7,0) , FL_RE } , 0 } ,
/*06*/ 	{ 4, { LR,LC,LR,LF }, SL2(0,2)     , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , TILE_START_TAG } ,
/*07*/ 	{ 3, { LF,LC,LC,LF }, SL22(0,3,1,2), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*08*/ 	{ 3, { LR,LC,LC,LR }, SL22(0,3,1,2), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(0,7) , FL_RE } , 0 } ,
/*09*/ 	{ 2, { LF,LC,LC,LF }, SL22(0,3,1,2), SL_NONE      , 0, { 0 ,SPE, 0 , 0 }, 0, { FL_RE } , 0 } ,
/*10*/ 	{ 2, { LR,LC,LC,LR }, SL22(0,3,1,2), SL_NONE      , 0, { 0 ,SPE, 0 , 0 }, 0, { BIT2(0,7) , FL_RE } , 0  } ,
/*11*/ 	{ 1, { LC,LF,LC,LF }, SL2(0,2)     , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*12*/ 	{ 2, { LC,LF,LC,LF }, SL2(0,2)     , SL_NONE      , 0, {SPE, 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*13*/ 	{ 3, { LC,LC,LC,LF }, SL3(0,1,2)   , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*14*/ 	{ 1, { LC,LC,LC,LR }, SL3(0,1,2)   , SL_ALL       , 0, { 0 , 0 , 0 , 0 }, 0, { BIT4(7,0,1,2) , FL_RE } , 0 } ,
/*15*/ 	{ 1, { LC,LC,LC,LF }, SL3(0,1,2)   , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*16*/ 	{ 2, { LC,LC,LC,LR }, SL3(0,1,2)   , SL_ALL       , 0, {SPE, 0 , 0 , 0 }, 0, { BIT4(7,0,1,2) , FL_RE } , 0 } ,
/*17*/ 	{ 1, { LC,LC,LC,LC }, SL_ALL       , SL_NONE      , 0, {SPE, 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*18*/ 	{ 4, { LF,LF,LF,LF }, SL_ALL       , SL_NONE      ,TCL,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*19*/ 	{ 2, { LF,LF,LF,LR }, SL3(0,1,2)   , SL2(3,4)     ,TCL,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*20*/ 	{ 8, { LF,LR,LF,LR }, SL2(1,3)     , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT4(7,0,1,2) , FL_RE } , 0 } ,
/*21*/ 	{ 9, { LF,LF,LR,LR }, SL22(0,1,2,3), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*22*/ 	{ 4, { LR,LF,LR,LR }, SL_NONE      , SL3(0,2,3)   , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , BIT2(7,0) , FL_RE } , 0 } ,
/*23*/ 	{ 1, { LR,LR,LR,LR }, SL_NONE      , SL_ALL       , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
	};

	static TileDefine DataRiver1[] = 
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*00*/ 	{ 1, { LF,LF,LF,LL }, SL3(0,1,2)   , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , TILE_START_TAG } ,
/*01*/ 	{ 1, { LF,LL,LF,LF }, SL3(0,2,3)   , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , TILE_END_TAG } ,
/*02*/ 	{ 1, { LC,LL,LL,LC }, SL22(0,3,1,2), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(3,4) , FL_RE } , 0 } ,
/*03*/ 	{ 1, { LC,LL,LR,LL }, SL2(1,3)     , SL2(0,3)     , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
/*04*/ 	{ 1, { LL,LC,LL,LC }, SL2(0,2)     , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , BIT4(5,6,7,0) } , 0 } ,
/*05*/ 	{ 1, { LL,LF,LF,LL }, SL22(0,3,1,2), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(7,0) , FL_RE } , 0 } ,
/*06*/ 	{ 1, { LL,LF,LL,LR }, SL2(0,2)     , SL2(3,4)     ,TCL,{ 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , BIT2(7,0) , FL_RE } , 0 } ,
/*07*/ 	{ 1, { LR,LR,LL,LL }, SL22(0,1,2,3), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(5,6) , FL_RE } , 0 } ,
/*08*/ 	{ 1, { LL,LR,LL,LR }, SL22(0,2,1,3), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
/*09*/ 	{ 2, { LF,LL,LF,LL }, SL2(1,3)     , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT4(7,0,1,2) , FL_RE } , 0 } ,
	};

	static TileDefine DataPrincessDragon[] =
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*00*/ 	{ 5, { LC,LC,LF,LC }, SL2(0,3)     , SL_NONE      , 0, {SPE,SPR, 0 , 0 }, BIT2(2,6), { BIT3(3,4,5) , FL_RE } , 0 } ,
/*01*/ 	{ 5, { LC,LC,LF,LC }, SL3(0,1,3)   , SL_NONE      , 0, {SPR, 0 , 0 , 0 }, 0, { BIT4(3,4,5,6) } , 0 } ,
/*02*/ 	{ 5, { LC,LC,LF,LC }, SL3(0,1,3)   , SL_NONE      ,TMP,{SPE, 0 , 0 , 0 }, 0, { BIT4(3,4,5,6) } , 0 } ,
/*03*/ 	{ 5, { LC,LC,LF,LC }, SL3(0,1,3)   , SL_NONE      ,TDR|TCC,{0, 0 , 0,0 }, 0, { BIT4(3,4,5,6) } , 0 } ,
/*04*/ 	{ 1, { LC,LC,LF,LF }, SL2(0,1)     , SL_NONE      , 0, {SPR, 0 , 0 , 0 }, 0, { BIT4(2,3,4,5) , FL_RE } , 0 } ,
/*05*/ 	{ 1, { LC,LC,LF,LF }, SL22(0,1,2,3), SL_NONE      ,TDR,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*06*/ 	{ 1, { LC,LC,LF,LF }, SL22(0,1,2,3), SL_NONE      , 0, {SPR, 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*07*/ 	{ 1, { LC,LC,LF,LF }, SL2(2,3)     , SL_NONE      ,TVO,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
	};

	static TileDefine DataTower[] =
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*00*/ 	{ 1, { LC,LC,LC,LC }, SL3(0,1,2)   , SL_NONE      ,TTF, {SPE, 0 , 0 , 0 }, BIT2(6,7), { BIT4(5,6,7,0) } , 0 } ,
/*01*/ 	{ 1, { LC,LC,LR,LC }, SL3(0,1,3)   , SL_ALL       ,TTF, { 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*02*/ 	{ 1, { LC,LC,LF,LF }, SL22(0,1,2,3), SL_NONE      ,TTF, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*03*/ 	{ 1, { LC,LC,LF,LF }, SL2(2,3)     , SL_NONE      ,TTF, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*04*/ 	{ 1, { LC,LC,LF,LR }, SL2(0,1)     , SL3(0,1,3)   ,TTF, { 0 , 0 , 0 , 0 }, 0, { BIT4(7,0,1,2) , BIT3(3,4,5) , BIT2(6,0) } , 0 } ,
/*05*/ 	{ 1, { LF,LC,LF,LF }, SL3(0,2,3)   , SL_NONE      ,TTF, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
	};

	static TileDefine DataAbbeyAndMayor[] =
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*00*/ 	{ 0, { LA,LA,LA,LA }, SL_NONE      , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { 0 } , TILE_ABBEY } ,
	};

	static TileDefine DataTest[] =
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*04*/ 	{ 1, { LC,LC,LF,LR }, SL2(0,1)     , SL3(0,1,3)   ,TTF, { 0 , 0 , 0 , 0 }, 0, { BIT4(7,0,1,2) , BIT3(3,4,5) , BIT2(6,0) } , 0 } ,
/*00*/ 	{ 1, { LC,LC,LC,LC }, SL_NONE      , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
	};



#define EXPDATA( NAME , DATA ) { NAME , DATA , ARRAYSIZE( DATA ) } 
	ExpansionTileContent gAllExpansionTileContents[] =
	{
		
		EXPDATA( EXP_BASIC     , DataBasic ) ,
		EXPDATA( EXP_THE_RIVER , DataRiver1 ) ,
		EXPDATA( EXP_THE_PRINCESS_AND_THE_DRAGON , DataPrincessDragon ),
		EXPDATA( EXP_THE_TOWER , DataTower ),
		EXPDATA( EXP_ABBEY_AND_MAYOR , DataAbbeyAndMayor ) ,


		EXPDATA( EXP_TEST , DataTest ) ,

		{ EXP_NULL , 0 , 0 } 
	};
}
