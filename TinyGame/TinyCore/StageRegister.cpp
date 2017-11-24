#include "StageRegister.h"

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
}

StageRegisterCollection& StageRegisterCollection::Get()
{
	static StageRegisterCollection instance;
	return instance;
}

StageRegisterHelper::StageRegisterHelper(StageInfo const& info)
{
	StageRegisterCollection::Get().registerStage(info);
}
