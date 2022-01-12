#include "TinyGamePCH.h"
#include "StageBase.h"


StageBase::StageBase() : mManager( nullptr )
{
	
}

StageBase::~StageBase()
{

}

void StageBase::update( long time )
{
	TaskHandler::runTask( time );
	onUpdate( time );
}

void StageBase::render( float dFrame )
{
	onRender( dFrame );
}

MsgReply StageBase::onChar( unsigned code )
{
	return MsgReply::Unhandled();
}

MsgReply StageBase::onKey(KeyMsg const& msg)
{
	return MsgReply::Unhandled();
}

MsgReply StageBase::onMouse( MouseMsg const& msg )
{
	return MsgReply::Unhandled();
}

void StageBase::onTaskMessage( TaskBase* task , TaskMsg const& msg )
{


}

void StageManager::setNextStage( StageBase* chStage )
{
	if ( chStage == nullptr )
		chStage = resolveChangeStageFail( FailReason::NoStage );

	if ( chStage )
	{
		if ( mNextStage )
			delete mNextStage;
		mNextStage = chStage;
		mNextStage->mManager = this;
	}
}


class EmptyStage : public StageBase 
{ 
public:
};

StageManager::StageManager() 
	:mCurStage(  new EmptyStage )
	,mNextStage( nullptr )
{

}

StageManager::~StageManager()
{
	cleanup();
}

void StageManager::setupStage()
{
	prevStageChange();

	mCurStage->mManager = this;
	for(;;)
	{
		if( initializeStage(mCurStage) )
			break;

		mCurStage->onInitFail();

		StageBase* stage = resolveChangeStageFail( FailReason::InitFail );
		if ( stage == nullptr )
		{
			stage = new EmptyStage;
		}

		if ( stage != mCurStage )
		{
			delete mCurStage;
			stage->mManager = this;
		}
		mCurStage = stage;
	}

	postStageChange( mCurStage );
}

StageBase* StageManager::changeStage( StageID stageId )
{
	StageBase* chStage = createStage( stageId );
	setNextStage( chStage );
	return chStage;
}


void StageManager::checkNewStage()
{
	if ( mNextStage )
	{

		mCurStage->onEnd();
		postStageEnd(mCurStage);
		delete mCurStage;


		mCurStage = mNextStage;
		mNextStage = nullptr;
		setupStage();
	}
}

void StageManager::cleanup()
{
	TaskHandler::clearTask();


	if ( mCurStage )
	{
		mCurStage->onEnd();
		postStageEnd(mCurStage);

		delete mCurStage;
		mCurStage = nullptr;
	}

	delete mNextStage;
	mNextStage = nullptr;

}
