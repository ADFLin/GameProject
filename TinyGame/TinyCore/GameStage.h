#ifndef GameStage_h__
#define GameStage_h__

#include "StageBase.h"

#include "GameConfig.h"
#include "GameDefines.h"
#include "GameControl.h"

#include "Holder.h"
#include "CppVersion.h"

struct GameAttribute;

#define  LAST_REPLAY_NAME     "LastReplay.rpf"
#define  REPLAY_DIR           "Replay"

enum class GameState
{
	Start,
	Run,
	Pause,
	End,
};

struct GameLevelInfo
{
	uint64 seed;
	void*  data;
	size_t numData;

	GameLevelInfo()
	{
		seed = 0;
		data = nullptr;
		numData = 0;
	}
};

class LocalPlayerManager;

class GameStageMode;
class INetEngine;
class IFrameActionTemplate;

class GameStageBase : public StageBase
{
	using BaseClass = StageBase;
public:

	GameStageBase();
	virtual bool setupNetwork(NetWorker* worker, INetEngine** engine) { return true; }
	virtual void buildServerLevel(GameLevelInfo& info) {}
	virtual void buildLocalLevel(GameLevelInfo& info) {}

	virtual void setupLocalGame(LocalPlayerManager& playerManager) {}
	virtual void setupLevel(GameLevelInfo const& info) {}
	virtual void setupScene(IPlayerManager& playerManager) {}


	virtual bool queryAttribute(GameAttribute& value) { return false; }
	virtual bool setupAttribute(GameAttribute const& value) { return false; }

	bool onInit() override;
	void onEnd() override;
	
	void onRender(float dFrame) override {}
	void onUpdate(long time) override;

	virtual void onRestart(bool beInit) {}
	virtual void tick() {}
	virtual void updateFrame(int frame) {}

	bool onMouse(MouseMsg const& msg) override { return true; }
	bool onChar(unsigned code) override { return true; }
	bool onKey(unsigned key, bool isDown) override;
	bool onWidgetEvent(int event, int id, GWidget* ui) override;

	GameStageBase* getGameStage() override { return this; }

	virtual void onChangeState(GameState state) {}
	virtual IFrameActionTemplate* createActionTemplate(unsigned version) { return nullptr; }           

	void             setupStageMode(GameStageMode* mode);
	GameStageMode*   getStageMode() { return mStageMode; }

	ActionProcessor& getActionProcessor() { return mProcessor; }
	IGameModule*     getGame() { return mGame; }
	long             getTickTime() { return mTickTime; }

	bool             changeState(GameState state);
	GameState        getGameState() const;
	StageModeType    getModeType() const;

	GameStageMode*   mStageMode;
	ActionProcessor  mProcessor;
	IGameModule*     mGame;
	long             mTickTime;
};

#endif // GameStage_h__
