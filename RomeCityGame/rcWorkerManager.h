#ifndef rcWorkerManager_h__
#define rcWorkerManager_h__

#include "rcBase.h"

#include "DataStructure/TTable.h"

class rcPath;
class rcLevelCity;

enum  WorkerMsg;

class rcWorkerManager
{
public:
	rcWorkerManager();

	rcWorker* createWorker( unsigned typeTag );
	void      destoryWorker( rcWorker* worker );

	void   removeWorkerLink( rcWorker* worker , Vec2i const& mapPos );
	void   updateWorkerPos( rcWorker* worker , Vec2i const& newMapPos );
	void    destoryPath( rcPath* path );
	rcPath* createPath( Vec2i const& from , Vec2i const& to );
	rcPath* createPath( Vec2i const& from , unsigned blgID );

	rcBuilding* tryTransportProduct( rcWorker* worker , unsigned pTag, int numProduct );
	void    update( long time );

	rcBuilding* getBuilding( unsigned blgID );

	void       sendWorkerMsg( rcWorker* worker , WorkerMsg msg , unsigned content );
	rcLevelMap* getLevelMap();

	typedef TTable< rcWorker* > WorkerTable;
	WorkerTable   mWorkerTable;
	void  setLevelCity( rcLevelCity* city ){ mLevelCity = city; }
private:
	rcLevelCity* getLevelCity(){ return mLevelCity; }
	rcWorker*     mOutMapWorkerList;
	rcLevelCity*  mLevelCity;

};
#endif // rcWorkerManager_h__