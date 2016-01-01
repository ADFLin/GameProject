#include "CARDefine.h"

#pragma warning( disable : 4482 ; error : 4002 )
namespace CAR
{

#define LF SideType::eField
#define LC SideType::eCity
#define LR SideType::eRoad
#define LS SideType::eRiver
#define LA SideType::eAbbey

#define SPE SideContent::ePennant
#define SIN SideContent::eInn
#define SWH SideContent::eWineHouse
#define SGH SideContent::eGrainHouse
#define SCH SideContent::eClothHouse
#define SPR SideContent::ePrincess
#define SNC SideContent::eNotSemiCircularCity

#define TCL TileContent::eCloister
#define TCA TileContent::eCathedral
#define TMP TileContent::eMagicPortal
#define TDR TileContent::eTheDragon
#define TVO TileContent::eVolcano
#define TTF TileContent::eTowerFoundation
#define TBZ TileContent::eBazaar

#define BIT2( A , B )     ( BIT(A)|BIT(B) )
#define BIT3( A , B , C ) ( BIT(A)|BIT(B)|BIT(C) )
#define BIT4( A , B , C , D ) ( BIT(A)|BIT(B)|BIT(C)|BIT(D) )

#define SL2( A , B )     { BIT2(A,B) }
#define SL3( A , B , C ) { BIT3(A,B,C) }
#define SL4( A , B , C , D ) { BIT4(A,B,C,D) } 
#define SL22( A , B , C , D ) { BIT2(A,B) , BIT2(C,D)  }
#define SL_ALL_C { 0x1f }
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

	static TileDefine DataInnCathedral[] =
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*00*/ 	{ 1, { LC,LC,LC,LC }, SL_NONE      , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, BIT4(0,2,4,6), { FL_RE } , 0 } ,
/*01*/ 	{ 2, { LC,LC,LC,LC }, SL_ALL       , SL_NONE      ,TCA,{ 0 , 0 , 0 , 0 }, 0, { 0 } , 0 } ,
/*02*/ 	{ 1, { LC,LC,LF,LC }, SL_NONE      , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*03*/ 	{ 1, { LC,LC,LF,LC }, SL2(0,3)     , SL_NONE      , 0, {SPE, 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*04*/ 	{ 1, { LC,LC,LR,LF }, SL2(0,1)     , SL3(0,1,2)   , 0, { 0 , 0 ,SIN, 0 }, 0, { BIT2(3,4) , FL_RE } , 0 } ,
/*05*/ 	{ 1, { LC,LC,LF,LR }, SL2(0,1)     , SL3(0,1,3)   , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(7,0) , FL_RE } , 0 } ,
/*06*/ 	{ 1, { LC,LC,LR,LR }, SL22(0,1,2,3), SL_NONE      , 0, {SPE, 0 , 0 ,SIN}, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*07*/ 	{ 1, { LF,LC,LF,LF }, SL2(2,3)     , SL_NONE      , 0, { 0 ,SNC, 0 , 0 }, 0, { BIT3(0,1,2) , FL_RE } , 0 } ,
/*08*/ 	{ 1, { LF,LC,LF,LR }, SL_NONE      , SL2(1,3)     , 0, { 0 , 0 , 0 , 0 }, 0, { BIT4(7,0,1,2) , FL_RE } , 0 } ,
/*09*/ 	{ 1, { LF,LC,LR,LR }, SL2(2,3)     , SL_NONE      , 0, { 0 , 0 , 0 ,SIN}, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*10*/ 	{ 1, { LR,LC,LR,LC }, SL2(1,3)     , SL22(0,1,2,3), 0, { 0 , 0 , 0 ,SPE}, 0, { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
/*11*/ 	{ 1, { LF,LF,LR,LR }, SL22(0,1,2,3), SL_NONE      , 0, { 0 , 0 , 0 ,SIN}, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*12*/ 	{ 1, { LR,LF,LR,LF }, SL_NONE      , SL3(0,2,4)   ,TCL,{ 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*13*/ 	{ 1, { LR,LF,LR,LF }, SL2(0,2)     , SL_NONE      , 0, {SIN, 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*14*/ 	{ 1, { LR,LF,LR,LR }, SL_NONE      , SL3(0,2,3)   , 0, {SIN, 0 , 0 , 0 }, 0, { BIT2(5,6) , BIT2(7,0) , FL_RE } , 0 } ,
/*15*/ 	{ 1, { LR,LR,LR,LR }, SL22(1,2,3,0), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(3,4) , BIT2(7,0) , FL_RE } , 0 } ,
/*16*/ 	{ 1, { LR,LC,LR,LC }, SL_NONE      , SL2(0,2)     , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
	};

	static TileDefine DataTraderBuilder[] =
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*00*/ 	{ 1, { LC,LC,LC,LC }, SL2(1,2)     , SL_NONE      , 0, { 0 ,SCH, 0 , 0 }, BIT3(0,2,6), { FL_RE } , 0 } ,
/*01*/ 	{ 1, { LC,LC,LC,LC }, SL22(1,2,3,0), SL_NONE      , 0, { 0 ,SWH, 0 , 0 }, BIT2(2,6), { FL_RE } , 0 } ,
/*02*/ 	{ 1, { LC,LC,LF,LC }, SL2(1,3)     , SL_NONE      , 0, { 0 , 0 ,SWH, 0 }, BIT2(0,1), { BIT4(7,0,1,2) , FL_RE } , 0 } ,
/*03*/ 	{ 1, { LC,LC,LF,LC }, SL2(1,3)     , SL_NONE      , 0, { 0 , 0 ,SCH, 0 }, BIT2(0,1), { BIT4(7,0,1,2) , FL_RE } , 0 } ,
/*04*/ 	{ 1, { LC,LC,LF,LC }, SL3(0,1,3)   , SL_NONE      , 0, {SGH, 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*05*/ 	{ 1, { LC,LC,LR,LC }, SL2(0,3)     , SL3(0,2,3)   , 0, {SGH, 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*06*/ 	{ 1, { LC,LC,LR,LC }, SL2(0,3)     , SL3(0,2,3)   , 0, {SCH, 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*07*/ 	{ 1, { LC,LC,LR,LC }, SL3(0,1,3)   , SL_ALL       , 0, {SWH, 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*08*/ 	{ 1, { LC,LC,LF,LF }, SL2(0,1)     , SL_NONE      , 0, {SWH, 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*09*/ 	{ 1, { LC,LC,LF,LF }, SL2(0,1)     , SL_NONE      , 0, {SGH, 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*10*/ 	{ 1, { LC,LC,LR,LF }, SL2(0,1)     , SL3(0,1,2)   , 0, {SGH, 0 , 0 , 0 }, 0, { BIT2(3,4) , BIT3(6,7,0) , FL_RE } , 0 } ,
/*11*/ 	{ 1, { LC,LC,LR,LF }, SL2(0,1)     , SL3(0,1,2)   , 0, {SGH, 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*12*/ 	{ 1, { LC,LC,LF,LR }, SL2(0,1)     , SL3(0,1,3)   , 0, {SCH, 0 , 0 , 0 }, 0, { BIT4(7,0,1,2) , FL_RE } , 0 } ,
/*13*/ 	{ 1, { LC,LC,LF,LR }, SL2(0,1)     , SL3(0,1,3)   , 0, {SWH, 0 , 0 , 0 }, 0, { BIT2(7,0) , BIT3(3,4,5) , FL_RE } , 0 } ,
/*14*/ 	{ 1, { LC,LC,LR,LR }, SL2(0,1)     , SL22(0,3,1,2), 0, {SWH, 0 , 0 , 0 }, 0, { BIT2(7,0) , BIT2(3,4) , FL_RE } , 0 } ,
/*15*/ 	{ 1, { LC,LC,LR,LR }, SL2(0,1)     , SL22(0,3,1,2), 0, {SCH, 0 , 0 , 0 }, 0, { BIT2(7,0) , BIT2(3,4) , BIT2(5,2) , BIT2(6,1) } , 0 } ,
/*16*/ 	{ 1, { LF,LC,LF,LC }, SL2(1,3)     , SL_NONE      , 0, { 0 ,SWH, 0 , 0 }, 0, { BIT4(7,0,1,2) , FL_RE } , 0 } ,
/*17*/ 	{ 1, { LF,LC,LR,LC }, SL2(1,3)     , SL3(1,2,3)   , 0, { 0 ,SGH, 0 , 0 }, 0, { BIT2(3,4) , BIT2(5,6) , FL_RE } , 0 } ,
/*18*/ 	{ 1, { LF,LC,LR,LC }, SL2(1,3)     , SL3(1,2,3)   , 0, { 0 ,SWH, 0 , 0 }, 0, { BIT2(3,4) , BIT2(5,6) , FL_RE } , 0 } ,
/*19*/ 	{ 1, { LF,LC,LR,LR }, SL_NONE      , SL2(1,3)     , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(3,4) , BIT2(5,6) , FL_RE } , 0 } ,
/*20*/ 	{ 1, { LR,LC,LR,LC }, SL2(1,3)     , SL22(0,1,2,3), 0, { 0 ,SWH, 0 , 0 }, 0, { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
/*21*/ 	{ 1, { LR,LC,LF,LF }, SL2(2,3)     , SL2(0,1)     , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , FL_RE } , 0 } ,
/*22*/ 	{ 1, { LR,LF,LR,LR }, SL_NONE      , SL4(0,2,3,4) ,TCL,{ 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , BIT2(7,0) , FL_RE } , 0 } ,
/*23*/ 	{ 1, { LR,LR,LR,LR }, SL22(0,2,1,3), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
	};

	static TileDefine DataRiver1[] = 
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*00*/ 	{ 1, { LF,LF,LF,LS }, SL3(0,1,2)   , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , TILE_START_TAG } ,
/*01*/ 	{ 1, { LF,LS,LF,LF }, SL3(0,2,3)   , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , TILE_END_TAG } ,
/*02*/ 	{ 1, { LC,LS,LS,LC }, SL22(0,3,1,2), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(3,4) , FL_RE } , 0 } ,
/*03*/ 	{ 1, { LC,LS,LR,LS }, SL2(1,3)     , SL2(0,3)     , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
/*04*/ 	{ 1, { LS,LC,LS,LC }, SL2(0,2)     , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , BIT4(5,6,7,0) } , 0 } ,
/*05*/ 	{ 1, { LS,LF,LF,LS }, SL22(0,3,1,2), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(7,0) , FL_RE } , 0 } ,
/*06*/ 	{ 1, { LS,LF,LS,LR }, SL2(0,2)     , SL2(3,4)     ,TCL,{ 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , BIT2(7,0) , FL_RE } , 0 } ,
/*07*/ 	{ 1, { LR,LR,LS,LS }, SL22(0,1,2,3), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(5,6) , FL_RE } , 0 } ,
/*08*/ 	{ 1, { LS,LR,LS,LR }, SL22(0,2,1,3), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
/*09*/ 	{ 2, { LF,LS,LF,LS }, SL2(1,3)     , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT4(7,0,1,2) , FL_RE } , 0 } ,
	};

	static TileDefine DataKingRobber[] =
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*00*/ 	{ 1, { LC,LC,LC,LC }, SL22(0,2,1,3), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, BIT4(0,2,3,6), { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
/*01*/ 	{ 1, { LC,LC,LR,LR }, SL2(0,1)     , SL22(0,3,1,2), 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(7,0) , BIT2(3,4) , BIT2(5,2) , BIT2(6,1) } , 0 } ,
/*02*/ 	{ 1, { LF,LC,LF,LF }, SL3(0,2,3)   , SL_NONE      ,TCL,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*03*/ 	{ 1, { LF,LC,LR,LF }, SL2(0,3)     , SL2(1,2)     , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(3,4) , FL_RE } , 0 } ,
/*04*/ 	{ 1, { LR,LC,LR,LR }, SL2(0,3)     , SL2(1,2)     , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(3,4) , BIT2(7,0) , FL_RE } , 0 } ,
	};

	static TileDefine DataPrincessDragon[] =
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*00*/ 	{ 1, { LC,LC,LF,LC }, SL2(0,3)     , SL_NONE      , 0, {SPE,SPR|SNC,0,0}, BIT2(2,6), { BIT3(3,4,5) , FL_RE } , 0 } ,
/*01*/ 	{ 1, { LC,LC,LF,LC }, SL3(0,1,3)   , SL_NONE      , 0, {SPR, 0 , 0 , 0 }, 0, { BIT4(3,4,5,6) } , 0 } ,
/*02*/ 	{ 1, { LC,LC,LF,LC }, SL3(0,1,3)   , SL_NONE      ,TMP,{SPE, 0 , 0 , 0 }, 0, { BIT4(3,4,5,6) } , 0 } ,
/*03*/ 	{ 1, { LC,LC,LF,LC }, SL3(0,1,3)   , SL_NONE      ,TDR|TCL,{0,0, 0 , 0 }, 0, { BIT4(3,4,5,6) } , 0 } ,
/*04*/ 	{ 1, { LC,LC,LF,LF }, SL2(0,1)     , SL_NONE      , 0, {SPR, 0 , 0 , 0 }, 0, { BIT4(2,3,4,5) , FL_RE } , 0 } ,
/*05*/ 	{ 1, { LC,LC,LF,LF }, SL22(0,1,2,3), SL_NONE      ,TDR,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*06*/ 	{ 1, { LC,LC,LF,LF }, SL22(0,1,2,3), SL_NONE      , 0, {SPR, 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*07*/ 	{ 1, { LC,LC,LF,LF }, SL2(2,3)     , SL_NONE      ,TVO,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*08*/ 	{ 1, { LC,LC,LR,LR }, SL22(0,1,2,3), SL_NONE      , 0, {SPR, 0 , 0 , 0 }, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*09*/ 	{ 1, { LC,LC,LR,LR }, SL22(0,1,2,3), SL_NONE      ,TMP,{ 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*10*/ 	{ 1, { LF,LC,LF,LC }, SL2(1,3)     , SL_NONE      ,TDR,{ 0 , 0 , 0 ,SPE}, 0, { BIT4(7,0,1,2) , FL_RE } , 0 } ,
/*11*/ 	{ 1, { LF,LC,LF,LF }, SL3(0,2,3)   , SL_NONE      ,TDR,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*12*/ 	{ 1, { LF,LC,LF,LF }, SL3(0,2,3)   , SL_NONE      ,TVO,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*13*/ 	{ 1, { LF,LC,LR,LR }, SL2(2,3)     , SL_NONE      ,TDR,{ 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*14*/ 	{ 1, { LF,LC,LR,LR }, SL2(2,3)     , SL_NONE      ,TMP,{ 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*15*/ 	{ 1, { LR,LC,LR,LC }, SL22(0,2,1,3), SL_NONE      ,TDR,{ 0 , 0 , 0 , 0 }, 0, { BIT4(7,0,1,2) , FL_RE } , 0 } ,
/*16*/ 	{ 1, { LR,LC,LF,LR }, SL2(0,3)     , SL_NONE      ,TDR,{ 0 , 0 , 0 , 0 }, 0, { BIT2(0,7) , FL_RE } , 0 } ,
/*17*/ 	{ 1, { LR,LC,LF,LR }, SL2(0,3)     , SL_NONE      ,TMP,{ 0 , 0 , 0 , 0 }, 0, { BIT2(0,7) , FL_RE } , 0 } ,
/*18*/ 	{ 1, { LR,LC,LR,LR }, SL_NONE      , SL3(0,2,3)   , 0, { 0 ,SPR, 0 , 0 }, 0, { BIT2(5,6) , BIT2(7,0) , FL_RE } , 0 } ,
/*19*/ 	{ 1, { LF,LF,LF,LF }, SL_ALL       , SL_NONE      ,TVO,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*20*/ 	{ 1, { LF,LF,LR,LF }, SL3(0,1,3)   , SL_NONE      ,TVO,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*21*/ 	{ 2, { LF,LF,LR,LR }, SL22(0,1,2,3), SL_NONE      ,TDR,{ 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*22*/ 	{ 1, { LF,LF,LR,LR }, SL22(0,1,2,3), SL_NONE      ,TVO,{ 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*23*/ 	{ 1, { LR,LF,LR,LF }, SL2(0,2)     , SL_NONE      ,TDR,{ 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*24*/ 	{ 1, { LR,LF,LR,LF }, SL2(0,2)     , SL_NONE      ,TVO,{ 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*25*/ 	{ 1, { LR,LF,LR,LR }, SL_NONE      , SL4(0,2,3,4) ,TDR|TCL,{0,0, 0 , 0 }, 0, { BIT2(5,6) , BIT2(7,0) , FL_RE } , 0 } ,
/*26*/ 	{ 1, { LR,LF,LR,LR }, SL_NONE      , SL3(0,2,3)   ,TDR,{ 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , BIT2(7,0) , FL_RE } , 0 } ,
/*27*/ 	{ 1, { LR,LF,LR,LR }, SL_NONE      , SL3(0,2,3)   ,TMP,{ 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , BIT2(7,0) , FL_RE } , 0 } ,
/*28*/ 	{ 1, { LR,LR,LR,LR }, SL22(1,2,3,0), SL_NONE      ,TMP,{ 0 , 0 , 0 , 0 }, 0, { BIT2(3,4) , BIT2(7,0) , FL_RE } , 0 } ,
	};

	static TileDefine DataTower[] =
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*00*/ 	{ 1, { LC,LC,LC,LC }, SL3(0,1,2)   , SL_NONE      ,TTF,{SPE, 0 , 0 , 0 }, BIT2(6,7), { BIT4(5,6,7,0) } , 0 } ,
/*01*/ 	{ 1, { LC,LC,LR,LC }, SL3(0,1,3)   , SL_ALL       ,TTF,{ 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*02*/ 	{ 1, { LC,LC,LF,LF }, SL22(0,1,2,3), SL_NONE      ,TTF,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*03*/ 	{ 1, { LC,LC,LF,LF }, SL2(2,3)     , SL_NONE      ,TTF,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*04*/ 	{ 1, { LC,LC,LF,LR }, SL2(0,1)     , SL3(0,1,3)   ,TTF,{ 0 , 0 , 0 , 0 }, 0, { BIT4(7,0,1,2) , BIT3(3,4,5) , BIT2(6,0) } , 0 } ,
/*05*/ 	{ 2, { LF,LC,LF,LF }, SL3(0,2,3)   , SL_NONE      ,TTF,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*06*/ 	{ 1, { LF,LC,LF,LR }, SL_NONE      , SL2(1,3)     ,TTF,{ 0 , 0 , 0 , 0 }, 0, { BIT4(7,0,1,2) , FL_RE } , 0 } ,
/*07*/ 	{ 1, { LF,LC,LR,LR }, SL2(2,3)     , SL_NONE      ,TTF,{ 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*08*/ 	{ 1, { LR,LC,LR,LC }, SL22(0,2,1,3), SL_NONE      ,TTF,{ 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
/*09*/ 	{ 1, { LR,LC,LR,LC }, SL2(0,2)     , SL_NONE      ,TTF,{ 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*10*/ 	{ 1, { LR,LC,LR,LF }, SL2(0,2)     , SL_NONE      ,TTF,{ 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(3,4) , FL_RE } , 0 } ,
/*11*/ 	{ 1, { LF,LF,LF,LF }, SL_ALL       , SL_NONE      ,TTF,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*12*/ 	{ 1, { LF,LF,LF,LF }, SL_ALL       , SL_NONE      ,TTF|TCL,{0,0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*13*/ 	{ 1, { LF,LF,LR,LR }, SL2(0,1)     , SL2(2,3)     ,TTF,{ 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*14*/ 	{ 1, { LR,LR,LR,LF }, SL_NONE      , SL3(0,1,2)   ,TTF,{ 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(3,4) , FL_RE } , 0 } ,
/*15*/ 	{ 1, { LR,LR,LR,LR }, SL22(1,2,3,0), SL_NONE      ,TTF,{ 0 , 0 , 0 , 0 }, 0, { BIT2(3,4) , BIT2(7,0) , FL_RE } , 0 } ,
/*16*/ 	{ 1, { LR,LR,LR,LR }, SL_NONE      , SL4(0,1,2,3) ,TTF,{ 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
	};

	static TileDefine DataAbbeyMayor[] =
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*00*/ 	{ 0, { LA,LA,LA,LA }, SL_NONE      , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { 0 } , TILE_ABBEY } ,
/*01*/ 	{ 1, { LC,LC,LC,LC }, SL_ALL       , SL_NONE      , 0, { 0 ,SPE,SPE, 0 }, 0, { FL_RE } , 0 } ,
/*02*/ 	{ 1, { LC,LC,LC,LC }, SL22(0,2,1,3), SL_NONE      , 0, { 0 ,SPE, 0 , 0 }, BIT4(2,3,6,7), { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*03*/ 	{ 1, { LC,LC,LF,LC }, SL2(1,3)     , SL_NONE      , 0, {SPE|SNC,0,0, 0 }, BIT2(2,7), { BIT4(7,0,1,2) , BIT4(3,4,5,6)|BIT2(0,1) } , 0 } ,
/*04*/ 	{ 1, { LC,LC,LR,LR }, SL_NONE      , SL22(0,3,1,2), 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(3,4) , BIT2(7,0) , FL_RE } , 0 } ,
/*05*/ 	{ 1, { LF,LC,LF,LF }, SL_NONE      , SL_NONE      , 0, { 0 ,SPE, 0 , 0 }, 0, { BIT3(0,1,2) , BIT3(3,4,5) , BIT2(6,7)|BIT2(2,3) } , 0 } ,
/*06*/ 	{ 1, { LF,LC,LR,LF }, SL2(0,3)     , SL2(1,2)     , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(3,4) , FL_RE } , 0 } ,
/*07*/ 	{ 1, { LF,LC,LR,LR }, SL2(2,3)     , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(3,4) , BIT2(5,6) , FL_RE } , 0 } ,
/*08*/ 	{ 1, { LR,LC,LR,LC }, SL22(0,2,1,3), SL_NONE      , 0, { 0 ,SPE, 0 , 0 }, 0, { BIT2(7,0) , BIT2(1,2) , FL_RE } , 0 } ,
/*09*/ 	{ 1, { LR,LC,LF,LR }, SL2(0,3)     , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(7,0) , BIT2(1,2) , FL_RE } , 0 } ,
/*10*/ 	{ 1, { LF,LF,LR,LF }, SL3(0,1,3)   , SL2(1,2)     , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*11*/ 	{ 1, { LR,LF,LR,LR }, SL3(0,2,3)   , SL_NONE      , 0,{ 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , BIT2(7,0) , FL_RE } , 0 } ,
/*12*/ 	{ 1, { LR,LR,LR,LR }, SL_NONE      , SL_ALL_C     ,TCL,{ 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
	};

	static TileDefine DataRiver2[] =
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*00*/ 	{ 1, { LF,LF,LS,LF }, SL3(0,1,3)   , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , TILE_START_TAG } ,
/*01*/ 	{ 1, { LS,LF,LS,LS }, SL3(0,2,3)   , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , BIT2(7,0) , FL_RE } , TILE_FRIST_PLAY_TAG } ,
/*02*/ 	{ 1, { LF,LC,LF,LS }, SL_NONE      , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT4(7,0,1,2) , FL_RE } , 0 } ,
/*03*/ 	{ 1, { LF,LF,LS,LF }, SL3(0,1,3)   , SL_NONE      ,TVO,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , TILE_END_TAG } ,
/*04*/ 	{ 1, { LC,LC,LS,LS }, SL22(0,1,2,3), SL_NONE      , 0, {SPE, 0 , 0 , 0 }, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*05*/ 	{ 1, { LS,LC,LS,LR }, SL2(0,2)     , SL2(1,3)     , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
/*06*/ 	{ 1, { LF,LF,LS,LS }, SL22(0,1,2,3), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*07*/ 	{ 1, { LS,LC,LS,LC }, SL22(0,2,1,3), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*08*/ 	{ 1, { LF,LF,LS,LS }, SL22(0,1,2,3), SL2(3,4)     , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(5,6) , FL_RE } , 0 } ,
/*09*/ 	{ 1, { LS,LF,LS,LF }, SL2(0,2)     , SL_NONE      ,TCL,{ 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*10*/ 	{ 1, { LR,LR,LS,LS }, SL22(0,1,2,3), SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(1,2) , BIT2(5,6) , FL_RE } , 0 } ,
/*11*/ 	{ 1, { LS,LR,LS,LR }, SL22(0,2,1,3), SL_NONE      , 0, { 0 , 0 , 0 ,SIN}, 0, { BIT2(1,2) , BIT2(3,4) , BIT2(5,6) , BIT2(7,0) } , 0 } ,
	};
	static TileDefine DataBrigeCastleBazaar[] =
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*00*/ 	{ 2, { LC,LC,LC,LC }, SL_ALL       , SL_NONE      ,TBZ,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*01*/ 	{ 1, { LC,LC,LR,LC }, SL_NONE      , SL2(2,3)     , 0, { 0 , 0 , 0 , 0 }, 0, { BIT2(5,6), FL_RE } , 0 } ,
/*02*/ 	{ 1, { LC,LF,LC,LF }, SL2(0,2)     , SL_NONE      ,TBZ,{ 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*03*/ 	{ 1, { LC,LF,LC,LF }, SL2(0,2)     , SL_NONE      ,TCL,{ 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
/*04*/ 	{ 1, { LF,LC,LF,LR }, SL2(0,2)     , SL_NONE      ,TBZ,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*05*/ 	{ 1, { LF,LC,LF,LR }, SL_NONE      , SL2(1,3)     , 0, { 0 ,SNC, 0 , 0 }, 0, { BIT3(0,1,2) , BIT4(3,4,5,6) , BIT2(7,3) } , 0 } ,
/*06*/ 	{ 1, { LF,LF,LF,LF }, SL_ALL       , SL_NONE      ,TBZ,{ 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*07*/ 	{ 1, { LF,LF,LF,LR }, SL3(0,1,2)   , SL_NONE      ,TBZ,{ 0 , 0 , 0 ,SIN}, 0, { FL_RE } , 0 } ,
/*08*/ 	{ 1, { LF,LR,LF,LR }, SL_NONE      , SL2(1,3)     ,TBZ,{ 0 , 0 , 0 , 0 }, 0, { BIT4(7,0,1,2) , FL_RE } , 0 } ,
/*09*/ 	{ 1, { LF,LR,LF,LR }, SL_NONE      , SL2(1,3)     ,TBZ,{ 0 , 0 , 0 ,SIN}, 0, { BIT4(7,0,1,2) , FL_RE } , 0 } ,
/*10*/ 	{ 1, { LR,LF,LR,LF }, SL2(0,2)     , SL_NONE      ,TCL,{ 0 , 0 , 0 , 0 }, 0, { BIT4(1,2,3,4) , FL_RE } , 0 } ,
	};

	static TileDefine DataTest[] =
	{
// numPiece     linkType      sideLink       roadLink   content   sidecontent centerFarmMask farmLink tag
/*04*/ 	{ 1, { LC,LC,LF,LR }, SL2(0,1)     , SL3(0,1,3)   , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
/*00*/ 	{ 1, { LC,LC,LR,LC }, SL_NONE      , SL_NONE      , 0, { 0 , 0 , 0 , 0 }, 0, { FL_RE } , 0 } ,
	};

	ExpansionTileContent gAllExpansionTileContents[] =
	{
#define EXPDATA( NAME , DATA ) { NAME , DATA , ARRAY_SIZE( DATA ) } ,
		EXPDATA( EXP_BASIC , DataBasic )
		EXPDATA( EXP_INNS_AND_CATHEDRALS , DataInnCathedral )
		EXPDATA( EXP_TRADERS_AND_BUILDERS , DataTraderBuilder )
		EXPDATA( EXP_KING_AND_ROBBER , DataKingRobber )
		EXPDATA( EXP_THE_RIVER , DataRiver1 )
		EXPDATA( EXP_THE_PRINCESS_AND_THE_DRAGON , DataPrincessDragon )
		EXPDATA( EXP_THE_TOWER , DataTower )
		EXPDATA( EXP_ABBEY_AND_MAYOR , DataAbbeyMayor )
		EXPDATA( EXP_THE_RIVER_II , DataRiver2 ) 
		EXPDATA( EXP_BRIDGES_CASTLES_AND_BAZAARS , DataBrigeCastleBazaar )
		EXPDATA( EXP_TEST , DataTest ) 
#undef EXPDATA
		{ EXP_NULL , 0 , 0 } 
	};

}
