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

	int	getNumOverlappingObjects() const  {  return m_overlappingObjects.size();  }

	btCollisionObject*        getOverlappingObject(int index)       {  return m_overlappingObjects[index];  }
	btCollisionObject const*  getOverlappingObject(int index) const {  return m_overlappingObjects[index]; }

	CollisionObjectArray&	     getOverlappingPairs()       {  return m_overlappingObjects;  }
	CollisionObjectArray const&  getOverlappingPairs() const {  return m_overlappingObjects;  }

	btHashedOverlappingPairCache*	getOverlappingPairCache(){  return m_hashPairCache; }

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

	void clear()
	{
		m_overlappingObjects.clear();

		m_hashPairCache->~btHashedOverlappingPairCache();
		btAlignedFree( m_hashPairCache );

		m_hashPairCache = new (btAlignedAlloc(sizeof(btHashedOverlappingPairCache),16)) btHashedOverlappingPairCache();
	}
private:

	btCollisionObject*             mObject;
	CollisionObjectArray           m_overlappingObjects;
	btHashedOverlappingPairCache*  m_hashPairCache;
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