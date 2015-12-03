#ifndef MainMenuStage_h__
#define MainMenuStage_h__

#include "GameMenuStage.h"

class MainMenuStage : public GameMenuStage
{
	typedef GameMenuStage BaseClass;
public:

	static const int MAX_NUM_GROUP = 100;

	enum
	{
		UI_GAME_BUTTON = BaseClass::NEXT_UI_ID ,
		UI_VIEW_REPLAY ,
		UI_GAME_OPTION ,

		UI_MAIN_GROUP ,
		UI_TEST_GROUP ,
		UI_GRAPHIC_TEST_GROUP ,
		UI_GAME_DEV_GROUP ,
		UI_GAME_DEV2_GROUP ,
		UI_GAME_DEV3_GROUP ,
		UI_CARD_GAME_DEV_GROUP ,

		UI_NET_TEST_SV  ,
		UI_NET_TEST_CL  ,

		UI_BIG2_TEST ,
		UI_HOLDEM_TEST ,
		UI_FREECELL_TEST ,

		UI_GROUP_INDEX ,
		UI_SINGLE_DEV_INDEX    = UI_GROUP_INDEX ,
		UI_GAME_DEV_INDEX      = UI_SINGLE_DEV_INDEX + MAX_NUM_GROUP,
		UI_GAME_DEV2_INDEX     = UI_GAME_DEV_INDEX + MAX_NUM_GROUP,
		UI_GAME_DEV3_INDEX     = UI_GAME_DEV2_INDEX + MAX_NUM_GROUP,
		UI_TEST_INDEX          = UI_GAME_DEV3_INDEX + MAX_NUM_GROUP ,
		UI_GRAPHIC_TEST_INDEX  = UI_TEST_INDEX + MAX_NUM_GROUP ,
		
		NEXT_UI_ID             = UI_GRAPHIC_TEST_INDEX + MAX_NUM_GROUP ,
	};
	
	bool onInit();
	bool onEvent( int event , int id , GWidget* ui );
	void doChangeGroup( StageGroupID group );
};



#endif // MainMenuStage_h__