#include "rcPCH.h"
#include "rcBuildingManager.h"

#include "rcBuilding.h"
#include "rcLevelCity.h"
#include "rcWorker.h"
#include "rcGameData.h"
#include "rcMapLayerTag.h"

rcBuildingManager* g_BuildingMgr = NULL;

rcBuildingManager::rcBuildingManager() 
{
	g_BuildingMgr = this;
	mBTable.reserve( 2048 );
}

rcBuilding* rcBuildingManager::createBuilding(  rcCityInfo& cInfo , unsigned bTag , unsigned modelID , Vec2i const& pos , bool swapAxis )
{
	if ( bTag == BT_ROAD || bTag == BT_AQUADUCT )
		return NULL;

	rcBuildingInfo info = g_buildingInfo[ bTag ];

	rcBuilding* building = NULL;
	switch( info.classGroup )
	{
	case BGC_BASIC      : building = new rcBasicBuilding; break;
	case BGC_WORKSHOP   : building = new rcWorkShop; break;
	case BGC_ROW_FACTORY: building = new rcRowFactory; break;
	case BGC_FARM       : building = new rcFarm; break;
	case BGC_HOUSE      : building = new rcHouse; break;
	case BGC_SERVICE    : building = new rcServiceBuilding; break;
	case BGC_STORAGE    : building = new rcStorageHouse; break;
	case BGC_MARKET     : building = new rcMarket; break;
	}

	if ( building )
	{
		building->mTagType = bTag;
		building->mManageID = mBTable.insert( building );
		building->mPos = pos;
		building->setModelID( modelID );

		BuildingList& blgList = getBuildingList( building );
		blgList.push_back( building );

		if ( swapAxis )
			building->addFlag( rcBuilding::eSwapAxis );

		building->onCreate( cInfo );
	}

	return building;
}


void rcBuildingManager::destoryBuilding(  rcCityInfo& cInfo , rcBuilding* building )
{
	if ( !mBTable.getItem( building->getManageID() ) )
	{
		return;
	}

	building->onDestroy( cInfo );

	BuildingList& blgList = getBuildingList( building );

	BuildingList::iterator iter = std::find( blgList.begin() , blgList.end() , building );
	blgList.erase( iter );

	mBTable.remove( building->getManageID() );


	delete building;
}


void rcBuildingManager::update( rcCityInfo& cInfo , long time )
{

	for( BuildingTable::iterator iter = mBTable.begin();
		 iter != mBTable.end() ; ++iter )
	{
		rcBuilding* building = *iter;
		building->update( cInfo , time );
	}
}


rcBuilding* rcBuildingManager::findStorage( FindCookie& cookie , unsigned pTag )
{
	ProductStorage& ps = rcDataManager::getInstance().getProductStorage( pTag );

	BuildingList* curBList;
	if ( cookie.var == -1 )
	{
		cookie.var = 0;
		curBList = &mBuildingTagMap[ ps.buildTag[0] ];
		cookie.iter = curBList->begin();
	}
	else
	{
		curBList = &mBuildingTagMap[ ps.buildTag[ cookie.var ] ];
		++cookie.iter;
	}

	if ( cookie.iter != curBList->end() )
		return *cookie.iter;

	for( ++cookie.var ; cookie.var < ps.num ; ++ cookie.var )
	{
		curBList = &mBuildingTagMap[ ps.buildTag[ cookie.var ] ];

		cookie.iter = curBList->begin(); 
		if ( cookie.iter != curBList->end()  )
			return *cookie.iter;
	}

	return NULL;
}

rcHouse* rcBuildingManager::findEmptyHouse( FindCookie& cookie , int& numEmptySpace )
{
	BuildingList& blgList = mBuildingTagMap[ BT_HOUSE_SIGN ];

	if ( cookie.var == -1 )
	{
		cookie.iter = blgList.begin();
		cookie.var = 0;
	}
	else
	{
		++cookie.iter;
	}

	for( ; cookie.iter != blgList.end(); ++cookie.iter )
	{
		assert( (*cookie.iter)->downCast< rcHouse >() != NULL );
		rcHouse* house = static_cast< rcHouse* >( *cookie.iter );

		if ( !house->checkFlag( rcHouse::eAccessRoad ) )
			continue;
		numEmptySpace = house->getEmptySpaceNum();
		if ( numEmptySpace )
			return house;
	}

	cookie.var = -1;
	return NULL;
}

void rcBuildingManager::updateBuildingList()
{
	struct HouseSortFun
	{
		bool operator()( rcBuilding* b1 , rcBuilding* b2 )
		{
			rcHouse* h1 = static_cast< rcHouse* >( b1 );
			rcHouse* h2 = static_cast< rcHouse* >( b2 );
			return h1->getEmptySpaceNum() > h2->getEmptySpaceNum();
		}
	};

}

rcBuildingManager::BuildingList& rcBuildingManager::getBuildingList( rcBuilding* building )
{
	if ( building->getBuildingInfo().classGroup == BGC_HOUSE )
	{
		return mBuildingTagMap[ BT_HOUSE_SIGN ];
	}
	else
	{
		return mBuildingTagMap[ building->getTypeTag() ];
	}
}

