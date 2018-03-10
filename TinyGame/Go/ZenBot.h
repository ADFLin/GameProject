#pragma once

#ifndef ZenBot_h__
#define ZenBot_h__

//#include "zen.h"

#include <cassert>
#include "GoBot.h"

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
		//v7
		int   PnLevel;
		float PnWeight;
		float VnMixRate;

		CoreSetting()
		{
			numThreads = 1;
			numSimulations = 1000000;
			//numSimulations = 100000;
			amafWightFactor = 1.0f;
			priorWightFactor = 1.0f;
			maxTime = 60.0f;
			bUseDCNN = true;

			PnLevel = 3;
			PnWeight = 1.0f;
			VnMixRate = 0.75;

		}
	};

	using Go::GameSetting;


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

		virtual ~IBotCore(){}
		virtual void     setCoreSetting(CoreSetting const& setting) = 0;
		virtual void     startGame(GameSetting const& setting) = 0;
		virtual void     doInitializeResource() = 0;
		virtual void     doReleaseResource() = 0;
		virtual void     think(ThinkResult& result, Color color = Color::Empty) = 0;
		virtual bool     playStone(int x, int y, Color color) = 0;
		virtual void     playPass(Color color = Color::Empty) = 0;
		virtual bool     undo() = 0;
		virtual Color    getBoardColor(int x, int y) = 0;
		virtual void     getTerritoryStatictics(int territoryStatictics[19][19]) = 0;
		virtual int      getBlackPrisonerNum() = 0;
		virtual int      getWhitePrisonerNum() = 0;
		virtual Color    getNextColor() = 0;
		virtual void     startThink(Color color = Color::Empty) = 0;
		virtual void     stopThink() = 0;
		virtual bool     isThinking() = 0;
		virtual void     getThinkResult(ThinkResult& result) = 0;

	};

	class StaticLibrary
	{
	public:
		enum { IsSingleton = 1, };

		bool initialize(TCHAR const* name, int version)
		{
			return true;
		}
		void release()
		{

		}

		bool isInitialized() const
		{
			return true;
		}
	};

	class DynamicLibrary
	{
	public:
		DynamicLibrary();

		enum { IsSingleton = 0, };

		bool initialize(int version);
		void release()
		{
			if( mhModule )
			{
				FreeLibrary(mhModule);
				mhModule = NULL;
			}
		}

		bool isInitialized() const
		{
			return mhModule != NULL;
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
		
		void(__cdecl *ZenSetBoardSize)(int);

		void(__cdecl *ZenSetKomi)(float);
		void(__cdecl *ZenSetMaxTime)(float);
		void(__cdecl *ZenSetNextColor)(int);
		void(__cdecl *ZenSetNumberOfSimulations)(int);
		void(__cdecl *ZenSetNumberOfThreads)(int);
		
		void(__cdecl *ZenStartThinking)(int);
		void(__cdecl *ZenStopThinking)(void);
		void(__cdecl *ZenTimeLeft)(int, int, int);
		void(__cdecl *ZenTimeSettings)(int, int, int);
		bool(__cdecl *ZenUndo)(int);


		void(__cdecl *ZenSetAmafWeightFactor)(float);
		void(__cdecl *ZenSetPriorWeightFactor)(float);
		//v6
		void(__cdecl *ZenSetDCNN)(bool);
		void(__cdecl *ZenGetPriorKnowledge)(int(*const)[19]);
		//v7
		void( __cdecl *ZenGetPolicyKnowledge)(int(*const)[19]);
		void( __cdecl *ZenSetPnLevel)(int);
		void( __cdecl *ZenSetPnWeight)(float);
		void( __cdecl *ZenSetVnMixRate)(float);
	};

	template< int Version , class Lib = DynamicLibrary >
	class TBotCore : public IBotCore , public Lib
	{
		typedef Lib ZenLibrary;
	public:

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


		static typename std::enable_if< !Lib::IsSingleton , TBotCore* >::type
		Create()
		{
			return new TBotCore;
		}

		static TBotCore& Get()
		{
			static TBotCore instance;
			return instance;
		}

		bool isInitialized() const { return ZenLibrary::isInitialized(); }

		bool initialize()
		{
			if( !ZenLibrary::initialize(Version) )
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
			ZenFixedHandicap(setting.fixedHandicap);
			ZenSetKomi(setting.komi);
			ZenSetNextColor(setting.bBlackFrist ? (int)Color::Black : (int)Color::White);
			if( Version == 6 )
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

		void think(ThinkResult& result , Color color )
		{
			if( color == Color::Empty )
				color = (Color)ZenGetNextColor();
			ZenStartThinking((int)color);

			while( ZenIsThinking() )
			{
				SystemPlatform::Sleep(1);
				//std::cout << ZenGetBestMoveRate() << std::endl;
			}
			ZenReadGeneratedMove(result.x, result.y, result.bPass, result.bResign);
		}

		void startThink(Color color) override
		{
			if( color == Color::Empty )
				color = (Color)ZenGetNextColor();
			ZenStartThinking((int)color);
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

		bool playStone(int x, int y, Color color)
		{
			return ZenPlay(x, y, (int)color);
		}
		void playPass(Color color)
		{
			ZenPass((int)color);
		}
		bool undo()
		{
			return ZenUndo(ZenGetNextColor() == (int)Color::Black ? (int)Color::White : (int)Color::Black);
		}

		void setCoreSetting(CoreSetting const& setting)
		{
			ZenSetNumberOfThreads(setting.numThreads);
			ZenSetNumberOfSimulations(setting.numSimulations);
			if( Version <= 6 )
			{
				ZenSetAmafWeightFactor(setting.amafWightFactor);
				ZenSetPriorWeightFactor(setting.priorWightFactor);
			}
			else if( Version == 7 )
			{
				ZenSetPnLevel(setting.PnLevel);
				ZenSetPnWeight(setting.PnWeight);
				ZenSetVnMixRate(setting.VnMixRate);
			}
			ZenSetMaxTime(setting.maxTime);
		}

		Color getBoardColor(int x, int y) { return (Color)ZenGetBoardColor(x, y); }
		void  getTerritoryStatictics(int territoryStatictics[19][19]) override
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
		bool playStone(int x, int y, Color color);
		void playPass(Color color);
		void startGame(GameSetting const& gameSetting);
		void printBoard(int x, int y);
		void calcTerritoryStatictics(int threshold, int& numB, int& numW);

		IBotCore*   mCore;
		GameSetting mGameSetting;
	};


}

#include "SystemPlatform.h"

namespace Go
{

	class ZenBot : public IBotInterface
	{

	public:

		ZenBot(int version = 6)
			:mCoreVersion( version )
		{

		}

		template< int Version >
		Zen::IBotCore* buildCoreT()
		{
			typedef Zen::TBotCore< Version > ZenCore;

			ZenCore* core = ZenCore::Create();
			if( !core->initialize() )
				return nullptr;
			return core;
		}

		virtual bool initilize(void* settingData) override
		{

			switch(  mCoreVersion )
			{
			case 4: mCore.reset( buildCoreT< 4 >() ); break;
			case 6: mCore.reset( buildCoreT< 6 >() ); break;
			case 7: mCore.reset( buildCoreT< 7 >() ); break;
			default:
				break;
			}

			if( mCore == nullptr )
				return false;

			mCore->caputureResource();

			if( settingData )
			{
				mCore->setCoreSetting(*static_cast<Zen::CoreSetting*>(settingData));
			}
			else
			{
				Zen::CoreSetting setting;
				mCore->setCoreSetting(setting);
			}


			return true;
		}

		virtual void destroy() override
		{
			mCore->stopThink();
			mCore->releaseResource();

			switch( mCoreVersion )
			{
			case 4: static_cast< Zen::TBotCore< 4 >* >(mCore.get())->release(); break;
			case 6: static_cast< Zen::TBotCore< 6 >* >(mCore.get())->release(); break;
			case 7: static_cast< Zen::TBotCore< 7 >* >(mCore.get())->release(); break;
			}
			mCore.release();
		}

		virtual bool setupGame(GameSetting const& setting) override
		{
			mCore->startGame(setting);
			bWaitResult = false;
			return true;
		}


		virtual bool playStone(int x, int y, int color) override
		{
			return mCore->playStone(x, y, ToZColor(color));
		}

		virtual bool playPass(int color) override
		{
			mCore->playPass(ToZColor(color));
			return true;
		}
		virtual bool undo() override
		{
			return mCore->undo();
		}
		virtual bool thinkNextMove(int color) override
		{
			mCore->startThink();
			bWaitResult = true;
			requestColor = color;
			return true;
		}
		virtual bool isThinking() override
		{
			return mCore->isThinking();
		}

		virtual void update(IGameCommandListener& listener) override
		{
			if( !bWaitResult )
				return;

			if( mCore->isThinking() )
				return;

			Zen::ThinkResult thinkStep;
			mCore->getThinkResult(thinkStep);

			GameCommand com;
			if( thinkStep.bPass )
			{
				com.id = GameCommand::ePass;
				listener.notifyCommand(com);
			}
			else if( thinkStep.bResign )
			{
				com.id = GameCommand::eResign;
				listener.notifyCommand(com);
			}
			else
			{
				mCore->playStone(thinkStep.x, thinkStep.y, ToZColor(requestColor));
				com.id = GameCommand::ePlay;
				com.playColor = requestColor;
				com.pos[0] = thinkStep.x;
				com.pos[1] = thinkStep.y;
				listener.notifyCommand(com);
			}
			bWaitResult = false;
		}

		static Zen::Color ToZColor(int color)
		{
			switch( color )
			{
			case StoneColor::eBlack: return Zen::Color::Black;
			case StoneColor::eWhite: return Zen::Color::White;
			case StoneColor::eEmpty: return Zen::Color::Empty;
			}

			assert(0);
			return Zen::Color::Empty;
		}


		int  mCoreVersion = 6;
		std::unique_ptr< Zen::IBotCore > mCore;
		int  requestColor;
		bool bWaitResult;
	};



}


#endif // ZenBot_h__