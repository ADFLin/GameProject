#ifndef rcGUIData_h__
#define rcGUIData_h__

enum rcUIGroup
{
	UG_BUILD_BTN ,
	UG_TEXT_BTN ,
	UG_PANEL ,
};
enum
{
	/////////////////
	UI_BLD_BTN_START ,
	UI_BLD_HOUSE_BTN = UI_BLD_BTN_START,
	UI_BLD_WATER_BTN ,
	UI_BLD_CLEAR_BTN ,
	UI_BLD_ROAD_BTN ,
	UI_BLD_GOV_BTN  ,
	UI_BLD_ENT_BTN  ,
	UI_BLD_EDU_BTN  ,
	UI_BLD_BTN_END = UI_BLD_EDU_BTN,
	/////////////////
	UI_CTRL_PANEL   ,
	UI_SIMPLE_CTRL_PANEL ,

	UI_TOTAL_NUMBER ,
};


#endif // rcGUIData_h__