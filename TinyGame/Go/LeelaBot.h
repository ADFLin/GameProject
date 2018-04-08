#pragma once
#ifndef LeelaBot_H_42280575_AB74_4D08_8444_953C8BD1062B
#define LeelaBot_H_42280575_AB74_4D08_8444_953C8BD1062B

#include "GoBot.h"

#include "Thread.h"
#include "Platform/Windows/WindowsProcess.h"

#define LEELA_NET_DIR "networks/"

namespace Go
{
	int constexpr LeelaGoSize = 19;

	namespace LeelaGameParam
	{
		enum
		{
			eJobMode,
			eLastNetWeight,
			eMatchChallengerColor,
			eWinRate ,

			eBestMoveVertex ,
		};
	}

	struct GTPCommand
	{
		enum Id
		{
			eNone,
			eKomi,
			eHandicap,
			eRestart ,
			ePlay,
			eGenmove,
			ePass,
			eUndo,
			eFinalScore ,
			eQuit,
		};

		Id  id;
		int meta;
	};

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
		template< class T >
		void procOutputCommand(T fun)
		{
			bRequestRestart = false;
			for( GameCommand& com : mOutputCommands )
			{
				fun(com);
				if ( bRequestRestart )
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
			if (outputThread )
				outputThread->update();
		}

		void stop();

		bool restart();
		bool playStone(int x , int y, int color);
		bool playPass();
		bool thinkNextMove(int color);
		bool undo();
		bool setupGame(GameSetting const& setting);
		bool showResult();

		bool inputCommand(char const* command, GTPCommand com);
		bool inputProcessStream(char const* command, int length = 0);

		template< class T >
		bool buildProcessT(char const* appPath, char const* command)
		{
			if( !process.create(appPath, command) )
				return false;

			auto myThread = new T;
			myThread->process = &process;
			myThread->start();
			myThread->setDisplayName("Output Thread");
			outputThread = myThread;
			
			return true;
		}
	};

	struct LeelaAISetting
	{
		char const* weightName = nullptr;
		uint64 seed = 0;
		//Weaken engine by limiting the number of playouts. Requires --noponder.
		int playouts = 6400;//1600;
		//Play more randomly the first x moves.
		int randomcnt = 0;//30;
		//Number of threads to use.
		int numThread = 8;
		//Resign when winrate is less than x%. -1 uses 10% but scales for handicap
		int resignpct = 10;
		//Weaken engine by limiting the number of visits.
		int visits = 0;
		//Don't use heuristics for smarter passing.
		bool bDumbPass = true; 
		//Enable policy network randomization.
		bool bNoise = true;
		//Disable all diagnostic output.
		bool bQuiet = false;
		//Disable thinking on opponent's time.
		bool bNoPonder = true;

		bool bGTPMode = true;

		static LeelaAISetting GetDefalut();

		std::string toString() const
		{
			std::string result;

#define  AddCom( NAME , VALUE )\
			if( VALUE )\
			{\
				result += NAME;\
			}\

#define  AddComValue( NAME , VALUE )\
			if( VALUE )\
			{\
				result += NAME;\
				result += std::to_string(VALUE);\
			}\

			AddComValue(" -r ", resignpct);
			AddComValue(" -t ", numThread);
			AddComValue(" -p ", playouts);
			AddComValue(" -v ", visits);
			AddComValue(" -m ", randomcnt);
			AddComValue(" -s ", seed );
			if( weightName )
			{
				result += " -w " LEELA_NET_DIR;
				result += weightName;
			}
			AddCom(" -q", bQuiet);
			AddCom(" -d", bDumbPass);
			AddCom(" -n", bNoise);
			AddCom(" -g", bGTPMode);
			AddCom(" --noponder", bNoPonder);
			AddCom(" -q", bQuiet);
#undef AddCom
#undef AddComValue
			return result;
		}
	};

	struct LeelaAppRun : public GTPLikeAppRun
	{
		static char const* InstallDir;

		static std::string GetLastWeightName();
		static std::string GetDesiredWeightName();
		bool buildLearningGame();
		bool buildPlayGame(LeelaAISetting const& setting);
		bool buildAnalysisGame();
	};


	template< class T >
	class TGTPLikeBot : public IBotInterface
	{
	public:
		virtual void destroy()
		{
			mAI.stop();
		}
		virtual bool setupGame(GameSetting const& setting ) override
		{
			return mAI.setupGame(setting);
		}
		virtual bool restart() override
		{
			return mAI.restart();
		}
		virtual bool playStone(int x, int y, int color) override
		{
			return mAI.playStone(x , y, color);
		}
		virtual bool playPass(int color) override
		{
			return mAI.playPass();
		}
		virtual bool undo() override
		{
			return mAI.undo();
		}
		virtual bool thinkNextMove(int color) override
		{
			return mAI.thinkNextMove(color);
		}
		virtual bool isThinking() override
		{
			//TODO
			return false;
		}
		virtual void update(IGameCommandListener& listener) override
		{
			mAI.update();
			auto MyFun = [&](GameCommand const& com)
			{
				listener.notifyCommand(com);
			};
			mAI.outputThread->procOutputCommand(MyFun);
		}

		bool inputProcessStream(char const* command)
		{
			int numWrite = 0;
			return mAI.process.writeInputStream(command, strlen(command), numWrite);
		}
		T mAI;
	};

	class LeelaBot : public TGTPLikeBot< LeelaAppRun >
	{
	public:
		virtual bool initilize(void* settingData) override;

	};

	struct AQAppRun : public GTPLikeAppRun
	{
	public:
		static char const* InstallDir;

		bool buildPlayGame();

	};


	class AQBot : public TGTPLikeBot< AQAppRun >
	{
	public:
		virtual bool initilize(void* settingData) override;
	};

}//namespace Go

#endif // LeelaBot_H_42280575_AB74_4D08_8444_953C8BD1062B