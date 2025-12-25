#include "TinyGamePCH.h"
#include "CFrameActionNetEngine.h"

#include "GameModule.h"
#include "GamePlayer.h"

CFrameActionEngine::CFrameActionEngine( INetFrameManager* netFrameMgr )
{
	assert( netFrameMgr );
	mNetFrameMgr = netFrameMgr;
}

CFrameActionEngine::~CFrameActionEngine()
{
	mNetFrameMgr->release();
}

bool CFrameActionEngine::build( BuildParam& buildParam )
{
	mWorker   = buildParam.worker;
	mTickTime = buildParam.tickTime;

	// Add InputControl to frame manager's collection processor
	buildParam.game->getInputControl().setupInput(mNetFrameMgr->getCollectionProcessor());
	
	// Attach frame manager to main processor (registers as IActionInput for execution)
	mNetFrameMgr->attachTo(*buildParam.processor);

	return true;
}

void CFrameActionEngine::update( IFrameUpdater& updater , long time )
{
	if ( mWorker->getActionState() == NAS_LEVEL_RUN )
	{
		int updateFrames = time / mTickTime;
		int maxDelayFrames = mWorker->getNetLatency() / mTickTime ;
		int curFrame = mNetFrameMgr->evalFrame( updater , updateFrames , maxDelayFrames );
	}
	else
	{




	}
}

void CFrameActionEngine::setupInputAI( IPlayerManager& manager , ActionProcessor& processor )
{
	for( IPlayerManager::Iterator iter = manager.createIterator(); iter ; ++iter )
	{
		GamePlayer* player = iter.getElement();
		if ( player->getAI() )
		{
			IActionInput* input = player->getAI()->getActionInput();
			if ( input )
				processor.addInput( *input );
		}
	}
}

void CFrameActionEngine::restart()
{
	mNetFrameMgr->resetFrameData();
}
