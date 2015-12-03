#include "TDPCH.h"
#include "TDGame.h"

#include "TDStage.h"

namespace TowerDefend
{
	GameSubStage* CGamePackage::createSubStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME: 
			return new LevelStage;
		}

		return NULL;
	}

	void CGamePackage::enter( StageManager& manger )
	{
		manger.changeStage( STAGE_SINGLE_GAME );
	}

}//namespace TowerDefend

