#ifndef MainMenuStage_h__
#define MainMenuStage_h__

#include "GameMenuStage.h"

#include "StageRegister.h"

class MainMenuStage : public GameMenuStage
	                , public IGameExecutionContext
{
	typedef GameMenuStage BaseClass;
public:

	static const int MAX_NUM_GROUP = 500;

	enum
	{
		UI_VIEW_REPLAY = BaseClass::NEXT_UI_ID,
		UI_VIEW_HISTORY,
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
		UI_GROUP_CARD_INDEX  = UI_GROUP_START_INDEX, // Deprecated alias, kept for compatibility if needed, but risky.
		
		UI_GROUP_STAGE_INDEX = UI_GROUP_START_INDEX + MAX_NUM_GROUP,

		UI_POKER_CARD_START  = UI_GROUP_STAGE_INDEX + MAX_NUM_GROUP,

		UI_SCROLLBAR,
		UI_BACK_GROUP,
		UI_PLAYER_PROFILE,
		NEXT_UI_ID           = UI_POKER_CARD_START + 100,
	};

	enum class ViewMode
	{
		Group,
		Category,
		History,
	};
	
	bool onInit();
	bool onWidgetEvent( int event , int id , GWidget* ui );
	void doChangeWidgetGroup( StageGroupID group );

	void changeStageGroup(EExecGroup group);
	void onRender(float dFrame) override;
	MsgReply onMouse(MouseMsg const& msg) override;
	void refreshMainWorkspace();
	void createSidebar(bool bClearTweens = true, bool bAnimateChange = false);
	void updateHistoryList();

	void createStageGroupButton(int& delay, int& xUI, int& yUI);

	void execEntry(ExecutionEntryInfo const& info);

	std::vector< GWidget* > mSidebarWidgets;
	int mSidebarScrollOffset = 0;
	int mMaxSidebarScroll = 0;
	GSlider* mScrollBar = nullptr;
	int mScrollOffset = 0;
	ViewMode mCurViewMode = ViewMode::Group;
	ViewMode mSidebarMode = ViewMode::Group;
	EExecGroup mCurGroup;
	int mCurSubGroup = UI_ANY;
	HashString mCurCategory;

	void changeStage(StageBase* stage) override;
	void playSingleGame(char const* name) override;

};

#endif // MainMenuStage_h__