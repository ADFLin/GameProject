#include "CmtPCH.h"
#include "CmtGame.h"

#include "CmtStage.h"

namespace Chromatron
{

	bool CGamePackage::getAttribValue( AttribValue& value )
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

	void CGamePackage::beginPlay( GameType type, StageManager& manger )
	{
		IGamePackage::beginPlay( type , manger );
	}

	StageBase* CGamePackage::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME: return new LevelStage;
		}
		return NULL;
	}

	GameSubStage* CGamePackage::createSubStage( unsigned id )
	{
		return NULL;
	}


}//namespace Chromatron


EXPORT_GAME( Chromatron::CGamePackage )