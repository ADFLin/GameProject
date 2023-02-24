#include "LeelaBot.h"
#include "GTPOutput.h"

#include "FileSystem.h"
#include "StringParse.h"
#include "Core/StringConv.h"
#include "Core/Memory.h"
#include "PropertySet.h"

//#TODO:remove me
#include "GameGlobal.h"

namespace Go
{
	TArrayView< char const* const> ELFWeights = ARRAY_VIEW_REAONLY_DATA(char const*,
		"d13c40993740cb77d85c838b82c08cc9c3f0fbc7d8c3761366e5d59e8f371cbd",
		"62b5417b64c46976795d10a6741801f15f857e5029681a42d02c9852097df4b9"
	);


	class AutoGTPOutputThread : public CGameOutputThread
	{
	public:
		bool bEngineStart = false;
		int  curStep = 0;

		std::string lastNetworkName;
		std::string blackNetworkName;
		std::string whiteNetworkName;

		void processBuffer() override
		{
			if( mNumUsed )
			{
				int numSaved = parseOutput(mBuffer.data(), mNumUsed);
				if( numSaved && numSaved != mNumUsed )
				{
					FMemory::Move(mBuffer.data(), mBuffer.data() + mNumUsed - numSaved, numSaved);
				}
				mNumUsed = numSaved;
			}
		}


		int parseOutput(char* buffer, int bufferSize)
		{

#define STR_SCORE "Score:"
#define STR_GOT_NEW_JOB "Got new job:"
#define STR_NET_FILENAME "Net fileName:"
#define STR_NET          "net:"
#define STR_1ST_NETWORK  "first network:"
#define STR_2ND_NETWORK  "second network"

			char const* cur = buffer;
			while( *cur != 0 )
			{
				if( cur - buffer >= bufferSize )
				{
					LogError("Parse logic error!!");
					return 0;
				}
				int step;
				char coord[32];
				int numRead;
				char color[32];
				cur = FStringParse::SkipSpace(cur);

				if( bEngineStart )
				{
					GameCommand com;

					if( sscanf(cur, "%d%s%s%n", &step, color, coord, &numRead) == 3 && color[0] == '(' )
					{
						if( step == 1 )
						{
							com.id = GameCommand::eStart;
							addOutputCommand(com);
							curStep = 1;
						}

						if( curStep != step )
						{
							LogMsg("Warning:Error Step");
						}
						if( StartWith(coord, "pass") )
						{
							com.id = GameCommand::ePass;
							addOutputCommand(com);
						}
						else if( StartWith(coord, "resign") )
						{
							com.id = GameCommand::eResign;
							addOutputCommand(com);
						}
						else if ( Go::ReadCoord(coord, com.pos) )
						{
							com.id = GameCommand::ePlayStone;
							if( color[1] == 'B' || color[1] == 'b' )
							{
								com.playColor = EStoneColor::Black;
							}
							else if( color[1] == 'W' || color[1] == 'w' )
							{
								com.playColor = EStoneColor::White;
							}
							else
							{
								LogWarning(0 , "Unknown color : %s" , color);
								com.playColor = EStoneColor::Empty;
							}
							addOutputCommand(com);
							
						}
						else
						{
							LogMsg("Unknown Com = %s", coord);
						}
						++curStep;
						cur += numRead;
					}
					else if( sscanf(cur, "%d(", &step, &numRead) == 1 )
					{
						//LogMsg("==comand not complete : %s ==", cur);
						return bufferSize - (cur - buffer);
					}
					else
					{
						char const* next = FStringParse::SkipToNextLine(cur);
						int num = next - cur;
						if( num )
						{
							InlineString< 512 > str{ cur , num };
							if( bLogMsg )
							{
								LogMsg("%s", str.c_str());
							}

							if( StartWith(str , STR_SCORE) )
							{
								char const* strResult = FStringParse::SkipSpace(str.c_str() + StrLen(STR_SCORE) );
								GameCommand com;
								com.id = GameCommand::eEnd;
								com.winner = ParseResult(strResult, com.winNum);
								addOutputCommand(com);
								bEngineStart = false;
							}

							cur = next;
						}
					}
				}
				else
				{
					char const* next = FStringParse::SkipToNextLine(cur);


					int num = next - cur;
					if( num )
					{
						while( num )
						{
							char c = cur[num-1];
							if ( c != ' ' && c != '\r' && c != '\n' )
								break;
							--num;
						}
						InlineString< 512 > str{ cur , num };
						if( bLogMsg )
						{
							LogMsg(str.c_str());
						}

						if( StartWith(str, STR_NET) )
						{
							lastNetworkName = FStringParse::SkipSpace(str.c_str() + StrLen(STR_NET));
							if ( lastNetworkName.back() == '.' )
								lastNetworkName.pop_back();
							GameCommand com;
							com.setParam(LeelaGameParam::eNetWeight, lastNetworkName.c_str());
							addOutputCommand(com);
						}
						else if( StartWith(str, STR_GOT_NEW_JOB) )
						{
							bool bMatchJob = false;

							char const* jobName = FStringParse::SkipSpace(str.c_str() + StrLen(STR_GOT_NEW_JOB));
							if( StartWith(jobName, "match") )
							{
								bMatchJob = true;
							}
							GameCommand com;
							com.setParam(LeelaGameParam::eJobMode, bMatchJob);
							addOutputCommand(com);
						}
						else if( StartWith(str, STR_1ST_NETWORK) )
						{
							blackNetworkName = FStringParse::SkipSpace(str.c_str() + StrLen(STR_1ST_NETWORK));
							if( blackNetworkName.back() == '.' )
								blackNetworkName.pop_back();

							if( blackNetworkName == lastNetworkName )
							{
								GameCommand com;
								com.setParam(LeelaGameParam::eMatchChallengerColor, EStoneColor::White);
								addOutputCommand(com);
							}

						}
						else if( StartWith(str, STR_2ND_NETWORK) )
						{
							whiteNetworkName = FStringParse::SkipSpace(str.c_str() + StrLen(STR_2ND_NETWORK));
							if( whiteNetworkName.back() == '.' )
								whiteNetworkName.pop_back();

							if( whiteNetworkName == lastNetworkName )
							{
								GameCommand com;
								com.setParam(LeelaGameParam::eMatchChallengerColor, EStoneColor::Black);
								addOutputCommand(com);
							}
						}
						else if( StartWith(str, "Engine has started.") )
						{
							bEngineStart = true;
						}
						cur = next;
					}
				}
			}

			return 0;
		}

	};


	class LeelaOutputThread : public GTPOutputThread
	{
	public:
		LeelaAppRun* mAppRun;

		LeelaThinkInfoVec thinkResults[2];
		int  indexResultWrite = 0;
		bool bPondering = false;

	
		bool bReadingBoard = false;
		void dumpCommandMsgBegin(GTPCommand com) override 
		{
			switch( com.id )
			{
			case GTPCommand::eKomi:
				break;
			case GTPCommand::eHandicap:
				break;
			case GTPCommand::ePlay:
				break;
			case GTPCommand::eGenmove:
			case GTPCommand::eStopPonder:
				indexResultWrite = 1 - indexResultWrite;
				thinkResults[indexResultWrite].clear();
				break;
			case GTPCommand::ePass:
				break;
			case GTPCommand::eUndo:
				break;
			case GTPCommand::eQuit:
				break;
			case GTPCommand::eShowBoard:
				bReadingBoard = true;
				break;

			default:
				break;
			}
		}

		void dumpCommandMsgEnd(GTPCommand com) override 
		{
			switch( com.id )
			{
			case GTPCommand::eStartPonder:
				{
					bPondering = true;
				}
				break;
			case GTPCommand::eStopPonder:
				if( bPondering )
				{
					bPondering = false;
				}
				break;
			}
		}


		bool readThinkInfo(char* buffer, int num)
		{
			if (strstr(buffer, "->") == nullptr)
			{
				return false;
			}
			InlineString<128>  coord;
			int   nodeVisited;
			float LCB;
			float winRate;
			float evalValue;
			int   playout;
			int   numRead;

			if( sscanf(buffer, "%s -> %d (V: %f%%) (LCB: %f%%) (N: %f%%)%n", coord.data(), &nodeVisited, &winRate, &LCB, &evalValue, &numRead) != 5 )
			{
				LogWarning(0, "Leela Output : Parse Think Info fail : %s", buffer);
				return false;
			}

			PlayVertex vertex = GetVertex(coord);
			if( vertex == PlayVertex::Undefiend() )
			{
				//LogWarning(0, "Error Think Str = %s", buffer);
				//return;
			}

			LeelaThinkInfo info;
			info.v = vertex;
			info.nodeVisited = nodeVisited;
			info.winRate = winRate;
			info.evalValue = evalValue;
			char const* vBuffer = buffer + numRead + StrLen(" PV:");
			vBuffer = FStringParse::SkipSpace(vBuffer);

			while( *vBuffer != 0 )
			{
				PlayVertex v = ReadVertex(vBuffer, numRead);
				if( numRead == 0 )
					break;

				info.vSeq.push_back(v);
				vBuffer = FStringParse::SkipSpace(vBuffer + numRead);
			}
			thinkResults[indexResultWrite].push_back(info);
			return true;
		}

		void sendUsageThinkResult()
		{
			LeelaThinkInfoVec* ptr = &thinkResults[indexResultWrite];
			GameCommand paramCom;
			paramCom.setParam(LeelaGameParam::eThinkResult, ptr);
			addOutputCommand(paramCom);
		}

		bool bRecvThinkInfo = false;
		LeelaThinkInfo bestThinkInfo;
		void procDumpCommandMsg(GTPCommand com, char* buffer, int num) override
		{
			switch( com.id )
			{
			case GTPCommand::eNone:
				if ( bPondering )
				{
#define INFO_MOVE_STR "info move"
					while( StartWith(buffer, INFO_MOVE_STR) )
					{
						buffer += StrLen(INFO_MOVE_STR);

						InlineString<128>  coord;
						int   visits = 0;
						int   winrate = 0;
						int   prior = 0;
						int   lcb = 0;
						int   order = 0;
						int   numRead = 0;
						if( sscanf(buffer, "%s visits %d winrate %d prior %d lcb %d order %d pv%n", coord.data(), &visits, &winrate, &prior, &lcb , &order, &numRead) == 6 )
						{
							PlayVertex vertex = GetVertex(coord);
							if( vertex == PlayVertex::Undefiend() )
							{
								//LogWarning(0, "Error Think Str = %s", buffer);
								//return;
							}

							LeelaThinkInfo info;
							info.v = vertex;
							info.nodeVisited = visits;
							info.winRate = float(winrate)/100.0f;
							info.evalValue = prior;
							buffer = const_cast<char*>(FStringParse::SkipSpace(buffer + numRead));
							
							while( *buffer != 0 || !StartWith(buffer,INFO_MOVE_STR))
							{
								PlayVertex v = ReadVertex(buffer, numRead);
								if( numRead == 0 )
									break;

								info.vSeq.push_back(v);
								buffer = const_cast<char*>( FStringParse::SkipSpace(buffer + numRead) );
							}
							thinkResults[indexResultWrite].push_back(info);
						}
						else
						{
							LogWarning(0, "Leela Output : Parse 'info move' fail : %s" , buffer );
						}
					}

					sendUsageThinkResult();
					indexResultWrite = 1 - indexResultWrite;
					thinkResults[indexResultWrite].clear();
				}
				break;
			case GTPCommand::eStopPonder:
			case GTPCommand::eGenmove:
				{
#define PLAYOUTS_STR "Playouts:"
					if( StartWith( buffer , PLAYOUTS_STR) )
					{
						buffer += StrLen(PLAYOUTS_STR);
						InlineString<128>  coord;
						//int   nodeVisited;
						float winRate;
						float evalValue = 0;
						int   playout;
						int   numRead;

						if (sscanf(buffer, "%d, Win: %f%% , PV: %s%n", &playout, &winRate, coord.data(), &numRead) == 3)
						{
							PlayVertex vertex = GetVertex(coord);
							if (vertex == PlayVertex::Undefiend())
							{
								LogWarning(0, "Error Think Str = %s", buffer);
								return;
							}

							bestThinkInfo.evalValue = evalValue;
							bestThinkInfo.nodeVisited = playout;
							bestThinkInfo.winRate = winRate;
							bestThinkInfo.v = vertex;
							bestThinkInfo.vSeq.clear();
							bestThinkInfo.vSeq.push_back(vertex);

							char const* vBuffer = FStringParse::SkipSpace(buffer + numRead);
							while (*vBuffer != 0)
							{
								PlayVertex v = ReadVertex(vBuffer, numRead);
								if (numRead == 0)
									break;

								bestThinkInfo.vSeq.push_back(v);
								vBuffer = FStringParse::SkipSpace(vBuffer + numRead);
							}

							GameCommand com;
							com.setParam(LeelaGameParam::eBestMoveVertex, &bestThinkInfo);
							addOutputCommand(com);

						}
					}
					else if( readThinkInfo( buffer , num ) )
					{
						
					}
				}
				break;
			case GTPCommand::eShowBoard:
				{
					char* endRead;
					int y = strtol(buffer, (char**)&endRead, 10);
					if (buffer != endRead)
					{
						buffer = endRead;
						int x = 0;
						
						int* pData = mOutReadBoard + LeelaGoSize * (y - 1);

						StringView token;
						EStringToken tokenType;
						char const* readBuffer = buffer;

						int count = 0;
						while( (tokenType = FStringParse::StringToken(readBuffer, " ()", ".+XO\n", token) ) != EStringToken::None )
						{					
							if (tokenType == EStringToken::Delim )
							{					
								if (token[0] == '\n')
									break;

								switch (token[0])
								{
								case '+':
								case '.': 
									*pData = EStoneColor::Empty; ++pData; ++count; break;
								case 'X': *pData = EStoneColor::Black; ++pData; ++count; break;
								case 'O': *pData = EStoneColor::White; ++pData; ++count; break;
								}

								if (count == LeelaGoSize)
									break;
							}
						}

						if (y == 1)
						{
							GameCommand com;
							com.id = GameCommand::eBoardState;
							com.pBoardData = mOutReadBoard;
							mOutReadBoard = nullptr;
							addOutputCommand( com );

							popHeadComandMsg();
						}
					}
				}
			}
		}

		void onOutputCommand(GTPCommand com, GameCommand const& outCom) override
		{
			switch( com.id )
			{
			case GTPCommand::eStopPonder:
			case GTPCommand::eGenmove:
				auto iter = std::max_element(
					thinkResults[ indexResultWrite ].begin(), 
					thinkResults[ indexResultWrite ].end(), 
					[](auto const& a, auto const& b) { return a.winRate < b.winRate; });

				if ( iter != thinkResults[ indexResultWrite ].end() )
				{
					GameCommand paramCom;
					paramCom.setParam(LeelaGameParam::eWinRate, iter->winRate);
					addOutputCommand(paramCom);
				}

				{
					GameCommand paramCom;
					paramCom.setParam(LeelaGameParam::eThinkResult, &thinkResults[indexResultWrite]);
					addOutputCommand(paramCom);
				}
				break;
			}

		}

	};

	char const* LeelaAppRun::InstallDir = nullptr;

	std::string LeelaAppRun::GetLastWeightName()
	{
		FileIterator fileIter;
		InlineString<256> path;
		path.format("%s/%s" , InstallDir , LEELA_NET_DIR_NAME );
		if( !FFileSystem::FindFiles(path, nullptr, fileIter) )
		{
			return "";
		}

		bool haveBest = false;
		DateTime bestDate;
		std::string result;
		for( ; fileIter.haveMore(); fileIter.goNext() )
		{
			if( FFileUtility::GetExtension(fileIter.getFileName()) != nullptr )
				continue;

			if( strlen(fileIter.getFileName()) != 64 )
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

	std::string LeelaAppRun::GetBestWeightName()
	{
		char const* name;
		if( ::Global::GameConfig().tryGetStringValue("Leela.LastNetWeight", "Go", name) )
		{
			return std::string(name) + ".gz";
			//return name;
		}
		return LeelaAppRun::GetLastWeightName();
	}


	LeelaAISetting LeelaAISetting::GetDefalut()
	{
		LeelaAISetting setting;
		setting.seed = generateRandSeed();
#if 0
		setting.bNoise = true;
		setting.numThread = 4;
		setting.playouts = 10000;
#else
		setting.bNoise = ::Global::GameConfig().getIntValue("bNoise", "LeelaZeroSetting", 0);
		setting.bDumbPass = ::Global::GameConfig().getIntValue("bDumbPass", "LeelaZeroSetting", 0);
		setting.numThread = ::Global::GameConfig().getIntValue("numThread", "LeelaZeroSetting", Math::Min(8,SystemPlatform::GetProcessorNumber()));
		setting.visits = ::Global::GameConfig().getIntValue("visits", "LeelaZeroSetting", 20000);
		setting.playouts = ::Global::GameConfig().getIntValue("playouts", "LeelaZeroSetting", 0);
#endif
		return setting;
	}

#define  AddCom( NAME , VALUE )\
			if( VALUE )\
			{\
				result += NAME;\
			}\

#define  AddComValue( NAME , VALUE )\
			if( VALUE )\
			{\
				result += NAME;\
				result += FStringConv::From(VALUE);\
			}\

#define  AddComValueNoCheck( NAME , VALUE )\
			{\
				result += NAME;\
				result += FStringConv::From(VALUE);\
			}\

	std::string LeelaAISetting::toParamString() const
	{
		std::string result;
		AddComValueNoCheck(" -p ", playouts);
		AddComValue(" -v ", visits);
		if( weightName )
		{
			result += " -w ";
			char const* subName = FFileUtility::GetExtension(weightName);
			if( subName )
			{
				result += weightName;
				result.resize(result.size() - strlen(subName) - 1);
			}
			else
			{
				result += weightName;
			}
		}
		return result;
	}

	std::string LeelaAISetting::toString() const
	{
		std::string result;

		AddComValueNoCheck(" -r ", resignpct);
		AddComValue(" -t ", numThread);
		AddComValueNoCheck(" -p ", playouts);
		AddComValue(" -v ", visits);
		AddComValue(" -m ", randomcnt);
		AddComValue(" -s ", seed);
		if( weightName )
		{
			result += " -w " LEELA_NET_DIR_NAME "/";
			result += weightName;
		}
		AddCom(" -q", bQuiet);
		AddCom(" -d", bDumbPass);
		AddCom(" -n", bNoise);
		AddCom(" -g", bGTPMode);
		AddCom(" --noponder", bNoPonder);
		AddCom(" -q", bQuiet);

		return result;
	}
#undef AddCom
#undef AddComValue

	bool LeelaAppRun::buildLearningGame()
	{

		InlineString<256> path;
		path.format("%s/%s", InstallDir, "autogtp.exe");
		bool result = buildProcessT< AutoGTPOutputThread >( path , nullptr );
		if (result)
		{
			mProcessExrcuteCommand = path;
		}
		return result;
	}

	bool LeelaAppRun::buildPlayGame(LeelaAISetting const& setting)
	{
		if( setting.weightName == nullptr )
			return false;

		mUseWeightName = setting.weightName;

		InlineString<256> path;
		path.format("%s/%s", InstallDir, "leelaz.exe");

		LogMsg("Play weight = %s", setting.weightName);

		std::string opitions = setting.toString();
		bool result = buildProcessT< LeelaOutputThread >(path, opitions.c_str());
		if (result)
		{
			mProcessExrcuteCommand = path;
			mProcessExrcuteCommand += " ";
			mProcessExrcuteCommand += opitions;
		}

		return result;
	}


	bool LeelaAppRun::buildAnalysisGame( bool bUseELF )
	{
		LeelaAISetting setting;
		mUseWeightName = LeelaAppRun::GetBestWeightName();
		setting.weightName = mUseWeightName.c_str();
		setting.bNoise = false;
		setting.bDumbPass = false;
		setting.bNoPonder = true;
		setting.seed = generateRandSeed();
		
		setting.playouts = 0;
		setting.visits = 0;
		setting.randomcnt = 0;
		setting.resignpct = 0;

		if( bUseELF )
		{
			setting.weightName = "ELF2";
		}
		bool result = buildPlayGame(setting);
		if( result )
		{
			static_cast<LeelaOutputThread*>(outputThread)->bLogMsg = false;
		}

		return result;
	}


	void LeelaAppRun::startPonder( int color )
	{
		InlineString<128> com;
		com.format("lz-analyze %c 10\n", ToColorChar(color));
		inputCommand(com, { GTPCommand::eStartPonder , 0 });
	}

	void LeelaAppRun::stopPonder()
	{
		inputCommand("name\n", { GTPCommand::eStopPonder , 0 });
	}


	bool LeelaBot::initialize(IBotSetting* setting)
	{
		if(setting)
		{
			if( !mAI.buildPlayGame(*static_cast<TBotSettingData<LeelaAISetting>*>(setting)) )
				return false;
		}
		else
		{
			LeelaAISetting setting = LeelaAISetting::GetDefalut();
			std::string weightName = LeelaAppRun::GetBestWeightName();
			setting.weightName = weightName.c_str();
			if( !mAI.buildPlayGame(setting) )
				return false;
		}
		return true;
	}

	bool LeelaBot::getMetaData(int id, uint8* dataBuffer, int size)
	{
		switch( id )
		{
		case LeelaBot::eWeightName:
			assert(size == sizeof(std::string));
			*reinterpret_cast<std::string*>(dataBuffer) = mAI.mUseWeightName;
			return true;
		}
		return false;
	}

}//namespace Go

