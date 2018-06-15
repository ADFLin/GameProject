#ifndef StageRegister_h__
#define StageRegister_h__

#include "GameConfig.h"

#include <map>
#include <vector>

enum class EStageGroup
{
	GraphicsTest ,
	Test ,
	PhyDev ,
	Dev ,
	Dev4 ,
	FeatureDev,

	NumGroup ,
};

class IStageFactory
{
public:
	virtual void* create() = 0;
};

struct StageInfo
{
	char const*     title;
	IStageFactory*  factory;
	EStageGroup     group;
	int             priority;

	StageInfo(char const* inTitle, IStageFactory* inFactory, EStageGroup inGroup , int inPriority = 0)
		:title(inTitle), factory(inFactory), group(inGroup) , priority(inPriority)
	{
	}
};

template< class T >
class TStageFactory : public IStageFactory
{
public:
	void* create() { return new T; }
};

template< class T >
static IStageFactory* makeStageFactory()
{
	static TStageFactory< T > myFactory;
	return &myFactory;
}

struct GameStageInfo
{
	char const* title;
	char const* game;
	EStageGroup group;
};

class StageRegisterCollection
{
public:
	StageRegisterCollection();
	~StageRegisterCollection();

	TINY_API void registerStage(StageInfo const& info);

	TINY_API static StageRegisterCollection& Get();

	std::vector< StageInfo > const& getGroupStage(EStageGroup group) { return mStageGroupMap[group]; }
private:
	std::map< EStageGroup, std::vector< StageInfo > > mStageGroupMap;
};


struct StageRegisterHelper
{
	TINY_API StageRegisterHelper(StageInfo const& info);
};


#define REGISTER_STAGE( TITLE , CLASS , GROUP )\
	static StageRegisterHelper ANONYMOUS_VARIABLE(gStageRegister)( StageInfo( TITLE , makeStageFactory< CLASS >() , GROUP ) );

#define REGISTER_STAGE2( TITLE , CLASS , GROUP , PRIORITY)\
	static StageRegisterHelper ANONYMOUS_VARIABLE(gStageRegister)( StageInfo( TITLE , makeStageFactory< CLASS >() , GROUP , PRIORITY) );

#endif // StageRegister_h__