#pragma once
#ifndef LeelaBot_H_42280575_AB74_4D08_8444_953C8BD1062B
#define LeelaBot_H_42280575_AB74_4D08_8444_953C8BD1062B

#include "GoBot.h"

#include "Thread.h"
#include "Platform/Windows/WindowsProcess.h"

namespace Go
{
	class IGameOutputThread : public RunnableThreadT< IGameOutputThread >
	{
	public:
		virtual ~IGameOutputThread() {}
		virtual unsigned run() = 0;


		ChildProcess* process = nullptr;
		void addCommand(GameCommand const& com)
		{
			Mutex::Locker lock(mComMutex);
			mCommands.push_back(com);
		}
		template< class T >
		void procCommand(T fun)
		{
			Mutex::Locker lock(mComMutex);
			for( GameCommand& com : mCommands )
			{
				fun(com);
			}
			mCommands.clear();
		}
		std::vector< GameCommand > mCommands;
		Mutex mComMutex;
	};

	struct LeelaAISetting
	{
		char const* weightName = nullptr;
		int playouts = 5000;//1600;
		int randomcnt = 0;//30;
		int numThread = 8;
		int resignpct = 10;
	};

	struct LeelaAIRun
	{
		ChildProcess        process;
		IGameOutputThread*  outputThread = nullptr;

		~LeelaAIRun();

		static char const* LeelaZeroDir;

		static std::string GetBestWeightName();
		void stop();
		bool buildLearningGame();
		bool buildPlayGame(LeelaAISetting const& setting);
		bool playStone(Vec2i const& pos, int color);
		bool playPass();

		bool thinkNextMove(int color);

		bool undo();

		bool setupGame(GameSetting const& setting);

		bool inputProcessStream(char const* command, int length = 0);


	};

	class LeelaBot : public IBotInterface
	{
	public:

		virtual bool initilize(void* settingData) override;
		virtual void destroy()
		{
			mAI.stop();
		}
		virtual bool setupGame(GameSetting const& setting) override
		{
			return mAI.setupGame(setting);
		}
		virtual bool playStone(Vec2i const& pos, int color) override
		{
			return mAI.playStone(pos, color);
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
			auto MyFun = [&](GameCommand const& com)
			{
				listener.notifyCommand(com);
			};
			mAI.outputThread->procCommand(MyFun);
		}

		bool inputProcessStream(char const* command)
		{
			int numWrite = 0;
			return mAI.process.writeInputStream(command, strlen(command), numWrite);
		}


		LeelaAIRun  mAI;
	};

}//namespace Go

#endif // LeelaBot_H_42280575_AB74_4D08_8444_953C8BD1062B
