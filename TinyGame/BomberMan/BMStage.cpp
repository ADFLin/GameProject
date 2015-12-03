#include "BMPCH.h"
#include "BMStage.h"

#include "BMAction.h"

#include "GamePackage.h"
#include "RenderUtility.h"
#include "GameSingleStage.h"

#include "GameGUISystem.h"
#include "WidgetUtility.h"

#include "CSyncFrameManager.h"

namespace BomberMan
{
	int const TimeReadyWait = 3000;

	bool LevelStage::onInit()
	{
		getStage()->getActionProcessor().setEnumer( this );
		return true;
	}

	void LevelStage::onEnd()
	{
		getStage()->getActionProcessor().setEnumer( NULL );

		delete &mMode->getMapGenerator(); 
		delete mMode;
	}


	void LevelStage::onRestart( uint64 seed , bool beInit )
	{
		mGameTime = 0;
		setStep( STEP_READY , TimeReadyWait );
		Global::RandSeedNet( seed );

		mMode->restart( beInit );

		World& level = mMode->getWorld();
		Player* player = level.getPlayer( 0 );
		player->addItem( ITEM_BOMB_KICK );
		player->addItem( ITEM_POWER_GLOVE );
		player->addItem( ITEM_RUBBER_BOMB );

	}

	void LevelStage::onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();

		g.beginXForm();
		{
			g.identityXForm();
			g.translateXForm( 50 , 30 );
			g.scaleXForm( 1.0 , 1.0 );
			mScene.render( g );
		}
		g.finishXForm();

		switch( mStep )
		{
		case STEP_READY:
			if ( mNextStepTime - mGameTime > TimeReadyWait / 2 )
			{
				RenderUtility::setFont( g , FONT_S24 );
				g.setTextColor( 255 , 255 , 255 );
				g.drawText( Vec2i(0,0) , Global::getDrawEngine()->getScreenSize() , "Ready" );
			}
			else
			{
				RenderUtility::setFont( g , FONT_S24 );
				g.setTextColor( 255 , 0 , 0  );
				g.drawText( Vec2i(0,0) , Global::getDrawEngine()->getScreenSize() , "Go!" );
			}
		}
	}

	void LevelStage::setupLocalGame( LocalPlayerManager& playerManager )
	{
		for( int i = 0 ; i < 2 ; ++i )
		{
			playerManager.createPlayer( i );
		}
		playerManager.setUserID( 0 );

		IMapGenerator* mapGen = new CSimpleMapGen( Vec2i(21,17) , NULL );

		BattleMode* mode = new BattleMode( *mapGen );

		BattleMode::Info& modeInfo = mode->getInfo();
		modeInfo.numBomb   = 2;
		modeInfo.power     = 2;
		modeInfo.time      = 10 * 60 * 1000;

		setMode( mode );
	}

	void LevelStage::setupScene( IPlayerManager& playerMgr )
	{
		for( IPlayerManager::Iterator iter = playerMgr.getIterator();
			iter.haveMore() ; iter.goNext() )
		{
			GamePlayer* player = iter.getElement();
			if ( player->getType() != PT_SPECTATORS )
			{
				player->getInfo().actionPort = mMode->addPlayer()->getId();
			}
		}

		GameController& controller = getGame()->getController();
		
		switch( getGameType() )
		{
		case GT_NET_GAME:
			{
				GamePlayer* player = playerMgr.getUser();
				controller.setPortControl( player->getActionPort() , 0 );
			}
			break;
		case GT_SINGLE_GAME:
			{
				controller.setPortControl( 0 , 0 );
				controller.setPortControl( 1 , 1 );
			}
			break;
		}

		mScene.setViewPos( Vec2i( 0 , 0 ) );
		::Global::getGUI().cleanupWidget();

		GFrame* frame = WidgetUtility::createDevFrame();
	}

	void LevelStage::tick()
	{
		mGameTime += gDefaultTickTime;

		if ( mNextStepTime != -1 && mNextStepTime <= mGameTime )
		{
			switch( mStep )
			{
			case STEP_READY:
				if ( changeState( GS_RUN ))
				{
					setStep( STEP_RUN , -1 );
				}
				break;
			case STEP_ROUND_RESULT:
				if ( mMode->getState() == Mode::eGAME_OVER )
				{
					setStep( STEP_GAME_OVER , 1000 );
				}
				else
				{
					getStage()->restart( false );
				}
				break;
			case STEP_GAME_OVER:
				switch( getGameType() )
				{
				case GT_NET_GAME:
					if ( getManager()->getNetWorker()->isServer() )
						getManager()->getNetWorker()->changeState( NAS_ROOM_ENTER );
					break;
				case GT_SINGLE_GAME:
					getManager()->changeStage( STAGE_GAME_MENU );
					break;
				case GT_REPLAY:
					getManager()->changeStage( STAGE_MAIN_MENU );
					break;
				}
				break;
			}
		}
		switch( getState() )
		{
		case GS_RUN:
			mMode->tick();
			if ( mMode->getState() != Mode::eRUN )
			{
				setStep( STEP_ROUND_RESULT , 3000 );
				changeState( GS_END );
			}
			break;
		case GS_END:
			break;
		}
	}

	void LevelStage::updateFrame( int frame )
	{
		mScene.updateFrame( frame );
	}

	IFrameActionTemplate* LevelStage::createActionTemplate( unsigned version )
	{ 
		switch ( version )
		{
		case CFrameActionTemplate::LastVersion:
		case LAST_VERSION:
			return new CFrameActionTemplate( mScene.getWorld() ) ; 
		}
		return NULL;
	}

	bool  LevelStage::setupNetwork( NetWorker* netWorker , INetEngine** engine )
	{
		IFrameActionTemplate* actionTemplate = createActionTemplate( LAST_VERSION );
		INetFrameManager* netFrameMgr;
		if ( netWorker->isServer() )
		{
			CServerFrameGenerator* frameGenerator = new CServerFrameGenerator( gMaxPlayerNum );
			netFrameMgr = new SVSyncFrameManager( netWorker , actionTemplate , frameGenerator );
		}
		else
		{
			CClientFrameGenerator* frameGenerator = new CClientFrameGenerator;
			netFrameMgr = new CLSyncFrameManager( netWorker , actionTemplate , frameGenerator );

		}
		*engine  = new CFrameActionEngine( netFrameMgr );
		return true;
	}


	void LevelStage::prevScanAction( bool beUpdateFrame )
	{
		if ( beUpdateFrame )
		{
			mScene.getWorld().clearAction();
		}
	}

	void LevelStage::fireAction( ActionTrigger& trigger )
	{
		World& world = mScene.getWorld();
		for( int i = 0 ; i < world.getPlayerNum() ; ++i )
		{
			Player* player = world.getPlayer(i);
			trigger.setPort( player->getId() );
			evalPlayerAction( *player , trigger );
		}
	}

	void LevelStage::setMode( Mode* mode )
	{
		mMode = mode;
		mMode->setupWorld( mScene.getWorld() );
	}

	void LevelStage::setStep( StageStep step , long time )
	{
		if ( time == -1 )
			mNextStepTime = -1;
		else
			mNextStepTime = mGameTime + time;

		mStep = step;
	}

}//namespace BomberMan


