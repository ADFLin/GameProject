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

extern void RegisterStageGlobal();

//--- Custom Launcher Item Widget ---
class GLauncherItem : public GWidget
{
	typedef GWidget BaseClass;
public:
	ExecutionEntryInfo mInfo;
	GButton* mLaunchBtn;

	GLauncherItem(int id, ExecutionEntryInfo const& info, Vec2i const& pos, Vec2i const& size, GWidget* parent)
		:BaseClass(pos, size, parent)
		,mInfo(info)
	{
		mID = id;
		// Create the "Launch" button at the bottom right
		int btnW = 70;
		int btnH = 30;
		mLaunchBtn = new GButton(id, Vec2i(size.x - btnW - 10, size.y - btnH - 12), Vec2i(btnW, btnH), this);
		mLaunchBtn->setTitle("Launch");
	}

	void onRender() override
	{
		IGraphics2D& g = Global::GetIGraphics2D();
		Vec2i p = getWorldPos();
		Vec2i s = getSize();

		// Draw Card Background
		g.setPen(Color3ub(45, 140, 180)); // Cyan border
		g.setBrush(Color3ub(35, 38, 42)); // Dark Slate
		g.drawRoundRect(p, s, Vec2i(12, 12));

		// Text Rendering
		RenderUtility::SetFont(g, FONT_S12);
		g.setTextColor(Color3ub(255, 255, 255));
		g.drawText(p + Vec2i(15, 12), mInfo.title); // Title

		RenderUtility::SetFont(g, FONT_S8);
		g.setTextColor(Color3ub(120, 180, 220)); // Subtle Cyan
		char const* category = (mInfo.categories.empty()) ? "General" : mInfo.categories.begin()->c_str();
		g.drawText(p + Vec2i(15, 36), category); // Row 2: Technical Tags (Category)

		// Row 3: Dynamic Status (Replaces Ready)
		char const* statusStr = "ACTIVE";
		Color3ub statusColor(140, 180, 140); // Default Muted Green

		switch (mInfo.group)
		{
		case EExecGroup::Dev:
		case EExecGroup::SingleDev:
		case EExecGroup::FeatureDev:
			statusStr = "DEVELOPING";
			statusColor = Color3ub(220, 180, 60); // Golden Yellow
			break;
		case EExecGroup::GraphicsTest:
		case EExecGroup::PhyDev:
		case EExecGroup::Test:
			statusStr = "EXPERIMENTAL";
			statusColor = Color3ub(180, 120, 220); // Purple/Cyan
			break;
		case EExecGroup::SingleGame:
			statusStr = "STABLE";
			statusColor = Color3ub(100, 220, 100); // Bright Green
			break;
		}

		g.setTextColor(statusColor);
		RenderUtility::SetFont(g, FONT_S8);
		g.drawText(p + Vec2i(15, s.y - 23), statusStr); 
	}

	MsgReply onMouseMsg(MouseMsg const& msg) override
	{
		return BaseClass::onMouseMsg(msg);
	}
};

class GSidebarButton : public GButtonBase
{
	typedef GButtonBase BaseClass;
public:
	GSidebarButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
		:BaseClass(id, pos, size, parent)
	{
	}

	void setTitle(char const* str) { mTitle = str; }
	
	void onRender() override
	{
		IGraphics2D& g = Global::GetIGraphics2D();
		
		Vec2i pos = getWorldPos();
		Vec2i size = getSize();

		if (getButtonState() == BS_HIGHLIGHT || getButtonState() == BS_PRESS)
		{
			g.setBrush(Color3ub(65, 70, 75)); // More visible highlight
		}
		else if ( bSelected )
		{
			g.setBrush(Color3ub(55, 120, 180)); // Brighter selected
		}
		else
		{
			g.setBrush(Color3ub(30, 33, 36)); // Normal
		}
		RenderUtility::SetPen(g, EColor::Null);
		g.drawRoundRect(pos, size, Vec2i(4, 4));

		if (bGroupTitle)
		{
			// Render Group Arrow/indicator
			Vec2i arrowPos = pos + Vec2i(10, size.y / 2 - 4);
			g.setPen(Color3ub(120, 120, 120));
			if( bExpanded )
				RenderUtility::DrawTriangle(g, arrowPos, Vec2i(8, 8), RenderUtility::eDown);
			else
				RenderUtility::DrawTriangle(g, arrowPos, Vec2i(8, 8), RenderUtility::eRight);
		}

		RenderUtility::SetFont(g, bGroupTitle ? FONT_S10 : FONT_S8);
		g.setTextColor(bSelected ? Color3ub(255, 255, 255) : Color3ub(180, 180, 180));
		
		// Always use the same text position for consistent alignment
		int textX = 30;
		g.drawText(pos + Vec2i(textX, (size.y - 12) / 2), mTitle.c_str());
	}

	void onMouse(bool beIn) override
	{
		BaseClass::onMouse(beIn);
	}

	String mTitle;
	bool bSelected = false;
	bool bGroupTitle = false;
	bool bExpanded = false;
};

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

	// Set Default Group
	mCurGroup = EExecGroup::NumGroup;
	mCurViewMode = ViewMode::History;
	mSidebarMode = ViewMode::Group;

	// Initialize Sidebar and Default Workspace
	mScrollBar = new GSlider(UI_SCROLLBAR, Vec2i(Global::GetScreenSize().x - 25, 80), Global::GetScreenSize().y - 160, false, nullptr);
	::Global::GUI().addWidget(mScrollBar);
	mScrollBar->show(false);

	refreshMainWorkspace();

	return true;
}

void MainMenuStage::onRender(float dFrame)
{
	BaseClass::onRender(dFrame);

	IGraphics2D& g = Global::GetIGraphics2D();
	g.beginRender();

	RenderUtility::DrawDashboardBackground(g);

	int sidebarWidth = 200;
	int screenHeight = Global::GetScreenSize().y;

	// Draw Sidebar Background Panel
	g.setPen(Color3ub(45, 140, 180), 1);
	g.setBrush(Color3ub(20, 23, 26)); 
	g.drawRect(Vec2i(0, 0), Vec2i(sidebarWidth, screenHeight));

	// Draw Header background
	g.setBrush(Color3ub(35, 38, 42));
	RenderUtility::SetPen(g, EColor::Null);
	g.drawRect(Vec2i(1, 1), Vec2i(sidebarWidth - 2, 60));

	// Draw vertical separator line
	g.setPen(Color3ub(45, 140, 180), 2);
	g.drawLine(Vec2i(sidebarWidth, 0), Vec2i(sidebarWidth, screenHeight));

	// Sidebar Title
	RenderUtility::SetFont(g, FONT_S12);
	g.setTextColor(Color3ub(255, 255, 255));
	g.drawText(Vec2i(15, 20), "DASHBOARD");

	g.endRender();
}

void MainMenuStage::createSidebar(bool bClearTweens, bool bAnimateChange)
{
	// Clean up old sidebar widgets if any
	// Prioritize clearing animations to prevent accessing destroyed widgets
	if (bClearTweens)
	{
		::Global::GUI().getTweener().clear();
	}
	for (auto ui : mSidebarWidgets) 
	{ 
		ui->destroy(); 
	}
	mSidebarWidgets.clear();

	int sidebarWidth = 180;
	int sidebarX = 10;
	int btnHeight = 28;
	int screenHeight = Global::GetScreenSize().y;

	// Calculate content limits
	int topLimit = 145;  // Below MODE/HISTORY buttons
	int bottomLimit = screenHeight - 60; // Above Option/Exit buttons
	int viewportHeight = bottomLimit - topLimit;

	int totalContentHeight = 0;
	if (mSidebarMode == ViewMode::Category)
	{
		totalContentHeight = (int)ExecutionRegisterCollection::Get().getRegisteredCategories().size() * (btnHeight + 4);
	}
	else
	{
		totalContentHeight = 11 * (btnHeight + 4); // Based on static cats array size
	}

	mMaxSidebarScroll = std::max(0, totalContentHeight - viewportHeight + 20); // Add a small padding

	int currentY = topLimit + mSidebarScrollOffset;

	if (mSidebarMode == ViewMode::Group || mSidebarMode == ViewMode::History)
	{
		struct CatEntry { char const* name; EExecGroup group; int id = UI_ANY; };
		CatEntry cats[] = {
			{"Game Dev", EExecGroup::Dev, UI_GAME_DEV_GROUP},
			{"Game Dev 2", EExecGroup::SingleDev, UI_GAME_DEV2_GROUP},
			{"Physics Test", EExecGroup::PhyDev, UI_GAME_DEV3_GROUP},
			{"Dev Test", EExecGroup::Dev4, UI_GAME_DEV4_GROUP},
			{"Graphic Test", EExecGroup::GraphicsTest, UI_GRAPHIC_TEST_GROUP},
			{"Test", EExecGroup::Test, UI_TEST_GROUP},
			{"Feature Dev", EExecGroup::FeatureDev, UI_FEATURE_DEV_GROUP},
			{"Misc Test", EExecGroup::MiscTest, UI_MISC_TEST_GROUP},
			{"Single Player", EExecGroup::SingleGame, UI_SINGLEPLAYER},
			{"Multi Player", EExecGroup::NumGroup, UI_MULTIPLAYER},
			{"View Replay", EExecGroup::NumGroup, UI_VIEW_REPLAY}
		};

		auto hasSubGroup = [&](CatEntry const& cat)
		{
			if (cat.id == UI_MULTIPLAYER) return true;
			if (cat.group == EExecGroup::Dev) return true;
			return false;
		};
		
		int expandedPushHeight = 0;
		GWidget* expandedGroupWidget = nullptr;

		auto AddSidebarBtn = [&](int id, char const* name, bool bSelected, int level = 1, bool bIsGroup = false, bool bIsExpanded = false)
		{
			if (currentY >= topLimit - btnHeight && currentY <= bottomLimit)
			{
				int x = sidebarX + (level - 1) * 15;
				int w = sidebarWidth - 20 - (level - 1) * 15;
				GSidebarButton* btn = new GSidebarButton(id, Vec2i(x, currentY), Vec2i(w, (level > 1 ? btnHeight - 2 : btnHeight)), nullptr);
				btn->setTitle(name);
				btn->bSelected = bSelected && mCurViewMode == ViewMode::Group;
				btn->bGroupTitle = bIsGroup;
				btn->bExpanded = bIsExpanded;

				::Global::GUI().addWidget(btn);
				mSidebarWidgets.push_back(btn);

				
				if (level > 1) // Animate sub-items
				{
					Vec2i startPos = Vec2i(x - 20, currentY);
					Vec2i endPos = Vec2i(x, currentY);
					// Ensure we are at start pos immediately in case frame update is slow
					btn->setPos(startPos); 
					::Global::GUI().addMotion<Easing::OQuad>(btn, startPos, endPos, 200, (currentY - topLimit) / 2);
				}
				else if (bAnimateChange && expandedPushHeight > 0) // Animate push down for items below expanded group
				{
					Vec2i startPos = Vec2i(x, currentY - expandedPushHeight);
					Vec2i endPos = Vec2i(x, currentY);
					//btn->setPos(startPos); // Wait, this might flash if we don't handle it right
					::Global::GUI().addMotion<Easing::OQuad>(btn, startPos, endPos, 200, 0);
				}
				
			}
			currentY += (level > 1 ? btnHeight - 2 : btnHeight) + 4;
		};

		for (auto const& cat : cats)
		{
			int yBeforeGroup = currentY;
			bool bGroupSelected = (mCurGroup == cat.group) && (mCurViewMode == ViewMode::Group);
			if (mCurGroup == EExecGroup::NumGroup)
			{
				bGroupSelected = (cat.id == UI_MULTIPLAYER) && (mCurViewMode == ViewMode::Group);
			}
			else if (cat.id == UI_VIEW_REPLAY)
			{
				bGroupSelected = false;
			}

			char const* title = cat.name;

			AddSidebarBtn(cat.id != UI_ANY ? cat.id : (UI_GROUP_START_INDEX + (int)cat.group), title, bGroupSelected, 1, hasSubGroup(cat), bGroupSelected);

			if (bGroupSelected)
			{
				if (cat.id == UI_MULTIPLAYER)
				{
					AddSidebarBtn(UI_CREATE_SERVER, LOCTEXT("Create Server"), mCurSubGroup == UI_CREATE_SERVER, 2);
					AddSidebarBtn(UI_BUILD_CLIENT, LOCTEXT("Connect Server"), mCurSubGroup == UI_BUILD_CLIENT, 2);
				}
				else if (cat.group == EExecGroup::Dev)
				{
					AddSidebarBtn(UI_CARD_GAME_DEV_GROUP, "Card Game..", mCurSubGroup == UI_CARD_GAME_DEV_GROUP, 2);
					if (mCurSubGroup == UI_CARD_GAME_DEV_GROUP)
					{
						/*
						using namespace Poker;
						for (int i = 0; i < RULE_COUNT; ++i)
						{
							AddSidebarBtn(UI_GROUP_CARD_INDEX + i, ToRuleString((GameRule)i), false, 3);
						}
						*/
					}
					AddSidebarBtn(UI_NET_TEST_SV, LOCTEXT("Net Test( Server )"), mCurSubGroup == UI_NET_TEST_SV, 2);
					AddSidebarBtn(UI_NET_TEST_CL, LOCTEXT("Net Test( Client )"), mCurSubGroup == UI_NET_TEST_CL, 2);
				}
				
				// AddSidebarBtn(UI_BACK_GROUP, LOCTEXT("Back"), false, 2);
				
				expandedPushHeight = currentY - (yBeforeGroup + btnHeight + 4);
			}
		}
	}
	else if (mSidebarMode == ViewMode::Category)
	{
		auto registeredCats = ExecutionRegisterCollection::Get().getRegisteredCategories();
		int idx = 0;
		for (auto const& cat : registeredCats)
		{
			if (currentY >= topLimit - btnHeight && currentY <= bottomLimit)
			{
				GSidebarButton* btn = new GSidebarButton(UI_GROUP_START_INDEX + 300 + idx, Vec2i(sidebarX, currentY), Vec2i(sidebarWidth - 20, btnHeight), nullptr);
				btn->setTitle(cat.c_str());
				btn->bSelected = (mCurCategory == cat && mCurViewMode == ViewMode::Category);
				::Global::GUI().addWidget(btn);
				mSidebarWidgets.push_back(btn);
			}
			currentY += btnHeight + 4;
			idx++;
		}
	}

	// --- Fixed Elements (Not Scaled by Scroll) ---
	
	// View Mode Toggle (Wide Button) - Fixed at top under Dashboard Header
	GButton* modeBtn = new GButton(UI_MAIN_GROUP, Vec2i(sidebarX, 70), Vec2i(sidebarWidth - 20, 32), nullptr);
	if (mSidebarMode == ViewMode::Category)
	{
		modeBtn->setTitle("MODE: CATEGORY");
		modeBtn->setColor(Color3ub(60, 140, 220));
	}
	else
	{
		modeBtn->setTitle("MODE: GROUP");
		modeBtn->setColor(Color3ub(60, 140, 220));
	}
	::Global::GUI().addWidget(modeBtn);
	mSidebarWidgets.push_back(modeBtn);

	// History Button - Fixed at top below Mode
	GSidebarButton* historyBtn = new GSidebarButton(UI_VIEW_HISTORY, Vec2i(sidebarX, 105), Vec2i(sidebarWidth - 20, 32), nullptr);
	historyBtn->setTitle("VIEW HISTORY");
	historyBtn->bSelected = (mCurViewMode == ViewMode::History);
	::Global::GUI().addWidget(historyBtn);
	mSidebarWidgets.push_back(historyBtn);

	// Basic Options & Exit - Wider buttons at the bottom
	int bottomBtnW = (sidebarWidth - 30) / 2;
	GButton* optionBtn = new GButton(UI_GAME_OPTION, Vec2i(sidebarX, screenHeight - 50), Vec2i(bottomBtnW, 35), nullptr);
	optionBtn->setTitle("Option");
	::Global::GUI().addWidget(optionBtn);
	mSidebarWidgets.push_back(optionBtn);

	GButton* exitBtn = new GButton(UI_EXIT_GAME, Vec2i(sidebarX + bottomBtnW + 10, screenHeight - 50), Vec2i(bottomBtnW, 35), nullptr);
	exitBtn->setTitle("Exit");
	::Global::GUI().addWidget(exitBtn);
	mSidebarWidgets.push_back(exitBtn);
}

void MainMenuStage::refreshMainWorkspace()
{
	clearTask();
	mSkipTasks.clear();
	mScrollOffset = 0;
	if (mScrollBar) { mScrollBar->setValue(0); mScrollBar->show(false); }

	int sidebarWidth = 200;
	int startX = sidebarWidth + 20;
	int startY = 50; // Balanced top position

	// Clear previous UI and animations to prevent crash on rapid switching
	// Reverted cleanupWidget as it was too aggressive
	::Global::GUI().getTweener().clear();
	for (auto ui : mGroupUI) 
	{ 
		ui->destroy(); 
	}
	mGroupUI.clear();
	int cardWidth = 220;
	int cardHeight = 110;
	int spacing = 20;

	int columns = (Global::GetScreenSize().x - startX - 40) / (cardWidth + spacing);
	if (columns < 1) columns = 1;

	int idx = 0;
	int delay = 0;

	auto AddCard = [&](ExecutionEntryInfo const& info)
	{
		int x = startX + (idx % columns) * (cardWidth + spacing);
		int ry = (idx / columns) * (cardHeight + spacing);
		int y = startY + ry;

		GLauncherItem* item = new GLauncherItem(UI_GROUP_STAGE_INDEX + idx, info, Vec2i(::Global::GetScreenSize().x + 50, y), Vec2i(cardWidth, cardHeight), nullptr);
		::Global::GUI().addWidget(item);
		mGroupUI.push_back(item);

		::Global::GUI().addMotion<Easing::OQuad>(item, Vec2i(::Global::GetScreenSize().x + 50, y), Vec2i(x, y), 350, delay);
		delay += 15;
		idx++;
	};

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
				EExecGroup::Dev,
				0
			);
			
			// Use special ID for custom cards
			int cardId = UI_POKER_CARD_START + i;
			int x = startX + (idx % columns) * (cardWidth + spacing);
			int ry = (idx / columns) * (cardHeight + spacing);
			int y = startY + ry;

			GLauncherItem* item = new GLauncherItem(cardId, info, Vec2i(::Global::GetScreenSize().x + 50, y), Vec2i(cardWidth, cardHeight), nullptr);
			::Global::GUI().addWidget(item);
			mGroupUI.push_back(item);

			::Global::GUI().addMotion<Easing::OQuad>(item, Vec2i(::Global::GetScreenSize().x + 50, y), Vec2i(x, y), 350, delay);
			delay += 15;
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
			if (info) AddCard(*info);
		}
	}
	else if (mCurViewMode == ViewMode::Category)
	{
		if (!mCurCategory.empty())
		{
			auto infosPtr = ExecutionRegisterCollection::Get().getExecutionsByCategory(mCurCategory);
			for (auto const* pInfo : infosPtr) AddCard(*pInfo);
		}
	}
	else if (mCurGroup == EExecGroup::NumGroup)
	{
		// Shell group - no cards to display
	}
	else
	{
		auto const& execInfos = ExecutionRegisterCollection::Get().getGroupExecutions(mCurGroup);
		for (auto const& info : execInfos) AddCard(info);
	}

	if (idx > 0)
	{
		int totalRows = (idx + columns - 1) / columns;
		int totalHeight = totalRows * (cardHeight + spacing);
		int viewportHeight = Global::GetScreenSize().y - startY - 60; // Align with sidebar bottom limit

		if (totalHeight > viewportHeight)
		{
			mScrollBar->setRange(0, std::max(0, totalHeight - viewportHeight + 10));
			
			// Dynamic Tip Size
			int scrollBarLength = mScrollBar->getSize().y;
			int tipHeight = (int)((float)viewportHeight / totalHeight * scrollBarLength);
			if (tipHeight < 30) tipHeight = 30; // Min height for usability
			if (tipHeight > scrollBarLength) tipHeight = scrollBarLength;

			mScrollBar->getTipWidget()->setSize(Vec2i(mScrollBar->getSize().x, tipHeight));
			mScrollBar->show(true);
		}
	}

	createSidebar(false);
}

void MainMenuStage::updateHistoryList()
{
	mCurViewMode = ViewMode::History;
	refreshMainWorkspace();
}

MsgReply MainMenuStage::onMouse(MouseMsg const& msg)
{
	if (msg.x() < 200)
	{
		if (msg.onWheelFront())
		{
			mSidebarScrollOffset += 30;
			if (mSidebarScrollOffset > 0) mSidebarScrollOffset = 0;
			createSidebar();
			return MsgReply::Handled();
		}
		else if (msg.onWheelBack())
		{
			mSidebarScrollOffset -= 30;
			if (mSidebarScrollOffset < -mMaxSidebarScroll) mSidebarScrollOffset = -mMaxSidebarScroll;
			createSidebar();
			return MsgReply::Handled();
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
		if (mSidebarMode == ViewMode::Group || mSidebarMode == ViewMode::History)
		{
			if (id < UI_GROUP_START_INDEX + MAX_NUM_GROUP)
			{
				EExecGroup group = (EExecGroup)(id - UI_GROUP_START_INDEX);
				if (mCurGroup == group && mCurSubGroup == UI_ANY && mCurViewMode == ViewMode::Group)
					return false;

				mCurGroup = group;
				mCurSubGroup = UI_ANY;
				mCurSubGroup = UI_ANY;
				mCurViewMode = ViewMode::Group;
				refreshMainWorkspace();
				createSidebar(false, true);
			}
		}
		else if (mSidebarMode == ViewMode::Category)
		{
			int idx = id - (UI_GROUP_START_INDEX + 300);
			auto registeredCats = ExecutionRegisterCollection::Get().getRegisteredCategories();
			if (idx >= 0 && idx < (int)registeredCats.size())
			{
				if (mCurCategory == registeredCats[idx] && mCurViewMode == ViewMode::Category)
					return false;

				mCurCategory = registeredCats[idx];
				mCurViewMode = ViewMode::Category;
				refreshMainWorkspace();
			}
		}
		return false;
	}

	switch (id)
	{
	case UI_MAIN_GROUP:
		mSidebarMode = (mSidebarMode == ViewMode::Group) ? ViewMode::Category : ViewMode::Group;
		if (mSidebarMode == ViewMode::Category && mCurCategory.empty())
		{
			auto registeredCats = ExecutionRegisterCollection::Get().getRegisteredCategories();
			if (registeredCats.size() > 0) mCurCategory = registeredCats[0];
		}
		mSidebarScrollOffset = 0; // Reset scroll when switching view
		createSidebar(); // Only refresh sidebar, don't change workspace items
		return false;

	case UI_VIEW_REPLAY:
		getManager()->changeStage(STAGE_REPLAY_EDIT);
		return false;

	case UI_VIEW_HISTORY:
		mCurViewMode = (mCurViewMode == ViewMode::History) ? ViewMode::Group : ViewMode::History;
		refreshMainWorkspace();
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
		// Special: Shell groups with no entries don't refresh the workspace
		createSidebar(true, true);
		return false;

	case UI_GAME_DEV_GROUP:
	case UI_GAME_DEV2_GROUP:
	case UI_GAME_DEV3_GROUP:
	case UI_GAME_DEV4_GROUP:
	case UI_FEATURE_DEV_GROUP:
	case UI_GRAPHIC_TEST_GROUP:
	case UI_TEST_GROUP:
	case UI_SINGLEPLAYER:
	case UI_MISC_TEST_GROUP:
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
			case UI_MISC_TEST_GROUP: group = EExecGroup::MiscTest; break;
			}

			if (mCurGroup == group && mCurSubGroup == UI_ANY && mCurViewMode == ViewMode::Group)
				return false;

			mCurGroup = group;
			mCurSubGroup = UI_ANY;
			mCurViewMode = ViewMode::Group;
			refreshMainWorkspace();
			createSidebar(false, true); // Update sidebar selection and animation
		}
		return false;

	case UI_CARD_GAME_DEV_GROUP:
		mCurSubGroup = UI_CARD_GAME_DEV_GROUP;
		refreshMainWorkspace();
		createSidebar(false, true);
		return false;

	case UI_BACK_GROUP:
		mCurSubGroup = UI_ANY;
		refreshMainWorkspace();
		createSidebar(false, true);
		return false;

	case UI_CREATE_SERVER: return true;
	case UI_BUILD_CLIENT: return true;
	case UI_EXIT_GAME: return true;

	case UI_NET_TEST_SV:
	case UI_NET_TEST_CL:
		changeStage(new NetTestStage());
		return false;

	case UI_SCROLLBAR:
		{
			int offset = mScrollBar->getValue();
			int diff = offset - mScrollOffset;
			if (diff != 0)
			{
				mScrollOffset = offset;
				for (auto ui : mGroupUI)
				{
					Vec2i pos = ui->getPos();
					pos.y -= diff;
					ui->setPos(pos);
				}
			}
		}
		return false;
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
			if (idx < (int)historyTitles.size()) info = ExecutionRegisterCollection::Get().findExecutionByTitle(historyTitles[idx].c_str());
		}
		else if (mCurViewMode == ViewMode::Category)
		{
			if (!mCurCategory.empty())
			{
				auto infos = ExecutionRegisterCollection::Get().getExecutionsByCategory(mCurCategory);
				if (idx < (int)infos.size()) info = infos[idx];
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
	info.execFunc(*this);
}

void MainMenuStage::changeStage(StageBase* stage) { getManager()->setNextStage(stage); }
void MainMenuStage::playSingleGame(char const* name)
{
	IGameModule* game = Global::ModuleManager().changeGame(name);
	if (game) game->beginPlay(*getManager(), EGameMode::Single);
}
