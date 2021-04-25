#ifndef StageRegister_h__
#define StageRegister_h__

#include "GameConfig.h"
#include "HashString.h"

#include "Meta/IsBaseOf.h"
#include "Meta/EnableIf.h"

#include <map>
#include <vector>
#include <unordered_set>


class StageBase;
class StageManager;

enum class EStageGroup
{
	GraphicsTest ,
	Test ,
	PhyDev ,
	Dev ,
	Dev4 ,
	FeatureDev,

	Main ,

	SingleDev,
	SingleGame,
	NumGroup ,
};

class IStageOperation
{
public:
	virtual void process(StageManager* manager) = 0;
};

struct StageInfo
{
	char const*       title;
	IStageOperation*  operation;
	EStageGroup       group;
	int               priority;

	std::unordered_set< HashString > categories;

	StageInfo(char const* inTitle, IStageOperation* inOperation, EStageGroup inGroup , int inPriority, char const* categoryStr = nullptr)
		:title(inTitle), operation(inOperation), group(inGroup) , priority(inPriority)
	{
		AddCategories(categories, group);
		ParseCategories(categories, categoryStr);
	}
	StageInfo(char const* inTitle, IStageOperation* inOperation, EStageGroup inGroup,  char const* categoryStr = nullptr)
		:StageInfo(inTitle, inOperation, inGroup , 0 , categoryStr)
	{

	}

	TINY_API static void ParseCategories(std::unordered_set< HashString >& inoutCategories, char const* categoryStr);
	TINY_API static void AddCategories(std::unordered_set< HashString >& inoutCategories, EStageGroup group);
};


class ChangeStageOperation : public IStageOperation
{
public:
	TINY_API virtual void process(StageManager* manager);
	virtual StageBase* createStage() { return nullptr; }
};

template< class T >
class TChangeStageOperation : public ChangeStageOperation
{
public:
	StageBase* createStage() { return new T; }
};

template< class T , typename = TEnableIf_Type< TIsBaseOf< T, StageBase >::Value > >
IStageOperation* MakeChangeStageOperation()
{
	static TChangeStageOperation< T > operation;
	return &operation;
}

class ChangeSingleGameOperation : public IStageOperation
{
public:
	ChangeSingleGameOperation(char const* inGameName)
		:gameName(inGameName)
	{

	}

	TINY_API virtual void process(StageManager* manager);
	char const* gameName;
};

class StageRegisterCollection
{
public:
	StageRegisterCollection();
	~StageRegisterCollection();

	TINY_API void registerStage(StageInfo const& info);

	TINY_API static StageRegisterCollection& Get();

	std::vector< StageInfo > const& getGroupStages(EStageGroup group) { return mStageGroupMap[group]; }
	TINY_API std::vector< StageInfo const* > getAllStages(HashString category);

	std::vector< HashString > getCategories() const
	{
		return std::vector< HashString >{ mCategories.begin(), mCategories.end() };
	}
private:
	std::map< EStageGroup, std::vector< StageInfo > > mStageGroupMap;
	std::unordered_set< HashString > mCategories;
};


struct StageRegisterHelper
{
	TINY_API StageRegisterHelper(StageInfo const& info);
};


#define REGISTER_STAGE( TITLE , CLASS , GROUP , ... )\
	static StageRegisterHelper ANONYMOUS_VARIABLE(gStageRegister)( StageInfo( TITLE , MakeChangeStageOperation< CLASS >() , GROUP , ##__VA_ARGS__) );


#endif // StageRegister_h__