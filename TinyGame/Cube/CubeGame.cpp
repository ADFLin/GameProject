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

	void GameModule::beginPlay( StageModeType type, StageManager& manger )
	{
		IGameModule::beginPlay( type , manger );
	}

	void GameModule::enter()
	{

	}

	void GameModule::exit()
	{

	}

}//namespace Cube


