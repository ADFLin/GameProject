#pragma once
#ifndef KataBot_H_DB01698C_6FC9_40BA_A32D_C6CA090EBA2C
#define KataBot_H_DB01698C_6FC9_40BA_A32D_C6CA090EBA2C

#include "GTPBotBase.h"
#include "Template/Optional.h"

#define KATA_MODEL_DIR_NAME "models"

namespace Go
{
	namespace KataGameParam
	{
		enum
		{
			eWinRate,
			eThinkResult,
		};
	}

	struct KataThinkInfo
	{
		PlayVertex v;
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
		std::vector< PlayVertex > vSeq;
	};


	struct KataAISetting
	{
		char const* modelName = nullptr;

		bool bUseDefaultConfig = false;
		bool bUseCuda = true;


		template< class T >
		class TVar : public TOptional< T >
		{
		public:
			TVar() = default;

			TVar(T const& value)
				:TOptional<T>(value)
			{
				mDefalut = value;
			}

			TVar(T const& value , ENoInit)
				:TOptional<T>()
			{
				mDefalut = value;
			}

			void setDefault()
			{
				*this = mDefalut;
			}
			T mDefalut;
		};


		enum ERules
		{
			TrompTaylor ,
			Chinese ,
			Japanse ,
			Korean ,
			Aga ,
			ChineseOgs,
			NewZealand,
		};

		ERules rules = ERules::TrompTaylor;

		enum EKoRule
		{
			SIMPLE,
			POSITIONAL, 
			SITUATIONAL,
		};

		TVar<EKoRule> koRule;

		enum EScoringRule
		{
			//Area scoring
			AREA,
			//Territory scoring (uses a sort of special computer-friendly territory ruleset)
			TERRITORY,
		};
		TVar<EScoringRule> scoringRule;

		enum ETexRule
		{
			//All surrounded empty points are scored
			NONE,
			//Eyes in seki do NOT count as points
			SEKI,
			//All groups are taxed up to 2 points for the two eyes needed to live
			ALL,
		};
		TVar< ETexRule > texRule;

		// Is multiple-stone suicide legal? (Single-stone suicide is always illegal).
		TVar< bool > multiStoneSuicideLegal = { true , NoInit };

		bool allowResignation = true;
		float resignThreshold = -0.90f;
		int resignConsecTurns = 3;
		TVar< int > resignMinScoreDifference = { 10 , NoInit };

		// Handicap------------ 
		TVar< bool > assumeMultipleStartingBlackMovesAreHandicap = { true , NoInit };
		TVar< double > dynamicPlayoutDoublingAdvantageCapPerOppLead = { 0.045 , NoInit };
		TVar< double > playoutDoublingAdvantage = { 0.0 , NoInit };
		enum EStoneColor
		{
			BLACK,
			WHITE,
		};
		TVar< EStoneColor > playoutDoublingAdvantagePla;

		//Passing and cleanup
		TVar< bool > conservativePass = { true , NoInit };
		TVar< bool > preventCleanupPhase = { true , NoInit };
		//Misc Behavior
		// Uncomment and set to true to make KataGo avoid a particular joseki that some KataGo nets misevaluate,
		// and also to improve opening diversity versus some particular other bots that like to play it all the time.
		TVar< bool > avoidMYTDaggerHack = { false , NoInit };

		// Search limits---------------------------------------------------------------------------------- -
		// If provided, limit maximum number of root visits per search to this much. (With tree reuse, visits do count earlier search)
		int maxVisits = 3200; //1000
		// If provided, limit maximum number of new playouts per search to this much. (With tree reuse, playouts do not count earlier search)
		TVar< int > maxPlayouts = { 300 , NoInit };
		// If provided, cap search time at this many seconds(search will still try to follow GTP time controls)
		TVar< double > maxTime = { 10 , NoInit };
		// Ponder on the opponent's turn?
		bool ponderingEnabled = false;
		TVar< int > maxVisitsPondering;
		TVar< int > maxPlayoutsPondering;
		TVar< double > maxTimePondering = 60.0;

		double lagBuffer = 1.0;

		// Number of threads to use in search
		int numSearchThreads = 20;

		// Play a little faster if the opponent is passing, for friendliness
		double searchFactorAfterOnePass = 0.50;
		double searchFactorAfterTwoPass = 0.25;

		// Play a little faster if super - winning, for friendliness
		double searchFactorWhenWinning = 0.40;
		double searchFactorWhenWinningThreshold = 0.95;

		KataAISetting()
		{

		}

		bool writeToConifg(char const* path) const;
	};

	class KataAppRun : public GTPLikeAppRun
	{
	public:
		static char const* InstallDir;

		enum EGenMoveMethod
		{
			Normal ,
			Debug ,
			Analyze ,
		};
		EGenMoveMethod mGenMoveMethod = EGenMoveMethod::Analyze;
		bool bShowInfo = true;
		
		bool thinkNextMove(int color);

		static std::string GetLastModeltName();

		class KataOutputThread* getOutputThred();
		void clearCache()
		{


		}


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