#ifndef TPhySystem_h__
#define TPhySystem_h__

#include "common.h"
#include "singleton.hpp"



enum CollisionFilter
{
	COF_NORMAL  = BIT(0) , //No Collision CallBack
	COF_ACTOR   = BIT(1) ,
	COF_OBJECT  = BIT(2) ,
	COF_TRIGGER = BIT(3) ,
	COF_WATER   = BIT(4),
	COF_TERRAIN = BIT(5) ,

	COF_ALL     = -1,
	COF_SOILD = COF_TERRAIN |  COF_ACTOR | COF_OBJECT | COF_WATER ,
	
};

class  TPhyEntity;
struct FlyModelDataInfo;
struct GameOverlapFilterCallback;

typedef btCollisionWorld::ClosestConvexResultCallback ClosestConvexResultCallback;
typedef btCollisionWorld::ClosestRayResultCallback    ClosestRayResultCallback;
typedef btCollisionWorld::LocalRayResult              LocalRayResult;
typedef btCollisionWorld::LocalConvexResult           LocalConvexResult;


class TPhySystem : public Singleton<TPhySystem>
{
public:
	TPhySystem();


	btDynamicsWorld* getDynamicsWorld() const { return m_dynamicsWorld; }

	void stepSimulation(Float time);


	void processCollision( TPhyBody& body );

	void removeEntity( TPhyEntity* entity );
	void addEntity( TPhyEntity* entity );

	void addTerrain( btRigidBody* terrain );
	void addPhyBody( TPhyBody& body )
	{

	}

	int getSleepObjNum();
	void enableSumulation( bool beE ){ m_enable = beE; }

protected:

	bool m_enable;

	btDiscreteDynamicsWorld*   m_dynamicsWorld;
	btBroadphaseInterface*	   m_broadphase;
	btCollisionDispatcher*	   m_dispatcher;
	btConstraintSolver*	       m_solver;
	btDefaultCollisionConfiguration* m_collisionConfiguration;

};

struct TraceResult
{
	CollisionFilter surface;
	TPhyEntity*     entity;
	TPhyEntity*     ignore;

	int triID;
};


class TraceCallback : public ClosestRayResultCallback
	                 ,public TraceResult
{
public:
	TraceCallback();
	btScalar     addSingleResult( LocalRayResult& rayResult , bool normalInWorldSpace);
	Vec3D const& getHitPos(){ return m_hitPointWorld; }
	Vec3D const& getHitNormal(){ return m_hitNormalWorld; }
};

class ConvexCallback : public ClosestConvexResultCallback
	                 , public TraceResult
{
public:
	ConvexCallback(btCollisionObject* me , Vec3D const& from,  Vec3D const& to);
	virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace);
protected:
	btCollisionObject* m_me;
};


bool RayLineTest(Vec3D const& from , Vec3D const& to , unsigned ColFilterbit , TPhyEntity* ignore , TraceCallback* callback );
bool ConvexSweepTest( btConvexShape* shape , Vec3D const& from , Vec3D const& to , unsigned ColFilterbit , ConvexCallback* callback  );
bool ConvexSweepTest( TPhyBody* phyBody  , Vec3D const& from , Vec3D const& to , unsigned ColFilterbit , ConvexCallback* callback  );


#endif // TPhySystem_h__