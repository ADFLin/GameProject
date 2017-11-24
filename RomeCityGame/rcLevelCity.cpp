#include "rcPCH.h"
#include "rcLevelCity.h"

#include "rcWorker.h"
#include "rcBuilding.h"
#include "rcLevelMap.h"
#include "rcLevelScene.h"
#include "rcGameData.h"
#include "rcMapLayerTag.h"
#include "rcPathFinder.h"

#include "rcWorkerManager.h"
#include "rcBuildingManager.h"

#include "TileRegion.h"

rcLevelCity* g_LevelCity = NULL;

rcLevelCity* getLevelCity()
{
	return g_LevelCity;
}

rcLevelCity::rcLevelCity() 
	:mLevelMap   ( new rcLevelMap )
	,mWorkerMgr  ( new rcWorkerManager )
	,mBuildingMgr( new rcBuildingManager )
	,mLevelScene ( new rcLevelScene )
{

	g_LevelCity = this;
	mWorkerMgr->setLevelCity( this );

	//FIXME
	mLevelScene->mBuildingMgr = mBuildingMgr;
	mLevelScene->setLevelMap( mLevelMap );
}

rcLevelCity::~rcLevelCity()
{

}

bool rcLevelCity::init()
{
	mLevelScene->init();
	std::fill_n( mProductSaveNum , ARRAY_SIZE( mProductSaveNum ) , 0 );
	mNumSettler = 0;

	mInfo.workerMgr = mWorkerMgr;
	mInfo.blgQuery  = mBuildingMgr;

	return true;
}

void rcLevelCity::setupMap( Vec2i const& size )
{
	mLevelMap->init( size.x , size.y );

	mInfo.setLevelMap( mLevelMap );

	mPathFinder.reset( rcPathFinder::creatPathFinder( size ) );
	mPathFinder->setLevelMap( mLevelMap );

	mRegionMgr.reset( new RegionManager( size ) );
}

void rcLevelCity::update( long time )
{
	extern rcGlobal g_Global;
	g_Global.cityTime += time;
	
	{
		PROFILE_ENTRY2( "BuildingManager::update"  , PROF_DISABLE_CHILDREN );
		mBuildingMgr->update( mInfo , time );
	}

	{
		PROFILE_ENTRY2( "updateSettler", PROF_DISABLE_CHILDREN  );
		updateSettler( time );
	}

	{
		PROFILE_ENTRY( "WorkerManager::update");
		mWorkerMgr->update( time );
	}

	mLevelScene->update( time );
}



void rcLevelCity::render()
{
	mLevelScene->renderLevel();
}

bool rcLevelCity::buildContruction( Vec2i const& pos , unsigned tagID , int idxModel , bool swapAxis )
{
	if ( !mLevelMap->isRange( pos ) )
		return false;

	rcBuildingInfo const& info = 
		rcDataManager::getInstance().getBuildingInfo( tagID );

	int forceDir;
	Vec2i size;

	unsigned modelID;
	if ( idxModel != 0 && info.numExtraModel > idxModel - 1 )
	{
		modelID = info.startExtraModel + idxModel - 1 ;
	}
	else
	{
		modelID = tagID ;
	}

	if ( tagID == BT_AQUADUCT || tagID == BT_ROAD )
	{
		if ( !checkRoadAquaductConnect( pos , tagID , forceDir ) )
			return false;

		size.setValue( 1 , 1 );
	}
	else
	{
		ModelInfo const& infoModel = rcDataManager::getInstance().getBuildingModel( modelID );

		assert( infoModel.xSize !=0 && infoModel.ySize != 0 );
		if ( swapAxis ) 
			size = Vec2i( infoModel.ySize , infoModel.xSize );
		else
			size = Vec2i( infoModel.xSize , infoModel.ySize );
	}

	if ( !mLevelMap->checkCanBuild( info , pos ,size ) )
		return false;

	rcBuilding* building = mBuildingMgr->createBuilding( mInfo , tagID , modelID , pos , swapAxis );

	if ( building )
	{
		ModelInfo const& infoModel = building->getModel();
		size = building->getSize();

		mLevelMap->markBuilding( info.typeTag , building->getManageID() , pos , size  );

		if ( building->updateEntryPos( *mLevelMap ) )
			building->addFlag( rcBuilding::eAccessRoad );
	}
	else //Road Wall
	{
		size.setValue( 1 , 1 );
		mLevelMap->markBuilding( info.typeTag , RC_ERROR_BUILDING_ID , pos , size  );
	}

	//mRegionMgr->productBlock( pos , size );

	rcMapData& data = mLevelMap->getData( pos );

	switch( tagID )
	{
	case BT_ROAD:
		{
			data.addConnect( CT_ROAD );

			if ( data.haveConnect( CT_WATER_WAY ) )
				mLevelMap->updateAquaductType( pos );
			else
				mLevelMap->updateRoadType( pos );

			for( int i = 0 ; i < 4 ; ++i )
			{
				Vec2i destPos = pos + g_OffsetDir[i];
				if ( !mLevelMap->isRange( destPos ) )
					continue;
				updateRoadLink( destPos );
			}
		}
		break;
	case BT_AQUADUCT:
		{
			data.addConnect( CT_WATER_WAY );

			bool haveWater = false;
			bool needUpdate = false;

			for( int i = 0 ; i < 4 ; ++i )
			{
				Vec2i destPos = pos + g_OffsetDir[i];
				rcMapData const* linkData = mLevelMap->getLinkData( pos , i );
				if ( linkData && linkData->haveConnect( CT_WATER_WAY ) )
				{
					if ( linkData->haveConnect( CT_LINK_WATER ) )
						haveWater = true;
					else
						needUpdate = true;
				}
			}

			if ( haveWater )
			{
				data.addConnect( CT_LINK_WATER );

				if ( needUpdate )
					mInfo.updateWaterSource( pos, true );
			}

			mLevelMap->updateAquaductType( pos , forceDir );

			for( int i = 0 ; i < 4 ; ++i )
			{
				Vec2i destPos = pos + g_OffsetDir[i];
				if ( !mLevelMap->isRange( destPos ) )
					continue;
				updateAquaductLink( destPos );
			}
		}
		break;
	default:
		updateRoadLink( pos );
	}

	return true;
}

void rcLevelCity::destoryContruction( Vec2i const& pos )
{
	if ( !mLevelMap->isRange( pos ) )
		return;

	rcMapData& data = mLevelMap->getData( pos );
	
	if ( data.buildingID != RC_ERROR_BUILDING_ID )
	{
		rcBuilding* building = mBuildingMgr->getBuilding( data.buildingID );

		mLevelMap->unmarkBuilding( building->getMapPos() , building->getSize() );

		mBuildingMgr->destoryBuilding( mInfo , building );
		return;
	}
	else if ( data.layer[ ML_BUILDING ]  )
	{
		switch ( data.layer[ ML_BUILDING ] )
		{
		case BT_ROAD:
		case BT_AQUADUCT:
			{
				bool updateRoad = data.haveConnect( CT_ROAD );
				bool updateWater = data.haveConnect( CT_LINK_WATER );
				bool updateAquaduct = data.haveConnect( CT_WATER_WAY );

				data.removeConnect( CT_WATER_WAY );
				data.removeConnect( CT_ROAD );
				data.removeConnect( CT_LINK_WATER );
				mLevelMap->unmarkBuilding( pos , Vec2i(1,1) );

				if ( updateRoad || updateAquaduct )
				{
					for( int i = 0 ; i < 4 ; ++i )
					{
						Vec2i destPos = pos + g_OffsetDir[i];
						if ( !mLevelMap->isRange( destPos ) )
							continue;
						if ( updateRoad )
							updateRoadLink( destPos );
						if ( updateAquaduct )
							updateAquaductLink( destPos );
					}
				}
				if ( updateWater )
				{
					mInfo.updateWaterSource( pos , false );
				}
			}
			break;
		}
	}
}

void rcLevelCity::updateAquaductLink( Vec2i const& pos )
{
	rcMapData& data = mLevelMap->getData( pos );

	if ( !data.layer[ ML_BUILDING ] )
		return;

	rcBuilding* building = mBuildingMgr->getBuilding( data.buildingID );

	if ( building )
	{
		if ( building->getTypeTag() != BT_RESEVIOR )
			return;
	}
	else if ( data.haveConnect( CT_WATER_WAY ) )
	{
		mLevelMap->updateAquaductType( pos );
	}

}
void rcLevelCity::updateRoadLink( Vec2i const& pos )
{
	rcMapData& data = mLevelMap->getData( pos );

	if ( !data.layer[ ML_BUILDING ] )
		return;

	rcBuilding* building = mBuildingMgr->getBuilding( data.buildingID );

	if ( building )
	{
		if ( !building->checkFlag( rcBuilding::eNoNeedRoad ) )
		{
			bool needCalc = !building->checkFlag( rcBuilding::eAccessRoad  ) ||
				!mLevelMap->getData( building->getEntryPos() ).haveRoad();

			if ( needCalc )
			{
				Vec2i entryPos;
				if (  building->updateEntryPos( *mLevelMap ) )
				{
					building->addFlag( rcBuilding::eAccessRoad );
				}
				else
				{
					building->removeFlag( rcBuilding::eAccessRoad );
				}
			}
		}
	}
	else if ( data.haveRoad() )
	{
		mLevelMap->updateRoadType( pos );
	}
}

void rcLevelCity::updateSettler( long time )
{
	rcHouse* house = NULL;
	Vec2i mapPos(0,0);

	int const MaxNumComingRate = 50;
	int numComing = 0;
	while(  mNumSettler > 0 )
	{
		int numEmptySpace;
		rcBuildingManager::FindCookie cookie;
		house = mBuildingMgr->findEmptyHouse( cookie , numEmptySpace );

		if ( !house )
			break;

		int num = std::min( mNumSettler , numEmptySpace );
		if ( house->addSettler( mInfo , mapPos , num ) )
		{
			mNumSettler -= num;
			numComing   += num;

			if ( mNumSettler == 0 )
				break;

			if ( numComing > MaxNumComingRate )
				break;
		}

	}
	
}

rcPath* rcLevelCity::createPath( Vec2i const& from , Vec2i const& to )
{
	mPathFinder->setupDestion( to );
	return mPathFinder->sreachPath(from);
}

rcPath* rcLevelCity::createPath( Vec2i const& from , unsigned blgID )
{
	mPathFinder->setupDestion( mBuildingMgr->getBuilding( blgID ) );
	return mPathFinder->sreachPath(from);
}

rcBuilding* rcLevelCity::tryTransportProduct( rcWorker* worker , unsigned pTag, int numProduct )
{
	rcBuildingManager::FindCookie cookie;

	rcBuilding* building = NULL;

	int maxTryNum = 1;

	int count = 0;
	while( count < maxTryNum  )
	{
		++count;
		rcBuilding* building = mBuildingMgr->findStorage( cookie , pTag );
		if ( building == NULL )
			break;

		if ( !building->checkFlag( rcBuilding::eAccessRoad ) )
			continue;

		//rcWorkShop* ws = building->downCast< rcWorkShop >();
		if ( building->getFreeProductStorage( pTag ) >= numProduct )
		{
			return NULL;

			rcWorker::TransportInfo& info = worker->getBuildingVar().infoTP;
			info.pTag = pTag;
			info.num  = numProduct;
			worker->changeMission( 
				new TransportProductMission( building->getManageID() ,true ) );
			return building;
		}
	}

	return NULL;
}



rcBuilding* rcLevelCity::tryGetProduct( rcWorker* worker , unsigned pTag, int numProduct )
{
	rcBuildingManager::FindCookie cookie;

	rcBuilding* building = NULL;
	while( 1 )
	{
		rcBuilding* building = mBuildingMgr->findStorage( cookie , pTag );
		if ( building == NULL )
			break;

		if ( !building->checkFlag( rcBuilding::eAccessRoad ) )
			continue;

		//rcWorkShop* ws = building->downCast< rcWorkShop >();
		if ( building->getFreeProductStorage( pTag ) >= numProduct )
		{
			rcWorker::TransportInfo& info = worker->getBuildingVar().infoTP;
			info.pTag = pTag;
			info.num  = numProduct;
			worker->changeMission( 
				new TransportProductMission( building->getManageID() ,true ) );
			return building;
		}
	}

	return NULL;
}

void rcLevelCity::sendWorkerMsg( rcWorker* worker , unsigned blgID , WorkerMsg msg , unsigned content )
{
	rcBuilding* building = getBuildingManager().getBuilding( blgID );
	building->_OnWorkerMsg( worker , msg , content );
}

rcBuilding* rcLevelCity::getBuilding( unsigned blgID )
{
	return mBuildingMgr->getBuilding( blgID );
}

rcBuilding* rcLevelCity::getBuilding( Vec2i const& pos )
{
	if ( !mLevelMap->isRange( pos ) )
		return NULL;

	rcMapData const& data = mLevelMap->getData( pos );

	if ( data.buildingID == RC_ERROR_BUILDING_ID )
		return NULL;

	return mBuildingMgr->getBuilding( data.buildingID );
}

int rcLevelCity::obtainLabor( Vec2i const& pos , int wantNum )
{
	bool haveFind = false;
	for( int i = 0 ; i < 4 ; ++i )
	{
		Vec2i destPos = pos + g_OffsetDir[i];

		rcBuilding* building;

		building = getBuilding( destPos );
		if ( building && building->getBuildingInfo().classGroup == BGC_HOUSE &&
			building->getPeopleNum() == 0 )
		{
			haveFind = true;
			break;
		}

		destPos += g_OffsetDir[i];

		building = getBuilding(  destPos );
		if ( building && building->getBuildingInfo().classGroup == BGC_HOUSE &&
			building->getPeopleNum() == 0 )
		{
			haveFind = true;
			break;
		}
	}

	if ( !haveFind )
		return 0;

	//FIXME
	return wantNum;
}

bool rcLevelCity::checkRoadAquaductConnect( Vec2i const& pos , unsigned tagID , int& allowDir )
{
	if ( !mLevelMap->isRange( pos ) )
		return false;
	assert( tagID == BT_AQUADUCT || tagID == BT_ROAD );
	ConnectType checkType = ( tagID == BT_AQUADUCT ) ? CT_ROAD : CT_WATER_WAY;

	rcMapData const& data = mLevelMap->getData( pos );

	if ( !data.haveConnect( checkType ) )
	{
		allowDir = -1;
		return true;
	}

	bool prvHave = false;
	for( int i = 0 ; i < 4 ; ++i )
	{
		Vec2i destPos = pos + g_OffsetDir[i];
		rcMapData const& destData = mLevelMap->getData( destPos );
		if ( destData.haveConnect( CT_ROAD ) &&
			 destData.haveConnect( CT_WATER_WAY ) )
			return false;

		if ( destData.haveConnect( checkType ) )
		{
			if ( prvHave )
				return false;
			prvHave = true;
			allowDir = ( i + 1 ) % 2;
		}
		else
		{
			prvHave = false;
		}

	}
	return true;
}
