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

		int vIterNum = 50;
		int pIterNum = 10;

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

				body->mLinearVel *= 1.0 / ( 1 + body->mLinearDamping );
			}
		}

		ContactManifold** sortedContact = new ( mAllocator ) ContactManifold* [ mColManager.mMainifolds.size() ];
		int numMainfold = mColManager.mMainifolds.size(); 

		int idxStatic = mColManager.mMainifolds.size() - 1;
		int idxNormal = 0;

		for( int i = 0 ; i < mColManager.mMainifolds.size() ; ++i )
		{
			ContactManifold& cm = *mColManager.mMainifolds[i];

			RigidBody* bodyA = static_cast< RigidBody* >( cm.mContect.object[0] );
			RigidBody* bodyB = static_cast< RigidBody* >( cm.mContect.object[1] );

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

		for( int i = 0 ; i < numMainfold ; ++i )
		{
			ContactManifold& cm = *sortedContact[i];
			Contact& c = cm.mContect;

			Vector2 cp = 0.5 * ( c.pos[0] + c.pos[1] );

			RigidBody* bodyA = static_cast< RigidBody* >( c.object[0] );
			RigidBody* bodyB = static_cast< RigidBody* >( c.object[1] );

			Vector2 vA = bodyA->getVelFromWorldPos( cp );
			Vector2 vB = bodyB->getVelFromWorldPos( cp );
			float vrel = c.normal.dot( vB - vA );
			float restitution = 0.6f;
			cm.velocityBias = 0;
			if ( vrel < -1 )
			{
				cm.velocityBias = -restitution * vrel;
			}

			//cm.impulse = 0;

			////warm start
			Vector2 rA = cp - bodyA->mPosCenter;
			Vector2 rB = cp - bodyB->mPosCenter;
			Vector2 dp = cm.impulse * c.normal;

			bodyA->mLinearVel -= dp * bodyA->mInvMass;
			//bodyA->mAngularVel -= rA.cross( dp ) * bodyA->mInvI;
			bodyB->mLinearVel += dp * bodyB->mInvMass;
			//bodyB->mAngularVel += rB.cross( dp ) * bodyB->mInvI;
		}

		if ( numMainfold != 0 )
			GDebugJumpFun();

		//std::sort( sortedContact.begin() , sortedContact.end() , DepthSort() );

		for( int nIter = 0 ; nIter < vIterNum ; ++nIter )
		{
			for( int i = 0 ; i < numMainfold ; ++i )
			{
				ContactManifold& cm = *sortedContact[i];
				Contact& c = cm.mContect;

				RigidBody* bodyA = static_cast< RigidBody* >( c.object[0] );
				RigidBody* bodyB = static_cast< RigidBody* >( c.object[1] );

				Vector2 cp = 0.5 * ( c.pos[0] + c.pos[1] );

				Vector2 vA = bodyA->getVelFromWorldPos( cp );
				Vector2 vB = bodyB->getVelFromWorldPos( cp );
				Vector2 rA = cp - bodyA->mPosCenter;
				Vector2 rB = cp - bodyB->mPosCenter;
				Vector2 vrel = vB - vA;
				float vn = vrel.dot( c.normal );
				float nrA = rA.cross( c.normal );
				float nrB = rB.cross( c.normal );

				float invMass = 0;
				invMass += bodyA->mInvMass + bodyA->mInvI * nrA * nrA;
				invMass += bodyB->mInvMass + bodyB->mInvI * nrB * nrB;

				float impulse =  -( vn - cm.velocityBias ) / invMass;
				float newImpulse = Math::Max( cm.impulse + impulse , 0.0f );
				impulse = newImpulse - cm.impulse;

				Vector2 P = impulse * c.normal;
				bodyA->mLinearVel -= P * bodyA->mInvMass;
				//bodyA->mAngularVel -= impulse * nrA * bodyA->mInvI;
				bodyB->mLinearVel += P * bodyB->mInvMass;
				//bodyB->mAngularVel += impulse * nrB * bodyB->mInvI;

				cm.impulse = newImpulse;

				float fa = impulse * nrA * bodyA->mInvI;
				float fb = impulse * nrB * bodyB->mInvI;
				if ( fa != 0 || fb !=0 )
				{
	
					int i = 1;
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

	
			//body->mAngularVel = 0;
		}

		for( int nIter = 0 ; nIter < pIterNum ; ++nIter )
		{
			float const kValueB = 0.8f;
			float const kMaxDepth = 2.f;
			float const kSlopValue = 0.0001f;

			float maxDepth = 0.0;
			for( int i = 0 ; i < numMainfold ; ++i )
			{
				ContactManifold& cm = *sortedContact[i];
				Contact& c = cm.mContect;

				RigidBody* bodyA = static_cast< RigidBody* >( c.object[0] );
				RigidBody* bodyB = static_cast< RigidBody* >( c.object[1] );

				if ( bodyB->getMotionType() != BodyMotion::eDynamic &&
					 bodyB->getMotionType() != BodyMotion::eDynamic )
					 continue;

				Vector2 cpA = bodyA->mXForm.transformPosition( c.posLocal[0] );
				Vector2 cpB = bodyB->mXForm.transformPosition( c.posLocal[1] );
				//#TODO: normal change need concerned
				Vector2 normal = c.normal;

				float depth = normal.dot( cpA - cpB ) + 0.001;
				if ( depth <= 0 )
					continue;

				Vector2 cp = 0.5 * ( cpA + cpB );
				Vector2 rA = cp - bodyA->mPosCenter;
				Vector2 rB = cp - bodyB->mPosCenter;


				float nrA = rA.cross( normal );
				float nrB = rB.cross( normal );

				float invMass = 0;
				invMass += bodyA->mInvMass + bodyA->mInvI * nrA * nrA;
				invMass += bodyB->mInvMass + bodyB->mInvI * nrB * nrB;


				float offDepth = Math::Clamp<float>(  ( depth - kSlopValue ) , 0 , kMaxDepth );

				float impulse = ( invMass > 0 ) ? kValueB * offDepth / invMass : 0;
				if ( impulse > 0 )
				{

					bodyA->mXForm.translate( -impulse * normal * bodyA->mInvMass );
					bodyA->mRotationAngle += -impulse * nrA * bodyA->mInvI;
					bodyA->synTransform();
	
					bodyB->mXForm.translate( impulse * normal * bodyB->mInvMass );
					bodyB->mRotationAngle += impulse * nrB * bodyB->mInvI;
					bodyB->synTransform();
				}

				{
					Vector2 cpA = bodyA->mXForm.transformPosition( c.posLocal[0] );
					Vector2 cpB = bodyB->mXForm.transformPosition( c.posLocal[1] );

					//#TODO: normal change need concerned
					float depth2 = normal.dot( cpA - cpB );

					float dp = depth - depth2;
					LogMsg( "dp = %f " , dp );

					int i = 1;
				}
			}

			if ( maxDepth < 3 * kMaxDepth )
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