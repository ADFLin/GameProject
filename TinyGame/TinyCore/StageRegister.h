#ifndef StageRegister_h__
#define StageRegister_h__

#include "GameConfig.h"
#include "HashString.h"

#include "Meta/IsBaseOf.h"
#include "Meta/EnableIf.h"
#include "DataStructure/Array.h"
#include "Math/TVector2.h"

#include "SystemPlatform.h"
#include "SystemMessage.h"

#include <unordered_map>
#include <unordered_set>
#include <functional>

class StageBase;
class StageManager;

enum class EExecGroup
{
	GraphicsTest ,
	Test ,
	PhyDev ,
	Dev ,
	Dev4 ,
	FeatureDev,

	Main ,

	MiscTest ,
	MiscTestFunc,

	SingleDev,
	SingleGame,
	NumGroup ,
};

class IGameExecutionContext
{
public:
	virtual void changeStage(StageBase* stage) = 0;
	virtual void playSingleGame(char const* name) = 0;
};

using ExecuteFunc = std::function< void(IGameExecutionContext&) >;

struct ExecutionEntryInfo
{
	char const*       title;

	ExecuteFunc       execFunc;
	EExecGroup        group;
	int               priority;

	std::unordered_set< HashString > categories;

	ExecutionEntryInfo(char const* inTitle, ExecuteFunc inExecFunc, EExecGroup inGroup, int inPriority, char const* categoryStr = nullptr)
		:title(inTitle), execFunc(inExecFunc), group(inGroup), priority(inPriority)
	{
		AddCategories(categories, group);
		ParseCategories(categories, categoryStr);
	}

	ExecutionEntryInfo(char const* inTitle, ExecuteFunc inExecFunc, EExecGroup inGroup, char const* categoryStr = nullptr)
		:ExecutionEntryInfo(inTitle, inExecFunc, inGroup, 0, categoryStr)
	{
		AddCategories(categories, group);
		ParseCategories(categories, categoryStr);
	}
	TINY_API static void ParseCategories(std::unordered_set< HashString >& inoutCategories, char const* categoryStr);
	TINY_API static void AddCategories(std::unordered_set< HashString >& inoutCategories, EExecGroup group);
	TINY_API static void RecordHistory(ExecutionEntryInfo const& info);
};


BITWISE_RELLOCATABLE_FAIL(ExecutionEntryInfo);


class ExecutionRegisterCollection
{
public:
	ExecutionRegisterCollection();
	~ExecutionRegisterCollection();

	TINY_API void registerExecution(ExecutionEntryInfo const& info);

	TINY_API static ExecutionRegisterCollection& Get();

	TArray< ExecutionEntryInfo > const& getGroupExecutions(EExecGroup group) { return mGroupMap[group]; }

	template< typename TFunc >
	void sortGroup(EExecGroup group, TFunc&& func)
	{
		auto& execInfoList = mGroupMap[group];
		std::sort(execInfoList.begin(), execInfoList.end(), std::forward<TFunc>(func));
	}

	TINY_API TArray< ExecutionEntryInfo const* > getExecutionsByCategory(HashString category);

	TINY_API ExecutionEntryInfo const* findExecutionByTitle(char const* title);
	void cleanup()
	{
		mGroupMap.clear();
		mCategories.clear();
	}
	TArray< HashString > getRegisteredCategories() const
	{
		return TArray< HashString >{ mCategories.begin(), mCategories.end() };
	}

private:
	std::unordered_map< EExecGroup, TArray< ExecutionEntryInfo > > mGroupMap;
	std::unordered_set< HashString > mCategories;
};

struct ExecutionRegisterHelper
{
	ExecutionRegisterHelper(ExecutionEntryInfo const& info)
	{
		ExecutionRegisterCollection::Get().registerExecution(info);
	}
};


template< class T, TEnableIf_Type< TIsBaseOf< T, StageBase >::Value, bool > = true >
ExecuteFunc MakeChangeStageOperation()
{
	return [](IGameExecutionContext& context)
	{
		StageBase* stage = new T();
		context.changeStage(stage);
	};
}

FORCEINLINE ExecuteFunc MakeChangeSingleGame(char const* gameName)
{
	return [gameName](IGameExecutionContext& context)
	{
		context.playSingleGame(gameName);
	};
}

template< class TFunc >
ExecuteFunc MakeSimpleExection(TFunc&& func)
{
	return [func](IGameExecutionContext&)
	{
		func();
	};
}


class IGraphics2D;
using MiscRenderFunc = std::function< void(IGraphics2D&) >;

struct MiscRenderScope
{
	MiscRenderScope()
	{
		flag = nullptr;
	}

	MiscRenderScope(volatile int32* inFlag)
		:flag(inFlag)
	{
		if (flag)
		{
			SystemPlatform::AtomIncrement(flag);
		}
	}

	~MiscRenderScope()
	{
		if (flag)
		{
			SystemPlatform::AtomDecrement(flag);
		}
	}

	volatile int32* flag;
};

class IMiscTestCore
{
public:
	TINY_API IMiscTestCore();
	TINY_API virtual ~IMiscTestCore();

	virtual void pauseExecution(uint32 threadId) = 0;
	virtual MiscRenderScope registerRender(uint32 threadId, MiscRenderFunc const& func, TVector2<int> const& size, bool bTheadSafe) = 0;
	virtual EKeyCode::Type  waitInputKey(uint32 threadId) = 0;
	virtual std::string     waitInputText(uint32 threadId, char const* defaultText) = 0;
};

struct TINY_API FMiscTestUtil
{
	static bool IsTesting();
	static void Pause();
	static MiscRenderScope RegisterRender(MiscRenderFunc const& func, TVector2<int> const& size, bool bTheadSafe = true);
	static EKeyCode::Type  WaitInputKey();
	static std::string     WaitInputText(char const* defaultText);
};

#define TEST_CHECK(C)\
	if (!(C)){ LogWarning(0, "Check Fail - %s (%d): %s", __FILE__ ,__LINE__,  #C); }

#define REGISTER_STAGE_ENTRY( NAME , CLASS , GROUP , ... )\
	static ExecutionRegisterHelper ANONYMOUS_VARIABLE(GExecutionRegister)( ExecutionEntryInfo( NAME , MakeChangeStageOperation< CLASS >() , GROUP , ##__VA_ARGS__) );

#define REGISTER_MISC_TEST_ENTRY( NAME , FUNC , ...)\
	static ExecutionRegisterHelper ANONYMOUS_VARIABLE(GExecutionRegister)( ExecutionEntryInfo( NAME , MakeSimpleExection(FUNC) , EExecGroup::MiscTestFunc , ##__VA_ARGS__ ) );


#endif // StageRegister_h__