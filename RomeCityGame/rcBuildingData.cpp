#include "rcPCH.h"
#include "rcGameData.h"
#include "rcMapLayerTag.h"
#include "rcBuilding.h"

#define BEGIN_BUILDING()\
	rcBuildingInfo g_buildingInfo[] = {

#define BUILDING( id ) { id , BGC_UNDEFINE , 1 , 0 , 0 },
#define BUILDING2( id , CLASS ) { id , CLASS , 1 , 0 , 0 },
#define BUILDING3( id , CLASS , numP ) { id , CLASS , numP , 0 , 0 },

#define END_BUILDING() };

BEGIN_BUILDING()

	BUILDING( 0 )
	BUILDING( 0 )
	BUILDING( 0 )
	BUILDING( 0 )

	BUILDING(        BT_ROAD )
	BUILDING(        BT_WALL )
	BUILDING( BT_SIMON_STUFF )
	BUILDING(    BT_AQUADUCT )
	BUILDING(         BT_DIG )

	BUILDING2( BT_HOUSE_SIGN , BGC_HOUSE )
	BUILDING2(    BT_HOUSE01 , BGC_HOUSE )
	BUILDING2(    BT_HOUSE02 , BGC_HOUSE )
	BUILDING2(    BT_HOUSE03 , BGC_HOUSE )
	BUILDING2(    BT_HOUSE04 , BGC_HOUSE )
	BUILDING2( BT_HOUSE05, BGC_HOUSE )
	BUILDING2( BT_HOUSE06, BGC_HOUSE )
	BUILDING2( BT_HOUSE07, BGC_HOUSE )
	BUILDING2( HOUSE08, BGC_HOUSE )
	BUILDING2( HOUSE09, BGC_HOUSE )
	BUILDING2( HOUSE10, BGC_HOUSE )
	BUILDING2( HOUSE11, BGC_HOUSE )
	BUILDING2( HOUSE12, BGC_HOUSE )
	BUILDING2( HOUSE13, BGC_HOUSE )
	BUILDING2( HOUSE14, BGC_HOUSE )
	BUILDING2( HOUSE15, BGC_HOUSE )
	BUILDING2( HOUSE16, BGC_HOUSE )
	BUILDING2( HOUSE17, BGC_HOUSE )
	BUILDING2( HOUSE18, BGC_HOUSE )
	BUILDING2( HOUSE19, BGC_HOUSE )
	BUILDING2( HOUSE20, BGC_HOUSE )

	BUILDING2(    BT_AMPITHEATRE, BGC_SERVICE )
	BUILDING2(        BT_THEATRE, BGC_SERVICE )
	BUILDING2(        BT_HIPPODROME, BGC_SERVICE )
	BUILDING2(         BT_COLLOSEUM, BGC_SERVICE )
	BUILDING2(     BT_GLADIATOR_PIT, BGC_SERVICE )
	BUILDING2(          BT_LION_PIT, BGC_SERVICE )
	BUILDING2(     BT_ARTIST_COLONY, BGC_SERVICE )
	BUILDING2( BT_CHATIOTEER_SCHOOL, BGC_SERVICE )

	BUILDING(   PLAZA )
	BUILDING( BT_GARDENS )
	BUILDING(    FORT )


	BUILDING2( BT_PREFECTURE , BGC_SERVICE )

	BUILDING2( BT_MARKET , BGC_MARKET )
	BUILDING2( BT_GRANERY , BGC_STORAGE )
	BUILDING2( BT_WAREHOUSE , BGC_STORAGE )

	BUILDING2( BT_RESEVIOR , BGC_BASIC )
	BUILDING2( BT_FOUNTAIN , BGC_BASIC )
	BUILDING2( BT_WATER_WELL , BGC_BASIC )

    BUILDING2( BT_FARM_WHEAT     , BGC_FARM )
	BUILDING2( BT_FARM_VEGETABLE , BGC_FARM )
	BUILDING2( BT_FARM_FIG       , BGC_FARM )
	BUILDING2( BT_FARM_OLIVE     , BGC_FARM )
	BUILDING2( BT_FARM_VINEYARD  , BGC_FARM )
	BUILDING2( BT_FARM_MEAT      , BGC_FARM )

	BUILDING2( BT_MINE     , BGC_ROW_FACTORY )
	BUILDING2( BT_CALY_PIT , BGC_ROW_FACTORY )

	BUILDING2( BT_WS_WINE      , BGC_WORKSHOP )
	BUILDING2( BT_WS_OIL       , BGC_WORKSHOP )
	BUILDING2( BT_WS_WEAPONS   , BGC_WORKSHOP )
	BUILDING2( BT_WS_FURNITURE , BGC_WORKSHOP )
	BUILDING2( BT_WS_POTTERY   , BGC_WORKSHOP )

END_BUILDING()

int const g_BuildingNum = ARRAY_SIZE( g_buildingInfo );


#undef BEGIN_BUILDING
#undef BUILDING
#undef END_BUILDING


enum
{
	ANIM_NO_ANIM       = RC_ERROR_ANIM_ID,
	ANIM_NO_BACKGROUND = RC_ANIM_NO_BACKGROUND ,

	ANIM_AMPITHEATRE ,
	ANIM_THEARE ,
	ANIM_COLLOSEUM ,
	ANIM_ARTIST_COLONY ,
	ANIM_WAREHOUSE  ,

	ANIM_NUM ,
};

ImageBGInfo      g_AnimImageInfo[ ANIM_NUM ];

void rcDataManager::initAnimImageInfo() 
{
#define BEGIN_ANIM() 

#define ANIM( id ,  bgX , bgY )\
	g_AnimImageInfo[ id ] = ImageBGInfo( bgX , bgY );

#define END_ANIM()

	BEGIN_ANIM()

		ANIM(    ANIM_NO_BACKGROUND,    0,    0 )
		ANIM(      ANIM_AMPITHEATRE ,  -48, -110)
		ANIM(           ANIM_THEARE,     0,    0)
		ANIM(        ANIM_COLLOSEUM,   -81, -181)
		ANIM(        ANIM_WAREHOUSE,     0,    0)

	END_ANIM()

#undef BEGIN_ANIM
#undef ANIM
#undef END_ANIM
}



#define BEGIN_BUILDING_MODEL_MAP()                \
	void rcDataManager::loadBuildingTexture()  {  \

#define BEGIN_BUILDING_MODEL( dataID ) {       \
	unsigned  tagBuilding = dataID;    \

#define MODEL( index , sx , sy , animID )        \
	loadBuilding( tagBuilding , index , animID , sx , sy );

#define END_BUILDING_MODEL() }

#define END_BUILDING_MODEL_MAP() }


BEGIN_BUILDING_MODEL_MAP()


	BEGIN_BUILDING_MODEL( BT_HOUSE_SIGN )
	MODEL( 2822 , 1 , 1, ANIM_NO_ANIM )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_HOUSE01 )
	MODEL( 2778 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2779 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2782 , 2 , 2 , ANIM_NO_ANIM )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_HOUSE02 )
	MODEL( 2780 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2781 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2783 , 2 , 2 , ANIM_NO_ANIM )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_HOUSE03 )
	MODEL( 2784 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2785 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2788 , 2 , 2 , ANIM_NO_ANIM )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_HOUSE04 )
	MODEL( 2786 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2787 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2789 , 2 , 2 , ANIM_NO_ANIM )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_HOUSE05 )
	MODEL( 2790 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2791 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2794 , 2 , 2 , ANIM_NO_ANIM )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_HOUSE06 )
	MODEL( 2792 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2793 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2795 , 2 , 2 , ANIM_NO_ANIM )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_HOUSE07 )
	MODEL( 2796 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2797 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2798 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2799 , 1 , 1 , ANIM_NO_ANIM )
	MODEL( 2800 , 2 , 2 , ANIM_NO_ANIM )
	MODEL( 2801 , 2 , 2 , ANIM_NO_ANIM )
	END_BUILDING_MODEL()


	//Entertainment

	BEGIN_BUILDING_MODEL( BT_AMPITHEATRE )
	MODEL( 3038 , 3 , 3 , ANIM_AMPITHEATRE )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_THEATRE )
	MODEL( 3050 , 2 , 2 , ANIM_THEARE )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_COLLOSEUM )
	MODEL( 3073 , 5 , 5 , ANIM_COLLOSEUM )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_GLADIATOR_PIT )
	MODEL( 3088 , 3 , 3 , ANIM_NO_ANIM )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_LION_PIT )
	MODEL( 3099 , 3 , 3 , ANIM_NO_ANIM )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_ARTIST_COLONY )
	MODEL( 3118 , 3 , 3 , ANIM_ARTIST_COLONY )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_CHATIOTEER_SCHOOL )
	MODEL( 3128 , 3 , 3 , ANIM_NO_ANIM )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_PREFECTURE )
	MODEL( 3164 , 1 , 1 , ANIM_NO_BACKGROUND )
	END_BUILDING_MODEL()

	/////////////////////////////

	BEGIN_BUILDING_MODEL( BT_MARKET )
	MODEL( 2871 , 2 , 2 , ANIM_NO_BACKGROUND )
	MODEL( 3028 , 2 , 2 , ANIM_NO_BACKGROUND )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_GRANERY )
	MODEL( 3010 , 3 , 3 , ANIM_NO_ANIM )
	END_BUILDING_MODEL()


	BEGIN_BUILDING_MODEL( BT_WAREHOUSE )
	MODEL( 3318 , 3 , 3 , ANIM_WAREHOUSE )
	END_BUILDING_MODEL()

	//////////////////////////
	BEGIN_BUILDING_MODEL( BT_RESEVIOR )
	MODEL( 2862 , 3 , 3 , ANIM_NO_BACKGROUND )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_FOUNTAIN )
	MODEL( 2838 , 1 , 1 , ANIM_NO_BACKGROUND )
	MODEL( 2830 , 1 , 1 , ANIM_NO_BACKGROUND )
	MODEL( 2854 , 1 , 1 , ANIM_NO_BACKGROUND )
	END_BUILDING_MODEL()


	BEGIN_BUILDING_MODEL( BT_WATER_WELL )
	MODEL( 2829 , 1 , 1 , ANIM_NO_ANIM )
	END_BUILDING_MODEL()
	/////////////////////////////

	BEGIN_BUILDING_MODEL( BT_FARM_WHEAT )
	MODEL( 2882 , 3 , 3 , ANIM_NO_ANIM )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_FARM_VEGETABLE )
	MODEL( -1 , 3 , 3 , ANIM_NO_ANIM  )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_FARM_FIG  )
	MODEL( -1 , 3 , 3 , ANIM_NO_ANIM  )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_FARM_OLIVE )
	MODEL( -1 , 3 , 3 , ANIM_NO_ANIM  )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_FARM_VINEYARD )
	MODEL( -1 , 3 , 3 , ANIM_NO_ANIM  )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_FARM_MEAT )
	MODEL( -1 , 3 , 3 , ANIM_NO_ANIM  )
	END_BUILDING_MODEL()

	//////////////////////////////////////

	BEGIN_BUILDING_MODEL( BT_MINE )
	MODEL( 2924 , 2 , 2 , ANIM_NO_BACKGROUND  )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_CALY_PIT )
	MODEL( 2931 , 2, 2 , ANIM_NO_BACKGROUND  )
	END_BUILDING_MODEL()

	/////////////////////////////
	BEGIN_BUILDING_MODEL( BT_WS_WINE )
	MODEL( 2956 , 2, 2 , ANIM_NO_BACKGROUND )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_WS_OIL )
	MODEL( 2969 , 2, 2 , ANIM_NO_BACKGROUND )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_WS_WEAPONS )
	MODEL( 2978 , 2, 2 , ANIM_NO_BACKGROUND )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_WS_POTTERY )
	MODEL( 2987 , 2, 2 , ANIM_NO_BACKGROUND )
	END_BUILDING_MODEL()

	BEGIN_BUILDING_MODEL( BT_WS_POTTERY )
	MODEL( 3002 , 2, 2 , ANIM_NO_BACKGROUND )
	END_BUILDING_MODEL()
	///////////////////////////////////////
END_BUILDING_MODEL_MAP()


#undef BEGIN_BUILDING_MODEL_MAP
#undef BEGIN_BUILDING_MODEL
#undef MODEL
#undef END_BUILDING_MODEL
#undef END_BUILDING_MODEL_MAP



#define BEGIN_HOUSE_MAP()

#define HOUSE( bTag , numNextLV , requestID )\


#define END_HOUSE_MAP()


BEGIN_HOUSE_MAP()

	HOUSE( BT_HOUSE_SIGN , 2 ,LR_00_ROAD_ACCESS )
	HOUSE( BT_HOUSE01    , 2 ,LR_01_PEOPLE )
	HOUSE( BT_HOUSE02    , 2 ,LR_02_WATER_SOURCE )
	HOUSE( BT_HOUSE03    , 2 ,LR_03_ONE_FOOD )
	HOUSE( BT_HOUSE04    , 2 ,LR_04_ONE_GOD_ACCESS )
	HOUSE( BT_HOUSE05    , 2 ,LR_05_WATER_FOUNTAIN )
	HOUSE( BT_HOUSE06    , 2 ,LR_06_1ST_ENTERTAINMENT )
	HOUSE( BT_HOUSE07    , 2 ,LR_07_BASIC_EDUCATION )
	HOUSE( HOUSE08, 2 ,LR_08_BATH_HOUSE_AND_POTTERY )
	HOUSE( HOUSE09, 2 ,LR_09_2ND_ENTERTAINMENT )
	HOUSE( HOUSE10, 2 ,LR_10_DOCTOR_AND_FURNITURE )
	HOUSE( HOUSE11, 2 ,LR_11_SCHOOL_AND_LIBRARY )
	HOUSE( HOUSE12, 2 ,LR_12_3RD_ENTERTAINMENT )
	HOUSE( HOUSE13, 2 ,LR_13_WINE_AND_TWO_GODS )
	HOUSE( HOUSE14, 2 ,LR_14_BATH_HOUSE_AND_DUCTOR_AND_HOSPITAL )
	HOUSE( HOUSE15, 2 ,LR_15_ACADEMY )
	HOUSE( HOUSE16, 2 ,LR_16_THREE_FOOD_TYPE_AND_THREE_GODS )
	HOUSE( HOUSE17, 2 ,LR_17_2ND_WINE )
	HOUSE( HOUSE18, 2 ,LR_18_FOUR_GODS )
	HOUSE( HOUSE19, 2 ,LR_19_4TH_EENTERTAINMENT )
	HOUSE( HOUSE20, 2 ,LR_20_HIGHT_DESIRABILITY )

END_HOUSE_MAP()