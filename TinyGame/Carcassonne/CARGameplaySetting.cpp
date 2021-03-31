#include "CARGameplaySetting.h"

namespace CAR
{
	void AddExpansionRule(GameplaySetting& setting, Expansion exp)
	{
		auto& MyProc = [&setting](ERule rule)
		{
			setting.addRule(rule);
		};
		ProcExpansionRule(exp, MyProc);
	}

	GameplaySetting::GameplaySetting()
	{
		mNumField = 0;
		mFarmScoreVersion = 3;
		for( auto& info : mFieldInfos )
		{
			info.index = INDEX_NONE;
			info.num = 0;
		}

	}

	bool GameplaySetting::have(ERule ruleFunc) const
	{
		return mRuleFlags.check((unsigned)ruleFunc);
	}

	void GameplaySetting::addRule(ERule ruleFunc)
	{
		mRuleFlags.add((unsigned)ruleFunc);
	}

	unsigned GameplaySetting::getFollowerMask() const
	{
		unsigned const BaseFollowrMask =
			BIT(EActor::Meeple) | BIT(EActor::BigMeeple) | BIT(EActor::Abbot) |
			BIT(EActor::Mayor) | BIT(EActor::Wagon) | BIT(EActor::Phantom);

		return BaseFollowrMask;
	}


}//namespace CAR

