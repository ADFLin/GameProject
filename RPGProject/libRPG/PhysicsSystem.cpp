#include "ProjectPCH.h"
#include "PhysicsSystem.h"

#include "PhysicsPrivate.h"

#include <cstdio>


class CollisionDetectorCallback : public btOverlappingPairCallback
{
public:
	//virtual 
	btBroadphasePair*	addOverlappingPair(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1)
	{
		btCollisionObject* colObj0 = static_cast< btCollisionObject* >( proxy0->m_clientObject );
		btCollisionObject* colObj1 = static_cast< btCollisionObject* >( proxy1->m_clientObject );

		PhyCollision* comp0 = static_cast< PhyCollision* >( colObj0->getUserPointer() );
		PhyCollision* comp1 = static_cast< PhyCollision* >( colObj1->getUserPointer() );

		if ( comp0->_getOverlapDetector() )
			comp0->_getOverlapDetector()->addOverlappingObject(proxy1, proxy0);
		if ( comp1->_getOverlapDetector() )
			comp1->_getOverlapDetector()->addOverlappingObject(proxy0, proxy1);
		return 0;
	}

	//virtual 
	void*	removeOverlappingPair(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1,btDispatcher* dispatcher)
	{

		btCollisionObject* colObj0 = static_cast< btCollisionObject* >( proxy0->m_clientObject );
		btCollisionObject* colObj1 = static_cast< btCollisionObject* >( proxy1->m_clientObject );

		PhyCollision* comp0 = static_cast< PhyCollision* >( colObj0->getUserPointer() );
		PhyCollision* comp1 = static_cast< PhyCollision* >( colObj1->getUserPointer() );

		if ( comp0->_getOverlapDetector() )
			comp0->_getOverlapDetector()->removeOverlappingObject(proxy1,dispatcher,proxy0);
		if ( comp1->_getOverlapDetector() )
			comp1->_getOverlapDetector()->removeOverlappingObject(proxy0,dispatcher,proxy1);
		return 0;

	}
	//virtual 
	void	removeOverlappingPairsContainingProxy(btBroadphaseProxy* proxy0,btDispatcher* dispatcher)
	{
		return;

		//btCollisionObject* colObj0 = static_cast< btCollisionObject* >( proxy0->m_clientObject );
		//CollisionComponent* comp0 = static_cast< CollisionComponent* >( colObj0->getUserPointer() );

		//if ( comp0->_getOverlapDetector() )
		//	comp0->_getOverlapDetector()->getOverlappingPairCache()->removeOverlappingPairsContainingProxy(proxy0,dispatcher);
	}

};



PhysicsWorld::PhysicsWorld()
{
	mCollisionConfiguration = new btDefaultCollisionConfiguration();
	mCollisionConfiguration->setConvexConvexMultipointIterations(10,5);

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	mDispatcher = new	btCollisionDispatcher(mCollisionConfiguration);
	mBroadphase = new  btAxisSweep3( btVector3(-100,-100,-100) , btVector3(100,100,100)) ;
	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	mSolver = new btSequentialImpulseConstraintSolver;

	mDynamicsWorld = new btDiscreteDynamicsWorld(mDispatcher,mBroadphase,mSolver,mCollisionConfiguration);
	mDynamicsWorld->setGravity( btVector3(0,0,-9.8) );

	//btOverlapFilterCallback * filterCallback = new GameOverlapFilterCallback();
	//m_dynamicsWorld->getPairCache()->setOverlapFilterCallback(filterCallback);

	mBroadphase->getOverlappingPairCache()->setInternalGhostPairCallback( new CollisionDetectorCallback );
	//m_dispatcher->setNearCallback(GameNearCallback);

	//gContactAddedCallback = GameContactAddedCallback;

	mDbgDrawer = new PhysicsDebugDrawer;
}

PhysicsWorld::~PhysicsWorld()
{
	delete mDynamicsWorld;
	delete mBroadphase;
	delete mDispatcher;
	delete mSolver;
	delete mCollisionConfiguration;

	delete mDbgDrawer;
}

void PhysicsWorld::setGravity( Vec3D const& value )
{
	mDynamicsWorld->setGravity( 1 / g_GraphicScale * convCF2Bullet( value ) );
}

PhyRigid* PhysicsWorld::createRigidComponent( PhyShapeHandle shape , float mass , unsigned flag , unsigned group  , unsigned mask )
{
	btCollisionShape* colShape = static_cast< btCollisionShape* >( shape );

	bool isDynamic = ( mass != 0.f);
	btVector3 localInertia(0,0,0);
	if (isDynamic)
		colShape->calculateLocalInertia( mass , localInertia );

	btRigidBody::btRigidBodyConstructionInfo info( mass , NULL , colShape , localInertia );

	btRigidBody* body = new btRigidBody( info );
	PhyRigid* comp = new PhyRigid( body );

	configComponent( comp , flag );
	addComponent( comp , group , mask );
	return comp;
}

PhyCollision* PhysicsWorld::createCollisionComponent( PhyShapeHandle shape , unsigned flag , unsigned group  , unsigned mask  )
{
	btCollisionObject* colObj = new btCollisionObject;
	colObj->setCollisionShape( static_cast< btCollisionShape*>( shape ) );

	PhyCollision* comp = new PhyCollision( colObj );

	configComponent( comp , flag );
	addComponent( comp , group , mask );
	return comp;
}

void PhysicsWorld::update( long time )
{
	mDynamicsWorld->stepSimulation( time / 1000.0f , 10 );
	detectCollision();
}

void PhysicsWorld::detectCollision()
{
	btCollisionObjectArray const& objs = mDynamicsWorld->getCollisionWorld()->getCollisionObjectArray();

	for( int i = 0 ; i < objs.size() ; ++i )
	{
		btCollisionObject const* object = objs[i];
		if ( object->getUserPointer() )
		{
			PhyCollision* comp = static_cast< PhyCollision* >( object->getUserPointer() );
			comp->_processCollision( mDynamicsWorld );
		}
	}
}

void PhysicsWorld::enableDebugDraw( bool beDraw )
{
	mDynamicsWorld->setDebugDrawer( ( beDraw ) ? mDbgDrawer : 0 );
}

void PhysicsWorld::renderDebugDraw()
{
	if ( !mDynamicsWorld->getDebugDrawer() )
		return;

	if ( mDbgDrawer->beginRender() )
	{
		mDynamicsWorld->debugDrawWorld();
		mDbgDrawer->endRender();
	}
}

btDiscreteDynamicsWorld* PhysicsWorld::_getBulletWorld()
{
	return mDynamicsWorld;
}


class TraceCallback : public ClosestRayResultCallback
{
public:

	TraceCallback(const btVector3&	rayFromWorld,const btVector3&	rayToWorld)
		:ClosestRayResultCallback( rayFromWorld , rayToWorld ){}

	PhyCollision* ignore;
	RayTraceResult*     traceResult;

	btScalar     addSingleResult( LocalRayResult& rayResult , bool normalInWorldSpace)
	{
		if ( rayResult.m_collisionObject->getUserPointer() && ignore && 
			 rayResult.m_collisionObject->getUserPointer() == ignore )
		{
			traceResult->hitIgnore = true;
			return 1.0;
		}
		traceResult->surface  = (CollisionFilter) rayResult.m_collisionObject->getBroadphaseHandle()->m_collisionFilterGroup;
		return ClosestRayResultCallback::addSingleResult( rayResult, normalInWorldSpace );
	}
};


bool PhysicsWorld::testRayCollision( Vec3D const& from , Vec3D const& to , unsigned colFilterbit , PhyCollision* ignore , RayTraceResult& result )
{
	TraceCallback callback(
		( 1.0f / g_GraphicScale ) * convCF2Bullet( from ) ,
		( 1.0f / g_GraphicScale ) * convCF2Bullet( to ) );

	callback.m_collisionFilterGroup = COF_ALL;
	callback.m_collisionFilterMask  = colFilterbit;

	callback.ignore      = ignore;
	callback.traceResult = &result;

	result.hitIgnore = false;

	_getBulletWorld()->rayTest( callback.m_rayFromWorld , callback.m_rayToWorld , callback );

	if  ( callback.hasHit() )
	{
		result.haveHit = true;
		result.entity  = static_cast< PhyCollision*>( callback.m_collisionObject->getUserPointer() );
		result.hitFraction = callback.m_closestHitFraction;
		result.hitPos      = g_GraphicScale * convBullet2CF( callback.m_hitPointWorld );
		result.hitNormal   = convBullet2CF( callback.m_hitNormalWorld );
	}
	else
	{
		result.haveHit = false;
		result.entity = nullptr;
		result.hitFraction = 1.0f;
	}
	return result.haveHit;
}

void PhysicsWorld::configComponent( PhyCollision* comp , unsigned flag )
{
	comp->mPhyFlag = flag;
	if ( comp->mPhyFlag & (PHY_COLLISION_DETCTION | PHY_ACTOR_OBJ ) )
		comp->enableCollisionDetection( true );

	if ( flag & PHY_ACTOR_OBJ )
		comp->mPhyImpl->setCollisionFlags( btCollisionObject::CF_CHARACTER_OBJECT );
};
void PhysicsWorld::addComponent( PhyCollision* comp , unsigned group , unsigned mask )
{
	comp->mWorld = this;
	mDynamicsWorld->addCollisionObject( comp->mPhyImpl , group , mask );
}

void PhysicsWorld::addComponent( PhyRigid* comp , unsigned group  , unsigned mask )
{
	comp->mWorld = this;
	mDynamicsWorld->addRigidBody(comp->getRigidBody() , group , mask );
}

CollisionDetector::~CollisionDetector()
{
	mHashPairCache->~btHashedOverlappingPairCache();
	btAlignedFree( mHashPairCache );
}

void CollisionDetector::addOverlappingObject( btBroadphaseProxy* otherProxy, btBroadphaseProxy* thisProxy )
{
	btBroadphaseProxy*actualThisProxy = thisProxy;
	btAssert(actualThisProxy);

	btCollisionObject* otherObject = (btCollisionObject*)otherProxy->m_clientObject;
	btAssert(otherObject);
	int index = mOverlappingObjects.findLinearSearch(otherObject);
	if (index==mOverlappingObjects.size())
	{
		mOverlappingObjects.push_back(otherObject);
		mHashPairCache->addOverlappingPair(actualThisProxy,otherProxy);
	}
}

void CollisionDetector::removeOverlappingObject( btBroadphaseProxy* otherProxy,btDispatcher* dispatcher,btBroadphaseProxy* thisProxy )
{
	btCollisionObject* otherObject = (btCollisionObject*)otherProxy->m_clientObject;
	btBroadphaseProxy* actualThisProxy = thisProxy;
	btAssert(actualThisProxy);

	btAssert(otherObject);
	int index = mOverlappingObjects.findLinearSearch(otherObject);
	if (index<mOverlappingObjects.size())
	{
		mOverlappingObjects[index] = mOverlappingObjects[mOverlappingObjects.size()-1];
		mOverlappingObjects.pop_back();
		mHashPairCache->removeOverlappingPair(actualThisProxy,otherProxy,dispatcher);
	}
}

void CollisionDetector::processCollision( btDynamicsWorld* world )
{
	btManifoldArray   manifoldArray;
	btBroadphasePairArray& pairArray = getOverlappingPairCache()->getOverlappingPairArray();
	int numPairs = pairArray.size();

	for (int i=0;i<numPairs;i++)
	{
		manifoldArray.clear();

		const btBroadphasePair& pair = pairArray[i];

		if ( pair.m_pProxy0->m_clientObject == &mObject &&
			pair.m_pProxy0->getUid() < pair.m_pProxy1->getUid() )
		{
			btCollisionObject* obj = static_cast< btCollisionObject* >(  pair.m_pProxy1->m_clientObject );
			PhyCollision* comp = static_cast< PhyCollision* >( obj->getUserPointer() );

			if ( comp == NULL || comp->_getOverlapDetector() == NULL )
				continue;
		}


		btBroadphasePair* collisionPair ;
		//unless we manually perform collision detection on this pair, the contacts are in the dynamics world paircache:
		collisionPair = world->getPairCache()->findPair(pair.m_pProxy0,pair.m_pProxy1);

		if (!collisionPair)
			continue;

		if (collisionPair->m_algorithm )
			collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);


		for (int j=0;j<manifoldArray.size();j++)
		{
			btPersistentManifold* manifold = manifoldArray[j];

			int numContacts = 0;

			btVector3 avgPtA(0,0,0);
			btVector3 avgPtB(0,0,0);
			btVector3 avgNormalOnB(0,0,0);
			float avgDepth = 0;

			btCollisionObject* objA = static_cast<btCollisionObject*>( manifold->getBody0() );
			btCollisionObject* objB = static_cast<btCollisionObject*>( manifold->getBody1() );

			PhyCollision* compA = static_cast< PhyCollision* >( objA->getUserPointer() );
			PhyCollision* compB = static_cast< PhyCollision* >( objB->getUserPointer() );

			for (int p=0;p<manifold->getNumContacts();p++ )
			{
				const btManifoldPoint& pt = manifold->getContactPoint(p);
				if ( pt.getDistance() < 0.0f )
				{
					++numContacts;

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
				avgPtA /= numContacts;
				avgPtB /= numContacts;
				avgDepth /= numContacts;
				avgNormalOnB.normalize();

				Vec3D avgPtA_ = g_GraphicScale * convBullet2CF( avgPtA );
				Vec3D avgPtB_ = g_GraphicScale * convBullet2CF( avgPtB );
				Vec3D avgNormalOnB_ = convBullet2CF( avgNormalOnB );

				if ( compA )
					compA->_onCollision( compB , avgDepth , avgPtA_ , avgPtB_ ,  avgNormalOnB_ );
				if ( compB )
					compB->_onCollision( compA , avgDepth , avgPtB_ , avgPtA_ , -avgNormalOnB_ );
				break;
			}
		}
	}
}

CollisionDetector::CollisionDetector( btCollisionObject* obj )
{
	mObject = obj;
	mHashPairCache = new (btAlignedAlloc(sizeof(btHashedOverlappingPairCache),16)) btHashedOverlappingPairCache();
}

void CollisionDetector::convexSweepTest( const btConvexShape* castShape, const btTransform& convexFromWorld, const btTransform& convexToWorld, btCollisionWorld::ConvexResultCallback& resultCallback, btScalar allowedCcdPenetration ) const
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
	for (i=0;i<mOverlappingObjects.size();i++)
	{
		btCollisionObject*	collisionObject= mOverlappingObjects[i];
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

void CollisionDetector::rayTest( const btVector3& rayFromWorld, const btVector3& rayToWorld, btCollisionWorld::RayResultCallback& resultCallback ) const
{
	btTransform rayFromTrans;
	rayFromTrans.setIdentity();
	rayFromTrans.setOrigin(rayFromWorld);
	btTransform  rayToTrans;
	rayToTrans.setIdentity();
	rayToTrans.setOrigin(rayToWorld);


	int i;
	for (i=0;i<mOverlappingObjects.size();i++)
	{
		btCollisionObject*	collisionObject= mOverlappingObjects[i];
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

void PhyRigid::applyCentralForce( Vec3D const& f )
{
	getRigidBody()->applyCentralForce( ( 1 / g_GraphicScale ) * convCF2Bullet( f ) );
}

Vec3D PhyRigid::getLinearVelocity() const
{
	return g_GraphicScale * convBullet2CF( getRigidBody()->getLinearVelocity() );
}

Vec3D PhyRigid::getAngularVelocity() const
{
	return g_GraphicScale * convBullet2CF( getRigidBody()->getAngularVelocity() );
}

btRigidBody* PhyRigid::getRigidBody()
{
	return static_cast< btRigidBody* >( mPhyImpl );
}

btRigidBody const* PhyRigid::getRigidBody() const
{
	return const_cast< PhyRigid* >( this )->getRigidBody();
}

PhyRigid::PhyRigid( btRigidBody* body )
	:PhyCollision( body )
{
	mType = PT_RIGID_BODY;
}

PhyRigid* PhyRigid::downCast( PhyCollision* comp )
{
	if ( comp->getPhysicalType() == PT_RIGID_BODY )
		return static_cast< PhyRigid* >( comp );
	return nullptr;
}

void PhyRigid::changeWorldImpl( PhysicsWorld* world , unsigned group , unsigned mask )
{
	world->_getBulletWorld()->removeRigidBody( static_cast< btRigidBody* >( mPhyImpl ) );
	world->addComponent( this , group , mask );
}

PhyCollision::PhyCollision( btCollisionObject* object )
	:IComponent( COMP_PHYSICAL )
{
	mWorld = nullptr;
	mCollisionDetector = nullptr;
	mPhyImpl = object;
	mPhyImpl->setUserPointer( this );
	mType = PT_COLLISION_OBJ;
	mPhyFlag = 0;
}

PhyCollision::~PhyCollision()
{
	delete mCollisionDetector;
	delete mPhyImpl;
}

PhyShapeHandle PhyCollision::getShape() const
{
	return mPhyImpl->getCollisionShape();
}

void PhyCollision::_processCollision( btDynamicsWorld* world )
{
	if ( mCollisionDetector )
		mCollisionDetector->processCollision( world );
}

void PhyCollision::getTransform( Mat4x4& trans ) const
{
	btTransform const& btTrans = mPhyImpl->getWorldTransform();
	btTrans.getOpenGLMatrix( &trans[0] );

	trans[12] *= g_GraphicScale;
	trans[13] *= g_GraphicScale;
	trans[14] *= g_GraphicScale;
}

void PhyCollision::setTransform( Mat4x4 const& trans )
{
	btTransform& btTrans = mPhyImpl->getWorldTransform();

	btTrans.setFromOpenGLMatrix( &trans[0] );
	btTrans.setOrigin( ( 1 / g_GraphicScale ) * btTrans.getOrigin() );
}

void PhyCollision::setPostion( Vec3D const& pos )
{
	mPhyImpl->getWorldTransform().setOrigin( 
		( 1.0f / g_GraphicScale ) * convCF2Bullet( pos ) );
}

void PhyCollision::enableCollisionDetection( bool beE )
{
	if ( beE )
	{
		if ( mCollisionDetector == NULL )
		{
			mCollisionDetector = new CollisionDetector( mPhyImpl );
			if ( mWorld )
				_relinkWorld();
		}
	}
	else
	{
		delete mCollisionDetector;
		mCollisionDetector = nullptr;
	}
}

bool PhyCollision::isSleeping()
{
	return mPhyImpl->getActivationState() == ISLAND_SLEEPING;
}

void PhyCollision::changeWorld( PhysicsWorld* world )
{
	if ( world == mWorld )
		return;

	btBroadphaseProxy* proxy = mPhyImpl->getBroadphaseHandle();
	unsigned group = proxy->m_collisionFilterGroup;
	unsigned mask  = proxy->m_collisionFilterMask;
	changeWorldImpl( world , group , mask );
	mWorld = world;
}

void PhyCollision::setupCollisionFilter( unsigned group , unsigned mask )
{
	static btBroadphaseProxy* proxy = mPhyImpl->getBroadphaseHandle();
	proxy->m_collisionFilterGroup = group;
	proxy->m_collisionFilterMask  = mask;

	if ( mCollisionDetector )
		mCollisionDetector->reset();

	switch( mType )
	{
	case PT_COLLISION_OBJ:
		mWorld->_getBulletWorld()->removeCollisionObject( mPhyImpl );
		mWorld->_getBulletWorld()->addCollisionObject( mPhyImpl , group , mask  );
		break;
	case PT_RIGID_BODY:
		mWorld->_getBulletWorld()->removeRigidBody( static_cast<btRigidBody*>( mPhyImpl ) );
		mWorld->_getBulletWorld()->addRigidBody( static_cast<btRigidBody*>( mPhyImpl ) , group , mask );
		break;
	}
}

void PhyCollision::_relinkWorld()
{
	assert( mWorld );

	static btBroadphaseProxy* proxy = mPhyImpl->getBroadphaseHandle();
	short group = proxy->m_collisionFilterGroup ;
	short mask  = proxy->m_collisionFilterMask ;

	switch( mType )
	{
	case PT_COLLISION_OBJ:
		mWorld->_getBulletWorld()->removeCollisionObject( mPhyImpl );
		mWorld->_getBulletWorld()->addCollisionObject( mPhyImpl , group , mask  );
		break;
	case PT_RIGID_BODY:
		mWorld->_getBulletWorld()->removeRigidBody( static_cast<btRigidBody*>( mPhyImpl ) );
		mWorld->_getBulletWorld()->addRigidBody( static_cast<btRigidBody*>( mPhyImpl ) , group , mask );
		break;
	}

}

void PhyCollision::changeWorldImpl( PhysicsWorld* world , unsigned group , unsigned mask )
{
	world->_getBulletWorld()->removeCollisionObject( mPhyImpl );
	world->addComponent( this , group , mask );
}

bool PhysicsSystem::initSystem()
{
	return true;
}

PhysicsWorld* PhysicsSystem::createWorld()
{
	return new PhysicsWorld;
}

void PhysicsSystem::destroyWorld( PhysicsWorld* world )
{
	delete world;
}

PhyShapeHandle PhysicsSystem::createShape( PhyShapeParams const& info )
{
	float s = 1.0f / g_GraphicScale;
	btCollisionShape* shape = NULL;
	switch( info.type )
	{
	case CSHAPE_BOX:
		shape = new btBoxShape( 
			0.5f * btVector3( info.box.x , info.box.y , info.box.z ) );
		break;
	case CSHAPE_SPHERE:
		shape = new btSphereShape( info.sphere.radius );
		break;
	case CSHAPE_TRIANGLE_MESH:
		shape = createTriangleMeshShape( info , NULL );
		break;
	}
	shape->setLocalScaling( btVector3( s , s , s ) );
	return shape;
}



void PhysicsSystem::destroyShape( PhyShapeHandle shape )
{
	btCollisionShape* btShape = static_cast< btCollisionShape* >( shape );
	
	if ( !btShape )
		return;

	switch( btShape->getShapeType() )
	{
	case TRIANGLE_MESH_SHAPE_PROXYTYPE:
		{
			btBvhTriangleMeshShape* meshShape = static_cast< btBvhTriangleMeshShape* >( btShape );
			delete meshShape->getTriangleInfoMap();
		}
		break;
	}

	delete btShape;
}


void PhysicsSystem::getShapeParamInfo( PhyShapeHandle shape , PhyShapeParams& info )
{
	btCollisionShape* btShape = static_cast< btCollisionShape* >( shape );

	switch( btShape->getShapeType() )
	{
	case BOX_SHAPE_PROXYTYPE:
		{
			btBoxShape* box = static_cast< btBoxShape* >( btShape );
			info.type = CSHAPE_BOX;
			btVector3 len = box->getHalfExtentsWithoutMargin();
			info.box.x = g_GraphicScale * 2 * len.x();
			info.box.y = g_GraphicScale * 2 * len.y();
			info.box.z = g_GraphicScale * 2 * len.z();
		}
		return;
	case SPHERE_SHAPE_PROXYTYPE:
		{
			btSphereShape* sphere = static_cast< btSphereShape* >( btShape );
			info.type = CSHAPE_SPHERE;
			info.sphere.radius = g_GraphicScale * sphere->getRadius();
		}
		return;
	}
}



btCollisionShape* PhysicsSystem::createTriangleMeshShape( PhyShapeParams const& info , char const* bvhFile )
{
	btTriangleIndexVertexArray* indexVertexArrays = new btTriangleIndexVertexArray(
		info.triMesh.numTriangles , 
		info.triMesh.triangleIndex ,
		info.triMesh.triangleIndexStride ,
		info.triMesh.numVertices ,
		info.triMesh.vertex ,
		info.triMesh.vertexStride );

	bool useQuantizedAabb = true;

	btBvhTriangleMeshShape* trimeshShape;
	FILE* file = NULL;
	if ( bvhFile )
		file = fopen( bvhFile , "rb");

	if ( file != NULL )
	{
		trimeshShape  = new btBvhTriangleMeshShape(indexVertexArrays,useQuantizedAabb,false);
		int size=0;
		btOptimizedBvh* bvh = 0;
		if (fseek(file, 0, SEEK_END) || (size = ftell(file)) == EOF || fseek(file, 0, SEEK_SET)) 
		{       
			/* File operations denied? ok, just close and return failure */
			printf("Error: cannot get filesize from %s\n", bvhFile );
			//exit(0);
		} 
		else
		{

			fseek(file, 0, SEEK_SET);

			int buffersize = size + btOptimizedBvh::getAlignmentSerializationPadding();

			void* buffer = btAlignedAlloc(buffersize,16);
			int read = fread(buffer,1,size,file);
			fclose(file);
			bool swapEndian = false;
			bvh = btOptimizedBvh::deSerializeInPlace(buffer,buffersize,swapEndian);
			trimeshShape->setOptimizedBvh(bvh);
		}
	}
	else
	{
		//float boundmax = 1e16;
		//btVector3 aabbMin(-boundmax,-boundmax,-boundmax);
		//btVector3 aabbMax(boundmax,boundmax,boundmax);

		btVector3 aabbMin(-12000,-12000,-12000),aabbMax(12000,12000,12000);

		//trimeshShape  = new btBvhTriangleMeshShape(indexVertexArrays,useQuantizedAabb,aabbMin,aabbMax);

		trimeshShape  = new btBvhTriangleMeshShape(indexVertexArrays,useQuantizedAabb );
	}

	btTriangleInfoMap* triangleInfoMap = new btTriangleInfoMap();
	btGenerateInternalEdgeInfo(trimeshShape,triangleInfoMap);
	return trimeshShape;
}

bool PhysicsSystem::saveTriMeshBVHFile( PhyShapeHandle shape ,char const* path )
{
	btCollisionShape* btShape = static_cast< btCollisionShape* >( shape );
	if ( btShape->getShapeType() != TRIANGLE_MESH_SHAPE_PROXYTYPE )
		return false;

	btBvhTriangleMeshShape* triMeshShape = static_cast< btBvhTriangleMeshShape* >( btShape );

	void* buffer = 0;
	int numBytes = triMeshShape->getOptimizedBvh()->calculateSerializeBufferSize();
	buffer = btAlignedAlloc(numBytes,16);
	bool swapEndian = false;
	triMeshShape->getOptimizedBvh()->serialize(buffer,numBytes,swapEndian);
	FILE* file = fopen( path ,"wb" );
	fwrite(buffer,1,numBytes,file);
	fclose(file);
	btAlignedFree(buffer);

	return true;
}

//void build()
//{
//	struct TriIndex
//	{
//		int v[3];
//	};
//
//	std::vector< tri_t > m_triVec;
//	std::vector< Vec3D > m_vertexVec;
//
//	class MyConvexDecomposition : public ConvexDecomposition::ConvexDecompInterface
//	{
//	public:
//
//		int   	mBaseCount;
//		int		mHullCount;
//		FILE*	mOutputFile;
//
//		btAlignedObjectArray<btConvexHullShape*> m_convexShapes;
//		btAlignedObjectArray<btVector3> m_convexCentroids;
//
//		MyConvexDecomposition ()
//			:mBaseCount(0)
//			,mHullCount(0)
//
//		{
//		}
//
//		virtual void ConvexDecompResult(ConvexDecomposition::ConvexResult &result)
//		{
//
//			btVector3	centroid(0,0,0);
//
//			btTriangleMesh* trimesh = new btTriangleMesh();
//
//			btVector3 localScaling(1.f,1.f,1.f);
//
//			btAlignedObjectArray<btVector3> vertices;
//			if ( 1 )
//			{
//				//const unsigned int *src = result.mHullIndices;
//				for (unsigned int i=0; i<result.mHullVcount; i++)
//				{
//					btVector3 vertex(result.mHullVertices[i*3],result.mHullVertices[i*3+1],result.mHullVertices[i*3+2]);
//					vertex *= localScaling;
//					centroid += vertex;
//
//				}
//			}
//
//			centroid *= 1.f/(float(result.mHullVcount) );
//
//			if ( 1 )
//			{
//				//const unsigned int *src = result.mHullIndices;
//				for (unsigned int i=0; i<result.mHullVcount; i++)
//				{
//					btVector3 vertex(result.mHullVertices[i*3],result.mHullVertices[i*3+1],result.mHullVertices[i*3+2]);
//					vertex *= localScaling;
//					vertex -= centroid ;
//					vertices.push_back(vertex);
//				}
//			}
//
//
//
//			if ( 1 )
//			{
//				const unsigned int *src = result.mHullIndices;
//				for (unsigned int i=0; i<result.mHullTcount; i++)
//				{
//					unsigned int index0 = *src++;
//					unsigned int index1 = *src++;
//					unsigned int index2 = *src++;
//
//
//					btVector3 vertex0(result.mHullVertices[index0*3], result.mHullVertices[index0*3+1],result.mHullVertices[index0*3+2]);
//					btVector3 vertex1(result.mHullVertices[index1*3], result.mHullVertices[index1*3+1],result.mHullVertices[index1*3+2]);
//					btVector3 vertex2(result.mHullVertices[index2*3], result.mHullVertices[index2*3+1],result.mHullVertices[index2*3+2]);
//					vertex0 *= localScaling;
//					vertex1 *= localScaling;
//					vertex2 *= localScaling;
//
//					vertex0 -= centroid;
//					vertex1 -= centroid;
//					vertex2 -= centroid;
//
//
//					trimesh->addTriangle(vertex0,vertex1,vertex2);
//
//					index0+=mBaseCount;
//					index1+=mBaseCount;
//					index2+=mBaseCount;
//
//				}
//			}
//
//			float mass = 1.f;
//			//float collisionMargin = 0.01f;
//
//			//this is a tools issue: due to collision margin, convex objects overlap, compensate for it here:
//			//#define SHRINK_OBJECT_INWARDS 1
//#ifdef SHRINK_OBJECT_INWARDS
//
//			std::vector<btVector3> planeEquations;
//			btGeometryUtil::getPlaneEquationsFromVertices(vertices,planeEquations);
//
//			std::vector<btVector3> shiftedPlaneEquations;
//			for (int p=0;p<planeEquations.size();p++)
//			{
//				btVector3 plane = planeEquations[p];
//				plane[3] += 5*collisionMargin;
//				shiftedPlaneEquations.push_back(plane);
//			}
//			std::vector<btVector3> shiftedVertices;
//			btGeometryUtil::getVerticesFromPlaneEquations(shiftedPlaneEquations,shiftedVertices);
//
//
//			btConvexHullShape* convexShape = new btConvexHullShape(&(shiftedVertices[0].getX()),shiftedVertices.size());
//
//#else //SHRINK_OBJECT_INWARDS
//
//			btConvexHullShape* convexShape = new btConvexHullShape(&(vertices[0].getX()),vertices.size());
//#endif 
//
//			convexShape->setMargin(0.01);
//			m_convexShapes.push_back(convexShape);
//			m_convexCentroids.push_back(centroid);
//
//			mBaseCount+=result.mHullVcount; // advance the 'base index' counter.
//		}
//
//
//	};
//
//
//
//
//	unsigned int depth = 5;
//	float cpercent     = 5;
//	float ppercent     = 15;
//	unsigned int maxv  = 16;
//	float skinWidth    = 0.0;
//
//	ConvexDecomposition::DecompDesc desc;
//	desc.mVcount       = m_vertexVec.size();
//	desc.mVertices     = &m_vertexVec[0].x;
//	desc.mTcount       = m_triVec.size();
//	desc.mIndices      = ( unsigned int*) &m_triVec[0].v[0];
//	desc.mDepth        = depth;
//	desc.mCpercent     = cpercent;
//	desc.mPpercent     = ppercent;
//	desc.mMaxVertices  = maxv;
//	desc.mSkinWidth    = skinWidth;
//
//	MyConvexDecomposition	convexDecomposition;
//	desc.mCallback = &convexDecomposition;
//
//
//
//	//convexDecomposition.performConvexDecomposition(desc);
//
//	ConvexBuilder cb(desc.mCallback);
//	cb.process(desc);
//
//	//now create some bodies
//
//	{
//		btCompoundShape* compound = new btCompoundShape(false);
//
//		//m_collisionShapes.push_back (compound);
//
//		btTransform trans;
//		trans.setIdentity();
//		for (int i=0;i<convexDecomposition.m_convexShapes.size();i++)
//		{
//
//			btVector3 centroid = convexDecomposition.m_convexCentroids[i];
//			trans.setOrigin(centroid);
//			btConvexHullShape* convexShape = convexDecomposition.m_convexShapes[i];
//			compound->addChildShape(trans,convexShape);
//		}
//		btScalar mass=10.f;
//	}
//}

#include "CFObject.h"

void PhysicsDebugDrawer::pushVertex( const btVector3& v , const btVector3& color )
{
	mVertex.push_back( g_GraphicScale * v.x() );
	mVertex.push_back( g_GraphicScale * v.y() );
	mVertex.push_back( g_GraphicScale * v.z() );
	mVertex.push_back( color.x() );
	mVertex.push_back( color.y() );
	mVertex.push_back( color.z() );
}

void PhysicsDebugDrawer::drawLine( const btVector3& from,const btVector3& to,const btVector3& color )
{
	pushVertex( from , color );
	pushVertex( to   , color );
}


bool PhysicsDebugDrawer::beginRender()
{
	mVertex.clear();
	return  mDbgObject != 0;
}

void PhysicsDebugDrawer::endRender()
{
	if ( mDbgObject )
	{
		mDbgObject->removeAllElement();

		if ( !mVertex.empty() )
			mDbgObject->createLines( nullptr , CFly::CF_LINE_SEGMENTS , CFly::CFVT_XYZ_CF1 , &mVertex[0] ,  mVertex.size() / 6  );
	}
}
