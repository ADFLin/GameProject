#include "TinyGamePCH.h"
#include "GameStageMode.h"

#include "GameReplay.h"
#include "GamePackage.h"
#include "GameAction.h"

LevelStageMode::LevelStageMode(StageModeType mode) 
	:BaseClass(mode)
{

}

bool LevelStageMode::saveReplay(char const* name)
{
	if( getGameState() == GS_START )
		return false;

	if( !mReplayRecorder.get() )
		return false;

	mReplayRecorder->stop();
	std::string path = std::string(REPLAY_DIR) + "/" + name;

	AttribValue attrInfo(ATTR_REPLAY_INFO, &mReplayRecorder->getReplay().getInfo());

	if( !getStage()->getAttribValue(attrInfo) )
	{
		return false;
	}
	return mReplayRecorder->save(path.c_str());
}

void LevelStageMode::onRestart(uint64& seed)
{
	BaseClass::onRestart(seed);
	if( mReplayRecorder.get() )
		mReplayRecorder->start(seed);
}

bool LevelStageMode::buildReplayRecorder()
{
	IGamePackage* game = getGame();
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

	processor.setListener(mReplayRecorder.get());

	return true;
}

GameStageMode::GameStageMode(StageModeType mode) 
	:mStageMode(mode)
	, mCurStage(nullptr)
	, mGameState(GameState::GS_END)
	, mReplayFrame(0)
{

}

void GameStageMode::restart(bool beInit)
{
	uint64 seed;
	onRestart(seed);
	mCurStage->onRestart(seed, beInit);
	mReplayFrame = 0;
	changeState(GS_START);
}

bool GameStageMode::togglePause()
{
	if( getGameState() == GS_RUN )
		return changeState(GS_PAUSE);
	else if( getGameState() == GS_PAUSE )
		return changeState(GS_RUN);
	return false;
}
