#ifndef INetEngine_h__
#define INetEngine_h__

class NetWorker;
class ComWorker;
class ActionProcessor;
class IPlayerManager;
class IGameModule;
class IFrameUpdater;

using FrameStateHandle = int;
class IFameStateContainer
{
public:
	virtual FrameStateHandle allocState() = 0;
	virtual void* getState(FrameStateHandle handle) = 0;
	virtual void  freeState(FrameStateHandle handle) = 0;
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

		// Dedicated server mode - engine should create internal World
		// and provide its own IFrameUpdater implementation
		bool             bDedicatedServer = false;
	};

	virtual bool build( BuildParam& buildParam ) = 0;
	virtual void update( IFrameUpdater& updater , long time ) = 0;
	virtual void close(){}
	virtual void restart(){}
	virtual void setupInputAI( IPlayerManager& manager , ActionProcessor& processor ) = 0;
	virtual void release() = 0;
	
	// Called by DedicatedServerMode when game is ready to start
	// Used to trigger initial game state setup (e.g., world init)
	virtual void onGameStart() {}
	
	// Get internal IFrameUpdater for dedicated server mode
	// Returns nullptr if not in dedicated server mode or not supported
	virtual IFrameUpdater* getDedicatedUpdater() { return nullptr; }
	
	// Configure level settings for network sync (used by DedicatedServerMode)
	virtual void configLevelSetting(struct GameLevelInfo& info) {}
};

#endif // INetEngine_h__
