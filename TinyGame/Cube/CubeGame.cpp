#include "CubePCH.h"
#include "CubeGame.h"

#include "CubeStage.h"

namespace Cube
{
	StageBase* GameInstance::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME:
			return new TestStage;
		}
		return nullptr;
	}

	void GameInstance::beginPlay( StageModeType type, StageManager& manger )
	{
		IGameInstance::beginPlay( type , manger );
	}

	void GameInstance::enter()
	{

	}

	void GameInstance::exit()
	{

	}

}//namespace Cube

EXPORT_GAME( Cube::GameInstance )
