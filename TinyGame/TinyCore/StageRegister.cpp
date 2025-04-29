#include "StageRegister.h"

#include "StageBase.h"
#include "StringParse.h"

#include "GameModule.h"
#include "GameGlobal.h"
#include "GameModuleManager.h"

#include <algorithm>
#include "PlatformThread.h"
#include "PropertySet.h"


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
	auto& execInfoList = mGroupMap[info.group];

	auto iter = std::upper_bound(execInfoList.begin(), execInfoList.end(), info, 
		[](ExecutionEntryInfo const& lhs, ExecutionEntryInfo const& rhs) -> bool
		{
			return lhs.priority > rhs.priority;
		});
	execInfoList.insert(iter, info);
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

void ExecutionEntryInfo::RecordHistory(ExecutionEntryInfo const& info)
{
	PropertySet& config = ::Global::GameConfig();
	
	char const* SectionName = "ExecHistory";
	char const* EntryName = "Entry";

	int maxEntry = config.getIntValue("MaxCount", SectionName, 10);
	TArray< std::string > execHistroy;
	config.getStringValues(EntryName, SectionName, execHistroy);

	int index = execHistroy.findIndexPred([&info](std::string const& name)
	{
		return FCString::Compare(info.title, name.c_str()) == 0;
	});

	if (index != INDEX_NONE)
	{
		if (index != 0)
		{
			execHistroy.removeIndex(index);
			execHistroy.insertAt(0, info.title);
		}
	}
	else if (execHistroy.size() >= maxEntry)
	{
		execHistroy.pop_back();
		execHistroy.insertAt(0, info.title);
	}
	else
	{
		execHistroy.insertAt(0, info.title);
	}

	config.setStringValues(EntryName, SectionName, execHistroy, true);
}

TINY_API IMiscTestCore* GTestCore = nullptr;
bool FMiscTestUtil::IsTesting()
{
	return !!GTestCore;
}

void FMiscTestUtil::Pause()
{
	if (GTestCore)
	{
		GTestCore->pauseExecution(Thread::GetCurrentThreadId());
	}
}

MiscRenderScope FMiscTestUtil::RegisterRender(MiscRenderFunc const& func, TVector2<int> const& size, bool bTheadSafe)
{
	if (GTestCore)
	{
		return GTestCore->registerRender(Thread::GetCurrentThreadId(), func, size, bTheadSafe);
	}

	return MiscRenderScope{};
}

EKeyCode::Type FMiscTestUtil::WaitInputKey()
{
	if (GTestCore)
	{
		return GTestCore->waitInputKey(Thread::GetCurrentThreadId());
	}

	return EKeyCode::None;
}

std::string FMiscTestUtil::WaitInputText(char const* defaultText)
{
	if (GTestCore)
	{
		return GTestCore->waitInputText(Thread::GetCurrentThreadId(), defaultText);
	}

	return "";
}

IMiscTestCore::IMiscTestCore()
{
	if (GTestCore == nullptr)
	{
		GTestCore = this;
	}
}

IMiscTestCore::~IMiscTestCore()
{
	if (GTestCore == this)
	{
		GTestCore = nullptr;
	}
}
