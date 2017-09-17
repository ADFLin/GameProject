#include "CARGameplaySetting.h"

namespace CAR
{

	GameplaySetting::GameplaySetting()
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

	bool GameplaySetting::have(Rule ruleFunc) const
	{

		return mRuleFlags.check((unsigned)ruleFunc);
	}

	void GameplaySetting::addRule(Rule ruleFunc)
	{
		mRuleFlags.add((unsigned)ruleFunc);
	}

	unsigned GameplaySetting::getFollowerMask()
	{
		unsigned const BaseFollowrMask =
			BIT(ActorType::eMeeple) | BIT(ActorType::eBigMeeple) |
			BIT(ActorType::eMayor) | BIT(ActorType::eWagon) |
			BIT(ActorType::eBarn) | BIT(ActorType::ePhantom);

		return BaseFollowrMask;
	}

}//namespace CAR

