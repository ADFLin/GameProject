#include "Collision.h"

#include "Phy2D/RigidBody.h"
#include "Collision2D/SATSolver.h"
#include "ProfileSystem.h"

namespace Phy2D
{
#if PHY2D_DEBUG
	static GJK StaticGJK;
	static MPR StaticMPR;
	MinkowskiBase* GDebugGJK = &StaticGJK;
	MinkowskiBase& GetGJK(){ return *GDebugGJK; }
#endif

	class CollisionAlgoGJK : public CollisionAlgo
	{
	public:
		virtual bool test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c);
	};

	class CollisionAlgoCircle : public CollisionAlgo
	{	
	public:
		virtual bool test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c);
	};

	class CollisionAlgoBoxCircle : public CollisionAlgo
	{
	public:
		virtual bool test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c);
	};

	class BoxBoxAlgo : public CollisionAlgo
	{
	public:
		virtual bool test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c);
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
			int index = ToIndex(a, b);
			mMap[index] = &algo;
		};

		RegisterAlgo(Shape::eCircle, Shape::eCircle, StaticCircleAlgo);
		//RegisterAlgo(Shape::eBox, Shape::eCircle, StaticBoxCircleAlgo);
		
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
		for(CollisionProxy* proxy : mProxys)
		{
			CollideObject* object = proxy->object;
			object->mShape->calcAABB( object->mXForm , proxy->aabb );
			Vector2 dp = Vector2( mContactBreakThreshold + 0.5  , mContactBreakThreshold + 0.5 );
			proxy->aabb.min -= dp;
			proxy->aabb.max += dp;
		}

		for( ProxyList::iterator iter = mProxys.begin(), itEnd = mProxys.end(); 
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
		if (proxyA > proxyB)
		{
			std::swap(proxyA, proxyB);
		}
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
		if (proxyA > proxyB)
		{
			std::swap(proxyA, proxyB);
		}
		ProxyPair* pair = findProxyPair( proxyA , proxyB );
		if ( pair == NULL )
			return false;

		pair->hook.unlink();
		proxyA->remove( pair );
		proxyB->remove( pair );
		delete pair;
		return true;
	}

	void MinkowskiBase::init(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB)
	{
		assert(shapeA.isConvex() && shapeB.isConvex());
		mObjects[0] = &objA;
		mObjects[1] = &objB;
		mShapes[0] = &shapeA;
		mShapes[1] = &shapeB;
		mBToALocal = XForm2D::MakeRelative(objB.mXForm, objA.mXForm);
#if PHY2D_DEBUG
		mDBG.clear();
#endif
	}

	MinkowskiBase::Vertex* MinkowskiBase::calcSupport(Vertex* sv, Vector2 const& dir)
	{
		assert( sv );
		sv->v   = getSupport(dir);
		sv->dir = dir;
		return  sv;
	}

	void MinkowskiBase::buildContact( Edge* e, Contact &c )
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
		XForm2D const& worldTrans = mObjects[0]->mXForm;
		c.normal = worldTrans.transformVector( e->normal );
		c.depth  = e->depth;

		c.object[0] = mObjects[0];
		c.object[1] = mObjects[1];
		c.posLocal[0] = p[0] * ( mShapes[0]->getSupport( e->sv->dir ) ) +
			            p[1] * ( mShapes[0]->getSupport( next->sv->dir ) );
		c.posLocal[1] = mBToALocal.transformPositionInv( c.posLocal[0] - d );
		c.pos[0] = mObjects[0]->mXForm.transformPosition( c.posLocal[0] );
		c.pos[1] = mObjects[1]->mXForm.transformPosition( c.posLocal[1] );
	}

	MinkowskiBase::Edge* MinkowskiBase::addEdge( Vertex* sv , Vector2 const& b )
	{
		Edge* e = mEdges + mNumEdge;
		++mNumEdge;
		e->sv  = sv;
		updateEdge( e , b );
		return e;
	}

	void MinkowskiBase::updateEdge( Edge* e ,Vector2 const& b )
	{
		assert( e );
		Vertex* sv = e->sv;
		Vector2 d = b - sv->v;
		e->normal = TripleProduct( d , sv->v , d );
		e->normal.normalize();
		e->depth = e->normal.dot( sv->v );
	}

	MinkowskiBase::Edge* MinkowskiBase::insertEdge(Edge* cur , Vertex* sv)
	{
		assert( cur );
		updateEdge( cur , sv->v );
		Edge* next = cur->next;
		Edge* result = addEdge( sv , next->sv->v );
		result->next = next;
		cur->next = result;
		return result;
	}

	MinkowskiBase::Edge* MinkowskiBase::getClosetEdge()
	{
		assert( mNumEdge != 0 );
		float depthMin = mEdges[0].depth;
		Edge* edge = &mEdges[0];
		for( int i = 1 ; i < mNumEdge ; ++i )
		{
			if (mEdges[i].depth < depthMin )
			{
				depthMin = mEdges[i].depth;
				edge = &mEdges[i];
			}
		}
		return edge;
	}

	void MinkowskiBase::generateContact(Contact& c)
	{
		if ((mSv[0]->v - mSv[1]->v).cross(mSv[0]->v - mSv[2]->v) < 0)
			std::swap(mSv[0], mSv[1]);

		addEdge(mSv[0], mSv[1]->v)->next = mEdges + 1;
		addEdge(mSv[1], mSv[2]->v)->next = mEdges + 2;
		addEdge(mSv[2], mSv[0]->v)->next = mEdges + 0;

		Edge* bestEdge;
		for (int i = 0; i < MaxIterNum - 1; ++i)
		{
			bestEdge = getClosetEdge();
			assert(bestEdge);
			Vertex* sv = &mStorage[mNumSimplex++];
			calcSupport(sv, bestEdge->normal);

			float depth = bestEdge->normal.dot(sv->v);
			if (depth - bestEdge->depth < 1e-4)
				break;

			Edge* newEdge = insertEdge(bestEdge, sv);
		}

		buildContact(bestEdge, c);
	}

	GJK::GJK()
	{

	}

	void GJK::init(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB)
	{
		MinkowskiBase::init(objA, shapeA, objB, shapeB);
		mSv[0] = mStorage + 0;
		mSv[1] = mStorage + 1;
		mSv[2] = mStorage + 2;
		mNumEdge = 0;
		mNumSimplex = 3;
	}

	bool GJK::test()
	{
		Vector2 dir = Math::GetNormal(mBToALocal.transformPosition(mObjects[1]->getPos()));
		Vector2 d;
		Vertex* sv = calcSupport( mSv[0] , dir );
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

	void MPR::init(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB)
	{
		MinkowskiBase::init(objA, shapeA, objB, shapeB);
		mSv[0] = mStorage + 0;
		mSv[1] = mStorage + 1;
		mSv[2] = mStorage + 2;
		mNumEdge = 0;
		mNumSimplex = 3;
	}

	Vector2 MPR::getPointInside() const
	{
		return 0.25 * (getSupport(Vector2(1, 0)) + getSupport(Vector2(-1, 0)) + getSupport(Vector2(0, 1)) + getSupport(Vector2(0, -1)));
	}

	bool MPR::test()
	{
		Vector2 pointInside = getPointInside();

		Vertex* sv;
		sv = calcSupport(mSv[0], -pointInside);
#if PHY2D_DEBUG
		mDBG.push_back(*sv);
#endif
		Vector2 dSide = sv->v - pointInside;
		Vector2 dSidePrep = TripleProduct(pointInside, dSide, dSide);
		dSidePrep.normalize();
		sv = calcSupport(mSv[1], dSidePrep);
#if PHY2D_DEBUG
		mDBG.push_back(*sv);
#endif
		for (int iterCount = 0; iterCount < 20; ++iterCount)
		{
			Vector2 d = mSv[1]->v - mSv[0]->v;
			Vector2 dPerp = TripleProduct(mSv[0]->v, d, d);
			dPerp.normalize();
			sv = calcSupport(mSv[2], dPerp);

			if (dPerp.dot(pointInside) >= 0)
				return true;

#if PHY2D_DEBUG
			mDBG.push_back(*sv);
#endif
			if (dPerp.dot(sv->v) < 0)
				return false;

			dSide = sv->v - pointInside;
			dSidePrep = TripleProduct(pointInside, dSide, dSide);
			if (dSidePrep.dot(mSv[0]->v - mSv[2]->v) < 0)
			{
				std::swap(mSv[0], mSv[2]);
			}
			else
			{
				std::swap(mSv[1], mSv[2]);
			}
		}

		return false;
	}

	bool CollisionAlgoGJK::test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c)
	{
		static bool bUseGJK = false;
		if (bUseGJK)
		{
			TIME_SCOPE("GJK");
			bUseGJK = !bUseGJK;
			GDebugGJK = &StaticGJK;
			StaticGJK.init(objA, shapeA, objB, shapeB);

			{
				TIME_SCOPE("Test");
				if (!StaticGJK.test())
					return false;
			}

			{
				TIME_SCOPE("GenContact");
				StaticGJK.generateContact(c);
			}
			return true;
		}
		else
		{
			TIME_SCOPE("MPR");
			bUseGJK = !bUseGJK;
			GDebugGJK = &StaticMPR;
			StaticMPR.init(objA, shapeA, objB, shapeB);

			{
				TIME_SCOPE("Test");
				if (!StaticMPR.test())
					return false;
			}
			
			{
				TIME_SCOPE("GenContact");
				StaticMPR.generateContact(c);
			}
			return true;
		}
	}

	bool CollisionAlgoCircle::test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c)
	{
		assert( shapeA.getType() == Shape::eCircle &&
			    shapeB.getType() == Shape::eCircle );

		CircleShape& ca = static_cast< CircleShape& >(shapeA);
		CircleShape& cb = static_cast< CircleShape& >(shapeB);

		Vector2 offset = objB.getPos() - objA.getPos();
		float r = ca.getRadius() + cb.getRadius();
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

		c.object[0] = &objA;
		c.object[1] = &objB;
		c.pos[0] = objA.getPos() + ca.getRadius() * c.normal;
		c.pos[1] = objB.getPos() - cb.getRadius() * c.normal;
		c.posLocal[0] = objA.mXForm.transformPositionInv( c.pos[0] );
		c.posLocal[1] = objB.mXForm.transformPositionInv( c.pos[1] );
		return true;
	}

	bool CollisionAlgoBoxCircle::test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c)
	{
		if (shapeA.getType() == Shape::eCircle)
		{
			if (test(objB, shapeB, objA, shapeA, c))
			{
				std::swap(c.object[0], c.object[1]);
				std::swap(c.posLocal[0], c.posLocal[1]);
				std::swap(c.pos[0], c.pos[1]);
				c.normal = -c.normal;
				return true;
			}
			return false;
		}

		assert(shapeA.getType() == Shape::eBox && shapeB.getType() == Shape::eCircle );

		Vector2 offset = objA.mXForm.transformPositionInv( objB.getPos() );
		Vector2 const& boxHalfSize = static_cast< BoxShape& >( shapeA ).mHalfExt;
		float radius = static_cast< CircleShape& >(shapeB).getRadius();

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

		c.object[0] = &objA;
		c.object[1] = &objB;
		c.normal = objA.mXForm.transformVector(normalLocal);
		c.pos[0] = objA.mXForm.transformPosition(c.posLocal[0]);
		c.pos[1] = objA.mXForm.transformPosition(offset - radius * normalLocal);
		c.posLocal[1] = objB.mXForm.transformPositionInv(c.pos[1]);
		return true;
	}

	bool BoxBoxAlgo::test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c)
	{



		return false;

	}


}//namespace Phy2D
