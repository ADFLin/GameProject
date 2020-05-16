#include "GTPBotBase.h"
#include "GTPOutput.h"

#include "SystemPlatform.h"

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

	bool GTPLikeAppRun::restart()
	{
		if (outputThread)
			outputThread->restart();
		return inputCommand("clear_board\n", { GTPCommand::eRestart , 0 });
	}

	bool GTPLikeAppRun::playStone(int x, int y, int color)
	{
		FixString<128> com;
		com.format("play %c %c%d\n", ToColorChar(color) , ToCoordChar(x), y + 1);
		return inputCommand(com, { GTPCommand::ePlay , color });
	}

	bool GTPLikeAppRun::addStone(int x, int y, int color)
	{
		FixString<128> com;
		com.format("play %c %c%d\n", ToColorChar(color), ToCoordChar(x), y + 1);
		return inputCommand(com, { GTPCommand::eAdd , color });
	}

	bool GTPLikeAppRun::playPass(int color)
	{
		FixString<128> com;
		if (color != StoneColor::eEmpty)
			com.format("play %c pass\n", ToColorChar(color));
		else
			com.format("play pass\n");
		return inputCommand(com, { GTPCommand::ePass , color });
	}

	bool GTPLikeAppRun::thinkNextMove(int color)
	{
		FixString<128> com;
		com.format("genmove %c\n", ToColorChar(color));
		return inputCommand(com, { GTPCommand::eGenmove , color });
	}

	bool GTPLikeAppRun::undo()
	{
		return inputCommand("undo\n", { GTPCommand::eUndo , 0 });
	}

	bool GTPLikeAppRun::requestUndo()
	{
		return inputCommand("undo\n", { GTPCommand::eRequestUndo , 1 });
	}

	bool GTPLikeAppRun::setupGame(GameSetting const& setting)
	{
		FixString<128> com;
		com.format("komi %.1f\n", setting.komi);
		inputCommand(com, { GTPCommand::eKomi , 0 });
		if (setting.numHandicap && setting.bFixedHandicap )
		{
			com.format("fixed_handicap %d\n", setting.numHandicap);
			inputCommand(com, { GTPCommand::eHandicap , setting.numHandicap });
		}

		inputCommand("clear_board\n", { GTPCommand::eRestart , 0 });
		//com.format()

		return true;
	}

	bool GTPLikeAppRun::showResult()
	{
		if (!inputCommand("final_score\n", { GTPCommand::eFinalScore , 0 }))
			return false;

		return true;
	}

	bool GTPLikeAppRun::readBoard(int* outData)
	{
		if (!inputCommand("showboard\n", { GTPCommand::eShowBoard , 0 }))
			return false;
		static_cast<GTPOutputThread*>(outputThread)->mOutReadBoard = outData;
		return true;
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
		return process.writeInputStream(command, length ? length : strlen(command), numWrite);
	}

	void GTPLikeAppRun::notifyCommandResult(GTPCommand com, EGTPComExecuteResult result)
	{
		LogMsg("== %s %s", GTPCommand::ToString( com.id ) , result == EGTPComExecuteResult::Success ? "true" : "false" );
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

