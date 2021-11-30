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
			if( getModeType() == EGameStageMode::Net )
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
		case EGameStageMode::Single:
		case EGameStageMode::Net:
			{
				GamePlayer* player = playerManager.getUser();
				getGame()->getInputControl().setPortControl( player->getActionPort() , 0 );
			}
			break;
		}
		
	}

	void LevelStage::tick()
	{
		switch( getGameState() )
		{
		case EGameState::Start:
			changeState( EGameState::Run );
			break;
		case EGameState::Run:
			mScene->tick();
			if ( mScene->isOver() )
				changeState( EGameState::End );
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

	void LevelStage::onChangeState( EGameState state )
	{
		switch ( state )
		{
		case EGameState::End:
			switch( getModeType() )
			{
			case EGameStageMode::Single:
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
				if ( getModeType() == EGameStageMode::Single )
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
