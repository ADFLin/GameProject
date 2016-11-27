#include "TinyGamePCH.h"
#include "StageBase.h"

#include "GamePackage.h"
#include "GamePackageManager.h"

#include "GameGUISystem.h"

StageBase::StageBase() : mManager( NULL )
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

bool StageBase::onChar( unsigned code )
{
	return ::Global::GUI().getManager().procCharMsg( code );
}

bool StageBase::onKey( unsigned key , bool isDown )
{
	bool result = ::Global::GUI().getManager().procKeyMsg( key , isDown );
	//getManager()->getController().setBlocked( !result );

	return result;
}

bool StageBase::onMouse( MouseMsg const& msg )
{
	return true;
}

void StageBase::onTaskMessage( TaskBase* task , TaskMsg const& msg )
{


}

void StageManager::setNextStage( StageBase* chStage )
{
	if ( chStage == NULL )
		chStage = resolveChangeStageFail( FR_NO_STAGE );

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
	,mNextStage( NULL )
{

}

StageManager::~StageManager()
{
	cleanup();
}

void StageManager::setupStage()
{
	prevChangeStage();

	mCurStage->mManager = this;
	while( !mCurStage->onInit() )
	{
		StageBase* stage = resolveChangeStageFail( FR_INIT_FAIL );
		if ( stage == NULL )
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
		delete mCurStage;
		mCurStage = mNextStage;
		mNextStage = NULL;
		setupStage();
	}
}

void StageManager::cleanup()
{
	TaskHandler::clearTask();

	delete mNextStage;
	mNextStage = NULL;

	if ( mCurStage )
	{
		mCurStage->onEnd();
		delete mCurStage;
		mCurStage = NULL;
	}
}
