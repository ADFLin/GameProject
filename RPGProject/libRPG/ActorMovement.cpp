#include "ProjectPCH.h"
#include "ActorMovement.h"

#include "PhysicsPrivate.h"

#include "LinearMath/btIDebugDraw.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletCollision/CollisionShapes/btMultiSphereShape.h"
#include "BulletCollision/BroadphaseCollision/btOverlappingPairCache.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "LinearMath/btDefaultMotionState.h"


// static helper method
static btVector3 getNormalizedVector(const btVector3& v)
{
	btVector3 n = v.normalized();
	if (n.length() < SIMD_EPSILON) 
	{
		n.setValue(0, 0, 0);
	}
	return n;
}


///@todo Intersect with dynamic objects,
///Ride kinematicly animated platforms properly
///More realistic (or maybe just a config option) falling
/// -> Should integrate falling velocity manually and use that in stepDown()
///Support jumping
///Support ducking
class btKinematicClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
{
public:
	btKinematicClosestNotMeRayResultCallback (btCollisionObject* me) : btCollisionWorld::ClosestRayResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0))
	{
		m_me = me;
	}

	virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace)
	{
		if (rayResult.m_collisionObject == m_me)
			return 1.0;

		return ClosestRayResultCallback::addSingleResult (rayResult, normalInWorldSpace);
	}
protected:
	btCollisionObject* m_me;
};

class btKinematicClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
	btKinematicClosestNotMeConvexResultCallback (btCollisionObject* me, const btVector3& up, btScalar minSlopeDot)
	: btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0))
	, m_me(me)
	, m_up(up)
	, m_minSlopeDot(minSlopeDot)
	{
	}

	virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace)
	{
		if (convexResult.m_hitCollisionObject == m_me)
			return btScalar(1.0);

		btVector3 hitNormalWorld;
		if (normalInWorldSpace)
		{
			hitNormalWorld = convexResult.m_hitNormalLocal;
		} else
		{
			///need to transform normal into worldspace
			hitNormalWorld = m_hitCollisionObject->getWorldTransform().getBasis()*convexResult.m_hitNormalLocal;
		}

		btScalar dotUp = m_up.dot(hitNormalWorld);
		if (dotUp < m_minSlopeDot) {
			return btScalar(1.0);
		}

		return ClosestConvexResultCallback::addSingleResult (convexResult, normalInWorldSpace);
	}
protected:
	btCollisionObject* m_me;
	const btVector3 m_up;
	btScalar m_minSlopeDot;
};

/*
 * Returns the reflection direction of a ray going 'direction' hitting a surface with normal 'normal'
 *
 * from: http://www-cs-students.stanford.edu/~adityagp/final/node3.html
 */
btVector3 computeReflectionDirection (const btVector3& direction, const btVector3& normal)
{
	return direction - (btScalar(2.0) * direction.dot(normal)) * normal;
}

/*
 * Returns the portion of 'direction' that is parallel to 'normal'
 */
btVector3 parallelComponent (const btVector3& direction, const btVector3& normal)
{
	btScalar magnitude = direction.dot(normal);
	return normal * magnitude;
}

/*
 * Returns the portion of 'direction' that is perpindicular to 'normal'
 */
btVector3 perpindicularComponent (const btVector3& direction, const btVector3& normal)
{
	return direction - parallelComponent(direction, normal);
}

ActorMovement::ActorMovement( PhyCollision* comp  , btScalar stepHeight, int upAxis )
{

	mCollisionComp = comp;
	mCollisionComp->enableCollisionDetection( true );
	collisionWorld = comp->getWorld()->_getBulletWorld();

	mDetector = comp->_getOverlapDetector();
	m_ghostObject = comp->_getOverlapDetector()->getCollisionObj();
	m_ghostObject->setCollisionFlags( btCollisionObject::CF_CHARACTER_OBJECT );

	m_convexShape = static_cast< btConvexShape*>( comp->getShape() );

	m_upAxis = upAxis;
	m_addedMargin = 0.02;
	m_walkDirection.setValue(0,0,0);
	m_useGhostObjectSweepTest = true;
	
	m_stepHeight = ( 1.0 / g_GraphicScale ) * stepHeight;
	m_turnAngle = btScalar(0.0);

	m_useWalkDirection = true;	// use walk direction by default, legacy behavior
	m_velocityTimeInterval = 0.0;
	m_verticalVelocity     = 0.0;
	m_verticalOffset       = 0.0;
	m_gravity   = 9.8;
	m_fallSpeed = 55.0; // Terminal velocity of a sky diver in m/s.
	m_jumpSpeed = 10.0; // ?
	m_wasOnGround = false;
	m_wasJumping = false;
	setMaxSlope(btRadians(45.0));
}

ActorMovement::~ActorMovement ()
{
}

btCollisionObject* ActorMovement::getGhostObject()
{
	return m_ghostObject;
}

bool ActorMovement::recoverFromPenetration()
{
	bool penetration = false;

	btHashedOverlappingPairCache* overlappingPairCache = mDetector->getOverlappingPairCache();

	collisionWorld->getDispatcher()->dispatchAllCollisionPairs(
		overlappingPairCache ,
		collisionWorld->getDispatchInfo(), 
		collisionWorld->getDispatcher() );

	m_currentPosition = m_ghostObject->getWorldTransform().getOrigin();
	
	btScalar maxPen = btScalar(0.0);
	for (int i = 0; i < overlappingPairCache->getNumOverlappingPairs(); i++)
	{
		m_manifoldArray.resize(0);

		btBroadphasePair* collisionPair = &overlappingPairCache->getOverlappingPairArray()[i];
		
		if (collisionPair->m_algorithm)
			collisionPair->m_algorithm->getAllContactManifolds(m_manifoldArray);

		
		for (int j=0;j<m_manifoldArray.size();j++)
		{
			btPersistentManifold* manifold = m_manifoldArray[j];
			btScalar directionSign = manifold->getBody0() == m_ghostObject ? btScalar(-1.0) : btScalar(1.0);
			for (int p=0;p<manifold->getNumContacts();p++)
			{
				const btManifoldPoint&pt = manifold->getContactPoint(p);

				btScalar dist = pt.getDistance();

				if (dist < 0.0)
				{
					if (dist < maxPen)
					{
						maxPen = dist;
						m_touchingNormal = pt.m_normalWorldOnB * directionSign;//??

					}
					m_currentPosition += pt.m_normalWorldOnB * directionSign * dist * btScalar(0.2);
					penetration = true;
				} 
				else 
				{
					//printf("touching %f\n", dist);
				}
			}
			
			//manifold->clearManifold();
		}
	}
	btTransform newTrans = m_ghostObject->getWorldTransform();
	newTrans.setOrigin(m_currentPosition);
	m_ghostObject->setWorldTransform(newTrans);

//	printf("m_touchingNormal = %f,%f,%f\n",m_touchingNormal[0],m_touchingNormal[1],m_touchingNormal[2]);
	return penetration;
}

void ActorMovement::stepUp ()
{
	// phase 1: up
	btTransform start, end;
	m_targetPosition = m_currentPosition + getUpAxisDirections()[m_upAxis] * (m_stepHeight + (m_verticalOffset > 0.f?m_verticalOffset:0.f));

	start.setIdentity ();
	end.setIdentity ();

	/* FIXME: Handle penetration properly */
	start.setOrigin (m_currentPosition + getUpAxisDirections()[m_upAxis] * (m_convexShape->getMargin() + m_addedMargin));
	end.setOrigin (m_targetPosition);

	btKinematicClosestNotMeConvexResultCallback callback (m_ghostObject, -getUpAxisDirections()[m_upAxis], btScalar(0.7071));
	callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
	callback.m_collisionFilterMask = getGhostObject()->getBroadphaseHandle()->m_collisionFilterMask;
	
	if (m_useGhostObjectSweepTest)
	{
		mDetector->convexSweepTest (m_convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration);
	}
	else
	{
		collisionWorld->convexSweepTest (m_convexShape, start, end, callback);
	}
	
	if (callback.hasHit())
	{
		// Only modify the position if the hit was a slope and not a wall or ceiling.
		if(callback.m_hitNormalWorld.dot(getUpAxisDirections()[m_upAxis]) > 0.0)
		{
			// we moved up only a fraction of the step height
			m_currentStepOffset = m_stepHeight * callback.m_closestHitFraction;
			m_currentPosition.setInterpolate3 (m_currentPosition, m_targetPosition, callback.m_closestHitFraction);
		}
		m_verticalVelocity = 0.0;
		m_verticalOffset = 0.0;
	} 
	else 
	{
		m_currentStepOffset = m_stepHeight;
		m_currentPosition = m_targetPosition;
	}
}

void ActorMovement::updateTargetPositionBasedOnCollision (const btVector3& hitNormal, btScalar tangentMag, btScalar normalMag)
{
	btVector3 movementDirection = m_targetPosition - m_currentPosition;
	btScalar movementLength = movementDirection.length();
	if (movementLength>SIMD_EPSILON)
	{
		movementDirection.normalize();

		btVector3 reflectDir = computeReflectionDirection (movementDirection, hitNormal);
		reflectDir.normalize();

		btVector3 parallelDir, perpindicularDir;

		parallelDir = parallelComponent (reflectDir, hitNormal);
		perpindicularDir = perpindicularComponent (reflectDir, hitNormal);

		m_targetPosition = m_currentPosition;
		if (0)//tangentMag != 0.0)
		{
			btVector3 parComponent = parallelDir * btScalar (tangentMag*movementLength);
//			printf("parComponent=%f,%f,%f\n",parComponent[0],parComponent[1],parComponent[2]);
			m_targetPosition +=  parComponent;
		}

		if (normalMag != 0.0)
		{
			btVector3 perpComponent = perpindicularDir * btScalar (normalMag*movementLength);
//			printf("perpComponent=%f,%f,%f\n",perpComponent[0],perpComponent[1],perpComponent[2]);
			m_targetPosition += perpComponent;
		}
	} else
	{
//		printf("movementLength don't normalize a zero vector\n");
	}
}

void ActorMovement::stepForwardAndStrafe( const btVector3& walkMove)
{
	// printf("m_normalizedDirection=%f,%f,%f\n",
	// 	m_normalizedDirection[0],m_normalizedDirection[1],m_normalizedDirection[2]);
	// phase 2: forward and strafe
	btTransform start, end;
	m_targetPosition = m_currentPosition + walkMove;

	start.setIdentity ();
	end.setIdentity ();
	
	btScalar fraction = 1.0;
	btScalar distance2 = (m_currentPosition-m_targetPosition).length2();
//	printf("distance2=%f\n",distance2);

	if (m_touchingContact)
	{
		if (m_normalizedDirection.dot(m_touchingNormal) > btScalar(0.0))
		{
			updateTargetPositionBasedOnCollision (m_touchingNormal);
		}
	}

	int maxIter = 10;

	while (fraction > btScalar(0.01) && maxIter-- > 0)
	{
		start.setOrigin (m_currentPosition);
		end.setOrigin (m_targetPosition);
		btVector3 sweepDirNegative(m_currentPosition - m_targetPosition);

		btKinematicClosestNotMeConvexResultCallback callback (m_ghostObject, sweepDirNegative, btScalar(0.0));
		callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
		callback.m_collisionFilterMask = getGhostObject()->getBroadphaseHandle()->m_collisionFilterMask;


		btScalar margin = m_convexShape->getMargin();
		m_convexShape->setMargin(margin + m_addedMargin);

		if (m_useGhostObjectSweepTest)
		{
			mDetector->convexSweepTest (m_convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration);
		} 
		else
		{
			collisionWorld->convexSweepTest (m_convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration);
		}
		
		m_convexShape->setMargin(margin);

		
		fraction -= callback.m_closestHitFraction;

		if (callback.hasHit())
		{	
			// we moved only a fraction
			btScalar hitDistance;
			hitDistance = (callback.m_hitPointWorld - m_currentPosition).length();

//			m_currentPosition.setInterpolate3 (m_currentPosition, m_targetPosition, callback.m_closestHitFraction);

			updateTargetPositionBasedOnCollision (callback.m_hitNormalWorld);
			btVector3 currentDir = m_targetPosition - m_currentPosition;
			distance2 = currentDir.length2();
			if (distance2 > SIMD_EPSILON)
			{
				currentDir.normalize();
				/* See Quake2: "If velocity is against original velocity, stop ead to avoid tiny oscilations in sloping corners." */
				if (currentDir.dot(m_normalizedDirection) <= btScalar(0.0))
				{
					break;
				}
			} 
			else
			{
//				printf("currentDir: don't normalize a zero vector\n");
				break;
			}

		} else {
			// we moved whole way
			m_currentPosition = m_targetPosition;
		}

	//	if (callback.m_closestHitFraction == 0.f)
	//		break;

	}
}

void ActorMovement::stepDown ( btScalar dt)
{
	btTransform start, end;

	// phase 3: down
	/*btScalar additionalDownStep = (m_wasOnGround && !onGround()) ? m_stepHeight : 0.0;
	btVector3 step_drop = getUpAxisDirections()[m_upAxis] * (m_currentStepOffset + additionalDownStep);
	btScalar downVelocity = (additionalDownStep == 0.0 && m_verticalVelocity<0.0?-m_verticalVelocity:0.0) * dt;
	btVector3 gravity_drop = getUpAxisDirections()[m_upAxis] * downVelocity; 
	m_targetPosition -= (step_drop + gravity_drop);*/

	btScalar downVelocity = (m_verticalVelocity<0.f?-m_verticalVelocity:0.f) * dt;
	if(downVelocity > 0.0 && downVelocity < m_stepHeight
		&& (m_wasOnGround || !m_wasJumping))
	{
		downVelocity = m_stepHeight;
	}

	btVector3 step_drop = getUpAxisDirections()[m_upAxis] * (m_currentStepOffset + downVelocity);
	m_targetPosition -= step_drop;

	start.setIdentity ();
	end.setIdentity ();

	start.setOrigin (m_currentPosition);
	end.setOrigin (m_targetPosition);

	btKinematicClosestNotMeConvexResultCallback callback (m_ghostObject, getUpAxisDirections()[m_upAxis], m_maxSlopeCosine);
	callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
	callback.m_collisionFilterMask = getGhostObject()->getBroadphaseHandle()->m_collisionFilterMask;
	
	if (m_useGhostObjectSweepTest)
	{
		mDetector->convexSweepTest (m_convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration);
	} 
	else
	{
		collisionWorld->convexSweepTest (m_convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration);
	}

	if (callback.hasHit())
	{
		// we dropped a fraction of the height -> hit floor
		m_currentPosition.setInterpolate3 (m_currentPosition, m_targetPosition, callback.m_closestHitFraction);
		m_verticalVelocity = 0.0;
		m_verticalOffset = 0.0;
		m_wasJumping = false;
	} 
	else 
	{
		// we dropped the full height
		
		m_currentPosition = m_targetPosition;
	}
}



void ActorMovement::setWalkDirection( Vec3D const& walkDirection )
{
	m_useWalkDirection = true;
	m_walkDirection = ( 1.0 / g_GraphicScale ) * convCF2Bullet( walkDirection );
	m_normalizedDirection = getNormalizedVector(m_walkDirection);
}



void ActorMovement::setVelocityForTimeInterval( Vec3D const& velocity , btScalar timeInterval )
{
//	printf("setVelocity!\n");
//	printf("  interval: %f\n", timeInterval);
//	printf("  velocity: (%f, %f, %f)\n",
//		 velocity.x(), velocity.y(), velocity.z());

	m_useWalkDirection = false;
	m_walkDirection = ( 1.0 / g_GraphicScale ) * convCF2Bullet( velocity );
	m_normalizedDirection = getNormalizedVector(m_walkDirection);
	m_velocityTimeInterval = timeInterval;
}



void ActorMovement::reset ()
{
}

void ActorMovement::warp (const btVector3& origin)
{
	btTransform xform;
	xform.setIdentity();
	xform.setOrigin (origin);
	m_ghostObject->setWorldTransform (xform);
}


void ActorMovement::preStep()
{
	int numPenetrationLoops = 0;
	m_touchingContact = false;
	while (recoverFromPenetration() )
	{
		numPenetrationLoops++;
		m_touchingContact = true;
		if (numPenetrationLoops > 4)
		{
			//printf("character could not recover from penetration = %d\n", numPenetrationLoops);
			break;
		}
	}

	m_currentPosition = m_ghostObject->getWorldTransform().getOrigin();
	m_targetPosition = m_currentPosition;
//	printf("m_targetPosition=%f,%f,%f\n",m_targetPosition[0],m_targetPosition[1],m_targetPosition[2]);

	
}

#include <stdio.h>

void ActorMovement::playerStep ( btScalar dt)
{
//	printf("playerStep(): ");
//	printf("  dt = %f", dt);

	// quick check...
	if (!m_useWalkDirection && m_velocityTimeInterval <= 0.0) 
	{
		m_useWalkDirection = true;
		m_walkDirection.setZero();
		
//		printf("\n");
	}

	m_wasOnGround = onGround();

	// Update fall velocity.
	m_verticalVelocity -= m_gravity * dt;
	if(m_verticalVelocity > 0.0 && m_verticalVelocity > m_jumpSpeed)
	{
		m_verticalVelocity = m_jumpSpeed;
	}
	if(m_verticalVelocity < 0.0 && btFabs(m_verticalVelocity) > btFabs(m_fallSpeed))
	{
		m_verticalVelocity = -btFabs(m_fallSpeed);
	}
	m_verticalOffset = m_verticalVelocity * dt;


	btTransform xform;
	xform = m_ghostObject->getWorldTransform ();

//	printf("walkDirection(%f,%f,%f)\n",walkDirection[0],walkDirection[1],walkDirection[2]);
//	printf("walkSpeed=%f\n",walkSpeed);

	stepUp();
	if (m_useWalkDirection) 
	{
		stepForwardAndStrafe(m_walkDirection);
	} 
	else 
	{
		//printf("  time: %f", m_velocityTimeInterval);
		// still have some time left for moving!
		btScalar dtMoving =
			(dt < m_velocityTimeInterval) ? dt : m_velocityTimeInterval;
		m_velocityTimeInterval -= dt;

		// how far will we move while we are moving?
		btVector3 move = m_walkDirection * dtMoving;

		//printf("  dtMoving: %f", dtMoving);

		// okay, step
		stepForwardAndStrafe( move );
	}
	stepDown(dt);


	// printf("\n");

	xform.setOrigin (m_currentPosition);
	m_ghostObject->setWorldTransform (xform);
}

void ActorMovement::setFallSpeed (btScalar fallSpeed)
{
	m_fallSpeed = ( 1.0f / g_GraphicScale ) * fallSpeed;
}

void ActorMovement::setJumpSpeed (btScalar jumpSpeed)
{
	m_jumpSpeed = ( 1.0f / g_GraphicScale ) * jumpSpeed;
}

void ActorMovement::setMaxJumpHeight (btScalar maxJumpHeight)
{
	m_maxJumpHeight = ( 1.0f / g_GraphicScale ) * maxJumpHeight;
}

bool ActorMovement::canJump () const
{
	return onGround();
}

void ActorMovement::jump ()
{
	if (!canJump())
		return;

	m_verticalVelocity = m_jumpSpeed;
	m_wasJumping = true;

#if 0
	currently no jumping.
	btTransform xform;
	m_rigidBody->getMotionState()->getWorldTransform (xform);
	btVector3 up = xform.getBasis()[1];
	up.normalize ();
	btScalar magnitude = (btScalar(1.0)/m_rigidBody->getInvMass()) * btScalar(8.0);
	m_rigidBody->applyCentralImpulse (up * magnitude);
#endif
}

void ActorMovement::setGravity(btScalar gravity)
{
	m_gravity = gravity;
}

btScalar ActorMovement::getGravity() const
{
	return m_gravity;
}

void ActorMovement::setMaxSlope(btScalar slopeRadians)
{
	m_maxSlopeRadians = slopeRadians;
	m_maxSlopeCosine = btCos(slopeRadians);
}

btScalar ActorMovement::getMaxSlope() const
{
	return m_maxSlopeRadians;
}

bool ActorMovement::onGround () const
{
	return m_verticalVelocity == 0.0 && m_verticalOffset == 0.0;
}


btVector3* ActorMovement::getUpAxisDirections()
{
	static btVector3 sUpAxisDirection[3] = { btVector3(1.0f, 0.0f, 0.0f), btVector3(0.0f, 1.0f, 0.0f), btVector3(0.0f, 0.0f, 1.0f) };
	
	return sUpAxisDirection;
}

void ActorMovement::debugDraw(btIDebugDraw* debugDrawer)
{
}

void ActorMovement::setMoveType( ActorMoveType type )
{
	//FIXME
	mMoveType = type;
}
