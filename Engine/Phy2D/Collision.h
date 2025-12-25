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

	// Box2D style: ManifoldPoint stores per-contact-point data
	struct ManifoldPoint
	{
		Vector2 posLocal[2];  // Local position on each body
		Vector2 pos[2];       // World position on each body
		float   depth;        // Penetration depth at this point

		// Accumulated impulses for warm starting (per-point)
		float   normalImpulse;
		float   tangentImpulse;

		ManifoldPoint()
			: depth(0)
			, normalImpulse(0)
			, tangentImpulse(0)
		{
		}
	};

	// Contact stores shared data for a collision pair
	struct Contact
	{
		CollideObject* object[2];
		Vector2 normal;       // Collision normal (A -> B)
	};

	// Box2D style ContactManifold: supports up to 2 contact points for 2D
	static constexpr int MaxManifoldPoints = 2;

	class ContactManifold
	{
	public:

		ContactManifold()
		{
			mNumContacts = 0;
		}

		void addContact(Contact const& c, ManifoldPoint const& point)
		{
			mContact = c;
			if (mNumContacts == 0)
			{
				mAge = 0;
			}
			// Add point if we have room
			if (mNumContacts < MaxManifoldPoints)
			{
				mPoints[mNumContacts] = point;
				mNumContacts++;
			}
		}

		void setContact(Contact const& c, ManifoldPoint const* points, int numPoints)
		{
			mContact = c;
			mNumContacts = Math::Min(numPoints, MaxManifoldPoints);
			for (int i = 0; i < mNumContacts; ++i)
			{
				mPoints[i] = points[i];
			}
			mAge = 0;
		}

		// Legacy single-point interface for backward compatibility
		void addContact(Contact const& c)
		{
			ManifoldPoint point;
			// Note: This requires the old Contact format with posLocal/pos/depth
			// For now, we'll handle this in the collision detection code
			mContact.object[0] = c.object[0];
			mContact.object[1] = c.object[1];
			mContact.normal = c.normal;
			mAge = 0;
			mNumContacts = 0;  // Will be set by collision algorithm
		}

		bool update()
		{
			if (mNumContacts == 0)
				return false;

			++mAge;
			if (mAge != 1)
			{
				return false;
			}
			return true;
		}

		void reset()
		{
			mNumContacts = 0;
			for (int i = 0; i < MaxManifoldPoints; ++i)
			{
				mPoints[i].normalImpulse = 0;
				mPoints[i].tangentImpulse = 0;
			}
			impulse = 0;
			tangentImpulse = 0;
		}

		// Per-manifold velocity bias (computed from max relative velocity)
		float   velocityBias;
		float   impulse;
		float   tangentImpulse;
		int     mNumContacts;
		int     mAge;
		Contact mContact;
		ManifoldPoint mPoints[MaxManifoldPoints];

		// Added for Position Solver stability
		Vector2       localNormal;  // Normal in reference body's local space
		Vector2       localPoint;   // Reference point on reference body (optional, for future use)
		bool          refIsA;       // Flag to store which body is reference
	};

	class CollisionAlgo
	{
	public:
		virtual ~CollisionAlgo() = default;
		
		// Legacy single-point test (for backward compatibility)
		virtual bool test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c, ManifoldPoint& point) = 0;
		
		// Multi-point manifold test - default implementation uses single-point test
		virtual bool testManifold(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, ContactManifold& manifold)
		{
			Contact c;
			ManifoldPoint point;
			if (test(objA, shapeA, objB, shapeB, c, point))
			{
				manifold.mContact = c;
				manifold.mPoints[0] = point;
				manifold.mNumContacts = 1;
				manifold.mAge = 0;
				// Default local normal logic (assume A is reference for simplification in legacy path)
				manifold.refIsA = true;
				manifold.localNormal = objA.mXForm.transformVectorInv(c.normal);
				return true;
			}
			return false;
		}
		
		bool testManifold(CollideObject& objA, CollideObject& objB, ContactManifold& manifold)
		{
			return testManifold(objA, *objA.mShape, objB, *objB.mShape, manifold);
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

		bool testManifold(CollideObject* objA, CollideObject* objB, ContactManifold& manifold)
		{                                                                
			int index = ToIndex(objA->mShape->getType(), objB->mShape->getType());
			CollisionAlgo* algo = mMap[index];
			return algo->testManifold(*objA, *objB, manifold);
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

		void  buildContact(Edge* e, Contact& c, ManifoldPoint& point);

		Edge* addEdge(Vertex* sv, Vector2 const& b);
		void  updateEdge(Edge* e, Vector2 const& b);
		Edge* insertEdge(Edge* cur, Vertex* sv);
		Edge* getClosetEdge();

		void generateContact(Contact& c, ManifoldPoint& point);


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
