#pragma once
#ifndef KataBot_H_DB01698C_6FC9_40BA_A32D_C6CA090EBA2C
#define KataBot_H_DB01698C_6FC9_40BA_A32D_C6CA090EBA2C

#include "GTPBotBase.h"

namespace Go
{
	namespace KataGameParam
	{
		enum
		{
			eWinRate,
		};
	}

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
			SIMPLE, //Simple ko rules(triple ko = no result)
			POSITIONAL, //Positional superko
			SITUATIONAL, //Situational superko
			SPIGHT,   //Spight superko - https://senseis.xmp.net/?SpightRules
		};

		KoRule koRule = POSITIONAL;

		enum ScoringRule
		{
			AREA, //Area scoring
			TERRITORY, //Territory scoring(uses a sort of special computer - friendly territory ruleset)
		};
		ScoringRule scoringRule = AREA;

		//Is multiple - stone suicide legal ? (Single - stone suicide is always illegal).
		bool multiStoneSuicideLegal = false;

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
		bool initialize(void* settingData) override;
		bool isGPUBased() const override { return true; }
	};

}//namespace Go

#endif // KataBot_H_DB01698C_6FC9_40BA_A32D_C6CA090EBA2C