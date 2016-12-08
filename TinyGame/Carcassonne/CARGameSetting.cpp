#include "CARGameSetting.h"

namespace CAR
{

	GameSetting::GameSetting()
	{
		mExpansionMask = 0;
		mNumField = 0;
		mFarmScoreVersion = 3;
		std::fill_n(mFieldIndex, (int)FieldType::NUM, -1);
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

