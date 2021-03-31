#include "RichPCH.h"
#include "RichGame.h"

#include "RichStage.h"

namespace Rich
{
	EXPORT_GAME_MODULE(GameModule)

	StageBase* GameModule::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME:
			return new LevelStage;
		}
		return nullptr;
	}

	void GameModule::beginPlay( StageManager& manger, EGameStageMode modeType )
	{
		changeDefaultStage(manger, modeType);
	}

}//namespace Rich