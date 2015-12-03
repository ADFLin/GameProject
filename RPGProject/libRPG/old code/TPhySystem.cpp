#include "TPhySystem.h"
#include "TObject.h"
#include "TActor.h"
#include "TTrigger.h"
#include "TBullet.h"
#include "TResManager.h"
#include "FlyModelFileLoader.h"
#include "profile.h"
#include "debug.h"



bool GameContactAddedCallback( btManifoldPoint& cp, 
							   const btCollisionObject* colObj0,  int partId0 , int index0,
							   const btCollisionObject* colObj1,  int partId1 , int index1)
{

	float dist = cp.m_distance1;


	TActor* obj0 = (TActor*) colObj0->getUserPointer();
	TActor* obj1 = (TActor*) colObj1->getUserPointer();

	if ( obj0 == NULL || obj1 == NULL )
		return false;

	if ( dist > 0 ) 
		return false;


	if ( obj0->getModelData().mass >= obj1->getModelData().mass )
	{
		Vec3D pos = obj1->getPosition();
		obj1->setPosition( pos + dist * cp.m_normalWorldOnB );
	}
	else
	{
		Vec3D pos = obj0->getPosition();
		obj0->setPosition(  pos - dist * cp.m_normalWorldOnB );
	}

	cp.setDistance( 0 );
	cp.m_appliedImpulseLateral1 = 0;
	cp.m_appliedImpulseLateral2 = 0;



	return true;
}

struct GameOverlapFilterCallback : public btOverlapFilterCallback
{
	// return true when pairs need collision
	virtual bool needBroadphaseCollision(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1) const
	{
		bool collides = (proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) != 0;
		collides = collides && (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask);
		return collides;
	}
};


void GameNearCallback(btBroadphasePair& collisionPair,
					  btCollisionDispatcher& dispatcher,
					  btDispatcherInfo const& dispatchInfo) 
{
	TPhyBody* body0 = (TPhyBody*)collisionPair.m_pProxy0->m_clientObject;
	TPhyBody* body1 = (TPhyBody*)collisionPair.m_pProxy1->m_clientObject;

	TActor* actor1 = (TActor*) body0->getUserPointer();

}

TPhySystem::TPhySystem()
{

	m_enable =  true;


	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_collisionConfiguration->setConvexConvexMultipointIterations();
	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	m_dispatcher = new	btCollisionDispatcher(m_collisionConfiguration);
	m_broadphase = new  btAxisSweep3( btVector3(-100,-100,-100) , btVector3(100,100,100)) ;
	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	m_solver = new btSequentialImpulseConstraintSolver;

	m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher,m_broadphase,m_solver,m_collisionConfiguration);
	m_dynamicsWorld->setGravity( btVector3(0,0,-g_GlobalVal.gravity ) );

	//btOverlapFilterCallback * filterCallback = new GameOverlapFilterCallback();
	//m_dynamicsWorld->getPairCache()->setOverlapFilterCallback(filterCallback);

	m_broadphase->getOverlappingPairCache()->setInternalGhostPairCallback( new TPhyBodyPairCallback );
	//m_dispatcher->setNearCallback(GameNearCallback);

	//gContactAddedCallback = GameContactAddedCallback;
}

void TPhySystem::processCollision( TPhyBody& body )
{
	btManifoldArray   manifoldArray;
	btBroadphasePairArray& pairArray = body.getOverlappingPairCache()->getOverlappingPairArray();
	int numPairs = pairArray.size();

	for (int i=0;i<numPairs;i++)
	{
		manifoldArray.clear();

		const btBroadphasePair& pair = pairArray[i];

		if ( pair.m_pProxy0->m_clientObject == &body &&
			 pair.m_pProxy0->getUid() < pair.m_pProxy1->getUid() )
		{
			PhyRigidBody* rigidbody = (PhyRigidBody*) pair.m_pProxy1->m_clientObject;
			if ( TPhyBody::upcast( rigidbody ) != NULL )
				continue;
		}


		btBroadphasePair* collisionPair ;
		{
			TPROFILE("findPair");
			//unless we manually perform collision detection on this pair, the contacts are in the dynamics world paircache:
			collisionPair = getDynamicsWorld()->getPairCache()->findPair(pair.m_pProxy0,pair.m_pProxy1);
		}

		if (!collisionPair)
			continue;

		{
			TPROFILE("getAllContactManifolds");
			if (collisionPair->m_algorithm)
				collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);

		}


		TPROFILE("manifoldArray");

		for (int j=0;j<manifoldArray.size();j++)
		{
			btPersistentManifold* manifold = manifoldArray[j];

			int numContacts = 0;

			Vec3D avgPtA(0,0,0);
			Vec3D avgPtB(0,0,0);
			Vec3D avgNormalOnB(0,0,0);
			Float avgDepth = 0;

			btCollisionObject* objA = static_cast<btCollisionObject*>( manifold->getBody0() );
			btCollisionObject* objB = static_cast<btCollisionObject*>( manifold->getBody1() );

			TPhyEntity* entityA = static_cast< TPhyEntity* >( objA->getUserPointer() );
			TPhyEntity* entityB = static_cast< TPhyEntity* >( objB->getUserPointer() );

			bool haveTerrain = ( entityA == NULL || entityB == NULL );

			for (int p=0;p<manifold->getNumContacts();p++ )
			{
				const btManifoldPoint& pt = manifold->getContactPoint(p);
				if ( pt.getDistance() < 0.0f )
				{
					++numContacts;

					if ( haveTerrain )
						break;

					avgPtA  += pt.getPositionWorldOnA();
					avgPtB  += pt.getPositionWorldOnB();
					avgNormalOnB += pt.m_normalWorldOnB;
					avgDepth -= pt.getDistance();

					break;
					/// work here
				}
			}

			if ( numContacts > 0 )
			{
				if ( haveTerrain )
				{
					if ( entityA )
						entityA->OnCollisionTerrain();
					if ( entityB )
						entityB->OnCollisionTerrain();
				}
				else
				{
					avgPtA /= numContacts;
					avgPtB /= numContacts;
					avgDepth /= numContacts;
					avgNormalOnB.normalize();

					entityA->OnCollision( entityB , avgDepth , avgPtA , avgPtB ,  avgNormalOnB );
					entityB->OnCollision( entityA , avgDepth , avgPtB , avgPtA , -avgNormalOnB );
				}

				break;
			}
		}
	}
}

void TPhySystem::addEntity( TPhyEntity* entity )
{
	PhyRigidBody* body = entity->getPhyBody();
	body->setCcdMotionThreshold( 2 * g_GraphicScale );
	body->setCcdSweptSphereRadius( 0.2 * 2 * g_GraphicScale );

	entity->testCollision( true );

	if ( TObject::upCast( entity ) )
	{
		m_dynamicsWorld->addRigidBody( 
			body , COF_OBJECT , COF_ALL );
	}
	else if ( TActor::upCast( entity ) )
	{
		m_dynamicsWorld->addCollisionObject( 
			body  , COF_ACTOR ,  COF_ALL );
	}
	else if ( TTrigger::upCast( entity ) )
	{
		m_dynamicsWorld->addCollisionObject( 
			body  , COF_TRIGGER , COF_ALL & (~COF_TERRAIN) );
	}
	else if ( TBullet::upCast( entity ) )
	{
		m_dynamicsWorld->addCollisionObject( 
			body  , COF_TRIGGER , COF_ALL );
	}
	else
	{
		m_dynamicsWorld->addCollisionObject( body  );
	}
}

void TPhySystem::removeEntity( TPhyEntity* entity )
{
	getDynamicsWorld()->removeRigidBody( entity->getPhyBody() );
	entity->testCollision( false );
}



void TPhySystem::addTerrain( btRigidBody* terrain )
{

	PhyShape* shape = terrain->getCollisionShape();
	shape->setMargin( shape->getMargin() * g_GraphicScale );
	m_dynamicsWorld->addRigidBody( terrain  , COF_TERRAIN , COF_ALL );
}

void TPhySystem::stepSimulation( Float time )
{
	if ( m_enable )
	{
		//m_dynamicsWorld->stepSimulation( time , 10 , 1.0f/ 120 );
		m_dynamicsWorld->stepSimulation( time  );
	}

}

int TPhySystem::getSleepObjNum()
{
	int result = 0;
	btCollisionObjectArray& array = m_dynamicsWorld->getCollisionObjectArray();
	for( int i = 0 ; i < array.size() ; ++i )
	{
		btCollisionObject* obj = array[i];

		btRigidBody* body = btRigidBody::upcast( obj);
		if ( body && body->getActivationState() ==  ISLAND_SLEEPING )
		{
			++result;
		}
	}
	return result;
}
bool RayLineTest(Vec3D const& from , Vec3D const& to , unsigned ColFilterbit , TPhyEntity* ignore , TraceCallback* callback )
{
	callback->m_rayFromWorld = from;
	callback->m_rayToWorld   = to;
	callback->m_collisionFilterGroup = COF_ALL;
	callback->m_collisionFilterMask  = ColFilterbit;
	callback->ignore = ignore;

	TPhySystem::instance().getDynamicsWorld()->rayTest( from , to , *callback );

	if  ( callback->hasHit() )
		return true;

	return false;
}


TraceCallback::TraceCallback() 
	:ClosestRayResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0))
{

}

btScalar TraceCallback::addSingleResult( LocalRayResult& rayResult , bool normalInWorldSpace )
{
	if ( rayResult.m_collisionObject->getUserPointer() && ignore != NULL &&
		 rayResult.m_collisionObject->getUserPointer() == ignore )
		return 1.0;

	surface  = (CollisionFilter) rayResult.m_collisionObject->getBroadphaseHandle()->m_collisionFilterGroup;
	return ClosestRayResultCallback::addSingleResult (rayResult, normalInWorldSpace);
}


bool ConvexSweepTest( btConvexShape* shape , Vec3D const& from , Vec3D const& to , unsigned ColFilterbit , ConvexCallback* callback  )
{

	return false;

}

bool ConvexSweepTest( TPhyBody* phyBody  , Vec3D const& from , Vec3D const& to , unsigned ColFilterbit , ConvexCallback* callback  )
{

	return false;
}

ConvexCallback::ConvexCallback( btCollisionObject* me , Vec3D const& from, Vec3D const& to ) 
	: ClosestConvexResultCallback(  from , to )
	,m_me(me)
{
	m_me = me;
}

btScalar ConvexCallback::addSingleResult( btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace )
{
	if ( convexResult.m_hitCollisionObject == m_me )
		return 1.0;
	return ClosestConvexResultCallback::addSingleResult (convexResult, normalInWorldSpace);
}