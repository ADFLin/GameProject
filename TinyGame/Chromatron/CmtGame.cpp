#include "CmtPCH.h"
#include "CmtGame.h"

#include "CmtStage.h"

namespace Chromatron
{
	EXPORT_GAME(GameInstance)

	bool GameInstance::getAttribValue( AttribValue& value )
	{
		switch( value.id )
		{
		case ATTR_SINGLE_SUPPORT:
			value.iVal = true;
			return true;
		case ATTR_AI_SUPPORT:
		case ATTR_NET_SUPPORT:
			value.iVal = false;
			return true;
		case ATTR_CONTROLLER_DEFUAULT_SETTING:
			return true;
		}
		return false;
	}

	void GameInstance::beginPlay( StageModeType type, StageManager& manger )
	{
		IGameInstance::beginPlay( type , manger );
	}

	StageBase* GameInstance::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME: return new LevelStage;
		}
		return NULL;
	}


}//namespace Chromatron


