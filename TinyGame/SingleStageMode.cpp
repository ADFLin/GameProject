#include "TinyGamePCH.h"
#include "SingleStageMode.h"

#include "GameInstance.h"
#include "GameGUISystem.h"
#include "GameWidgetID.h"
#include "GameStage.h"

SingleStageMode::SingleStageMode() 
	:BaseClass(SMT_SINGLE_GAME)
	,mPlayerManager(new LocalPlayerManager)
{

}

bool SingleStageMode::postStageInit()
{
	if( !BaseClass::postStageInit() )
		return false;

	GameStageBase* stage = getStage();

	stage->setupLocalGame(*mPlayerManager.get());
	stage->setupScene(*mPlayerManager.get());

	for( auto iter = getPlayerManager()->createIterator(); iter; ++iter )
	{
		GamePlayer* player = iter.getElement();
		if( player->getAI() )
		{
			IActionInput* input = player->getAI()->getActionInput();
			if( input )
				stage->getActionProcessor().addInput(*input);
		}
	}

	if( !buildReplayRecorder() )
	{

	}

	ActionProcessor& processor = stage->getActionProcessor();

	processor.addInput(getGame()->getController());
	restartImpl(true);
	return true;
}

void SingleStageMode::onRestart(uint64& seed)
{
	seed = ::generateRandSeed();
	BaseClass::onRestart(seed);
}

void SingleStageMode::updateTime(long time)
{
	int frame = time / mCurStage->getTickTime();
	ActionProcessor& processor = mCurStage->getActionProcessor();

	for( int i = 0; i < frame; ++i )
	{
		unsigned flag = 0;
		switch( getGameState() )
		{
		case GS_RUN:
			++mReplayFrame;
			break;
		default:
			flag |= CTF_FREEZE_FRAME;
		}
		processor.beginAction(flag);
		mCurStage->tick();
		processor.endAction();
	}

	mCurStage->updateFrame(frame);
	::Global::GUI().scanHotkey(getGame()->getController());
}

bool SingleStageMode::onWidgetEvent(int event, int id, GWidget* ui)
{
	switch( id )
	{
	case UI_PAUSE_GAME:
		togglePause();
		return false;
	case UI_GAME_MENU:
		if( event == EVT_BOX_YES )
		{
			saveReplay(LAST_REPLAY_NAME);
			getStage()->getManager()->changeStage(STAGE_GAME_MENU);
			return true;
		}
		else if( event == EVT_BOX_NO )
		{
			togglePause();
			return false;
		}
		else
		{
			togglePause();
			::Global::GUI().showMessageBox(UI_GAME_MENU, LAN("Back Game Menu?"));
			return false;
		}
		break;

	case UI_RESTART_GAME:
		if( event == EVT_BOX_YES )
		{
			restartImpl(false);
			return false;
		}
		else if( event == EVT_BOX_NO )
		{

		}
		else
		{
			if( getGameState() != GS_END )
			{
				::Global::GUI().showMessageBox(
					UI_RESTART_GAME, LAN("Do you Want to Stop Current Game?"));
			}
			else
			{
				restartImpl(false);
			}
			return false;

		}
		break;
	}

	return BaseClass::onWidgetEvent(event, id, ui);
}

bool SingleStageMode::tryChangeState(GameState state)
{
	switch( state )
	{
	case GS_END:
		saveReplay(LAST_REPLAY_NAME);
		break;
	}
	return true;
}

LocalPlayerManager* SingleStageMode::getPlayerManager()
{
	return mPlayerManager.get();
}
