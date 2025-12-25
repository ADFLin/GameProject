#include "World.h"

#include "Phy2D/Shape.h"
#include "Phy2D/RigidBody.h"

#include "LogSystem.h"


namespace Phy2D
{
	void EmptyDebugJump(){}
	DebugJumpFunc GDebugJumpFun = EmptyDebugJump;

	void World::simulate(float dt)
	{
		//LogDevMsg(0,"World::sim");
		mAllocator.clearFrame();

		// Box2D default iteration counts
		int vIterNum = 10;   // Increased for stability
		int pIterNum = 10;   // Increased from 3 to 10 to handle deep stacks

		mColManager.preocss( dt );

		for( RigidBodyList::iterator iter = mRigidBodies.begin() ,itEnd = mRigidBodies.end();
			iter != itEnd ; ++iter )
		{
			RigidBody* body = *iter;
			body->saveState();
			if ( body->getMotionType() != BodyMotion::eStatic )
			{
				if ( body->getMotionType() == BodyMotion::eDynamic )
					body->addLinearImpulse( body->mMass * mGrivaty * dt );
				body->applyImpulse();

			}
		}

		ContactManifold** sortedContact = new ( mAllocator ) ContactManifold* [ mColManager.mMainifolds.size() ];
		int numMainfold = mColManager.mMainifolds.size(); 

		int idxStatic = mColManager.mMainifolds.size() - 1;
		int idxNormal = 0;

		for( int i = 0 ; i < mColManager.mMainifolds.size() ; ++i )
		{
			ContactManifold& cm = *mColManager.mMainifolds[i];

			RigidBody* bodyA = static_cast< RigidBody* >( cm.mContact.object[0] );
			RigidBody* bodyB = static_cast< RigidBody* >( cm.mContact.object[1] );

			if ( bodyA->getMotionType() == BodyMotion::eStatic || 
				 bodyB->getMotionType() == BodyMotion::eStatic )
			{
				sortedContact[idxStatic--] = &cm;
			}
			else
			{
				sortedContact[idxNormal++] = &cm;
			}

		}

		// Phase 1: Compute velocity bias for all contact points (using pre-warmstart velocities)
		for( int i = 0 ; i < numMainfold ; ++i )
		{
			ContactManifold& cm = *sortedContact[i];
			Contact& c = cm.mContact;

			RigidBody* bodyA = static_cast< RigidBody* >( c.object[0] );
			RigidBody* bodyB = static_cast< RigidBody* >( c.object[1] );

			// Compute velocity bias using first contact point (shared for manifold)
			if (cm.mNumContacts > 0)
			{
				ManifoldPoint& mp = cm.mPoints[0];
				Vector2 cpA = bodyA->mXForm.transformPosition( mp.posLocal[0] );
				Vector2 cpB = bodyB->mXForm.transformPosition( mp.posLocal[1] );
				Vector2 cp = 0.5 * ( cpA + cpB );

				Vector2 vA = bodyA->getVelFromWorldPos( cp );
				Vector2 vB = bodyB->getVelFromWorldPos( cp );
				float vrel = c.normal.dot( vB - vA );
				
				float restitution = 0.5f;
				float VelocityThreshold = 1.0f;
				cm.velocityBias = 0;
				if ( vrel < -VelocityThreshold)
				{
					cm.velocityBias = -restitution * vrel;
				}
			}
		}

		// Phase 2: Apply warm start for all contact points
		bool enableWarmStart = false;  // Disable warm start for debugging
		for( int i = 0 ; i < numMainfold ; ++i )
		{
			ContactManifold& cm = *sortedContact[i];
			Contact& c = cm.mContact;

			if ( !enableWarmStart )
			{
				// Reset all impulses when warm start is disabled
				for (int j = 0; j < cm.mNumContacts; ++j)
				{
					cm.mPoints[j].normalImpulse = 0;
					cm.mPoints[j].tangentImpulse = 0;
				}
				continue;
			}

			RigidBody* bodyA = static_cast< RigidBody* >( c.object[0] );
			RigidBody* bodyB = static_cast< RigidBody* >( c.object[1] );

			// Warm start each contact point
			for (int j = 0; j < cm.mNumContacts; ++j)
			{
				ManifoldPoint& mp = cm.mPoints[j];
				Vector2 cpA = bodyA->mXForm.transformPosition( mp.posLocal[0] );
				Vector2 cpB = bodyB->mXForm.transformPosition( mp.posLocal[1] );
				Vector2 cp = 0.5 * ( cpA + cpB );

				Vector2 rA = cp - bodyA->mPosCenter;
				Vector2 rB = cp - bodyB->mPosCenter;
				
				Vector2 dp = mp.normalImpulse * c.normal;

				bodyA->mLinearVel -= dp * bodyA->mInvMass;
				bodyA->mAngularVel -= rA.cross( dp ) * bodyA->mInvI;
				bodyB->mLinearVel += dp * bodyB->mInvMass;
				bodyB->mAngularVel += rB.cross( dp ) * bodyB->mInvI;
			}
		}

		if ( numMainfold != 0 )
			GDebugJumpFun();

		// Velocity solver iterations
		for( int nIter = 0 ; nIter < vIterNum ; ++nIter )
		{
			for( int i = 0 ; i < numMainfold ; ++i )
			{
				ContactManifold& cm = *sortedContact[i];
				Contact& c = cm.mContact;

				RigidBody* bodyA = static_cast< RigidBody* >( c.object[0] );
				RigidBody* bodyB = static_cast< RigidBody* >( c.object[1] );

				// For circle-circle, recalculate normal based on current positions
				Vector2 normal;
				if ( bodyA->mShape->getType() == Shape::eCircle && 
				     bodyB->mShape->getType() == Shape::eCircle )
				{
					Vector2 offset = bodyB->getPos() - bodyA->getPos();
					float len = offset.length();
					normal = (len > 0.0001f) ? offset / len : Vector2(1, 0);
				}
				else
				{
					normal = c.normal;
				}
				
				// Tangent direction
				Vector2 tangent = Math::Perp( normal );
				
				// Friction coefficient (Increased for stability)
				float friction = 0.5f;

				// Solve each contact point
				for (int j = 0; j < cm.mNumContacts; ++j)
				{
					ManifoldPoint& mp = cm.mPoints[j];
					
					// Recalculate contact point using current transforms
					Vector2 cpA = bodyA->mXForm.transformPosition( mp.posLocal[0] );
					Vector2 cpB = bodyB->mXForm.transformPosition( mp.posLocal[1] );
					Vector2 cp = 0.5 * ( cpA + cpB );

					Vector2 vA = bodyA->getVelFromWorldPos( cp );
					Vector2 vB = bodyB->getVelFromWorldPos( cp );
					Vector2 rA = cp - bodyA->mPosCenter;
					Vector2 rB = cp - bodyB->mPosCenter;
					Vector2 vrel = vB - vA;
					
					float nrA = rA.cross( normal );
					float nrB = rB.cross( normal );
					float trA = rA.cross( tangent );
					float trB = rB.cross( tangent );

					// ============ Solve tangent constraints (friction) first ============
					{
						float vt = vrel.dot( tangent );
						
						float kTangent = bodyA->mInvMass + bodyB->mInvMass 
						               + bodyA->mInvI * trA * trA + bodyB->mInvI * trB * trB;
						float tangentMass = (kTangent > 0.0f) ? 1.0f / kTangent : 0.0f;
						
						float lambda = -tangentMass * vt;
						
						// Coulomb friction per-point
						float maxFriction = friction * mp.normalImpulse;
						float newTangentImpulse = Math::Clamp( mp.tangentImpulse + lambda, -maxFriction, maxFriction );
						lambda = newTangentImpulse - mp.tangentImpulse;
						mp.tangentImpulse = newTangentImpulse;
						
						// Apply tangent impulse
						Vector2 Pt = lambda * tangent;
						bodyA->mLinearVel -= Pt * bodyA->mInvMass;
						bodyA->mAngularVel -= lambda * trA * bodyA->mInvI;
						bodyB->mLinearVel += Pt * bodyB->mInvMass;
						bodyB->mAngularVel += lambda * trB * bodyB->mInvI;
					}

					// ============ Solve normal constraints ============
					// Re-compute relative velocity after friction was applied
					vA = bodyA->getVelFromWorldPos( cp );
					vB = bodyB->getVelFromWorldPos( cp );
					vrel = vB - vA;
					float vn = vrel.dot( normal );

					float kNormal = bodyA->mInvMass + bodyB->mInvMass 
					              + bodyA->mInvI * nrA * nrA + bodyB->mInvI * nrB * nrB;
					float normalMass = (kNormal > 0.0f) ? 1.0f / kNormal : 0.0f;

					float impulse = -normalMass * ( vn - cm.velocityBias );
					float newImpulse = Math::Max( mp.normalImpulse + impulse , 0.0f );
					impulse = newImpulse - mp.normalImpulse;

					Vector2 P = impulse * normal;
					bodyA->mLinearVel -= P * bodyA->mInvMass;
					bodyA->mAngularVel -= impulse * nrA * bodyA->mInvI;
					bodyB->mLinearVel += P * bodyB->mInvMass;
					bodyB->mAngularVel += impulse * nrB * bodyB->mInvI;

					mp.normalImpulse = newImpulse;
				}

				if ( numMainfold != 0 )
					GDebugJumpFun();
			}
		}


		for( RigidBodyList::iterator iter = mRigidBodies.begin() ,itEnd = mRigidBodies.end();
			iter != itEnd ; ++iter )
		{
			RigidBody* body = *iter;
			body->intergedTramsform( dt );
				if ( body->getMotionType() == BodyMotion::eDynamic )
			{
				// Apply linear damping
				float linearDampFactor = Math::Max( 0.0f, 1.0f - body->mLinearDamping * dt );
				body->mLinearVel *= linearDampFactor;
				
				// Apply angular damping
				float angularDampFactor = Math::Max( 0.0f, 1.0f - body->mAngularDamping * dt );
				body->mAngularVel *= angularDampFactor;
			}

	
			//body->mAngularVel = 0;
		}

		// Position solver iterations
		for( int nIter = 0 ; nIter < pIterNum ; ++nIter )
		{
			float const LinearSlop = 0.005f;
			float const Baumgarte = 0.2f;
			float const MaxLinearCorrection = 0.2f;

			float maxSeparationError = 0.0f;
			// Process in reverse order: static contacts at the end, processed first
			for( int i = numMainfold - 1 ; i >= 0 ; --i )
			{
				ContactManifold& cm = *sortedContact[i];
				Contact& c = cm.mContact;

				RigidBody* bodyA = static_cast< RigidBody* >( c.object[0] );
				RigidBody* bodyB = static_cast< RigidBody* >( c.object[1] );

				// For circle-circle, recalculate normal
				Vector2 normal;
				if ( bodyA->mShape->getType() == Shape::eCircle && 
				     bodyB->mShape->getType() == Shape::eCircle )
				{
					Vector2 offset = bodyB->getPos() - bodyA->getPos();
					float len = offset.length();
					normal = (len > 0.0001f) ? offset / len : Vector2(1, 0);
				}
				else
				{
					// CRITICAL FIX: Recalculate normal based on current body rotation
					if (cm.refIsA)
						normal = bodyA->mXForm.transformVector(cm.localNormal);
					else
						normal = bodyB->mXForm.transformVector(cm.localNormal);
				}

				// Solve each contact point for position
				for (int j = 0; j < cm.mNumContacts; ++j)
				{
					ManifoldPoint& mp = cm.mPoints[j];
					
					// Recalculate contact points using current transforms
					Vector2 cpA = bodyA->mXForm.transformPosition( mp.posLocal[0] );
					Vector2 cpB = bodyB->mXForm.transformPosition( mp.posLocal[1] );
					
					// For circles, recalculate contact points
					if ( bodyA->mShape->getType() == Shape::eCircle && 
					     bodyB->mShape->getType() == Shape::eCircle )
					{
						float radiusA = static_cast<CircleShape*>(bodyA->mShape)->getRadius();
						float radiusB = static_cast<CircleShape*>(bodyB->mShape)->getRadius();
						cpA = bodyA->getPos() + radiusA * normal;
						cpB = bodyB->getPos() - radiusB * normal;
					}
					// CRITICAL: For boxes, use stored local positions
					// These were calculated during collision detection
					// The key is that posLocal should remain valid even as bodies move

					// Separation: negative means penetrating
					float separation = normal.dot( cpB - cpA );
					
					// Track max constraint error
					maxSeparationError = Math::Min( maxSeparationError, separation );

					// Compute position correction
					float C = Math::Clamp( Baumgarte * (separation + LinearSlop), -MaxLinearCorrection, 0.0f );
					
					if ( C >= 0.0f )
						continue;  // No correction needed

					Vector2 cp = 0.5 * ( cpA + cpB );
					Vector2 rA = cp - bodyA->mPosCenter;
					Vector2 rB = cp - bodyB->mPosCenter;

					float nrA = rA.cross( normal );
					float nrB = rB.cross( normal );

					// Compute effective mass
					float K = bodyA->mInvMass + bodyA->mInvI * nrA * nrA
					        + bodyB->mInvMass + bodyB->mInvI * nrB * nrB;

					// Compute position impulse
					float impulse = K > 0.0f ? -C / K : 0.0f;

					Vector2 P = impulse * normal;

					bodyA->mXForm.translate( -P * bodyA->mInvMass );
					bodyA->mRotationAngle += -impulse * nrA * bodyA->mInvI;
					bodyA->synTransform();

					bodyB->mXForm.translate( P * bodyB->mInvMass );
					bodyB->mRotationAngle += impulse * nrB * bodyB->mInvI;
					bodyB->synTransform();
				}
			}

			// Early exit if constraints are satisfied
			if ( maxSeparationError >= -3.0f * LinearSlop )
				break;
		}

		if ( numMainfold != 0 )
			GDebugJumpFun();

	}

	void World::destroyRigidBody(RigidBody* body)
	{
		assert( body && body->hook.isLinked() );
		mColManager.removeObject( body );
		body->hook.unlink();
		delete body;
	}

	CollideObject* World::createCollideObject(Shape* shape)
	{
		CollideObject* obj = new CollideObject;
		obj->mShape = shape;
		mColManager.addObject( obj );
		mColObjects.push_back( obj );
		return obj;
	}

	void World::destroyCollideObject(CollideObject* obj)
	{
		assert( obj && obj->hook.isLinked() );
		mColManager.removeObject( obj );
		obj->hook.unlink();
		delete obj;
	}

	RigidBody* World::createRigidBody(Shape* shape , BodyInfo const& info)
	{
		RigidBody* body = new RigidBody;
		body->init( shape , info );
		mColManager.addObject( body );
		mRigidBodies.push_back( body );
		return body;
	}

	void World::clearnup( bool beDelete )
	{

	}




}//namespace Phy2D
