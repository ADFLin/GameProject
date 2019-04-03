#include "TinyGamePCH.h"
#include "GameStage.h"

#include "GameModule.h"
#include "GameModuleManager.h"
#include "GameReplay.h"
#include "GameAction.h"
#include "GameStageMode.h"
#include "GameGUISystem.h"

class EmptyStageMode : public GameStageMode
{
public:
	EmptyStageMode():GameStageMode( SMT_UNKNOW ){}
	virtual IPlayerManager* getPlayerManager() override
	{
		return &mPlayerManager;
	}

	void updateTime(long time)
	{
		int frame = time / mCurStage->getTickTime();
		ActionProcessor& processor = mCurStage->getActionProcessor();

		for( int i = 0; i < frame; ++i )
		{
			unsigned flag = 0;
			switch( getGameState() )
			{
			case GameState::Run:
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


private:
	LocalPlayerManager mPlayerManager;
};


GameStageBase::GameStageBase()
{
	mGame = nullptr;
	mStageMode = nullptr;
	mTickTime = gDefaultTickTime;
}

bool GameStageBase::onInit()
{
	mGame = Global::GameManager().getRunningGame();

	if( !mStageMode )
	{
		mStageMode = new EmptyStageMode;
		mStageMode->mCurStage = this;
	}

	AttribValue attrTime(ATTR_TICK_TIME);
	if( getAttribValue(attrTime) )
	{
		mTickTime = attrTime.iVal;
	}
	else
	{
		mTickTime = gDefaultTickTime;
	}

	getManager()->setTickTime(mTickTime);

	if( !BaseClass::onInit() )
		return false;

	return true;
}

void GameStageBase::onEnd()
{
	mStageMode->onEnd();
	mStageMode->mCurStage = nullptr;
	getManager()->setTickTime(gDefaultTickTime);
	BaseClass::onEnd();
}

void GameStageBase::onUpdate(long time)
{
	mStageMode->updateTime(time);
	BaseClass::onUpdate(time);
}

bool GameStageBase::onKey(unsigned key, bool isDown)
{
	return mStageMode->onKey(key, isDown);
}

bool GameStageBase::onWidgetEvent(int event, int id, GWidget* ui)
{
	if( !mStageMode->onWidgetEvent(event, id, ui) )
		return false;

	if( id >= UI_GAME_STAGE_MODE_ID )
		return false;

	return BaseClass::onWidgetEvent(event, id, ui);
}

void GameStageBase::setupStageMode(GameStageMode* mode)
{
	assert(mStageMode == nullptr);
	mStageMode = mode;
	if ( mode )
		mode->mCurStage = this;
}

bool GameStageBase::changeState(GameState state)
{
	return mStageMode->changeState(state);
}

GameState GameStageBase::getGameState() const
{
	return mStageMode->getGameState();
}

StageModeType GameStageBase::getModeType() const
{
	return mStageMode->getModeType();
}
