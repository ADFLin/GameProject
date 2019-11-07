#include "TDPCH.h"
#include "TDGame.h"

#include "TDStage.h"

namespace TowerDefend
{
	EXPORT_GAME_MODULE(GameModule)

	StageBase* GameModule::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME: 
			return new LevelStage;
		}

		return NULL;
	}

	void GameModule::beginPlay( StageManager& manger, StageModeType modeType )
	{
		manger.changeStage( STAGE_SINGLE_GAME );
	}

}//namespace TowerDefend

