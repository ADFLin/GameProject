#include "Collision.h"

#include "Phy2D/RigidBody.h"
#include "Collision2D/SATSolver.h"

namespace Phy2D
{
#if PHY2D_DEBUG
	GJK gGJK;
#endif

	class CollisionAlgoGJK : public CollisionAlgo
	{
	public:
		virtual bool test( CollideObject* objA , CollideObject* objB , Contact& c );
	};

	class CollisionAlgoCircle : public CollisionAlgo
	{	
	public:
		virtual bool test( CollideObject* objA , CollideObject* objB , Contact& c );
	};

	class CollisionAlgoBoxCircle : public CollisionAlgo
	{
	public:
		virtual bool test( CollideObject* objA , CollideObject* objB , Contact& c );
	};

	class BoxBoxAlgo : public CollisionAlgo
	{
	public:
		virtual bool test( CollideObject* objA , CollideObject* objB , Contact& c );
		SATSolver mSolver;
	};


	CollisionManager::CollisionManager()
	{
		static CollisionAlgoGJK        StaticGJKAlgo;
		static CollisionAlgoCircle     StaticCircleAlgo;
		static CollisionAlgoBoxCircle  StaticBoxCircleAlgo;

		std::fill_n( mMap , ARRAY_SIZE(mMap) , &StaticGJKAlgo );

		auto RegisterAlgo = [this](Shape::Type a, Shape::Type b, CollisionAlgo& algo)
		{
			int index = Math::PairingFunction<int>(a, b);
			mMap[index] = &algo;
		};

		RegisterAlgo(Shape::eCircle, Shape::eCircle, StaticCircleAlgo);
		RegisterAlgo(Shape::eBox, Shape::eCircle, StaticBoxCircleAlgo);
		
		mDefaultConvexAlgo = &StaticGJKAlgo;
	}

	void CollisionManager::preocss(float dt)
	{
		mBroadphase.process( mPairManager , dt );

		for (ProxyPair* pair : mPairManager.mProxyList)
		{
			Contact  contact;
			CollideObject* objA = pair->proxy[0]->object;
			CollideObject* objB = pair->proxy[1]->object;
			if ( test( objA , objB , contact ) )
			{
				if ( pair->proxy[0]->object->getType() != PhyObject::eCollideType &&
					 pair->proxy[1]->object->getType() != PhyObject::eCollideType )
				{
					if ( static_cast< RigidBody* >( objA )->mMotionType == BodyMotion::eStatic && 
						 static_cast< RigidBody* >( objB )->mMotionType == BodyMotion::eStatic )
						 continue;

					pair->manifold.addContact( contact );
				}
			}
		}

		mMainifolds.clear();
		for(ProxyPair* pair : mPairManager.mProxyList)
		{
			if ( pair->manifold.update() )
			{
				mMainifolds.push_back( &pair->manifold );
			}
		}

	}

	void Broadphase::process( ContactManager& pairManager , float dt )
	{
		for( ProxyList::iterator iter = mProxys.begin(), itEnd = mProxys.end() ; 
			 iter != itEnd ; ++iter )
		{
			CollisionProxy* proxy = *iter;
			CollideObject* object = proxy->object;
			object->mShape->calcAABB( object->mXForm , proxy->aabb );
			Vector2 dp = Vector2( mContactBreakThreshold + 0.5  , mContactBreakThreshold + 0.5 );
			proxy->aabb.min -= dp;
			proxy->aabb.max += dp;
		}

		for( ProxyList::iterator iter = mProxys.begin(), itEnd = mProxys.end() ; 
			iter != itEnd ; ++iter )
		{
			CollisionProxy* proxyA = *iter;
			ProxyList::iterator iter2 = iter; 
			++iter2;
			for(  ; iter2 != itEnd ; ++iter2 )
			{
				CollisionProxy* proxyB = *iter2;
				if ( proxyA->aabb.isInterect( proxyB->aabb ) )
				{
					pairManager.addProxyPair( proxyA , proxyB );
				}
				else
				{
					pairManager.removeProxyPair( proxyA , proxyB );
				}
			}
		}
	}



	ProxyPair* ContactManager::findProxyPair(CollisionProxy* proxyA , CollisionProxy* proxyB)
	{
		for( ProxyPairList::iterator iter = mProxyList.begin(), itEnd = mProxyList.end() ; 
			 iter != itEnd ; ++iter )
		{
			ProxyPair* pair = *iter;
			if ( pair->proxy[0] == proxyA && pair->proxy[1] == proxyB )
				return pair;
			if ( pair->proxy[0] == proxyB && pair->proxy[1] == proxyA )
				return pair;
		}

		//for( int i = 0; i < proxyA->pairs.size() ; ++i )
		//{
		//	ProxyPair* pair = proxyA->pairs[i];
		//	if ( pair->proxy[0] == proxyB || pair->proxy[1] == proxyB )
		//		return pair;
		//}
		return NULL;
	}

	bool ContactManager::addProxyPair(CollisionProxy* proxyA , CollisionProxy* proxyB)
	{
		ProxyPair* pair = findProxyPair( proxyA , proxyB );
		if ( pair )
			return false;

		pair = new ProxyPair;
		pair->proxy[0] = proxyA;
		pair->proxy[1] = proxyB;

		mProxyList.push_back( pair );
		proxyA->pairs.push_back( pair );
		proxyB->pairs.push_back( pair );
		return true;
	}

	bool ContactManager::removeProxyPair(CollisionProxy* proxyA , CollisionProxy* proxyB)
	{
		ProxyPair* pair = findProxyPair( proxyA , proxyB );
		if ( pair == NULL )
			return false;

		pair->hook.unlink();
		proxyA->remove( pair );
		proxyB->remove( pair );
		delete pair;
		return true;
	}

	GJK::GJK()
	{

	}

	void GJK::init(CollideObject* objA, CollideObject* objB)
	{
		assert( objA->getShape()->isConvex() && objB->getShape()->isConvex() );
		mObj[0] = objA;
		mObj[1] = objB;
		mBToALocal = XForm2D::MakeRelative(objB->mXForm, objA->mXForm);
		mSv[0] = mStorage + 0;
		mSv[1] = mStorage + 1;
		mSv[2] = mStorage + 2;
		mNumEdge = 0;
		mNumSimplex = 3;
#if PHY2D_DEBUG
		mDBG.clear();
#endif
	}

	GJK::Simplex* GJK::calcSupport(Simplex* sv , Vector2 const& dir)
	{
		assert( sv );
		Vector2 v0 = mObj[0]->mShape->getSupport( dir );
		Vector2 v1 = mObj[1]->mShape->getSupport( mBToALocal.transformVectorInv(-dir) );
#if PHY2D_DEBUG
		sv->vObj[0] = v0;
		sv->vObj[1] = v1;
#endif
		sv->v   = v0 - mBToALocal.transformPosition( v1 );
		sv->dir = dir;
		return  sv;
	}

	bool GJK::test()
	{
		Vector2 dir = Math::GetNormal(mBToALocal.transformPosition(mObj[1]->getPos()));
		Vector2 d;
		Simplex* sv = calcSupport( mSv[0] , dir );
#if PHY2D_DEBUG
		mDBG.push_back( *sv );
#endif
		dir = -sv->v;

		sv = calcSupport( mSv[1] , dir );
#if PHY2D_DEBUG
		mDBG.push_back( *sv );
#endif
		if ( dir.dot( sv->v ) < 0 )
		{
			return false;
		}
		d = sv->v + dir;
		dir = TripleProduct( sv->v , d , d );
		//#FIXME
		//if( dir.length2() )

		for(int iterCount = 0; iterCount < 20 ; ++iterCount )
		{
			sv = calcSupport( mSv[2] , dir );
#if PHY2D_DEBUG
			mDBG.push_back( *sv );
#endif
			if ( dir.dot( sv->v ) < 0 )
			{
				return false;
			}

			Vector2 d0 = mSv[0]->v - sv->v;
			Vector2 d1 = mSv[1]->v - sv->v;

			Vector2 d0Prep = TripleProduct( d1 , d0 , d0 );
			Vector2 d1Prep = TripleProduct( d0 , d1 , d1 );

			if ( d0Prep.dot( sv->v ) <= 0 )
			{
				std::swap( mSv[1] , mSv[2] );
				dir = d0Prep;
			}
			else if ( d1Prep.dot( sv->v ) <= 0 )
			{
				std::swap( mSv[0] , mSv[2] );
				dir = d1Prep;
			}
			else
			{
				return true;
			}
		}

		return true;
	}

	void GJK::generateContact(Contact& c)
	{
		if ( ( mSv[0]->v - mSv[1]->v ).cross( mSv[0]->v - mSv[2]->v ) < 0 )
			std::swap( mSv[0] , mSv[1]);

		addEdge( mSv[0] , mSv[1]->v )->next = mEdges + 1;
		addEdge( mSv[1] , mSv[2]->v )->next = mEdges + 2;
		addEdge( mSv[2] , mSv[0]->v )->next = mEdges + 0;

		Edge* bestEdge;
		for( int i = 0 ; i < MaxIterNum - 1; ++i )
		{
			bestEdge = getClosetEdge();
			assert( bestEdge );
			Simplex* sv = &mStorage[ mNumSimplex++ ];
			calcSupport( sv , bestEdge->normal );

			float depth = bestEdge->normal.dot( sv->v );
			if ( depth - bestEdge->depth < 1e-4 )
				break;

			Edge* newE = insertEdge( bestEdge , sv );	
		}

		buildContact(bestEdge, c);
	}

	void GJK::buildContact( Edge* e, Contact &c )
	{
		Edge* next = e->next;

		Vector2 d = e->depth * e->normal;
		float p[2];
		p[1] = e->sv->v.cross( e->normal );
		p[0] = e->normal.cross( next->sv->v );
		float sum = p[0] + p[1];
		if ( Math::Abs( sum ) < FLOAT_DIV_ZERO_EPSILON )
		{
			p[0] = p[1] = 0.5;
		}
		else
		{
			p[0] /= sum;
			p[1] /= sum;
		}
		XForm2D const& worldTrans = mObj[0]->mXForm;
		c.normal = worldTrans.transformVector( e->normal );
		c.depth  = e->depth;

		c.object[0] = mObj[0];
		c.object[1] = mObj[1];
		c.posLocal[0] = p[0] * ( mObj[0]->mShape->getSupport( e->sv->dir ) ) +
			            p[1] * ( mObj[0]->mShape->getSupport( next->sv->dir ) );
		c.posLocal[1] = mBToALocal.transformPositionInv( c.posLocal[0] - d );
		c.pos[0] = mObj[0]->mXForm.transformPosition( c.posLocal[0] );
		c.pos[1] = mObj[1]->mXForm.transformPosition( c.posLocal[1] );
	}

	GJK::Edge* GJK::addEdge( Simplex* sv , Vector2 const& b )
	{
		Edge* e = mEdges + mNumEdge;
		++mNumEdge;
		e->sv  = sv;
		updateEdge( e , b );
		return e;
	}

	void GJK::updateEdge( Edge* e ,Vector2 const& b )
	{
		assert( e );
		Simplex* sv = e->sv;
		Vector2 d = b - sv->v;
		e->normal = TripleProduct( d , sv->v , d );
		e->normal.normalize();
		e->depth = e->normal.dot( sv->v );
	}

	GJK::Edge* GJK::insertEdge(Edge* cur , Simplex* sv)
	{
		assert( cur );
		updateEdge( cur , sv->v );
		Edge* next = cur->next;
		Edge* result = addEdge( sv , next->sv->v );
		result->next = next;
		cur->next = result;
		return result;
	}

	GJK::Edge* GJK::getClosetEdge()
	{
		assert( mNumEdge != 0 );
		float depthMin = mEdges[0].depth;
		Edge* edge = &mEdges[0];
		for( int i = 1 ; i < mNumEdge ; ++i )
		{
			if ( depthMin > mEdges[i].depth )
			{
				depthMin = mEdges[i].depth;
				edge = &mEdges[i];
			}
		}
		return edge;
	}

	bool CollisionAlgoGJK::test(CollideObject* objA , CollideObject* objB , Contact& c )
	{
#if PHY2D_DEBUG
		GJK& gjk = gGJK;
#else
		GJK gjk;
#endif
		gjk.init( objA , objB );
		if (gjk.test() )
		{
			gjk.generateContact( c );
			return true;
		}
		return false;
	}

	bool CollisionAlgoCircle::test(CollideObject* objA , CollideObject* objB , Contact& c)
	{
		assert( objA->mShape->getType() == Shape::eCircle &&  
			    objB->mShape->getType() == Shape::eCircle );

		CircleShape* ca = static_cast< CircleShape* >( objA->mShape );
		CircleShape* cb = static_cast< CircleShape* >( objA->mShape );

		Vector2 offset = objB->getPos() - objA->getPos();
		float r = ca->getRadius() + cb->getRadius();
		float len2 = offset.length2();
		if ( len2 > r * r )
			return false;

		float len = Math::Sqrt( len2 );
		if ( len < FLOAT_DIV_ZERO_EPSILON )
		{
			c.depth = r;
			c.normal = Vector2(1,0);
		}
		else
		{
			c.depth  = len - r;
			c.normal = offset / len;
		}

		c.object[0] = objA;
		c.object[1] = objB;
		c.pos[0] = objA->getPos() + ca->getRadius() * c.normal;
		c.pos[1] = objB->getPos() - cb->getRadius() * c.normal;
		c.posLocal[0] = objA->mXForm.transformPositionInv( c.pos[0] );
		c.posLocal[1] = objB->mXForm.transformPositionInv( c.pos[1] );
		return true;
	}

	bool CollisionAlgoBoxCircle::test( CollideObject* objA , CollideObject* objB , Contact& c)
	{
		bool bSwapped = false;
		if (objA->mShape->getType() == Shape::eCircle)
		{
			std::swap(objA, objB);
			bSwapped = true;
		}
		assert( objA->mShape->getType() == Shape::eBox && objB->mShape->getType() == Shape::eCircle );

		Vector2 offset = objA->mXForm.transformPositionInv( objB->getPos() );
		Vector2 const& boxHalfSize = static_cast< BoxShape* >( objA->mShape )->mHalfExt;
		float radius = static_cast< CircleShape* >( objB->mShape )->getRadius();

		Vector2 offsetAbs = offset.abs();

		if (offsetAbs.x > boxHalfSize.x + radius)
			return false;
		if (offsetAbs.y > boxHalfSize.y + radius)
			return false;

		Vector2 normalLocal;
		if (offsetAbs.x > boxHalfSize.x && offsetAbs.y > boxHalfSize.y)
		{
			if ((offsetAbs - boxHalfSize).length2() > Math::Square(radius))
				return false;
			
			c.posLocal[0] = Vector2(Math::Sign(offset.x) * boxHalfSize.x , Math::Sign(offset.y) * boxHalfSize.y);
			normalLocal = offset - c.posLocal[0];
			float len = normalLocal.normalize();
			if (len == 0)
			{
				normalLocal = Math::GetNormal(offset);
				len = radius;
			}
			else if (normalLocal.dot(offset) < 0)
			{
				normalLocal = -normalLocal;
			}
			c.depth  = radius - len;
		}
		else
		{
			Vector2 depthV = boxHalfSize + Vector2(radius, radius) - offsetAbs;

			bool bHorizonal;	
			if (offsetAbs.x > boxHalfSize.x)
			{
				CHECK(depthV.x > 0);
				bHorizonal = true;
			}
			else if (offsetAbs.y > boxHalfSize.y)
			{
				CHECK(depthV.y > 0);
				bHorizonal = false;
			}
			else
			{
				CHECK(depthV.x > 0 && depthV.y > 0);
				bHorizonal = depthV.x <= depthV.y;
			}

			if (bHorizonal)
			{
				c.depth = depthV.x;
				normalLocal = Vector2(Math::Sign(offset.x), 0);
				c.posLocal[0] = Vector2(normalLocal.x * boxHalfSize.x, offset.y);
			}
			else
			{
				c.depth = depthV.y;
				normalLocal = Vector2(0, Math::Sign(offset.y));
				c.posLocal[0] = Vector2(offset.x, normalLocal.y * boxHalfSize.y);
			}
		}

		c.object[0] = objA;
		c.object[1] = objB;
		c.normal = objA->mXForm.transformVector(normalLocal);
		c.pos[0] = objA->mXForm.transformPosition(c.posLocal[0]);
		c.pos[1] = objA->mXForm.transformPosition(offset - radius * normalLocal);
		c.posLocal[1] = objB->mXForm.transformPositionInv(c.pos[1]);

		if (bSwapped)
		{
			std::swap(c.object[0], c.object[1]);
			std::swap(c.posLocal[0], c.posLocal[1]);
			std::swap(c.pos[0], c.pos[1]);
			c.normal = -c.normal;
		}
		return true;
	}

	bool BoxBoxAlgo::test( CollideObject* objA , CollideObject* objB , Contact& c)
	{





		return false;
		
	}

}//namespace Phy2D
