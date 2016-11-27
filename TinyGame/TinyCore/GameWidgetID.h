#ifndef GameWidgetID_h__
#define GameWidgetID_h__

#include "GameWidget.h"

enum
{
	UI_SINGLEPLAYER = UI_NEXT_GLOBAL_ID  ,

	UI_GAME_SETTING_PANEL ,
	UI_PLAYER_LIST_PANEL  ,

	UI_RESTART_GAME ,
	UI_EXIT_GAME ,

	UI_PAUSE_GAME ,

	UI_MAIN_MENU  ,
	UI_GAME_MENU  ,
	////

	//GameSettingPanel
	UI_GAME_CHOICE ,
	UI_PLAYER_SLOT ,
	////
	UI_PANEL ,
	UI_SET_DEFULT ,

	UI_MULTIPLAYER ,
	UI_CREATE_SERVER ,
	UI_BUILD_CLIENT ,


};

#endif // GameWidgetID_h__