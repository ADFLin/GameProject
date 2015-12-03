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

	void GamePackage::enter( StageManager& manger )
	{
		manger.changeStage( STAGE_SINGLE_GAME );

	}

}//namespace Rich