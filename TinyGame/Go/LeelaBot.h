#pragma once
#ifndef LeelaBot_H_42280575_AB74_4D08_8444_953C8BD1062B
#define LeelaBot_H_42280575_AB74_4D08_8444_953C8BD1062B

#include "GoBot.h"

#include "PlatformThread.h"
#include "Platform/Windows/WindowsProcess.h"

#include "Template/ArrayView.h"
#define LEELA_NET_DIR_NAME "networks"

namespace Go
{
	int constexpr LeelaGoSize = 19;
	extern TArrayView< char const* const> ELFWeights;

	namespace LeelaGameParam
	{
		enum
		{
			eJobMode,
			eNetWeight,
			eMatchChallengerColor,
			eWinRate,

			eBestMoveVertex,
			eThinkResult ,
		};
	}

	struct GTPCommand
	{
		enum Id
		{
			eNone,
			eKomi,
			eHandicap,
			eRestart ,
			ePlay,
			eAdd ,
			eGenmove,
			ePass,
			eUndo,
			eFinalScore ,
			eQuit,

			eStartPonder ,
			eStopPonder ,
		};

		Id  id;
		int meta;
	};

	class IGameOutputThread : public RunnableThreadT< IGameOutputThread >
	{
	public:
		virtual ~IGameOutputThread() {}
		virtual unsigned run() = 0;
		virtual void update() {}
		virtual void restart()
		{
			bRequestRestart = true;
			mOutputCommands.clear();
		}


		ChildProcess* process = nullptr;
		void addOutputCommand(GameCommand const& com)
		{
			mOutputCommands.push_back(com);
		}
		template< class T >
		void procOutputCommand(T fun)
		{
			bRequestRestart = false;
			for( GameCommand& com : mOutputCommands )
			{
				fun(com);
				if ( bRequestRestart )
					break;
			}
			mOutputCommands.clear();
		}


		bool bRequestRestart = false;
		std::vector< GameCommand > mOutputCommands;
		
	};

	class GTPLikeAppRun
	{
	public:
		ChildProcess        process;
		IGameOutputThread*  outputThread = nullptr;
		bool  bThinking = false;

		~GTPLikeAppRun();

		void update()
		{
			if (outputThread )
				outputThread->update();
		}

		void stop();

		bool restart();
		bool playStone(int x , int y, int color);
		bool addStone(int x, int y, int color);
		bool playPass(int color = StoneColor::eEmpty );
		bool thinkNextMove(int color);
		bool undo();
		bool requestUndo();
		bool setupGame(GameSetting const& setting);
		bool showResult();

		bool inputCommand(char const* command, GTPCommand com);
		bool inputProcessStream(char const* command, int length = 0);

		template< class T >
		bool buildProcessT(char const* appPath, char const* command)
		{
			if( !process.create(appPath, command) )
				return false;

			auto myThread = new T;
			myThread->process = &process;
			myThread->start();
			myThread->setDisplayName("Output Thread");
			outputThread = myThread;
			
			return true;
		}
	};

	struct LeelaAISetting
	{
		char const* weightName = nullptr;
		uint64 seed = 0;
		//Weaken engine by limiting the number of playouts. Requires --noponder.
		int playouts = 6400;//1600;
		//Play more randomly the first x moves.
		int randomcnt = 0;//30;
		//Number of threads to use.
		int numThread = 8;
		//Resign when winrate is less than x%. -1 uses 10% but scales for handicap
		int resignpct = 10;
		//Weaken engine by limiting the number of visits.
		int visits = 0;
		//Don't use heuristics for smarter passing.
		bool bDumbPass = true; 
		//Enable policy network randomization.
		bool bNoise = true;
		//Disable all diagnostic output.
		bool bQuiet = false;
		//Disable thinking on opponent's time.
		bool bNoPonder = true;

		bool bGTPMode = true;

		static LeelaAISetting GetDefalut();

		std::string toParamString() const;

		std::string toString() const;

	};

	struct LeelaThinkInfo
	{
		PlayVertex v;
		int   nodeVisited;
		float winRate;
		float evalValue;
		std::vector< PlayVertex > vSeq;
	};

	typedef std::vector< LeelaThinkInfo >  LeelaThinkInfoVec;

	struct LeelaAppRun : public GTPLikeAppRun
	{
		static char const* InstallDir;

		static std::string GetLastWeightName();
		static std::string GetBestWeightName();

		bool buildLearningGame();
		bool buildPlayGame(LeelaAISetting const& setting);
		bool buildAnalysisGame(bool bUseELF);
		void startPonder(int color);
		void stopPonder();

		std::string mUseWeightName;
		std::string mProcessExrcuteCommand;

	};


	template< class T >
	class TGTPLikeBot : public IBotInterface
	{
	public:
		virtual void destroy()
		{
			mAI.stop();
		}
		virtual bool setupGame(GameSetting const& setting ) override
		{
			return mAI.setupGame(setting);
		}
		virtual bool restart() override
		{
			return mAI.restart();
		}
		virtual bool playStone(int x, int y, int color) override
		{
			return mAI.playStone(x , y, color);
		}
		virtual bool playPass(int color) override
		{
			return mAI.playPass(color);
		}
		virtual bool undo() override
		{
			return mAI.undo();
		}
		virtual bool requestUndo() override
		{
			return mAI.requestUndo();
		}
		virtual bool thinkNextMove(int color) override
		{
			return mAI.thinkNextMove(color);
		}
		virtual bool isThinking() override
		{
			//TODO
			return false;
		}
		virtual void update(IGameCommandListener& listener) override
		{
			mAI.update();
			auto MyFun = [&](GameCommand const& com)
			{
				listener.notifyCommand(com);
			};
			mAI.outputThread->procOutputCommand(MyFun);
		}

		bool inputProcessStream(char const* command)
		{
			int numWrite = 0;
			return mAI.process.writeInputStream(command, strlen(command), numWrite);
		}
		T mAI;
	};

	class LeelaBot : public TGTPLikeBot< LeelaAppRun >
	{
	public:
		enum MetaId
		{
			eWeightName ,
		};
	
		virtual bool initialize(void* settingData) override;
		virtual bool isGPUBased() const override { return true; }
		virtual bool getMetaData(int id, uint8* dataBuffer, int size);

		virtual bool playPass(int color) override
		{
			return mAI.playPass(color);
		}

	};

	struct AQAppRun : public GTPLikeAppRun
	{
	public:
		static char const* InstallDir;

		bool buildPlayGame();

	};


	class AQBot : public TGTPLikeBot< AQAppRun >
	{
	public:
		virtual bool initialize(void* settingData) override;
		virtual bool isGPUBased() const override { return true; }
	};

	struct KataAISetting
	{
		char const* modelName = nullptr;

		// Search limits---------------------------------------------------------------------------------- -
		// If provided, limit maximum number of root visits per search to this much. (With tree reuse, visits do count earlier search)
		int maxVisits = 1601; //1000
		// If provided, limit maximum number of new playouts per search to this much. (With tree reuse, playouts do not count earlier search)
		int maxPlayouts = 0; //1000
		// If provided, cap search time at this many seconds(search will still try to follow GTP time controls)
		double maxTime = 0;//60
		// Ponder on the opponent's turn?
		bool ponderingEnabled = false;
		// Same limits but for ponder searches if pondering is enabled
		int maxVisitsPondering = 0;//1000
		int maxPlayoutsPondering = 0; //1000
		double maxTimePondering = 0; //60
		// Number of seconds to buffer for lag for GTP time controls
		double lagBuffer = 1.0;
		// Number of threads to use in search
		int numSearchThreads = 4;
		// Play a little faster if the opponent is passing, for friendliness
		double searchFactorAfterOnePass = 0.50;
		double searchFactorAfterTwoPass = 0.25;
		// Play a little faster if super - winning, for friendliess
		double searchFactorWhenWinning = 0.40;
		double searchFactorWhenWinningThreshold = 0.95;

		enum KoRule
		{
			SIMPLE , //Simple ko rules(triple ko = no result)
			POSITIONAL , //Positional superko
			SITUATIONAL , //Situational superko
			SPIGHT ,   //Spight superko - https://senseis.xmp.net/?SpightRules
		};

		KoRule koRule = POSITIONAL;

		enum ScoringRule
		{
			AREA , //Area scoring
			TERRITORY , //Territory scoring(uses a sort of special computer - friendly territory ruleset)
		};
		ScoringRule scoringRule = AREA;

		//Is multiple - stone suicide legal ? (Single - stone suicide is always illegal).
		bool multiStoneSuicideLegal = true;

		// Make the bot capture stones that are part of pass - alive territory
		// This is necessary to get correct play under tromp - taylor rules since the bot otherwise assumes(and is trained under)
		// a ruleset where those stones need not be captured.It obviously should NOT be enabled if playing under territory scoring.
		bool cleanupBeforePass = true;

		// Uncomment this to make it so that if the game seems to be a handicap game, assume that white gets + 1 point per
		// black handicap stone.Some Go servers like OGS will silently give white such points without including it in the komi.
		int whiteBonusPerHandicapStone = 0; // 1

		// Resignation occurs if for at least resignConsecTurns in a row,
		// the winLossUtility(which is on a[-1, 1] scale) is below resignThreshold.
		bool allowResignation = true;
		double resignThreshold = -0.90;
		int  resignConsecTurns = 3;

		// Search randomization------------------------------------------------------------------------------
		// Note that multithreading can also introduce a significant amount of nondeterminism.
		// If provided, force usage of a specific seed for various things in the search instead of randomizing
		std::string searchRandSeed;
		// Temperature for the early game, randomize between chosen moves with this temperature
		double chosenMoveTemperatureEarly = 0.5;
		// Decay temperature for the early game by 0.5 every this many moves, scaled with board size.
		int chosenMoveTemperatureHalflife = 19;
		// At the end of search after the early game, randomize between chosen moves with this temperature
		double chosenMoveTemperature = 0.10;
		// Subtract this many visits from each move prior to applying chosenMoveTemperature (unless all moves have too few visits) to downweight unlikely moves
		int chosenMoveSubtract = 0;
		// The same as chosenMoveSubtract but only prunes moves that fall below the threshold, does not affect moves above
		int chosenMovePrune = 1;
		// Use dirichlet noise for the root node policy ?
		bool rootNoiseEnabled = false;
		// Dirichlet noise alpha is set to this divided by number of legal moves. 10.83 produces an alpha of 0.03 on an empty 19x19 board.
		double rootDirichletNoiseTotalConcentration = 10.83;
		// Proportion of root policy that is noise
		double rootDirichletNoiseWeight = 0.25;
		// Using LCB for move selection ?
		bool useLcbForSelection = true;
		// How many stdevs a move needs to be better than another for LCB selection
		double lcbStdevs = 5.0;
			// Only use LCB override when a move has this proportion of visits as the top move
		double minVisitPropForLCB = 0.15;

		// Internal params------------------------------------------------------------------------------
		// Scales the utility of winning / losing
		double winLossUtilityFactor = 1.0;
		// Scales the utility for trying to maximize score
		double staticScoreUtilityFactor = 0.20;
		double dynamicScoreUtilityFactor = 0.20;
		// Adjust dynamic score center this proportion of the way towards zero, capped at a reasonable amount.
		double dynamicScoreCenterZeroWeight = 0.20;
		// The utility of getting a "no result" due to triple ko or other long cycle in non - superko rulesets(-1 to 1)
		double noResultUtilityForWhite = 0.0;
		// The number of wins that a draw counts as, for white. (0 to 1)
		double drawEquivalentWinsForWhite = 0.5;
		// Exploration constant for mcts
		double cpuctExploration = 1.1;
		// FPU reduction constant for mcts
		double fpuReductionMax = 0.2;
		// Use parent average value for fpu base point instead of point value net estimate
		bool fpuUseParentAverage = true;
		// Amount to apply a downweighting of children with very bad values relative to good ones
		double valueWeightExponent = 0.5;
		// Slight incentive for the bot to behave human - like with regard to passing at the end, filling the dame,
		// not wasting time playing in its own territory, etc, and not play moves that are equivalent in terms of
		// points but a bit more unfriendly to humans.
		double rootEndingBonusPoints = 0.5;
		// Make the bot prune useless moves that are just prolonging the game to avoid losing yet
		bool rootPruneUselessMoves = true;
		// How big to make the mutex pool for search synchronization
		int mutexPoolSize = 8192;
		// How many virtual losses to add when a thread descends through a node
		int numVirtualLossesPerThread = 1;

		KataAISetting()
		{

		}

		bool writeToConifg(char const* path) const;
	};

	class KataAppRun : public GTPLikeAppRun
	{
	public:
		static char const* InstallDir;

		bool bShowInfo = true;
		bool thinkNextMove(int color);

		static std::string GetLastModeltName();

		bool buildPlayGame(KataAISetting const& setting);

	};

	class KataBot : public TGTPLikeBot< KataAppRun >
	{
	public:
		virtual bool initialize(void* settingData) override;
		virtual bool isGPUBased() const override { return true; }
	};

}//namespace Go

#endif // LeelaBot_H_42280575_AB74_4D08_8444_953C8BD1062B
