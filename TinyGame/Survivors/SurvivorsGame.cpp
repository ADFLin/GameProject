#include "SurvivorsGame.h"
#include "SurvivorsStage.h"
#include "StageBase.h"

namespace Survivors
{
	EXPORT_GAME_MODULE(GameModule)

	void GameModule::beginPlay(StageManager& manger, EGameMode modeType)
	{
		changeDefaultStage(manger, modeType);
	}

	StageBase* GameModule::createStage(unsigned id)
	{
		switch (id)
		{
		case STAGE_SINGLE_GAME:
			return nullptr;
		}
		return nullptr;
	}

	bool GameModule::queryAttribute(GameAttribute& value)
	{
		switch (value.id)
		{
		case ATTR_NET_SUPPORT:
			value.iVal = false;
			return true;
		case ATTR_SINGLE_SUPPORT:
			value.iVal = true;
			return true;
		case ATTR_REPLAY_SUPPORT:
			value.iVal = false;
			return true;
		case ATTR_INPUT_DEFUAULT_SETTING:
			return true;
		}
		return false;
	}

}//namespace Survivors
