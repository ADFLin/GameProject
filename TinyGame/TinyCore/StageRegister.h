#ifndef StageRegister_h__
#define StageRegister_h__

#include "CommonMarco.h"

#include <map>
#include <vector>

enum class StageRegisterGroup
{
	GraphicsTest ,
	Test ,
	PhyDev ,
	Dev ,
	Dev4 ,

	NumGroup ,
};

class IStageFactory
{
public:
	virtual void* create() = 0;
};

struct StageInfo
{
	char const*    decl;
	IStageFactory* factory;
	StageRegisterGroup group;

	StageInfo(char const* decl, IStageFactory* factory, StageRegisterGroup group)
		:decl(decl), factory(factory), group(group)
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
	StageRegisterGroup group;
};

class StageRegisterCollection
{
public:
	StageRegisterCollection();
	~StageRegisterCollection();
	GAME_API void registerStage(StageInfo const& info);
	void registerGameStage()
	{

	}

	std::vector< StageInfo > const& getGroupStage(StageRegisterGroup group) { return mStageGroupMap[group]; }
private:
	std::map< StageRegisterGroup, std::vector< StageInfo > > mStageGroupMap;
};

GAME_API extern StageRegisterCollection gStageRegisterCollection;

struct StageRegisterHelper
{
	GAME_API StageRegisterHelper(StageInfo const& info);
};




#define REGISTER_STAGE( DECL , CLASS , GROUP )\
	static StageRegisterHelper ANONYMOUS_VARIABLE(gStageRegister)( StageInfo( DECL , makeStageFactory< CLASS >() , GROUP ) );



#endif // StageRegister_h__