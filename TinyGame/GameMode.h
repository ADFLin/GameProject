#ifndef GameMode_h__da1aa5ac_75a7_41c9_b095_517ce0f26670
#define GameMode_h__da1aa5ac_75a7_41c9_b095_517ce0f26670

#include "StageBase.h"

#include "GameDefines.h"
#include "GameConfig.h"
#include "GameControl.h"
#include "GameMode.h"

#include "THolder.h"
#include "CppVersion.h"

class GameMode
{
public:
	enum
	{
		NEXT_UI_ID = UI_GAME_MODE_ID ,
	};
	virtual bool onWidgetEvent( int event , int id , GWidget* ui )
	{
		return true;
	}
};

enum GameState
{
	GS_START  ,
	GS_RUN    ,
	GS_PAUSE  ,
	GS_END    ,
};


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

	virtual bool onChar( unsigned code );
	virtual bool onMouse( MouseMsg const& msg );
	virtual bool onKey( unsigned key , bool isDown );
	virtual bool onWidgetEvent( int event , int id , GWidget* ui );

	virtual bool   tryChangeState( GameState state ){ return true; }


	ActionProcessor  mProcessor;
	long             mTickTime;
	long             mReplayFrame;
	GameSubStage*    mSubStage;
	GameType const   mGameType;
	IGamePackage*    mGame;
	GameState        mGameState;
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

class IReplayModeInterface
{

};

class INetModeInterface
{

};

class ILocalModeInterface
{


};



#endif // GameMode_h__da1aa5ac_75a7_41c9_b095_517ce0f26670
