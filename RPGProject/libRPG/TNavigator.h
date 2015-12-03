#ifndef TNavigator_h__
#define TNavigator_h__

#include "common.h"

#include "NavMesh.h"
#include "GameObjectComponent.h"


class ISpatialComponent;

enum NavGoalType
{
	NG_LOCATION,
	NG_ENTITY,
};

struct NavGoalInfo
{
	NavGoalInfo()
	{
		entity = nullptr;
		spatialComp = nullptr;
	}
	NavGoalType navType;
	Vec3D       pos;
	float       offset;

	EHandle     entity;
	ISpatialComponent* spatialComp;
};

enum NavigatorStats
{
	NAV_NO_FIND_PATH ,
	NAV_MOVE_FINISH ,
	NAV_NO_NAVMESH  ,
	NAV_BLOCK ,
	NAV_MOVING ,
};



class TNavigator
{
public:
	TNavigator();
	void     performMovement();

	void     setGoalEntity( ILevelObject* entity , float offset );
	void     setGoalPos   ( Vec3D const& pos , float offset );
	
	ILevelObject* getOuter(){ return mObjComp; }
	NavPath const& getRoutingPath(){ return mRoutingPath; }

	void        cancelRoute()
	{	
		if ( mNavStats == NAV_MOVING )
			mNavStats = NAV_MOVE_FINISH; 
	}

	ILevelObject* getTarget(){  return goalInfo.entity; }
	NavigatorStats        getNavStats() const { return mNavStats; }
	void                  updateEntityPos( Vec3D const& pos );
	void                  setNavMesh( NavMesh* mesh ){ mNavMesh = mesh; }
	float                 getOuterOffset() const { return mOuterOffset; }

	void setupOuter( ILevelObject* logicComp );

protected:

	bool setNextWayPoint();
	void buildPath();

	NavMesh*        mNavMesh;
	NavPath         mRoutingPath;
	Waypoint*       mCurWaypoint;
	NavPath::Cookie mPathCookie;
	NavigatorStats  mNavStats;

	float          mMaxRotationAngle;
	float          mMaxMoveSpeed;
	Vec3D          mLocalFaceDir;
	float          mOuterOffset;

	ISpatialComponent*  mSpatialComp;
	ILevelObject*       mObjComp;


	float         blockOffsetAngle;
	NavGoalInfo   goalInfo;


	bool findPath( Vec3D const& pos );
};


#endif // TNavigator_h__