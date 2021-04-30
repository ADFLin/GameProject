#include "KataBot.h"
#include "GTPOutput.h"

#include "FileSystem.h"
#include "Core/StringConv.h"
#include "StringParse.h"

namespace Go
{
	char const* KataAppRun::InstallDir = nullptr;

	class KataOutputThread : public GTPOutputThread
	{
	public:
		KataOutputThread():GTPOutputThread(204800){}

		void procDumpCommandMsg(GTPCommand com, char* buffer, int num) override 
		{
			if ( com.id == GTPCommand::eGenmove )
			{
				if ( mAppRun->mGenMoveMethod == KataAppRun::Debug )
				{
					if (*buffer == ':')
					{
						float temp;
						float winRate;
						if (sscanf(buffer, ": T %fc W %fc", &temp, &winRate) == 2)
						{
							//-100~100 => 0 100
							winRate = (0.5f * winRate + 50.f);
							GameCommand paramCom;
							paramCom.setParam(KataGameParam::eWinRate, winRate);
							addOutputCommand(paramCom);
						}
					}
				}
				else if (mAppRun->mGenMoveMethod == KataAppRun::Analyze)
				{
					if (StartWith(buffer, "play"))
					{
						GameCommand gameCom;
						char const* coord = buffer + StrLen("play") + 1;
						if (parsePlayResult(coord, com.meta, gameCom))
						{
							if (onCommandResult && com.id != GTPCommand::eNone)
							{
								onCommandResult(com, EGTPComExecuteResult::Success);
							}

							addOutputCommand(gameCom);
							onOutputCommand(com, gameCom);
							popHeadComandMsg();

							int numRead;
							PlayVertex v = ReadVertex(coord, numRead);
							if (numRead)
							{
								if (!thinkResults[1 - indexResultWrite].empty())
								{
									for (auto const& info : thinkResults[1 - indexResultWrite])
									{
										if (info.v == v)
										{
											GameCommand paramCom;
											paramCom.setParam(KataGameParam::eWinRate, info.winRate);
											addOutputCommand(paramCom);
											break;
										}

									}
								}

							}
			
						}
						else
						{


						}

					}
#define INFO_MOVE_STR "info move"
					else if (StartWith(buffer, INFO_MOVE_STR))
					{
						mbShowParseLine = false;
						thinkResults[indexResultWrite].clear();

						char* cur = buffer;
						InlineString<128>  coord;
						int   nodeVisited;
						float utility;
						float winRate;
						float scoreMean;
						float scoreStdev;
						float scoreLead;
						float scoreSelfplay;
						float prior;
						float LCB;
						float utilityLCB;
						int   order;

						int   numRead;

						while (sscanf(cur, "info move %s visits %d utility %f winrate %f scoreMean %f scoreStdev %f "
							"scoreLead %f scoreSelfplay %f prior %f lcb %f utilityLcb %f order %d pv%n",
							coord.data(), &nodeVisited, &utility, &winRate, &scoreMean, &scoreStdev,
							&scoreLead, &scoreSelfplay, &prior, &LCB, &utilityLCB, &order, &numRead) == 12)
						{
							PlayVertex vertex = GetVertex(coord);
							if (vertex == PlayVertex::Undefiend())
							{
								//LogWarning(0, "Error Think Str = %s", buffer);
								//return;
							}
							KataThinkInfo info;
							info.v = vertex;
							info.nodeVisited = nodeVisited;
							info.utility = utility;
							info.winRate = winRate * 100;
							info.scoreMean = scoreMean;
							info.scoreStdev = scoreStdev;
							info.scoreLead = scoreLead;
							info.scoreSelfplay = scoreSelfplay;
							info.prior = prior;
							info.LCB = LCB;
							info.utilityLCB = utilityLCB;
							info.order = order;

							cur = const_cast<char*>(FStringParse::SkipSpace(cur + numRead));

							while (*cur != 0 || !StartWith(cur, INFO_MOVE_STR))
							{
								PlayVertex v = ReadVertex(cur, numRead);
								if (numRead == 0)
									break;

								info.vSeq.push_back(v);
								cur = const_cast<char*>(FStringParse::SkipSpace(cur + numRead));
							}
							thinkResults[indexResultWrite].push_back(info);
						}

						GameCommand paramCom;
						paramCom.setParam(KataGameParam::eThinkResult, &thinkResults[indexResultWrite]);
						addOutputCommand(paramCom);
						indexResultWrite = 1 - indexResultWrite;
					}
				}
			}
		}

		std::vector< KataThinkInfo > thinkResults[2];
		int  indexResultWrite = 0;
		KataAppRun* mAppRun;
	};

	bool KataAppRun::thinkNextMove(int color)
	{
		switch (mGenMoveMethod)
		{
		case KataAppRun::Normal:
			return GTPLikeAppRun::thinkNextMove(color);
		case KataAppRun::Debug:
			{
				InlineString<128> com;
				com.format("genmove_debug %c\n", ToColorChar(color));
				return inputCommand(com, { GTPCommand::eGenmove , color });
			}
			break;
		case KataAppRun::Analyze:
			{
				InlineString<128> com;
				com.format("kata-genmove_analyze %c 20\n", ToColorChar(color));
				return inputCommand(com, { GTPCommand::eGenmove , color });
			}
			break;
		default:
			break;
		}
		return false;
	}

	std::string KataAppRun::GetLastModeltName()
	{
		return "g170-b40c256x2-s5095420928-d1229425124.bin.gz";
	}

	class KataOutputThread* KataAppRun::getOutputThred()
	{
		return static_cast<KataOutputThread*>(outputThread);
	}

	bool KataAppRun::buildPlayGame(KataAISetting const& setting)
	{
		char const* configName = "default_gtp.cfg";
		if (!setting.bUseDefaultConfig)
		{
			configName = "temp.cfg";
			InlineString<256> configPath;
			configPath.format("%s/%s", InstallDir, configName);
			if (!setting.writeToConifg(configPath))
				return false;
		}

		std::string modelName = setting.modelName ? setting.modelName : GetLastModeltName().c_str();

		InlineString<256> path;
		path.format("%s/%s", InstallDir, setting.bUseCuda ? "KataGoCuda.exe" : "KataGo.exe");
		InlineString<512> command;
		command.format(" gtp -model %s/%s -config %s", KATA_MODEL_DIR_NAME, modelName.c_str(), configName);
		bool result = buildProcessT< KataOutputThread >(path, command);
		if (result)
		{
			getOutputThred()->mAppRun = this;
		}
		return result;
	}

	char const* FixedConfigText = R"CODE_STRING_(
# Config for C++ (non-python) gtp bot

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
#
# ----------------------------------------------------------------------------------------

# For the `katago gtp` command, ALL of THE BELOW VALUES MAY BE SET OR OVERRIDDEN if desired via
# the command line arguments:
# -override-config KEY=VALUE,KEY=VALUE,...

# Logs and files--------------------------------------------------------------------------

# Where to output log?
logDir = gtp_logs    # Each run of KataGo will log to a separate file in this dir
# logFile = gtp.log  # Use this instead of logDir to just specify a single file directly

# Logging options
logAllGTPCommunication = true
logSearchInfo = true
logToStderr = false

# KataGo will display some info to stderr on GTP startup
# Uncomment this to suppress that and remain silent
# startupPrintMessageToStderr = false

# Chat some stuff to stderr, for use in things like malkovich chat to OGS.
# ogsChatToStderr = true

# Optionally override where KataGo will attempt to save things like openCLTuner files and other cached data.
# homeDataDir = DIRECTORY

# Analysis------------------------------------------------------------------------------------

# Configure the maximum length of analysis printed out by lz-analyze and other places.
# Controls the number of moves after the first move in a variation.
# analysisPVLen = 15

# Report winrates for chat and analysis as (BLACK|WHITE|SIDETOMOVE).
# Default is SIDETOMOVE, which is what tools that use LZ probably also expect
# reportAnalysisWinratesAs = SIDETOMOVE

# Uncomment and and set to a positive value to make KataGo explore the top move(s) less deeply and accurately,
# but explore and give evaluations to a greater variety of moves, for analysis (does NOT affect play).
# A value like 0.03 or 0.06 will give various mildly but still noticeably wider searches.
# An extreme value like 1 will distribute many playouts across every move on the board, even very bad moves.
# analysisWideRootNoise = 0.0

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


		template< class T >
		static void Write(std::string& text, char const* paramName, KataAISetting::TVar<T> const& value)
		{
			if (!value.isSet())
				return;

			Write(text, paramName, static_cast<T const&>(value));
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

		static void Write(std::string& text, char const* paramName, KataAISetting::ERules value)
		{
			text += paramName;
			text += " = ";
			switch (value)
			{
			case KataAISetting::TrompTaylor:
				text += "tromp-taylor";
				break;
			case KataAISetting::Chinese:
				text += "chinese";
				break;
			case KataAISetting::Japanse:
				text += "japanese";
				break;
			case KataAISetting::Korean:
				text += "korean";
				break;
			case KataAISetting::Aga:
				text += "aga";
				break;
			case KataAISetting::ChineseOgs:
				text += "chinese-ogs";
				break;
			case KataAISetting::NewZealand:
				text += "new-zealand";
				break;
			default:
				text += "tromp-taylor";
				break;
			}
			text += "\n";
		}

#define ENUM_CASE_OP( V ) case KataAISetting::V : text += #V; break;
		static void Write(std::string& text, char const* paramName, KataAISetting::EKoRule value)
		{
			text += paramName;
			text += " = ";
#define KO_RULE_LIST(op) op(SIMPLE) op(POSITIONAL) op(SITUATIONAL)
			switch (value) { KO_RULE_LIST(ENUM_CASE_OP) }
#undef KO_RULE_LIST
			text += "\n";

		}
		static void Write(std::string& text, char const* paramName, KataAISetting::EScoringRule value)
		{
			text += paramName;
			text += " = ";
#define SCORING_RULE_LIST(op) op(AREA) op(TERRITORY)
			switch (value) { SCORING_RULE_LIST(ENUM_CASE_OP) }
#undef SCORING_RULE_LIST
			text += "\n";
		}

		static void Write(std::string& text, char const* paramName, KataAISetting::ETexRule value)
		{
			text += paramName;
			text += " = ";
#define TEX_RULE_LIST(op) op(NONE) op(SEKI) op(ALL)
			switch (value) { TEX_RULE_LIST(ENUM_CASE_OP) }
#undef TEX_RULE_LIST
			text += "\n";
		}
#undef ENUM_CASE_OP
	};


	bool KataAISetting::writeToConifg(char const* path) const
	{
		std::string configText;

#define CONFIG_VALUE(VAR) FConfigHelper::Write(configText, #VAR , VAR );

		CONFIG_VALUE(rules);
		CONFIG_VALUE(koRule);
		CONFIG_VALUE(scoringRule);
		CONFIG_VALUE(texRule);
		CONFIG_VALUE(multiStoneSuicideLegal);
		CONFIG_VALUE(allowResignation);
		CONFIG_VALUE(resignThreshold);
		CONFIG_VALUE(resignConsecTurns);
		CONFIG_VALUE(resignMinScoreDifference);
		CONFIG_VALUE(assumeMultipleStartingBlackMovesAreHandicap);
		CONFIG_VALUE(dynamicPlayoutDoublingAdvantageCapPerOppLead);
		CONFIG_VALUE(playoutDoublingAdvantage);
		CONFIG_VALUE(playoutDoublingAdvantagePla);
		CONFIG_VALUE(conservativePass);
		CONFIG_VALUE(preventCleanupPhase);
		CONFIG_VALUE(avoidMYTDaggerHack);
		CONFIG_VALUE(maxVisits);
		CONFIG_VALUE(maxPlayouts);
		CONFIG_VALUE(maxTime);
		CONFIG_VALUE(ponderingEnabled);
		CONFIG_VALUE(maxVisitsPondering);
		CONFIG_VALUE(maxPlayoutsPondering);
		CONFIG_VALUE(maxTimePondering);
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

