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

#include "EasingFun.h"

#include <cmath>

extern void RegisterStageGlobal();

GameStageInfo gSingleDev[] = 
{
	{ "Carcassonne Pototype" , "Carcassonne" } ,
	{ "TD Test" , "TowerDefend" } ,
	{ "Rich Test" , "Rich" } ,
	{ "BomberMan Test" , "BomberMan" } ,
	{ "Bubble Test" , "Bubble" } ,
	{ "CubeWorld Test" , "CubeWorld" } ,
};


bool MainMenuStage::onInit()
{
	if ( !BaseClass::onInit() )
		return false;

	RegisterStageGlobal();

	getManager()->setTickTime( gDefaultTickTime );

	::Global::GUI().cleanupWidget();

#if 0
	GButton* button = new GButton( UI_ANY , Vec2i(100 ,20 ) , Vec2i(100 ,20 )  , NULL );

	::Global::getGUI().getTweens()
		.tween< Easing::CLinear , WidgetSize >( *button , Vec2i(100 ,20 )  , Vec2i(200 , 40 )  , 2000 , 0 )
		.repeat( 10 ).repeatDelay( 10 );

	button->setTitle( "Test" );

	::Global::getGUI().getManager().addUI( button );
#endif

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
	int xUI = ( Global::GetDrawEngine().getScreenWidth() - MenuBtnSize.x ) / 2 ;
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
		changeStageGroup(EStageGroup::Main);
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
		changeStageGroup(EStageGroup::GraphicsTest);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON( UI_BACK_GROUP     , LOCTEXT("Back")          );
		break;
	case UI_TEST_GROUP:
		changeStageGroup(EStageGroup::Test);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON( UI_BACK_GROUP  , LOCTEXT("Back")          );
		break;
	case UI_GAME_DEV_GROUP:
		changeStageGroup(EStageGroup::Dev);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON( UI_CARD_GAME_DEV_GROUP  , "Card Game.." );
#if 1
		CREATE_BUTTON(UI_NET_TEST_SV, LOCTEXT("Net Test( Server )"));
		CREATE_BUTTON(UI_NET_TEST_CL, LOCTEXT("Net Test( Client )"));
#endif
		CREATE_BUTTON( UI_BACK_GROUP     , LOCTEXT("Back")          );
		break;
	case UI_GAME_DEV3_GROUP:
		changeStageGroup(EStageGroup::PhyDev);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON(UI_BACK_GROUP, LOCTEXT("Back"));
		break;
	case UI_GAME_DEV4_GROUP:
		changeStageGroup(EStageGroup::Dev4);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON(UI_BACK_GROUP, LOCTEXT("Back"));
		break;
	case UI_FEATURE_DEV_GROUP:
		changeStageGroup(EStageGroup::FeatureDev);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON(UI_BACK_GROUP, LOCTEXT("Back"));
		break;
	case UI_GAME_DEV2_GROUP:
		for( int i = 0 ; i < ARRAY_SIZE( gSingleDev ) ; ++i )
		{
			CREATE_BUTTON( UI_SINGLE_DEV_INDEX + i , LOCTEXT( gSingleDev[i].title ) );
		}
		CREATE_BUTTON( UI_BACK_GROUP     , LOCTEXT("Back")          );
		break;


	case UI_CARD_GAME_DEV_GROUP:
		CREATE_BUTTON( UI_BIG2_TEST      , "Big2 Test" );
		CREATE_BUTTON( UI_HOLDEM_TEST    , "Holdem Test" );
		CREATE_BUTTON( UI_FREECELL_TEST  , "FreeCell Test" );
		CREATE_BUTTON( UI_BACK_GROUP  , LOCTEXT("Back")          );
		break;
	case UI_SINGLEPLAYER:
		{
			GameModuleVec games;
			Global::GameManager().classifyGame( ATTR_SINGLE_SUPPORT , games );

			for( GameModuleVec::iterator iter = games.begin() ; 
				 iter != games.end() ; ++iter )
			{
				IGameModule* g = *iter;
				GWidget* widget = CREATE_BUTTON( UI_GAME_BUTTON , g->getName() );
				widget->setUserData( intptr_t(g) );
			}
			CREATE_BUTTON( UI_BACK_GROUP    , LOCTEXT("Back")          );
		}
		break;
	}

}

void MainMenuStage::createStageGroupButton( int& delay , int& xUI , int& yUI )
{
	int idx = 0;
	int numStage = StageRegisterCollection::Get().getGroupStage(mCurGroup).size();

	if( numStage > 10 )
	{
		int PosLX = xUI - (MenuBtnSize.x + 6) / 2;
		int PosRX = xUI + (MenuBtnSize.x + 6) / 2;
		for( auto info : StageRegisterCollection::Get().getGroupStage(mCurGroup) )
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
		for( auto info : StageRegisterCollection::Get().getGroupStage(mCurGroup) )
		{
			CREATE_BUTTON(UI_GROUP_STAGE_INDEX + idx, info.title);
			++idx;
		}
	}
}
#undef  CREATE_BUTTON

void MainMenuStage::changeStageGroup(EStageGroup group)
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
				IGameModule* game = Global::GameManager().changeGame( "Poker" );
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

			return false;
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
		case UI_GAME_BUTTON:
			{
				IGameModule* game = reinterpret_cast< IGameModule* >( ui->getUserData() );
				Global::GameManager().changeGame( game->getName() );
				game->beginPlay( SMT_SINGLE_GAME , *getManager() );
			}
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
	else if ( id < UI_SINGLE_DEV_INDEX + MAX_NUM_GROUP )
	{
		IGameModule* game = Global::GameManager().changeGame( gSingleDev[ id - UI_SINGLE_DEV_INDEX ].game );
		if ( !game )
			return false;
		game->beginPlay( SMT_SINGLE_GAME , *getManager() );
		return false;
	}
	else
	{
		StageInfo const* info = nullptr;

		if( id < UI_GROUP_STAGE_INDEX + MAX_NUM_GROUP )
		{
			info = &StageRegisterCollection::Get().getGroupStage(mCurGroup)[id - UI_GROUP_STAGE_INDEX];
		}

		if ( info )
		{
			getManager()->setNextStage( static_cast< StageBase* >( info->factory->create() ) );
			return false;
		}
	}

	return true;
}
