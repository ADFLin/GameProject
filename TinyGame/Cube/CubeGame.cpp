#include "CubePCH.h"
#include "CubeGame.h"

#include "CubeStage.h"

namespace Cube
{
	StageBase* CGamePackage::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME:
			return new TestStage;
		}
		return nullptr;
	}

	void CGamePackage::enter( StageManager& manger )
	{
		manger.changeStage( STAGE_SINGLE_GAME );
	}

	bool CGamePackage::load()
	{
		return true;
	}

	void CGamePackage::release()
	{

	}

}//namespace Cube

EXPORT_GAME( Cube::CGamePackage )
