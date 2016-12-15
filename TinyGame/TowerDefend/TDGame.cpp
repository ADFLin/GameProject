#include "TDPCH.h"
#include "TDGame.h"

#include "TDStage.h"

namespace TowerDefend
{
	EXPORT_GAME(GameInstance)

	StageBase* GameInstance::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME: 
			return new LevelStage;
		}

		return NULL;
	}

	void GameInstance::beginPlay( StageModeType type, StageManager& manger )
	{
		manger.changeStage( STAGE_SINGLE_GAME );
	}

}//namespace TowerDefend

