#include "TinyGamePCH.h"
#include "MainMenuStage.h"

#include "GameWidget.h"
#include "GameWidgetID.h"

#include "GameRoomUI.h"
#include "GamePackage.h"
#include "GamePackageManager.h"

#include "GameSingleStage.h"
#include "NetGameStage.h"

#include "RenderUtility.h"
#include "Localization.h"

#include "Poker/PokerGame.h"
#include "Go/GoStage.h"
#include "TripleTown/TTStage.h"
#include "Shoot2D/FlightGame.h"
#include "Bejeweled/BJStage.h"
#include "MonumentValley/MVStage.h"

#include "LightingStage.h"
#include "TestStage.h"
#include "Phy2DStage.h"
#include "BloxorzStage.h"
#include "AStarStage.h"
#include "FBirdStage.h"
#include "PlanetStage.h"

#include "EasingFun.h"

#include <cmath>

class IFactory
{
public:
	virtual void* create() = 0;
};


template< class T >
class CFactory : public IFactory
{
public:
	void* create(){ return new T; }
};

template< class T >
static IFactory* makeFactory()
{
	static CFactory< T > myFactory;
	return &myFactory;
}

struct StageInfo
{
	char const*    decl;
	IFactory* factory;
};

#define INFO( DECL , CLASS )\
	{ DECL , makeFactory< CLASS >() } 

StageInfo gGraphicsTestGroup[] = 
{
	INFO( "2D Lighting Test"     , Lighting::TestStage ) ,
	INFO( "Shader Test"  , GS::TestStage ), 

	INFO( "GLGraphics2D Test"   , GLGraphics2DTestStage ) ,
};

StageInfo gTestGroup[] =
{
	INFO( "Bsp Test"    , Bsp2D::TestStage ) ,
	INFO( "A-Star Test" , AStar::TestStage ) ,
	INFO( "SAT Col Test"    , G2D::TestStage ) ,
	INFO( "GJK Col Test" , Phy2D::CollideTestStage ) ,
	INFO( "QHull Test"   , G2D::QHullTestStage ) ,
	INFO( "RB Tree Test"   , TreeTestStage )  ,
	INFO( "Tween Test"  , TweenTestStage ) ,

#if 1
	INFO( "TileMap Test"  , TileMapTestStage ), 
	INFO( "Corontine Test" , CoroutineTestStage ) ,
	INFO( "SIMD Test" , SIMD::TestStage ) ,
	INFO( "XML Prase Test" , XMLPraseTestStage ) ,
#endif
};

StageInfo gPhyDevGroup[] =
{	
	INFO( "RigidBody Test" , Phy2D::WorldTestStage ) ,	
};

StageInfo gDevGroup[] =
{
	INFO( "Monument Valley" , MV::TestStage ) ,
	INFO( "Triple Town Test"  , TripleTown::LevelStage ) ,
	INFO( "Bejeweled Test"    , Bejeweled::TestStage ) ,
	INFO( "Bloxorz Test"      , Bloxorz::TestStage ), 
	INFO( "FlappyBird Test"   , FlappyBird::TestStage ) , 
	
};

StageInfo gDev4Group[] =
{
	INFO( "Mario Test"        , Mario::TestStage ) ,
	INFO( "Shoot2D Test"      , Shoot2D::TestStage ) ,
	INFO( "Go Test"           , Go::Stage ) ,
};

#undef INFO

struct GameStageInfo
{
	char const* decl;
	char const* game;
};

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

	getManager()->setTickTime( gDefaultTickTime );

	::Global::getGUI().cleanupWidget();

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
	page->setRenderCallback( RenderCallBack::create( this , &MainOptionBook::renderUser ) );
	{
		UserProfile& profile = ::Global::getGUI().getUserProfile();

		Vec2i uiPos( 130 , 20 );
		Vec2i const uiSize( 150 , 25 );
		Vec2i const off( 0 , 35 );
		GTextCtrl* texCtrl = new GTextCtrl( UI_USER_NAME , uiPos  , uiSize.x , page );
		texCtrl->setValue( profile.name );

		uiPos += off;
		GChoice* choice = new GChoice( UI_LAN_CHHICE , uiPos , uiSize , page );
		choice->appendItem( "ÁcÅé¤¤¤å" );
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
			UserProfile& profile = ::Global::getGUI().getUserProfile();
			profile.name = static_cast< GTextCtrl* >( getCurPage()->findChild( UI_USER_NAME ) )->getValue();
			::Global::getGUI().getManager().getModalUI()->destroy();
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


void MainMenuStage::doChangeGroup( StageGroupID group )
{
	Vec2i btnSize( 120 , 25 );
	int xUi = ( Global::getDrawEngine()->getScreenWidth() - btnSize.x ) / 2 ;
	int yUi = 200;
	int offset = 30;

	int delay = 0;
	int dDelay = 100;

	fadeoutGroup( dDelay );
	mGroupUI.clear();

#define CREATE_BUTTON( ID , NAME )\
	createButton( delay += dDelay , ID  , NAME  , Vec2i( xUi , yUi += offset  ) , btnSize )

	switch( group )
	{
	case UI_MAIN_GROUP:
		CREATE_BUTTON( UI_GAME_DEV_GROUP  , "Game Dev"   );
		CREATE_BUTTON( UI_GAME_DEV2_GROUP  , "Game Dev2"   );
		CREATE_BUTTON( UI_GAME_DEV3_GROUP  , "Phyics Test"   );
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
		for( int i = 0 ; i < ARRAY_SIZE( gGraphicsTestGroup ) ; ++i )
		{
			CREATE_BUTTON( UI_GRAPHIC_TEST_INDEX + i , LAN( gGraphicsTestGroup[i].decl ) );
		}
		CREATE_BUTTON( UI_BACK_GROUP     , LAN("Back")          );
		break;
	case UI_TEST_GROUP:
		for( int i = 0 ; i < ARRAY_SIZE( gTestGroup ) ; ++i )
		{
			CREATE_BUTTON( UI_TEST_INDEX + i , LAN( gTestGroup[i].decl ) );
		}
#if 0
		CREATE_BUTTON( UI_NET_TEST_SV , LAN("Net Test( Server )") );
		CREATE_BUTTON( UI_NET_TEST_CL , LAN("Net Test( Client )") );
#endif
		CREATE_BUTTON( UI_BACK_GROUP  , LAN("Back")          );
		break;
	case UI_GAME_DEV_GROUP:
		for( int i = 0 ; i < ARRAY_SIZE( gDevGroup ) ; ++i )
		{
			CREATE_BUTTON( UI_GAME_DEV_INDEX + i , LAN( gDevGroup[i].decl ) );
		}

		CREATE_BUTTON( UI_CARD_GAME_DEV_GROUP  , "Card Game.." );
		CREATE_BUTTON( UI_BACK_GROUP     , LAN("Back")          );
		break;

	case UI_GAME_DEV2_GROUP:
		for( int i = 0 ; i < ARRAY_SIZE( gSingleDev ) ; ++i )
		{
			CREATE_BUTTON( UI_SINGLE_DEV_INDEX + i , LAN( gSingleDev[i].decl ) );
		}
		CREATE_BUTTON( UI_BACK_GROUP     , LAN("Back")          );
		break;
	case UI_GAME_DEV3_GROUP:
		for( int i = 0 ; i < ARRAY_SIZE( gPhyDevGroup ) ; ++i )
		{
			CREATE_BUTTON( UI_GAME_DEV3_INDEX + i , LAN( gPhyDevGroup[i].decl ) );
		}
		CREATE_BUTTON( UI_BACK_GROUP     , LAN("Back")          );
		break;
	case UI_GAME_DEV4_GROUP:
		for( int i = 0 ; i < ARRAY_SIZE( gDev4Group ) ; ++i )
		{
			CREATE_BUTTON( UI_GAME_DEV4_INDEX + i , LAN( gDev4Group[i].decl ) );
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
			GamePackageVec games;
			Global::getGameManager().classifyGame( ATTR_SINGLE_SUPPORT , games );

			for( GamePackageVec::iterator iter = games.begin() ; 
				 iter != games.end() ; ++iter )
			{
				IGamePackage* g = *iter;
				GWidget* widget = CREATE_BUTTON( UI_GAME_BUTTON , g->getName() );
				widget->setUserData( intptr_t(g) );
			}
			CREATE_BUTTON( UI_BACK_GROUP    , LAN("Back")          );
		}
		break;
	}
#undef  CREATE_BUTTON
}

bool MainMenuStage::onWidgetEvent( int event , int id , GWidget* ui )
{
	if ( id < UI_GROUP_INDEX  )
	{
		switch ( id )
		{
		case UI_NET_TEST_CL:
			{
				GameNetLevelStage* stage = new GameNetLevelStage;
				ClientWorker* worker = static_cast< ClientWorker* >( getManager()->buildNetwork( false ) );
				stage->setSubStage( new Net::TestStage );
				stage->initWorker( worker , NULL );
				getManager()->setNextStage( stage );
			}
			break;
		case UI_NET_TEST_SV:
			{
				GameNetLevelStage* stage = new GameNetLevelStage;
				ServerWorker* server = static_cast< ServerWorker* >( getManager()->buildNetwork( true ) );
				stage->setSubStage( new Net::TestStage );
				stage->initWorker( server->createLocalWorker( getManager()->getUserProfile() ) , server );
				getManager()->setNextStage( stage );
			}
			break;
		case UI_HOLDEM_TEST:
		case UI_FREECELL_TEST:
		case UI_BIG2_TEST:
			{
				IGamePackage* game = Global::getGameManager().changeGame( "Poker" );
				if ( !game )
					return false;
				Poker::GameRule rule;
				switch( id )
				{
				case UI_BIG2_TEST:     rule = Poker::RULE_BIG2; break;
				case UI_FREECELL_TEST: rule = Poker::RULE_FREECELL; break;
				case UI_HOLDEM_TEST:   rule = Poker::RULE_HOLDEM; break;
				}
				static_cast< Poker::CGamePackage* >( game )->setRule( rule );
				getManager()->changeStage( STAGE_SINGLE_GAME );
			}
			return false;
		case UI_CARD_GAME_DEV_GROUP:
		case UI_SINGLEPLAYER:
		case UI_MULTIPLAYER:
		case UI_GAME_DEV_GROUP:
		case UI_GAME_DEV2_GROUP:
		case UI_GAME_DEV3_GROUP:
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
				IGamePackage* game = reinterpret_cast< IGamePackage* >( ui->getUserData() );
				Global::getGameManager().changeGame( game->getName() );
				game->beginPlay( GT_SINGLE_GAME , *getManager() );
			}
			return false;
		case UI_GAME_OPTION:
			{
				Vec2i pos = ::Global::getGUI().calcScreenCenterPos( MainOptionBook::UISize );
				MainOptionBook* book = new MainOptionBook( UI_GAME_OPTION , pos  , NULL );
				::Global::getGUI().getManager().addUI( book );
				book->setTop();
				book->doModal();
			}
			return false;
		}
	}
	else if ( id < UI_SINGLE_DEV_INDEX + MAX_NUM_GROUP )
	{
		IGamePackage* game = Global::getGameManager().changeGame( gSingleDev[ id - UI_SINGLE_DEV_INDEX ].game );
		if ( !game )
			return false;
		game->beginPlay( GT_SINGLE_GAME , *getManager() );
		getManager()->changeStage( STAGE_SINGLE_GAME );
		return false;
	}
	else
	{
		StageInfo const* info = nullptr;

		if ( id < UI_GAME_DEV_INDEX + MAX_NUM_GROUP )
		{
			info = &gDevGroup[ id - UI_GAME_DEV_INDEX ];
		}
		else if ( id < UI_GAME_DEV3_INDEX + MAX_NUM_GROUP )
		{
			info = &gPhyDevGroup[ id - UI_GAME_DEV3_INDEX ];
		}
		else if ( id < UI_GAME_DEV4_INDEX + MAX_NUM_GROUP )
		{
			info = &gDev4Group[ id - UI_GAME_DEV4_INDEX ];
		}
		else if ( id < UI_TEST_INDEX + MAX_NUM_GROUP )
		{
			info = &gTestGroup[ id - UI_TEST_INDEX ];
		}
		else if ( id < UI_GRAPHIC_TEST_INDEX + MAX_NUM_GROUP )
		{
			info = &gGraphicsTestGroup[ id - UI_GRAPHIC_TEST_INDEX ];
		}

		if ( info )
		{
			getManager()->setNextStage( static_cast< StageBase* >( info->factory->create() ) );
			return false;
		}
	}

	return true;
}
