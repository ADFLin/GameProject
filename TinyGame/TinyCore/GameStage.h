#ifndef GameStage_h__
#define GameStage_h__

#include "StageBase.h"

#include "GameConfig.h"
#include "GameDefines.h"
#include "GameControl.h"

#include "THolder.h"
#include "CppVersion.h"

struct AttribValue;


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
	IGamePackage*    getGame() { return mGame; }
	long             getTickTime() { return mTickTime; }

	bool             changeState(GameState state);
	GameState        getState() const;
	StageModeType         getGameType();

	GameStageMode*   mStageMode;
	ActionProcessor  mProcessor;
	IGamePackage*    mGame;
	long             mTickTime;
};

class GameSubStage;
class IReplayRecorder;

#include "THolder.h"

class GameStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	GameStage(StageModeType gameType);

	void           restart(bool beInit);
	IGamePackage*  getGame() { return mGame; }
	GameState      getState() { return mGameState; }
	StageModeType       getGameType() { return mGameType; }
	long           getTickTime() { return mTickTime; }
	bool           changeState(GameState state);
	bool           togglePause();
	GameSubStage*  getSubStage() { return mSubStage; }
	void           setSubStage(GameSubStage* subStage);
	virtual bool   saveReplay(char const* name) { return false; }

	ActionProcessor& getActionProcessor() { return mProcessor; }

	virtual IPlayerManager* getPlayerManager() = 0;

protected:
	virtual void   onRestart(uint64& seed) {}
	virtual bool   onInit();
	virtual void   onEnd();
	virtual void   onRender(float dFrame);

	virtual bool onChar(unsigned code);
	virtual bool onMouse(MouseMsg const& msg);
	virtual bool onKey(unsigned key, bool isDown);
	virtual bool onWidgetEvent(int event, int id, GWidget* ui);

	virtual bool   tryChangeState(GameState state) { return true; }


	ActionProcessor  mProcessor;
	long             mTickTime;
	long             mReplayFrame;
	GameSubStage*    mSubStage;
	StageModeType const   mGameType;
	IGamePackage*    mGame;
	GameState        mGameState;
};

class  GameLevelStage : public GameStage
{
	typedef GameStage BaseClass;
public:
	GameLevelStage(StageModeType gameType = SMT_UNKNOW);
	~GameLevelStage();
	bool saveReplay(char const* name);
protected:
	void   onRestart(uint64& seed);
	bool   buildReplayRecorder();
	TPtrHolder< IReplayRecorder > mReplayRecorder;
};


class GameSubStage
{
public:
	enum
	{
		NEXT_UI_ID = UI_SUB_STAGE_ID ,
	};
	GameSubStage(){ mGameStage = NULL; }
	virtual void tick(){};
	virtual void updateFrame( int frame ){};

	//Multi-Player
	virtual bool setupNetwork( NetWorker* worker , INetEngine** engine ){ return true; }
	virtual void buildServerLevel( GameLevelInfo& info ){}

	virtual void setupLocalGame( LocalPlayerManager& playerManager ){}
	virtual void setupLevel( GameLevelInfo const& info ){}
	virtual void setupScene( IPlayerManager& playerManager ){}
	

	virtual bool getAttribValue( AttribValue& value );
	virtual bool setupAttrib( AttribValue const& value );

	virtual bool onInit(){ return true; }
	virtual void onEnd(){}
	virtual void onRestart( uint64 seed , bool beInit ){}
	virtual void onRender( float dFrame ){}

	virtual bool onMouse( MouseMsg const& msg );
	virtual bool onChar(unsigned code){  return true;  }
	virtual bool onKey(unsigned key , bool isDown){  return true;  }
	virtual bool onWidgetEvent( int event , int id , GWidget* ui ){ return true; }

	virtual void onChangeState( GameState state ){}
	virtual IFrameActionTemplate* createActionTemplate( unsigned version ){ return NULL; }



public:
	GameStage*   getStage()  {  return mGameStage;  }


public:
	//Helper function
	GameState       getState()   { return getStage()->getState(); }
	StageModeType        getGameType(){ return getStage()->getGameType(); }
	StageManager*   getManager() { return getStage()->getManager(); }
	IGamePackage*   getGame()    { return getStage()->getGame(); }
	long            getTickTime(){ return getStage()->getTickTime(); }
	bool            changeState( GameState state ){ return getStage()->changeState( state );  }
	IPlayerManager* getPlayerManager(){ return getStage()->getPlayerManager();  }

protected:
	friend class GameStage;
	GameStage*  mGameStage;
};



#endif // GameStage_h__
