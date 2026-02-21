#include "TinyGamePCH.h"
#include "MainMenuStage.h"

#include "GameWidget.h"
#include "GameWidgetID.h"

#include "Widget/GameRoomUI.h"
#include "GameModule.h"
#include "GameModuleManager.h"

#include "NetGameMode.h"

#include "StageRegister.h"
#include "RenderUtility.h"
#include "Localization.h"
#include "PropertySet.h"

#include "Poker/PokerGame.h"
#include "MiscTestStage.h"

#include "EasingFunction.h"

#include <cmath>
#include "StringParse.h"
#include "UserProfile.h"

#include "SystemPlatform.h"
#include "MiscTestRegister.h"
#include "CommonWidgets.h"
#include "MenuLayoutHelper.h"


#include "ExecutionManager.h"
#include "ExecutionPanel.h"


extern void RegisterStageGlobal();
extern TINY_API IExecutionServices* GExecutionServices;


// GLauncherItem moved to WorkspaceRenderer.cpp


// GSidebarButton moved to SidebarNavigator

class MainOptionBook : public GNoteBook
{
	typedef GNoteBook BaseClass;
public:
	static Vec2i const UISize;

	enum
	{
		UI_USER_NAME = BaseClass::NEXT_UI_ID ,
		UI_LAN_CHHICE ,
	};
	MainOptionBook( int id , Vec2i const& pos , GWidget* parent );
	virtual bool onChildEvent( int event , int id , GWidget* ui );
	void renderUser( GWidget* ui );
};


Vec2i const MainOptionBook::UISize( 300 , 300 );

MainOptionBook::MainOptionBook( int id , Vec2i const& pos , GWidget* parent ) 
	:BaseClass( id , pos , UISize , parent )
{
	Vec2i pageSize = getPageSize();
	Page* page;
	page = addPage( LOCTEXT("User") );
	page->setRenderCallback( RenderCallBack::Create( this , &MainOptionBook::renderUser ) );
	{
		UserProfile& profile = ::Global::GetUserProfile();

		Vec2i uiPos( 130 , 20 );
		Vec2i const uiSize( 150 , 25 );
		Vec2i const off( 0 , 35 );
		GTextCtrl* texCtrl = new GTextCtrl( UI_USER_NAME , uiPos  , uiSize.x , page );
		texCtrl->setValue( profile.name );

		uiPos += off;
		GChoice* choice = new GChoice( UI_LAN_CHHICE , uiPos , uiSize , page );
		choice->addItem( LOCTEXT("Traditional Chinese") );
		choice->addItem( LOCTEXT("English") );
		switch( profile.language )
		{
		case LAN_CHINESE_T: choice->setSelection(0); break;
		case LAN_ENGLISH:   choice->setSelection(1); break;
		}
		uiPos += off;
	}
	{
		Vec2i buttonSize( 100 , 20 );
		Vec2i buttonPos = pageSize - buttonSize - Vec2i( 5 , 5 );
		Vec2i bPosOff( -( buttonSize.x + 10 ) , 0 );

		GButton* button; 
		button = new GButton( UI_YES        , buttonPos , buttonSize , page );
		button->setTitle( LOCTEXT("Accept") );
		button = new GButton( UI_SET_DEFULT , buttonPos += bPosOff  , buttonSize , page );
		button->setTitle( LOCTEXT("Set Default") );
	}
}

bool MainOptionBook::onChildEvent( int event , int id , GWidget* ui )
{
	switch( id )
	{
	case UI_YES:
		{
			UserProfile& profile = ::Global::GetUserProfile();
			profile.name = getCurPage()->findChildT<GTextCtrl>( UI_USER_NAME )->getValue();
			::Global::GUI().getManager().getModalWidget()->destroy();
		}
		return false;
	}
	return true;
}

void MainOptionBook::renderUser( GWidget* ui )
{
	Vec2i pos = ui->getWorldPos() + Vec2i( 20 , 20 );

	IGraphics2D& g = Global::GetIGraphics2D();
	RenderUtility::SetFont( g , FONT_S12 );
	g.drawText( pos , LOCTEXT("Player Name") );
	pos.y += 35;
	g.drawText( pos , LOCTEXT("Language") );
}

struct SingleDevEntry
{
	char const* gameName;
	ExecutionEntryInfo entry;
};

SingleDevEntry GSingleDev[] =
{
#define STAGE_INFO( DECL , GAME_NAME , ... ) { GAME_NAME , ExecutionEntryInfo( DECL , MakeChangeSingleGame(GAME_NAME) , __VA_ARGS__ ) }

	STAGE_INFO( "Carcassonne" , "Carcassonne", EExecGroup::SingleDev ),
	STAGE_INFO( "TD Test" , "TowerDefend", EExecGroup::SingleDev) ,
	STAGE_INFO( "Rich Test" , "Rich", EExecGroup::SingleDev) ,
	STAGE_INFO( "BomberMan Test" , "BomberMan", EExecGroup::SingleDev) ,
	STAGE_INFO( "Bubble Test" , "Bubble", EExecGroup::SingleDev) ,
	STAGE_INFO( "CubeWorld Test" , "CubeWorld", EExecGroup::SingleDev) ,

#undef STAGE_INFO
};


MainMenuStage::MainMenuStage()
	: mNavigator()
	, mWorkspaceRenderer()
{

}

bool MainMenuStage::onInit()
{
	if ( !BaseClass::onInit() )
		return false;

	GameModuleVec games;
	Global::ModuleManager().classifyGame(ATTR_SINGLE_SUPPORT, games);

	auto IsIncludeDev = [](char const* name) -> bool
	{
		for (auto const& info : GSingleDev)
		{
			if (FCString::Compare(info.gameName, name) == 0)
				return true;
		}
		return false;
	};

	for (IGameModule* game : games)
	{
		if (IsIncludeDev(game->getName()))
			continue;

		if (ExecutionRegisterCollection::Get().findExecutionByTitle(game->getName()) == nullptr)
		{
			ExecutionRegisterCollection::Get().registerExecution({
				game->getName(),
				[game](IGameExecutionContext& context)
				{
					context.playSingleGame(game->getName());
				}, EExecGroup::SingleGame
			});
		}
	}

	for (auto const& info : GSingleDev)
	{
		if (ExecutionRegisterCollection::Get().findExecutionByTitle(info.entry.title) == nullptr)
		{
			ExecutionRegisterCollection::Get().registerExecution(info.entry);
		}
	}

	RegisterStageGlobal();

	getManager()->setTickTime( gDefaultTickTime );

	::Global::GUI().cleanupWidget();
	mWorkspaceRenderer.init();

	// Set Default Group
	mCurGroup = EExecGroup::NumGroup;
	// mNavigator.setMode(SidebarNavigator::ViewMode::History); // Remnant removed
	// mSidebarMode = ViewMode::Group; // Removed

	int const LayerGroup = (int)ViewMode::Group;
	int const LayerCategory = (int)ViewMode::Category;
	int const LayerHistory = (int)ViewMode::History;

	using Item = SidebarNavigator::SidebarItem;
	// Helper to cast enum
	auto G = [](EExecGroup g) { return (int)g; };

	mNavigator.clearItems(LayerGroup);

	// Standard Group Layer items
	mNavigator.addItem(LayerGroup, Item{ "View History", UI_VIEW_HISTORY, G(EExecGroup::NumGroup) });
	mNavigator.addItem(LayerGroup, Item{ "Game Dev", UI_GAME_DEV_GROUP, G(EExecGroup::Dev), {
		{ "Card Game..", UI_CARD_GAME_DEV_GROUP, G(EExecGroup::Dev) },
		{ "Net Test( Server )", UI_NET_TEST_SV, G(EExecGroup::Dev) },
		{ "Net Test( Client )", UI_NET_TEST_CL, G(EExecGroup::Dev) },
	} });
	mNavigator.addItem(LayerGroup, Item{ "Game Dev 2", UI_GAME_DEV2_GROUP, G(EExecGroup::SingleDev) });
	mNavigator.addItem(LayerGroup, Item{ "Physics Test", UI_GAME_DEV3_GROUP, G(EExecGroup::PhyDev) });
	mNavigator.addItem(LayerGroup, Item{ "Dev Test", UI_GAME_DEV4_GROUP, G(EExecGroup::Dev4) });
	mNavigator.addItem(LayerGroup, Item{ "Graphic Test", UI_GRAPHIC_TEST_GROUP, G(EExecGroup::GraphicsTest) });
	mNavigator.addItem(LayerGroup, Item{ "Test", UI_TEST_GROUP, G(EExecGroup::Test) });
	mNavigator.addItem(LayerGroup, Item{ "Feature Dev", UI_FEATURE_DEV_GROUP, G(EExecGroup::FeatureDev) });
	mNavigator.addItem(LayerGroup, Item{ "Misc Test", UI_MISC_TEST_GROUP, G(EExecGroup::MiscTest), {
		{ "Test Stages", UI_MISC_TEST_STAGE_SUBGROUP, G(EExecGroup::MiscTest) },
		{ "Test Functions", UI_MISC_TEST_FUNC_SUBGROUP, G(EExecGroup::MiscTest) },
	} });
	mNavigator.addItem(LayerGroup, Item{ "Single Player", UI_SINGLEPLAYER, G(EExecGroup::SingleGame) });
	mNavigator.addItem(LayerGroup, Item{ "Multi Player", UI_MULTIPLAYER, G(EExecGroup::NumGroup), {
		{ "Create Server", UI_CREATE_SERVER, G(EExecGroup::NumGroup) },
		{ "Connect Server", UI_BUILD_CLIENT, G(EExecGroup::NumGroup) },
	} });
	mNavigator.addItem(LayerGroup, Item{ "View Replay", UI_VIEW_REPLAY, G(EExecGroup::NumGroup) });

	// Populate Categories
	mNavigator.clearItems(LayerCategory);
	auto registeredCats = ExecutionRegisterCollection::Get().getRegisteredCategories();
	int catIdx = 0;
	for (auto const& cat : registeredCats)
	{
		mNavigator.addItem(LayerCategory, Item{ cat, UI_GROUP_START_INDEX + 300 + catIdx, 0 });
		catIdx++;
	}

	// Selection Logic
	mNavigator.setSelectionPolicy([this](SidebarNavigator::SidebarItem const& item, int level) -> bool
	{
		if (mCurViewMode == ViewMode::Category)
		{
			return item.name.compare(mCurCategory) == 0;
		}
		
		// Group or History mode
		if (level == 1)
		{
			if (mCurViewMode == ViewMode::History)
			{
				return item.id == UI_VIEW_HISTORY;
			}
			
			// Only highlight groups if in Group View Mode
			if (mCurViewMode != ViewMode::Group)
				return false;

			bool bSelected = ((int)mCurGroup == item.userData);
			if (mCurGroup == EExecGroup::NumGroup && item.id != UI_ANY)
			{
				if (item.id == UI_MULTIPLAYER && mCurGroup == EExecGroup::NumGroup) bSelected = true;
			}
			return bSelected;
		}
		else
		{
			return (mCurSubGroup == item.id) && (mCurSubGroup != UI_ANY);
		}
	});



	// Set Default View to History but Sidebar Mode to Group (to show list)
	mCurViewMode = ViewMode::History;
	mNavigator.setMode((int)ViewMode::Group);

	// Initialize Sidebar and Default Workspace
	// mScrollBar moved to WorkspaceRenderer

	mExecutionManager.setCommandQueue([this](std::function<void()> func)
	{
		addGameThreadCommnad(std::move(func));
	});

	refreshMainWorkspace();

	GExecutionServices = &mExecutionManager;

	return true;
}

void MainMenuStage::onUpdate(GameTimeSpan deltaTime)
{
	BaseClass::onUpdate(deltaTime);
	processGameThreadCommands();
}

void MainMenuStage::onRender(float dFrame)
{
	BaseClass::onRender(dFrame);

	IGraphics2D& g = Global::GetIGraphics2D();
	g.beginRender();

	RenderUtility::DrawDashboardBackground(g);

	int sidebarWidth = MenuLayout::SidebarWidth;
	int screenHeight = Global::GetScreenSize().y;

	// Draw Sidebar Background Panel
	// Draw Sidebar Background Panel
	g.setPen(MenuLayout::SidebarBorderColor, 1);
	g.setBrush(MenuLayout::SidebarBgColor); 
	g.drawRect(Vec2i(0, 0), Vec2i(MenuLayout::SidebarWidth, screenHeight));

	// Draw Header background
	g.setBrush(MenuLayout::HeaderBgColor);
	RenderUtility::SetPen(g, EColor::Null);
	g.drawRect(Vec2i(1, 1), Vec2i(MenuLayout::SidebarWidth - 2, MenuLayout::HeaderHeight));

	// Draw vertical separator line
	g.setPen(MenuLayout::SidebarBorderColor, 2);
	g.drawLine(Vec2i(MenuLayout::SidebarWidth, 0), Vec2i(MenuLayout::SidebarWidth, screenHeight));

	// Sidebar Title
	RenderUtility::SetFont(g, FONT_S12);
	g.setTextColor(Color3ub(255, 255, 255));
	g.drawText(Vec2i(15, 20), "DASHBOARD");

	g.endRender();
}


// Removed createSidebar
// createSidebar implementation moved to SidebarNavigator

void MainMenuStage::refreshMainWorkspace()
{
	clearTask();
	mSkipTasks.clear();
	mWorkspaceRenderer.resetScroll();
	
	std::vector<WorkspaceItem> items;

	// Helper to convert Info to Item
	auto ConvertToItem = [&](ExecutionEntryInfo const& info, int id) -> WorkspaceItem
	{
		WorkspaceItem item;
		item.title = info.title;
		item.detail = (info.categories.empty()) ? "General" : info.categories.begin()->c_str();
		item.uiID = id;
		
		item.statusText = "ACTIVE";
		item.statusColor = Color3ub(140, 180, 140); // Default Green

		switch (info.group)
		{
		case EExecGroup::Dev:
		case EExecGroup::SingleDev:
		case EExecGroup::FeatureDev:
			item.statusText = "DEVELOPING";
			item.statusColor = Color3ub(220, 180, 60); // Golden Yellow
			break;
		case EExecGroup::GraphicsTest:
		case EExecGroup::PhyDev:
		case EExecGroup::Test:
			item.statusText = "EXPERIMENTAL";
			item.statusColor = Color3ub(180, 120, 220); // Purple
			break;
		case EExecGroup::SingleGame:
			item.statusText = "STABLE";
			item.statusColor = Color3ub(100, 220, 100); // Bright Green
			break;
		}
		return item;
	};

	int idx = 0;

	if (mCurSubGroup == UI_CARD_GAME_DEV_GROUP)
	{
		using namespace Poker;
		for (int i = 0; i < RULE_COUNT; ++i)
		{
			GameRule rule = (GameRule)i;
			ExecutionEntryInfo info(
				ToRuleString(rule),
				[this, rule](IGameExecutionContext& context){
					Poker::GameModule* game = (Poker::GameModule*)Global::ModuleManager().changeGame(POKER_GAME_NAME);
					if (game)
					{
						game->setRule(rule);
						game->beginPlay(*getManager(), EGameMode::Single);
					}
				},
				EExecGroup::Dev
			);
			// Use special ID for custom cards
			int cardId = UI_POKER_CARD_START + i;
			items.push_back(ConvertToItem(info, cardId));
			idx++;
		}
	}
	else if (mCurViewMode == ViewMode::History)
	{
		PropertySet& config = ::Global::GameConfig();
		TArray< std::string > historyTitles;
		config.getStringValues("Entry", "ExecHistory", historyTitles);

		for (auto const& title : historyTitles)
		{
			ExecutionEntryInfo const* info = ExecutionRegisterCollection::Get().findExecutionByTitle(title.c_str());
			if (info)
			{
				items.push_back(ConvertToItem(*info, UI_GROUP_STAGE_INDEX + idx));
				idx++;
			}
		}
	}
	else if (mCurViewMode == ViewMode::Category)
	{
		if (!mCurCategory.empty())
		{
			auto infosPtr = ExecutionRegisterCollection::Get().getExecutionsByCategory(mCurCategory);
			for (auto const* pInfo : infosPtr)
			{
				items.push_back(ConvertToItem(*pInfo, UI_GROUP_STAGE_INDEX + idx));
				idx++;
			}
		}
	}
	else if (mCurGroup == EExecGroup::NumGroup)
	{
		// Shell group - no cards to display
	}
	else if (mCurGroup == EExecGroup::MiscTest)
	{
		auto const& execInfos = ExecutionRegisterCollection::Get().getGroupExecutions(
			(mCurSubGroup == UI_MISC_TEST_FUNC_SUBGROUP) ? EExecGroup::MiscTestFunc : EExecGroup::MiscTest);

		for (auto const& info : execInfos)
		{
			items.push_back(ConvertToItem(info, UI_GROUP_STAGE_INDEX + idx));
			idx++;
		}
	}
	else
	{
		auto const& execInfos = ExecutionRegisterCollection::Get().getGroupExecutions(mCurGroup);
		for (auto const& info : execInfos)
		{
			items.push_back(ConvertToItem(info, UI_GROUP_STAGE_INDEX + idx));
			idx++;
		}
	}

	mWorkspaceRenderer.refresh(items);
	mNavigator.createSidebar(nullptr, false, false);
}

void MainMenuStage::updateHistoryList()
{
	mCurViewMode = ViewMode::History;
	refreshMainWorkspace();
}

MsgReply MainMenuStage::onMouse(MouseMsg const& msg)
{
	if (msg.x() < MenuLayout::SidebarWidth)
	{
		if (msg.onWheelFront())
		{
			mNavigator.handleMouseWheel(30);
			return MsgReply::Handled();
		}
		else if (msg.onWheelBack())
		{
			mNavigator.handleMouseWheel(-30);
			return MsgReply::Handled();
		}
	}
	else
	{
		if (msg.onWheelFront())
		{
			mWorkspaceRenderer.handleMouseWheel(30);
		}
		else if (msg.onWheelBack())
		{
			mWorkspaceRenderer.handleMouseWheel(-30);
		}
	}
	return BaseClass::onMouse(msg);
}

void MainMenuStage::doChangeWidgetGroup( StageGroupID group ) { refreshMainWorkspace(); }
void MainMenuStage::createStageGroupButton( int& delay , int& xUI , int& yUI ) {}
void MainMenuStage::changeStageGroup(EExecGroup group) { mCurGroup = group; }

bool MainMenuStage::onWidgetEvent( int event , int id , GWidget* ui )
{
	if (id >= UI_GROUP_START_INDEX && id < UI_GROUP_STAGE_INDEX)
	{
	// Sidebar selection
		if (mNavigator.getMode() == (int)ViewMode::Group || mNavigator.getMode() == (int)ViewMode::History)
		{
			if (id < UI_GROUP_START_INDEX + MAX_NUM_GROUP)
			{
				EExecGroup group = (EExecGroup)(id - UI_GROUP_START_INDEX);
				if (mCurGroup == group && mCurSubGroup == UI_ANY && mCurViewMode == ViewMode::Group)
					return false;

				EExecGroup oldGroup = mCurGroup;
				mCurGroup = group;
				mCurSubGroup = UI_ANY;
				mCurViewMode = ViewMode::Group;
				
				// Sync Navigator
				mNavigator.setMode((int)ViewMode::Group);

				refreshMainWorkspace();
				// Only animate if group structure changes (opened/closed different group)
				// refreshMainWorkspace already calls createSidebar(false, false)
				if (oldGroup != mCurGroup)
				{
					mNavigator.createSidebar(nullptr, false, true);
				}
			}
		}
		else if (mNavigator.getMode() == (int)ViewMode::Category)
		{
			int idx = id - (UI_GROUP_START_INDEX + 300);
			auto registeredCats = ExecutionRegisterCollection::Get().getRegisteredCategories();
			if (idx >= 0 && idx < (int)registeredCats.size())
			{
				if (mCurCategory == registeredCats[idx] && mCurViewMode == ViewMode::Category)
					return false;

				mCurCategory = registeredCats[idx];
				mCurViewMode = ViewMode::Category;
				// Sync Navigator
				// mNavigator.setCategory(mCurCategory); // Removed

				refreshMainWorkspace();
			}
		}
		return false;
	}

	switch (id)
	{
	case UI_MAIN_GROUP:
		{
			auto newMode = (mNavigator.getMode() == (int)ViewMode::Group) ? ViewMode::Category : ViewMode::Group;
			// mCurViewMode = newMode; // Don't change view mode, only sidebar mode
			mNavigator.setMode((int)newMode);

			if (newMode == ViewMode::Category && mCurCategory.empty())
			{
				auto registeredCats = ExecutionRegisterCollection::Get().getRegisteredCategories();
				if (registeredCats.size() > 0) mCurCategory = registeredCats[0];
			}
			
			mNavigator.resetScroll();
			// refreshMainWorkspace(); // Don't refresh workspace
			mNavigator.createSidebar(nullptr, true, false); // Only refresh sidebar
		}
		return false;

	case UI_VIEW_REPLAY:
		getManager()->changeStage(STAGE_REPLAY_EDIT);
		return false;

	case UI_VIEW_HISTORY:
		{
			mCurViewMode = ViewMode::History;
			mCurGroup = EExecGroup::NumGroup; // Clear current group selection
			refreshMainWorkspace();
			// Ensure sidebar updates (to show View History selected)
			mNavigator.createSidebar(nullptr, false, false);
		}
		return false;

	case UI_GAME_OPTION:
		{
			Vec2i pos = GUISystem::calcScreenCenterPos(MainOptionBook::UISize);
			MainOptionBook* book = new MainOptionBook(UI_ANY, pos, nullptr);
			::Global::GUI().addWidget(book);
			::Global::GUI().doModel(book);
		}
		return false;

	case UI_MULTIPLAYER:
		mCurGroup = EExecGroup::NumGroup;
		mCurSubGroup = UI_ANY;
		mCurViewMode = ViewMode::Group;
		
		mNavigator.setMode((int)ViewMode::Group);

		// Special: Shell groups with no entries don't refresh the workspace
		mNavigator.createSidebar(nullptr, true, true);
		return false;

	case UI_MISC_TEST_GROUP:
		{
			EExecGroup group = EExecGroup::MiscTest;
			if (mCurGroup == group && mCurSubGroup == UI_ANY && mCurViewMode == ViewMode::Group)
				return false;

			mCurGroup = group;
			mCurSubGroup = UI_ANY;
			mCurViewMode = ViewMode::Group;

			mNavigator.setMode((int)ViewMode::Group);

			mNavigator.createSidebar(nullptr, false, true);
		}
		return false;

	case UI_GAME_DEV_GROUP:
	case UI_GAME_DEV2_GROUP:
	case UI_GAME_DEV3_GROUP:
	case UI_GAME_DEV4_GROUP:
	case UI_FEATURE_DEV_GROUP:
	case UI_GRAPHIC_TEST_GROUP:
	case UI_TEST_GROUP:
	case UI_SINGLEPLAYER:
		{
			EExecGroup group = EExecGroup::NumGroup;
			switch(id)
			{
			case UI_GAME_DEV_GROUP: group = EExecGroup::Dev; break;
			case UI_GAME_DEV2_GROUP: group = EExecGroup::SingleDev; break;
			case UI_GAME_DEV3_GROUP: group = EExecGroup::PhyDev; break;
			case UI_GAME_DEV4_GROUP: group = EExecGroup::Dev4; break;
			case UI_FEATURE_DEV_GROUP: group = EExecGroup::FeatureDev; break;
			case UI_GRAPHIC_TEST_GROUP: group = EExecGroup::GraphicsTest; break;
			case UI_TEST_GROUP: group = EExecGroup::Test; break;
			case UI_SINGLEPLAYER: group = EExecGroup::SingleGame; break;
			}

			if (mCurGroup == group && mCurSubGroup == UI_ANY && mCurViewMode == ViewMode::Group)
				return false;

			EExecGroup oldGroup = mCurGroup;
			mCurGroup = group;
			mCurSubGroup = UI_ANY;
			mCurViewMode = ViewMode::Group;
			refreshMainWorkspace();
			// Only animate if group structure changes
			if (oldGroup != mCurGroup)
			{
				mNavigator.createSidebar(nullptr, false, true);
			}
		}
		return false;

	case UI_CARD_GAME_DEV_GROUP:
		mCurSubGroup = UI_CARD_GAME_DEV_GROUP;
		// mNavigator.setSubGroup(mCurSubGroup);
		refreshMainWorkspace();
		// Update sidebar to show selection
		mNavigator.createSidebar(nullptr, false, false);
		return false;

	case UI_MISC_TEST_STAGE_SUBGROUP:
		mCurSubGroup = UI_MISC_TEST_STAGE_SUBGROUP;
		// mNavigator.setSubGroup(mCurSubGroup);
		refreshMainWorkspace();
		mNavigator.createSidebar(nullptr, false, false);
		return false;

	case UI_MISC_TEST_FUNC_SUBGROUP:
		mCurSubGroup = UI_MISC_TEST_FUNC_SUBGROUP;
		// mNavigator.setSubGroup(mCurSubGroup);
		refreshMainWorkspace();
		mNavigator.createSidebar(nullptr, false, false);
		return false;

	case UI_BACK_GROUP:
		mCurSubGroup = UI_ANY;
		// mNavigator.setSubGroup(mCurSubGroup);
		refreshMainWorkspace();
		mNavigator.createSidebar(nullptr, false, true);
		return false;

	case UI_CREATE_SERVER: return true;
	case UI_BUILD_CLIENT: return true;
	case UI_EXIT_GAME: return true;

	case UI_NET_TEST_SV:
	case UI_NET_TEST_CL:
		changeStage(new NetTestStage());
		return false;

	// UI_SCROLLBAR case removed

	}

	if (id >= UI_POKER_CARD_START && id < UI_POKER_CARD_START + Poker::RULE_COUNT)
	{
		// Sidebar poker buttons are disabled, this code might not be reached but keeping it safe
		int ruleIdx = id - UI_POKER_CARD_START;
		Poker::GameModule* game = (Poker::GameModule*)Global::ModuleManager().changeGame(POKER_GAME_NAME);
		if (game)
		{
			game->setRule((Poker::GameRule)ruleIdx);
			game->beginPlay(*getManager(), EGameMode::Single);
		}
		return false;
	}

	if (id >= UI_GROUP_STAGE_INDEX && id < UI_GROUP_STAGE_INDEX + MAX_NUM_GROUP)
	{
		int idx = id - UI_GROUP_STAGE_INDEX;
		ExecutionEntryInfo const* info = nullptr;
		if (mCurViewMode == ViewMode::History)
		{
			PropertySet& config = ::Global::GameConfig();
			TArray< std::string > historyTitles;
			config.getStringValues("Entry", "ExecHistory", historyTitles);
			
			int findIdx = 0;
			for (auto const& title : historyTitles)
			{
				ExecutionEntryInfo const* pInfo = ExecutionRegisterCollection::Get().findExecutionByTitle(title.c_str());
				if (pInfo)
				{
					if (findIdx == idx)
					{
						info = pInfo;
						break;
					}
					findIdx++;
				}
			}
		}
		else if (mCurViewMode == ViewMode::Category)
		{
			if (!mCurCategory.empty())
			{
				auto infos = ExecutionRegisterCollection::Get().getExecutionsByCategory(mCurCategory);
				if (idx < (int)infos.size()) info = infos[idx];
			}
		}
		else if (mCurGroup == EExecGroup::MiscTest)
		{
			if (mCurSubGroup == UI_MISC_TEST_FUNC_SUBGROUP)
			{
				auto const& execInfos = ExecutionRegisterCollection::Get().getGroupExecutions(EExecGroup::MiscTestFunc);
				if (idx < (int)execInfos.size()) info = &execInfos[idx];
			}
			else
			{
				auto const& execInfos = ExecutionRegisterCollection::Get().getGroupExecutions(EExecGroup::MiscTest);
				if (idx < (int)execInfos.size()) info = &execInfos[idx];
			}
		}
		else
		{
			auto const& execInfos = ExecutionRegisterCollection::Get().getGroupExecutions(mCurGroup);
			if (idx < (int)execInfos.size()) info = &execInfos[idx];
		}

		if (info) { execEntry(*info); return false; }
	}

	return true;
}

void MainMenuStage::execEntry(ExecutionEntryInfo const& info)
{
	ExecutionEntryInfo::RecordHistory(info);
	if (info.group == EExecGroup::MiscTestFunc)
	{
		char const* name = info.title;
		Async([this, name, func = info.execFunc](Thread* thread)
		{
			thread->setDisplayName(InlineString<>::Make("%s Thread", name));
			if (GExecutionServices)
			{
				mExecutionManager.registerThread(thread, name);

				class EmptyContext : public IGameExecutionContext
				{
				public:
					virtual void changeStage(StageBase* stage) {}
					virtual void playSingleGame(char const* name) {}
				};
				func(EmptyContext());
				if (GExecutionServices)
				{
					mExecutionManager.unregisterThread(thread);
				}
			}
		});
	}
	else
	{
		info.execFunc(*this);
	}
}

void MainMenuStage::changeStage(StageBase* stage) { getManager()->setNextStage(stage); }
void MainMenuStage::playSingleGame(char const* name)
{
	IGameModule* game = Global::ModuleManager().changeGame(name);
	if (game) game->beginPlay(*getManager(), EGameMode::Single);
}


void MainMenuStage::processGameThreadCommands()
{
	Mutex::Locker locker(mMutexGameThreadCommands);
	for (auto const& command : mGameThreadCommands)
	{
		command();
	}
	mGameThreadCommands.clear();
}

void MainMenuStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
{
	systemConfigs.bWasUsedPlatformGraphics = true;
	systemConfigs.bUseRenderThread = true;
}
