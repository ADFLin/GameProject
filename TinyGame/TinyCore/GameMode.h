#ifndef GameStageMode_h__
#define GameStageMode_h__

#include "StageBase.h"
#include "GameStage.h"
#include "GameDefines.h"
#include "GameConfig.h"
#include "GameControl.h"

#include "Holder.h"
#include "CppVersion.h"


class  IGameModule;
class  IReplayRecorder;
class  IPlayerManager;
class  GameStageBase;

class ReplayGameMode;
class NetGameMode;

class GameModeBase
{
public:

	enum
	{
		NEXT_UI_ID = UI_GAME_STAGE_MODE_ID,
	};
	GameModeBase(EGameMode mode);
	virtual ~GameModeBase();
	
	// Initialize the mode itself (before stage creation or independently)
	// Override this to setup mode-specific resources that don't depend on Stage
	virtual bool initialize() { return true; }
	
	// Called when Stage is created to setup stage-specific resources
	// This should call stage->onInit() and setup scene/UI
	virtual bool initializeStage(GameStageBase* stage) { return true; }
	virtual void onEnd(){}
	virtual void onRestart(uint64& seed) {}
	virtual MsgReply onKey(KeyMsg const& msg) { return MsgReply::Unhandled();  }
	virtual bool onWidgetEvent(int event, int id, GWidget* ui) { return true; }

	virtual void restart(bool beInit);

	virtual void   updateTime(GameTimeSpan deltaTime){}
	virtual bool   canRender() { return true; }
	virtual bool   saveReplay(char const* name) { return false; }
	virtual IPlayerManager* getPlayerManager() = 0;
	virtual bool   prevChangeState(EGameState state) { return true; }

	virtual ReplayGameMode* getReplayMode() { return nullptr; }
	virtual NetGameMode* getNetLevelMode() { return nullptr; }
	
	EGameMode getModeType() const { return mStageMode;  }
	bool  changeState(EGameState state);
	bool  togglePause();

	GameStageBase* getStage() { return mCurStage; }
	IGameModule*   getGame() { return mCurStage ? mCurStage->getGame() : nullptr; }
	EGameState     getGameState() { return mGameState; }

	// Action Processing - Mode controls input processing
	ActionProcessor& getActionProcessor() { return mProcessor; }

	EGameState      mGameState;
	EGameMode const mStageMode;
	GameStageBase* mCurStage;
	long           mReplayFrame;
	ActionProcessor mProcessor;

protected:

	void doRestart(bool bInit);
};


class GameLevelMode : public GameModeBase
{
	using BaseClass = GameModeBase;
public:
	GameLevelMode(EGameMode mode);
	~GameLevelMode();
	void   onRestart(uint64& seed) override;
	bool   saveReplay(char const* name) override;

protected:
	bool buildReplayRecorder(GameStageBase* Stage);
	TPtrHolder< IReplayRecorder > mReplayRecorder;
};

class IGameReplayFeature
{

};

class IGameNetFeature
{

};


#endif // GameStageMode_h__
