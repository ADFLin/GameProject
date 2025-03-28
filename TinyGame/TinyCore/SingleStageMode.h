#pragma once
#ifndef SingleStageMode_H_2188C010_CF34_4BDF_9C40_0FAC3FF0E33C
#define SingleStageMode_H_2188C010_CF34_4BDF_9C40_0FAC3FF0E33C

#include "GameStageMode.h"

#include "GamePlayer.h"
#include "Holder.h"

class SingleStageMode : public LevelStageMode
{
	typedef LevelStageMode BaseClass;
public:
	SingleStageMode();
	~SingleStageMode();

	bool   postStageInit();
	void   onRestart(uint64& seed);
	void   updateTime(GameTimeSpan deltaTime);
	bool   prevChangeState(EGameState state);
	bool   onWidgetEvent(int event, int id, GWidget* ui);
	
	LocalPlayerManager* getPlayerManager();
	TPtrHolder< LocalPlayerManager > mPlayerManager;
	float mDeltaTimeAcc = 0.0;
};


#endif // SingleStageMode_H_2188C010_CF34_4BDF_9C40_0FAC3FF0E33C
