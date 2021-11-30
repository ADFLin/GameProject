#include "TinyGamePCH.h"
#include "SingleStageMode.h"

#include "GameModule.h"
#include "GameGUISystem.h"
#include "GameWidgetID.h"
#include "GameStage.h"

SingleStageMode::SingleStageMode() 
	:BaseClass(EGameStageMode::Single)
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
	if( getGame() )
	{
		getGame()->getInputControl().setupInput(processor);
	}

	restart(true);
	return true;
}

void SingleStageMode::onRestart(uint64& seed)
{
	//seed = ::generateRandSeed();
	seed = 0;
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
		case EGameState::Run:
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
	if ( getGame() )
		::Global::GUI().scanHotkey(getGame()->getInputControl());
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
			::Global::GUI().showMessageBox(UI_GAME_MENU, LOCTEXT("Back Game Menu?"));
			return false;
		}
		break;

	case UI_RESTART_GAME:
		if( event == EVT_BOX_YES )
		{
			restart(false);
			return false;
		}
		else if( event == EVT_BOX_NO )
		{

		}
		else
		{
			if( getGameState() != EGameState::End )
			{
				::Global::GUI().showMessageBox(
					UI_RESTART_GAME, LOCTEXT("Do you Want to Stop Current Game?"));
			}
			else
			{
				restart(false);
			}
			return false;

		}
		break;
	}

	return BaseClass::onWidgetEvent(event, id, ui);
}

bool SingleStageMode::prevChangeState(EGameState state)
{
	switch( state )
	{
	case EGameState::End:
		saveReplay(LAST_REPLAY_NAME);
		break;
	}
	return true;
}

LocalPlayerManager* SingleStageMode::getPlayerManager()
{
	return mPlayerManager.get();
}
