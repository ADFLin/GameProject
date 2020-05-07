#include "KataBot.h"
#include "GTPOutput.h"

#include "FileSystem.h"
#include "Core/StringConv.h"

namespace Go
{
	char const* KataAppRun::InstallDir = nullptr;

	class KataOutputThread : public GTPOutputThread
	{
	public:

		void procDumpCommandMsg(GTPCommand com, char* buffer, int num) override 
		{
			if ( com.id == GTPCommand::eGenmove )
			{
				if (*buffer == ':')
				{
					float temp;
					float winRate;
					if (sscanf(buffer, ": T %fc W %fc", &temp, &winRate) == 2)
					{			
						//-100~100 => 0 100
						winRate = ( 0.5f * winRate + 50.f );
						GameCommand paramCom;
						paramCom.setParam(KataGameParam::eWinRate, winRate);
						addOutputCommand(paramCom);
					}
				}
			}

		}

	};

	bool KataAppRun::thinkNextMove(int color)
	{
		if (bShowInfo)
		{
			FixString<128> com;
			com.format("genmove_debug %c\n",ToColorChar(color));
			return inputCommand(com, { GTPCommand::eGenmove , color });
		}
		else
		{
			return GTPLikeAppRun::thinkNextMove(color);
		}
	}

	std::string KataAppRun::GetLastModeltName()
	{
		return "g104.txt.gz";
	}

	bool KataAppRun::buildPlayGame(KataAISetting const& setting)
	{
		char const* configName = "gtp_example.cfg";
		if (!setting.bUseDefaultConfig)
		{
			FixString<256> configPath;
			configPath.format("%s/%s", InstallDir, configName);
			if (!setting.writeToConifg(configPath))
				return false;

			configName = "temp.cfg";
		}

		std::string modelName = setting.modelName ? setting.modelName : GetLastModeltName().c_str();

		FixString<256> path;
		path.format("%s/%s", InstallDir, "KataGo.exe");
		FixString<512> command;
		command.format(" gtp -model %s/%s -config %s", KATA_MODEL_DIR_NAME, modelName.c_str(), configName);
		return buildProcessT< KataOutputThread >(path, command);
	}

	char const* FixedConfigText = R"CODE_STRING_(
# Example config for C++ (non-python) gtp bot

# RUNNING ON AN ONLINE SERVER OR IN A REAL TOURNAMENT OR MATCH:
# If you plan to do so, you may want to read through the "Rules" section
# below carefully for proper handling of komi and handicap games and end-of-game cleanup
# and various other details.

# NOTES ABOUT PERFORMANCE AND MEMORY USAGE:
# You will likely want to tune one or more the following:
#
# numSearchThreads:
# The number of CPU threads to use. If your GPU is powerful, it can actually be much higher than
# the number of cores on your processor because you will need many threads to feed large enough
# batches to make good use of the GPU.
#
# The "./katago benchmark" command can help you tune this parameter, as well as to test out the effect
# of changes to any of the other parameters below!
#
# nnCacheSizePowerOfTwo:
# This controls the NN Cache size, which is the primary RAM/memory use.
# Increase this if you don't mind the memory use and want better performance for searches with
# tens of thousands of visits or more. Decrease this if you want to limit memory usage.
#
# If you're someone who is happy to do a bit of math - each neural net entry takes very
# approximately 1.5KB, except when using whole-board ownership/territory visualizations, each
# entry will take very approximately 3KB. The number of entries is (2 ** nnCacheSizePowerOfTwo),
# for example 2 ** 18 = 262144.
#
# OTHER NOTES:
# If you have more than one GPU, take a look at "OpenCL GPU settings" or "CUDA GPU settings" below.
#
# If using OpenCL, you will want to verify that KataGo is picking up the correct device!
# (e.g. some systems may have both an Intel CPU OpenCL and GPU OpenCL, if KataGo appears to pick
# the wrong one, you correct this by specifying "openclGpuToUse" below).
#
# You may also want to adjust "maxVisits", "ponderingEnabled", "resignThreshold", and possibly
# other parameters depending on your intended usage.


# Logs------------------------------------------------------------------------------------

# Where to output log?
logFile = gtp.log
# Logging options
logAllGTPCommunication = true
logSearchInfo = true
logToStderr = false

# KataGo will display some info to stderr on GTP startup
# Uncomment this to suppress that and remain silent
# startupPrintMessageToStderr = false

# Chat some stuff to stderr, for use in things like malkovich chat to OGS.
# ogsChatToStderr = true

# Configure the maximum length of analysis printed out by lz-analyze and other places.
# Controls the number of moves after the first move in a variation.
# analysisPVLen = 9

# Report winrates for chat and analysis as (BLACK|WHITE|SIDETOMOVE).
# Default is SIDETOMOVE, which is what tools that use LZ probably also expect
# reportAnalysisWinratesAs = SIDETOMOVE

# Default rules------------------------------------------------------------------------------------
# See https://lightvector.github.io/KataGo/rules.html for a description of the rules.
# These rules are defaults and can be changed mid-run by several custom GTP commands.
# See https://github.com/lightvector/KataGo/blob/master/docs/GTP_Extensions.md for those commands.

# Some other legal values are: "chinese", "japanese", "korean", "aga", "chinese-ogs", "new-zealand".
# KataGo does not claim to exactly match any particular human ruleset, but KataGo will try to behave
# as closely as possible given the rules it has implemented.
rules = tromp-taylor

# Use the below instead to specify an arbitrary combination of individual rules.

# koRule = SIMPLE       # Simple ko rules (triple ko = no result)
# koRule = POSITIONAL     # Positional superko
# koRule = SITUATIONAL  # Situational superko

# scoringRule = AREA         # Area scoring
# scoringRule = TERRITORY  # Territory scoring (uses a sort of special computer-friendly territory ruleset)

# taxRule = NONE    # All surrounded empty points are scored
# taxRule = SEKI  # Eyes in seki do NOT count as points
# taxRule = ALL   # All groups are taxed up to 2 points for the two eyes needed to live

# multiStoneSuicideLegal = true  #Is multiple-stone suicide legal? (Single-stone suicide is always illegal).

# hasButton = false # Set to true when area scoring to award 0.5 points to the first pass.

# whiteHandicapBonus = 0      # In handicap games, give white no compensation for black's handicap stones (Tromp-taylor, NZ, JP)
# whiteHandicapBonus = N-1  # In handicap games, give white N-1 points for black's handicap stones (AGA)
# whiteHandicapBonus = N    # In handicap games, give white N points for black's handicap stones (Chinese)

# Bot behavior---------------------------------------------------------------------------------------

# Resignation -------------

# Resignation occurs if for at least resignConsecTurns in a row,
# the winLossUtility (which is on a [-1,1] scale) is below resignThreshold.
allowResignation = true
resignThreshold = -0.98
resignConsecTurns = 3

# Handicap -------------

# Assume that if black makes many moves in a row right at the start of the game, then the game is a handicap game.
# This is necessary on some servers and for some GUIs and also when initializing from many SGF files, which may
# set up a handicap games using repeated GTP "play" commands for black rather than GTP "place_free_handicap" commands.
# However, it may also lead to incorrect understanding of komi if whiteHandicapBonus is used and a server does NOT
# have such a practice.
# Defaults to true! Uncomment and set to false to disable this behavior.
# assumeMultipleStartingBlackMovesAreHandicap = true

# Makes katago dynamically adjust to play more aggressively in handicap games based on the handicap and the current state of the game.
# Comment to disable this and make KataGo play the same always.
dynamicPlayoutDoublingAdvantageCapPerOppLead = 0.04
# Instead of setting dynamicPlayoutDoublingAdvantageCapPerOppLead, you can uncomment these and set this to a value from -3.0 to 3.0
# to set KataGo's aggression to a FIXED level.
# Negative makes KataGo behave as if it is much weaker than the opponent, preferring to play defensively
# Positive makes KataGo behave as if it is much stronger than the opponent, prefering to play aggressively or even overplay slightly.
# playoutDoublingAdvantage = 0.0

# Controls which side dynamicPlayoutDoublingAdvantageCapPerOppLead or playoutDoublingAdvantage applies to.
playoutDoublingAdvantagePla = WHITE

# Passing and cleanup -------------

# Make the bot never assume that its pass will end the game, even if passing would end and "win" under Tromp-Taylor rules.
# Usually this is a good idea when using it for analysis or playing on servers where scoring may be implemented non-tromp-taylorly.
# Defaults to true! Uncomment and set to false to disable this.
# conservativePass = true

# When using territory scoring, self-play games continue beyond two passes with special cleanup
# rules that may be confusing for human players. This option prevents the special cleanup phases from being
# reachable when using the bot for GTP play.
# Defaults to true! Uncomment and set to false if you want KataGo to be able to enter special cleanup.
# For example, if you are testing it against itself, or against another bot that has precisely implemented the rules
# documented at https://lightvector.github.io/KataGo/rules.html
# preventCleanupPhase = true

# GPU Settings-------------------------------------------------------------------------------

# Maximum number of positions to send to a single GPU at once.
# The default value here is roughly equal to numSearchThreads, but you can specify it manually
# if you are running out of memory, or if you are using multiple GPUs that expect to split
# up the work.
# nnMaxBatchSize = <integer>

# Cache up to (2 ** this) many neural net evaluations in case of transpositions in the tree.
# Uncomment and edit to change if you want to adjust a major component of KataGo's RAM usage.
# nnCacheSizePowerOfTwo = 20

# Size of mutex pool for nnCache is (2 ** this).
# nnMutexPoolSizePowerOfTwo = 16

# Randomize board orientation when running neural net evals? Uncomment and set to false to disable.
# nnRandomize = true
# If provided, force usage of a specific seed for nnRandomize instead of randomizing.
# nnRandSeed = abcdefg

# TO USE MULTIPLE GPUS:
# Set this to the number of GPUs you have and/or would like to use.
# **AND** if it is more than 1, uncomment the appropriate CUDA or OpenCL section below.
# numNNServerThreadsPerModel = 1

# CUDA GPU settings--------------------------------------
# These only apply when using the CUDA version of KataGo.

# IF USING ONE GPU: optionally uncomment and change this if the GPU you want to use turns out to be not device 0
# cudaDeviceToUse = 0

# IF USING TWO GPUS: Uncomment these two lines (AND set numNNServerThreadsPerModel above):
# cudaDeviceToUseThread0 = 0  # change this if the first GPU you want to use turns out to be not device 0
# cudaDeviceToUseThread1 = 1  # change this if the second GPU you want to use turns out to be not device 1

# IF USING THREE GPUS: Uncomment these three lines (AND set numNNServerThreadsPerModel above):
# cudaDeviceToUseThread0 = 0  # change this if the first GPU you want to use turns out to be not device 0
# cudaDeviceToUseThread1 = 1  # change this if the second GPU you want to use turns out to be not device 1
# cudaDeviceToUseThread2 = 2  # change this if the third GPU you want to use turns out to be not device 2

# You can probably guess the pattern if you have four, five, etc. GPUs.

# KataGo will automatically use FP16 or not based on the compute capability of your NVIDIA GPU. If you
# want to try to force a particular behavior though you can uncomment these lines and change them
# to "true" or "false". E.g. it's using FP16 but on your card that's giving an error, or it's not using
# FP16 but you think it should.
# cudaUseFP16 = auto
# cudaUseNHWC = auto

# OpenCL GPU settings--------------------------------------
# These only apply when using the OpenCL version of KataGo.

# Uncomment to tune OpenCL for every board size separately, rather than only the largest possible size
# openclReTunePerBoardSize = true

# IF USING ONE GPU: optionally uncomment and change this if the best device to use is guessed incorrectly.
# The default behavior tries to guess the 'best' GPU or device on your system to use, usually it will be a good guess.
# openclDeviceToUse = 0

# IF USING TWO GPUS: Uncomment these two lines and replace X and Y with the device ids of the devices you want to use.
# It might NOT be 0 and 1, some computers will have many OpenCL devices. You can see what the devices are when
# KataGo starts up - it should print or log all the devices it finds.
# (AND also set numNNServerThreadsPerModel above)
# openclDeviceToUseThread0 = X
# openclDeviceToUseThread1 = Y

# IF USING THREE GPUS: Uncomment these three lines and replace X and Y and Z with the device ids of the devices you want to use.
# It might NOT be 0 and 1 and 2, some computers will have many OpenCL devices. You can see what the devices are when
# KataGo starts up - it should print or log all the devices it finds.
# (AND also set numNNServerThreadsPerModel above)
# openclDeviceToUseThread0 = X
# openclDeviceToUseThread1 = Y
# openclDeviceToUseThread2 = Z

# You can probably guess the pattern if you have four, five, etc. GPUs.


# Root move selection and biases------------------------------------------------------------------------------
# Uncomment and edit any of the below values to change them from their default.

# If provided, force usage of a specific seed for various things in the search instead of randomizing
# searchRandSeed = hijklmn

# Temperature for the early game, randomize between chosen moves with this temperature
# chosenMoveTemperatureEarly = 0.5
# Decay temperature for the early game by 0.5 every this many moves, scaled with board size.
# chosenMoveTemperatureHalflife = 19
# At the end of search after the early game, randomize between chosen moves with this temperature
# chosenMoveTemperature = 0.10
# Subtract this many visits from each move prior to applying chosenMoveTemperature
# (unless all moves have too few visits) to downweight unlikely moves
# chosenMoveSubtract = 0
# The same as chosenMoveSubtract but only prunes moves that fall below the threshold, does not affect moves above
# chosenMovePrune = 1

# Number of symmetries to sample (WITH replacement) and average at the root
# rootNumSymmetriesToSample = 1

# Using LCB for move selection?
# useLcbForSelection = true
# How many stdevs a move needs to be better than another for LCB selection
# lcbStdevs = 5.0
# Only use LCB override when a move has this proportion of visits as the top move
# minVisitPropForLCB = 0.15

# Internal params------------------------------------------------------------------------------
# Uncomment and edit any of the below values to change them from their default.

# Scales the utility of winning/losing
# winLossUtilityFactor = 1.0
# Scales the utility for trying to maximize score
# staticScoreUtilityFactor = 0.10
# dynamicScoreUtilityFactor = 0.30
# Adjust dynamic score center this proportion of the way towards zero, capped at a reasonable amount.
# dynamicScoreCenterZeroWeight = 0.20
# dynamicScoreCenterScale = 0.75
# The utility of getting a "no result" due to triple ko or other long cycle in non-superko rulesets (-1 to 1)
# noResultUtilityForWhite = 0.0
# The number of wins that a draw counts as, for white. (0 to 1)
# drawEquivalentWinsForWhite = 0.5

# Exploration constant for mcts
# cpuctExploration = 0.9
# cpuctExplorationLog = 0.4
# FPU reduction constant for mcts
# fpuReductionMax = 0.2
# rootFpuReductionMax = 0.1
# Use parent average value for fpu base point instead of point value net estimate
# fpuUseParentAverage = true
# Amount to apply a downweighting of children with very bad values relative to good ones
# valueWeightExponent = 0.5
# Slight incentive for the bot to behave human-like with regard to passing at the end, filling the dame,
# not wasting time playing in its own territory, etc, and not play moves that are equivalent in terms of
# points but a bit more unfriendly to humans.
# rootEndingBonusPoints = 0.5
# Make the bot prune useless moves that are just prolonging the game to avoid losing yet
# rootPruneUselessMoves = true

# How big to make the mutex pool for search synchronization
# mutexPoolSize = 16384
# How many virtual losses to add when a thread descends through a node
# numVirtualLossesPerThread = 1

)CODE_STRING_";

	struct FConfigHelper
	{
		template< class T >
		static void Write(std::string& text, char const* paramName, T const& value)
		{
			text += paramName;
			text += " = ";
			text += FStringConv::From(value);
			text += "\n";
		}

		static void Write(std::string& text, char const* paramName, std::string const& value)
		{
			text += paramName;
			text += " = ";
			text += value;
			text += "\n";
		}

		static void Write(std::string& text, char const* paramName, bool value)
		{
			text += paramName;
			text += " = ";
			text += value ? "true" : "false";
			text += "\n";
		}
#if 0
#define ENUM_CASE_OP( V ) case KataAISetting::V : text += #V; break;
		static void Write(std::string& text, char const* paramName, KataAISetting::KoRule value)
		{
			text += paramName;
			text += " = ";
#define KO_RULE_LIST(op) op(SIMPLE) op(POSITIONAL) op(SITUATIONAL) op(SPIGHT)
			switch (value) { KO_RULE_LIST(ENUM_CASE_OP) }
#undef KO_RULE_LIST
			text += "\n";

		}

		static void Write(std::string& text, char const* paramName, KataAISetting::ScoringRule value)
		{
			text += paramName;
			text += " = ";
#define SCORING_RULE_LIST(op) op(AREA) op(TERRITORY)
			switch (value) { SCORING_RULE_LIST(ENUM_CASE_OP) }
#undef KO_RULE_LIST
			text += "\n";
		}
#undef ENUM_CASE_OP
#endif
	};


	bool KataAISetting::writeToConifg(char const* path) const
	{
		std::string configText;

#define CONFIG_VALUE(VAR) FConfigHelper::Write(configText, #VAR , VAR );

		if (maxVisits) CONFIG_VALUE(maxVisits);
		if (maxPlayouts) CONFIG_VALUE(maxPlayouts);
		if (maxTime != 0.0) CONFIG_VALUE(maxTime);
		CONFIG_VALUE(ponderingEnabled);

		if (maxVisitsPondering) CONFIG_VALUE(maxVisitsPondering);
		if (maxPlayoutsPondering) CONFIG_VALUE(maxPlayoutsPondering);
		if (maxTimePondering != 0.0) CONFIG_VALUE(maxTimePondering);

		CONFIG_VALUE(lagBuffer);
		CONFIG_VALUE(numSearchThreads);
		CONFIG_VALUE(searchFactorAfterOnePass);
		CONFIG_VALUE(searchFactorAfterTwoPass);
		CONFIG_VALUE(searchFactorWhenWinning);
		CONFIG_VALUE(searchFactorWhenWinningThreshold);

#undef CONFIG_VALUE

		configText += FixedConfigText;

		return FileUtility::SaveFromBuffer(path, configText.c_str(), configText.length());
	}

	bool KataBot::initialize(void* settingData)
	{
		if (!mAI.buildPlayGame(*static_cast<KataAISetting*>(settingData)))
			return false;

		//static_cast<GTPOutputThread*>(mAI.outputThread)->bLogMsg = false;
		return true;
	}

}//namespace Go

