#include "TinyGamePCH.h"
#include "GameMode.h"

#include "GameReplay.h"
#include "GameModule.h"
#include "GameAction.h"

GameModeBase::GameModeBase(EGameMode mode) 
	:mStageMode(mode)
	, mCurStage(nullptr)
	, mGameState(EGameState::End)
	, mReplayFrame(0)
{

}

GameModeBase::~GameModeBase()
{

}

bool GameModeBase::changeState(EGameState state)
{
	if( mGameState == state )
		return true;

	if( !prevChangeState(state) )
		return false;

	mGameState = state;
	if (mCurStage)
	{
		mCurStage->onChangeState(state);
	}
	return true;
}

void GameModeBase::restart(bool beInit)
{
	getGame()->getInputControl().restart();
	doRestart(beInit);
}

void GameModeBase::doRestart(bool bInit)
{
	uint64 seed;
	onRestart(seed);
	::Global::RandSeedNet(seed);
	if (mCurStage)
	{
		mCurStage->onRestart(bInit);
	}

	mReplayFrame = 0;
	changeState(EGameState::Start);
}

bool GameModeBase::togglePause()
{
	if( getGameState() == EGameState::Run )
		return changeState(EGameState::Pause);
	else if( getGameState() == EGameState::Pause )
		return changeState(EGameState::Run);
	return false;
}


GameLevelMode::GameLevelMode(EGameMode mode)
	:BaseClass(mode)
{

}

GameLevelMode::~GameLevelMode()
{

}

bool GameLevelMode::saveReplay(char const* name)
{
	if( getGameState() == EGameState::Start )
		return false;

	if( !mReplayRecorder.get() )
		return false;

	mReplayRecorder->stop();
	GameAttribute attrInfo(ATTR_REPLAY_INFO, &mReplayRecorder->getReplay().getInfo());
	if( !getStage()->queryAttribute(attrInfo) )
	{
		return false;
	}

	InlineString< 512 > path;
	path.format("%s/%s", REPLAY_DIR, name);
	//std::string path = std::string(REPLAY_DIR) + "/" + name;
	return mReplayRecorder->save(path);
}

void GameLevelMode::onRestart(uint64& seed)
{
	BaseClass::onRestart(seed);
	if( mReplayRecorder.get() )
		mReplayRecorder->start(seed);
}

bool GameLevelMode::buildReplayRecorder(GameStageBase* stage)
{
	IGameModule* game = getGame();
	if( !game )
		return false;

	GameAttribute replaySupport(ATTR_REPLAY_SUPPORT);
	if( !game->queryAttribute(replaySupport) )
		return false;
	if( replaySupport.iVal == 0 )
		return false;

	ActionProcessor& processor = stage->getActionProcessor();
	IPlayerManager* playerManager = getPlayerManager();

	if( IFrameActionTemplate* actionTemplate = stage->createActionTemplate(LAST_VERSION) )
	{
		actionTemplate->setupPlayer(*playerManager);
		mReplayRecorder.reset(
			new ReplayRecorder(actionTemplate, mReplayFrame));

		GameAttribute dataValue(ATTR_REPLAY_INFO_DATA, &mReplayRecorder->getReplay().getInfo());
		if( !stage->queryAttribute(dataValue) )
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

