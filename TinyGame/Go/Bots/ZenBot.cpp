
#include "ZenBot.h"

#include "SystemPlatform.h"

#include <iostream>

#include "GameGlobal.h"
#include "PropertyKey.h"

namespace Zen
{
	DynamicLibrary::DynamicLibrary()
	{
		::memset(this, 0, sizeof(*this));
		mhModule = NULL;
	}

	template<class FunType>
	static inline bool readProcAddress(HMODULE dll, FunType& fun, char* funName)
	{
		fun = (FunType)GetProcAddress(dll, funName);
		return fun != nullptr;
	}

	bool DynamicLibrary::initialize(int version)
	{
		TCHAR const* name;
		switch( version )
		{
		case 4: name = TEXT("Go/Zen4/Zen.dll"); break;
		case 6: name = TEXT("Go/Zen6/Zen.dll"); break;
		case 7: name = TEXT("Go/Zen7/Zen.dll"); break;
		default:
			LogError("Zen Version No Dll");
			return false;
		}

		mhModule = LoadLibrary(name);
		if( mhModule == NULL )
			return false;

#define LoadZenFun( FN , NAME )\
		if ( !readProcAddress(mhModule,  FN, NAME ) )\
		{\
			LogError("Can't Load %s!!" , #FN );\
			return false;\
		}

		LoadZenFun(ZenAddStone, "?ZenAddStone@@YA_NHHH@Z");
		LoadZenFun(ZenClearBoard, "?ZenClearBoard@@YAXXZ");
		LoadZenFun(ZenFixedHandicap, "?ZenFixedHandicap@@YAXH@Z");
		LoadZenFun(ZenGetBestMoveRate, "?ZenGetBestMoveRate@@YAHXZ");
		LoadZenFun(ZenGetBoardColor, "?ZenGetBoardColor@@YAHHH@Z");
		LoadZenFun(ZenGetHistorySize, "?ZenGetHistorySize@@YAHXZ");
		LoadZenFun(ZenGetNextColor, "?ZenGetNextColor@@YAHXZ");
		LoadZenFun(ZenGetNumBlackPrisoners, "?ZenGetNumBlackPrisoners@@YAHXZ");
		LoadZenFun(ZenGetNumWhitePrisoners, "?ZenGetNumWhitePrisoners@@YAHXZ");
		LoadZenFun(ZenGetTerritoryStatictics, "?ZenGetTerritoryStatictics@@YAXQAY0BD@H@Z");
		LoadZenFun(ZenGetTopMoveInfo, "?ZenGetTopMoveInfo@@YAXHAAH00AAMPADH@Z");
		LoadZenFun(ZenInitialize, "?ZenInitialize@@YAXPBD@Z");
		LoadZenFun(ZenIsLegal, "?ZenIsLegal@@YA_NHHH@Z");
		LoadZenFun(ZenIsSuicide, "?ZenIsSuicide@@YA_NHHH@Z");
		LoadZenFun(ZenIsThinking, "?ZenIsThinking@@YA_NXZ");
		LoadZenFun(ZenMakeShapeName, "?ZenMakeShapeName@@YAXHHHPADH@Z");
		LoadZenFun(ZenPass, "?ZenPass@@YAXH@Z");
		LoadZenFun(ZenPlay, "?ZenPlay@@YA_NHHH@Z");
		LoadZenFun(ZenReadGeneratedMove, "?ZenReadGeneratedMove@@YAXAAH0AA_N1@Z");
		
		LoadZenFun(ZenSetBoardSize, "?ZenSetBoardSize@@YAXH@Z");
		LoadZenFun(ZenSetKomi, "?ZenSetKomi@@YAXM@Z");
		LoadZenFun(ZenSetMaxTime, "?ZenSetMaxTime@@YAXM@Z");
		LoadZenFun(ZenSetNextColor, "?ZenSetNextColor@@YAXH@Z");
		LoadZenFun(ZenSetNumberOfSimulations, "?ZenSetNumberOfSimulations@@YAXH@Z");
		LoadZenFun(ZenSetNumberOfThreads, "?ZenSetNumberOfThreads@@YAXH@Z");

		LoadZenFun(ZenStartThinking, "?ZenStartThinking@@YAXH@Z");
		LoadZenFun(ZenStopThinking, "?ZenStopThinking@@YAXXZ");
		LoadZenFun(ZenTimeLeft, "?ZenTimeLeft@@YAXHHH@Z");
		LoadZenFun(ZenTimeSettings, "?ZenTimeSettings@@YAXHHH@Z");
		LoadZenFun(ZenUndo, "?ZenUndo@@YA_NH@Z");

		if( version <= 6 )
		{
			LoadZenFun(ZenSetAmafWeightFactor, "?ZenSetAmafWeightFactor@@YAXM@Z");
			LoadZenFun(ZenSetPriorWeightFactor, "?ZenSetPriorWeightFactor@@YAXM@Z");
		}
		if( version == 6 )
		{
			LoadZenFun(ZenSetDCNN, "?ZenSetDCNN@@YAX_N@Z");
			LoadZenFun(ZenGetPriorKnowledge, "?ZenGetPriorKnowledge@@YAXQAY0BD@H@Z");
		}
		else if( version == 7 )
		{
			LoadZenFun(ZenGetPolicyKnowledge,"?ZenGetPolicyKnowledge@@YAXQAY0BD@H@Z");
			LoadZenFun(ZenSetPnLevel,"?ZenSetPnLevel@@YAXH@Z");
			LoadZenFun(ZenSetPnWeight,"?ZenSetPnWeight@@YAXM@Z");
			LoadZenFun(ZenSetVnMixRate, "?ZenSetVnMixRate@@YAXM@Z");
		}

#undef ZenFunLoad
		return true;
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

	bool ZenBot::restart()
	{
		mCore->restart();
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

	bool ZenBot::playPass(int color)
	{
		mCore->playPass(ToZColor(color));
		return true;
	}

	bool ZenBot::undo()
	{
		return mCore->undo();
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