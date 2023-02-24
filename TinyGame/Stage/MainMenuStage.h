#ifndef MainMenuStage_h__
#define MainMenuStage_h__

#include "GameMenuStage.h"

#include "StageRegister.h"

class MainMenuStage : public GameMenuStage
{
	typedef GameMenuStage BaseClass;
public:

	static const int MAX_NUM_GROUP = 100;

	enum
	{
		UI_VIEW_REPLAY = BaseClass::NEXT_UI_ID,
		UI_GAME_OPTION ,

		UI_MAIN_GROUP ,
		UI_TEST_GROUP ,
		UI_GRAPHIC_TEST_GROUP ,
		UI_GAME_DEV_GROUP ,
		UI_GAME_DEV2_GROUP ,
		UI_GAME_DEV3_GROUP ,
		UI_GAME_DEV4_GROUP ,
		UI_CARD_GAME_DEV_GROUP ,
		UI_MISC_TEST_GROUP ,
		UI_FEATURE_DEV_GROUP,

		UI_NET_TEST_SV  ,
		UI_NET_TEST_CL  ,



		UI_GROUP_START_INDEX,
		UI_GROUP_CARD_INDEX  = UI_GROUP_START_INDEX,
		UI_GROUP_STAGE_INDEX = UI_GROUP_CARD_INDEX + MAX_NUM_GROUP,

		NEXT_UI_ID           = UI_GROUP_STAGE_INDEX + MAX_NUM_GROUP ,
	};
	
	bool onInit();
	bool onWidgetEvent( int event , int id , GWidget* ui );
	void doChangeWidgetGroup( StageGroupID group );

	void changeStageGroup(EExecGroup group);
	void createStageGroupButton(int& delay, int& xUI, int& yUI);


	std::vector< GWidget* > mCategoryWidgets;
	EExecGroup mCurGroup;


	std::vector< GWidget* > mHistoryWidgets;
};



#endif // MainMenuStage_h__