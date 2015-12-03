#include "common.h"
#include "UtilityFlyFun.h"
#include "UtilityMath.h"

#include "TLogicObj.h"
#include "TTrigger.h"
#include "TLevel.h"
#include "TGame.h"

void TSpawnZone::debugDraw( bool beDebug )
{
	if ( beDebug )
	{
		FnMaterial mat = UF_CreateMaterial();
		mat.SetEmissive( Vec3D(1,1,1) );
		m_dbgObj.Object( getLiveLevel()->getFlyScene().CreateObject() );
		UF_CreateBoxLine( &m_dbgObj , 5 , 5 , 10 , mat.Object());
		m_dbgObj.SetWorldPosition( pos );
	}
	else
	{
		UF_DestoryObject( m_dbgObj );
	}
}

void PlayerBoxTrigger::debugDraw( bool beDebug )
{
	if ( beDebug )
	{
		FnMaterial mat = UF_CreateMaterial();
		mat.SetEmissive( Vec3D(1,1,1) );
		Vec3D min , max;
		getPhyBody()->getAabb( min , max  );
		Vec3D len = 0.5 * ( max - min );
		UF_CreateBoxLine( &m_dbgObj , len.x() , len.y() , len.z() , mat.Object());
	}
	else
	{
		m_dbgObj.RemoveAllGeometry();
	}

}