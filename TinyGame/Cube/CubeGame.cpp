#include "CubePCH.h"
#include "CubeGame.h"

#include "CubeStage.h"

namespace Cube
{
	EXPORT_GAME_MODULE(GameModule)

	StageBase* GameModule::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME:
			return new TestStage;
		}
		return nullptr;
	}

	bool GameModule::queryAttribute(GameAttribute& value)
	{
		switch( value.id )
		{
		case ATTR_SINGLE_SUPPORT:
			value.iVal = 1;
			return true;
		}
		return false;
	}

	void GameModule::beginPlay(StageManager& manger, EGameStageMode modeType)
	{
		changeDefaultStage(manger, modeType);
	}

	void GameModule::enter()
	{

	}

	void GameModule::exit()
	{

	}

}//namespace Cube


