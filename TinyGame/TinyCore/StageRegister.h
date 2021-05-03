#ifndef StageRegister_h__
#define StageRegister_h__

#include "GameConfig.h"
#include "HashString.h"

#include "Meta/IsBaseOf.h"
#include "Meta/EnableIf.h"

#include <map>
#include <vector>
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

	SingleDev,
	SingleGame,
	NumGroup ,
};

using ExecuteFunc = std::function< void() >;

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

	}
	TINY_API static void ParseCategories(std::unordered_set< HashString >& inoutCategories, char const* categoryStr);
	TINY_API static void AddCategories(std::unordered_set< HashString >& inoutCategories, EExecGroup group);
};

class ExecutionRegisterCollection
{
public:
	ExecutionRegisterCollection();
	~ExecutionRegisterCollection();

	TINY_API void registerExecution(ExecutionEntryInfo const& info);

	TINY_API static ExecutionRegisterCollection& Get();

	std::vector< ExecutionEntryInfo > const& getGroupExecutions(EExecGroup group) { return mGroupMap[group]; }
	TINY_API std::vector< ExecutionEntryInfo const* > getExecutionsByCategory(HashString category);

	void cleanup()
	{
		mGroupMap.clear();
		mCategories.clear();
	}
	std::vector< HashString > getRegisteredCategories() const
	{
		return std::vector< HashString >{ mCategories.begin(), mCategories.end() };
	}
private:
	std::map< EExecGroup, std::vector< ExecutionEntryInfo > > mGroupMap;
	std::unordered_set< HashString > mCategories;
};


struct ExecutionRegisterHelper
{
	TINY_API ExecutionRegisterHelper(ExecutionEntryInfo const& info);

	TINY_API static void ChangeStage(StageBase* stage);
	TINY_API static void ChangeSingleGame(char const* name);

	static TINY_API StageManager* Manager;
};


template< class T, TEnableIf_Type< TIsBaseOf< T, StageBase >::Value, bool > = true >
ExecuteFunc MakeChangeStageOperation()
{
	return []
	{
		StageBase* stage = new T();
		ExecutionRegisterHelper::ChangeStage(stage);
	};
}

FORCEINLINE ExecuteFunc MakeChangeSingleGame(char const* gameName)
{
	return [gameName]
	{
		ExecutionRegisterHelper::ChangeSingleGame(gameName);
	};
}

#define REGISTER_STAGE_ENTRY( NAME , CLASS , GROUP , ... )\
	static ExecutionRegisterHelper ANONYMOUS_VARIABLE(GExecutionRegister)( ExecutionEntryInfo( NAME , MakeChangeStageOperation< CLASS >() , GROUP , ##__VA_ARGS__) );

#define REGISTER_MISC_TEST_ENTRY( NAME , FUNC , ...)\
	static ExecutionRegisterHelper ANONYMOUS_VARIABLE(GExecutionRegister)( ExecutionEntryInfo( NAME , FUNC , EExecGroup::MiscTest , ##__VA_ARGS__ ) );

#endif // StageRegister_h__