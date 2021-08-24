#include "rcPCH.h"
#include "rcBuilding.h"

#include "rcCityInfo.h"
#include "rcGameData.h"
#include "rcMapLayerTag.h"
#include "rcWorker.h"
#include "rcWorkerData.h"
#include "rcLevelMap.h"
#include "rcProductTag.h"

#include "rcBuildingManager.h"
#include "rcLevelCity.h"

#include <algorithm>

extern rcLevelCity* getLevelCity();
long const g_FindLaborTime = 2000;

rcBuilding::rcBuilding() 
	:mFlag(0)
	,mState( BS_UNKNOWN )
	,mSize(0,0)
	,mNumPeople(0)
	,mModelID(0)
	,mPos( -1 , -1 )
	,mEntryPos( -1 , -1 )
{

}


ModelInfo const& rcBuilding::getModel()
{
	return rcDataManager::Get().getBuildingModel( mModelID );
}

Vec2i const& rcBuilding::getSize()
{
	return mSize;
}

void rcBuilding::updateState()
{
	if ( checkFlag( eForceClose ) )
	{
		mState = BS_CLOSE;
		return;
	}

	switch( mState )
	{
	case BS_UNKNOWN:
		if ( !checkFlag( eNoNeedRoad ) && !checkFlag( eAccessRoad ) )
		{
			mState = BS_NEED_ROAD;
			updateState();
		}
		else
		{
			mState = BS_IDLE;
		}
		return;
	case BS_NEED_ROAD:
		if ( checkFlag( eAccessRoad ) )
		{
			mState = BS_IDLE;
		}
		return;
	}
	doUpdateState();
}

void rcBuilding::setModelID( unsigned id )
{
	ModelInfo& info = rcDataManager::Get().getBuildingModel( id );

	mSize.x = info.xSize;
	mSize.y = info.ySize;

	if ( mSize.x == 1 && mSize.y == 1 )
	{
		addFlag( eSingleTile );
	}
	else
	{
		removeFlag( eSingleTile );

		if ( checkFlag( eSwapAxis ) )
			mSize = Vec2i( info.ySize , info.xSize );
		else
			mSize = Vec2i( info.xSize , info.ySize );
	}
	mModelID = id;
}


bool rcBuilding::updateEntryPos( rcLevelMap& levelMap )
{
	bool result = levelMap.calcBuildingEntry( 
		getMapPos() , getSize() , mEntryPos );

	return result;
}

void rcBuilding::update( rcCityInfo& cInfo , long time )
{
	{
		PROFILE_ENTRY( "rcBuilding::updateState");
		
		updateState();

		struct SecurityInfo
		{
			int speed;
			int maxVaule;
		};

		int const g_CollapseSpeed = 10;
		int const g_FireSpeed = 10;
		int const g_MaxCollapseValue = 2000;
		int const g_MaxFireValue = 2000;

		if ( !checkFlag( eNoCollapse ) )
		{
			mSecurityValue[ SET_COLLAPSE ] += g_CollapseSpeed * time;
			if ( mSecurityValue[ SET_COLLAPSE ]  > g_MaxCollapseValue )
			{

			}
		}

		if ( !checkFlag( eNoFire ) )
		{
			mSecurityValue[ SET_FIRE ] +=  g_FireSpeed * time;
			if ( mSecurityValue[ SET_COLLAPSE ] > g_MaxFireValue )
			{

			}
		}
	}

	{
		PROFILE_ENTRY( "rcBuilding::onUpdate");
		onUpdate( cInfo , time );
	}


}


void rcFartory::onCreate( rcCityInfo& cInfo )
{
	BaseClass::onCreate( cInfo );

	createLaborFindingWorker( cInfo );
	WorkerSlot* slot = createWorker( cInfo , DELMAN_SLOT , WT_DELIVERY_MAN );
}

void rcFartory::onUpdate( rcCityInfo& cInfo ,long time )
{
	BaseClass::onUpdate( cInfo , time );

	switch( getState() )
	{
	case BS_WORKING:
		produceGoods( time );
		break;
	}
}

void rcFartory::onUpdateWorker( WorkerSlot& slot , long time )
{
	if ( getTypeTag() == BT_FARM_WHEAT && checkFlag( eAccessRoad ) )
	{
		int i = 1;
	}

	switch( slot.id )
	{
	case DELMAN_SLOT:
		if ( getState() == BS_IDLE || getState() == BS_WORKING )
		{
			slot.time += time;
			if ( slot.state == WS_IDLE && mNumProduct && checkFlag( eAccessRoad ) )
			{
				int num = std::min( mNumProduct , 1 );

				slot.worker->setMapPos( getEntryPos() );

				rcBuilding* building = getLevelCity()->tryTransportProduct( 
					slot.worker , getFactoryInfo().productID , num );

				if ( building )
				{
					slot.destBuilding = building;
					mNumProduct -= num;
					changeWorkerState( slot , WS_RUN_MISSION );
				}
			}
		}
		break;
	case FIND_HOUSE_SLOT:
		slot.time += time;
		if ( slot.state == WS_IDLE && checkFlag( eAccessRoad ) )
		{
			if ( slot.time > 2000  )
			{
				slot.worker->setMapPos( getEntryPos() );

				slot.time = g_FindLaborTime;
				changeWorkerState( slot , WS_RUN_MISSION );
			}
		}
		break;
	}
}

void rcFartory::onWorkerMsg( WorkerSlot& slot , WorkerMsg msg , unsigned content )
{
	switch( msg )
	{
	case WMT_FINISH: 
		changeWorkerState( slot , WS_IDLE );
		switch( slot.id )
		{
		case FIND_HOUSE_SLOT:
			slot.time = 0;
			break;
		}
		break;
	}

}

void rcFartory::doUpdateState()
{
	rcFactoryInfo& info = getFactoryInfo();
	switch( getState() )
	{
	case BS_IDLE:
		if ( mNumPeople != 0 &&
			 mNumProduct < info.maxProductStorage )
		{
			mState = BS_WORKING;
		}
		return;
	case BS_WORKING:
		if ( mNumProduct >= info.maxProductStorage )
			mState = BS_IDLE;
		return;
	}
}

rcFactoryInfo& rcFartory::getFactoryInfo()
{
	return gFactoryInfo[ mTagType - BT_FACTORY_START ];
}

void rcFartory::produceGoods( long time )
{
	rcFactoryInfo& info = getFactoryInfo();
	rcBuildingInfo& BdgInfo = getBuildingInfo();
	mProgressProduct += info.produceSpeed * mNumPeople / BdgInfo.numMaxPeople * time;

	if ( mProgressProduct > MaxProgressFinish )
	{
		mProgressProduct -= MaxProgressFinish;
		mNumProduct += info.NumProduct;

		mProgressProduct = 0;
		updateState();
	}
}

void rcWorkShop::doUpdateState()
{
	rcFactoryInfo& info = getFactoryInfo();
	switch( getState() )
	{
	case BS_IDLE:
		if ( mNumPeople != 0 && 
			 mNumProduct < info.maxProductStorage )
		{
			unsigned pTagLost;
			if ( tryCastSuff( pTagLost ) )
			{
				mState = BS_WORKING;
			}
			else
			{

			}
		}
		return;
	case BS_WORKING:
		if ( mNumProduct >= info.maxProductStorage )
			mState = BS_IDLE;
		return;
	}
}
void rcWorkShop::onCreate( rcCityInfo& cInfo )
{
	BaseClass::onCreate( cInfo );
}

void rcWorkShop::onUpdate( rcCityInfo& cInfo , long time )
{
	BaseClass::onUpdate( cInfo , time );


}

int rcWorkShop::getFreeProductStorage( unsigned pTag )
{
	int index = getStuffIndex( pTag );

	if ( index == INDEX_NONE )
		return 0;
	rcFactoryInfo& info = getFactoryInfo();
	return info.stuff[index].storage - mNumStuff[index];

}

int rcWorkShop::getStuffIndex( unsigned pTag )
{
	rcFactoryInfo& info = getFactoryInfo();
	for( int i = 0 ; i < info.numStuff ; ++i )
	{
		if ( info.stuff[i].productID == pTag )
			return i;
	}
	return INDEX_NONE;
}

bool rcWorkShop::tryCastSuff( unsigned& pTagLost )
{
	rcFactoryInfo& info = getFactoryInfo();
	for ( int i = 0 ; i < info.numStuff ; ++i )
	{
		if ( mNumStuff[i] < info.stuff[i].cast )
		{
			pTagLost = info.stuff[i].productID;
			return false;
		}
	}

	for ( int i = 0 ; i < info.numStuff ; ++i )
	{
		mNumStuff[i] -= info.stuff[i].cast;
	}
	return true;
}

bool rcWorkShop::recvGoods( unsigned pTag , int num )
{
	int index = getStuffIndex( pTag );

	if ( index == INDEX_NONE )
		return false;
	rcFactoryInfo& info = getFactoryInfo();
	if ( info.stuff[index].storage - mNumStuff[index] < num )
		return false;

	mNumStuff[ index ] += num;
	return true;
}

int g_GraneryStorageNum = 160;
void rcStorageHouse::onCreate( rcCityInfo& cInfo )
{
	BaseClass::onCreate( cInfo );
	
	switch( getTypeTag() )
	{
	case BT_WAREHOUSE:
		mStorgeProductBit = WareHouseProductBit;
		mNumMaxSave = 2;
		mCanMixSave = false;
		setupGoodsRule( FoodProductBit , SR_STOP );
		break;
	case BT_GRANERY:
		mStorgeProductBit = GranaryProductBit;
		mNumMaxSave = g_GraneryStorageNum;
		mCanMixSave = true;
		{
			assert( getModel().xSize == 3 && getModel().ySize == 3 );
			rcLevelMap& levelMap = *cInfo.levelMap;

			Vec2i centerPos = getMapPos() + Vec2i( 1 , 1 );
			levelMap.getData( centerPos ).addConnect( CT_ROAD );
			for( int i = 0 ; i < 4 ; ++i )
			{
				Vec2i const& pos = centerPos + g_OffsetDir[ i ];
				levelMap.getData( pos ).addConnect( CT_ROAD );
			}
		}
		break;
	}

	mAllowProductBit = mStorgeProductBit;
}

void rcStorageHouse::onDestroy( rcCityInfo& cInfo )
{
	switch( getTypeTag() )
	{
	case BT_GRANERY:
		{
			rcLevelMap& levelMap = *cInfo.levelMap;

			Vec2i centerPos = getMapPos() + Vec2i( 1 , 1 );
			levelMap.getData( centerPos ).removeConnect( CT_ROAD );
			for( int i = 0 ; i < 4 ; ++i )
			{
				Vec2i const& pos = centerPos + g_OffsetDir[ i ];
				levelMap.getData( pos ).removeConnect( CT_ROAD );
			}
		}
		break;
	}

	BaseClass::onDestroy( cInfo );
}


void rcStorageHouse::setupGoodsRule( unsigned pTagBit , Rule rule )
{
	pTagBit &= mStorgeProductBit;

	switch( rule )
	{
	case SR_REMOVED:
		mAllowProductBit &= ~pTagBit;
		mWantProductBit  &= ~pTagBit;
		mRemoveProductBit |= pTagBit;
		break;
	case SR_STOP:
		mAllowProductBit &= ~pTagBit;
		mWantProductBit  &= ~pTagBit;
		mRemoveProductBit &= ~pTagBit;
		break;
	case SR_WANT:
		mAllowProductBit |=  pTagBit;
		mWantProductBit  |=  pTagBit;
		mRemoveProductBit &= ~pTagBit;
		break;
	case SR_ALLOW:
		mAllowProductBit |= pTagBit;
		mWantProductBit  &= ~pTagBit;
		mRemoveProductBit &= ~pTagBit;
		break;
	}
}

bool rcStorageHouse::recvGoods( unsigned pTag , int num )
{
	//FIXME
	if ( !( mAllowProductBit & BIT( pTag ) ) )
		return false;

	int numFree = getFreeProductStorage( pTag );
	if ( numFree < num )
		return false;


	if ( mCanMixSave )
	{
		int idxCell = findPorductCell( pTag  );
		if ( idxCell == -1 )
			return false;

		mStroageCell[ idxCell ].productTag = pTag;
		mStroageCell[ idxCell ].numGoods += num;

	}
	else
	{
		for( int i = 0 ; i < NumStageCell; )
		{
			StroageCell& cell = mStroageCell[i];
			if ( cell.productTag == PT_NULL_PRODUCT )
			{
				int useNum = std::min( num , mNumMaxSave );
				cell.productTag = pTag;
				cell.numGoods   = useNum;
				num -= useNum;
			}
			else if ( cell.productTag == pTag )
			{
				int useNum = std::min( num , mNumMaxSave - cell.numGoods );
				if ( useNum  )
				{
					cell.numGoods += useNum;
					num -= useNum;
				}
				else ++i;
			}
			else
			{
				++i;
			}

			if ( num == 0 )
				break;
		}
	}

	mNumCurSave += num;

	return true;
}

int rcStorageHouse::takeGoods( unsigned pTag , int num , bool testEnough )
{
	if ( mWantProductBit & BIT( pTag ) )
		return 0;

	int result = 0;

	if ( mCanMixSave )
	{
		if ( mNumCurSave + num > mNumMaxSave )
			return false;

		int idxCell = findPorductCell( pTag  );
		assert( idxCell != -1 );

		if ( testEnough )
		{
			if ( mStroageCell[ idxCell ].numGoods < num )
				return 0;
			result = num;
		}
		else
		{
			result = std::min ( num , mStroageCell[ idxCell ].numGoods );
		}
		mStroageCell[ idxCell ].numGoods -= result;
		

		return result;
	}
	else
	{
		if ( testEnough )
		{
			for( int i = 0 ; i < NumStageCell; ++i )
			{
				StroageCell& cell = mStroageCell[i];

				if ( cell.productTag == pTag )
				{
					result += cell.numGoods;
				}

				if ( result >= num )
					break;
			}
			if ( result < num )
				return 0;
		}

		result = 0;
		for( int i = 0 ; i < NumStageCell; )
		{
			StroageCell& cell = mStroageCell[i];
			if ( cell.productTag == pTag )
			{
				if ( cell.numGoods > num - result )
				{
					result = num;
					cell.numGoods -= num - result;
					break;
				}
				else
				{
					result += cell.numGoods;
					cell.productTag = PT_NULL_PRODUCT;
					cell.numGoods = 0;
				}
			}
		}
	}


	mNumMaxSave -= result;
	return result;
}

int rcStorageHouse::findPorductCell( unsigned pTag )
{
	for( int i = 0 ; i < NumStageCell; ++i )
	{
		StroageCell& cell = mStroageCell[i];
		if ( cell.productTag == PT_NULL_PRODUCT )
			return i;
	}

	return -1;
}

int rcStorageHouse::getFreeProductStorage( unsigned pTag )
{
	if ( !( mAllowProductBit & BIT(pTag )) )
		return 0;

	if ( mCanMixSave )
		return  mNumMaxSave - mNumCurSave;

	int num = 0;
	for( int i = 0 ; i < NumStageCell; ++i )
	{
		StroageCell& cell = mStroageCell[i];
		if ( cell.productTag == PT_NULL_PRODUCT )
			num += mNumMaxSave;
		else if ( cell.productTag == pTag )
		{
			num += mNumMaxSave - cell.numGoods;
		}
	}
	return  num;
}

bool rcStorageHouse::updateEntryPos( rcLevelMap& levelMap )
{
	if ( getTypeTag() == BT_GRANERY )
	{
		setEntryPos( getMapPos() + Vec2i(1,1) );
		return true;
	}
	return rcBuilding::updateEntryPos( levelMap );
}

unsigned const rcStorageHouse::FoodProductBit =
	BIT( PT_WHEAT ) | BIT( PT_FISHING ) | BIT( PT_VEGETABLES ) | BIT( PT_FRUIT );
unsigned const rcStorageHouse::GranaryProductBit = rcStorageHouse::FoodProductBit;
unsigned const rcStorageHouse::WareHouseProductBit = 0xffffffff;


rcStorageHouse::StroageCell::StroageCell() 
	:productTag( PT_NULL_PRODUCT )
	,numGoods(0)
{

}

rcWorkerBuilding::WorkerSlot* rcWorkerBuilding::createWorker( rcCityInfo& cInfo , int slotID , unsigned workerTag )
{
	assert( mNumWorker < MaxWorkerSlotNum );

	int slotPos;
	if ( mSlot[ mNumWorker ].worker == NULL )
	{
		slotPos = mNumWorker;
	}
	else
	{
		for( int i = 0 ; i < MaxWorkerSlotNum ; ++i )
			if ( mSlot[i].worker == NULL )
				slotPos = i;
	}

	WorkerSlot& slot = mSlot[ slotPos ];
	slot.id     = slotID;
	slot.worker = cInfo.createWorker( workerTag );
	slot.worker->mOwner = getManageID();
	slot.state  = WS_IDLE;
	slot.worker->mSlotPos = slotPos;

	++mNumWorker;
	return  &slot;
}

bool rcWorkerBuilding::changeWorkerState( WorkerSlot& slot , WorkerState state )
{
	switch( state )
	{
	case WS_IDLE:
		slot.worker->removeFlag( rcWorker::eVisible );
		break;
	case WS_RUN_MISSION:
		if ( !slot.worker->restartMission() )
			return false;
		slot.worker->addFlag( rcWorker::eVisible );
		break;
	}

	slot.state = state;
	return true;
}

void rcWorkerBuilding::updateWorker( rcCityInfo& cInfo , long time )
{
	int num = mNumWorker;
	for ( int i = 0 ; i < num ; ++i  )
	{
		if ( mSlot[i].worker == NULL )
			continue;

		if ( mSlot[i].state == WS_REMOVE )
		{
			removeWorker( cInfo , mSlot[i] );
			continue;
		}
		onUpdateWorker( mSlot[i] , time );
	}
}

void rcWorkerBuilding::removeWorker( rcCityInfo& cInfo , WorkerSlot& slot )
{
	int slotPos = (&slot) - mSlot;

	cInfo.destoryWorker( slot.worker );
	slot.worker = NULL;
	--mNumWorker;
}


void rcWorkerBuilding::_OnWorkerMsg( rcWorker* worker , WorkerMsg msg , unsigned content )
{
	onWorkerMsg( mSlot[ worker->mSlotPos ] , msg , content );
}

void rcWorkerBuilding::onFindLabor( rcWorker* worker )
{
	WorkerSlot& slot = mSlot[ worker->mSlotPos ];

	if ( slot.time > g_FindLaborTime )
	{
		int wantNum = getBuildingInfo().numMaxPeople - mNumPeople;

		int num = getLevelCity()->obtainLabor( 
			worker->getMapPos() ,  wantNum );

		if ( num )
		{
			slot.time = 0;
			mNumPeople += num;
			//FIXME
		}
	}
}

void rcWorkerBuilding::createLaborFindingWorker( rcCityInfo& cInfo )
{
	WorkerSlot* slot = createWorker( cInfo , FIND_HOUSE_SLOT , WT_FIND_MAN );
	WorkerInfo& info = rcDataManager::Get().getWorkerInfo( slot->worker->getType() );
	RandomMoveMission* mission =  new RandomMoveMission( info.randMoveDist );
	mission->setOnTileCallback( this , &rcWorkerBuilding::onFindLabor );
	slot->worker->changeMission( mission );
}

void rcWorkerBuilding::onService( rcWorker* worker )
{

}

void rcWorkerBuilding::onDestroy( rcCityInfo& cInfo )
{
	for( int i = 0 ; i < MaxWorkerSlotNum ; ++i )
	{
		if ( mSlot[i].worker )
			removeWorker( cInfo , mSlot[i] );
	}
	assert( mNumWorker == 0 );

	BaseClass::onDestroy( cInfo );
}

void rcWorkerBuilding::onUpdate( rcCityInfo& cInfo , long time )
{
	BaseClass::onUpdate( cInfo , time );
	updateWorker( cInfo , time );
}

bool rcHouse::addSettler( rcCityInfo& cInfo , Vec2i const& mapPos , int numPepole )
{
	WorkerSlot* slot = createWorker( cInfo , SETTLER_SLOT , WT_SETTLER );

	if( slot == NULL )
		return false;

	slot->worker->getBuildingVar().infoSettler.num = numPepole;

	mNumPeopleComing += numPepole;

	slot->worker->setMapPos( mapPos );
	slot->worker->changeMission(
		new DestionMoveMission( getEntryPos() ) );

	changeWorkerState( *slot , WS_RUN_MISSION );

	return true;
}

void rcHouse::updateHouseLevel( rcCityInfo& cInfo )
{
	unsigned level = checkHouseLevel( cInfo );

	unsigned newLevelType;
	if ( level == LR_00_ROAD_ACCESS )
		newLevelType  = BT_HOUSE_SIGN;
	else
		newLevelType = level + BT_HOUSE_SIGN - 1;

	if (  newLevelType != getTypeTag() )
	{
		mTagType = newLevelType;
		
		//FIXME
		setModelID( newLevelType );
	}
}

void rcHouse::updateService( rcLevelMap& levelMap , long time )
{
	struct WaterVisitor : public rcLevelMap::Visitor
	{
		/*virtual */
		bool visit( rcMapData& data )
		{
			if ( data.layer[ ML_SUPPORT_WATER ] > 0 )
			{
				support = true;
				return false;
			}
			return true;
		}
		bool support;
	};

	if ( checkFlag( eCheckWaterSupport ) )
	{
		WaterVisitor visitor;
		visitor.support = false;
		levelMap.visitMapData( visitor , getMapPos() , getSize() );

		changeFlag( eWaterSupport , visitor.support );
		
		removeFlag( eCheckWaterSupport );
	}


}

bool rcHouse::updateEntryPos( rcLevelMap& levelMap )
{
	if ( levelMap.calcBuildingEntry( getMapPos() , getSize() , mEntryPos ) )
		return true;
	if ( levelMap.calcBuildingEntry( getMapPos()-Vec2i(1,0) , getSize() + Vec2i(2,0), mEntryPos ) )
		return true;
	if ( levelMap.calcBuildingEntry( getMapPos()-Vec2i(0,1) , getSize() + Vec2i(0,2), mEntryPos ) )
		return true;

	return false;
}

void rcHouse::onWorkerMsg( WorkerSlot& slot , WorkerMsg msg , unsigned content )
{
	if ( slot.id == SETTLER_SLOT )
	{
		switch( msg )
		{
		case WMT_FINISH:
			mNumPeople += slot.worker->getBuildingVar().infoSettler.num ;
			break;
		}

		mNumPeopleComing -= slot.worker->getBuildingVar().infoSettler.num ;
		changeWorkerState( slot , WS_REMOVE );
		return;
	}
}

void rcHouse::onCreate( rcCityInfo& cInfo )
{
	BaseClass::onCreate( cInfo );
	addFlag( eCheckWaterSupport );
	mMaxLivePeopleNum = 10;
}

void rcHouse::onUpdate( rcCityInfo& cInfo , long time )
{
	BaseClass::onUpdate( cInfo , time );
	updateService( *cInfo.levelMap , time );
	updateHouseLevel( cInfo );
}

unsigned rcHouse::checkHouseLevel( rcCityInfo& cInfo )
{
	if ( !checkFlag( rcHouse::eAccessRoad ) || getPeopleNum() == 0 )
		return LR_00_ROAD_ACCESS;

	(void)LR_01_PEOPLE ;

	if ( !checkFlag( rcHouse::eWaterSupport ) )
		return LR_02_WATER_SOURCE;

	if ( !checkFlag( rcHouse::eFoodSupport ) )
		return LR_03_ONE_FOOD;

	if ( !checkFlag( rcHouse::eGodSupport ) )
		return LR_04_ONE_GOD_ACCESS;

	if ( getServiceValue( ST_WATER_FOUNTAIN ) == 0 )
		return LR_05_WATER_FOUNTAIN;

	if ( !checkFlag( rcHouse::eEntSupport )  )
		return LR_06_1ST_ENTERTAINMENT;


	int countFood = 0;
	countFood += getServiceValue( ST_FOOD_WHEAT ) ? 1 : 0;
	countFood += getServiceValue( ST_FOOD_MEAT  ) ? 1 : 0;
	//countFood += getServiceValue( ST_FOOD_MEAT ) ? 1 : 0;
	//countFood += getServiceValue( ST_FOOD_MEAT ) ? 1 : 0;

	int countGod = 0;
	countGod += getServiceValue( ST_GOD_1 ) ? 1 : 0;
	countGod += getServiceValue( ST_GOD_2 ) ? 1 : 0;
	countGod += getServiceValue( ST_GOD_3 ) ? 1 : 0;
	countGod += getServiceValue( ST_GOD_4 ) ? 1 : 0;
	countGod += getServiceValue( ST_GOD_5 ) ? 1 : 0;

	int countEnt= 0;
	countEnt += getServiceValue( ST_ENT_1 ) ? 1 : 0;
	countEnt += getServiceValue( ST_ENT_2 ) ? 1 : 0;
	countEnt += getServiceValue( ST_ENT_3 ) ? 1 : 0;
	countEnt += getServiceValue( ST_ENT_4 ) ? 1 : 0;
	countEnt += getServiceValue( ST_ENT_5 ) ? 1 : 0;

	return LR_20_HIGHT_DESIRABILITY;
}

rcHouse::rcHouse() 
	:mMaxLivePeopleNum(0)
	,mNumPeopleComing(0)
{
	std::fill_n( mServiceValue , (int)NumHouseService , 0 );
}

void rcBasicBuilding::doUpdateState()
{
	switch( getTypeTag() )
	{
	case BT_RESEVIOR:
		switch( getState() )
		{
		case BS_IDLE:
			if ( checkFlag( eWaterFilled ))
			{
				mState = BS_WORKING;
			}
			break;
		case BS_WORKING:
			if ( !checkFlag( eWaterFilled ) )
			{
				mState = BS_IDLE;
			}
			break;
		}
		break;
	}
}


void rcServiceBuilding::onCreate( rcCityInfo& cInfo )
{
	BaseClass::onCreate( cInfo );

	createLaborFindingWorker( cInfo );

	WorkerSlot* slot;
	switch( getTypeTag() )
	{
#define  CASE_BUILDING_SERVICE( bTag , wTag , sTag )\
	case bTag :\
		slot = createWorker( cInfo , SERVICE_SLOT , wTag );\
		slot->worker->getBuildingVar().serviceTag = sTag;\
		break;

	CASE_BUILDING_SERVICE( BT_THEATRE , WT_ACTOR , ST_ENT_THEATRE );
	CASE_BUILDING_SERVICE( BT_PREFECTURE , WT_PREFECT , ST_SEC_FIRE );


#undef CASE_BUILDING_SERVICE
	}

	if ( slot )
	{
		WorkerInfo& info = rcDataManager::Get().getWorkerInfo( slot->worker->getType() );
		RandomMoveMission* mission =  new RandomMoveMission( info.randMoveDist );
		mission->setOnTileCallback( this , &rcWorkerBuilding::onService );

		slot->worker->changeMission( mission );
	}
}



int g_WellSupportRange = 3;
void rcBasicBuilding::onCreate( rcCityInfo& cInfo )
{
	BaseClass::onCreate( cInfo );

	switch( getTypeTag() )
	{
	case BT_WATER_WELL:
		supportWater( cInfo , true , g_WellSupportRange );
		break;
	case BT_RESEVIOR:
		{
			addFlag( eNoNeedRoad );

			ModelInfo const& model = getModel();
			assert( getModel().xSize == 3 && getModel().ySize == 3 );

			Vec2i centerPos = getMapPos() + Vec2i(1,1);

			setResviorWaterWay( *cInfo.levelMap , true );

			//FIXME
			bool nearWater = getMapPos().x & 1;

			changeFlag( eWaterFilled , nearWater );
			if ( nearWater )
			{
				addFlag( eWaterSource );
				cInfo.updateWaterSource( centerPos , true );
			}
		}
		break;
	}
}

void rcBasicBuilding::onDestory( rcCityInfo& cInfo )
{

	switch( getTypeTag() )
	{
	case BT_WATER_WELL:
		supportWater( cInfo , false , g_WellSupportRange );
		break;

	case BT_RESEVIOR:
		{
			setResviorWaterWay( *cInfo.levelMap , false );
			Vec2i centerPos = getMapPos() + Vec2i(1,1);
			for( int i = 0 ; i < 4 ; ++i )
			{
				Vec2i pos = centerPos + 2 * g_OffsetDir[i];
				if ( !cInfo.levelMap->isRange( pos ) )
					continue;

				cInfo.updateWaterSource( pos , false , i == 0 );
			}
		}
		break;
	}
}

class WaterSupportVisitor : public MapLayerCountVisitor
{
public:
	WaterSupportVisitor( rcCityInfo& c , bool support )
		:city( c )
	{
		layerType = ML_SUPPORT_WATER;
		count =  support ? 1 : -1;
		supportNum = support;
	}

	bool visit( rcMapData& data )
	{
		MapLayerCountVisitor::visit( data );

		if ( data.layer[ ML_SUPPORT_WATER ] != supportNum  )
			return true;

		rcBuilding* building = city.getBuilding( data.buildingID );
		if ( building && building->downCast< rcHouse >() )
		{
			if ( building->checkFlag( rcBuilding::eCheckWaterSupport ) )
				return true;

			if ( supportNum != (int)building->checkFlag( rcBuilding::eWaterSupport ) )
				building->addFlag( rcBuilding::eCheckWaterSupport );
		}
		return true;
	}

	int supportNum;

	rcCityInfo& city;
};

void rcBasicBuilding::supportWater( rcCityInfo& cInfo , bool beSupport , int range )
{
	WaterSupportVisitor visitor( cInfo , true );
	int len = 2 * range + 1;
	cInfo.levelMap->visitMapData( visitor , getMapPos() - Vec2i( range , range ) , Vec2i( len , len ) );
}

void rcBasicBuilding::setResviorWaterWay( rcLevelMap& levelMap , bool beE )
{

	Vec2i centerPos = getMapPos() + Vec2i(1,1);
	levelMap.getData( centerPos ).setConnect( CT_WATER_WAY , beE );
	for( int i = 0 ; i < 4 ; ++i )
	{
		Vec2i pos = centerPos + g_OffsetDir[i];
		levelMap.getData( pos ).setConnect( CT_WATER_WAY , beE );
	}
}



rcMarket::rcMarket()
{
	mProvideServiceBit = 0;
	mCurTakeIdx = 0;
	mNextProvideIdx = 0;
	std::fill_n( mPruductSave , ARRAY_SIZE( mPruductSave ) , 0 );
}

void rcMarket::onUpdateWorker( WorkerSlot& slot , long time )
{
	switch ( slot.id )
	{
	case SERVICE_SLOT:
		if ( slot.state == WS_DIE )
		{
			int i , idx;
			int next = -1;
			for( i = 0 ; i < numSerivce ; ++i )
			{
				idx = mNextProvideIdx + i;
				if ( idx > numSerivce )
					idx -= numSerivce;

				if ( mProvideServiceBit & BIT( idx ) )
				{
					if ( mPruductSave[ idx ] != 0 )
						break;

					if ( next == -1 )
						next = idx;
					mTakeServiceBit |= BIT( idx );
				}
			}
			if ( i != numSerivce )
			{

			}
		}
		break;
	case TAKE_GOODS_SLOT:

		break;
	}
}

void rcMarket::onMarketService( rcWorker* worker )
{


}

void rcMarket::onCreate( rcCityInfo& cInfo )
{
	BaseClass::onCreate( cInfo );
	WorkerSlot* slot;

	slot = createWorker( cInfo , SERVICE_SLOT , WT_MARKET_TRADER );

	WorkerInfo& info = rcDataManager::Get().getWorkerInfo( slot->worker->getType() );
	RandomMoveMission* mission =  new RandomMoveMission( info.randMoveDist );
	mission->setOnTileCallback( this , &rcMarket::onMarketService );

	slot->worker->changeMission( mission );

	slot = createWorker( cInfo , TAKE_GOODS_SLOT , WT_MARKET_TRADER );
}

rcBuilding* rcBuildingQuery::getBuilding( unsigned buildingID )
{
	if ( buildingID == RC_ERROR_BUILDING_ID )
		return NULL;

	rcBuilding** building = mBTable.getItem( buildingID );
	if ( building ) 
		return *building;

	return NULL;
}
