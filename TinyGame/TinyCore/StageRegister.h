#ifndef StageRegister_h__
#define StageRegister_h__

#include "MarcoCommon.h"

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
	char const*        decl;
	IStageFactory*     factory;
	EStageGroup group;
	int                priority;

	StageInfo(char const* inDecl, IStageFactory* inFactory, EStageGroup inGroup , int inPriority = 0)
		:decl(inDecl), factory(inFactory), group(inGroup) , priority(inPriority)
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
	char const* decl;
	char const* game;
	EStageGroup group;
};

class StageRegisterCollection
{
public:
	StageRegisterCollection();
	~StageRegisterCollection();

	GAME_API void registerStage(StageInfo const& info);

	GAME_API static StageRegisterCollection& Get();

	std::vector< StageInfo > const& getGroupStage(EStageGroup group) { return mStageGroupMap[group]; }
private:
	std::map< EStageGroup, std::vector< StageInfo > > mStageGroupMap;
};


struct StageRegisterHelper
{
	GAME_API StageRegisterHelper(StageInfo const& info);
};




#define REGISTER_STAGE( DECL , CLASS , GROUP )\
	static StageRegisterHelper ANONYMOUS_VARIABLE(gStageRegister)( StageInfo( DECL , makeStageFactory< CLASS >() , GROUP ) );



#endif // StageRegister_h__