#include "TDPCH.h"
#include "TDGame.h"

#include "TDStage.h"

namespace TowerDefend
{
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

EXPORT_GAME( TowerDefend::GameInstance )
