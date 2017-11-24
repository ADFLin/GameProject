#include "ProjectPCH.h"
#include "TNavigator.h"

#include "CActor.h"
#include "SpatialComponent.h"

#include "UtilityMath.h"

TNavigator::TNavigator()
{
	mNavMesh     = nullptr;
	mCurWaypoint = nullptr;
	mNavStats = NAV_MOVE_FINISH;
}

void TNavigator::performMovement()
{
	if ( mNavStats == NAV_MOVE_FINISH ||
		mNavStats == NAV_NO_NAVMESH )
		return;

	if ( mNavStats == NAV_NO_FIND_PATH )
	{
		buildPath();
		return;
	}

	if ( goalInfo.entity )
	{
		updateEntityPos( goalInfo.spatialComp->getPosition() );
	}

	float goalDist;

	Vec3D curPos = mSpatialComp->getPosition();

	switch( goalInfo.navType )
	{
	case NG_ENTITY:
		goalDist = Math::Distance( goalInfo.spatialComp->getPosition() , curPos );
		break;
	case NG_LOCATION:
		goalDist = Math::Distance( goalInfo.pos , curPos );
		break;
	default:
		goalDist = 0;
	}
	if ( goalDist < goalInfo.offset )
	{
		mNavStats = NAV_MOVE_FINISH;
		return;
	}

	Vec3D offset = mCurWaypoint->pos - curPos;
	float maxOffset = mMaxMoveSpeed * g_GlobalVal.frameTime;

	float dist = sqrt( offset.x*offset.x + offset.y * offset.y );

	if ( maxOffset > dist && offset.z < 100 )
	{
		setNextWayPoint();
	}
	else
	{
		dist = maxOffset;
	}

	float angle = 0;

	Vec3D faceDir = Math::TransformVector( mLocalFaceDir , mSpatialComp->getTransform() );


	if ( dist > 1e-8 )
	{
		offset[2] = 0;
		offset.normalize();

		angle = UM_Angle( offset , faceDir );
	}


	//FIXME
	CActor* actorComp = static_cast< CActor* >( mObjComp );

	if ( fabs( angle ) > Math::Deg2Rad( 10 ) )
	{

		if ( offset.cross( faceDir ).z < 0 )
			angle = -TMin( mMaxRotationAngle , angle );
		else
			angle =  TMin( mMaxRotationAngle , angle );

		actorComp->turnRight( angle );
	}
	else
	{
		if ( dist > 10 )
		{
			actorComp->setFaceDir( offset );
		}
		actorComp->moveFront( dist / g_GlobalVal.frameTime );
	}
}


bool TNavigator::findPath( Vec3D const& pos )
{
	PROFILE_ENTRY("FindPath");

	if ( !mNavMesh )
	{
		mNavStats = NAV_NO_NAVMESH;
		return false;
	}

	Vec3D ext( 50 , 50 , 100 );
	Vec3D startPos =  mSpatialComp->getPosition() - Vec3D( 0,0, 40 );
	Vec3D endPos   =  pos;

	if ( !mNavMesh->findPath( mRoutingPath , startPos , endPos , ext ) ) 
	{
		mNavStats = NAV_NO_FIND_PATH;
		return false;
	}

	mRoutingPath.startRoute( mPathCookie );
	mCurWaypoint = mRoutingPath.nextRoutePoint( mPathCookie );

	mNavStats = NAV_MOVING;
	return true;
}

void TNavigator::setGoalEntity( ILevelObject* entity , float offset )
{
	assert( entity );

	goalInfo.navType = NG_ENTITY;
	goalInfo.entity  = entity;
	goalInfo.spatialComp = static_cast< ISpatialComponent* >( entity->getSpatialComponent() );
	goalInfo.pos    = goalInfo.spatialComp->getPosition();
	goalInfo.offset = mOuterOffset + offset;

	buildPath();
}

bool TNavigator::setNextWayPoint()
{
	mCurWaypoint = mRoutingPath.nextRoutePoint( mPathCookie );

	if ( !mCurWaypoint )
	{
		mNavStats = NAV_MOVE_FINISH;
		return false;
	}
	return true;
}


void TNavigator::updateEntityPos( Vec3D const& pos )
{
	assert(  goalInfo.navType == NG_ENTITY );


	Waypoint const* goal = mRoutingPath.getGoalWaypoint();

	if ( !goal )
		return;

	float dist = Math::DistanceSqure( goalInfo.pos , pos );

	float const minRebulidPathDist = 20;

	Vec3D ext( 50 , 50 , 100 );

	if ( dist >  minRebulidPathDist ||
		!mNavMesh->updateGoalPos( mRoutingPath , pos , ext ) )
	{
		buildPath();
	}
	else
	{
		mNavStats = NAV_MOVING;
	}
}

void TNavigator::setGoalPos( Vec3D const& pos , float offset )
{
	goalInfo.navType = NG_LOCATION;
	goalInfo.entity  = nullptr;
	goalInfo.pos = pos;
	goalInfo.offset =  mOuterOffset + offset;

	buildPath();
}

void TNavigator::buildPath()
{
	switch( goalInfo.navType )
	{
	case NG_ENTITY:
		findPath( goalInfo.spatialComp->getPosition() );
		break;
	case NG_LOCATION:
		findPath( goalInfo.pos );
		break;
	}
}


void TNavigator::setupOuter( ILevelObject* logicComp )
{
	mSpatialComp = static_cast< ISpatialComponent* >( logicComp->getSpatialComponent() ) ;
	assert( mSpatialComp );
	mObjComp = logicComp;

	CActor* actorLogicComp = CActor::downCast( mObjComp );
	if ( actorLogicComp )
	{
		mMaxMoveSpeed     = actorLogicComp->getMaxMoveSpeed();
		mMaxRotationAngle = actorLogicComp->getMaxRotateAngle();
		mOuterOffset      = actorLogicComp->getFaceOffest();
		mLocalFaceDir.setValue( 0 , -1 , 0 );
	}
	else
	{
		mMaxMoveSpeed = 20;
		mMaxRotationAngle = Math::Deg2Rad( 180 );
		mOuterOffset = 0;
		mLocalFaceDir.setValue( 1,0,0 );
	}
}
