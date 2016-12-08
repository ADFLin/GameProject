#ifndef GameSingleStage_h__
#define GameSingleStage_h__

#include "GameStageMode.h"

#include "GamePlayer.h"
#include "THolder.h"

class SingleStageMode : public LevelStageMode
{
	typedef LevelStageMode BaseClass;
public:
	SingleStageMode();

	bool   postStageInit();
	void   onRestart(uint64& seed);
	void   updateTime(long time);
	bool   tryChangeState(GameState state);
	bool   onWidgetEvent(int event, int id, GWidget* ui);
	
	LocalPlayerManager* getPlayerManager();
	TPtrHolder< LocalPlayerManager > mPlayerManager;
};

#endif // GameSingleStage_h__