#include "ZumaPCH.h"
#include "ZumaGame.h"

#include "ResID.h"
#include "ZEvent.h"
#include "ZFont.h"
#include "ZBallConGroup.h"
#include "ZLevelManager.h"
#include "ZDraw.h"

#include "ZLevelStage.h"

#include <fstream>
#include <sstream>

#define RES_XML_PATH    "properties/resources.xml"
#define LEVEL_XML_PATH "levels/levels.xml"

namespace Zuma
{

	int DBG_X = 0;
	int DBG_Y = 0;

	GameTime g_GameTime;

	static IRenderSystem* sRenderSystem = NULL;

	GameCore* gGame = NULL;
	IRenderSystem& Global::getRenderSystem()
	{
		return *sRenderSystem;
	}

	ResManager& Global::getResManager()
	{
		return gGame->resManager;
	}

	GameCore::GameCore() 
		:logRecorder("log.txt")
	{
		gGame = this;
		sRenderSystem = NULL;

		renderFrame = 0;
		showFPS     = 0;

		mLevelMode = NULL;
		isGmaePause = false;
	}


	GameCore::~GameCore()
	{
		if ( sRenderSystem )
		{
			delete sRenderSystem;
			sRenderSystem = NULL;
		}
	}

	bool GameCore::init( GameInitialer& initilizer )
	{
		if ( !initilizer.setupWindow( "Zuma Clone" , g_ScreenWidth , g_ScreenHeight ) )
			return false;

		sRenderSystem = initilizer.createRenderSystem();

		if ( !sRenderSystem )
			return false;

		if ( !ZRenderUtility::init(  ) )
			return false;

		if ( !audioPlayer.init() )
			return false;

		if ( !uiSystem.init( *sRenderSystem ) )
			return false;

		uiSystem.setParentHandler( this );

		if (!resManager.init( RES_XML_PATH ) )
			return false;
		resManager.setResLoader( RES_IMAGE , sRenderSystem );
		resManager.setResLoader( RES_FONT  , sRenderSystem );
		resManager.setResLoader( RES_SOUND , &audioPlayer );

		if ( !ZLevelManager::getInstance().init( LEVEL_XML_PATH ) )
			return false;

		mSetting.setDefault();

		g_GameTime.curTime    = 0;
		g_GameTime.updateTime = 20;
		g_GameTime.nextTime   = g_GameTime.curTime + g_GameTime.updateTime;

		resManager.load( "Init" );
		resManager.load( "MainMenu" );
		resManager.load( "Register" );
		
		return true;
	}

	void GameCore::cleanup()
	{
		TaskHandler::clearTask();

		resManager.cleanup();
		audioPlayer.cleunup();
	}


	void GameCore::render( float dFrame )
	{
		IRenderSystem& renderSys = Global::getRenderSystem();

		if ( !renderSys.beginRender() )
			return;

		getCurStage()->onRender( renderSys );

		renderSys.setCurOrder( RO_UI );

		uiSystem.render();

		static long beTime = GetTickCount();

		if ( renderFrame > 1000 )
		{
			showFPS = 1000.0f *float( renderFrame ) / ( GetTickCount() - beTime );

			beTime = GetTickCount();
			renderFrame = 0;
		}

		ZFont* font;

		font = renderSys.getFontRes( FONT_BROWNTITLE );

		if ( !font )
			font = renderSys.getFontRes( FONT_PLAIN );

		if ( font )
		{
			char str[128];

			renderSys.setColor( 255 , 255 , 125 , 255 );

			sprintf( str , "FPS=%.2f" , showFPS );
			font->print( Vec2D( g_ScreenWidth - 100 , 10 ) ,
				str , FF_ALIGN_VCENTER | FF_ALIGN_HCENTER );

			sprintf( str , "x:%d,y:%d" , DBG_X , DBG_Y );
			font->print( Vec2D( g_ScreenWidth - 100 , 35 ) ,
				str , FF_ALIGN_VCENTER | FF_ALIGN_HCENTER );

			renderSys.setColor( 255 , 255 , 255 , 255 );
		}

		++renderFrame;

		renderSys.endRender();
	}

	void GameCore::update( long time )
	{
		g_GameTime.curTime += time;
		g_GameTime.nextTime = g_GameTime.curTime + g_GameTime.updateTime;

		audioPlayer.update( time );
		TaskHandler::runTask( time );
		ZStageController::update( time );
	}

	void GameCore::onStageEnd( ZStage* stage , unsigned flag )
	{
		if ( ( flag & SF_DONT_CLEAR_UI ) == 0 )
			getUISystem()->cleanupWidgets();
		if ( ( flag & SF_DONT_CLEAR_TASK ) == 0 )
			clearTask();
	}

	bool GameCore::onEvent( int evtID , ZEventData const& data , ZEventHandler* sender )
	{
		IRenderSystem& renderSys = Global::getRenderSystem();

		static unsigned char const baseColor[ zColorNum ][3] =
		{
			100 , 100 , 0,
			0 , 0 , 100,
			0 , 100 , 100 ,
			100, 0, 100,
			20 , 20 , 20,
			0 , 0 , 0
		};

		switch ( evtID )
		{
		case EVT_UI_SLIDER_CHANGE:
		case EVT_UI_BUTTON_CLICK:
			{
				ZWidget* ui = data.getPtr< ZWidget >();

				if ( getCurStage()->onUIEvent( evtID ,ui->getUIID(), ui ) )
					onUIEvent( evtID , ui->getUIID() ,  ui );
			}
			break;
		case EVT_FROG_SWAP_BALL:
			//playSound( SOUND_BUTTON2 );
			break;
		case EVT_BALL_CONNECT:
			playSound( SOUND_BALLCLICK1 );
			break;
		case EVT_INSERT_BALL:
			playSound( SOUND_BALLCLICK2 );
			break;
		case EVT_FIRE_BALL:
			playSound( SOUND_BALLFIRE );
			break;
		case EVT_GROUP_COMBO:
			{
				ZComboInfo* info = data.getPtr< ZComboInfo >();
				showComboText( *info );
				playSound( SOUND_COMBO , 0.5 , false , 1 + 0.05f * info->numComBoKeep  );
			}
			break;
		case EVT_BALL_CON_DESTROY:
			{
				ZConBall* ball = data.getPtr< ZConBall >();
				//BallPS* ps = new BallPS;

				//ps->baseColor.r = baseColor[ ball->getColor()][0];
				//ps->baseColor.g = baseColor[ ball->getColor()][1];
				//ps->baseColor.b = baseColor[ ball->getColor()][2];
				//ps->tex       =  RDSystem.getTexture( IMAGE_GRAY_EXPLOSION );
				//ps->getEmitter().setPos( ball->getPos() );
				//addTask( new PSTask( this , ps , ps->getLifeTime() ) );
			}
			break;
		case EVT_UPDATE_MOVE_BALL:
			{
				//ZMoveBall* ball = (ZMoveBall*) data;
				//if ( ball == &mvBallList.front() )
				//{
				//	samplePS.getEmitter().setPos( ball->getPos() );
				//	samplePS.emitDir = -ball->getSpeed();
				//	samplePS.enable( true );
				//}
			}
			break;
		case EVT_ADD_TASK:
			addTask( data.getPtr< TaskBase >() );
			break;
		case EVT_LEVEL_END:
			switch( mLevelMode->getCurLevelFlag() )
			{
			case MS_ADV_NEXT_LEVEL:
				{
					LevelStage* lvStage = mLevelMode->createNextLevelStage();
					lvStage->showQuake( &data.getPtr< LevelStage>()->getLevel().getInfo() );
					changeLevel( lvStage );
				}
				break;
			case MS_ADV_GROUP_FINISH:
				changeLevel( mLevelMode->createNextLevelStage() );
				break;
			}
			break;

		case EVT_LEVEL_CHANGE_NEXT:
			changeLevel( mLevelMode->createNextLevelStage() );
			break;
		case EVT_LOADING_FINISH:
			{
				ZMainMenuStage* stage = new ZMainMenuStage;
				changeStage( stage );
			}
			break;
		case EVT_EXIT_DEV_STAGE:
			if ( mLevelMode )
			{
				LevelStage* stage = mLevelMode->createCurLevelStage();
				changeLevel( stage );
			}
			else
			{
				changeStage( new ZMainMenuStage );
			}
			break;
		case EVT_CHANGE_DEV_STAGE:
			{
				ZLevelInfo* info = data.getPtr<ZLevelInfo>();
				changeStage( new ZDevStage( *info ) );
			}
			break;

		}
		return true;
	}

	bool GameCore::onUIEvent( int evtID , int uiID , ZWidget* ui )
	{
		switch ( uiID )
		{
		case UI_ADVENTLIRE:
			{
				mUserData.unlockStage = 2;
				changeStage( new AdvMenuStage(mUserData) );
			}
			break;
		case UI_PLAY_STAGE:
			{
				unsigned stageID =  ZAdvButton::selectStageID;
				if ( stageID != 0 )
				{
					mLevelMode = new ZAdventureMode( stageID , 0 );
					LevelStage* stage = mLevelMode->createCurLevelStage();
					changeLevel( stage );
				}
			}
			break;
		case UI_SOUND_VOLUME:
			{
				ZSlider* slider = GameUI::castFast< ZSlider* >( ui );
				audioPlayer.setSoundVolume( slider->getValue() / 100.0f );
				static unsigned timePlaySound = 0;
				if ( g_GameTime.curTime - timePlaySound > 300 )
				{
					playSound( SOUND_COMBO );
					timePlaySound = g_GameTime.curTime;
				}
			}
			break;
		case UI_OPTIONS_BUTTON:
			if ( !getUISystem()->findUI( UI_OPTIONS_PANEL ) )
			{
				ZPanel* panel = new ZSettingPanel( UI_OPTIONS_PANEL , Vec2i(100,100) , NULL );
				getUISystem()->addWidget( panel );
			}
			break;
		case UI_MAIN_MENU:
			changeStage( new ZMainMenuStage );
			break;
		case UI_DONE:
			ui->getParent()->destroy();
			break;
		case UI_QUIT_GAME:
			break;
		}
		return true;
	}

	void GameCore::applySetting()
	{
		audioPlayer.setSoundVolume( mSetting.soundVolume );
		audioPlayer.setMusicVolume( mSetting.musicVolume );
	}



	void GameCore::changeLevel( LevelStage* stage )
	{
		ZLevel& level = stage->getLevel();

		level.getInfo().loadTexture();
		changeStage( stage );
	}

	void GameCore::showComboText( ZComboInfo& info )
	{
		IRenderSystem& renderSys = Global::getRenderSystem();

		ShowTextTask* task[ 32 ];
		int           numTask = 0;

		assert( info.numTool < 30 );

		unsigned color = 0;
		switch ( info.color )
		{
		case zBlue  : color = 0xffff0000; break;
		case zYellow: color = 0xff00ffff; break;
		case zRed   : color = 0xff0000ff; break;
		case zGreen : color = 0xff00ff00; break;
		case zPurple: color = 0xffff0000; break;
		case zWhite : color = 0xffffffff; break;
		}

		char str[64];
		sprintf( str , "+%d" , info.score );

		task[ numTask ] = new ShowTextTask( str , FONT_FLOAT , 2000 );
		++numTask;

		if ( info.numComBoKeep )
		{
			sprintf( str , "Combo X %d" , info.numComBoKeep );
			task[ numTask ] = new ShowTextTask( str , FONT_FLOAT , 2000 );
			++numTask;
		}

		for ( int i = 0 ; i < info.numTool ; ++i )
		{
			char const* pStr = "Error Tool";
			switch ( info.tool[ i ] )
			{
			case TOOL_BACKWARDS: pStr = "BACKWARDS BALL"; break;
			case TOOL_BOMB:      pStr = "BOMB BALL";      break;
			case TOOL_ACCURACY:  pStr = "ACCURACY BALL";  break;
			case TOOL_SLOW:      pStr = "SLOW BALL";      break;
			}
			task[ numTask ] = new ShowTextTask( pStr , FONT_FLOAT , 2000 );
			++numTask;
		}

		Vec2D textPos = info.pos;

		for ( int i = 0 ; i < numTask ; ++i )
		{
			textPos += Vec2D( 0 , 20 );
			task[i]->pos   = textPos;
			task[i]->speed = Vec2D( 0 , -20.0f / 1000.0f );
			task[i]->color.value = color;
			task[i]->setOrder( LRO_TEXT );

			addTask( task[i] );
		}
	}

	bool GameCore::onMouse( MouseMsg const& msg )
	{
		if ( !uiSystem.procMouseMsg( msg ) )
			return true;

		getCurStage()->onMouse( msg );
		return true;

	}

	bool GameCore::onKey( unsigned key , bool isDown )
	{
		getCurStage()->onKey( key , isDown );

		switch ( key )
		{
		case VK_LEFT  : DBG_X -= 1; break;
		case VK_DOWN  : DBG_Y -= 1; break;
		case VK_RIGHT : DBG_X += 1; break;
		case VK_UP    : DBG_Y += 1; break;
			//case VK_F1    : toggleFullscreen(); break;
		}
		return true;
	}

	LevelStage* ZAdventureMode::createLevelStage( unsigned idStage , int idxLv )
	{
		if ( mIdStage != idStage )
		{
			ZLevelManager::getInstance().loadStage( idStage , mLvList );
		}

		if ( idxLv >= getLevelNum() )
		{
			++idStage;
			ZLevelManager::getInstance().loadStage( idStage , mLvList );
			idxLv = 0;
		}

		ZLevelInfo& info = getLevelInfo( idxLv );
		LevelStage* stage = new LevelStage( info );

		char str[32];
		sprintf( str , "Level %d-%d" , idStage , idxLv + 1 );
		stage->setTitle( str );

		mIdStage = idStage;
		mIdxLv   = idxLv;

		return stage;
	}

	ModeState ZAdventureMode::getCurLevelFlag()
	{
		if ( mIdxLv >= getLevelNum() )
			return MS_ADV_GROUP_FINISH;
		return MS_ADV_NEXT_LEVEL;
	}

	ZAdventureMode::ZAdventureMode( unsigned idStage , int idxLv )
	{
		ZLevelManager::getInstance().loadStage( idStage , mLvList );
		mIdStage = idStage;
		mIdxLv   = idxLv;
	}


	bool ZumaGame::onInit()
	{
		if ( !GameCore::init( *this ) )
			return false;
		LoadingStage* stage = new LoadingStage( "LoadingThread" );
		changeStage( stage );
		return true;
	}

}//namespace Zuma
