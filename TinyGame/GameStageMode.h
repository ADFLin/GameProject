#ifndef GameStageMode_h__
#define GameStageMode_h__

#include "StageBase.h"
#include "GameStage.h"
#include "GameDefines.h"
#include "GameConfig.h"
#include "GameControl.h"

#include "THolder.h"
#include "CppVersion.h"



class  IGameInstance;
class  IReplayRecorder;
class  IPlayerManager;
class  GameStageBase;

class GameStageMode
{
public:

	enum
	{
		NEXT_UI_ID = UI_GAME_STAGE_MODE_ID,
	};
	GameStageMode(StageModeType mode);
	virtual ~GameStageMode() {}
	virtual bool prevStageInit() { return true; }
	virtual bool postStageInit() {  return true; }
	virtual void onEnd(){}
	virtual void onRestart(uint64& seed) {}
	virtual bool onKey(unsigned key, bool isDown) { return true;  }
	virtual bool onWidgetEvent(int event, int id, GWidget* ui) { return true; }

	virtual void restart(bool beInit);

	virtual void   updateTime(long time){}
	virtual bool   canRender() { return true; }
	virtual bool   saveReplay(char const* name) { return false; }
	virtual IPlayerManager* getPlayerManager() = 0;
	virtual bool   tryChangeState(GameState state) { return true; }
	
	StageModeType getModeType() const { return mStageMode;  }
	bool  changeState(GameState state);
	bool  togglePause();

	GameStageBase* getStage() { return mCurStage; }
	IGameInstance* getGame() { return mCurStage->getGame(); }
	GameState      getGameState() { return mGameState; }
	StageManager*  getManager() { return mCurStage->getManager();  }

	GameState      mGameState;
	StageModeType const mStageMode;
	GameStageBase* mCurStage;
	long           mReplayFrame;

protected:

	void restartImpl(bool bInit);
};


class LevelStageMode : public GameStageMode
{
	typedef GameStageMode BaseClass;
public:
	LevelStageMode(StageModeType mode);

	void   onRestart(uint64& seed);
	bool   saveReplay(char const* name);

protected:
	bool buildReplayRecorder();
	TPtrHolder< IReplayRecorder > mReplayRecorder;
};

class IReplayModeInterface
{

};

class INetModeInterface
{

};

class ILocalModeInterface
{


};



#endif // GameStageMode_h__
