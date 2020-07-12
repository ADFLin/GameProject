#include "GTPOutput.h"

#include "StringParse.h"
#include "SystemPlatform.h"
#include "Core/StringConv.h"

namespace Go
{

	unsigned CGameOutputThread::run()
	{
		SystemPlatform::Sleep(gStartSleepMillionSecond);
		for (;;)
		{

			Mutex::Locker locker(mMutexBuffer);
			mBufferProcessedCV.wait(locker, [this] { return bNeedRead; });
			mNumNewRead = 0;

			int numRead;
			char* pData = mBuffer.data() + mNumUsed;

			int numCanRead = mBuffer.size() - 1 - mNumUsed;
			if (numCanRead <= 0)
			{
				LogWarning(0,"=== buffer too samll , can't Read Output ===");
				mBuffer.resize(mBuffer.size() * 2);
			}

			bool bSuccess = process->readOutputStream(pData, mBuffer.size() - 1 - mNumUsed, numRead);

			if (!bSuccess || (numRead == 0 && mNumUsed != (mBuffer.size() - 1)))
			{
				LogMsg("OutputThread can't read");
				break;
			}

			if (numRead)
			{
#if AUTO_GTP_DEBUG
				if (debugString.size() > 100000)
				{
					debugString.clear();
				}

				debugString.insert(debugString.end(), pData, pData + numRead);
#endif
				mNumNewRead = numRead;
				mNumUsed += numRead;
				mBuffer[mNumUsed] = 0;

				bNeedRead = false;
			}

		}

		return 0;
	}

	void CGameOutputThread::update()
	{
		if (mMutexBuffer.tryLock())
		{
			processBuffer();
			mNumNewRead = 0;
			bNeedRead = true;

			mBufferProcessedCV.notifyOne();
			mMutexBuffer.unlock();
		}
	}

	int CGameOutputThread::ParseResult(char const* str, float& resultNum)
	{
		int color;
		if (str[0] == 'B' || str[0] == 'b')
			color = StoneColor::eBlack;
		else if (str[0] == 'W' || str[0] == 'w')
			color = StoneColor::eWhite;
		else
			return StoneColor::eEmpty;

		assert(str[1] == '+');

		str = FStringParse::SkipSpace(str + 2);
		if (StartWith(str, "Resign"))
		{
			resultNum = 0;
		}
		else
		{
			resultNum = FStringConv::To<float>(str);
		}
		return color;
	}

	int CGameOutputThread::parsePlayResult(char const* str, int color, GameCommand& outCom)
	{
#define STR_RESIGN "resign"
#define STR_PASS "pass"
		int numRead;
		if (StartWith(str, STR_RESIGN))
		{
			outCom.id = GameCommand::eResign;
			return StrLen(STR_RESIGN);
		}
		else if (StartWith(str, STR_PASS))
		{
			outCom.id = GameCommand::ePass;
			return StrLen(STR_PASS);
		}
		else if (numRead = Go::ReadCoord(str, outCom.pos))
		{
			outCom.id = GameCommand::ePlayStone;
			outCom.playColor = color;
			return numRead;
		}
		LogWarning(0, "ParsePlayError : %s", str);
		return 0;
	}

	int CGameOutputThread::parsePlayResult(char const* str, int color)
	{
		int numRead;
		GameCommand com;
		if (StartWith(str, STR_RESIGN))
		{
			com.id = GameCommand::eResign;
			addOutputCommand(com);
			return StrLen(STR_RESIGN);
		}
		else if (StartWith(str, STR_PASS))
		{
			com.id = GameCommand::ePass;
			addOutputCommand(com);
			return StrLen(STR_PASS);
		}
		else if (numRead = Go::ReadCoord(str, com.pos))
		{
			com.id = GameCommand::ePlayStone;
			com.playColor = color;
			addOutputCommand(com);
			return numRead;
		}

		LogWarning(0, "ParsePlayError : %s", str);
		return 0;
	}

	void GTPOutputThread::processBuffer()
	{
		assert(mNumUsed >= mNumNewRead);

		char* pData = mBuffer.data();

		char* pDataEnd = mBuffer.data() + mNumUsed;
		char* pLineEnd = pDataEnd - mNumNewRead;

		for (;; )
		{
			for (; pLineEnd != pDataEnd; ++pLineEnd)
			{
				if (*pLineEnd == '\r' || *pLineEnd == '\n' || *pLineEnd == '\0')
					break;
			}
			if (pLineEnd == pDataEnd)
				break;

			*pLineEnd = 0;
			if (pData != pLineEnd)
			{
				mbShowParseLine = true;
				parseLine(pData, pLineEnd - pData);

				if (bLogMsg && mbShowParseLine)
				{
					LogMsg("GTP: %s ", pData);
				}
			}

			++pLineEnd;
			pData = pLineEnd;
		}

		if (pData != pDataEnd)
		{
			mNumUsed = pDataEnd - pData;
			if (pData != mBuffer.data())
			{
				::memmove(mBuffer.data(), pData, mNumUsed);
			}
			mBuffer[mNumUsed] = 0;
		}
		else
		{
			mNumUsed = 0;
		}
	}

	void GTPOutputThread::popHeadComandMsg()
	{
		//mProcQueue.pop_front();
		GTPCommand com = mProcQueue.front();
		mProcQueue.erase(mProcQueue.begin());
		bDumping = false;
		dumpCommandMsgEnd(com);
	}

	bool GTPOutputThread::parseLine(char* buffer, int num)
	{
		//LogMsg("%s", buffer);

		GTPCommand com = getHeadRequest();
		if (com.id != GTPCommand::eNone && bDumping == false)
		{
			bDumping = true;
			dumpCommandMsgBegin(com);
		}

		char const* cur = buffer;
		if (*cur == '=')
		{
			//LogDevMsg(0, "%d - %s - %u", com.id, buffer , mProcQueue.size() );
			cur = FStringParse::SkipSpace(cur + 1);

			bool bCommadTextReadFinish = true;

			if (onCommandResult && com.id != GTPCommand::eNone)
			{
				onCommandResult(com , EGTPComExecuteResult::Success);
			}

			switch (com.id)
			{
			case GTPCommand::eGenmove:
				{
					GameCommand gameCom;
					if (!parsePlayResult(cur, com.meta, gameCom))
						return false;
					addOutputCommand(gameCom);
					onOutputCommand(com, gameCom);
				}
				break;
			case GTPCommand::ePlay:
				break;
			case GTPCommand::eUndo:
			case GTPCommand::eRequestUndo:
				if (com.meta)
				{
					GameCommand gameCom;
					gameCom.id = GameCommand::eUndo;
					addOutputCommand(gameCom);
				}
				break;
			case GTPCommand::eHandicap:
				for (;;)
				{
					if (*cur == 0)
						break;
					GameCommand gameCom;
					int numRead = parsePlayResult(cur, StoneColor::eBlack, gameCom);
					if (numRead == 0 || gameCom.id != GameCommand::ePlayStone)
						return false;

					gameCom.id = GameCommand::eAddStone;
					addOutputCommand(gameCom);
					onOutputCommand(com, gameCom);
					cur = FStringParse::SkipSpace(cur + numRead);
				}
				break;
			case GTPCommand::eFinalScore:
				{
					GameCommand gameCom;
					gameCom.id = GameCommand::eEnd;
					gameCom.winner = ParseResult(cur, gameCom.winNum);
					addOutputCommand(gameCom);
				}
				break;
			case  GTPCommand::eShowBoard:
				{
					bCommadTextReadFinish = false;
				}
				break;

			default:
				break;
			}

			if (com.id != GTPCommand::eNone && bCommadTextReadFinish )
			{
				popHeadComandMsg();
			}
		}
		else if (*cur == '?')
		{
			//LogDevMsg(0, "%d - %s - %u", com.id, buffer, mProcQueue.size());
			cur = FStringParse::SkipSpace(cur + 1);
			//error operator handled

			if (com.id != GTPCommand::eNone)
			{
				if (onCommandResult)
				{
					onCommandResult(com, EGTPComExecuteResult::Fail);
				}

				//mProcQueue.pop_front(); 
				mProcQueue.erase(mProcQueue.begin());

				bDumping = false;
				dumpCommandMsgEnd(com);
			}
		}
		else
		{
			procDumpCommandMsg(com, buffer, num);
		}

		return true;
	}

}//namespace Go

