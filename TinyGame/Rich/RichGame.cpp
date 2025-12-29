#include "RichGame.h"

#include "RichStage.h"

namespace Rich
{
	EXPORT_GAME_MODULE(GameModule)

	StageBase* GameModule::createStage(unsigned id)
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME:
			return new LevelStage;
		}
		return nullptr;
	}

	void GameModule::beginPlay( StageManager& manger, EGameMode modeType )
	{
		::Global::GetDrawEngine().setupSystem(this);
		changeDefaultStage(manger, modeType);
	}

	void GameModule::endPlay()
	{
		::Global::GetDrawEngine().setupSystem(nullptr);
	}

	bool GameModule::queryAttribute(GameAttribute& value)
	{
		switch (value.id)
		{
		case ATTR_SINGLE_SUPPORT:
			value.iVal = true;
			return true;
		}
		return false;
	}


}//namespace Rich