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
		float   velParam;
		int     mNumContact;
		int     mAge;
		Contact mContect;
	};

	class CollisionAlgo
	{
	public:
		virtual bool test( CollideObject* objA , CollideObject* objB , Contact& c ) = 0;
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
				proxy->hook.unlink();
				delete proxy;
			}
		}

		void process( ContactManager& pairManager , float dt );
		float mContactBreakThreshold;
	
		typedef TIntrList< 
			CollisionProxy , 
			MemberHook< CollisionProxy , &CollisionProxy::hook > , 
			PointerType 
		> ProxyList;

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
			int index = Math::PairingFunction<int>(objA->mShape->getType(), objB->mShape->getType());
			CollisionAlgo* algo = mMap[index];
			return algo->test( objA , objB , c );
		}

		void preocss( float dt );
		
		CollisionAlgo*   mDefaultConvexAlgo;
		CollisionAlgo*   mMap[ Shape::NumShape * Shape::NumShape / 2 + 1];
		ContactManager   mPairManager;
		Broadphase       mBroadphase;
		TArray< ContactManifold* > mMainifolds;
	};

	class GJK
	{
	public:
		GJK();
		void init( CollideObject* objA , CollideObject* objB );

		struct Simplex
		{
			Vector2 v;
			Vector2 dir;
#if PHY2D_DEBUG
			Vector2 vObj[2];
#endif
		};
		struct Edge
		{
			Vector2  normal;
			float    depth;
			Edge*    next;
			Simplex* sv;
		};


		Simplex* calcSupport( Simplex* sv , Vector2 const& dir );

		bool     test();
		void     generateContact( Contact& c );

		Edge*    addEdge( Simplex* sv , Vector2 const& b );
		void     updateEdge( Edge* e ,Vector2 const& b );
		Edge*    insertEdge( Edge* cur , Simplex* sv );
		void     buildContact( Edge* e, Contact &c );
		Edge*    getClosetEdge();

		XForm2D     mBToALocal;


		static int const MaxIterNum = 20;
		int       mNumEdge;
		Edge      mEdges[ MaxIterNum + 3 ];
		int       mNumSimplex;
		Simplex   mStorage[ MaxIterNum + 3 ];
		Simplex*  mSv[3];
		CollideObject* mObj[2];
#if PHY2D_DEBUG
		TArray< Simplex > mDBG;
#endif

	};


#if PHY2D_DEBUG
	extern GJK gGJK;
#endif

}//namespace Phy2D

#endif // Collision_h__D60B683C_C710_4755_B3C8_9B0F35FCEDA9
