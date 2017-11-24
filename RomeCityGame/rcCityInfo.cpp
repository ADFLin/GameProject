#include "rcPCH.h"
#include "rcCityInfo.h"

#include "rcLevelMap.h"
#include "rcWorkerManager.h"
#include "rcBuilding.h"
#include "rcMapLayerTag.h"

rcCityInfo::rcCityInfo()
{
	levelMap = NULL;
	workerMgr = NULL;
	blgQuery = NULL;
}

rcBuilding* rcCityInfo::getBuilding( Vec2i const& pos )
{
	if ( !levelMap->isRange( pos ) )
		return NULL;

	rcMapData const& data = levelMap->getData( pos );

	if ( data.buildingID == RC_ERROR_BUILDING_ID )
		return NULL;

	return blgQuery->getBuilding( data.buildingID );
}

rcBuilding* rcCityInfo::getBuilding( unsigned blgID )
{
	return blgQuery->getBuilding( blgID );
}

void rcCityInfo::destoryWorker( rcWorker* worker )
{
	workerMgr->destoryWorker( worker );
}

rcWorker* rcCityInfo::createWorker( unsigned typeTag )
{
	return workerMgr->createWorker( typeTag );
}

bool rcCityInfo::updateResviorWaterState( rcBuilding& building , int x , int y , int count , bool waterState )
{
	assert( building.getTypeTag() == BT_RESEVIOR  && building.getSize() == Vec2i(3,3) );

	Vec2i centerPos = building.getMapPos() + Vec2i(1,1);

	if ( x != centerPos.x || y != centerPos.y )
		return false;

	building.removeFlag( rcBuilding::eWaterFilled );

	bool beFill = false;
	for( int i = 0 ; i < 4 ; ++i )
	{
		Vec2i pos = centerPos + g_OffsetDir[i];
		updateWaterSourceRecursive( pos.x , pos.y , count , waterState );
		beFill |= levelMap->getData( pos ).haveConnect( CT_LINK_WATER );
	}

	building.changeFlag( rcBuilding::eWaterFilled , beFill );

	return true;
}

void rcCityInfo::updateWaterSource( Vec2i const& pos , bool waterState , bool reset )
{
	if ( reset )
		mCheckData.fillValue( 0 );

	assert ( levelMap->isRange( pos ) );

	rcMapData& data = levelMap->getData( pos );

	if ( waterState )
	{
		data.addConnect( CT_LINK_WATER );
	}
	else
	{
		data.removeConnect( CT_LINK_WATER );
	}

	mCheckData.getData( pos.x , pos.y  ) = 1;
	updateWaterSourceRecursive( pos.x + 1 , pos.y , 1 , waterState );
	updateWaterSourceRecursive( pos.x - 1 , pos.y , 1 , waterState );
	updateWaterSourceRecursive( pos.x , pos.y + 1 , 1 , waterState );
	updateWaterSourceRecursive( pos.x , pos.y - 1 , 1 , waterState );
}

void rcCityInfo::updateWaterSourceRecursive( int x , int y , int count , bool waterState )
{
	if( !levelMap->isRange( x , y ) )
		return;

	rcMapData& data = levelMap->getData( x , y );

	if ( !data.haveConnect( CT_WATER_WAY ) )
		return;

	if ( mCheckData.getData( x , y ) >= count )
		return;

	mCheckData.getData( x , y ) = count;

	if ( data.buildingID != RC_ERROR_BUILDING_ID )
	{
		rcBuilding* building = getBuilding( data.buildingID );

		if ( building->getTypeTag() != BT_RESEVIOR )
			return;

		if ( !building->checkFlag( rcBuilding::eWaterSource ) )
		{
			if ( updateResviorWaterState( *building , x , y , count , waterState  ) )
				return;
		}
		else
		{
			if ( !waterState )
				++count;
			waterState = true;
		}
	}
	else //
	{
		if ( !waterState && !data.haveConnect( CT_LINK_WATER ) )
			return;
	}

	if ( waterState )
		data.addConnect( CT_LINK_WATER );
	else
		data.removeConnect( CT_LINK_WATER );

	updateWaterSourceRecursive( x + 1 , y , count , waterState  );
	updateWaterSourceRecursive( x - 1 , y , count , waterState  );
	updateWaterSourceRecursive( x , y + 1 , count , waterState  );
	updateWaterSourceRecursive( x , y - 1 , count , waterState  );
}

void rcCityInfo::setLevelMap( rcLevelMap* map )
{
	levelMap = map;
	mCheckData.resize( levelMap->getSizeX() , levelMap->getSizeY() );
}
