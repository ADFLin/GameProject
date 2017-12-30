#include "LeelaBot.h"

#include "StringParse.h"
#include "FileSystem.h"

namespace Go
{


	class AutoGTPOutputThread : public IGameOutputThread
	{
	public:

		int curStep = 0;

		unsigned run()
		{
			for( ;;)
			{
				char buffer[1024 + 1];
				int numRead;
				bool bSuccess = process->readOutputStream(buffer, ARRAY_SIZE(buffer) - 1, numRead);
				if( !bSuccess || numRead == 0 )
					break;
				buffer[numRead] = 0;
				parseOutput(buffer, numRead);
			}
			return 0;
		}

		void parseOutput(char* buffer, int num)
		{
			char const* cur = buffer;
			while( *cur != 0 )
			{
				int step;
				char coord[32];
				int numRead;
				cur = ParseUtility::SkipSpace(cur);
				if( sscanf(cur, "%d%s%n", &step, coord, &numRead) == 2 && coord[0] == '(' )
				{
					GameCommand com;
					if( step == 1 )
					{
						com.id = GameCommand::eStart;
						addCommand(com);
						curStep = 1;
					}

					if( curStep != step )
					{
						::Msg("Warning:Error Step");
					}
					if( strcmp("(pass)", coord) == 0 )
					{
						com.id = GameCommand::ePass;
						addCommand(com);
					}
					else if( strcmp("(resign)", coord) == 0 )
					{
						com.id = GameCommand::eResign;
						addCommand(com);
					}
					else if( ('A' <= coord[1] && coord[1] <= 'Z') &&
						('0' <= coord[2] && coord[2] <= '9') )
					{
						int pos[2];
						Go::ReadCoord(coord + 1, pos);
						com.pos[0] = pos[0];
						com.pos[1] = pos[1];
						addCommand(com);
					}
					else
					{
						::Msg("Unknown Com = %s", coord);
					}
					++curStep;
					cur += numRead;
				}
				else
				{
					char const* next = ParseUtility::SkipToNextLine(cur);
					int num = next - cur;
					if( num )
					{
						FixString< 512 > str{ cur , num };
						::Msg("%s", str.c_str());
						cur = next;
					}
				}
			}
		}

	};


	class LeelazOutputThread : public IGameOutputThread
	{
	public:

		char mBuffer[1024 + 1];
		int  mNumUsed = 0;
		unsigned run()
		{
			for( ;;)
			{
				int numRead;
				bool bSuccess = process->readOutputStream(mBuffer + mNumUsed, ARRAY_SIZE(mBuffer) - (1 + mNumUsed), numRead);
				if( !bSuccess || numRead == 0 )
					break;

				char* pData = mBuffer;
				char* pLineEnd = pData + mNumUsed;
				mNumUsed += numRead;
				char* pDataEnd = mBuffer + mNumUsed;
				for( ;; )
				{
					for( ; pLineEnd != pDataEnd; ++pLineEnd )
					{
						if( *pLineEnd == '\r' || *pLineEnd == '\n' )
							break;
					}
					if( pLineEnd == pDataEnd )
						break;

					*pLineEnd = 0;
					if( pData != pLineEnd )
					{
						::Msg(pData);
						parseLine(pData, pLineEnd - pData);
					}

					++pLineEnd;
					pData = pLineEnd;
				}

				if( pData != pDataEnd )
				{
					mNumUsed = pDataEnd - pData;
					for( int i = 0; i < mNumUsed; ++i )
						mBuffer[i] = *(pData++);
				}
				else
				{
					mNumUsed = 0;
				}

			}
			return 0;
		}
		void parseLine(char* buffer, int num)
		{
			char const* cur = buffer;
			while( *cur != 0 )
			{
				char coord[32];
				int numRead;
				cur = ParseUtility::SkipSpace(cur);
				if( sscanf(cur, "= %s%n", &coord, &numRead) == 1 )
				{
					GameCommand com;
					if( strcmp(coord, "resign") == 0 )
					{
						com.id = GameCommand::eResign;
						addCommand(com);
					}
					else if( strcmp(coord, "pass") == 0 )
					{
						com.id = GameCommand::ePass;
						addCommand(com);
					}
					else
					{
						Go::ReadCoord(coord, com.pos);

						if( 0 <= com.pos[0] && com.pos[0] < 19 &&
						   0 <= com.pos[1] && com.pos[1] < 19 &&
						   '0' <= coord[1] && coord[1] <= '9' )
						{
							addCommand(com);
						}
						else
						{
							::Msg(cur);
						}
					}

					cur += numRead;
				}
				else
				{
					FixString< 512 > str{ cur , num };
					//::Msg("%s", str.c_str());
					break;
				}
			}
		}
	};

	LeelaAIRun::~LeelaAIRun()
	{
		if( outputThread )
		{
			delete outputThread;
		}
	}

	char const* LeelaAIRun::LeelaZeroDir = nullptr;

	std::string LeelaAIRun::GetBestWeightName()
	{
		FileIterator fileIter;
		if( !FileSystem::FindFile(LeelaZeroDir, nullptr, fileIter) )
		{
			return "";
		}

		bool haveBest = false;
		DateTime bestDate;
		std::string result;
		for( ; fileIter.haveMore(); fileIter.goNext() )
		{
			if( FileUtility::GetSubName(fileIter.getFileName()) != nullptr )
				continue;

			DateTime date = fileIter.getLastModifyDate();

			if( !haveBest || date > bestDate )
			{
				result = fileIter.getFileName();
				bestDate = date;
				haveBest = true;
			}
		}
		return result;
	}

	void LeelaAIRun::stop()
	{
		if( outputThread )
		{
			outputThread->kill();
			delete outputThread;
			outputThread = nullptr;
		}
		process.terminate();
	}

	bool LeelaAIRun::buildLearningGame()
	{
		FixString<256> path;
		path.format("%s/%s", LeelaZeroDir, "/autogtp.exe");

		if( !process.create(path) )
			return false;

		auto myThread = new AutoGTPOutputThread;
		myThread->process = &process;
		myThread->start();
		myThread->setDisplayName("Output Thread");
		outputThread = myThread;

		return true;
	}


	bool LeelaAIRun::buildPlayGame(LeelaAISetting const& setting)
	{
		if( setting.weightName == nullptr )
			return false;

		FixString<256> path;
		path.format("%s/%s", LeelaZeroDir, "/leelaz.exe");
		FixString<512> command;
		::Msg("Play weight = %s", setting.weightName);
		command.format(" -r %d -g -t %d -q -d -n -w %s -m %d -p %d --noponder",
					   setting.resignpct, setting.numThread, setting.weightName, setting.randomcnt, setting.playouts);
		if( !process.create(path, command) )
			return false;

		auto myThread = new LeelazOutputThread;
		myThread->process = &process;
		myThread->start();
		myThread->setDisplayName("Output Thread");
		outputThread = myThread;

		return true;
	}

	bool LeelaAIRun::playStone(Vec2i const& pos, int color)
	{
		FixString<128> com;
		char coord = 'A' + pos.x;
		if( coord >= 'I' )
			++coord;
		com.format("play %c %c%d\n", (color == StoneColor::eBlack ? 'b' : 'w'), coord, pos.y + 1);
		return inputProcessStream(com);
	}

	bool LeelaAIRun::playPass()
	{
		FixString<128> com;
		com.format("play pass\n");
		return inputProcessStream(com);
	}

	bool LeelaAIRun::thinkNextMove(int color)
	{
		FixString<128> com;
		com.format("genmove %s\n", ((color == StoneColor::eBlack) ? "b" : "w"));
		return inputProcessStream(com);
	}

	bool LeelaAIRun::undo()
	{
		return inputProcessStream("undo\n");
	}

	bool LeelaAIRun::setupGame(GameSetting const& setting)
	{
		FixString<128> com;
		com.format("komi %.1f\n", setting.komi);
		inputProcessStream(com);
		if( setting.fixedHandicap )
		{
			com.format("fixed_handicap %d\n", setting.fixedHandicap);
			inputProcessStream(com);
		}
		//com.format()

		return true;
	}

	bool LeelaAIRun::inputProcessStream(char const* command, int length /*= 0*/)
	{
		int numWrite = 0;
		return process.writeInputStream(command, length ? length : strlen(command), numWrite);
	}

	bool LeelaBot::initilize(void* settingData)
	{
		if( settingData )
		{
			if( !mAI.buildPlayGame(*static_cast<LeelaAISetting*>(settingData)) )
				return false;
		}
		else
		{
			LeelaAISetting setting;
			std::string weightName = LeelaAIRun::GetBestWeightName();
			setting.weightName = weightName.c_str();
			if( !mAI.buildPlayGame(setting) )
				return false;
		}
		return true;
	}

}