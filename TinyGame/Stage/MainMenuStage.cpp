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

#include "Poker/PokerGame.h"
#include "MiscTestStage.h"

#include "EasingFunction.h"

#include <cmath>
#include "StringParse.h"

extern void RegisterStageGlobal();


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

		ExecutionRegisterCollection::Get().registerExecution({ game->getName(), [game]()
		{
			ExecutionRegisterHelper::ChangeSingleGame(game->getName());
		}, EExecGroup::SingleGame });
	}

	for (auto const& info : GSingleDev)
	{
		ExecutionRegisterCollection::Get().registerExecution(info.entry);
	}

	RegisterStageGlobal();

	getManager()->setTickTime( gDefaultTickTime );

	::Global::GUI().cleanupWidget();

#if 0
	GButton* button = new GButton( UI_ANY , Vec2i(100 ,20 ) , Vec2i(100 ,20 )  , NULL );

	::Global::GUI().getTweens()
		.tween< Easing::CLinear , WidgetSize >( *button , Vec2i(100 ,20 )  , Vec2i(200 , 40 )  , 2000 , 0 )
		.repeat( 10 ).repeatDelay( 10 );

	button->setTitle( "Test" );

	::Global::GUI().addWidget( button );
#endif

	std::vector< HashString > categories = ExecutionRegisterCollection::Get().getRegisteredCategories();
	GChoice* choice = new GChoice(UI_ANY, Vec2i(20, 20), Vec2i(100, 20), NULL);
	for (auto const& category : categories)
	{
		InlineString< 128 > title;
		int count = ExecutionRegisterCollection::Get().getExecutionsByCategory(category).size();
		title.format("%s (%d)", category.c_str(), count);
		uint slot = choice->addItem(title);
	}

	choice->onEvent = [this](int event, GWidget* widget) -> bool
	{
		for (auto widget :mCategoryWidgets )
		{
			widget->destroy();
		}
		mCategoryWidgets.clear();

		char const* selectValue = static_cast<GChoice*>(widget)->getSelectValue();
		char const* last = FStringParse::FindChar(selectValue, '(');
		StringView category = StringView{ selectValue , size_t( last - selectValue) };
		category.removeTailSpace();
		std::vector< ExecutionEntryInfo const*> stageInfoList = ExecutionRegisterCollection::Get().getExecutionsByCategory(category);

		int index = 0;
		for (auto& info : stageInfoList)
		{
			Vec2i pos = Vec2i(20, 45) + Vec2i(0, index * 25 );
			GButton* button = new GButton(UI_ANY, pos , Vec2i(200, 20), nullptr );
			button->setTitle(info->title);
			button->onEvent = [this,info](int event, GWidget* widget) -> bool
			{
				info->execFunc();
				return false;
			};
			mCategoryWidgets.push_back(button);
			::Global::GUI().addWidget(button);
			++index;
		}

		return false;
	};

	::Global::GUI().addWidget(choice);

	changeWidgetGroup( UI_MAIN_GROUP );
	return true;
}

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
		choice->addItem( "ÁcÅé¤¤¤å" );
		choice->addItem( "English" );
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

	Graphics2D& g = Global::GetGraphics2D();
	RenderUtility::SetFont( g , FONT_S12 );
	g.drawText( pos , LOCTEXT("Player Name") );
	pos.y += 35;
	g.drawText( pos , LOCTEXT("Language") );
}

Vec2i const MenuBtnSize(120, 25);
int const MenuDeltaDelay = 100;
int const MenuYOffset = 30;

#define CREATE_BUTTON( ID , NAME )\
	createButton( delay += MenuDeltaDelay , ID  , NAME  , Vec2i( xUI , yUI += MenuYOffset  ) , MenuBtnSize )

#define CREATE_BUTTON_POS_DELAY( ID , NAME , X , Y , DELAY )\
	createButton( DELAY  , ID  , NAME  , Vec2i( X , Y ) , MenuBtnSize )

void MainMenuStage::doChangeWidgetGroup( StageGroupID group )
{
	int xUI = ( Global::GetScreenSize().x - MenuBtnSize.x ) / 2 ;
	int yUI = 100;
	int offset = 30;
	int delay = 0;

	fadeoutGroup( MenuDeltaDelay );
	mGroupUI.clear();

	switch( group )
	{
	case UI_MAIN_GROUP:

		CREATE_BUTTON( UI_GAME_DEV_GROUP  , "Game Dev"   );
		CREATE_BUTTON( UI_GAME_DEV2_GROUP  , "Game Dev2"   );
		CREATE_BUTTON( UI_GAME_DEV3_GROUP  , "Phyics Test"   );
		CREATE_BUTTON( UI_GAME_DEV4_GROUP  , "Dev Test"   );
		CREATE_BUTTON( UI_GRAPHIC_TEST_GROUP , "Graphic Test" );
		CREATE_BUTTON( UI_TEST_GROUP      , "Test" );
		CREATE_BUTTON( UI_FEATURE_DEV_GROUP, "Feature Dev");
		changeStageGroup(EExecGroup::Main);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON( UI_SINGLEPLAYER  , LOCTEXT("SinglePlayer")  );
		CREATE_BUTTON( UI_MULTIPLAYER   , LOCTEXT("MultiPlayer")   );
		CREATE_BUTTON( UI_VIEW_REPLAY   , LOCTEXT("View Replay")   );
		//CREATE_BUTTON( UI_ABOUT_GAME    , LAN("About Game")    );
		CREATE_BUTTON( UI_GAME_OPTION   , LOCTEXT("Option")        );
		CREATE_BUTTON( UI_EXIT_GAME     , LOCTEXT("Exit")          );
		break;
	case UI_MULTIPLAYER:
		CREATE_BUTTON( UI_CREATE_SERVER , LOCTEXT("Create Server") );
		CREATE_BUTTON( UI_BUILD_CLIENT  , LOCTEXT("Connect Server"));
		CREATE_BUTTON( UI_BACK_GROUP    , LOCTEXT("Back")          );
		break;
	case UI_GRAPHIC_TEST_GROUP:
		changeStageGroup(EExecGroup::GraphicsTest);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON( UI_BACK_GROUP     , LOCTEXT("Back")          );
		break;
	case UI_TEST_GROUP:
		changeStageGroup(EExecGroup::Test);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON( UI_BACK_GROUP  , LOCTEXT("Back")          );
		break;
	case UI_GAME_DEV_GROUP:
		changeStageGroup(EExecGroup::Dev);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON( UI_CARD_GAME_DEV_GROUP  , "Card Game.." );
#if 1
		CREATE_BUTTON(UI_NET_TEST_SV, LOCTEXT("Net Test( Server )"));
		CREATE_BUTTON(UI_NET_TEST_CL, LOCTEXT("Net Test( Client )"));
#endif
		CREATE_BUTTON( UI_BACK_GROUP     , LOCTEXT("Back")          );
		break;
	case UI_GAME_DEV3_GROUP:
		changeStageGroup(EExecGroup::PhyDev);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON(UI_BACK_GROUP, LOCTEXT("Back"));
		break;
	case UI_GAME_DEV4_GROUP:
		changeStageGroup(EExecGroup::Dev4);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON(UI_BACK_GROUP, LOCTEXT("Back"));
		break;
	case UI_FEATURE_DEV_GROUP:
		changeStageGroup(EExecGroup::FeatureDev);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON(UI_BACK_GROUP, LOCTEXT("Back"));
		break;
	case UI_GAME_DEV2_GROUP:
		changeStageGroup(EExecGroup::SingleDev);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON( UI_BACK_GROUP     , LOCTEXT("Back")          );
		break;
	case UI_CARD_GAME_DEV_GROUP:
		CREATE_BUTTON( UI_BIG2_TEST      , "Big2 Test" );
		CREATE_BUTTON( UI_HOLDEM_TEST    , "Holdem Test" );
		CREATE_BUTTON( UI_FREECELL_TEST  , "FreeCell Test" );
		CREATE_BUTTON( UI_BACK_GROUP  , LOCTEXT("Back")          );
		break;
	case UI_SINGLEPLAYER:
		changeStageGroup(EExecGroup::SingleGame);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON( UI_BACK_GROUP    , LOCTEXT("Back")          );
		break;
	}

}

void MainMenuStage::createStageGroupButton( int& delay , int& xUI , int& yUI )
{
	int idx = 0;
	int numStage = ExecutionRegisterCollection::Get().getGroupExecutions(mCurGroup).size();

	if( numStage > 10 )
	{
		int PosLX = xUI - (MenuBtnSize.x + 6) / 2;
		int PosRX = xUI + (MenuBtnSize.x + 6) / 2;
		for( auto info : ExecutionRegisterCollection::Get().getGroupExecutions(mCurGroup) )
		{
			CREATE_BUTTON_POS_DELAY(UI_GROUP_STAGE_INDEX + idx, info.title, (idx % 2 ) ?PosRX : PosLX , yUI , delay );
			++idx;
			if( (idx % 2) == 0 )
			{
				yUI += MenuYOffset;
				delay += MenuDeltaDelay;
			}
		}
	}
	else
	{
		for( auto info : ExecutionRegisterCollection::Get().getGroupExecutions(mCurGroup) )
		{
			CREATE_BUTTON(UI_GROUP_STAGE_INDEX + idx, info.title);
			++idx;
		}
	}
}
#undef  CREATE_BUTTON

void MainMenuStage::changeStageGroup(EExecGroup group)
{
	mCurGroup = group;
}

bool MainMenuStage::onWidgetEvent( int event , int id , GWidget* ui )
{
	if ( id < UI_GROUP_INDEX  )
	{
		switch ( id )
		{
		case UI_NET_TEST_CL:
			{
				auto stage = new Net::TestStage;
				ClientWorker* worker = static_cast< ClientWorker* >( ::Global::GameNet().buildNetwork( false ) );
				auto stageMode = new NetLevelStageMode;
				stageMode->initWorker(worker, NULL);
				stage->setupStageMode(stageMode);
				getManager()->setNextStage( stage );
			}
			break;
		case UI_NET_TEST_SV:
			{
				auto stage = new Net::TestStage;
				ServerWorker* server = static_cast<ServerWorker*>(::Global::GameNet().buildNetwork(true));
				auto stageMode = new NetLevelStageMode;
				stageMode->initWorker(server->createLocalWorker(::Global::GetUserProfile().name), server);
				stage->setupStageMode(stageMode);
				getManager()->setNextStage(stage);

			}
			break;
		case UI_HOLDEM_TEST:
		case UI_FREECELL_TEST:
		case UI_BIG2_TEST:
			{
				IGameModule* game = Global::ModuleManager().changeGame( "Poker" );
				if ( !game )
					return false;
				Poker::GameRule rule;
				switch( id )
				{
				case UI_BIG2_TEST:     rule = Poker::RULE_BIG2; break;
				case UI_FREECELL_TEST: rule = Poker::RULE_FREECELL; break;
				case UI_HOLDEM_TEST:   rule = Poker::RULE_HOLDEM; break;
				}
				static_cast< Poker::GameModule* >( game )->setRule( rule );
				getManager()->changeStage( STAGE_SINGLE_GAME );
			}
			return false;
		case UI_MISC_TEST_GROUP:
		case UI_CARD_GAME_DEV_GROUP:
		case UI_SINGLEPLAYER:
		case UI_MULTIPLAYER:
		case UI_GAME_DEV_GROUP:
		case UI_GAME_DEV2_GROUP:
		case UI_GAME_DEV3_GROUP:
		case UI_GAME_DEV4_GROUP:
		case UI_TEST_GROUP:
		case UI_GRAPHIC_TEST_GROUP:
		case UI_FEATURE_DEV_GROUP:
			pushWidgetGroup( id );
			return false;
		case UI_BACK_GROUP:
			popWidgetGroup();
			return false;
		case UI_VIEW_REPLAY:
			getManager()->changeStage( STAGE_REPLAY_EDIT );
			return false;
		case UI_GAME_OPTION:
			{
				Vec2i pos = ::Global::GUI().calcScreenCenterPos( MainOptionBook::UISize );
				MainOptionBook* book = new MainOptionBook( UI_GAME_OPTION , pos  , NULL );
				::Global::GUI().getManager().addWidget( book );
				book->setTop();
				book->doModal();
			}
			return false;
		}
	}
	else
	{
		ExecutionEntryInfo const* info = nullptr;

		if( id < UI_GROUP_STAGE_INDEX + MAX_NUM_GROUP )
		{
			info = &ExecutionRegisterCollection::Get().getGroupExecutions(mCurGroup)[id - UI_GROUP_STAGE_INDEX];
		}

		if ( info )
		{
			info->execFunc();
			return false;
		}
	}

	return true;
}
