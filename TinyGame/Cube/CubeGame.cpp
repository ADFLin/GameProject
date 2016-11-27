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

	void CGamePackage::beginPlay( StageModeType type, StageManager& manger )
	{
		IGamePackage::beginPlay( type , manger );
	}

	void CGamePackage::enter()
	{

	}

	void CGamePackage::exit()
	{

	}

}//namespace Cube

EXPORT_GAME( Cube::CGamePackage )
