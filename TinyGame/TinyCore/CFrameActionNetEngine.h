#ifndef CFrameActionNetEngine_h__
#define CFrameActionNetEngine_h__


#include "GameWorker.h"
#include "GameControl.h"

#include "INetEngine.h"

class INetFrameManager : public IActionInput
{
public:
	virtual ActionProcessor& getActionProcessor() = 0;
	virtual bool sendFrameData() = 0;
	virtual int  evalFrame( IFrameUpdater& updater , int updateFrames , int maxDelayFrames )= 0;
	virtual void resetFrameData() = 0;

	//ActionInput
	virtual bool scanInput( bool beUpdateFrame ) = 0;
	virtual bool checkAction( ActionParam& param ) = 0;

	virtual void release() = 0;
};



class  CFrameActionEngine : public INetEngine
{
public:
	GAME_API CFrameActionEngine( INetFrameManager* netFrameMgr );
	GAME_API ~CFrameActionEngine();

	bool build( BuildParam& buildParam );
	void update( IFrameUpdater& updater , long time );
	void setupInputAI( IPlayerManager& manager );
	void restart();
	void release(){ delete this; }
	ComWorker*        mWorker;
	long              mTickTime;
	INetFrameManager* mNetFrameMgr;
};



#endif // CFrameActionNetEngine_h__
