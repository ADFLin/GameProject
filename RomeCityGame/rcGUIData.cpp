#include "rcPCH.h"
#include "rcGUIData.h"

#include "rcGUI.h"
#include "rcGameData.h"

extern rcWidgetInfo g_WidgetInfo[];

#define BEGIN_UI_MAP()\
	rcWidgetInfo g_WidgetInfo[] = {\


#define DEF_UI( ID , Class , IdxImage ,Pos , Size )\
	{  ID , Class , IdxImage , Pos , Size  },

#define DEF_BUILD_BTN( ID , IdxImage , Pos  )\
	DEF_UI( ID , UG_BUILD_BTN , IdxImage , Pos , Vec2i(-1,-1) )

#define DEF_PANEL( ID , IdxImage , Pos )\
	DEF_UI( ID , UG_PANEL , IdxImage , Pos , Vec2i(-1,-1) )


#define END_UI_MAP() };



BEGIN_UI_MAP()
	DEF_BUILD_BTN( UI_BLD_HOUSE_BTN , 992  , Vec2i(10,10) )
	DEF_BUILD_BTN( UI_BLD_WATER_BTN , 996  , Vec2i(10,10) )
	DEF_BUILD_BTN( UI_BLD_CLEAR_BTN , 1000 , Vec2i(10,40) )
	DEF_BUILD_BTN( UI_BLD_ROAD_BTN  , 1004 , Vec2i(10,70) )
	DEF_BUILD_BTN( UI_BLD_GOV_BTN   , 1008 , Vec2i(10,10) )
	DEF_BUILD_BTN( UI_BLD_ENT_BTN   , 1012 , Vec2i(10,10) )
	DEF_BUILD_BTN( UI_BLD_EDU_BTN   , 1016 , Vec2i(10,10) )

	DEF_PANEL( UI_CTRL_PANEL , 886 , Vec2i(0,0) )
END_UI_MAP()




void rcDataManager::loadUI()
{
	for( int i = 0 ; i < ARRAY_SIZE( g_WidgetInfo ) ; ++i )
	{
		assert( i == g_WidgetInfo[i].id );
		switch( g_WidgetInfo[i].group )
		{
		case UG_BUILD_BTN:
			g_WidgetInfo[i].texBase = createTexture( g_WidgetInfo[i].texBase , 4 );
			break;
		case UG_PANEL:
			g_WidgetInfo[i].texBase = createTexture( g_WidgetInfo[i].texBase );
			break;
		}
	}
}

rcWidgetInfo const& rcDataManager::getWidgetInfo( int idUI )
{
	return g_WidgetInfo[idUI];
}