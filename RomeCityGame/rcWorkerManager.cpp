#include "rcPCH.h"
#include "rcWorkerManager.h"

#include "rcWorker.h"
#include "rcBuilding.h"
#include "rcLevelMap.h"

#include "rcLevelCity.h"

rcWorkerManager* g_workerManager = NULL;

rcWorkerManager::rcWorkerManager()
	:mOutMapWorkerList(NULL)
{
	g_workerManager = this;
	mWorkerTable.reserve( 2048 );
}

rcPath* rcWorkerManager::createPath( Vec2i const& from , Vec2i const& to )
{
	return mLevelCity->createPath( from , to );

}

rcPath* rcWorkerManager::createPath( Vec2i const& from , unsigned blgID )
{
	return mLevelCity->createPath( from , blgID );

}


void rcWorkerManager::update( long time )
{
	for( WorkerTable::iterator iter = mWorkerTable.begin();
		iter != mWorkerTable.end() ; ++iter )
	{
		rcWorker* worker = *iter;
		worker->update( time );
	}
}

void rcWorkerManager::removeWorkerLink( rcWorker* worker , Vec2i const& mapPos )
{
	if ( worker->getPrevLink() == NULL )
	{
		if ( !getLevelMap()->isVaildRange( mapPos.x , mapPos.y ) )
		{
			mOutMapWorkerList = worker->getNextLink();
		}
		else
		{
			rcMapData& data = getLevelMap()->getData( mapPos );
			data.workerList = worker->getNextLink();
		}
	}
	worker->unlink();
}

rcWorker* rcWorkerManager::createWorker( unsigned typeTag )
{
	rcWorker* worker = new rcWorker;

	if ( mOutMapWorkerList )
		worker->insertFront( mOutMapWorkerList );

	mOutMapWorkerList = worker;

	worker->mManageID = mWorkerTable.insert( worker );
	worker->mType     = typeTag;

	return worker;
}

void rcWorkerManager::destoryWorker( rcWorker* worker )
{
	if ( !mWorkerTable.remove( worker->mManageID ) )
	{
		return;
	}

	removeWorkerLink( worker , worker->getMapPos() );
	delete worker;

	return;
}

void rcWorkerManager::updateWorkerPos( rcWorker* worker , Vec2i const& newMapPos )
{
	Vec2i mapPos = worker->getMapPos();

	if ( newMapPos != mapPos )
	{
		removeWorkerLink( worker , mapPos );

		if ( !getLevelMap()->isVaildRange( newMapPos.x , newMapPos.y ) )
		{
			if ( mOutMapWorkerList )
				worker->insertFront( mOutMapWorkerList );
			mOutMapWorkerList = worker;
		}
		else
		{
			rcMapData& data = getLevelMap()->getData( newMapPos );

			if ( data.workerList )
				worker->insertFront( data.workerList );
			data.workerList = worker;
		}
	}

	worker->mMapPos = newMapPos;
}

void rcWorkerManager::destoryPath( rcPath* path )
{
	delete path;
}

void rcWorkerManager::sendWorkerMsg( rcWorker* worker , WorkerMsg msg , unsigned content )
{
	if ( worker->mOwner == RC_ERROR_BUILDING_ID )
		return;

	getLevelCity()->sendWorkerMsg( worker , worker->mOwner , msg , content );
}

rcBuilding* rcWorkerManager::getBuilding( unsigned blgID )
{
	return getLevelCity()->getBuilding( blgID );
}

rcLevelMap* rcWorkerManager::getLevelMap()
{
	return &getLevelCity()->getLevelMap();
}

rcBuilding* rcWorkerManager::tryTransportProduct( rcWorker* worker , unsigned pTag, int numProduct )
{
	return getLevelCity()->tryTransportProduct( worker , pTag , numProduct );
}

