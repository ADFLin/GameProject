#ifndef GameStage_h__
#define GameStage_h__

#include "GameConfig.h"
#include "GameControl.h"
#include "StageBase.h"

#include "THolder.h"

#define  LAST_REPLAY_NAME     "LastReplay.rpf"
#define  REPLAY_DIR           "Replay"

struct AttribValue;

class  ComWorker;
class  NetWorker;
class  GameSubStage;
class  IFrameActionTemplate;
class  INetFrameManager;
class  INetEngine;
class  IFrameUpdater;
class  IReplayRecorder;
class  IPlayerManager;
class  LocalPlayerManager;

enum GameState
{
	GS_START  ,
	GS_RUN    ,
	GS_PAUSE  ,
	GS_END    ,
};


enum GameType
{
	GT_UNKNOW      ,
	GT_SINGLE_GAME ,
	GT_NET_GAME    ,
	GT_REPLAY      ,
};

class LocalPlayerManager;

class GameStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	GameStage( GameType gameType );

	void           restart( bool beInit );
	IGamePackage*  getGame()    { return mGame; }
	GameState      getState()   { return mGameState;  }
	GameType       getGameType(){ return mGameType;  }
	long           getTickTime(){ return mTickTime;  }
	bool           changeState( GameState state );
	bool           togglePause();
	GameSubStage*  getSubStage(){ return mSubStage;  }
	void           setSubStage( GameSubStage* subStage );
	virtual bool   saveReplay( char const* name ){ return false;  }

	ActionProcessor& getActionProcessor(){ return mProcessor; }

	virtual IPlayerManager* getPlayerManager() = 0;

protected:
	virtual void   onRestart( uint64& seed ){}
	virtual bool   onInit();
	virtual void   onEnd();
	virtual void   onRender( float dFrame );

	virtual bool   onMouse( MouseMsg const& msg );

	virtual bool   tryChangeState( GameState state ){ return true; }
	virtual bool   onEvent( int event , int id , GWidget* ui );

	ActionProcessor  mProcessor;
	long             mTickTime;
	long             mReplayFrame;
	GameSubStage*    mSubStage;
	GameType const   mGameType;
	IGamePackage*    mGame;
	GameState        mGameState;
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
	virtual void setupServerLevel(){}

	virtual void setupLocalGame( LocalPlayerManager& playerManager ){}
	virtual void setupScene( IPlayerManager& playerManager ){}

	virtual bool getAttribValue( AttribValue& value );
	virtual bool setupAttrib( AttribValue const& value );

	virtual bool onInit(){ return true; }
	virtual void onEnd(){}
	virtual void onRestart( uint64 seed , bool beInit ){}
	virtual void onRender( float dFrame ){}

	virtual bool onMouse( MouseMsg const& msg );

	virtual void onChangeState( GameState state ){}
	virtual IFrameActionTemplate* createActionTemplate( unsigned version ){ return NULL; }



public:
	GameStage*   getStage()  {  return mGameStage;  }

protected:
	virtual bool onWidgetEvent( int event , int id , GWidget* ui ){ return true; }

public:
	//Helper function
	GameState       getState()   { return getStage()->getState(); }
	GameType        getGameType(){ return getStage()->getGameType(); }
	StageManager*   getManager() { return getStage()->getManager(); }
	IGamePackage*   getGame()    { return getStage()->getGame(); }
	long            getTickTime(){ return getStage()->getTickTime(); }
	bool            changeState( GameState state ){ return getStage()->changeState( state );  }
	IPlayerManager* getPlayerManager(){ return getStage()->getPlayerManager();  }

protected:
	friend class GameStage;
	GameStage*  mGameStage;
};

class  GameLevelStage : public GameStage
{
	typedef GameStage BaseClass;
public:
	GameLevelStage( GameType gameType = GT_UNKNOW );
	~GameLevelStage();
	bool saveReplay( char const* name );
protected:
	void   onRestart( uint64& seed );
	bool   buildReplayRecorder();
	TPtrHolder< IReplayRecorder > mReplayRecorder;
};


#endif // GameStage_h__
