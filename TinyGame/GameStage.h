#ifndef GameStage_h__
#define GameStage_h__

#include "StageBase.h"

#include "GameConfig.h"
#include "GameDefines.h"
#include "GameControl.h"
#include "GameMode.h"


#include "THolder.h"

#include "CppVersion.h"

#define  LAST_REPLAY_NAME     "LastReplay.rpf"
#define  REPLAY_DIR           "Replay"

struct AttribValue;


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



#endif // GameStage_h__
