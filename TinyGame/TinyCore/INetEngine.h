#ifndef INetEngine_h__
#define INetEngine_h__

class NetWorker;
class ComWorker;
class ActionProcessor;
class IPlayerManager;
class IGameModule;


using FrameStateHandle = int;
class IFameStateContainer
{
public:
	virtual FrameStateHandle allocState() = 0;
	virtual void* getState(FrameStateHandle handle) = 0;
	virtual void  freeState(FrameStateHandle handle) = 0;
};


class IFrameUpdater
{
public:
	virtual void tick() = 0;
	virtual void updateFrame( int frame ) = 0;
};


class INetEngine
{
public:
	struct BuildParam
	{
		NetWorker*       netWorker;
		ComWorker*       worker;
		ActionProcessor* processor;
		IGameModule*     game;
		long             tickTime;
	};
	virtual bool build( BuildParam& buildParam ) = 0;
	virtual void update( IFrameUpdater& updater , long time ) = 0;
	virtual void close(){}
	virtual void restart(){}
	virtual void setupInputAI( IPlayerManager& manager ) = 0;
	virtual void release() = 0;
};

#endif // INetEngine_h__
