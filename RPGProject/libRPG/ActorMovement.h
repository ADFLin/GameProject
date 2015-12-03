#ifndef ActorMovement_h__
#define ActorMovement_h__

#include "CEntity.h"
#include "PhysicsSystem.h"

#include "LinearMath/btVector3.h"
#include "BulletCollision/CollisionShapes/btConvexShape.h"
#include "BulletDynamics/Character/btCharacterControllerInterface.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"

class btCollisionShape;
class btRigidBody;
class btCollisionWorld;
class btCollisionDispatcher;
class btPairCachingGhostObject;

class IRenderEntity;


enum ActorMoveType
{
	AMT_NORMAL ,
	AMT_FLY ,
	AMT_LADDER ,
	AMT_NOCLIP ,
	AMT_FREEZE ,
};

class ActorMovement
{
public:
	ActorMovement( PhyCollision* comp ,btScalar stepHeight, int upAxis = 2 );
	~ActorMovement();

	virtual void update( long time )
	{
		btScalar deltaTime = btScalar(time)/ 1000.0f;
		preStep();
		playerStep(deltaTime);
	}

	///btActionInterface interface
	void  debugDraw(btIDebugDraw* debugDrawer);

	void  setUpAxis (int axis)
	{
		if (axis < 0)
			axis = 0;
		if (axis > 2)
			axis = 2;
		m_upAxis = axis;
	}

	/// This should probably be called setPositionIncrementPerSimulatorStep.
	/// This is neither a direction nor a velocity, but the amount to
	///	increment the position each simulation iteration, regardless
	///	of dt.
	/// This call will reset any velocity set by setVelocityForTimeInterval().
	void setWalkDirection( Vec3D const& walkDirection );
	/// Caller provides a velocity with which the character should move for
	///	the given time period.  After the time period, velocity is reset
	///	to zero.
	/// This call will reset any walk direction set by setWalkDirection().
	/// Negative time intervals will result in no motion.
	void setVelocityForTimeInterval( Vec3D const& velocity , btScalar timeInterval );
	void reset ();
	void warp (const btVector3& origin);

	void preStep ();
	void playerStep ( btScalar dt);

	void setFallSpeed (btScalar fallSpeed);
	void setJumpSpeed (btScalar jumpSpeed);
	void setMaxJumpHeight (btScalar maxJumpHeight);

	bool canJump () const;

	void jump ();

	void     setGravity(btScalar gravity);
	btScalar getGravity() const;

	/// The max slope determines the maximum angle that the controller can walk up.
	/// The slope angle is measured in radians.
	void     setMaxSlope(btScalar slopeRadians);
	btScalar getMaxSlope() const;

	btCollisionObject* getGhostObject();
	void	setUseGhostSweepTest(bool useGhostObjectSweepTest)
	{
		m_useGhostObjectSweepTest = useGhostObjectSweepTest;
	}

	bool onGround () const;

	ActorMoveType getMoveType() const { return mMoveType; }
	void          setMoveType( ActorMoveType type );

	PhyCollision* getCollisionComp(){  return mCollisionComp; }

protected:

	ActorMoveType       mMoveType;
	PhyCollision* mCollisionComp;
	CollisionDetector*  mDetector;

	btCollisionWorld*  collisionWorld;

	btScalar           m_halfHeight;
	btCollisionObject* m_ghostObject;
	btConvexShape*	   m_convexShape;//is also in m_ghostObject, but it needs to be convex, so we store it here to avoid upcast

	btScalar m_verticalVelocity;
	btScalar m_verticalOffset;
	btScalar m_fallSpeed;
	btScalar m_jumpSpeed;
	btScalar m_maxJumpHeight;
	btScalar m_maxSlopeRadians; // Slope angle that is set (used for returning the exact value)
	btScalar m_maxSlopeCosine;  // Cosine equivalent of m_maxSlopeRadians (calculated once when set, for optimization)
	btScalar m_gravity;

	btScalar m_turnAngle;

	btScalar m_stepHeight;

	btScalar	m_addedMargin;//@todo: remove this and fix the code

	///this is the desired walk direction, set by the user
	btVector3	m_walkDirection;
	btVector3	m_normalizedDirection;

	//some internal variables
	btVector3 m_currentPosition;
	btScalar  m_currentStepOffset;
	btVector3 m_targetPosition;

	///keep track of the contact manifolds
	btManifoldArray	m_manifoldArray;

	bool      m_touchingContact;
	btVector3 m_touchingNormal;

	bool    m_wasOnGround;
	bool    m_wasJumping;
	bool	m_useGhostObjectSweepTest;
	bool	m_useWalkDirection;
	btScalar	m_velocityTimeInterval;
	int m_upAxis;

	static btVector3* getUpAxisDirections();

	bool recoverFromPenetration();
	void stepUp();
	void updateTargetPositionBasedOnCollision (const btVector3& hit_normal, btScalar tangentMag = btScalar(0.0), btScalar normalMag = btScalar(1.0));
	void stepForwardAndStrafe ( const btVector3& walkMove);
	void stepDown (btScalar dt);
};

#endif // ActorMovement_h__