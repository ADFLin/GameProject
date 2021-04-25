#include "StageRegister.h"

#include "StageBase.h"
#include "StringParse.h"

#include "GameModule.h"
#include "GameGlobal.h"
#include "GameModuleManager.h"

#include <algorithm>


StageRegisterCollection::StageRegisterCollection()
{

}

StageRegisterCollection::~StageRegisterCollection()
{

}

void StageRegisterCollection::registerStage(StageInfo const& info)
{
	auto& stageInfoList = mStageGroupMap[info.group];

	auto iter = std::upper_bound(stageInfoList.begin(), stageInfoList.end(), info, 
		[](StageInfo const& lhs, StageInfo const& rhs) -> bool
		{
			return lhs.priority > rhs.priority;
		});
	mStageGroupMap[info.group].insert(iter, info);
	mCategories.insert(info.categories.begin(), info.categories.end());
}

StageRegisterCollection& StageRegisterCollection::Get()
{
	static StageRegisterCollection instance;
	return instance;
}

std::vector< StageInfo const* > StageRegisterCollection::getAllStages(HashString category)
{
	std::vector< StageInfo const* > result;
	for (auto const& pair : mStageGroupMap)
	{
		for (auto const& info : pair.second)
		{
			if (info.categories.find(category) != info.categories.end())
			{
				result.push_back(&info);
			}
		}
	}
	return result;
}

StageRegisterHelper::StageRegisterHelper(StageInfo const& info)
{
	StageRegisterCollection::Get().registerStage(info);
}

void StageInfo::ParseCategories(std::unordered_set< HashString >& inoutCategories, char const* categoryStr)
{
	if (categoryStr)
	{
		StringView token;
		categoryStr = FStringParse::SkipSpace(categoryStr);
		while (FStringParse::StringToken(categoryStr, "|", token))
		{
			token.removeHeadSpace();
			token.removeTailSpace();
			inoutCategories.emplace(token);
			categoryStr = FStringParse::SkipSpace(categoryStr);
		}
	}
}

void StageInfo::AddCategories(std::unordered_set< HashString >& inoutCategories, EStageGroup group)
{
	switch (group)
	{
	case EStageGroup::GraphicsTest:
		inoutCategories.emplace("Render");
		inoutCategories.emplace("Test");
		break;
	case EStageGroup::Test:
		inoutCategories.emplace("Test");
		break;
	case EStageGroup::PhyDev:
		inoutCategories.emplace("Physics");
		break;
	case EStageGroup::Dev:
		inoutCategories.emplace("develop");
		break;
	case EStageGroup::Dev4:
		inoutCategories.emplace("develop");
		break;
	case EStageGroup::FeatureDev:
		inoutCategories.emplace("develop");
		break;
	case EStageGroup::SingleDev:
		inoutCategories.emplace("develop");
		inoutCategories.emplace("Game");
		break;
	case EStageGroup::SingleGame:
		inoutCategories.emplace("Game");
		break;
	}

}

void ChangeStageOperation::process(StageManager* manager)
{
	StageBase* stage = createStage();
	if (stage)
	{
		manager->setNextStage(stage);
	}
}

void ChangeSingleGameOperation::process(StageManager* manager)
{
	IGameModule* game = Global::ModuleManager().changeGame(gameName);
	if (game)
	{
		game->beginPlay(*manager, EGameStageMode::Single);
	}
}
