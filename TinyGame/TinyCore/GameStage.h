#ifndef GameStage_h__
#define GameStage_h__

#include "StageBase.h"

#include "GameConfig.h"
#include "GameDefines.h"
#include "GameControl.h"

#include "Holder.h"
#include "CppVersion.h"

struct AttribValue;

#define  LAST_REPLAY_NAME     "LastReplay.rpf"
#define  REPLAY_DIR           "Replay"

enum GameState
{
	GS_START,
	GS_RUN,
	GS_PAUSE,
	GS_END,
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
	typedef StageBase BaseClass;
public:

	GameStageBase();
	virtual bool setupNetwork(NetWorker* worker, INetEngine** engine) { return true; }
	virtual void buildServerLevel(GameLevelInfo& info) {}

	virtual void setupLocalGame(LocalPlayerManager& playerManager) {}
	virtual void setupLevel(GameLevelInfo const& info) {}
	virtual void setupScene(IPlayerManager& playerManager) {}


	virtual bool getAttribValue(AttribValue& value) { return false; }
	virtual bool setupAttrib(AttribValue const& value) { return false; }

	virtual bool onInit();
	virtual void onEnd();
	virtual void onRestart(uint64 seed, bool beInit) {}
	virtual void onRender(float dFrame) {}
	virtual void onUpdate(long time);

	virtual void tick() {}
	virtual void updateFrame(int frame) {}

	virtual bool onMouse(MouseMsg const& msg) { return true; }
	virtual bool onChar(unsigned code) { return true; }
	virtual bool onKey(unsigned key, bool isDown);
	virtual bool onWidgetEvent(int event, int id, GWidget* ui);

	virtual void onChangeState(GameState state) {}
	virtual IFrameActionTemplate* createActionTemplate(unsigned version) { return NULL; }           

	void             setupStageMode(GameStageMode* mode);
	GameStageMode*   getStageMode() { return mStageMode; }

	ActionProcessor& getActionProcessor() { return mProcessor; }
	IGameInstance*   getGame() { return mGame; }
	long             getTickTime() { return mTickTime; }

	bool             changeState(GameState state);
	GameState        getGameState() const;
	StageModeType    getModeType() const;

	GameStageMode*   mStageMode;
	ActionProcessor  mProcessor;
	IGameInstance*   mGame;
	long             mTickTime;
};

#endif // GameStage_h__
