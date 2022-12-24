#pragma once

#ifndef ZenBot_h__
#define ZenBot_h__

//#include "zen.h"

#include <cassert>
#include "Go/GoBot.h"
#include "Platform/PlatformModule.h"

namespace Zen
{

	enum class Color
	{
		Empty = 0,
		White = 1,
		Black = 2,
	};

	struct ThinkResult
	{
		int x, y;
		bool bResign;
		bool bPass;
		int  winRate;
	};

	using PlayVertex = ::Go::PlayVertex;
	struct ThinkInfo
	{
		PlayVertex v;
		float winRate;
		std::vector< PlayVertex > vSeq;
	};

	struct TerritoryInfo
	{
		static int const FieldValue = 300;
		int map[19][19];
		int fieldCounts[2];
		int totalRemainFields[2];
	};

	struct CoreSetting
	{
		int   version;
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
			version = 7;
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
		virtual void     restart() = 0;
		virtual void     doInitializeResource() = 0;
		virtual void     doReleaseResource() = 0;
		virtual void     think(ThinkResult& result, Color color = Color::Empty) = 0;
		virtual bool     playStone(int x, int y, Color color) = 0;
		virtual void     playPass(Color color = Color::Empty) = 0;
		virtual bool     undo() = 0;
		virtual int      getBoardSize() const = 0;
		virtual Color    getBoardColor(int x, int y) = 0;
		virtual void     getTerritoryStatictics(int territoryStatictics[19][19]) = 0;
		virtual int      getBlackPrisonerNum() = 0;
		virtual int      getWhitePrisonerNum() = 0;
		virtual Color    getNextColor() = 0;
		virtual void     startThink(Color color = Color::Empty) = 0;
		virtual void     stopThink() = 0;
		virtual bool     isThinking() = 0;
		virtual void     getThinkResult(ThinkResult& result) = 0;
		virtual bool     getBestThinkMove(ThinkInfo info[], int num) = 0;
		virtual void     getTerritoryStatictics( TerritoryInfo& info ) = 0;
		virtual int      getHistorySize() const = 0;

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

		bool initialize(int version, bool bCreateNewInstance);
		void release();

		bool isInitialized() const
		{
			return mhModule != nullptr;
		}

		static int InstanceCount;

		FPlatformModule::Handle mhModule;
		std::string mDllName;

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
		using ZenLibrary = Lib;
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

		template< class T = TBotCore<Version, Lib > >
		static typename std::enable_if< !Lib::IsSingleton , T* >::type
		Create()
		{
			return new T;
		}

		static TBotCore& Get()
		{
			static TBotCore instance;
			return instance;
		}

		bool isInitialized() const { return ZenLibrary::isInitialized(); }

		bool initialize(bool bCreateNewInstance = true)
		{
			if( !ZenLibrary::initialize(Version, bCreateNewInstance) )
				return false;

			ZenInitialize(nullptr);
			return true;
		}

		void release()
		{
			ZenLibrary::release();
		}

		void startGame(GameSetting const& setting) override
		{
			mBoardSize = setting.boardSize;

			ZenClearBoard();
			ZenSetBoardSize(setting.boardSize);

			if (setting.numHandicap && setting.bFixedHandicap )
			{
				//ZenFixedHandicap(setting.numHandicap);
				Go::VisitFixedHandicapPositions( mBoardSize, setting.numHandicap,
					[&](int x, int y)
					{
						ZenAddStone(x , y, (int)Color::Black);
					}
				);
			}

			ZenSetKomi(setting.komi);
			ZenSetNextColor(setting.bBlackFirst ? (int)Color::Black : (int)Color::White);
			if( Version == 6 )
			{
				ZenSetDCNN(true);
			}

		}

		int getBoardSize() const override { return mBoardSize; }

		void doInitializeResource() override
		{
			ZenClearBoard();
		}

		void doReleaseResource() override
		{

		}

		void restart() override
		{
			ZenStopThinking();
			ZenClearBoard();
		}

		void think(ThinkResult& result , Color color ) override
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

		bool getBestThinkMove( ThinkInfo infoList[] , int num ) override
		{
			for( int i = 0; i < num ; ++i )
			{
				auto& info = infoList[i];

				int x, y , c;
				char buf[256];
				ZenGetTopMoveInfo(i, x , y , c, info.winRate, buf, ARRAY_SIZE(buf));
				if( x == -4 )
					return false;

				info.v.x = x;
				info.v.y = y;

				char* pData = buf;
				while( true )
				{
					uint8 pos[2];
					int numRead = Go::ReadCoord(pData, pos);
					if  ( numRead == 0 )
						break;
					
					pos[1] = mBoardSize - ( pos[1] + 1 );

					PlayVertex vertex;
					vertex.x = pos[0];
					vertex.y = pos[1];
					info.vSeq.push_back(vertex);

					pData += numRead;
					if( *pData == 0 )
						break;

					++pData;
				}
			}
			return true;
		}

		void  getTerritoryStatictics(TerritoryInfo& info) override
		{
			if( mBoardSize == 0 )
				return;

			ZenGetTerritoryStatictics(info.map);

#if 0
			Color mapColor[19][19];
			for( int j = 0; j < mBoardSize; ++j )
			{
				for( int i = 0; i < mBoardSize; ++i )
				{
					int value = info.map[j][i];
					Color color;
					if( value > TerritoryInfo::FieldValue )
					{
						color = Color::Black;
					}
					else if( value < -TerritoryInfo::FieldValue )
					{
						color = Color::White;
					}
					else
					{
						color = Color::Empty;
					}

					mapColor[j][i] = color;
				}
			}

			for( int j = 0; j < mBoardSize; ++j )
			{
				for( int i = 0; i < mBoardSize; ++i )
				{
					if( ZenGetBoardColor(i, j) == (int)Color::Empty )
					{
						Color color = mapColor[j][i];
						if( ( i > 0 && mapColor[j][i-1] != color )  ||
							( i < mBoardSize - 1 && mapColor[j][i+1] != color) ||
						    ( j > 0 && mapColor[j-1][i] != color) ||
						    ( j < mBoardSize - 1 && mapColor[j+1][i] != color) )
						{
							info.map[j][i] = 0;
						}
					}
				}
			}
#endif

			info.fieldCounts[0] = info.fieldCounts[1] = 0;
			info.totalRemainFields[0] = info.totalRemainFields[1] = 0;
			for( int j = 0; j < mBoardSize; ++j )
			{
				for( int i = 0; i < mBoardSize; ++i )
				{
					int value = info.map[j][i];
					int index;
					if( value > 0 )
					{
						index = 0;
					}
					else
					{
						index = 1;
						value = -value;
					}
					if( value >= TerritoryInfo::FieldValue )
					{
						++info.fieldCounts[index];
					}
					else
					{
						info.totalRemainFields[index] += value;
					}
				}
			}
		}

		void startThink(Color color) override
		{
			if( color == Color::Empty )
				color = (Color)ZenGetNextColor();
			ZenStartThinking((int)color);
		}
		void stopThink() override
		{
			ZenStopThinking();
		}
		bool isThinking() override { return ZenIsThinking(); }
		void getThinkResult(ThinkResult& result) override
		{
			ZenReadGeneratedMove(result.x, result.y, result.bPass, result.bResign);
			result.winRate = ZenGetBestMoveRate();
		}

		bool playStone(int x, int y, Color color) override
		{
			return ZenPlay(x, y, (int)color);
		}
		bool addStone(int x, int y, Color color)
		{
			return ZenAddStone(x, y, (int)color);
		}
		void playPass(Color color) override
		{
			ZenPass((int)color);
		}
		bool undo() override
		{
			return ZenUndo(ZenGetNextColor() == (int)Color::Black ? (int)Color::White : (int)Color::Black);
		}

		void setCoreSetting(CoreSetting const& setting) override
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

		Color getBoardColor(int x, int y) override { return (Color)ZenGetBoardColor(x, y); }
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
		int getBlackPrisonerNum() override { return ZenGetNumBlackPrisoners(); }
		int getWhitePrisonerNum() override { return ZenGetNumWhitePrisoners(); }
		Color getNextColor() override { return Color(ZenGetNextColor()); }

		virtual int getHistorySize() const override
		{
			return ZenGetHistorySize();
		
		}
	protected:
		int mBoardSize;
		TBotCore()
		{
			mBoardSize = 0;
		}

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
	namespace ZenGameParam
	{
		enum
		{
			eBestMoveVertex ,
			eWinRate ,
		};
	}

	namespace ZenMetaParam
	{

		enum 
		{
			eVersion ,
		};
	}
	class ZenBot : public IBot
	{

	public:

		ZenBot();

		template< int Version >
		Zen::IBotCore* buildCoreT()
		{
			using ZenCore = Zen::TBotCore< Version >;

			ZenCore* core = ZenCore::Create();
			if (core)
			{
				if (!core->initialize())
				{
					delete core;
					return nullptr;
				}
			}
			return core;
		}

		static Zen::CoreSetting GetCoreConfigSetting();

		bool initialize(IBotSetting* setting) override;
		void destroy() override;
		bool setupGame(GameSetting const& setting) override;
		bool restart(GameSetting const& setting) override;
		EBotExecResult playStone(int x, int y, int color) override;
		EBotExecResult playPass(int color) override;
		EBotExecResult undo() override;
		bool requestUndo() override;
		bool thinkNextMove(int color) override;
		bool isThinking() override;
		bool isGPUBased() const override { return false; }

		void update(IGameCommandListener& listener) override;
		bool getMetaData(int id, uint8* dataBuffer, int size) override;
		EBotExecResult readBoard(int* outState) override;

		static int FormZColor(Zen::Color color)
		{
			switch (color)
			{
			case Zen::Color::Empty: return EStoneColor::Empty;
			case Zen::Color::White: return EStoneColor::White;
			case Zen::Color::Black: return EStoneColor::Black;
			}

			NEVER_REACH("Error Color Value");
			return EStoneColor::Empty;

		}

		static Zen::Color ToZColor(int color)
		{
			switch( color )
			{
			case EStoneColor::Black: return Zen::Color::Black;
			case EStoneColor::White: return Zen::Color::White;
			case EStoneColor::Empty: return Zen::Color::Empty;
			}

			NEVER_REACH("Error Color Value");
			return Zen::Color::Empty;
		}


		int  mCoreVersion = 0;
		std::unique_ptr< Zen::IBotCore > mCore;
		int  requestColor;
		bool bWaitResult;
		bool bRequestUndoDone;
	};



}


#endif // ZenBot_h__