#include "GreedySnakePCH.h"
#include "GreedySnakeGame.h"

#include "GreedySnakeStage.h"
#include "GreedySnakeScene.h"
#include "GreedySnakeMode.h"
#include "GreedySnakeAction.h"

#include "GameWidgetID.h"
#include "GameGUISystem.h"
#include "GameStageMode.h"

#include "CSyncFrameManager.h"

namespace GreedySnake
{
	LevelStage::LevelStage()
	{
		mGameMode = NULL;
	}

	LevelStage::~LevelStage()
	{
		delete mGameMode;
	}

	bool LevelStage::onInit()
	{
		if ( !BaseClass::onInit() )
			return false;

		if ( mGameMode == NULL )
		{
			if( getModeType() == SMT_NET_GAME )
			{
				mGameMode = new BattleMode;
			}
			else
			{
				mGameMode = new SurvivalMode;
			}
		}

		::Global::GUI().cleanupWidget();

		mScene.reset( new Scene( *mGameMode ) );
		getActionProcessor().setLanucher( mScene.get() );
		return true;
	}

	void LevelStage::onEnd()
	{
		BaseClass::onEnd();
	}

	void LevelStage::onRestart(bool beInit)
	{
		if( beInit )
		{

		}
		mScene->restart( beInit );
	}

	void LevelStage::onRender( float dFrame )
	{
		Graphics2D& g = Global::GetGraphics2D();
		mScene->render( g , dFrame );
	}

	void LevelStage::setupScene( IPlayerManager& playerManager )
	{
		mGameMode->setupLevel( playerManager );

		switch( getModeType() )
		{
		case SMT_SINGLE_GAME:
		case SMT_NET_GAME:
			{
				GamePlayer* player = playerManager.getUser();
				getGame()->getController().setPortControl( player->getActionPort() , 0 );
			}
			break;
		}
		
	}

	void LevelStage::tick()
	{
		switch( getGameState() )
		{
		case GameState::Start:
			changeState( GameState::Run );
			break;
		case GameState::Run:
			mScene->tick();
			if ( mScene->isOver() )
				changeState( GameState::End );
			break;
		}
	}

	void LevelStage::updateFrame( int frame )
	{
		mScene->updateFrame( frame );
	}

	bool LevelStage::queryAttribute( GameAttribute& value )
	{
		if ( BaseClass::queryAttribute( value ) )
			return true;

		return false;
	}

	bool LevelStage::setupAttribute( GameAttribute const& value )
	{
		if ( BaseClass::setupAttribute( value ) )
			return true;

		return false;
	}

	IFrameActionTemplate* LevelStage::createActionTemplate( unsigned version )
	{
		switch( version )
		{
		case LAST_VERSION: return new CFrameAcionTemplate( mScene );
		}
		return NULL;
	}

	void LevelStage::onChangeState( GameState state )
	{
		switch ( state )
		{
		case GameState::End:
			switch( getModeType() )
			{
			case SMT_SINGLE_GAME:
				::Global::GUI().showMessageBox( 
					UI_RESTART_GAME , LOCTEXT("Do You Want To Play Game Again ?") );
				break;
			}
			break;
		}
	}

	bool LevelStage::onWidgetEvent( int event , int id , GWidget* ui )
	{
		switch ( id )
		{
		case UI_RESTART_GAME:
			if ( event == EVT_BOX_NO )
			{
				if ( getModeType() == SMT_SINGLE_GAME )
					getManager()->changeStage( STAGE_MAIN_MENU );
			}
			break;
		}

		return BaseClass::onWidgetEvent( event , id , ui );
	}


	bool LevelStage::setupNetwork( NetWorker* netWorker , INetEngine** engine )
	{
		IFrameActionTemplate* actionTemplate = createActionTemplate( LAST_VERSION );
		INetFrameManager* netFrameMgr;
		if ( netWorker->isServer() )
		{
			ServerFrameCollector* frameCollector = new ServerFrameCollector( gMaxPlayerNum );
			netFrameMgr = new SVSyncFrameManager( netWorker , actionTemplate , frameCollector );
		}
		else
		{
			ClientFrameCollector* frameCollector = new ClientFrameCollector;
			netFrameMgr = new CLSyncFrameManager( netWorker , actionTemplate , frameCollector );
		}
		*engine = new CFrameActionEngine( netFrameMgr );
		return true;
	}

	bool LevelStage::onKey(KeyMsg const& msg)
	{
		if( !BaseClass::onKey(msg) )
			return false;

		if (msg.isDown())
		{
			switch(msg.getCode())
			{
			case EKeyCode::R:
				getStageMode()->restart(false);
				Global::Debug().clearDebugMsg();
				break;
			default:
				break;
			}
		}

		return true;
	}

	void LevelStage::setupLocalGame(LocalPlayerManager& playerManager)
	{
		playerManager.createPlayer( 0 );
		playerManager.setUserID( 0 );
	}

}
