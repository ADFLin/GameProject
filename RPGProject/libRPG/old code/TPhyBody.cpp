#include "TPhyBody.h"
#include "btBulletDynamicsCommon.h"
#include "common.h"
#include "profile.h"


TPhyBodyInfo::TPhyBodyInfo( float mass, btMotionState* motionState, btCollisionShape* collisionShape, const btVector3& localInertia /*= btVector3(0,0,0) */ ) 
	: btRigidBody::btRigidBodyConstructionInfo( mass , motionState , collisionShape , localInertia )
{
	m_angularDamping = 0.3;
	m_linearSleepingThreshold *= g_GraphicScale;
	m_angularSleepingThreshold = 1.1;
	m_restitution = 0.1;
}

TPhyBody::TPhyBody( TPhyBodyInfo const& info ) 
	:btRigidBody( info )
{	
	setCollisionFlags( getCollisionFlags() | CF_FLY_OBJECT );
	m_hashPairCache = new (btAlignedAlloc(sizeof(btHashedOverlappingPairCache),16)) btHashedOverlappingPairCache();
}

TPhyBody::~TPhyBody()
{
	m_hashPairCache->~btHashedOverlappingPairCache();
	btAlignedFree( m_hashPairCache );
}

TPhyBody const* TPhyBody::upcast( const btCollisionObject* colObj )
{
	if ( colObj->getCollisionFlags() & CF_FLY_OBJECT )
		return (const TPhyBody*)colObj;
	return 0;
}

TPhyBody* TPhyBody::upcast( btCollisionObject* colObj )
{
	if ( colObj->getCollisionFlags() & CF_FLY_OBJECT )
		return (TPhyBody*)colObj;
	return 0;
}

void TPhyBody::addOverlappingObjectInternal(btBroadphaseProxy* otherProxy,btBroadphaseProxy* thisProxy)
{
	btBroadphaseProxy*actualThisProxy = thisProxy ? thisProxy : getBroadphaseHandle();
	btAssert(actualThisProxy);

	btCollisionObject* otherObject = (btCollisionObject*)otherProxy->m_clientObject;
	btAssert(otherObject);
	int index = m_overlappingObjects.findLinearSearch(otherObject);
	if (index==m_overlappingObjects.size())
	{
		m_overlappingObjects.push_back(otherObject);
		m_hashPairCache->addOverlappingPair(actualThisProxy,otherProxy);
	}
}

void TPhyBody::removeOverlappingObjectInternal(btBroadphaseProxy* otherProxy,btDispatcher* dispatcher,btBroadphaseProxy* thisProxy1)
{
	btCollisionObject* otherObject = (btCollisionObject*)otherProxy->m_clientObject;
	btBroadphaseProxy* actualThisProxy = thisProxy1 ? thisProxy1 : getBroadphaseHandle();
	btAssert(actualThisProxy);

	btAssert(otherObject);
	int index = m_overlappingObjects.findLinearSearch(otherObject);
	if (index<m_overlappingObjects.size())
	{
		m_overlappingObjects[index] = m_overlappingObjects[m_overlappingObjects.size()-1];
		m_overlappingObjects.pop_back();
		m_hashPairCache->removeOverlappingPair(actualThisProxy,otherProxy,dispatcher);
	}
}


void	TPhyBody::convexSweepTest(const btConvexShape* castShape, 
								  const btTransform& convexFromWorld, 
								  const btTransform& convexToWorld, 
								  btCollisionWorld::ConvexResultCallback& resultCallback, 
								  btScalar allowedCcdPenetration) const
{
	btTransform	convexFromTrans,convexToTrans;
	convexFromTrans = convexFromWorld;
	convexToTrans = convexToWorld;
	btVector3 castShapeAabbMin, castShapeAabbMax;
	/* Compute AABB that encompasses angular movement */
	{
		btVector3 linVel, angVel;
		btTransformUtil::calculateVelocity (convexFromTrans, convexToTrans, 1.0, linVel, angVel);
		btTransform R;
		R.setIdentity ();
		R.setRotation (convexFromTrans.getRotation());
		castShape->calculateTemporalAabb (R, linVel, angVel, 1.0, castShapeAabbMin, castShapeAabbMax);
	}

	/// go over all objects, and if the ray intersects their aabb + cast shape aabb,
	// do a ray-shape query using convexCaster (CCD)
	int i;
	for (i=0;i<m_overlappingObjects.size();i++)
	{
		btCollisionObject*	collisionObject= m_overlappingObjects[i];
		//only perform raycast if filterMask matches
		if(resultCallback.needsCollision(collisionObject->getBroadphaseHandle())) {
			//RigidcollisionObject* collisionObject = ctrl->GetRigidcollisionObject();
			btVector3 collisionObjectAabbMin,collisionObjectAabbMax;
			collisionObject->getCollisionShape()->getAabb(collisionObject->getWorldTransform(),collisionObjectAabbMin,collisionObjectAabbMax);
			AabbExpand (collisionObjectAabbMin, collisionObjectAabbMax, castShapeAabbMin, castShapeAabbMax);
			btScalar hitLambda = btScalar(1.); //could use resultCallback.m_closestHitFraction, but needs testing
			btVector3 hitNormal;
			if (btRayAabb(convexFromWorld.getOrigin(),convexToWorld.getOrigin(),collisionObjectAabbMin,collisionObjectAabbMax,hitLambda,hitNormal))
			{
				btCollisionWorld::objectQuerySingle(
					castShape, convexFromTrans,convexToTrans,
					collisionObject,
					collisionObject->getCollisionShape(),
					collisionObject->getWorldTransform(),
					resultCallback,
					allowedCcdPenetration);
			}
		}
	}

}

void  TPhyBody::rayTest(const btVector3& rayFromWorld, const btVector3& rayToWorld, 
						btCollisionWorld::RayResultCallback& resultCallback) const
{
	btTransform rayFromTrans;
	rayFromTrans.setIdentity();
	rayFromTrans.setOrigin(rayFromWorld);
	btTransform  rayToTrans;
	rayToTrans.setIdentity();
	rayToTrans.setOrigin(rayToWorld);


	int i;
	for (i=0;i<m_overlappingObjects.size();i++)
	{
		btCollisionObject*	collisionObject= m_overlappingObjects[i];
		//only perform raycast if filterMask matches
		if( resultCallback.needsCollision(collisionObject->getBroadphaseHandle()) ) 
		{
			btCollisionWorld::rayTestSingle(rayFromTrans,rayToTrans,
				collisionObject,
				collisionObject->getCollisionShape(),
				collisionObject->getWorldTransform(),
				resultCallback);
		}
	}
}

btBroadphasePair* TPhyBodyPairCallback::addOverlappingPair( btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1 )
{
	//TPROFILE("TPhyBodyPairCallback");
	btCollisionObject* colObj0 = (btCollisionObject*) proxy0->m_clientObject;
	btCollisionObject* colObj1 = (btCollisionObject*) proxy1->m_clientObject;

	TPhyBody* ghost0 = 	TPhyBody::upcast(colObj0);
	TPhyBody* ghost1 = 	TPhyBody::upcast(colObj1);
	if (ghost0)
		ghost0->addOverlappingObjectInternal(proxy1, proxy0);
	if (ghost1)
		ghost1->addOverlappingObjectInternal(proxy0, proxy1);
	return 0;
}

void* TPhyBodyPairCallback::removeOverlappingPair( btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1,btDispatcher* dispatcher )
{
	//TPROFILE("TPhyBodyPairCallback");
	btCollisionObject* colObj0 = (btCollisionObject*) proxy0->m_clientObject;
	btCollisionObject* colObj1 = (btCollisionObject*) proxy1->m_clientObject;
	TPhyBody* ghost0 = 	TPhyBody::upcast(colObj0);
	TPhyBody* ghost1 = 	TPhyBody::upcast(colObj1);
	if (ghost0)
		ghost0->removeOverlappingObjectInternal(proxy1,dispatcher,proxy0);
	if (ghost1)
		ghost1->removeOverlappingObjectInternal(proxy0,dispatcher,proxy1);
	return 0;
}

void TPhyBodyPairCallback::removeOverlappingPairsContainingProxy( btBroadphaseProxy* proxy0 , btDispatcher* dispatcher )
{
	return;

	btCollisionObject* colObj0 = (btCollisionObject*) proxy0->m_clientObject;
	TPhyBody* ghost0 = 	TPhyBody::upcast(colObj0);
	if (ghost0)
		ghost0->m_hashPairCache->removeOverlappingPairsContainingProxy(proxy0,dispatcher);
	//btAssert(0);
	//need to keep track of all ghost objects and call them here
	//m_hashPairCache->removeOverlappingPairsContainingProxy(proxy0,dispatcher);
}
