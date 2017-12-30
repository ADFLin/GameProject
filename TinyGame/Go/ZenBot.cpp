
#include "ZenBot.h"

#include "SystemPlatform.h"

//#include <sysinfoapi.h>
//#include <process.h>
#include <iostream>

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

	bool DynamicLibrary::initialize(TCHAR const* name, int version)
	{
		assert(name);

		mhModule = LoadLibrary(name);
		if( mhModule == NULL )
			return false;

#define ZenFunLoad( FN , NAME )\
		if ( !readProcAddress(mhModule,  FN, NAME ) )\
			return false;

		ZenFunLoad(ZenAddStone, "?ZenAddStone@@YA_NHHH@Z");
		ZenFunLoad(ZenClearBoard, "?ZenClearBoard@@YAXXZ");
		ZenFunLoad(ZenFixedHandicap, "?ZenFixedHandicap@@YAXH@Z");
		ZenFunLoad(ZenGetBestMoveRate, "?ZenGetBestMoveRate@@YAHXZ");
		ZenFunLoad(ZenGetBoardColor, "?ZenGetBoardColor@@YAHHH@Z");
		ZenFunLoad(ZenGetHistorySize, "?ZenGetHistorySize@@YAHXZ");
		ZenFunLoad(ZenGetNextColor, "?ZenGetNextColor@@YAHXZ");
		ZenFunLoad(ZenGetNumBlackPrisoners, "?ZenGetNumBlackPrisoners@@YAHXZ");
		ZenFunLoad(ZenGetNumWhitePrisoners, "?ZenGetNumWhitePrisoners@@YAHXZ");
		ZenFunLoad(ZenGetTerritoryStatictics, "?ZenGetTerritoryStatictics@@YAXQAY0BD@H@Z");
		ZenFunLoad(ZenGetTopMoveInfo, "?ZenGetTopMoveInfo@@YAXHAAH00AAMPADH@Z");
		ZenFunLoad(ZenInitialize, "?ZenInitialize@@YAXPBD@Z");
		ZenFunLoad(ZenIsLegal, "?ZenIsLegal@@YA_NHHH@Z");
		ZenFunLoad(ZenIsSuicide, "?ZenIsSuicide@@YA_NHHH@Z");
		ZenFunLoad(ZenIsThinking, "?ZenIsThinking@@YA_NXZ");
		ZenFunLoad(ZenMakeShapeName, "?ZenMakeShapeName@@YAXHHHPADH@Z");
		ZenFunLoad(ZenPass, "?ZenPass@@YAXH@Z");
		ZenFunLoad(ZenPlay, "?ZenPlay@@YA_NHHH@Z");
		ZenFunLoad(ZenReadGeneratedMove, "?ZenReadGeneratedMove@@YAXAAH0AA_N1@Z");
		
		ZenFunLoad(ZenSetBoardSize, "?ZenSetBoardSize@@YAXH@Z");
		ZenFunLoad(ZenSetKomi, "?ZenSetKomi@@YAXM@Z");
		ZenFunLoad(ZenSetMaxTime, "?ZenSetMaxTime@@YAXM@Z");
		ZenFunLoad(ZenSetNextColor, "?ZenSetNextColor@@YAXH@Z");
		ZenFunLoad(ZenSetNumberOfSimulations, "?ZenSetNumberOfSimulations@@YAXH@Z");
		ZenFunLoad(ZenSetNumberOfThreads, "?ZenSetNumberOfThreads@@YAXH@Z");

		ZenFunLoad(ZenStartThinking, "?ZenStartThinking@@YAXH@Z");
		ZenFunLoad(ZenStopThinking, "?ZenStopThinking@@YAXXZ");
		ZenFunLoad(ZenTimeLeft, "?ZenTimeLeft@@YAXHHH@Z");
		ZenFunLoad(ZenTimeSettings, "?ZenTimeSettings@@YAXHHH@Z");
		ZenFunLoad(ZenUndo, "?ZenUndo@@YA_NH@Z");

		if( version <= 6 )
		{
			ZenFunLoad(ZenSetAmafWeightFactor, "?ZenSetAmafWeightFactor@@YAXM@Z");
			ZenFunLoad(ZenSetPriorWeightFactor, "?ZenSetPriorWeightFactor@@YAXM@Z");
		}
		if( version == 6 )
		{
			ZenFunLoad(ZenSetDCNN, "?ZenSetDCNN@@YAX_N@Z");
			ZenFunLoad(ZenGetPriorKnowledge, "?ZenGetPriorKnowledge@@YAXQAY0BD@H@Z");
		}
		else if( version == 7 )
		{
			ZenFunLoad(ZenGetPolicyKnowledge,"?ZenGetPolicyKnowledge@@YAXQAY0BD@H@Z");
			ZenFunLoad(ZenSetPnLevel,"?ZenSetPnLevel@@YAXH@Z");
			ZenFunLoad(ZenSetPnWeight,"?ZenSetPnWeight@@YAXM@Z");
			ZenFunLoad(ZenSetVnMixRate, "?ZenSetVnMixRate@@YAXM@Z");
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
	typedef Zen::TBotCore< Zen::DynamicLibrary , 4 > ZenCoreV4;
	if( !ZenCoreV4::Get().initialize(TEXT("ZenV4.dll")) )
	{
		return -1;
	}
	ZenCoreV4::Get().setCoreSetting(setting);
	typedef Zen::TBotCore< Zen::DynamicLibrary , 6 > ZenCoreV6;

	if( !ZenCoreV6::Get().initialize(TEXT("Zen.dll")) )
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
