#include "ABPCH.h"
#include "ABAction.h"
#include "ABStage.h"

namespace AutoBattler
{
	void ABFrameActionTemplate::firePortAction(ActionPort port, ActionTrigger& trigger)
	{

	}

	void ABFrameActionTemplate::setupPlayer(IPlayerManager& playerManager)
	{

	}

	void ABFrameActionTemplate::executeActions(LevelStage& stage)
	{
		for (size_t i = 0; i < mNumPort; ++i)
		{
			ActionPort port = mFrameData[i].port;
			for (int k = 0; k < mFrameData[i].actions.size(); ++k)
			{
				stage.executeAction(port, mFrameData[i].actions[k]);
			}
		}
	}
}
