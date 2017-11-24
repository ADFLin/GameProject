#include "rcPCH.h"
#include "rcWorker.h"

#include "rcWorkerManager.h"

#include "rcPathFinder.h"
#include "rcLevelMap.h"

#include "rcBuilding.h"
#include "rcProductTag.h"

#include "rcGameData.h"
#include "rcWorkerData.h"

#define SQRT_2 1.41421356

extern rcWorkerManager* g_workerManager;

enum
{
	MC_HOUSE_NO_FREE_SPACE = 1 ,
	MC_ERROR_TP_PATH = 2 ,
};

rcWorker::rcWorker() 
	:mManageID( RC_ERROR_WORKER_ID )
	,mOwner( RC_ERROR_BUILDING_ID )
	,mCurMission( NULL )
	,mLastExceptionContent( 0 )
	,mFlag( 0 )
	,mDir( 0 )
	,mHealth( 100 )
	,mMapPos(-1,-1)
	,mOffset(0)
	//,mType( WT_UNKNOWN )
{

}

rcWorker::~rcWorker()
{
	if ( mCurMission )
		mCurMission->onAbort();

	delete mCurMission;
}



void rcWorker::changeMission( rcMission* mission )
{
	assert( mission->mWorker == NULL || mission->mWorker == this );

	if ( mCurMission )
	{
		if ( checkFlag( eEnableMission ) )
		{
			getManager()->sendWorkerMsg( this , WMT_ABORT , 0 );
			mCurMission->onAbort();
		}
		destoryCurMission();
	}
	mCurMission = mission;
	
}

void rcWorker::update( long time )
{
	PROFILE_ENTRY( "rcWorker::update" );
	if ( mCurMission && checkFlag( eEnableMission ) )
	{
		mLastExceptionContent = 0;
		PROFILE_ENTRY( "rcMission::update" );
		mCurMission->onUpdate( time );
	}

	if ( mLastExceptionContent )
	{
		switch( mLastExceptionContent )
		{
		case MC_ERROR_TP_PATH:
		case MC_HOUSE_NO_FREE_SPACE:
			{
				if ( getManager()->tryTransportProduct( this , 
						mBuildingVar.infoTP.pTag ,
						mBuildingVar.infoTP.num ) )
				{
					restartMission();
				}
			}
			break;
		}

		if ( !checkFlag( eEnableMission ) )
		{
			getManager()->sendWorkerMsg( this , WMT_EXCEPTION , mLastExceptionContent );
		}
	}
}

void rcWorker::moveNextTile()
{
	Vec2i newPos = getMapPos() + g_DirOffset8[ mDir.getValue() ];
	setMapPos( newPos );
}

rcWorkerManager* rcWorker::getManager()
{
	return g_workerManager;
}

void rcWorker::sendMissionException( unsigned content )
{
	mLastExceptionContent = content;
	removeFlag( eEnableMission );
	return;
}

void rcWorker::finishMission()
{
	getManager()->sendWorkerMsg( this , WMT_FINISH , 0 );
	removeFlag( eEnableMission );
}

void rcWorker::destoryCurMission()
{
	rcMission* mission = mCurMission;
	mCurMission = NULL;
	removeFlag( eEnableMission );

	delete mission;
}

bool rcWorker::restartMission()
{
	if ( !mCurMission )
		return false;

	mCurMission->mWorker = this;
	if ( !mCurMission->onStart() )
		return false;

	addFlag( eEnableMission );

	return true;
}

void rcWorker::setMapPos( Vec2i const& mapPos )
{
	getManager()->updateWorkerPos( this , mapPos );
}

bool DestionMoveMission::onStart()
{
	if ( mPath == NULL )
	{
		mPath = getWorker()->getManager()->createPath( getWorker()->getMapPos() , mGoalPos );
	}
	else
	{
		getWorker()->setMapPos( mPath->getStartPos() );
	}

	if ( mPath == NULL )
	{
		getWorker()->sendMissionException( MC_ERROR_TP_PATH );
		return false;
	}

	if ( !mNavigator.startRouting( getWorker(), mPath ) )
	{
		getWorker()->finishMission();
		return true;
	}

	return true;
}

DestionMoveMission::~DestionMoveMission()
{
	if ( mPath )
		getWorker()->getManager()->destoryPath( mPath );
}

void DestionMoveMission::onUpdate( long time )
{
	if ( ! mNavigator.updateMoving( time ) )
	{
		getWorker()->finishMission();
		return;
	}
}


DestionMoveMission::DestionMoveMission( rcPath* path ) 
	:mGoalPos( path->getGoalPos() )
	,mPath( path )
{

}

DestionMoveMission::DestionMoveMission( Vec2i const& pos ) 
	:mGoalPos( pos )
	,mPath( NULL )
{

}



void TransportProductMission::onUpdate( long time )
{
	rcBuilding* building = getWorker()->getManager()->getBuilding( mBuildingID );

	if ( building == NULL && mState != MS_GO_HOME )
	{
		getWorker()->sendMissionException( MC_ERROR_TP_PATH );
		return;
	}

	if ( mState == MS_ROTING_DEST )
	{
		if ( !mRouter.updateMoving( time ) )
		{
			if ( getWorker()->getMapPos() != building->getEntryPos() )
			{
				getWorker()->sendMissionException( MC_ERROR_TP_PATH );
				return;
			}
			else
			{
				mState = MS_UNLOAD;
				mCount = 0;
				return;
			}
		}
	}
	else if ( mState == MS_UNLOAD )
	{
		mCount += time;
		if ( mCount > 300 )
		{
			rcWorker::TransportInfo& info = getWorker()->getBuildingVar().infoTP;
			if ( !building->recvGoods( info.pTag , info.num ) )
			{
				getWorker()->sendMissionException( MC_HOUSE_NO_FREE_SPACE );
				return;
			}
			else
			{
				info.pTag = PT_NULL_PRODUCT;
				info.num  = 0;

				rcPath* path = mRouter.getPath();
				path ->reverseRoute();

				if ( !mRouter.startRouting( getWorker() , path ) )
				{
					getWorker()->finishMission();
					return;
				}
				mState = MS_GO_HOME;
				return;
			}
		}
	}
	else if ( mState == MS_GO_HOME )
	{
		if ( !mRouter.updateMoving( time ) )
		{
			getWorker()->finishMission();
			return;
		}
	}
}

bool TransportProductMission::onStart()
{
	//set goal frist

	rcWorkerManager* manager = getWorker()->getManager();
	Vec2i goalPos = manager->getBuilding( mBuildingID )->getEntryPos();
	rcPath* path = manager->createPath( getWorker()->getMapPos() , goalPos );

	if ( path == NULL )
	{
		getWorker()->sendMissionException( MC_ERROR_TP_PATH );
		return false;
	}

	if ( !mRouter.startRouting( getWorker() , path ) )
	{
		mState = MS_UNLOAD;
	}
	else mState = MS_ROTING_DEST;

	return true;
}

TransportProductMission::~TransportProductMission()
{
	getWorker()->getManager()->destoryPath( mRouter.getPath() );
}

bool PathNavigator::updateMoving( long time )
{
	mDistMove += mSpeed * time;

	while( mDistMove > mDistNextTile )
	{
		mWorker->moveNextTile();
		mDistNextTile += mbIsometric ? SQRT_2 : 1.0f;


		if ( mDistNextTile > mDistNextRoute )
			break;
	}

	if ( mDistMove > mDistNextRoute )
	{
		mWorker->setMapOffset( 0 );

		if ( onTile )
			onTile( mWorker );

		if ( !refreshRoute() )
			return false;
	}
	else
	{
		float offset = 0.5f;
		if ( mbIsometric )
			offset -= ( mDistNextTile - mDistMove )/ SQRT_2;
		else
			offset -= ( mDistNextTile - mDistMove );
		mWorker->setMapOffset( offset );
	}
	return true;
}

bool PathNavigator::refreshRoute()
{
	if ( !mPath->getNextRoutePos( mCookie , mNextRoutePos ) )
		return false;

	Vec2i workerPos = mWorker->getMapPos();
	Vec2i dif = mNextRoutePos - workerPos;

	mDistMove = 0;
	mDistNextRoute = abs( dif.x ) + abs( dif.y );
	mDistNextTile = 0.5;
	mbIsometric = ( dif.x && dif.y );
	assert( !mbIsometric ||( abs( dif.x ) == abs( dif.y ) ) );

	mWorker->setDir( rcDirection( calcDirection( dif ) ) );

	if ( mbIsometric )
	{
		mDistNextRoute *= 0.5 * SQRT_2;
		mDistNextTile  = SQRT_2;
	}

	return true;
}

int PathNavigator::calcDirection( Vec2i const& v )
{
	Vec2i d = v;
	if ( d.x )
		d.x /= abs( d.x );
	if ( d.y )
		d.y /= abs( d.y );

	int s = d.x + d.y;
	switch( s )
	{
	case  0: return ( d.x > 0 ) ? 3 : 7;
	case  1: return ( d.x != 0 ) ? 0 : 2;
	case -1: return ( d.x != 0 ) ? 4 : 6;
	case  2: return 5;
	case -2: return 1;
	}

	assert( 0 );
	return 0;
}

bool PathNavigator::startRouting( rcWorker* worker , rcPath* path )
{
	setWorker( worker );
	mPath   = path;
	mPath->startRouting( mCookie );

	return refreshRoute();
}

int RandMoveNavigator::evalNextDir()
{
	rcLevelMap* levelMap = mWorker->getManager()->getLevelMap();

	int passibleDir[4];
	int numDir = 0;
	for( int i = 0 ; i < 4 ; ++i )
	{
		Vec2i pos = mWorker->getMapPos() + g_OffsetDir[i];
		if ( !levelMap->isRange( pos ) )
			continue;

		rcMapData& data = levelMap->getData( pos );
		if ( data.haveRoad() )
		{
			passibleDir[ numDir++ ] = i;
		}
	}

	switch( numDir )
	{
	case 0: return -1;
	case 1: return passibleDir[0];
	case 2:
		if ( ( ( passibleDir[0] + 2 ) % 4 ) == mCurDir  )
			return passibleDir[1];
		else
			return passibleDir[0];
	case 3:
	case 4:
		int idx = rand() % numDir;
		if ( ( ( passibleDir[idx] + 2 ) % 4 ) == mCurDir )
			idx = ( idx + 1 ) % numDir;
		return passibleDir[ idx ];
	}

	return -1;
}

bool RandMoveNavigator::refreshRoute()
{
	int nextDir = evalNextDir();
	if ( nextDir == -1 )
		return false;

	if ( nextDir != mCurDir )
	{
		mWorker->setDir( rcDirection( 2 * nextDir ) );
		mCurDir = nextDir;
	}

	mDistMove = 0;
	mDistNextRoute = 1.0;
	mDistNextTile = 0.5f;

	return true;
}

bool RandMoveNavigator::updateMoving( long time )
{
	mDistMove += mSpeed * time;

	while( mDistMove > mDistNextTile )
	{
		mWorker->moveNextTile();
		mDistNextTile += 1.0f;

		if ( mDistNextTile > mDistNextRoute )
			break;
	}

	if ( mDistMove > mDistNextRoute )
	{
		mTotalDistMove += 1.0f;
		mWorker->setMapOffset( 0 );

		if ( onTile )
			onTile( mWorker );

		if ( !refreshRoute() )
			return false;
	}
	else
	{
		float offset = 0.5f;
		offset -= ( mDistNextTile - mDistMove );
		mWorker->setMapOffset( offset );
	}
	return true;
}

bool RandMoveNavigator::startRouting( rcWorker* worker )
{
	setWorker( worker );

	mCurDir = -1;
	mTotalDistMove = 0;

	return refreshRoute();
}

void RandomMoveMission::onUpdate( long time )
{
	if( mState == MS_ROTING )
	{
		if ( !mRandNavigator.updateMoving( time )  ||
			  mRandNavigator.getTotalMoveDistance() >= mMaxDist )
		{
			rcPath* path = getWorker()->getManager()->createPath( 
				getWorker()->getMapPos() , getWorker()->getOwner() );

			if ( path == NULL )
			{
				getWorker()->sendMissionException( 0 );
				return;
			}
			if ( mPathNavigator.getPath() )
				getWorker()->getManager()->destoryPath( mPathNavigator.getPath() );

			if ( !mPathNavigator.startRouting( getWorker() , path ) )
			{
				getWorker()->finishMission();
				return;
			}

			mState = MS_GO_HOME;
			return;
		}
	}
	else if ( mState == MS_GO_HOME )
	{
		if ( !mPathNavigator.updateMoving( time ) )
		{
			getWorker()->finishMission();
			return;
		}
	}
}

bool RandomMoveMission::onStart()
{
	assert( getWorker() );

	if ( !mRandNavigator.startRouting( getWorker() ) )
		return false;

	mState = MS_ROTING;
	return true;
}

void Navigator::setWorker( rcWorker* worker )
{
	mWorker = worker;
	WorkerInfo& info = rcDataManager::getInstance().getWorkerInfo( worker->getType() );
	mSpeed = 1.0f / info.moveSpeed;
}
