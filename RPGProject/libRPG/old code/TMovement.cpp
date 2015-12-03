#include "TMovement.h"

#include "TPhySystem.h"
#include "TMovement.h"
#include "TActor.h"
#include "TObject.h"
#include "shareDef.h"
#include "profile.h"


float g_DefultStepHeight = 20;

static Vec3D upAxisDirection[3] = 
{ 
	Vec3D(1.0f, 0.0f, 0.0f), 
	Vec3D(0.0f, 1.0f, 0.0f), 
	Vec3D(0.0f, 0.0f, 1.0f),
};

// static helper method
static Vec3D getNormalizedVector(const Vec3D& v)
{
	Vec3D n = v.normalized();
	if (n.length() < SIMD_EPSILON) {
		n.setValue(0, 0, 0);
	}
	return n;
}


///@todo Interact with dynamic objects,
///Ride kinematicly animated platforms properly
///More realistic (or maybe just a config option) falling
/// -> Should integrate falling velocity manually and use that in stepDown()
///Support jumping
///Support ducking


class NotMeRayCallback : public ClosestRayResultCallback
{
public:
	NotMeRayCallback (btCollisionObject* me) 
		:ClosestRayResultCallback(Vec3D(0.0, 0.0, 0.0), Vec3D(0.0, 0.0, 0.0))
		,m_me(me )
	{
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





class NotMeConvexCallback : public ClosestConvexResultCallback
	, public TraceResult
{
public:
	NotMeConvexCallback(btCollisionObject* me)
		:ClosestConvexResultCallback( Vec3D(0,0,0) , Vec3D(0,0,0) )
		,m_me(me)
	{

	}
	virtual btScalar addSingleResult( LocalConvexResult& result, bool normalInWorldSpace )
	{
		if ( result.m_hitCollisionObject == m_me )
			return 1.0;

		return ClosestConvexResultCallback::addSingleResult ( result, normalInWorldSpace );
	}
protected:
	btCollisionObject* m_me;
};


class NormalThresholdCallback : public ClosestConvexResultCallback
{
public:
	NormalThresholdCallback(btCollisionObject* me  , float threshold ) 
		: ClosestConvexResultCallback(Vec3D(0.0, 0.0, 0.0), Vec3D(0.0, 0.0, 0.0))
		, m_me(me )
		, m_threshold( threshold )
	{
	}

	virtual btScalar addSingleResult( LocalConvexResult& result , bool normalInWorldSpace)
	{
		if ( result.m_hitCollisionObject == m_me )
			return 1.0;


		Vec3D hitNormal;
		if ( normalInWorldSpace )
		{
			hitNormal = result.m_hitNormalLocal;
		} 
		else
		{
			hitNormal = m_hitCollisionObject->getWorldTransform().getBasis() * result.m_hitNormalLocal;
		}

		if ( abs( hitNormal.dot( Vec3D( 0,0,1) ) ) <  m_threshold  )
			return 1.0;

		return ClosestConvexResultCallback::addSingleResult ( result, normalInWorldSpace );
	}

	virtual bool needsCollision(btBroadphaseProxy* proxy0) const
	{
		return ( proxy0->m_collisionFilterGroup & COF_SOILD );
	}



protected:
	float m_threshold;
	btCollisionObject* m_me;
};


TMovement::TMovement (TActor* actor , btConvexShape* convexShape,btScalar stepHeight, float height )
{
	m_world = TPhySystem::instance().getDynamicsWorld();

	m_actor = actor;
	m_ghostObject = (TPhyBody*)actor->getPhyBody();

	m_ghostObject->setSleepingThresholds (0.0, 0.0);
	m_ghostObject->setAngularFactor (0.0);

	m_upAxis = 2;
	m_addedMargin = 0.05f * g_GraphicScale;
	m_walkDirection.setValue(0,0,0);
	m_useGhostObjectSweepTest = true;
	m_stepHeight = stepHeight;
	m_turnAngle = btScalar(0.0);

	m_convexShape = convexShape;

	m_moveResult.type = MT_NORMAL_MOVE;

	m_useWalkDirection = true;	// use walk direction by default, legacy behavior
	m_velocityTimeInterval = 0.0;
	m_halfHeight = 0.5 * height; 

	m_minSlopVal = cos( DEG2RAD(45) );
	m_groundNormal = Vec3D(0,0,1);
	m_jumpVecticy = Vec3D(0,0,0);
	m_groudBalance = 7;
	m_maxFallSpeed = 1000.0;
	m_jumpSpeed = 200.0;
	m_state = MS_AIR;

	m_normalThresHold = cos( DEG2RAD(85) );
}

TMovement::~TMovement ()
{


}

TPhyBody* TMovement::getGhostObject()
{
	return m_ghostObject;
}

bool TMovement::recoverFromPenetration ( btCollisionWorld* collisionWorld )
{

	bool penetration = false;

	//collisionWorld->getDispatcher()->dispatchAllCollisionPairs(m_ghostObject->getOverlappingPairCache(), collisionWorld->getDispatchInfo(), collisionWorld->getDispatcher());

	m_currentPosition = m_ghostObject->getWorldTransform().getOrigin();

	btScalar maxPen = btScalar(0.0);
	for (int i = 0; i < m_ghostObject->getOverlappingPairCache()->getNumOverlappingPairs(); i++)
	{
		m_manifoldArray.resize(0);

		btBroadphasePair* collisionPair = &m_ghostObject->getOverlappingPairCache()->getOverlappingPairArray()[i];
		
		if ( collisionPair->m_pProxy0->m_collisionFilterGroup == COF_TRIGGER ||
			 collisionPair->m_pProxy1->m_collisionFilterGroup == COF_TRIGGER )
			continue;

		if (collisionPair->m_algorithm)
			collisionPair->m_algorithm->getAllContactManifolds(m_manifoldArray);


		for (int j=0;j<m_manifoldArray.size();j++)
		{
			btPersistentManifold* manifold = m_manifoldArray[j];
			btScalar directionSign = manifold->getBody0() == m_ghostObject ? btScalar(-1.0) : btScalar(1.0);
			for (int p=0;p<manifold->getNumContacts();p++)
			{
				const btManifoldPoint&pt = manifold->getContactPoint(p);

				if (pt.getDistance() < 0.0)
				{
					if (pt.getDistance() < maxPen)
					{
						maxPen = pt.getDistance();
						m_touchingNormal = pt.m_normalWorldOnB * directionSign;//??

					}
					m_currentPosition += pt.m_normalWorldOnB * directionSign * pt.getDistance() * btScalar(0.2);
					penetration = true;
				} 
				else 
				{
					//printf("touching %f\n", pt.getDistance());
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

void TMovement::stepMoveUp()
{
	TPROFILE("stepMoveUp");
	// phase 1: up
	btTransform start, end;
	m_targetPosition = m_currentPosition + upAxisDirection[m_upAxis] * m_stepHeight;

	start.setIdentity ();
	end.setIdentity ();

	/* FIXME: Handle penetration properly */
	start.setOrigin (m_currentPosition + upAxisDirection[m_upAxis] * btScalar(0.1f));
	end.setOrigin (m_targetPosition);

	NotMeConvexCallback callback (m_ghostObject);
	callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
	callback.m_collisionFilterMask = COF_SOILD;

	if (m_useGhostObjectSweepTest)
	{
		m_ghostObject->convexSweepTest (m_convexShape, start, end, callback, m_world->getDispatchInfo().m_allowedCcdPenetration);
	}
	else
	{
		m_world->convexSweepTest (m_convexShape, start, end, callback);
	}

	if (callback.hasHit())
	{
		// we moved up only a fraction of the step height
		m_currentStepOffset = m_stepHeight * callback.m_closestHitFraction;
		m_currentPosition.setInterpolate3 (m_currentPosition, m_targetPosition, callback.m_closestHitFraction);
	} else {
		m_currentStepOffset = m_stepHeight;
		m_currentPosition = m_targetPosition;
	}
}

void TMovement::stepMove ( const Vec3D& walkMove)
{
	TPROFILE("stepMove");
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
			updateTargetPositionBasedOnCollision ( m_touchingNormal , 1 , 0.1f );
	}

	int maxIter = 10;

	while (fraction > btScalar(0.01) && maxIter-- > 0)
	{
		start.setOrigin (m_currentPosition);
		end.setOrigin (m_targetPosition);

		NotMeConvexCallback callback (m_ghostObject);
		callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
		callback.m_collisionFilterMask = COF_SOILD;


		if ( m_convexShape->getShapeType() == BOX_SHAPE_PROXYTYPE )
		{
			Vec3D halfLen =  ((btBoxShape*)m_convexShape)->getHalfExtentsWithMargin();
			btBoxShape box( halfLen + Vec3D( 1 , 1, 1 )* m_addedMargin );

			if (m_useGhostObjectSweepTest)
			{
				m_ghostObject->convexSweepTest (&box , start, end, callback, m_world->getDispatchInfo().m_allowedCcdPenetration);
			} 
			else
			{
				m_world->convexSweepTest ( &box, start, end, callback, m_world->getDispatchInfo().m_allowedCcdPenetration);
			}
		}
		else
		{
			btScalar margin = m_convexShape->getMargin();
			m_convexShape->setMargin(margin + m_addedMargin);

			if (m_useGhostObjectSweepTest)
			{
				m_ghostObject->convexSweepTest (m_convexShape, start, end, callback, m_world->getDispatchInfo().m_allowedCcdPenetration);
			} 
			else
			{
				m_world->convexSweepTest (m_convexShape, start, end, callback, m_world->getDispatchInfo().m_allowedCcdPenetration);
			}

			m_convexShape->setMargin(margin);
		}
	

		fraction -= callback.m_closestHitFraction;

		if (callback.hasHit())
		{	
			// we moved only a fraction
			btScalar hitDistance = (callback.m_hitPointWorld - m_currentPosition).length();
			if (hitDistance<0.f)
			{
				//				printf("neg dist?\n");
			}

			/* If the distance is farther than the collision margin, move */
			if ( hitDistance > m_addedMargin)
			{
				//				printf("callback.m_closestHitFraction=%f\n",callback.m_closestHitFraction);
				m_currentPosition.setInterpolate3 (m_currentPosition, m_targetPosition, callback.m_closestHitFraction);
			}


			updateTargetPositionBasedOnCollision( callback.m_hitNormalWorld  , 1 , 0.00 );
			Vec3D currentDir = m_targetPosition - m_currentPosition;
			distance2 = currentDir.length2();

			if ( callback.m_hitNormalWorld.dot( Vec3D(0,0,1) ) < m_minSlopVal  )
			{
				break;
			}

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
		} 
		else 
		{
			// we moved whole way
			m_currentPosition = m_targetPosition;
		}

		//	if (callback.m_closestHitFraction == 0.f)
		//		break;

	}
}

void TMovement::stepMoveDown ( float stepOffset , btScalar dt)
{
	TPROFILE("stepMoveDown");

	btTransform start, end;

	// phase 3: down
	Vec3D step_drop = Vec3D(0,0,1) * stepOffset;
	m_targetPosition -= (step_drop) ;

	btScalar fraction = 1.0;
	btScalar distance2 = (m_currentPosition-m_targetPosition).length2();

	start.setIdentity ();
	end.setIdentity ();


	start.setOrigin (m_currentPosition);
	end.setOrigin (m_targetPosition);


	NormalThresholdCallback callback (m_ghostObject , m_normalThresHold );

	callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
	callback.m_collisionFilterMask  = COF_SOILD;


	if (m_useGhostObjectSweepTest)
	{
		m_ghostObject->convexSweepTest (m_convexShape, start, end, callback, m_world->getDispatchInfo().m_allowedCcdPenetration);
	} 
	else
	{
		m_world->convexSweepTest (m_convexShape, start, end, callback, m_world->getDispatchInfo().m_allowedCcdPenetration);
	}


	if ( callback.hasHit()  )
	{
		m_currentPosition.setInterpolate3 ( m_currentPosition , m_targetPosition, callback.m_closestHitFraction );

		moveColPos = callback.m_hitPointWorld;
		moveColNormal = callback.m_hitNormalWorld;

		float len = callback.m_hitNormalWorld.length();
		//if ( callback.m_hitNormalWorld.dot( Vec3D(0,0,1) ) < m_MinSlopVal &&
		//	 m_currentPosition.z() -  m_startPosition.z() > 0.3 * m_stepHeight )
		//{
		//	m_currentPosition = m_startPosition;
		//}

	} 
	else
	{
		m_currentPosition -= m_currentStepOffset * Vec3D(0,0,1);
		m_state = MS_AIR;
	}
}



void TMovement::updateTargetPositionBasedOnCollision (const Vec3D& hitNormal, btScalar tangentMag, btScalar normalMag)
{
	Vec3D movement = m_targetPosition - m_currentPosition;

	btScalar movementLength = movement.length();

	if ( movementLength > SIMD_EPSILON )
	{
		Vec3D movementNormal  =  hitNormal.dot( movement ) * hitNormal / hitNormal.length2();
		Vec3D movementTangent =  movement - movementNormal;

		m_targetPosition = m_currentPosition;

		if ( tangentMag != 0.0 )
		{
			m_targetPosition +=  tangentMag * movementTangent;
		}

		if ( normalMag != 0.0 )
		{
			m_targetPosition -=  normalMag * movementNormal;
		}
	} 
}



void TMovement::setWalkDirection( const Vec3D& walkDirection )
{
	m_useWalkDirection = true;
	m_walkDirection = walkDirection;
	m_normalizedDirection = getNormalizedVector(m_walkDirection);
}



void TMovement::setVelocityForTimeInterval( const Vec3D& velocity , btScalar timeInterval )
{
	//	printf("setVelocity!\n");
	//	printf("  interval: %f\n", timeInterval);
	//	printf("  velocity: (%f, %f, %f)\n",
	//	    velocity.x(), velocity.y(), velocity.z());

	m_useWalkDirection = false;
	m_walkDirection = velocity;
	m_normalizedDirection = getNormalizedVector(m_walkDirection);
	m_velocityTimeInterval = timeInterval;
}



void TMovement::reset ()
{
}




void TMovement::preStep()
{
	TPROFILE("PreStep");

	int numPenetrationLoops = 0;
	m_touchingContact = false;

	while (recoverFromPenetration (m_world))
	{
		numPenetrationLoops++;
		m_touchingContact = true;
		if (numPenetrationLoops > 1)
		{
			break;
		}
	}

	m_currentPosition = m_ghostObject->getWorldTransform().getOrigin();
	m_targetPosition = m_currentPosition;
}

void TMovement::playerNormalStep ( btScalar dt )
{


	btTransform xform;
	xform = m_ghostObject->getWorldTransform ();


	if (m_state == MS_AIR || m_state == MS_STEP_SLOPE )
	{
		if (m_touchingContact && m_jumpVecticy.dot(m_touchingNormal) > btScalar(0.0) )
		{
			m_jumpVecticy -= m_jumpVecticy.dot( m_touchingNormal ) * m_touchingNormal / m_touchingNormal.length2();
		}

		Vec3D offsetPos =  m_jumpVecticy * dt;
		stepJump( offsetPos );

		m_jumpVecticy += ( g_GlobalVal.gravity * dt ) * Vec3D( 0,0,-1);

		if ( m_jumpVecticy.z() < 0 )
			m_jumpVecticy[2] = TMax( m_jumpVecticy[2] , - m_maxFallSpeed );
	}
	else if( m_state == MS_GROUND )
	{
		if ( m_velocityTimeInterval > 0)
		{
			stepMoveUp ();
			if (m_useWalkDirection) 
			{
				stepMove (m_walkDirection);
			} 
			else 
			{
				//printf("  time: %f", m_velocityTimeInterval);
				// still have some time left for moving!
				btScalar dtMoving =
					(dt < m_velocityTimeInterval) ? dt : m_velocityTimeInterval;
				m_velocityTimeInterval -= dt;

				// how far will we move while we are moving?
				Vec3D move = m_walkDirection * dtMoving;
				// printf("  dtMoving: %f", dtMoving);
				// okay, step
				stepMove( move);
			}
			stepMoveDown ( m_currentStepOffset + m_stepHeight , dt );
			testOnGround();
		}

	}

	// printf("\n");

	xform.setOrigin (m_currentPosition);
	m_actor->setTransform( xform );

}

void TMovement::playerFlyStep( Float dt )
{
	btTransform xform;
	xform = m_ghostObject->getWorldTransform ();

	if ( m_velocityTimeInterval > 0)
	{
		if (m_useWalkDirection) 
		{
			stepMove(m_walkDirection);
		} 
		else 
		{
			preStep ();
			btScalar dtMoving =
				(dt < m_velocityTimeInterval) ? dt : m_velocityTimeInterval;
			m_velocityTimeInterval -= dt;
			Vec3D move = m_walkDirection * dtMoving;
			stepMove( move);
		}
	}
	testOnGround();

	xform.setOrigin (m_currentPosition);
	m_actor->setTransform( xform );
}


void TMovement::setMaxFallSpeed (btScalar fallSpeed)
{
	m_maxFallSpeed = fallSpeed;
}


void TMovement::setJumpHeight( float height )
{
	// v^2 = 2 * g * s
	m_jumpSpeed = sqrt( 2.0f * g_GlobalVal.gravity * height );
}

void TMovement::setJumpSpeed (btScalar jumpSpeed)
{
	m_jumpSpeed = jumpSpeed;
}

void TMovement::setMaxJumpHeight (btScalar maxJumpHeight)
{
	m_maxJumpHeight = maxJumpHeight;
}

bool TMovement::canJump () const
{
	return onGround() || m_moveResult.type == MT_LADDER;
}

bool TMovement::jump (Vec3D const& startVecticy )
{
	if (!canJump())
		return false;

	m_jumpVecticy = startVecticy + m_jumpSpeed * g_UpAxisDir;
	m_state = MS_AIR;

	return true;
}

void TMovement::jump (Float frontSpeed )
{
	if (!canJump())
		return;

	m_jumpVecticy = m_actor->getFaceDir() * frontSpeed +
		            m_jumpSpeed * Vec3D(0,0,1);
	m_state = MS_AIR;

}

bool TMovement::onGround () const
{
	return m_state == MS_GROUND;
}


void	TMovement::debugDraw(btIDebugDraw* debugDrawer)
{
}

bool TMovement::testOnGround()
{
	btDynamicsWorld* world = TPhySystem::instance().getDynamicsWorld();

	btTransform start, end;

	start.setIdentity ();
	end.setIdentity ();


	NotMeConvexCallback callback (m_ghostObject);
	callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
	callback.m_collisionFilterMask = COF_SOILD;

	Vec3D startPos = getGhostObject()->getCenterOfMassPosition();
	Vec3D endPos = startPos - Vec3D( 0 , 0, 3 );

	start.setOrigin (startPos);
	end.setOrigin (endPos);

	getGhostObject()->convexSweepTest (m_convexShape, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration );

	if ( callback.hasHit() )
	{
		m_groundPos = callback.m_hitPointWorld;
		m_groundNormal = callback.m_hitNormalWorld;

		if ( m_groundNormal.dot( Vec3D(0,0,1) ) < m_minSlopVal )
		{
			m_state = MS_STEP_SLOPE; 
		}
		else
		{
			m_state = MS_GROUND;
			m_jumpVecticy = Vec3D(0,0,0);
		}
		return true;
	}
	else
	{
		m_state = MS_AIR;
		m_jumpVecticy = m_walkDirection;
	}

	return false;
}




void TMovement::stepJump( Vec3D const& offset )
{
	// printf("m_normalizedDirection=%f,%f,%f\n",
	// 	m_normalizedDirection[0],m_normalizedDirection[1],m_normalizedDirection[2]);
	// phase 2: forward and strafe

	btDynamicsWorld* world = TPhySystem::instance().getDynamicsWorld();

	btTransform start, end;

	m_targetPosition = m_currentPosition + offset;
	start.setIdentity ();
	end.setIdentity ();

	btScalar fraction = 1.0;
	btScalar distance2 = (m_currentPosition - m_targetPosition).length2();
	//	printf("distance2=%f\n",distance2);

	if (m_touchingContact)
	{
		if (m_touchingContact)
		{
			if (m_normalizedDirection.dot(m_touchingNormal) > btScalar(0.0))
				updateTargetPositionBasedOnCollision (m_touchingNormal , 1 , 0 );
		}
	}

	int maxIter = 10;

	while (fraction > btScalar(0.01) && maxIter-- > 0)
	{
		start.setOrigin (m_currentPosition);
		end.setOrigin (m_targetPosition);

		NormalThresholdCallback callback ( m_ghostObject , m_normalThresHold );
		callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
		callback.m_collisionFilterMask = COF_SOILD;

		if ( m_convexShape->getShapeType() == BOX_SHAPE_PROXYTYPE )
		{
			Vec3D halfLen =  ((btBoxShape*)m_convexShape)->getHalfExtentsWithMargin();
			btBoxShape box( halfLen + Vec3D( 1 ,1, 1 )* m_addedMargin );

			if (m_useGhostObjectSweepTest)
			{
				m_ghostObject->convexSweepTest (&box , start, end, callback, m_world->getDispatchInfo().m_allowedCcdPenetration);
			} 
			else
			{
				m_world->convexSweepTest ( &box, start, end, callback, m_world->getDispatchInfo().m_allowedCcdPenetration);
			}
		}
		else
		{
			btScalar margin = m_convexShape->getMargin();
			m_convexShape->setMargin(margin + m_addedMargin);

			if (m_useGhostObjectSweepTest)
			{
				m_ghostObject->convexSweepTest (m_convexShape, start, end, callback, m_world->getDispatchInfo().m_allowedCcdPenetration);
			} 
			else
			{
				m_world->convexSweepTest (m_convexShape, start, end, callback, m_world->getDispatchInfo().m_allowedCcdPenetration);
			}

			m_convexShape->setMargin(margin);
		}


		fraction -= callback.m_closestHitFraction;

		if (callback.hasHit())
		{	
			// we moved only a fraction
			btScalar hitDistance = (callback.m_hitPointWorld - m_currentPosition).length();
			if (hitDistance<0.f)
			{
				//				printf("neg dist?\n");
			}

			/* If the distance is farther than the collision margin, move */
			if (hitDistance > m_addedMargin)
			{
				//printf("callback.m_closestHitFraction=%f\n",callback.m_closestHitFraction);
				m_currentPosition.setInterpolate3 (m_currentPosition, m_targetPosition, callback.m_closestHitFraction);
			}



			if ( callback.m_hitNormalWorld.dot( Vec3D(0,0,1) ) > m_minSlopVal )
			{
				m_state = MS_GROUND;
				m_jumpVecticy.setValue( 0,0,0);
				break;
			}
			else
			{
				m_state = MS_STEP_SLOPE;
			}


			updateTargetPositionBasedOnCollision ( callback.m_hitNormalWorld , 1 , 0 );

			Vec3D currentDir = m_targetPosition - m_currentPosition;
			distance2 = currentDir.length2();
			if (distance2 > SIMD_EPSILON )
			{
				currentDir.normalize();
			}

		} 
		else 
		{
			// we moved whole way
			m_currentPosition = m_targetPosition;
		}

		//	if (callback.m_closestHitFraction == 0.f)
		//		break;

	}

}

void TMovement::updateAction( btScalar deltaTime )
{
	TPROFILE("Movement Process");

	if ( !m_enable )
		return;

	m_startPosition = getGhostObject()->getCenterOfMassPosition();

	if ( getMoveResult().type == MT_FREEZE )
		return;

	preStep ();

	if ( getMoveResult().type == MT_NORMAL_MOVE )
	{
		playerNormalStep (deltaTime);
	}
	else if ( getMoveResult().type == MT_LADDER ||
		      getMoveResult().type == MT_FLY )
	{
		playerFlyStep (deltaTime);
	}
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              