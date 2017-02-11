#pragma once
#ifndef SingleStageMode_H_2188C010_CF34_4BDF_9C40_0FAC3FF0E33C
#define SingleStageMode_H_2188C010_CF34_4BDF_9C40_0FAC3FF0E33C

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


#endif // SingleStageMode_H_2188C010_CF34_4BDF_9C40_0FAC3FF0E33C
