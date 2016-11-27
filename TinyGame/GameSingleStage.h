#ifndef GameSingleStage_h__
#define GameSingleStage_h__

#include "GameStageMode.h"
#include "GameStage.h"
#include "GamePlayer.h"

#include "THolder.h"

class  GameSingleStage : public GameLevelStage
{
	typedef GameLevelStage BaseClass;
public:
	enum
	{

	};

	GameSingleStage();

	bool   onInit();
	void   onUpdate( long time );
	bool   onWidgetEvent( int event , int id , GWidget* ui );
	bool   tryChangeState( GameState state );
	void   onRestart( uint64& seed );
	void   onRender( float dFrame );
	LocalPlayerManager* getPlayerManager();
private:
	TPtrHolder< LocalPlayerManager > mPlayerManager;
	
};

class SingleStageMode : public LevelStageMode
{
	typedef LevelStageMode BaseClass;
public:
	SingleStageMode();

	bool   onInit();
	void   onRestart(uint64& seed);
	void   updateTime(long time);
	bool   tryChangeState(GameState state);
	bool   onWidgetEvent(int event, int id, GWidget* ui);
	
	LocalPlayerManager* getPlayerManager();
	TPtrHolder< LocalPlayerManager > mPlayerManager;
};

#endif // GameSingleStage_h__