#include "GTPBotBase.h"
#include "GTPOutput.h"

#include "SystemPlatform.h"
#include <functional>

namespace Go
{

	GTPLikeAppRun::~GTPLikeAppRun()
	{
		if (outputThread)
		{
			delete outputThread;
		}
	}

	void GTPLikeAppRun::stop()
	{
		inputProcessStream("quit\n");
		SystemPlatform::Sleep(1);
		if (outputThread)
		{
			outputThread->kill();
			delete outputThread;
			outputThread = nullptr;
		}
		process.terminate();
	}

	bool GTPLikeAppRun::restart(GameSetting const& setting)
	{
		if (outputThread)
			outputThread->restart();

		return setupGame(setting);
	}

	bool GTPLikeAppRun::playStone(int x, int y, int color)
	{
		InlineString<128> com;
		com.format("play %c %c%d\n", ToColorChar(color) , ToCoordChar(x), y + 1);
		if (!inputCommand(com, { GTPCommand::ePlay , color }))
			return false;
		return (bWaitCommandCompletion) ? waitCommandCompletion() : true;
	}

	bool GTPLikeAppRun::addStone(int x, int y, int color)
	{
		InlineString<128> com;
		com.format("play %c %c%d\n", ToColorChar(color), ToCoordChar(x), y + 1);
		if (!inputCommand(com, { GTPCommand::eAdd , color }))
			return false;
		return (bWaitCommandCompletion) ? waitCommandCompletion() : true;
	}

	bool GTPLikeAppRun::playPass(int color)
	{
		InlineString<128> com;
		if (color != EStoneColor::Empty)
			com.format("play %c pass\n", ToColorChar(color));
		else
			com.format("play pass\n");
		if (!inputCommand(com, { GTPCommand::ePass , color }))
			return false;
		return (bWaitCommandCompletion) ? waitCommandCompletion() : true;
	}

	bool GTPLikeAppRun::thinkNextMove(int color)
	{
		InlineString<128> com;
		com.format("genmove %c\n", ToColorChar(color));
		if (!inputCommand(com, { GTPCommand::eGenmove , color }))
			return false;
		return (bWaitCommandCompletion) ? waitCommandCompletion() : true;
	}

	bool GTPLikeAppRun::undo()
	{
		if (!inputCommand("undo\n", { GTPCommand::eUndo , 0 }))
			return false;
		return (bWaitCommandCompletion) ? waitCommandCompletion() : true;
	}

	bool GTPLikeAppRun::requestUndo()
	{
		if (!inputCommand("undo\n", { GTPCommand::eRequestUndo , 1 }))
			return false;
		return (bWaitCommandCompletion) ? waitCommandCompletion() : true;
	}

	bool GTPLikeAppRun::setupGame(GameSetting const& setting)
	{
		InlineString<128> com;
		com.format("komi %.1f\n", setting.komi);
		if (!inputCommand(com, { GTPCommand::eKomi , 0 }))
			return false;
		if (!inputCommand("clear_board\n", { GTPCommand::eRestart , 0 }))
			return false;
		if (setting.numHandicap && setting.bFixedHandicap )
		{
			com.format("fixed_handicap %d\n", setting.numHandicap);
			if (!inputCommand(com, { GTPCommand::eHandicap , setting.numHandicap }))
				return false;
		}
		//com.format()
		return (bWaitCommandCompletion) ? waitCommandCompletion() : true;
	}

	bool GTPLikeAppRun::showResult()
	{
		if (!inputCommand("final_score\n", { GTPCommand::eFinalScore , 0 }))
			return false;

		return (bWaitCommandCompletion) ? waitCommandCompletion() : true;
	}

	bool GTPLikeAppRun::readBoard(int* outData)
	{
		if (!inputCommand("showboard\n", { GTPCommand::eShowBoard , 0 }))
			return false;
		static_cast<GTPOutputThread*>(outputThread)->mOutReadBoard = outData;
		return true;
	}

	bool GTPLikeAppRun::isThinking()
	{
		return static_cast<GTPOutputThread*>(outputThread)->mProcQueue.back().id == GTPCommand::eGenmove;
	}

	bool GTPLikeAppRun::inputCommand(char const* command, GTPCommand com)
	{
		if (!inputProcessStream(command))
			return false;
		static_cast<GTPOutputThread*>(outputThread)->mProcQueue.push_back(com);
		return true;
	}

	bool GTPLikeAppRun::inputProcessStream(char const* command, int length /*= 0*/)
	{
		int numWrite = 0;
		return process.writeInputStream(command, length ? length : FCString::Strlen(command), numWrite);
	}

	template< class TR , class ...TArgs >
	struct TFuncWapper
	{
		template< class TFunc >
		TFuncWapper(TFunc&& func)
			:mFuncObject(func)
		{

		}

		TR execute(TArgs ...args)
		{
			return mFuncObject(args...);
		}

		std::function< TR (TArgs...) > mFuncObject;
	};

	bool GTPLikeAppRun::waitCommandCompletion()
	{
		mWaitCompletionResult = true;
		GTPOutputThread* myThread = static_cast<GTPOutputThread*>(outputThread);
		while (!myThread->mProcQueue.empty())
		{
			myThread->update();
		}
		return mWaitCompletionResult;
	}

	void GTPLikeAppRun::notifyCommandResult(GTPCommand com, EGTPComExecuteResult result)
	{
		LogMsg("== %s %s", GTPCommand::ToString( com.id ) , result == EGTPComExecuteResult::Success ? "true" : "false" );

		if (bWaitCommandCompletion)
		{
			if (result != EGTPComExecuteResult::Success)
				mWaitCompletionResult = false;
		}
		switch (com.id)
		{
		case GTPCommand::ePlay:
		case GTPCommand::ePass:
		case GTPCommand::eUndo:
			{
				GameCommand resultCom;
				resultCom.id = GameCommand::eExecResult;
				resultCom.result = result == EGTPComExecuteResult::Success ? EBotExecResult::BOT_OK : EBotExecResult::BOT_FAIL;
				outputThread->addOutputCommand(resultCom);
			}
			break;
		default:
			break;
		}
	}

	void GTPLikeAppRun::bindCallback()
	{
		static_cast<GTPOutputThread*>(outputThread)->onCommandResult = GTPCommandResultDelegate(this, &GTPLikeAppRun::notifyCommandResult);
	}

}//namespace Go

