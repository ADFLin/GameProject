#ifndef MainMenuStage_h__
#define MainMenuStage_h__

#include "GameMenuStage.h"

#include "StageRegister.h"

#include "PlatformThread.h"

#include "TinyCore/ExecutionManager.h"
#include "TinyCore/ExecutionPanel.h"

#include "SidebarNavigator.h"
#include "WorkspaceRenderer.h"

class MainMenuStage : public GameMenuStage
	                , public IGameExecutionContext
	                , public IGameRenderSetup
{
	typedef GameMenuStage BaseClass;
public:
	MainMenuStage();

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
		UI_MISC_TEST_STAGE_SUBGROUP,
		UI_MISC_TEST_FUNC_SUBGROUP,
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
	void onUpdate(GameTimeSpan deltaTime) override;
	MsgReply onMouse(MouseMsg const& msg) override;
	void refreshMainWorkspace();
	void updateHistoryList();

	void createStageGroupButton(int& delay, int& xUI, int& yUI);

	void execEntry(ExecutionEntryInfo const& info);

	// std::vector< GWidget* > mSidebarWidgets; // Moved to SidebarNavigator
	// int mSidebarScrollOffset = 0; // Moved
	// int mMaxSidebarScroll = 0;    // Moved
	
	ViewMode mCurViewMode = ViewMode::Group;
	EExecGroup mCurGroup;
	int mCurSubGroup = UI_ANY;
	HashString mCurCategory;
	// std::vector< GWidget* > mGroupUI; // Moved to WorkspaceRenderer
	// GSlider* mScrollBar; // Moved to WorkspaceRenderer
	// int mScrollOffset;   // Moved to WorkspaceRenderer

	WorkspaceRenderer mWorkspaceRenderer;
	SidebarNavigator mNavigator;
	friend class SidebarNavigator; // To allow navigator access UI_ constants if strictly needed, though public

	void changeStage(StageBase* stage) override;
	void playSingleGame(char const* name) override;


	//IGameRenderSetup
	void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override;


	void terminateThread(Thread* thread) { mExecutionManager.terminateThread(thread); }
	void resumeThread(Thread* thread) { mExecutionManager.resumeThread(thread); }
	void resumeAllThreads() { mExecutionManager.resumeAllThreads(); }

	template< class TFunc >
	void addGameThreadCommnad(TFunc&& func)
	{
		Mutex::Locker locker(mMutexGameThreadCommands);
		mGameThreadCommands.push_back(std::forward<TFunc>(func));
	}
	void processGameThreadCommands();

	ExecutionManager mExecutionManager;
	Mutex mMutexGameThreadCommands;
	TArray< std::function< void() > > mGameThreadCommands;
};

#endif // MainMenuStage_h__