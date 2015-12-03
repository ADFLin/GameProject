#ifndef TMovement_h__
#define TMovement_h__

#include "common.h"

class TActor;
class TPhyEntity;


extern float g_DefultStepHeight;

enum MoveType
{
	MT_NORMAL_MOVE ,
	MT_FLY ,
	MT_LADDER ,
	MT_NOCLIP ,
	MT_FREEZE ,
};

enum MoveState
{
	MS_GROUND ,
	MS_AIR ,
	MS_STEP_SLOPE ,
	MS_BLOCK ,
};


struct MoveResult
{
	MoveType  type;
	Vec3D     resqustMoveOffst;
	Vec3D     resultMoveOffset;
	MoveState resultState;
};


class TMovement
{
public:
	TMovement (TActor* actor , btConvexShape* convexShape,btScalar stepHeight, float height );
	~TMovement ();

	void updateAction( btScalar deltaTime );

	///btActionInterface interface
	void	debugDraw(btIDebugDraw* debugDrawer);

	void setUpAxis (int axis)
	{
		if (axis < 0)
			axis = 0;
		if (axis > 2)
			axis = 2;
		m_upAxis = axis;
	}

	bool testOnGround();

	MoveResult& getMoveResult(){ return m_moveResult; }


	/// This should probably be called setPositionIncrementPerSimulatorStep.
	/// This is neither a direction nor a velocity, but the amount to
	///   increment the position each simulation iteration, regardless
	///   of dt.
	/// This call will reset any velocity set by setVelocityForTimeInterval().
	virtual void	setWalkDirection(const Vec3D& walkDirection);

	/// Caller provides a velocity with which the character should move for
	///   the given time period.  After the time period, velocity is reset
	///   to zero.
	/// This call will reset any walk direction set by setWalkDirection().
	/// Negative time intervals will result in no motion.
	virtual void setVelocityForTimeInterval(const Vec3D& velocity,
		btScalar timeInterval);

	void reset ();

	void preStep();
	void playerNormalStep ( btScalar dt );
	void setMaxFallSpeed (btScalar fallSpeed);
	void setJumpSpeed (btScalar jumpSpeed);
	void setJumpHeight( float height );
	void setMaxJumpHeight (btScalar maxJumpHeight);
	bool canJump () const;
	void jump (Float frontSpeed );
	bool jump (Vec3D const& startVecticy );
	void processJumping()
	{

	}

	TPhyBody* getGhostObject();
	void	setUseGhostSweepTest(bool useGhostObjectSweepTest)
	{
		m_useGhostObjectSweepTest = useGhostObjectSweepTest;
	}

	bool      onGround () const;
	MoveState getState() const { return m_state; }

	void   enable( bool beE = true ){ m_enable = beE; }


	Vec3D moveColPos;
	Vec3D moveColNormal;

protected:
	bool  m_enable;
	btCollisionWorld* m_world;

	float       m_minSlopVal;
	Vec3D       m_groundPos;
	Vec3D       m_groundNormal;
	MoveState   m_state;
	MoveResult  m_moveResult;
	
	btScalar    m_halfHeight;

	TActor*     m_actor;
	TPhyBody*   m_ghostObject;
	btConvexShape*	m_convexShape;//is also in m_ghostObject, but it needs to be convex, so we store it here to avoid upcast

	btScalar m_maxFallSpeed;
	btScalar m_jumpSpeed;
	Vec3D    m_jumpVecticy;
	btScalar m_maxJumpHeight;

	btScalar m_turnAngle;
	btScalar m_stepHeight;

	btScalar	m_addedMargin;//@todo: remove this and fix the code

	///this is the desired walk direction, set by the user
	Vec3D	m_walkDirection;
	Vec3D	m_normalizedDirection;

	//some internal variables
	Vec3D     m_currentPosition;
	btScalar  m_currentStepOffset;
	Vec3D     m_targetPosition;
	Vec3D     m_startPosition;

	///keep track of the contact manifolds
	btManifoldArray	m_manifoldArray;

	bool      m_touchingContact;
	Vec3D     m_touchingNormal;
	Float     m_normalThresHold;

	bool	m_useGhostObjectSweepTest;
	bool	m_useWalkDirection;
	float	m_velocityTimeInterval;
	float   m_groudBalance;
	int     m_upAxis;

	bool recoverFromPenetration ( btCollisionWorld* collisionWorld);
	void updateTargetPositionBasedOnCollision (const Vec3D& hit_normal, btScalar tangentMag = btScalar(0.0), btScalar normalMag = btScalar(1.0));

	void stepMove ( const Vec3D& walkMove);
	void stepMoveUp ();
	void stepMoveDown ( float stepOffset , btScalar dt);
	void stepJump( Vec3D const& offset );
	void playerFlyStep( Float dt );
	
};
#endif // TMovement_h__