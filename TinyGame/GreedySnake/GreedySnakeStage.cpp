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

	void LevelStage::onRestart( uint64 seed , bool beInit )
	{
		if( beInit )
		{
			::Msg("Seed = %ld", seed);
			Global::RandSeedNet(seed);
		}
		mScene->restart( beInit );
	}

	void LevelStage::onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();
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
		case GS_START:
			changeState( GS_RUN );
			break;
		case GS_RUN:
			mScene->tick();
			if ( mScene->isOver() )
				changeState( GS_END );
			break;
		}
	}

	void LevelStage::updateFrame( int frame )
	{
		mScene->updateFrame( frame );
	}

	bool LevelStage::getAttribValue( AttribValue& value )
	{
		if ( BaseClass::getAttribValue( value ) )
			return true;

		return false;
	}

	bool LevelStage::setupAttrib( AttribValue const& value )
	{
		if ( BaseClass::setupAttrib( value ) )
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
		case GS_END:
			switch( getModeType() )
			{
			case SMT_SINGLE_GAME:
				::Global::GUI().showMessageBox( 
					UI_RESTART_GAME , "Do You Want To Play Game Again ?" );
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
			ServerFrameGenerator* frameGenerator = new ServerFrameGenerator( gMaxPlayerNum );
			netFrameMgr = new SVSyncFrameManager( netWorker , actionTemplate , frameGenerator );
		}
		else
		{
			ClientFrameGenerator* frameGenerator = new ClientFrameGenerator;
			netFrameMgr = new CLSyncFrameManager( netWorker , actionTemplate , frameGenerator );
		}
		*engine = new CFrameActionEngine( netFrameMgr );
		return true;
	}

	bool LevelStage::onKey(unsigned key, bool isDown)
	{
		if( !BaseClass::onKey(key, isDown) )
			return false;

		if ( isDown )
		{
			switch( key )
			{
			case Keyboard::eR:
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
