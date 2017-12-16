#pragma once
#ifndef LeelaZeroLearningStage_H_0CE72905_A16A_4C43_8495_8413EC3C9F82
#define LeelaZeroLearningStage_H_0CE72905_A16A_4C43_8495_8413EC3C9F82

#include "TestStageHeader.h"

#include "Go/GoCore.h"
#include "Go/GoRenderer.h"

#include "Thread.h"
#include "Platform/Windows/WindowsProcess.h"


using namespace RenderGL;
using namespace Go;


#define DETECT_LEELA_PROCESS 1

class LeelaGame : public Go::Game
{
public:
};


namespace GameComId
{
	enum
	{
		Restart = -1,
		Pass = -2,
		Resign = -3,
	};
}
struct GameProcessCommad
{
	union
	{
		int id;
		int pos[2];
	};
};
class IGameProcessThread : public RunnableThreadT< IGameProcessThread >
{
public:
	virtual ~IGameProcessThread() {}
	virtual unsigned run() = 0;


	ChildProcess* process = nullptr;
	void addCommand(GameProcessCommad const& com)
	{
		Mutex::Locker lock(mComMutex);
		mCommands.push_back(com);
	}
	template< class T >
	void procCommand(T fun)
	{
		Mutex::Locker lock(mComMutex);
		for( GameProcessCommad& com : mCommands )
		{
			fun(com);
		}
		mCommands.clear();
	}
	std::vector< GameProcessCommad > mCommands;
	Mutex mComMutex;
};


class LeelaZeroLearningStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	LeelaZeroLearningStage() {}

	enum
	{
		UI_TOGGLE_PAUSE_GAME = BaseClass::NEXT_UI_ID,
		UI_RUN_LEARNING,
		UI_PLAY_GAME,
		NEXT_UI_ID,
	};

	IGameProcessThread* mProcessThread = nullptr;


	ChildProcess mGameRunProcess;
	bool bProcessPaused = false;
	int  NumLearnedGame = 0;

	char const* mLeelaZeroDir;
	LeelaGame mGame;

#if DETECT_LEELA_PROCESS
	DWORD  mPIDLeela = -1;
	static long const RestartTime = 20000;
	long   mRestartTimer = RestartTime;
#endif

	GameRenderer mGameRenderer;


	virtual bool onInit();

	virtual void onEnd();
	virtual void onUpdate(long time);

	void onRender(float dFrame);


	virtual bool onWidgetEvent(int event, int id, GWidget* ui) override;

	bool onMouse(MouseMsg const& msg);


	virtual bool onKey(unsigned key, bool isDown) override;


	void restart() {}
	void tick(){}
	void updateFrame(int frame) {}

	bool buildLearningProcess();
	bool buildPlayProcess();
	void killGameProcess();


	void keepLeelaProcessRunning(long time);

	void processGameCommand();

	std::string getBestWeightName();



	bool bPlayMode = false;
	bool bPlayerControl[2] = { true , false };

	Vec2i const BoardPos = Vec2i(50, 50);

	void playStone(int color, Vec2i const& pos)
	{
		assert(isPlayerControl(color));
		FixString<128> com;
		char coord = 'A' + pos.x;
		if( coord >= 'I' )
			++coord;
		com.format("play %c %c%d\n", (color == Board::eBlack ? 'b' : 'w'), coord, pos.y + 1);
		inputProcessStream(com);
		checkLeelaPlay();
	}

	void checkLeelaPlay()
	{
		if( isLeelaTurn() )
		{
			FixString<128> com;
			com.format("genmove %s\n", ((mGame.getNextPlayColor() == Board::eBlack) ? "b" : "w"));
			inputProcessStream(com);
		}
	}

	bool isLeelaTurn()
	{
		return !isPlayerControl(mGame.getNextPlayColor());
	}
	bool isPlayerControl(int color)
	{
		return bPlayerControl[color - 1];
	}
	bool inputProcessStream(char const* command)
	{
		int numWrite = 0;
		return mGameRunProcess.writeInputStream(command, strlen(command), numWrite);
	}

	void startPlayGame()
	{
		mGame.restart();
		checkLeelaPlay();
	}


};

#endif // LeelaZeroLearningStage_H_0CE72905_A16A_4C43_8495_8413EC3C9F82
