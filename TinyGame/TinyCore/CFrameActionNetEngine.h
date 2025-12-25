#ifndef CFrameActionNetEngine_h__
#define CFrameActionNetEngine_h__


#include "GameWorker.h"
#include "GameControl.h"

#include "INetEngine.h"

class ActionProcessor;

class INetFrameManager
{
public:
	// Attach this manager to a main ActionProcessor
	// The manager registers as IActionInput to provide synced data during execution
	virtual void attachTo(ActionProcessor& processor) = 0;

	// Get the processor used for input collection phase
	virtual ActionProcessor& getCollectionProcessor() = 0;

	virtual int  evalFrame( IFrameUpdater& updater , int updateFrames , int maxDelayFrames )= 0;
	virtual void resetFrameData() = 0;

	virtual void release() = 0;
};



class  CFrameActionEngine : public INetEngine
{
public:
	TINY_API CFrameActionEngine( INetFrameManager* netFrameMgr );
	TINY_API ~CFrameActionEngine();

	bool build( BuildParam& buildParam );
	void update( IFrameUpdater& updater , long time );
	void setupInputAI( IPlayerManager& manager , ActionProcessor& processor );
	void restart();
	void release(){ delete this; }
	ComWorker*        mWorker;
	long              mTickTime;
	INetFrameManager* mNetFrameMgr;
};



#endif // CFrameActionNetEngine_h__
