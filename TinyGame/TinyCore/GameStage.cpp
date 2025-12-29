#include "TinyGamePCH.h"
#include "GameStage.h"

#include "GameModule.h"
#include "GameModuleManager.h"
#include "GameReplay.h"
#include "GameAction.h"
#include "GameMode.h"
#include "GameGUISystem.h"

class EmptyStageMode : public GameModeBase
{
public:
	EmptyStageMode():GameModeBase( EGameMode::Unknow ){}
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
	if (mStageMode)
	{
		mStageMode->onEnd();
		mStageMode->mCurStage = nullptr;
		
		// Release EmptyStageMode if created internally (getModeType == Unknow)
		// Modes created by StageManager (Net, Single, Replay) are managed by StageManager
		if (mStageMode->getModeType() == EGameMode::Unknow)
		{
			delete mStageMode;
			mStageMode = nullptr;
		}
	}
	BaseClass::onEnd();
}

void GameStageBase::onUpdate(GameTimeSpan deltaTime)
{
	// Note: Mode's updateTime is now called directly by StageManager
	// This method only handles Stage-specific update logic
	BaseClass::onUpdate(deltaTime);
}

MsgReply GameStageBase::onKey(KeyMsg const& msg)
{
	// Note: Mode's onKey is now called directly by StageManager
	// This method only handles Stage-specific key events
	return MsgReply::Unhandled();
}

bool GameStageBase::onWidgetEvent(int event, int id, GWidget* ui)
{
	// Note: Mode's onWidgetEvent is now called directly by StageManager
	// This method only handles Stage-specific widget events
	
	if( id >= UI_GAME_STAGE_MODE_ID )
		return true;  // Let StageManager handle Mode-level events

	return BaseClass::onWidgetEvent(event, id, ui);
}

void GameStageBase::setupStageMode(GameModeBase* mode)
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

EGameMode GameStageBase::getModeType() const
{
	return mStageMode->getModeType();
}

ActionProcessor& GameStageBase::getActionProcessor()
{
	return mStageMode->getActionProcessor();
}
