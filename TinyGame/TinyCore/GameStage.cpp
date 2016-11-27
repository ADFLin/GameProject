#include "TinyGamePCH.h"
#include "GameStage.h"

#include "GamePackage.h"
#include "GamePackageManager.h"
#include "GameReplay.h"
#include "GameAction.h"
#include "GameStageMode.h"
#include "GameGUISystem.h"

class EmptySubStage : public GameSubStage
{
	typedef GameSubStage BaseClass;
public:
	virtual bool setupAttrib( AttribValue const& value ){ return BaseClass::setupAttrib( value ); }
};

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
			case GS_RUN:
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

GameStage::GameStage( StageModeType gameType )
	:mGameType( gameType )
{
	mSubStage = NULL;
}

void GameStage::setSubStage( GameSubStage* subStage )
{
	subStage->mGameStage = this;
	mSubStage = subStage;
}

bool GameStage::onInit()
{
	mGame = Global::GameManager().getCurGame();

	if ( !BaseClass::onInit() )
		return false;

	

	if ( !mSubStage )
	{
		mSubStage = new EmptySubStage;
	}

	AttribValue attrTime( ATTR_TICK_TIME );
	if ( getSubStage()->getAttribValue( attrTime ) )
	{
		mTickTime = attrTime.iVal;
	}
	else
	{
		mTickTime = gDefaultTickTime;
	}

	getManager()->setTickTime( mTickTime );

	if ( !mSubStage->onInit() )
		return false;

	return true;
}

bool GameStage::onWidgetEvent( int event , int id , GWidget* ui )
{
	if ( !mSubStage->onWidgetEvent( event, id , ui ) )
		return false;

	if ( id >= UI_SUB_STAGE_ID )
		return false;

	return BaseClass::onWidgetEvent( event , id , ui );
}


bool GameStage::changeState( GameState state )
{
	if ( mGameState == state )
		return true;

	if ( !tryChangeState( state ) )
		return false;

	mSubStage->onChangeState( state );
	mGameState = state;
	return true;
}

bool GameStage::togglePause()
{
	if ( getState() == GS_RUN )
		return changeState( GS_PAUSE );
	else if ( getState() == GS_PAUSE )
		return changeState( GS_RUN );
	return false;
}

void GameStage::restart( bool beInit )
{
	uint64 seed;
	onRestart( seed );

	mSubStage->onRestart( seed , beInit );
	mReplayFrame = 0;
	changeState( GS_START );
}

void GameStage::onRender( float dFrame )
{
	mSubStage->onRender( dFrame );
}

void GameStage::onEnd()
{
	mSubStage->onEnd();
	delete mSubStage;
	getManager()->setTickTime( gDefaultTickTime );
	BaseClass::onEnd();
}

bool GameStage::onMouse( MouseMsg const& msg )
{
	return mSubStage->onMouse( msg );
}

bool GameStage::onChar(unsigned code)
{
	return mSubStage->onChar(code);
}

bool GameStage::onKey(unsigned key , bool isDown)
{
	return mSubStage->onKey(key,isDown);
}


bool GameSubStage::getAttribValue( AttribValue& value )
{
	return false;
}


bool GameSubStage::setupAttrib( AttribValue const& value )
{
	return false;
}

bool GameSubStage::onMouse( MouseMsg const& msg )
{
	return true;
}


GameLevelStage::GameLevelStage( StageModeType gameType ) 
	:GameStage( gameType )
{

}

GameLevelStage::~GameLevelStage()
{

}




bool GameLevelStage::buildReplayRecorder()
{
	IGamePackage* game = getGame();
	if ( !game )
		return false;

	AttribValue replaySupport( ATTR_REPLAY_SUPPORT );
	if ( !game->getAttribValue( replaySupport ) )
		return false;
	if ( replaySupport.iVal == 0 )
		return false;

	ActionProcessor& processor = getActionProcessor();
	IPlayerManager* playerManager = getPlayerManager();

	if ( IFrameActionTemplate* actionTemplate = getSubStage()->createActionTemplate( LAST_VERSION ) )
	{
		actionTemplate->setupPlayer( *playerManager );
		mReplayRecorder.reset( 
			new ReplayRecorder( actionTemplate , mReplayFrame ) );

		AttribValue dataValue( ATTR_REPLAY_INFO_DATA , &mReplayRecorder->getReplay().getInfo() );
		if ( !getSubStage()->getAttribValue( dataValue ) )
		{
			mReplayRecorder.clear();
		}
	}
	else if ( ReplayTemplate* replayTemplate = getGame()->createReplayTemplate( LAST_VERSION ) )
	{
		mReplayRecorder.reset( 
			new OldVersion::ReplayRecorder( replayTemplate , mReplayFrame ) );

		if ( !replayTemplate->getReplayInfo( mReplayRecorder->getReplay().getInfo() ) )
			mReplayRecorder.clear();
	}

	if ( !mReplayRecorder )
		return false;

	processor.setListener( mReplayRecorder.get() );

	return true;
}

bool GameLevelStage::saveReplay( char const* name )
{
	if ( getState() == GS_START )
		return false;

	if ( !mReplayRecorder.get() )
		return false;

	mReplayRecorder->stop();
	std::string path = std::string( REPLAY_DIR ) + "/" + name;

	AttribValue attrInfo( ATTR_REPLAY_INFO , &mReplayRecorder->getReplay().getInfo() );

	if ( !mSubStage->getAttribValue( attrInfo ) )
	{
		return false;
	}
	return mReplayRecorder->save( path.c_str() );
}

void GameLevelStage::onRestart( uint64& seed )
{
	if ( mReplayRecorder.get() )
		mReplayRecorder->start( seed );

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

GameStageBase::GameStageBase()
{
	mStageMode = nullptr;
	mTickTime = gDefaultTickTime;
	mGame = nullptr;
}

bool GameStageBase::onInit()
{
	if( !BaseClass::onInit() )
		return false;


	if( !mStageMode )
	{
		mStageMode = new EmptyStageMode;
		mStageMode->mCurStage = this;
	}

	mGame = Global::GameManager().getCurGame();

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

	if( !mStageMode->onInit() )
		return false;

	return true;
}

void GameStageBase::onEnd()
{
	mStageMode->onEnd();
	delete mStageMode;
	getManager()->setTickTime(gDefaultTickTime);
	BaseClass::onEnd();
}

void GameStageBase::onUpdate(long time)
{
	BaseClass::onUpdate(time);
	mStageMode->updateTime(time);
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

GameState GameStageBase::getState() const
{
	return mStageMode->getGameState();
}

StageModeType GameStageBase::getGameType()
{
	return mStageMode->getModeType();
}
