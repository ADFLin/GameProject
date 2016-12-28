#include "CARGameSetting.h"

namespace CAR
{

	GameSetting::GameSetting()
	{
		mExpansionMask = 0;
		mNumField = 0;
		mFarmScoreVersion = 3;
		for( auto& info : mFieldInfos )
		{
			info.index = -1;
			info.num = 0;
		}

	}

	bool GameSetting::haveRule(Rule ruleFunc) const
	{

		return mRuleFlags.check((unsigned)ruleFunc);
	}

	void GameSetting::addRule(Rule ruleFunc)
	{
		mRuleFlags.add((unsigned)ruleFunc);
	}

	unsigned GameSetting::getFollowerMask()
	{
		unsigned const BaseFollowrMask =
			BIT(ActorType::eMeeple) | BIT(ActorType::eBigMeeple) |
			BIT(ActorType::eMayor) | BIT(ActorType::eWagon) |
			BIT(ActorType::eBarn) | BIT(ActorType::ePhantom);

		return BaseFollowrMask;
	}

}//namespace CAR

