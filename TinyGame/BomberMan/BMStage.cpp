#include "BMPCH.h"
#include "BMStage.h"

#include "BMAction.h"

#include "GameModule.h"
#include "RenderUtility.h"

#include "GameGUISystem.h"
#include "Widget/WidgetUtility.h"

#include "GameStageMode.h"
#include "CSyncFrameManager.h"

namespace BomberMan
{
	int const TimeReadyWait = 3000;

	bool LevelStage::onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		getActionProcessor().setLanucher( this );
		getActionProcessor().addListener( *this );
		return true;
	}

	void LevelStage::onEnd()
	{
		getActionProcessor().setLanucher( NULL );
		getActionProcessor().removeListener(*this);

		delete &mMode->getMapGenerator(); 
		delete mMode;

		BaseClass::onEnd();
	}


	void LevelStage::onRestart( bool beInit )
	{
		mGameTime = 0;
		setStep( STEP_READY , TimeReadyWait );

		mMode->restart( beInit );

		World& level = mMode->getWorld();
		Player* player = level.getPlayer( 0 );
		player->addItem( ITEM_BOMB_KICK );
		player->addItem( ITEM_POWER_GLOVE );
		player->addItem( ITEM_RUBBER_BOMB );

	}

	void LevelStage::onRender( float dFrame )
	{
		Graphics2D& g = Global::GetGraphics2D();

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
				RenderUtility::SetFont( g , FONT_S24 );
				g.setTextColor(Color3ub(255 , 255 , 255) );
				g.drawText( Vec2i(0,0) , ::Global::GetScreenSize() , "Ready" );
			}
			else
			{
				RenderUtility::SetFont( g , FONT_S24 );
				g.setTextColor(Color3ub(255 , 0 , 0 ));
				g.drawText( Vec2i(0,0) , ::Global::GetScreenSize() , "Go!" );
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
		for( auto iter = playerMgr.createIterator(); iter; ++iter )
		{
			GamePlayer* player = iter.getElement();
			if ( player->getType() != PT_SPECTATORS )
			{
				player->setActionPort( mMode->addPlayer()->getId() );
			}
		}

		InputControl& inputControl = getGame()->getInputControl();
		
		switch( getModeType() )
		{
		case EGameStageMode::Net:
			{
				GamePlayer* player = playerMgr.getUser();
				inputControl.setPortControl( player->getActionPort() , 0 );
			}
			break;
		case EGameStageMode::Single:
			{
				inputControl.setPortControl( 0 , 0 );
				inputControl.setPortControl( 1 , 1 );
			}
			break;
		}

		mScene.setViewPos( Vec2i( 0 , 0 ) );
		::Global::GUI().cleanupWidget();

		GFrame* frame = WidgetUtility::CreateDevFrame();
	}

	void LevelStage::tick()
	{
		mGameTime += gDefaultTickTime;

		if ( mNextStepTime != -1 && mNextStepTime <= mGameTime )
		{
			switch( mStep )
			{
			case STEP_READY:
				if ( changeState( EGameState::Run ))
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
					getStageMode()->restart( false );
				}
				break;
			case STEP_GAME_OVER:
				switch( getModeType() )
				{
				case EGameStageMode::Net:
					if (::Global::GameNet().getNetWorker()->isServer())
					{
						::Global::GameNet().getNetWorker()->changeState(NAS_ROOM_ENTER);
					}
					break;
				case EGameStageMode::Single:
					getManager()->changeStage( STAGE_GAME_MENU );
					break;
				case EGameStageMode::Replay:
					getManager()->changeStage( STAGE_MAIN_MENU );
					break;
				}
				break;
			}
		}
		switch( getGameState() )
		{
		case EGameState::Run:
			mMode->tick();
			if ( mMode->getState() != Mode::eRUN )
			{
				setStep( STEP_ROUND_RESULT , 3000 );
				changeState( EGameState::End );
			}
			break;
		case EGameState::End:
			break;
		}
	}

	void LevelStage::updateFrame( int frame )
	{
		mScene.updateFrame( frame );
	}

	bool LevelStage::onKey(unsigned key, bool isDown)
	{
		if( !isDown )
			return false;

		switch( key )
		{
		case EKeyCode::R: getStageMode()->restart(false); break;
		}
		return false;
	}

	IFrameActionTemplate* LevelStage::createActionTemplate(unsigned version)
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
			CServerFrameGenerator* frameCollector = new CServerFrameGenerator( gMaxPlayerNum );
			netFrameMgr = new SVSyncFrameManager( netWorker , actionTemplate , frameCollector );
		}
		else
		{
			CClientFrameGenerator* frameCollector = new CClientFrameGenerator;
			netFrameMgr = new CLSyncFrameManager( netWorker , actionTemplate , frameCollector );

		}
		*engine  = new CFrameActionEngine( netFrameMgr );
		return true;
	}


	void LevelStage::fireAction( ActionTrigger& trigger )
	{
		World& world = mScene.getWorld();
		for( int i = 0 ; i < world.getPlayerNum() ; ++i )
		{
			Player* player = world.getPlayer(i);
			EvalPlayerAction( *player , trigger );
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

	void LevelStage::onScanActionStart(bool bUpdateFrame )
	{
		if( bUpdateFrame )
		{
			mScene.getWorld().clearAction();
		}
	}

	void LevelStage::onFireAction(ActionParam& param)
	{
		return;
	}

}//namespace BomberMan


