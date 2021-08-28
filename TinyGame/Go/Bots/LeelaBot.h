#pragma once
#ifndef LeelaBot_H_42280575_AB74_4D08_8444_953C8BD1062B
#define LeelaBot_H_42280575_AB74_4D08_8444_953C8BD1062B

#include "GTPBotBase.h"
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

		enum EGenMoveMethod
		{
			Normal,
			Analyze,
		};
		EGenMoveMethod mGenMoveMethod = EGenMoveMethod::Analyze;

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

	class LeelaBot : public TGTPLikeBot< LeelaAppRun >
	{
	public:
		enum MetaId
		{
			eWeightName ,
		};
	
		virtual bool initialize(IBotSetting* setting) override;
		virtual bool isGPUBased() const override { return true; }
		virtual bool getMetaData(int id, uint8* dataBuffer, int size);

		virtual EBotExecResult playPass(int color) override
		{
			if (!mAI.playPass(color))
				return BOT_FAIL;

			return BOT_OK;
		}

	};



}//namespace Go

#endif // LeelaBot_H_42280575_AB74_4D08_8444_953C8BD1062B
