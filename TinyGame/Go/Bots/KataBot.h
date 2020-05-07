#pragma once
#ifndef KataBot_H_DB01698C_6FC9_40BA_A32D_C6CA090EBA2C
#define KataBot_H_DB01698C_6FC9_40BA_A32D_C6CA090EBA2C

#include "GTPBotBase.h"

#define KATA_MODEL_DIR_NAME "models"

namespace Go
{
	namespace KataGameParam
	{
		enum
		{
			eWinRate,
		};
	}

	template< class T >
	struct TMemoryStorage
	{
		uint8 mData[sizeof(T)];
	};

	template< class T >
	class TOptional
	{
	public:
		bool isSet() const { return mIsSet; }


		
	private:
		TMemoryStorage<T> mValue;
		bool mIsSet;
	};


	struct KataAISetting
	{
		char const* modelName = nullptr;

		bool bUseDefaultConfig = false;

		// Search limits---------------------------------------------------------------------------------- -
		// If provided, limit maximum number of root visits per search to this much. (With tree reuse, visits do count earlier search)
		int maxVisits = 3200; //1000
		// If provided, limit maximum number of new playouts per search to this much. (With tree reuse, playouts do not count earlier search)
		int maxPlayouts = 0;//300
		//# If provided, cap search time at this many seconds(search will still try to follow GTP time controls)
		double maxTime = 0; //10
		// Ponder on the opponent's turn?
		bool ponderingEnabled = false;
		// Same limits but for ponder searches if pondering is enabled
		int maxVisitsPondering = 0;//1000
		int maxPlayoutsPondering = 0; //1000
		double maxTimePondering = 0; //60
		// Number of seconds to buffer for lag for GTP time controls
		double lagBuffer = 1.0;
		// Number of threads to use in search
		int numSearchThreads = 6;

		// Play a little faster if the opponent is passing, for friendliness
		double searchFactorAfterOnePass = 0.50;
		double searchFactorAfterTwoPass = 0.25;
		// Play a little faster if super - winning, for friendliess
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