#ifndef TPhyBody_h__
#define TPhyBody_h__

#define CF_FLY_OBJECT 128

#include "btBulletDynamicsCommon.h"

class TPhyBodyInfo : public btRigidBody::btRigidBodyConstructionInfo
{
public:
	TPhyBodyInfo( float mass, btMotionState* motionState,
		          btCollisionShape* collisionShape, 
				  const btVector3& localInertia = btVector3(0,0,0) );
};

class TPhyBody : public btRigidBody
{
public:
	TPhyBody(TPhyBodyInfo const& info);
	~TPhyBody();

	void  convexSweepTest(const class btConvexShape* castShape, 
		                  const btTransform& convexFromWorld, 
		                  const btTransform& convexToWorld, 
						  btCollisionWorld::ConvexResultCallback& resultCallback, 
						  btScalar allowedCcdPenetration = 0.f) const;

	void  rayTest(const btVector3& rayFromWorld, const btVector3& rayToWorld, 
		          btCollisionWorld::RayResultCallback& resultCallback) const; 

	void  addOverlappingObjectInternal(btBroadphaseProxy* otherProxy, btBroadphaseProxy* thisProxy=0);
	void  removeOverlappingObjectInternal(btBroadphaseProxy* otherProxy,btDispatcher* dispatcher,btBroadphaseProxy* thisProxy=0);

	int	getNumOverlappingObjects() const
	{
		return m_overlappingObjects.size();
	}

	btCollisionObject*	getOverlappingObject(int index)
	{
		return m_overlappingObjects[index];
	}

	const btCollisionObject*	getOverlappingObject(int index) const
	{
		return m_overlappingObjects[index];
	}

	btAlignedObjectArray<btCollisionObject*>&	getOverlappingPairs()
	{
		return m_overlappingObjects;
	}

	const btAlignedObjectArray<btCollisionObject*>	getOverlappingPairs() const
	{
		return m_overlappingObjects;
	}

	btHashedOverlappingPairCache*	getOverlappingPairCache()
	{
		return m_hashPairCache;
	}


	static TPhyBody const* upcast(const btCollisionObject* colObj);
	static TPhyBody*	   upcast(btCollisionObject* colObj);

protected:
	
	btAlignedObjectArray<btCollisionObject*> m_overlappingObjects;
	btHashedOverlappingPairCache*	         m_hashPairCache;
	friend class TPhyBodyPairCallback;
};


class TPhyBodyPairCallback : public btOverlappingPairCallback
{

public:
	TPhyBodyPairCallback()
	{
	}

	virtual ~TPhyBodyPairCallback()
	{

	}

	virtual btBroadphasePair*	addOverlappingPair(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1);
	virtual void*	removeOverlappingPair(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1,btDispatcher* dispatcher);
	virtual void	removeOverlappingPairsContainingProxy(btBroadphaseProxy* proxy0,btDispatcher* dispatcher);

};






#endif // TPhyBody_h__