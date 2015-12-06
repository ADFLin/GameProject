#include "TinyGamePCH.h"
#include "GameSingleStage.h"

#include "GamePackage.h"
#include "GameClient.h"

#include "GameGUISystem.h"
#include "GameWidgetID.h"

GameSingleStage::GameSingleStage() 
	:BaseClass( GT_SINGLE_GAME )
	,mPlayerManager( new LocalPlayerManager )
{

}

bool GameSingleStage::onInit()
{
	if ( !BaseClass::onInit() )
		return false;


	GameSubStage* subStage = getSubStage();

	subStage->setupLocalGame( *mPlayerManager.get() );
	subStage->setupScene( *mPlayerManager.get() );

	for( IPlayerManager::Iterator iter = getPlayerManager()->getIterator();
		iter.haveMore() ; iter.goNext() )
	{
		GamePlayer* player = iter.getElement();
		if ( player->getAI() )
		{
			ActionInput* input = player->getAI()->getActionInput();
			if ( input )
				getActionProcessor().addInput( *input );
		}
	}

	if  ( !buildReplayRecorder() )
	{

	}

	ActionProcessor& processor = getActionProcessor();

	processor.addInput( getGame()->getController() );
	restart( true );
	return true;
}

void GameSingleStage::onRestart( uint64& seed )
{
	seed = ::generateRandSeed();
	BaseClass::onRestart( seed );
}

void GameSingleStage::onUpdate( long time )
{
	int frame = time / getTickTime();
	ActionProcessor& processor = getActionProcessor();

	for( int i = 0 ; i < frame ; ++i )
	{
		unsigned flag = 0;
		switch( getState() )
		{
		case GS_RUN:
			++mReplayFrame;
			break;
		default:
			flag |= CTF_FREEZE_FRAME;
		}
		getActionProcessor().beginAction( flag );
		getSubStage()->tick();
		getActionProcessor().endAction();
	}

	getSubStage()->updateFrame( frame );

	::Global::getGUI().scanHotkey( getGame()->getController() );
}

void GameSingleStage::onRender( float dFrame )
{
	Graphics2D& g = Global::getGraphics2D();

	BaseClass::onRender( dFrame );
}


bool GameSingleStage::tryChangeState( GameState state )
{
	switch( state )
	{
	case GS_END:
		saveReplay( LAST_REPLAY_NAME );
		break;
	}
	return true;
}

bool GameSingleStage::onWidgetEvent( int event , int id , GWidget* ui )
{
	switch ( id )
	{
	case UI_PAUSE_GAME : 
		togglePause(); 
		return false;
	case UI_GAME_MENU:
		if ( event == EVT_BOX_YES )
		{
			saveReplay( LAST_REPLAY_NAME );
			getManager()->changeStage( STAGE_GAME_MENU );
			return true;
		}
		else if ( event == EVT_BOX_NO )
		{
			togglePause();
			return false;
		}
		else
		{
			togglePause();
			::Global::getGUI().showMessageBox( UI_GAME_MENU , LAN("Back Game Menu?") );
			return false;
		}
		break;

	case UI_RESTART_GAME:
		if ( event == EVT_BOX_YES )
		{
			restart( false ); 
			return false;
		}
		else if ( event == EVT_BOX_NO )
		{

		}
		else
		{
			if ( getState() != GS_END )
			{
				::Global::getGUI().showMessageBox( 
					UI_RESTART_GAME , LAN("Do you Want to Stop Current Game?") );
			}
			else
			{
				restart( false ); 
			}
			return false;

		}
		break;
	}

	return BaseClass::onWidgetEvent( event , id , ui );
}


LocalPlayerManager* GameSingleStage::getPlayerManager()
{
	return mPlayerManager.get();
}
