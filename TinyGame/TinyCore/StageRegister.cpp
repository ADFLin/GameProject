#include "StageRegister.h"

GAME_API StageRegisterCollection gStageRegisterCollection;

StageRegisterCollection::StageRegisterCollection()
{

}

StageRegisterCollection::~StageRegisterCollection()
{

}

void StageRegisterCollection::registerStage(StageInfo const& info)
{
	mStageGroupMap[info.group].push_back(info);
}

StageRegisterHelper::StageRegisterHelper(StageInfo const& info)
{
	gStageRegisterCollection.registerStage(info);
}
