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
		return "g104";
	}

	bool KataAppRun::buildPlayGame(KataAISetting const& setting)
	{
		char const* configName = "temp.cfg";
		FixString<256> configPath;
		configPath.format("%s/%s", InstallDir, configName);
		if (!setting.writeToConifg(configPath))
			return false;

		std::string modelName = setting.modelName ? setting.modelName : GetLastModeltName().c_str();

		FixString<256> path;
		path.format("%s/%s", InstallDir, "KataGo.exe");
		FixString<512> command;
		command.format(" gtp -model models/%s.txt.gz -config %s", modelName.c_str(), configName);
		return buildProcessT< KataOutputThread >(path, command);
	}

	char const* FixedConfigText = R"CODE_STRING_(
# Logs------------------------------------------------------------------------------------
logFile = gtp.log
logAllGTPCommunication = true
logSearchInfo = true
logToStderr = false

# startupPrintMessageToStderr = false
# ogsChatToStderr = true
# analysisPVLen = 9
# reportAnalysisWinratesAs = SIDETOMOVE

# Rules------------------------------------------------------------------------------------
# whiteBonusPerHandicapStone = 1

# GPU Settings-----------------------------------------------------------------------------
nnMaxBatchSize = 16
nnCacheSizePowerOfTwo = 18
nnMutexPoolSizePowerOfTwo = 14
nnRandomize = true
# nnRandSeed = abcdefg

numNNServerThreadsPerModel = 1

# CUDA GPU settings--------------------------------------
# cudaDeviceToUse = 0 #use device 0 for all server threads(numNNServerThreadsPerModel) unless otherwise specified per - model or per - thread - per - model
# cudaDeviceToUseModel0 = 3 #use device 3 for model 0 for all threads unless otherwise specified per - thread for this model
# cudaDeviceToUseModel1 = 2 #use device 2 for model 1 for all threads unless otherwise specified per - thread for this model
# cudaDeviceToUseModel0Thread0 = 3 #use device 3 for model 0, server thread 0
# cudaDeviceToUseModel0Thread1 = 2 #use device 2 for model 0, server thread 1

# cudaUseFP16 = true
# cudaUseNHWC = true

# OpenCL GPU settings--------------------------------------
# openclDeviceToUse = 0 #use device 0 for all server threads(numNNServerThreadsPerModel) unless otherwise specified per - model or per - thread - per - model
# openclDeviceToUseModel0 = 3 #use device 3 for model 0 for all threads unless otherwise specified per - thread for this model
# openclDeviceToUseModel1 = 2 #use device 2 for model 1 for all threads unless otherwise specified per - thread for this model
# openclDeviceToUseModel0Thread0 = 3 #use device 3 for model 0, server thread 0
# openclDeviceToUseModel0Thread1 = 2 #use device 2 for model 0, server thread 1

# openclReTunePerBoardSize = true
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
	};


	bool KataAISetting::writeToConifg(char const* path) const
	{
		std::string configText;

#define CONFIG_VALUE(VAR) FConfigHelper::Write(configText, #VAR , VAR );

		CONFIG_VALUE(koRule);
		CONFIG_VALUE(scoringRule);

		CONFIG_VALUE(multiStoneSuicideLegal);
		CONFIG_VALUE(cleanupBeforePass);

		CONFIG_VALUE(allowResignation);
		CONFIG_VALUE(resignThreshold);
		CONFIG_VALUE(resignConsecTurns);

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

		if (!searchRandSeed.empty())
		{
			CONFIG_VALUE(searchRandSeed);
		}

		CONFIG_VALUE(chosenMoveTemperatureEarly);
		CONFIG_VALUE(chosenMoveTemperatureHalflife);
		CONFIG_VALUE(chosenMoveTemperature);
		CONFIG_VALUE(chosenMoveSubtract);
		CONFIG_VALUE(chosenMovePrune);

		CONFIG_VALUE(rootNoiseEnabled);
		CONFIG_VALUE(rootDirichletNoiseTotalConcentration);
		CONFIG_VALUE(rootDirichletNoiseWeight);

		CONFIG_VALUE(useLcbForSelection);
		CONFIG_VALUE(lcbStdevs);
		CONFIG_VALUE(minVisitPropForLCB);

		CONFIG_VALUE(winLossUtilityFactor);
		CONFIG_VALUE(staticScoreUtilityFactor);
		CONFIG_VALUE(dynamicScoreUtilityFactor);
		CONFIG_VALUE(dynamicScoreCenterZeroWeight);
		CONFIG_VALUE(noResultUtilityForWhite);
		CONFIG_VALUE(drawEquivalentWinsForWhite);
		CONFIG_VALUE(cpuctExploration);
		CONFIG_VALUE(fpuReductionMax);
		CONFIG_VALUE(fpuUseParentAverage);
		CONFIG_VALUE(valueWeightExponent);
		CONFIG_VALUE(rootEndingBonusPoints);
		CONFIG_VALUE(rootPruneUselessMoves);
		CONFIG_VALUE(mutexPoolSize);
		CONFIG_VALUE(numVirtualLossesPerThread);

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

