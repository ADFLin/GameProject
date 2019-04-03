#include "TinyGamePCH.h"
#include "GameStageMode.h"

#include "GameReplay.h"
#include "GameModule.h"
#include "GameAction.h"

GameStageMode::GameStageMode(StageModeType mode) 
	:mStageMode(mode)
	, mCurStage(nullptr)
	, mGameState(GameState::End)
	, mReplayFrame(0)
{

}

bool GameStageMode::changeState(GameState state)
{
	if( mGameState == state )
		return true;

	if( !tryChangeState(state) )
		return false;

	mCurStage->onChangeState(state);
	mGameState = state;
	return true;
}

void GameStageMode::restart(bool beInit)
{
	getGame()->getController().restart();
	doRestart(beInit);
}

void GameStageMode::doRestart(bool bInit)
{
	uint64 seed;
	onRestart(seed);
	::Global::RandSeedNet(seed);
	mCurStage->onRestart(bInit);
	mReplayFrame = 0;
	changeState(GameState::Start);
}

bool GameStageMode::togglePause()
{
	if( getGameState() == GameState::Run )
		return changeState(GameState::Pause);
	else if( getGameState() == GameState::Pause )
		return changeState(GameState::Run);
	return false;
}


LevelStageMode::LevelStageMode(StageModeType mode)
	:BaseClass(mode)
{

}

bool LevelStageMode::saveReplay(char const* name)
{
	if( getGameState() == GameState::Start )
		return false;

	if( !mReplayRecorder.get() )
		return false;

	mReplayRecorder->stop();
	AttribValue attrInfo(ATTR_REPLAY_INFO, &mReplayRecorder->getReplay().getInfo());
	if( !getStage()->getAttribValue(attrInfo) )
	{
		return false;
	}

	FixString< 512 > path;
	path.format("%s/%s", REPLAY_DIR, name);
	//std::string path = std::string(REPLAY_DIR) + "/" + name;
	return mReplayRecorder->save(path);
}

void LevelStageMode::onRestart(uint64& seed)
{
	BaseClass::onRestart(seed);
	if( mReplayRecorder.get() )
		mReplayRecorder->start(seed);
}

bool LevelStageMode::buildReplayRecorder()
{
	IGameModule* game = getGame();
	if( !game )
		return false;

	AttribValue replaySupport(ATTR_REPLAY_SUPPORT);
	if( !game->getAttribValue(replaySupport) )
		return false;
	if( replaySupport.iVal == 0 )
		return false;

	ActionProcessor& processor = getStage()->getActionProcessor();
	IPlayerManager* playerManager = getPlayerManager();

	if( IFrameActionTemplate* actionTemplate = getStage()->createActionTemplate(LAST_VERSION) )
	{
		actionTemplate->setupPlayer(*playerManager);
		mReplayRecorder.reset(
			new ReplayRecorder(actionTemplate, mReplayFrame));

		AttribValue dataValue(ATTR_REPLAY_INFO_DATA, &mReplayRecorder->getReplay().getInfo());
		if( !getStage()->getAttribValue(dataValue) )
		{
			mReplayRecorder.clear();
		}
	}
	else if( ReplayTemplate* replayTemplate = getGame()->createReplayTemplate(LAST_VERSION) )
	{
		mReplayRecorder.reset(
			new OldVersion::ReplayRecorder(replayTemplate, mReplayFrame));

		if( !replayTemplate->getReplayInfo(mReplayRecorder->getReplay().getInfo()) )
			mReplayRecorder.clear();
	}

	if( !mReplayRecorder )
		return false;

	processor.addListener( *mReplayRecorder.get() );

	return true;
}

