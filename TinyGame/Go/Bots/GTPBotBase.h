#pragma once
#ifndef GTPBotBase_H_DC612B9E_D3AD_4F39_A6B9_114AD96E29AB
#define GTPBotBase_H_DC612B9E_D3AD_4F39_A6B9_114AD96E29AB

#include "Go/GoBot.h"
#include "PlatformThread.h"
#include "Platform/Windows/WindowsProcess.h"

namespace Go
{
	struct GTPCommand
	{
		enum Id
		{
			eNone,
			eKomi,
			eHandicap,
			eRestart,
			ePlay,
			eAdd,
			eGenmove,
			ePass,
			eUndo,
			eFinalScore,
			eShowBoard,
			eQuit,

			eStartPonder,
			eStopPonder,
		};

		Id  id;
		int meta;
	};

	FORCEINLINE char ToColorChar(int color)
	{
		return color == StoneColor::eBlack ? 'b' : 'w';
	}

	FORCEINLINE char ToCoordChar(int x)
	{
		assert(x < 25);
		char coord = 'A' + x;
		if (coord >= 'I')
			++coord;
		return coord;
	}

	class IGameOutputThread : public RunnableThreadT< IGameOutputThread >
	{
	public:
		virtual ~IGameOutputThread() {}
		virtual unsigned run() = 0;
		virtual void update() {}
		virtual void restart()
		{
			bRequestRestart = true;
			mOutputCommands.clear();
		}


		ChildProcess* process = nullptr;
		void addOutputCommand(GameCommand const& com)
		{
			mOutputCommands.push_back(com);
		}
		template< class TFunc >
		void procOutputCommand(TFunc&& func)
		{
			bRequestRestart = false;
			for (GameCommand& com : mOutputCommands)
			{
				func(com);
				if (bRequestRestart)
					break;
			}
			mOutputCommands.clear();
		}


		bool bRequestRestart = false;
		std::vector< GameCommand > mOutputCommands;

	};

	class GTPLikeAppRun
	{
	public:
		ChildProcess        process;
		IGameOutputThread*  outputThread = nullptr;
		bool  bThinking = false;

		~GTPLikeAppRun();

		void update()
		{
			if (outputThread)
				outputThread->update();
		}

		void stop();

		bool restart();
		bool playStone(int x, int y, int color);
		bool addStone(int x, int y, int color);
		bool playPass(int color = StoneColor::eEmpty);
		bool thinkNextMove(int color);
		bool undo();
		bool requestUndo();
		bool setupGame(GameSetting const& setting);
		bool showResult();
		bool readBoard(int* outData);

		bool inputCommand(char const* command, GTPCommand com);
		bool inputProcessStream(char const* command, int length = 0);

		template< class T >
		std::enable_if_t< std::is_base_of_v< IGameOutputThread , T > , bool>
		buildProcessT(char const* appPath, char const* command)
		{
			if (!process.create(appPath, command))
				return false;

			auto myThread = new T;
			myThread->process = &process;
			myThread->start();
			myThread->setDisplayName("Output Thread");
			outputThread = myThread;

			return true;
		}
	};


	template< class T >
	class TGTPLikeBot : public IBot
	{
	public:
		void destroy() override
		{
			mAI.stop();
		}
		bool setupGame(GameSetting const& setting) override
		{
			return mAI.setupGame(setting);
		}
		bool restart() override
		{
			return mAI.restart();
		}
		bool playStone(int x, int y, int color) override
		{
			return mAI.playStone(x, y, color);
		}
		bool playPass(int color) override
		{
			return mAI.playPass(color);
		}
		bool undo() override
		{
			return mAI.undo();
		}
		bool requestUndo() override
		{
			return mAI.requestUndo();
		}
		bool thinkNextMove(int color) override
		{
			return mAI.thinkNextMove(color);
		}
		bool isThinking() override
		{
			//TODO
			return false;
		}
		void update(IGameCommandListener& listener) override
		{
			mAI.update();
			auto MyFun = [&](GameCommand const& com)
			{
				listener.notifyCommand(com);
			};
			mAI.outputThread->procOutputCommand(MyFun);
		}

		EBotExecuteResult readBoard(int* outState) override { mAI.readBoard(outState); return BOT_WAIT; }

		bool inputProcessStream(char const* command)
		{
			int numWrite = 0;
			return mAI.process.writeInputStream(command, strlen(command), numWrite);
		}
		T mAI;
	};


}//namespace Go

#endif // GTPBotBase_H_DC612B9E_D3AD_4F39_A6B9_114AD96E29AB