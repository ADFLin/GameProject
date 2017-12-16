#ifndef Collision_h__D60B683C_C710_4755_B3C8_9B0F35FCEDA9
#define Collision_h__D60B683C_C710_4755_B3C8_9B0F35FCEDA9

#include "Phy2D/Base.h"
#include "Phy2D/Shape.h"

#include "DataStructure/IntrList.h"

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
		void         setPos( Vector2 const& p ){ mXForm.setTranslation( p ); }


		Vector2 getSupport( Vector2 const& dir ) const
		{
			Vector2 pos = mShape->getSupport( mXForm.transformVectorInv( dir ) );
			return mXForm.transformPosition( pos );
		}

		Shape* getShape(){ return mShape; }

		XForm  mXForm;
		Shape* mShape;
		CollisionProxy* mProxy;
		HookNode hook;
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

	class ColAlgo
	{
	public:
		virtual bool test( CollideObject* objA , CollideObject* objB , Contact& c ) = 0;
	};

	struct CollisionProxy
	{
		CollideObject*  object;
		AABB aabb;

		HookNode hook;
		std::vector< ProxyPair* > pairs;
		void remove( ProxyPair* pair )
		{
			pairs.erase( std::find( pairs.begin() , pairs.end() , pair ) );
		}
	};



	struct ProxyPair
	{
		CollisionProxy* proxy[2];
		ContactManifold manifold;
		HookNode hook;
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
			ColAlgo* algo = mMap[ objA->mShape->getType() ][ objB->mShape->getType() ];
			return algo->test( objA , objB , c );
		}

		void preocss( float dt  );
		ColAlgo*         mMap[ Shape::NumShape ][ Shape::NumShape ];
		ContactManager   mPairManager;
		Broadphase       mBroadphase;
		std::vector< ContactManifold* > mMainifolds;
	};

	class GJK
	{
	public:
		GJK()
		{
			mSv[0] = mStorage + 0;
			mSv[1] = mStorage + 1;
			mSv[2] = mStorage + 2;

		}
		void init( CollideObject* objA , CollideObject* objB );

		struct Simplex
		{
			Vector2 v;
			Vector2 d;
#if PHY2D_DEBUG
			Vector2 vObj[2];
#endif
		};
		struct Edge
		{
			Vector2    normal;
			float    depth;
			Edge*    next;
			Simplex* sv;
		};


		Simplex* calcSupport( Simplex* sv , Vector2 const& dir );

		bool     test( Vector2 const& initDir );
		void     generateContact( Contact& c );

		Edge*    addEdge( Simplex* sv , Vector2 const& b );
		void     updateEdge( Edge* e ,Vector2 const& b );
		Edge*    insertEdge( Edge* cur , Simplex* sv );
		void     buildContact( Edge* e, Contact &c );
		Edge*    getClosetEdge();

		XForm     mBToALocal;


		static int const MaxIterNum = 20;
		int       mNumEdge;
		Edge      mEdges[ MaxIterNum + 3 ];
		int       mNumSimplex;
		Simplex   mStorage[ MaxIterNum + 3 ];
		Simplex*  mSv[3];
		CollideObject* mObj[2];
#if PHY2D_DEBUG
		std::vector< Simplex > mDBG;
#endif

	};


#if PHY2D_DEBUG
	extern GJK gGJK;
#endif

}//namespace Phy2D

#endif // Collision_h__D60B683C_C710_4755_B3C8_9B0F35FCEDA9
