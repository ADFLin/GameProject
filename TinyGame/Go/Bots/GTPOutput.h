#pragma once
#ifndef GTPOutput_H_5EF71CEC_C74F_42C5_8BAF_218A149E1653
#define GTPOutput_H_5EF71CEC_C74F_42C5_8BAF_218A149E1653


#include "GTPBotBase.h"
#include "Delegate.h"

namespace Go
{
	template< int N >
	static int StartWith(char const* s1, char const (&s2)[N])
	{
		return ::strncmp(s1, s2, N - 1) == 0;
	}

	template< int N >
	static int StrLen(char const (&s)[N])
	{
		return N - 1;
	}


	static int const gStartSleepMillionSecond = 2000;
#define AUTO_GTP_DEBUG 1


	class CGameOutputThread : public IGameOutputThread
	{
	public:
		CGameOutputThread(int bufferSize = 20480)
			:mBuffer( bufferSize )
		{

		}

		int32 mNumNewRead = 0;
		int   mNumUsed = 0;

		TArray< char > mBuffer;

		bool  bNeedRead = true;
		bool  bLogMsg = true;
		Mutex mMutexBuffer;
		ConditionVariable mBufferProcessedCV;

#if AUTO_GTP_DEBUG
		TArray< char > debugString;
#endif

		unsigned run() override;

		void update() override;

		virtual void processBuffer() {}


		static int ParseResult(char const* str, float& resultNum);


		int parsePlayResult(char const* str, int color, GameCommand& outCom);

		int parsePlayResult(char const* str, int color);
	};

	DECLARE_DELEGATE(GTPCommandResultDelegate, void (GTPCommand , EGTPComExecuteResult))


	class GTPOutputThread : public CGameOutputThread
	{
		using BaseClass = CGameOutputThread;
	public:
		GTPOutputThread(int bufferSize = 20480)
			:CGameOutputThread(bufferSize)
		{

		}
		int  mColor;
		bool bThinking;
		bool bShowDiagnosticOutput = true;
		//TCycleQueue<GTPCommand> mProcQueue;
		TArray< GTPCommand > mProcQueue;

		int* mOutReadBoard = nullptr;
		bool mbShowParseLine;
		void restart() override
		{
			BaseClass::restart();
			bThinking = false;
			mProcQueue.clear();
		}

		GTPCommandResultDelegate onCommandResult;
		void processBuffer() override;

		virtual void dumpCommandMsgBegin(GTPCommand com) {}
		virtual void procDumpCommandMsg(GTPCommand com, char* buffer, int num) {}
		virtual void dumpCommandMsgEnd(GTPCommand com) {}
		virtual void onOutputCommand(GTPCommand com, GameCommand const& outCom) {}

		void popHeadComandMsg();

		GTPCommand getHeadRequest()
		{
			if (mProcQueue.empty())
				return{ GTPCommand::eNone , 0 };
			return mProcQueue.front();
		}

		bool bDumping = false;

		bool parseLine(char* buffer, int num);

		static PlayVertex ReadVertex(char const* buffer, int& outRead)
		{
			PlayVertex vertex = PlayVertex::Undefiend();

			if (FCString::CompareIgnoreCase(buffer, "Pass") == 0)
			{
				outRead = 4;
				return PlayVertex::Pass();
			}
			else if (FCString::CompareIgnoreCase(buffer, "Resign") == 0)
			{
				outRead = 6;
				return PlayVertex::Resign();
			}
			else
			{
				uint8 pos[2];
				outRead = Go::ReadCoord(buffer, pos);
				if (outRead)
				{
					PlayVertex vertex;
					vertex.x = pos[0];
					vertex.y = pos[1];
					return vertex;
				}
			}
			return PlayVertex::Undefiend();
		}

		template< int N >
		static PlayVertex GetVertex(InlineString<N> const& coord)
		{
			int numRead;
			return ReadVertex(coord.c_str(), numRead);
		}
	};
}//namespace Go

#endif // GTPOutput_H_5EF71CEC_C74F_42C5_8BAF_218A149E1653
