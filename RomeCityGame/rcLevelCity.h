#ifndef rcLevelCity_h__
#define rcLevelCity_h__

#include "rcBase.h"
#include "rcCityInfo.h"

#include "TGrid2D.h"

class RegionManager;

class rcLevelMap;
class rcLevelScene;
class rcBuilding;
class rcBasicBuilding;
class rcBuildingManager;
class rcWorkerManager;
class rcPath;
class rcPathFinder;

enum WorkerMsg;

class rcLevelCity
{
public:
	rcLevelCity();

	~rcLevelCity();

	bool init();
	void setupMap( Vec2i const& size );
	void update( long time );

	void render();

	bool buildContruction( Vec2i const& pos , unsigned tagID , int idxModel , bool swapAxis );
	void destoryContruction( Vec2i const& pos );


	rcBuilding* tryTransportProduct( rcWorker* worker , unsigned pTag, int numProduct );
	
	void      updateRoadLink( Vec2i const& pos );
	void      updateAquaductLink( Vec2i const& pos );
	


	int       obtainLabor( Vec2i const& pos , int wantNum );


	bool      checkRoadAquaductConnect( Vec2i const& pos , unsigned tagID , int& allowDir );


	void      updateWaterSource( Vec2i const& pos , bool waterState , bool reset = true );

	rcPath*   createPath( Vec2i const& from , Vec2i const& to );
	rcPath*   createPath( Vec2i const& from , unsigned blgID );

	void     updateSettler( long time );
	void     sendWorkerMsg( rcWorker* worker , unsigned blgID , WorkerMsg msg , unsigned content );

public:

	rcLevelMap&        getLevelMap(){ return *mLevelMap; }
	rcWorkerManager&   getWorkerManager(){ return *mWorkerMgr; }
	rcBuildingManager& getBuildingManager(){ return *mBuildingMgr; }

	rcBuilding* getBuilding( unsigned blgID );
	rcBuilding* getBuilding( Vec2i const& pos );
	rcBuilding* tryGetProduct( rcWorker* worker , unsigned pTag, int numProduct );

	TPtrHolder< rcLevelMap >        mLevelMap;
	TPtrHolder< rcWorkerManager >   mWorkerMgr;
	TPtrHolder< rcBuildingManager > mBuildingMgr;
	TPtrHolder< rcLevelScene >      mLevelScene;
	TPtrHolder< rcPathFinder >      mPathFinder;
	TPtrHolder< RegionManager >     mRegionMgr;

	int  mProductSaveNum[ 32 ];
	int  mNumSettler;

	rcCityInfo mInfo;
};


#endif // rcLevelCity_h__