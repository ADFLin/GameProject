#include "RichPCH.h"
#include "RichGame.h"

#include "RichStage.h"

namespace Rich
{
	EXPORT_GAME(GameInstance)

	StageBase* GameInstance::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME:
			return new LevelStage;
		}
		return nullptr;
	}

	void GameInstance::beginPlay( StageModeType type, StageManager& manger )
	{
		IGameInstance::beginPlay( type , manger );

	}

}//namespace Rich