#ifndef PhysicsPrivate_h__
#define PhysicsPrivate_h__

#include "common.h"
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"


typedef btCollisionWorld::ClosestConvexResultCallback ClosestConvexResultCallback;
typedef btCollisionWorld::ClosestRayResultCallback    ClosestRayResultCallback;
typedef btCollisionWorld::LocalRayResult              LocalRayResult;
typedef btCollisionWorld::LocalConvexResult           LocalConvexResult;

float const g_GraphicScale = 10.0f;

class CollisionDetector
{
public:
	typedef btAlignedObjectArray<btCollisionObject*> CollisionObjectArray;

	CollisionDetector( btCollisionObject* obj );
	~CollisionDetector();

	int	getNumOverlappingObjects() const  {  return mOverlappingObjects.size();  }

	btCollisionObject*        getOverlappingObject(int index)       {  return mOverlappingObjects[index];  }
	btCollisionObject const*  getOverlappingObject(int index) const {  return mOverlappingObjects[index]; }

	CollisionObjectArray&	     getOverlappingPairs()       {  return mOverlappingObjects;  }
	CollisionObjectArray const&  getOverlappingPairs() const {  return mOverlappingObjects;  }

	btHashedOverlappingPairCache*	getOverlappingPairCache(){  return mHashPairCache; }

	void  convexSweepTest(
		const btConvexShape* castShape, 
		const btTransform& convexFromWorld, 
		const btTransform& convexToWorld,
		btCollisionWorld::ConvexResultCallback& resultCallback, 
		btScalar allowedCcdPenetration ) const;

	void  rayTest(const btVector3& rayFromWorld, const btVector3& rayToWorld, 
		btCollisionWorld::RayResultCallback& resultCallback) const;

	void  addOverlappingObject(btBroadphaseProxy* otherProxy, btBroadphaseProxy* thisProxy );
	void  removeOverlappingObject(btBroadphaseProxy* otherProxy,btDispatcher* dispatcher,btBroadphaseProxy* thisProxy );
	void  processCollision( btDynamicsWorld* world );

	btCollisionObject* getCollisionObj(){ return mObject; }

	void reset()
	{
		mOverlappingObjects.clear();

		mHashPairCache->~btHashedOverlappingPairCache();
		btAlignedFree( mHashPairCache );

		mHashPairCache = new (btAlignedAlloc(sizeof(btHashedOverlappingPairCache),16)) btHashedOverlappingPairCache();
	}
private:

	btCollisionObject*             mObject;
	CollisionObjectArray           mOverlappingObjects;
	btHashedOverlappingPairCache*  mHashPairCache;
	friend class CollisionDetectorCallback;
};


inline Vec3D     convBullet2CF( btVector3 const& v )
{
	return Vec3D( v.x() , v.y() , v.z() );
}
inline btVector3 convCF2Bullet( Vec3D const& v )
{
	return btVector3( v.x , v.y , v.z );
}

inline Quat     convBullet2CF( btQuaternion const& q )
{
	return Quat( q.x() , q.y() , q.z() , q.w() );
}

inline btQuaternion convCF2Bullet( Quat const& q )
{
	return btQuaternion( q.x , q.y , q.z , q.w );
}

#endif // PhysicsPrivate_h__