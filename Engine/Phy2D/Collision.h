#ifndef Collision_h__D60B683C_C710_4755_B3C8_9B0F35FCEDA9
#define Collision_h__D60B683C_C710_4755_B3C8_9B0F35FCEDA9

#include "Phy2D/Base.h"
#include "Phy2D/Shape.h"

#include "DataStructure/IntrList.h"
#include "DataStructure/Array.h"

#include <algorithm>

namespace Phy2D
{

	struct CollisionProxy;
	struct ProxyPair;

	class PhyObject
	{
	public:
		enum PhyType
		{
			eCollideType ,
			eRigidType ,
		};
		virtual PhyType getType() const = 0;
	};

	class CollideObject : public PhyObject
	{
	public:
		CollideObject()
		{
			mShape = NULL;
			mProxy = NULL;
		}
		virtual PhyType getType() const { return PhyObject::eCollideType; }
		Vector2 const& getPos() const { return mXForm.getPos(); }
		Rotation2D const& getRotation() const { return mXForm.getRotation(); }
		void         setPos( Vector2 const& p ){ mXForm.setTranslation( p ); }

		AABB  getAABB()
		{
			AABB result;
			mShape->calcAABB(mXForm, result);
			return result;
		}

		Vector2 getSupport( Vector2 const& dir ) const
		{
			Vector2 pos = mShape->getSupport( mXForm.transformVectorInv( dir ) );
			return mXForm.transformPosition( pos );
		}

		Shape* getShape(){ return mShape; }

		XForm2D  mXForm;
		Shape* mShape;
		CollisionProxy* mProxy;
		LinkHook hook;
	};

	struct Contact
	{
		CollideObject* object[2];
		Vector2 posLocal[2];
		Vector2 pos[2];
		Vector2 normal;
		float depth;
		
	};

	class ContactManifold
	{
	public:

		ContactManifold()
		{
			impulse = 0;
			mNumContact = 0;
		}
		void addContact( Contact const& c )
		{
			mContect = c;
			mAge = 0;
			if ( mNumContact == 0 )
				impulse = 0;
			mNumContact = 1;
		}

		bool update()
		{
			if ( mNumContact == 0 )
				return false;

			++mAge;
			if ( mAge != 1 )
			{
				Contact& c = mContect;
				return false;
			}
			return true;
		}

		float   impulse;
		float   velocityBias;
		int     mNumContact;
		int     mAge;
		Contact mContect;
	};

	class CollisionAlgo
	{
	public:
		virtual ~CollisionAlgo() = default;
		virtual bool test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c) = 0;
		
		bool test(CollideObject& objA, CollideObject& objB, Contact& c)
		{
			return test(objA, *objA.mShape, objB, *objB.mShape, c);
		}
	};

	struct CollisionProxy
	{
		CollideObject*  object;
		AABB aabb;

		LinkHook hook;
		TArray< ProxyPair* > pairs;
		void remove( ProxyPair* pair )
		{
			pairs.erase( std::find( pairs.begin() , pairs.end() , pair ) );
		}
	};



	struct ProxyPair
	{
		CollisionProxy* proxy[2];
		ContactManifold manifold;
		LinkHook hook;
	};


	typedef TIntrList< ProxyPair , MemberHook< ProxyPair , &ProxyPair::hook > , PointerType > ProxyPairList;

	class ContactManager
	{
	public:
		ProxyPair* findProxyPair( CollisionProxy* proxyA , CollisionProxy* proxyB );
		bool addProxyPair( CollisionProxy* proxyA , CollisionProxy* proxyB );
		bool removeProxyPair( CollisionProxy* proxyA , CollisionProxy* proxyB );

		
		ProxyPairList mProxyList;
	};

	class Broadphase
	{
	public:
		Broadphase()
		{
			mContactBreakThreshold = 0.1;
		}
		void addObject( CollideObject* obj )
		{
			CollisionProxy* proxy = new CollisionProxy;
			proxy->object = obj;
			obj->mProxy = proxy;
			mProxys.push_back( proxy );
		}
		void removeObject( CollideObject* obj )
		{
			CollisionProxy* proxy = obj->mProxy;
			if ( proxy )
			{
#if 1
				proxy->hook.unlink();
#else
				mProxys.remove(proxy);
#endif
				delete proxy;
			}
		}

		void process( ContactManager& pairManager , float dt );
		float mContactBreakThreshold;
	
#if 1
		typedef TIntrList< 
			CollisionProxy , 
			MemberHook< CollisionProxy , &CollisionProxy::hook > , 
			PointerType 
		> ProxyList;		
#else
		using ProxyList = TArray< CollisionProxy* >;
#endif
		ProxyList mProxys;
	};

	class CollisionManager
	{
	public:
		CollisionManager();

		void addObject( CollideObject* obj )
		{
			mBroadphase.addObject( obj );
		}
		void removeObject( CollideObject* obj )
		{
			mBroadphase.removeObject( obj );
		}

		bool test( CollideObject* objA , CollideObject* objB , Contact& c )
		{                                                                
			int index = ToIndex(objA->mShape->getType(), objB->mShape->getType());
			CollisionAlgo* algo = mMap[index];
			return algo->test( *objA , *objB , c );
		}

		static int ToIndex(Shape::Type a, Shape::Type b)
		{
			if (a > b)
			{
				std::swap(a,b);
			}
			return a * ( a + 1 ) / 2 + b;
		}
		void preocss( float dt );
		
		CollisionAlgo*   mDefaultConvexAlgo;
		CollisionAlgo*   mMap[ Shape::NumShape * Shape::NumShape / 2 ];
		ContactManager   mPairManager;
		Broadphase       mBroadphase;
		TArray< ContactManifold* > mMainifolds;
	};

	class MinkowskiBase
	{
	public:

		struct Vertex
		{
			Vector2 v;
			Vector2 dir;
		};
		struct Edge
		{
			Vector2  normal;
			float    depth;
			Edge*    next;
			Vertex* sv;
		};

		void init(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB);

		Vertex* calcSupport(Vertex* sv, Vector2 const& dir);

		Vector2 getSupport(Vector2 const& dir) const
		{
			Vector2 v0 = mShapes[0]->getSupport(dir);
			Vector2 v1 = mShapes[1]->getSupport(mBToALocal.transformVectorInv(-dir));
			return v0 - mBToALocal.transformPosition(v1);
		}

		void  buildContact(Edge* e, Contact &c);

		Edge* addEdge(Vertex* sv, Vector2 const& b);
		void  updateEdge(Edge* e, Vector2 const& b);
		Edge* insertEdge(Edge* cur, Vertex* sv);
		Edge* getClosetEdge();

		void generateContact(Contact& c);


		XForm2D     mBToALocal;

		static int const MaxIterNum = 20;
		int       mNumEdge;
		Edge      mEdges[MaxIterNum + 3];
		int       mNumSimplex;
		Vertex    mStorage[MaxIterNum + 3];
		Vertex*   mSv[3];
		CollideObject*  mObjects[2];
		Shape*          mShapes[2];

#if PHY2D_DEBUG
		TArray< Vertex > mDBG;
#endif
	};


	class GJK : public MinkowskiBase
	{
	public:
		GJK();
		void init(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB);
		bool test();

	};

	class MPR : public MinkowskiBase
	{
	public:
		void init(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB);

		Vector2 getPointInside() const;

		bool test();
	};

#if PHY2D_DEBUG
	MinkowskiBase& GetGJK();
	#define gGJK GetGJK()
#endif

}//namespace Phy2D

#endif // Collision_h__D60B683C_C710_4755_B3C8_9B0F35FCEDA9
