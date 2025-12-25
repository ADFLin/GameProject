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
	EmptyStageMode():GameStageMode( EGameStageMode::Unknow ){}
	IPlayerManager* getPlayerManager() override
	{
		return &mPlayerManager;
	}

	void updateTime(GameTimeSpan deltaTime) override
	{
		mDeltaTimeAcc += 1000.0 * deltaTime;
		int frame = Math::FloorToInt(mDeltaTimeAcc / mCurStage->getTickTime());
		
		if (frame)
		{
			mDeltaTimeAcc -= frame * mCurStage->getTickTime();
			ActionProcessor& processor = mCurStage->getActionProcessor();
			for (int i = 0; i < frame; ++i)
			{
				unsigned flag = 0;
				switch (getGameState())
				{
				case EGameState::Run:
					break;
				default:
					flag |= CTF_FREEZE_FRAME;
				}
				processor.beginAction(flag);
				mCurStage->tick();
				processor.endAction();
			}

			mCurStage->updateFrame(frame);
		}
		::Global::GUI().scanHotkey(getGame()->getInputControl());
	}


private:
	float mDeltaTimeAcc = 0.0f;
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
	mGame = Global::ModuleManager().getRunningGame();

	if( !mStageMode )
	{
		mStageMode = new EmptyStageMode;
		mStageMode->mCurStage = this;
	}

	GameAttribute attrTime(ATTR_TICK_TIME);
	if( queryAttribute(attrTime) )
	{
		mTickTime = attrTime.iVal;
	}
	else
	{
		mTickTime = gDefaultTickTime;
	}

	if( !BaseClass::onInit() )
		return false;

	return true;
}

void GameStageBase::onEnd()
{
	mStageMode->onEnd();
	mStageMode->mCurStage = nullptr;
	BaseClass::onEnd();
}

void GameStageBase::onUpdate(GameTimeSpan deltaTime)
{
	mStageMode->updateTime(deltaTime);
	BaseClass::onUpdate(deltaTime);
}

MsgReply GameStageBase::onKey(KeyMsg const& msg)
{
	return mStageMode->onKey(msg);
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

bool GameStageBase::changeState(EGameState state)
{
	return mStageMode->changeState(state);
}

EGameState GameStageBase::getGameState() const
{
	return mStageMode->getGameState();
}

EGameStageMode GameStageBase::getModeType() const
{
	return mStageMode->getModeType();
}

ActionProcessor& GameStageBase::getActionProcessor()
{
	return mStageMode->getActionProcessor();
}
