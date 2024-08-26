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

class ReplayStageMode;
class NetLevelStageMode;

class GameStageMode
{
public:

	enum
	{
		NEXT_UI_ID = UI_GAME_STAGE_MODE_ID,
	};
	GameStageMode(EGameStageMode mode);
	virtual ~GameStageMode();
	virtual bool prevStageInit() { return true; }
	virtual bool postStageInit() {  return true; }
	virtual void onEnd(){}
	virtual void onRestart(uint64& seed) {}
	virtual MsgReply onKey(KeyMsg const& msg) { return MsgReply::Unhandled();  }
	virtual bool onWidgetEvent(int event, int id, GWidget* ui) { return true; }

	virtual void restart(bool beInit);

	virtual void   updateTime(long time){}
	virtual bool   canRender() { return true; }
	virtual bool   saveReplay(char const* name) { return false; }
	virtual IPlayerManager* getPlayerManager() = 0;
	virtual bool   prevChangeState(EGameState state) { return true; }

	virtual ReplayStageMode* getReplayMode() { return nullptr; }
	virtual NetLevelStageMode* getNetLevelMode() { return nullptr; }
	
	EGameStageMode getModeType() const { return mStageMode;  }
	bool  changeState(EGameState state);
	bool  togglePause();

	GameStageBase* getStage() { return mCurStage; }
	IGameModule*   getGame() { return mCurStage->getGame(); }
	EGameState     getGameState() { return mGameState; }
	StageManager*  getManager() { return mCurStage->getManager();  }

	EGameState      mGameState;
	EGameStageMode const mStageMode;
	GameStageBase* mCurStage;
	long           mReplayFrame;

protected:

	void doRestart(bool bInit);
};


class LevelStageMode : public GameStageMode
{
	using BaseClass = GameStageMode;
public:
	LevelStageMode(EGameStageMode mode);
	~LevelStageMode();
	void   onRestart(uint64& seed) override;
	bool   saveReplay(char const* name) override;

protected:
	bool buildReplayRecorder();
	TPtrHolder< IReplayRecorder > mReplayRecorder;
};

class IGameReplayFeature
{

};

class IGameNetFeature
{

};


#endif // GameStageMode_h__
