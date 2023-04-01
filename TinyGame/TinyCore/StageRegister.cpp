#include "StageRegister.h"

#include "StageBase.h"
#include "StringParse.h"

#include "GameModule.h"
#include "GameGlobal.h"
#include "GameModuleManager.h"

#include <algorithm>


ExecutionRegisterCollection::ExecutionRegisterCollection()
{

}

ExecutionRegisterCollection::~ExecutionRegisterCollection()
{

}

void ExecutionRegisterCollection::registerExecution(ExecutionEntryInfo const& info)
{
	if (info.group == EExecGroup::Dev)
	{
		int i = 1;
	}
	auto& stageInfoList = mGroupMap[info.group];

	auto iter = std::upper_bound(stageInfoList.begin(), stageInfoList.end(), info, 
		[](ExecutionEntryInfo const& lhs, ExecutionEntryInfo const& rhs) -> bool
		{
			return lhs.priority > rhs.priority;
		});
	stageInfoList.insert(iter, info);
	mCategories.insert(info.categories.begin(), info.categories.end());
}

ExecutionRegisterCollection& ExecutionRegisterCollection::Get()
{
	static ExecutionRegisterCollection instance;
	return instance;
}

TArray< ExecutionEntryInfo const* > ExecutionRegisterCollection::getExecutionsByCategory(HashString category)
{
	TArray< ExecutionEntryInfo const* > result;
	for (auto const& pair : mGroupMap)
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

ExecutionEntryInfo const* ExecutionRegisterCollection::findExecutionByTitle(char const* title)
{
	TArray< ExecutionEntryInfo const* > result;
	for (auto const& pair : mGroupMap)
	{
		for (auto const& info : pair.second)
		{
			if (FCString::Compare(info.title, title) == 0)
				return &info;
		}
	}
	return nullptr;
}

ExecutionRegisterHelper::ExecutionRegisterHelper(ExecutionEntryInfo const& info)
{
	ExecutionRegisterCollection::Get().registerExecution(info);
}

void ExecutionRegisterHelper::ChangeStage(StageBase* stage)
{
	if (Manager)
	{
		Manager->setNextStage(stage);
	}
}

void ExecutionRegisterHelper::ChangeSingleGame(char const* name)
{
	if (Manager)
	{
		IGameModule* game = Global::ModuleManager().changeGame(name);
		if (game)
		{
			game->beginPlay(*Manager, EGameStageMode::Single);
		}
	}
}

StageManager* ExecutionRegisterHelper::Manager = nullptr;

void ExecutionEntryInfo::ParseCategories(std::unordered_set< HashString >& inoutCategories, char const* categoryStr)
{
	if (categoryStr)
	{
		StringView token;
		while (FStringParse::StringToken(categoryStr, "|", token))
		{
			token.trimStartAndEnd();
			inoutCategories.emplace(token);
		}
	}
}

void ExecutionEntryInfo::AddCategories(std::unordered_set< HashString >& inoutCategories, EExecGroup group)
{
	switch (group)
	{
	case EExecGroup::GraphicsTest:
		inoutCategories.emplace("Render");
		inoutCategories.emplace("Test");
		break;
	case EExecGroup::Test:
		inoutCategories.emplace("Test");
		break;
	case EExecGroup::PhyDev:
		inoutCategories.emplace("Physics");
		break;
	case EExecGroup::Dev:
		inoutCategories.emplace("Develop");
		break;
	case EExecGroup::Dev4:
		inoutCategories.emplace("Develop");
		break;
	case EExecGroup::FeatureDev:
		inoutCategories.emplace("Develop");
		break;
	case EExecGroup::SingleDev:
		inoutCategories.emplace("Develop");
		inoutCategories.emplace("Game");
		break;
	case EExecGroup::SingleGame:
		inoutCategories.emplace("Game");
		break;
	}

}
