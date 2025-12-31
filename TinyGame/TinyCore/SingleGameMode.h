#pragma once
#ifndef SingleGameMode_H_1DF2CCEA_EF32_4270_A1B0_11DE41CDA15C
#define SingleGameMode_H_1DF2CCEA_EF32_4270_A1B0_11DE41CDA15C

#include "GameMode.h"

#include "GamePlayer.h"
#include "Holder.h"

class SingleGameMode : public GameLevelMode
{
	typedef GameLevelMode BaseClass;
public:
	SingleGameMode();
	~SingleGameMode();

	bool   initialize() override;
	bool   initializeStage(GameStageBase* stage) override;
	void   onRestart(uint64& seed);
	void   updateTime(GameTimeSpan deltaTime);
	bool   prevChangeState(EGameState state);
	bool   onWidgetEvent(int event, int id, GWidget* ui);
	
	LocalPlayerManager* getPlayerManager();
	TPtrHolder< LocalPlayerManager > mPlayerManager;
	float mDeltaTimeAcc = 0.0;
};


#endif // SingleGameMode_H_1DF2CCEA_EF32_4270_A1B0_11DE41CDA15C
