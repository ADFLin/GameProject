#ifndef rcCityInfo_h__
#define rcCityInfo_h__

#include "rcBase.h"

#include "TGrid2D.h"

class rcLevelMap;
class rcWorkerManager;
class rcBuildingQuery;

class rcCityInfo
{
public:
	rcCityInfo();
	rcLevelMap*      levelMap;
	rcWorkerManager* workerMgr;
	rcBuildingQuery* blgQuery;

	rcBuilding* getBuilding( Vec2i const& pos );
	rcBuilding* getBuilding( unsigned blgID );
	
	rcWorker*   createWorker( unsigned typeTag );
	void        destoryWorker( rcWorker* worker );
	void        updateWaterSource( Vec2i const& pos , bool waterState , bool reset = true );
	void        setLevelMap( rcLevelMap* map );

private:

	void  updateWaterSourceRecursive( int x , int y , int count , bool waterState );
	bool  updateResviorWaterState( rcBuilding& building , int x , int y , int count , bool waterState );
	TGrid2D< char > mCheckData;
};

#endif // rcCityInfo_h__
