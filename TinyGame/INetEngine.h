#ifndef INetEngine_h__
#define INetEngine_h__

class NetWorker;
class ComWorker;
class ActionProcessor;
class GameSubStage;
class IPlayerManager;
class IGamePackage;

class IFrameUpdater
{
public:
	virtual void tick() = 0;
	virtual void updateFrame( int frame ) = 0;
};


class INetEngine
{
public:
	struct BuildInfo
	{
		NetWorker*       netWorker;
		ComWorker*       worker;
		ActionProcessor* processor;
		IGamePackage*    game;
		long             tickTime;
	};
	virtual bool build( BuildInfo& info ) = 0;
	virtual void update( IFrameUpdater& updater , long time ) = 0;
	virtual void close(){}
	virtual void setupInputAI( IPlayerManager& manager ) = 0;
	virtual void release() = 0;
};

#endif // INetEngine_h__
