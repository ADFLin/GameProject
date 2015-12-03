#include "GreedySnakePCH.h"
#include "GreedySnakeGame.h"

#include "GreedySnakeStage.h"
#include "GreedySnakeScene.h"
#include "GreedySnakeMode.h"
#include "GreedySnakeAction.h"

#include "GameWidgetID.h"
#include "GameGUISystem.h"
#include "GameSingleStage.h"

#include "CSyncFrameManager.h"

namespace GreedySnake
{
	LevelStage::LevelStage()
	{
		mMode = NULL;
	}

	LevelStage::~LevelStage()
	{
		delete mMode;
	}

	bool LevelStage::onInit()
	{
		if ( !BaseClass::onInit() )
			return false;

		if ( mMode == NULL )
		{
			//return false;
			//mMode = new BattleMode;
			mMode = new SurvivalMode;
		}

		::Global::getGUI().cleanupWidget();

		mScene.reset( new Scene( *mMode ) );
		getStage()->getActionProcessor().setEnumer( mScene.get() );
		return true;
	}

	void LevelStage::onRestart( uint64 seed , bool beInit )
	{
		if ( beInit )
			Global::RandSeedNet( seed );
		mScene->restart( beInit );
	}

	void LevelStage::onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();
		mScene->render( g , dFrame );
	}

	void LevelStage::setupScene( IPlayerManager& playerManager )
	{
		mMode->setupLevel( playerManager );

		switch( getGameType() )
		{
		case GT_SINGLE_GAME:
		case GT_NET_GAME:
			{
				GamePlayer* player = playerManager.getUser();
				getGame()->getController().setPortControl( player->getActionPort() , 0 );
			}
			break;
		}
		
	}

	void LevelStage::tick()
	{
		switch( getState() )
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
			switch( getGameType() )
			{
			case GT_SINGLE_GAME:
				::Global::getGUI().showMessageBox( 
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
				if ( getGameType() == GT_SINGLE_GAME )
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

	void LevelStage::setupLocalGame( LocalPlayerManager& playerManager )
	{
		playerManager.createPlayer( 0 );
		playerManager.setUserID( 0 );
	}

}
