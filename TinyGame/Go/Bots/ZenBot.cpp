
#include "ZenBot.h"

#include "SystemPlatform.h"

#include <iostream>

#include "GameGlobal.h"
#include "PropertySet.h"
#include "FileSystem.h"
#include "Template/StringView.h"

namespace Zen
{
	DynamicLibrary::DynamicLibrary()
	{
		::memset(this, 0, sizeof(*this));
		mhModule = NULL;
	}

	template<class TFunc>
	static inline bool readProcAddress(HMODULE dll, TFunc& func, char* funcName)
	{
		func = (TFunc)GetProcAddress(dll, funcName);
		return func != nullptr;
	}

	int DynamicLibrary::InstanceCount = 0;

	bool DynamicLibrary::initialize(int version, bool bCreateNewInstance)
	{
		TCHAR const* path;
		switch( version )
		{
		case 4: path = TEXT("Go/Zen4/Zen.dll"); break;
		case 6: path = TEXT("Go/Zen6/Zen.dll"); break;
		case 7: path = TEXT("Go/Zen7/Zen.dll"); break;
		default:
			LogError("Zen Version No Dll");
			return false;
		}

		if (bCreateNewInstance && InstanceCount > 0)
		{
			StringView filePath = FFileUtility::GetDirectory(path);
			InlineString< 256 > tempDllPath;
			for(;;)
			{
				DateTime time = SystemPlatform::GetLocalTime();
				tempDllPath.format("%s/Zen%d-%d-%d.dll", filePath.toCString(), time.getHour(), time.getMinute(), time.getSecond());
				if (FFileSystem::CopyFile(path, tempDllPath.c_str(), true))
				{
					mDllName = tempDllPath;
					break;
				}
			}
			mhModule = LoadLibrary(mDllName.c_str());
		}
		else
		{
			mhModule = LoadLibrary(path);
		}

		if( mhModule == NULL )
			return false;


		++InstanceCount;

#define LoadZenFunc( FUNC , NAME )\
		if ( !readProcAddress(mhModule,  FUNC, NAME ) )\
		{\
			LogError("Can't Load %s!!" , #FUNC );\
			return false;\
		}

		LoadZenFunc(ZenAddStone, "?ZenAddStone@@YA_NHHH@Z");
		LoadZenFunc(ZenClearBoard, "?ZenClearBoard@@YAXXZ");
		LoadZenFunc(ZenFixedHandicap, "?ZenFixedHandicap@@YAXH@Z");
		LoadZenFunc(ZenGetBestMoveRate, "?ZenGetBestMoveRate@@YAHXZ");
		LoadZenFunc(ZenGetBoardColor, "?ZenGetBoardColor@@YAHHH@Z");
		LoadZenFunc(ZenGetHistorySize, "?ZenGetHistorySize@@YAHXZ");
		LoadZenFunc(ZenGetNextColor, "?ZenGetNextColor@@YAHXZ");
		LoadZenFunc(ZenGetNumBlackPrisoners, "?ZenGetNumBlackPrisoners@@YAHXZ");
		LoadZenFunc(ZenGetNumWhitePrisoners, "?ZenGetNumWhitePrisoners@@YAHXZ");
		LoadZenFunc(ZenGetTerritoryStatictics, "?ZenGetTerritoryStatictics@@YAXQAY0BD@H@Z");
		LoadZenFunc(ZenGetTopMoveInfo, "?ZenGetTopMoveInfo@@YAXHAAH00AAMPADH@Z");
		LoadZenFunc(ZenInitialize, "?ZenInitialize@@YAXPBD@Z");
		LoadZenFunc(ZenIsLegal, "?ZenIsLegal@@YA_NHHH@Z");
		LoadZenFunc(ZenIsSuicide, "?ZenIsSuicide@@YA_NHHH@Z");
		LoadZenFunc(ZenIsThinking, "?ZenIsThinking@@YA_NXZ");
		LoadZenFunc(ZenMakeShapeName, "?ZenMakeShapeName@@YAXHHHPADH@Z");
		LoadZenFunc(ZenPass, "?ZenPass@@YAXH@Z");
		LoadZenFunc(ZenPlay, "?ZenPlay@@YA_NHHH@Z");
		LoadZenFunc(ZenReadGeneratedMove, "?ZenReadGeneratedMove@@YAXAAH0AA_N1@Z");
		
		LoadZenFunc(ZenSetBoardSize, "?ZenSetBoardSize@@YAXH@Z");
		LoadZenFunc(ZenSetKomi, "?ZenSetKomi@@YAXM@Z");
		LoadZenFunc(ZenSetMaxTime, "?ZenSetMaxTime@@YAXM@Z");
		LoadZenFunc(ZenSetNextColor, "?ZenSetNextColor@@YAXH@Z");
		LoadZenFunc(ZenSetNumberOfSimulations, "?ZenSetNumberOfSimulations@@YAXH@Z");
		LoadZenFunc(ZenSetNumberOfThreads, "?ZenSetNumberOfThreads@@YAXH@Z");

		LoadZenFunc(ZenStartThinking, "?ZenStartThinking@@YAXH@Z");
		LoadZenFunc(ZenStopThinking, "?ZenStopThinking@@YAXXZ");
		LoadZenFunc(ZenTimeLeft, "?ZenTimeLeft@@YAXHHH@Z");
		LoadZenFunc(ZenTimeSettings, "?ZenTimeSettings@@YAXHHH@Z");
		LoadZenFunc(ZenUndo, "?ZenUndo@@YA_NH@Z");

		if( version <= 6 )
		{
			LoadZenFunc(ZenSetAmafWeightFactor, "?ZenSetAmafWeightFactor@@YAXM@Z");
			LoadZenFunc(ZenSetPriorWeightFactor, "?ZenSetPriorWeightFactor@@YAXM@Z");
		}
		if( version == 6 )
		{
			LoadZenFunc(ZenSetDCNN, "?ZenSetDCNN@@YAX_N@Z");
			LoadZenFunc(ZenGetPriorKnowledge, "?ZenGetPriorKnowledge@@YAXQAY0BD@H@Z");
		}
		else if( version == 7 )
		{
			LoadZenFunc(ZenGetPolicyKnowledge,"?ZenGetPolicyKnowledge@@YAXQAY0BD@H@Z");
			LoadZenFunc(ZenSetPnLevel,"?ZenSetPnLevel@@YAXH@Z");
			LoadZenFunc(ZenSetPnWeight,"?ZenSetPnWeight@@YAXM@Z");
			LoadZenFunc(ZenSetVnMixRate, "?ZenSetVnMixRate@@YAXM@Z");
		}

#undef LoadZenFunc
		return true;
	}


	void DynamicLibrary::release()
	{
		if (mhModule)
		{
			FreeLibrary(mhModule);
			mhModule = NULL;
			--InstanceCount;
		}

		if (!mDllName.empty())
		{
			FFileSystem::DeleteFile(mDllName.c_str());

		}
	}

	Bot::Bot() :mCore(nullptr)
	{

	}

	bool Bot::setup(IBotCore& core)
	{
		if( !core.caputureResource() )
			return false;
		mCore = &core;
		return true;
	}

	void Bot::release()
	{
		if( mCore )
		{
			mCore->releaseResource();
			mCore = nullptr;
		}
	}

	void Bot::think(ThinkResult& result)
	{
		assert(mCore);
		mCore->think(result);
	}

	bool Bot::playStone(int x, int y, Color color)
	{
		assert(mCore);
		return mCore->playStone(x, y, color);
	}

	void Bot::playPass(Color color)
	{
		assert(mCore);
		return mCore->playPass(color);
	}

	void Bot::startGame(GameSetting const& gameSetting)
	{
		mGameSetting = gameSetting;
		assert(mCore);
		mCore->startGame(mGameSetting);
	}



	void Bot::printBoard(int x, int y)
	{
		using namespace std;
		static char const* dstr[] =
		{
			"¢z","¢s","¢{",
			"¢u","¢q","¢t",
			"¢|","¢r","¢}",
		};

		int size = mGameSetting.boardSize;
		for( int j = 0; j < size; ++j )
		{
			for( int i = 0; i < size; ++i )
			{
				if( i == x && j == y )
				{
					cout << "¡ò";
					continue;
				}

				switch( mCore->getBoardColor(i, j) )
				{
				case Color::Black: cout << "¡³"; break;
				case Color::White: cout << "¡´"; break;
				case Color::Empty:
				{
					int index = 0;
					if( i != 0 )
					{
						index += (i != (size - 1)) ? 1 : 2;
					}
					if( j != 0 )
					{
						index += 3 * ((j != (size - 1)) ? 1 : 2);
					}
					cout << dstr[index];
				}
				break;
				}
			}
			cout << endl;
		}

		//for( int dir = 0; dir < Board::NumDir; ++dir )
		//{
		//	int nx = x + gDirOffset[dir][0];
		//	int ny = y + gDirOffset[dir][1];

		//	if( !mBoard.checkRange(nx, ny) )
		//		continue;
		//	DataType nType = mBoard.getData(nx, ny);
		//	if( nType == StoneColor::eEmpty )
		//		continue;
		//	int life = mBoard.getLife(nx, ny);
		//	std::cout << "dir =" << dir << " life = " << life << std::endl;
		//}
	}

	void Bot::calcTerritoryStatictics(int threshold, int& numB, int& numW)
	{
		numB = 0;
		numW = 0;
		int territoryStatictics[19][19];
		mCore->getTerritoryStatictics(territoryStatictics);
		for( int i = 0; i < mGameSetting.boardSize; ++i )
		{
			for( int j = 0; j < mGameSetting.boardSize; ++j )
			{
				int value = territoryStatictics[j][i];

				Color color = mCore->getBoardColor(i, j);

				if( std::abs(value) > threshold )
				{
					if( value > 0 )
					{
						if( color != Color::Black )
						{
							++numB;
							if( color == Color::White )
								++numB;
						}
					}
					else if( value < 0 )
					{
						if( color != Color::White )
						{
							++numW;
							if( color == Color::Black )
								++numW;
						}
					}
				}
			}
		}
	}

}


int runBotTest()
{
	int numCPU = SystemPlatform::GetProcessorNumber();

	Zen::CoreSetting setting;
	setting.numThreads = numCPU - 1;
	setting.numSimulations = 10000000;
	setting.maxTime = 20;
	typedef Zen::TBotCore< 4 > ZenCoreV4;
	if( !ZenCoreV4::Get().initialize() )
	{
		return -1;
	}
	ZenCoreV4::Get().setCoreSetting(setting);
	typedef Zen::TBotCore< 6 > ZenCoreV6;

	if( !ZenCoreV6::Get().initialize() )
	{
		return -1;
	}
	setting.maxTime /= 2;
	ZenCoreV6::Get().setCoreSetting(setting);

	int MaxRound = 100;
	int curRound = 0;

	int BlackWin = 0;
	int WhiteWin = 0;

	Zen::Bot botA;
	
	Zen::Bot botB;
	botB.setup(ZenCoreV6::Get());

	while( curRound < MaxRound )
	{
		Zen::GameSetting gameSetting;
		gameSetting.boardSize = 13;
		botA.startGame(gameSetting);
		botB.startGame(gameSetting);

		Zen::Bot* players[2] = { &botA , &botB };

		int passCount = 0;
		bool bShowTerritoryStatictics = false;
		int idxCur = 0;
		while( 1 )
		{
			Zen::Bot* curPlayer = players[idxCur];
			Zen::Color color = ZenCoreV6::Get().getNextColor();

			Zen::ThinkResult thinkStep;
			curPlayer->think(thinkStep);


			if( thinkStep.bPass )
			{
				botA.playPass(color);
				botB.playPass(color);
				++passCount;
				if( passCount == 2 )
					break;
			}
			else
			{
				if( !botA.playStone(thinkStep.x, thinkStep.y, color) ||
				   !botB.playStone(thinkStep.x, thinkStep.y, color) )
					break;

				passCount = 0;
			}

			std::cout << thinkStep.x << ' ' << thinkStep.y << ' ' << thinkStep.bPass << ' ' << thinkStep.bResign << std::endl;

			int numB, numW;
			botA.calcTerritoryStatictics(40, numB, numW);
			std::cout << "A:" << numB << " " << numW << " ";
			botB.calcTerritoryStatictics(400, numB, numW);
			std::cout << "B:" << numB << " " << numW << " ";
			std::cout << ZenCoreV6::Get().getBlackPrisonerNum() << " " << ZenCoreV6::Get().getWhitePrisonerNum() << std::endl;
			//std::cout << ZenGetBestMoveRate();
			//std::cout << ZenIsLegal(0, 0, 1);
			botA.printBoard(thinkStep.x, thinkStep.y);

			if( bShowTerritoryStatictics )
			{
				int territoryStatictics[19][19];
				ZenCoreV6::Get().getTerritoryStatictics(territoryStatictics);
				for( int i = 0; i < gameSetting.boardSize; ++i )
				{
					for( int j = 0; j < gameSetting.boardSize; ++j )
					{
						int value = territoryStatictics[j][i];
						if( value > 0 )
						{
							std::cout << 'B' << territoryStatictics[i][j] << ' ';
						}
						else if( value < 0 )
						{
							std::cout << 'W' << -territoryStatictics[i][j] << ' ';
						}
						else
						{
							std::cout << "E00" << ' ';
						}
					}
					std::cout << std::endl;
				}
			}

			idxCur = 1 - idxCur;
		}

		int numB, numW;
		botB.calcTerritoryStatictics(400, numB, numW);

		numB += ZenCoreV6::Get().getBlackPrisonerNum();
		numW += ZenCoreV6::Get().getWhitePrisonerNum(); 

		if( numB > numW )
			BlackWin += 1;
		else
			WhiteWin += 1;


		std::cout << "Black Win = " << BlackWin << std::endl;
		std::cout << "White Win = " << WhiteWin << std::endl;

		++curRound;
	}

	

    return 0;
}

namespace Go
{

	ZenBot::ZenBot() 
	{
		mCoreVersion = -1;
	}

	Zen::CoreSetting ZenBot::GetCoreConfigSetting()
	{
		Zen::CoreSetting setting;
		int numCPU = SystemPlatform::GetProcessorNumber();
		setting.numThreads = numCPU - 2;
		setting.numSimulations = ::Global::GameConfig().getIntValue("numSimulations", "ZenSetting", 20000000);
		setting.maxTime = ::Global::GameConfig().getFloatValue("maxTime", "ZenSetting", 25);
		return setting;
	}

	bool ZenBot::initialize(void* settingData)
	{
		auto const& setting = (settingData) ? *static_cast<Zen::CoreSetting*>(settingData) : GetCoreConfigSetting();
		mCoreVersion = setting.version;
		switch( setting.version )
		{
		case 4: mCore.reset(buildCoreT< 4 >()); break;
		case 6: mCore.reset(buildCoreT< 6 >()); break;
		case 7: mCore.reset(buildCoreT< 7 >()); break;
		default:
			break;
		}

		if( mCore == nullptr )
			return false;

		mCore->caputureResource();
		mCore->setCoreSetting(setting);
		return true;
	}

	void ZenBot::destroy()
	{
		mCore->stopThink();
		mCore->releaseResource();

		switch( mCoreVersion )
		{
		case 4: static_cast<Zen::TBotCore< 4 >*>(mCore.get())->release(); break;
		case 6: static_cast<Zen::TBotCore< 6 >*>(mCore.get())->release(); break;
		case 7: static_cast<Zen::TBotCore< 7 >*>(mCore.get())->release(); break;
		}
		mCore.release();
	}

	bool ZenBot::setupGame(GameSetting const& setting)
	{
		mCore->startGame(setting);
		bWaitResult = false;
		bRequestUndoDone = false;
		return true;
	}

	bool ZenBot::restart(GameSetting const& setting)
{
		mCore->restart();
		mCore->startGame(setting);
		bWaitResult = false;
		bRequestUndoDone = false;
		return true;
	}

	EBotExecResult ZenBot::playStone(int x, int y, int color)
	{
		if (mCore->playStone(x, y, ToZColor(color)))
			return BOT_OK;
		return BOT_FAIL;
	}

	EBotExecResult ZenBot::playPass(int color)
	{
		auto curColor = mCore->getNextColor();
		mCore->playPass(ToZColor(color));
		return curColor != mCore->getNextColor() ? BOT_OK : BOT_FAIL;
	}

	EBotExecResult ZenBot::undo()
	{
		return mCore->undo() ? BOT_OK : BOT_FAIL;
	}

	bool ZenBot::requestUndo()
{
		if( bWaitResult )
		{
			bWaitResult = false;
			mCore->stopThink();
		}
		else
		{
			mCore->undo();
		}
		bRequestUndoDone = true;
		return true;
	}

	bool ZenBot::thinkNextMove(int color)
	{
		mCore->startThink(ToZColor(color));
		bWaitResult = true;
		requestColor = color;
		return true;
	}

	bool ZenBot::isThinking()
	{
		return mCore->isThinking();
	}

	void ZenBot::update(IGameCommandListener& listener)
	{
		if( bRequestUndoDone )
		{
			GameCommand com;
			com.id = GameCommand::eUndo;
			listener.notifyCommand(com);
		}
		if( bWaitResult )
		{
			if( !mCore->isThinking() )
			{
				bWaitResult = false;

				Zen::ThinkResult thinkStep;
				mCore->getThinkResult(thinkStep);

				GameCommand com;
				if( thinkStep.bPass )
				{
					com.id = GameCommand::ePass;
				}
				else if( thinkStep.bResign )
				{
					com.id = GameCommand::eResign;
				}
				else
				{
					mCore->playStone(thinkStep.x, thinkStep.y, ToZColor(requestColor));
					com.id = GameCommand::ePlayStone;
					com.playColor = requestColor;
					com.pos[0] = thinkStep.x;
					com.pos[1] = thinkStep.y;
				}
				listener.notifyCommand(com);

				{
					GameCommand winRateCom;
					winRateCom.setParam(ZenGameParam::eWinRate, (float)thinkStep.winRate);
					listener.notifyCommand(winRateCom);
				}
			}
			else
			{
				Zen::ThinkInfo infoList[5];
				if ( mCore->getBestThinkMove(infoList, ARRAY_SIZE(infoList)) )
				{
					std::sort(infoList, infoList + ARRAY_SIZE(infoList),
						[](auto& lhs, auto &rhs)
						{
							return lhs.winRate > rhs.winRate;
						}
					);

					auto& bestInfo = infoList[0];
					GameCommand com;
					com.setParam(ZenGameParam::eBestMoveVertex, &bestInfo);
					listener.notifyCommand(com);
				}
			}
		}
	}

	bool ZenBot::getMetaData(int id, uint8* dataBuffer, int size)
	{

		switch( id )
		{
		case ZenMetaParam::eVersion:
			{
				if( size == sizeof(int) )
				{
					*((int*)dataBuffer) = mCoreVersion;
					return true;
				}
			}
		default:
			break;
		}
		return false;
	}

	EBotExecResult ZenBot::readBoard(int* outState)
	{
		int size = mCore->getBoardSize();
		for (int j = 0; j < size; ++j)
		{
			for (int i = 0; i < size; ++i)
			{
				*outState = FormZColor(mCore->getBoardColor(i, j));
				++outState;
			}
		}
		return BOT_OK;
	}

}