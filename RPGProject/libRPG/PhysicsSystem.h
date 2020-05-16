#ifndef PhysicsSystem_h__
#define PhysicsSystem_h__

#include "common.h"

#include "CEntity.h"

#include "LinearMath/btVector3.h"
#include "LinearMath/btMatrix3x3.h"
#include "LinearMath/btQuaternion.h"
#include "LinearMath/btTransform.h"

#include "FastDelegate/FastDelegate.h"

class btDiscreteDynamicsWorld;
class btDynamicsWorld;
class btBroadphaseInterface;
class btCollisionDispatcher;
class btConstraintSolver;
class btDefaultCollisionConfiguration;
class btCollisionObject;
class btCollisionShape;
class btRigidBody;
class btDispatcher;
class btHashedOverlappingPairCache;


typedef void* PhyShapeHandle;

struct btBroadphaseProxy;


enum PhyShapeType
{
	CSHAPE_BOX ,
	CSHAPE_SPHERE ,
	CSHAPE_CYLINDER ,
	CSHAPE_TRIANGLE_MESH ,

	CSHPAE_UNKNOW ,
};


enum CollisionFilter
{
	COF_DEFULT  = BIT(0) ,
	COF_ACTOR   = BIT(1) ,
	COF_OBJECT  = BIT(2) ,
	COF_TRIGGER = BIT(3) ,
	COF_WATER   = BIT(4) ,

	COF_TERRAIN = BIT(5) ,
	COF_STATIC  = BIT(6) ,

	COF_ALL     = -1,
	COF_SOILD = COF_TERRAIN |  COF_ACTOR | COF_OBJECT | COF_WATER ,
};


struct PhyShapeParams
{
	struct Sphere   {  float radius;  };
	struct Box      {  float x , y , z;  };
	struct Cylinder {  float radius , height;  };
	struct TriMesh
	{
		int    numTriangles;
		int*   triangleIndex;
		int    triangleIndexStride;
		int    numVertices;
		float* vertex;
		int    vertexStride;
	};

	PhyShapeParams()
	{
		type = CSHPAE_UNKNOW;
	}

	PhyShapeType type;
	union
	{
		Sphere   sphere;
		Box      box;
		Cylinder cylinder;
		TriMesh  triMesh;
	};

	void setBoxShape( Vec3D const& len )
	{
		type = CSHAPE_BOX;
		box.x = len.x;
		box.y = len.y;
		box.z = len.z;
	}
	void setSphereShape( float radius )
	{
		type = CSHAPE_SPHERE;
		sphere.radius = radius;
	}
	void setCylinder( float radius , float height )
	{
		type = CSHAPE_CYLINDER;
		cylinder.radius = radius;
		cylinder.height = height;
	}
};

class PhyCollision;
class PhyRigid;
class CollisionDetector;

#include "LinearMath/btIDebugDraw.h"
class PhysicsDebugDrawer : public btIDebugDraw
{

public:
	PhysicsDebugDrawer()
	{
		m_debugMode = btIDebugDraw::DBG_DrawWireframe;
		mDbgObject = 0;
	}
	virtual void  drawLine(const btVector3& from,const btVector3& to,const btVector3& color);
	virtual void  drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color){}
	virtual void  reportErrorWarning(const char* warningString){}
	virtual void  draw3dText(const btVector3& location,const char* textString){}
	virtual void  setDebugMode(int debugMode){ m_debugMode = debugMode; }
	virtual int	  getDebugMode() const { return m_debugMode;}

	virtual bool  beginRender();
	virtual void  endRender();

	void  setObject( CFly::Object* obj ){  mDbgObject = obj;  }

	virtual void addSegments( Vec3D const& p1 , Vec3D const& p2 , Vec3D const& color ){}
	virtual void addTriangle( Vec3D const& p1 , Vec3D const& p2 , Vec3D const& p3 , Vec3D const& color ){}
private:	
	int m_debugMode;
	void pushVertex( const btVector3& v , const btVector3& color);
	std::vector< float > mVertex;
	CFly::Object* mDbgObject;
};


enum
{
	PHY_ACTOR_OBJ          = BIT(0),
	PHY_COLLISION_DETCTION = BIT(1),
};


struct RayTraceResult
{
	bool  haveHit;
	CollisionFilter      surface;
	PhyCollision*  entity;
	bool  hitIgnore;
	Vec3D hitPos;
	Vec3D hitNormal;
	float hitFraction;
};

enum PhysicalType
{
	PT_COLLISION_OBJ ,
	PT_RIGID_BODY    ,
	PT_SOFT_BODY     ,
};


class PhysicsWorld
{
public:
	PhysicsWorld();
	~PhysicsWorld();

	void enableDebugDraw( bool beDraw );
	void update( long time );

	void setGravity( Vec3D const& value );
	bool testRayCollision( Vec3D const& from , Vec3D const& to , unsigned colFilterbit , PhyCollision* ignore , RayTraceResult& result );
	bool testConvexTest();

	PhyCollision* createCollisionComponent( 
		PhyShapeHandle shape , unsigned flag = 0 , 
		unsigned group = COF_DEFULT , unsigned mask = COF_ALL );
	PhyRigid*     createRigidComponent(  
		PhyShapeHandle shape , float mass , unsigned flag = 0 , 
		unsigned group = COF_DEFULT , unsigned mask = COF_ALL  );

	PhysicsDebugDrawer*  getDebugDrawer(){  return mDbgDrawer;  }

	void renderDebugDraw();

	void addComponent( PhyCollision* comp , unsigned group  , unsigned mask  );
	void addComponent( PhyRigid* comp , unsigned group  , unsigned mask );

public:
	btDiscreteDynamicsWorld* _getBulletWorld(); 
private:



	void detectCollision();
	void configComponent( PhyCollision* comp , unsigned flag );
	PhysicsDebugDrawer*        mDbgDrawer;
	btDiscreteDynamicsWorld*   mDynamicsWorld;
	btBroadphaseInterface*	   mBroadphase;
	btCollisionDispatcher*	   mDispatcher;
	btConstraintSolver*	       mSolver;
	btDefaultCollisionConfiguration* mCollisionConfiguration;
};

#include "Singleton.h"
class PhysicsSystem : public SingletonT< PhysicsSystem >
{
public:

	bool initSystem();
	void closeSystem()
	{

	}

	PhyShapeHandle createShape( PhyShapeParams const& info );
	void           destroyShape( PhyShapeHandle shape );
	PhysicsWorld*  createWorld();
	void           destroyWorld( PhysicsWorld* world );

	static void getShapeParamInfo( PhyShapeHandle shape , PhyShapeParams& info );
	static bool saveTriMeshBVHFile( PhyShapeHandle shape ,char const* path );
private:
	static btCollisionShape* createTriangleMeshShape( PhyShapeParams const& info , char const* bvhFile );
	
	typedef std::vector< PhyShapeHandle > ShapeList;
};

#include "LinearMath/btAlignedObjectArray.h"

class CollisionDetector;

class PhyEntity
{
public:
	PhysicalType  getPhysicalType(){ return mType; }
	PhysicalType   mType;
};

class PhysicalComponent : public IComponent
{
public:
	PhyEntity* getPhyEntity( int slot );
	PhyEntity* createCollisionEntity( int slot , PhysicsWorld* world );

};


class PhyCollision : public IComponent
{
public:
	~PhyCollision();

	typedef fastdelegate::FastDelegate< 
		void ( PhyCollision* , float , Vec3D const& ,Vec3D const& , Vec3D const& ) 
	> CollisionCallback;

	void  getTransform( Mat4x4& trans ) const;
	void  setTransform( Mat4x4 const& trans );
	void  setPostion( Vec3D const& pos );

	void  setupCollisionFilter( unsigned group , unsigned mask );
	void  changeWorld( PhysicsWorld* world );

	bool  isSleeping();
	void  enableCollisionDetection( bool beE = true );

	template< class TFun >
	void  setCollisionCallback( TFun func )               {  mCollisionCB.bind( func );  }
	template< class T , class TFun >
	void  setCollisionCallback( T* component , TFun func ){  mCollisionCB.bind( component , func );  }
	void  clearCallback(){  mCollisionCB.clear();  }


public:
	CollisionDetector*    _getOverlapDetector()
	{
		return mCollisionDetector;
	}


	void _processCollision( btDynamicsWorld* world );

	void _onCollision( PhyCollision* comp , float depth , Vec3D const& posSelf , Vec3D const& posOther , Vec3D const& normalOnOther )
	{
		if ( mCollisionCB )
			mCollisionCB( comp , depth , posSelf , posOther , normalOnOther );
	}

	PhyShapeHandle getShape() const;

	PhysicsWorld* getWorld(){ return mWorld; }
	PhysicalType  getPhysicalType(){ return mType; }

protected:
	virtual void changeWorldImpl( PhysicsWorld* world , unsigned group , unsigned mask );
	void _relinkWorld();

	CollisionCallback  mCollisionCB;
	PhysicalType       mType;
	btCollisionObject* mPhyImpl;
	CollisionDetector* mCollisionDetector;
	PhysicsWorld*      mWorld;
	unsigned           mPhyFlag;

	friend class PhysicsWorld;

	PhyCollision( btCollisionObject* pImpl );
};

class PhyRigid : public PhyCollision
{
public:
	Vec3D getLinearVelocity() const;
	Vec3D getAngularVelocity() const;
	void  applyCentralForce( Vec3D const& f );

	float getMass() const;
	static PhyRigid* downCast( PhyCollision* comp );

	virtual void changeWorldImpl( PhysicsWorld* world , unsigned group , unsigned mask );
protected:

	btRigidBody*       getRigidBody();
	btRigidBody const* getRigidBody() const;

	friend class PhysicsWorld;
	PhyRigid( btRigidBody* body );
};

#endif // PhysicsSystem_h__