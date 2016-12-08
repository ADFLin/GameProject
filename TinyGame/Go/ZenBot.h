#pragma once

#ifndef ZenBot_h__
#define ZenBot_h__

//#include "zen.h"

#include <cassert>

namespace Zen
{
	enum class Color
	{
		Empty = 0,
		Black = 2,
		White = 1,
	};

	struct ThinkResult
	{
		int x, y;
		bool bResign;
		bool bPass;
	};

	struct CoreSetting
	{
		int   numThreads;
		int   numSimulations;
		float amafWightFactor;
		float priorWightFactor;
		float maxTime;
		//V6
		bool  bUseDCNN;
		CoreSetting()
		{
			numThreads = 1;
			//numSimulations = 1000000;
			numSimulations = 100000;
			amafWightFactor = 1.0f;
			priorWightFactor = 1.0f;
			maxTime = 60.0f;
			bUseDCNN = true;
		}
	};

	struct GameSetting
	{
		int   boardSize;
		int   fixHandicap;
		float komi;
		bool  bBlackFrist;
		GameSetting()
		{
			boardSize = 19;
			fixHandicap = 0;
			komi = 6.5;
			bBlackFrist = true;
		}
	};

	class IBotCore
	{
	public:
		IBotCore()
		{
			mbCaputured = false;
		}
		bool caputureResource()
		{
			if( mbCaputured )
				return false;

			mbCaputured = true;
			doInitializeResource();
			return true;
		}
		void releaseResource()
		{
			doReleaseResource();
			mbCaputured = false;
		}

		bool isCaptured() { return mbCaputured; }
		bool mbCaputured;

		virtual void     startGame(GameSetting const& setting) = 0;
		virtual void     doInitializeResource() = 0;
		virtual void     doReleaseResource() = 0;
		virtual void     think(ThinkResult& result) = 0;
		virtual bool     play(int x, int y, Color color) = 0;
		virtual void     pass(Color color) = 0;
		virtual Color    getBoardColor(int x, int y) = 0;
		virtual void     getTerritoryStatictics(int territoryStatictics[19][19]) = 0;
		virtual int      getBlackPrisonerNum() = 0;
		virtual int      getWhitePrisonerNum() = 0;
		virtual Color    getNextColor() = 0;
		virtual void     startThink() = 0;
		virtual void     stopThink() = 0;
		virtual bool     isThinking() = 0;
		virtual void     getThinkResult(ThinkResult& result) = 0;

	};

	class StaticLibrary
	{
	public:
		bool initialize(TCHAR const* name, int version)
		{
			return true;
		}
		void release()
		{

		}
	};

	class DynamicLibrary
	{
	public:
		DynamicLibrary();

		bool initialize(TCHAR const* name, int version);
		void release()
		{
			if( mhModule )
			{
				FreeLibrary(mhModule);
				mhModule = NULL;
			}
		}

		HMODULE mhModule;
		bool(__cdecl *ZenAddStone)(int, int, int);
		void(__cdecl *ZenClearBoard)(void);
		void(__cdecl *ZenFixedHandicap)(int);
		int(__cdecl *ZenGetBestMoveRate)(void);
		int(__cdecl *ZenGetBoardColor)(int, int);
		int(__cdecl *ZenGetHistorySize)(void);
		int(__cdecl *ZenGetNextColor)(void);
		int(__cdecl *ZenGetNumBlackPrisoners)(void);
		int(__cdecl *ZenGetNumWhitePrisoners)(void);
		void(__cdecl *ZenGetPriorKnowledge)(int(*const)[19]);
		void(__cdecl *ZenGetTerritoryStatictics)(int(*const)[19]);
		void(__cdecl *ZenGetTopMoveInfo)(int, int &, int &, int &, float &, char *, int);
		void(__cdecl *ZenInitialize)(char const *);
		bool(__cdecl *ZenIsInitialized)(void);
		bool(__cdecl *ZenIsLegal)(int, int, int);
		bool(__cdecl *ZenIsSuicide)(int, int, int);
		bool(__cdecl *ZenIsThinking)(void);
		void(__cdecl *ZenMakeShapeName)(int, int, int, char *, int);
		void(__cdecl *ZenPass)(int);
		bool(__cdecl *ZenPlay)(int, int, int);
		void(__cdecl *ZenReadGeneratedMove)(int &, int &, bool &, bool &);
		void(__cdecl *ZenSetAmafWeightFactor)(float);
		void(__cdecl *ZenSetBoardSize)(int);
		void(__cdecl *ZenSetDCNN)(bool);
		void(__cdecl *ZenSetKomi)(float);
		void(__cdecl *ZenSetMaxTime)(float);
		void(__cdecl *ZenSetNextColor)(int);
		void(__cdecl *ZenSetNumberOfSimulations)(int);
		void(__cdecl *ZenSetNumberOfThreads)(int);
		void(__cdecl *ZenSetPriorWeightFactor)(float);
		void(__cdecl *ZenStartThinking)(int);
		void(__cdecl *ZenStopThinking)(void);
		void(__cdecl *ZenTimeLeft)(int, int, int);
		void(__cdecl *ZenTimeSettings)(int, int, int);
		bool(__cdecl *ZenUndo)(int);
	};

	template< class Lib, int Version >
	class TBotCore : public IBotCore , public Lib
	{
		typedef Lib ZenLibrary;
	public:
		static TBotCore& Get()
		{
			static TBotCore instance;
			return instance;
		}

		bool initialize(TCHAR const* dllName = nullptr)
		{
			if( !ZenLibrary::initialize(dllName, Version) )
				return false;
			ZenInitialize(nullptr);
			return true;
		}

		void release()
		{
			ZenLibrary::release();
		}

		void startGame(GameSetting const& setting)
		{
			ZenClearBoard();
			ZenSetBoardSize(setting.boardSize);
			ZenFixedHandicap(setting.fixHandicap);
			ZenSetKomi(setting.komi);
			ZenSetNextColor(setting.bBlackFrist ? (int)Color::Black : (int)Color::White);
			if( Version >= 6 )
			{
				ZenSetDCNN(true);
			}
		}

		void doInitializeResource()
		{

			ZenClearBoard();
		}

		void doReleaseResource()
		{

		}

		void think(ThinkResult& result)
		{
			int color = ZenGetNextColor();
			ZenStartThinking(color);

			while( ZenIsThinking() )
			{
				Sleep(1);
				//std::cout << ZenGetBestMoveRate() << std::endl;
			}
			ZenReadGeneratedMove(result.x, result.y, result.bPass, result.bResign);
		}

		void startThink()
		{
			int color = ZenGetNextColor();
			ZenStartThinking(color);
		}
		void stopThink()
		{
			ZenStopThinking();
		}
		bool isThinking() { return ZenIsThinking(); }
		void getThinkResult(ThinkResult& result)
		{
			ZenReadGeneratedMove(result.x, result.y, result.bPass, result.bResign);
		}

		bool play(int x, int y, Color color)
		{
			return ZenPlay(x, y, (int)color);
		}
		void     pass(Color color)
		{
			ZenPass((int)color);
		}

		void setSetting(CoreSetting const& setting)
		{
			ZenSetNumberOfThreads(setting.numThreads);
			ZenSetNumberOfSimulations(setting.numSimulations);
			ZenSetAmafWeightFactor(setting.amafWightFactor);
			ZenSetPriorWeightFactor(setting.priorWightFactor);
			ZenSetMaxTime(setting.maxTime);
		}

		Color getBoardColor(int x, int y) { return (Color)ZenGetBoardColor(x, y); }
		void     getTerritoryStatictics(int territoryStatictics[19][19]) override
		{
			ZenGetTerritoryStatictics(territoryStatictics);
		}
		void getPriorKnowledge(int PriorKnowledge[19][19])
		{
			if( Version >= 6 )
			{
				ZenGetPriorKnowledge(PriorKnowledge);
			}	
		}
		int getBlackPrisonerNum() { return ZenGetNumBlackPrisoners(); }
		int getWhitePrisonerNum() { return ZenGetNumWhitePrisoners(); }
		Color getNextColor() { return Color(ZenGetNextColor()); }

	private:
		TBotCore() {}

		TBotCore(TBotCore const&) = delete;
		TBotCore& operator = (TBotCore const&) = delete;
	};

	class Bot
	{
	public:
		Bot();
		bool setup(IBotCore& core);
		void release();
		GameSetting const & getGameSetting() { return mGameSetting; }

		void think(ThinkResult& result);
		bool play(int x, int y, Color color);
		void pass(Color color);
		void startGame(GameSetting const& gameSetting);
		void printBoard(int x, int y);
		void calcTerritoryStatictics(int threshold, int& numB, int& numW);

		IBotCore*   mCore;
		GameSetting mGameSetting;
	};


}


#endif // ZenBot_h__