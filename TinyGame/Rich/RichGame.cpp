#include "RichPCH.h"
#include "RichGame.h"

#include "RichStage.h"

EXPORT_GAME( Rich::GamePackage )

namespace Rich
{
	StageBase* GamePackage::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME:
			return new LevelStage;
		}
		return nullptr;
	}

	void GamePackage::beginPlay( GameType type, StageManager& manger )
	{
		IGamePackage::beginPlay( type , manger );

	}

}//namespace Rich