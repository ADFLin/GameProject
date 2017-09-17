#include "TinyGamePCH.h"
#include "MainMenuStage.h"

#include "GameWidget.h"
#include "GameWidgetID.h"

#include "GameRoomUI.h"
#include "GameInstance.h"
#include "GameInstanceManager.h"

#include "NetGameStage.h"

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

	changeGroup( UI_MAIN_GROUP );
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
	page = addPage( LAN("User") );
	page->setRenderCallback( RenderCallBack::Create( this , &MainOptionBook::renderUser ) );
	{
		UserProfile& profile = ::Global::getUserProfile();

		Vec2i uiPos( 130 , 20 );
		Vec2i const uiSize( 150 , 25 );
		Vec2i const off( 0 , 35 );
		GTextCtrl* texCtrl = new GTextCtrl( UI_USER_NAME , uiPos  , uiSize.x , page );
		texCtrl->setValue( profile.name );

		uiPos += off;
		GChoice* choice = new GChoice( UI_LAN_CHHICE , uiPos , uiSize , page );
		choice->appendItem( "�c�餤��" );
		choice->appendItem( "English" );
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
		button->setTitle( LAN("Accept") );
		button = new GButton( UI_SET_DEFULT , buttonPos += bPosOff  , buttonSize , page );
		button->setTitle( LAN("Set Default") );
	}
}

bool MainOptionBook::onChildEvent( int event , int id , GWidget* ui )
{
	switch( id )
	{
	case UI_YES:
		{
			UserProfile& profile = ::Global::getUserProfile();
			profile.name = static_cast< GTextCtrl* >( getCurPage()->findChild( UI_USER_NAME ) )->getValue();
			::Global::GUI().getManager().getModalWidget()->destroy();
		}
		return false;
	}
	return true;
}

void MainOptionBook::renderUser( GWidget* ui )
{
	Vec2i pos = ui->getWorldPos() + Vec2i( 20 , 20 );

	Graphics2D& g = Global::getGraphics2D();
	RenderUtility::setFont( g , FONT_S12 );
	g.drawText( pos , LAN("Player Name") );
	pos.y += 35;
	g.drawText( pos , LAN("Language") );
}

Vec2i const MenuBtnSize(120, 25);
int const MenuDeltaDelay = 100;
int const MenuYOffset = 30;

#define CREATE_BUTTON( ID , NAME )\
	createButton( delay += MenuDeltaDelay , ID  , NAME  , Vec2i( xUI , yUI += MenuYOffset  ) , MenuBtnSize )

void MainMenuStage::doChangeGroup( StageGroupID group )
{
	
	int xUI = ( Global::getDrawEngine()->getScreenWidth() - MenuBtnSize.x ) / 2 ;
	int yUI = 200;
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
		CREATE_BUTTON( UI_SINGLEPLAYER  , LAN("SinglePlayer")  );
		CREATE_BUTTON( UI_MULTIPLAYER   , LAN("MultiPlayer")   );
		CREATE_BUTTON( UI_VIEW_REPLAY   , LAN("View Replay")   );
		//CREATE_BUTTON( UI_ABOUT_GAME    , LAN("About Game")    );
		CREATE_BUTTON( UI_GAME_OPTION   , LAN("Option")        );
		CREATE_BUTTON( UI_EXIT_GAME     , LAN("Exit")          );
		break;
	case UI_MULTIPLAYER:
		CREATE_BUTTON( UI_CREATE_SERVER , LAN("Create Server") );
		CREATE_BUTTON( UI_BUILD_CLIENT  , LAN("Connect Server"));
		CREATE_BUTTON( UI_BACK_GROUP    , LAN("Back")          );
		break;
	case UI_GRAPHIC_TEST_GROUP:
		changeStageGroup(StageRegisterGroup::GraphicsTest);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON( UI_BACK_GROUP     , LAN("Back")          );
		break;
	case UI_TEST_GROUP:
		changeStageGroup(StageRegisterGroup::Test);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON( UI_BACK_GROUP  , LAN("Back")          );
		break;
	case UI_GAME_DEV_GROUP:
		changeStageGroup(StageRegisterGroup::Dev);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON( UI_CARD_GAME_DEV_GROUP  , "Card Game.." );
#if 1
		CREATE_BUTTON(UI_NET_TEST_SV, LAN("Net Test( Server )"));
		CREATE_BUTTON(UI_NET_TEST_CL, LAN("Net Test( Client )"));
#endif
		CREATE_BUTTON( UI_BACK_GROUP     , LAN("Back")          );
		break;
	case UI_GAME_DEV3_GROUP:
		changeStageGroup(StageRegisterGroup::PhyDev);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON(UI_BACK_GROUP, LAN("Back"));
		break;
	case UI_GAME_DEV4_GROUP:
		changeStageGroup(StageRegisterGroup::Dev4);
		createStageGroupButton(delay, xUI, yUI);
		CREATE_BUTTON(UI_BACK_GROUP, LAN("Back"));
		break;
	case UI_GAME_DEV2_GROUP:
		for( int i = 0 ; i < ARRAY_SIZE( gSingleDev ) ; ++i )
		{
			CREATE_BUTTON( UI_SINGLE_DEV_INDEX + i , LAN( gSingleDev[i].decl ) );
		}
		CREATE_BUTTON( UI_BACK_GROUP     , LAN("Back")          );
		break;

	case UI_CARD_GAME_DEV_GROUP:
		CREATE_BUTTON( UI_BIG2_TEST      , "Big2 Test" );
		CREATE_BUTTON( UI_HOLDEM_TEST    , "Holdem Test" );
		CREATE_BUTTON( UI_FREECELL_TEST  , "FreeCell Test" );
		CREATE_BUTTON( UI_BACK_GROUP  , LAN("Back")          );
		break;
	case UI_SINGLEPLAYER:
		{
			GameInstanceVec games;
			Global::GameManager().classifyGame( ATTR_SINGLE_SUPPORT , games );

			for( GameInstanceVec::iterator iter = games.begin() ; 
				 iter != games.end() ; ++iter )
			{
				IGameInstance* g = *iter;
				GWidget* widget = CREATE_BUTTON( UI_GAME_BUTTON , g->getName() );
				widget->setUserData( intptr_t(g) );
			}
			CREATE_BUTTON( UI_BACK_GROUP    , LAN("Back")          );
		}
		break;
	}

}

void MainMenuStage::createStageGroupButton( int& delay , int& xUI , int& yUI )
{
	int idx = 0;
	for( auto info : gStageRegisterCollection.getGroupStage( mCurGroup ) )
	{
		CREATE_BUTTON( UI_GROUP_STAGE_INDEX + idx , info.decl );
		++idx;
	}
}
#undef  CREATE_BUTTON

void MainMenuStage::changeStageGroup(StageRegisterGroup group)
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
				stageMode->initWorker(server->createLocalWorker(::Global::getUserProfile().name), server);
				stage->setupStageMode(stageMode);
				getManager()->setNextStage(stage);

			}
			break;
		case UI_HOLDEM_TEST:
		case UI_FREECELL_TEST:
		case UI_BIG2_TEST:
			{
				IGameInstance* game = Global::GameManager().changeGame( "Poker" );
				if ( !game )
					return false;
				Poker::GameRule rule;
				switch( id )
				{
				case UI_BIG2_TEST:     rule = Poker::RULE_BIG2; break;
				case UI_FREECELL_TEST: rule = Poker::RULE_FREECELL; break;
				case UI_HOLDEM_TEST:   rule = Poker::RULE_HOLDEM; break;
				}
				static_cast< Poker::GameInstance* >( game )->setRule( rule );
				getManager()->changeStage( STAGE_SINGLE_GAME );
			}
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
			pushGroup( id );
			return false;
		case UI_VIEW_REPLAY:
			getManager()->changeStage( STAGE_REPLAY_EDIT );
			return false;
		case UI_BACK_GROUP:
			popGroup();
			return false;
		case UI_GAME_BUTTON:
			{
				IGameInstance* game = reinterpret_cast< IGameInstance* >( ui->getUserData() );
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
		IGameInstance* game = Global::GameManager().changeGame( gSingleDev[ id - UI_SINGLE_DEV_INDEX ].game );
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
			info = &gStageRegisterCollection.getGroupStage(mCurGroup)[id - UI_GROUP_STAGE_INDEX];
		}

		if ( info )
		{
			getManager()->setNextStage( static_cast< StageBase* >( info->factory->create() ) );
			return false;
		}
	}

	return true;
}
