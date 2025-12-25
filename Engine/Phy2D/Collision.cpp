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
		virtual bool test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c, ManifoldPoint& point) override;
	};

	class CollisionAlgoCircle : public CollisionAlgo
	{	
	public:
		virtual bool test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c, ManifoldPoint& point) override;
	};

	class CollisionAlgoBoxCircle : public CollisionAlgo
	{
	public:
		virtual bool test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c, ManifoldPoint& point) override;
	};

	class BoxBoxAlgo : public CollisionAlgo
	{
	public:
		virtual bool test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c, ManifoldPoint& point) override;
		// Override testManifold to provide two contact points for box-box edge-face contacts
		virtual bool testManifold(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, ContactManifold& manifold) override;
		SATSolver mSolver;
	};


	CollisionManager::CollisionManager()
	{
		static CollisionAlgoGJK        StaticGJKAlgo;
		static CollisionAlgoCircle     StaticCircleAlgo;
		static CollisionAlgoBoxCircle  StaticBoxCircleAlgo;
		static BoxBoxAlgo              StaticBoxBoxAlgo;  // Add this

		std::fill_n( mMap , ARRAY_SIZE(mMap) , &StaticGJKAlgo );

		auto RegisterAlgo = [this](Shape::Type a, Shape::Type b, CollisionAlgo& algo)
		{
			int index = ToIndex(a, b);
			mMap[index] = &algo;
		};

		RegisterAlgo(Shape::eCircle, Shape::eCircle, StaticCircleAlgo);
		RegisterAlgo(Shape::eBox, Shape::eCircle, StaticBoxCircleAlgo);  // Enable specialized box-circle collision
		RegisterAlgo(Shape::eBox, Shape::eBox, StaticBoxBoxAlgo);        // Enable specialized box-box collision
		
		mDefaultConvexAlgo = &StaticGJKAlgo;
	}

	void CollisionManager::preocss(float dt)
	{
		mBroadphase.process( mPairManager , dt );

		for (ProxyPair* pair : mPairManager.mProxyList)
		{
			CollideObject* objA = pair->proxy[0]->object;
			CollideObject* objB = pair->proxy[1]->object;
			
			// Skip pure collision objects
			if ( pair->proxy[0]->object->getType() == PhyObject::eCollideType ||
				 pair->proxy[1]->object->getType() == PhyObject::eCollideType )
			{
				continue;
			}
			
			// Skip if both are static
			if ( static_cast< RigidBody* >( objA )->mMotionType == BodyMotion::eStatic && 
				 static_cast< RigidBody* >( objB )->mMotionType == BodyMotion::eStatic )
			{
				continue;
			}
			
			// CRITICAL FIX: Check if collision exists before using manifold
			if ( testManifold(objA, objB, pair->manifold) )
			{
				// Collision detected, manifold is populated with contact data
				// Mark manifold as active (will be picked up by update())
				if (pair->manifold.mAge != 0)
				{
					// This is a continuing contact, preserve it
				}
			}
			else
			{
				// No collision - reset the manifold to ensure clean state
				pair->manifold.mNumContacts = 0;
			}
		}

		mMainifolds.clear();
		for(ProxyPair* pair : mPairManager.mProxyList)
		{
			if ( pair->manifold.update() )
			{
				mMainifolds.push_back( &pair->manifold );
			}
			else
			{
				pair->manifold.reset();
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
				if ( proxyA->aabb.isIntersect( proxyB->aabb ) )
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

	void MinkowskiBase::buildContact( Edge* e, Contact& c, ManifoldPoint& point )
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
		point.depth = e->depth;

		c.object[0] = mObjects[0];
		c.object[1] = mObjects[1];
		point.posLocal[0] = p[0] * ( mShapes[0]->getSupport( e->sv->dir ) ) +
			            p[1] * ( mShapes[0]->getSupport( next->sv->dir ) );
		point.posLocal[1] = mBToALocal.transformPositionInv( point.posLocal[0] - d );
		point.pos[0] = mObjects[0]->mXForm.transformPosition( point.posLocal[0] );
		point.pos[1] = mObjects[1]->mXForm.transformPosition( point.posLocal[1] );
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

	void MinkowskiBase::generateContact(Contact& c, ManifoldPoint& point)
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

		buildContact(bestEdge, c, point);
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

	bool CollisionAlgoGJK::test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c, ManifoldPoint& point)
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
				StaticGJK.generateContact(c, point);
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
				StaticMPR.generateContact(c, point);
			}
			return true;
		}
	}

	bool CollisionAlgoCircle::test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c, ManifoldPoint& point)
	{
		assert( shapeA.getType() == Shape::eCircle &&
			    shapeB.getType() == Shape::eCircle );

		CircleShape& ca = static_cast< CircleShape& >(shapeA);
		CircleShape& cb = static_cast< CircleShape& >(shapeB);

		Vector2 offset = objB.getPos() - objA.getPos();
		float r = ca.getRadius() + cb.getRadius();
		float len2 = offset.length2();
		float const kContactBreakThreshold = 0.02f;
		if ( len2 > (r + kContactBreakThreshold) * (r + kContactBreakThreshold) )
			return false;

		float len = Math::Sqrt( len2 );
		if ( len < FLOAT_DIV_ZERO_EPSILON )
		{
			point.depth = r;
			c.normal = Vector2(1,0);
		}
		else
		{
			point.depth  = r - len;
			c.normal = offset / len;
		}

		c.object[0] = &objA;
		c.object[1] = &objB;
		point.pos[0] = objA.getPos() + ca.getRadius() * c.normal;
		point.pos[1] = objB.getPos() - cb.getRadius() * c.normal;
		point.posLocal[0] = objA.mXForm.transformPositionInv( point.pos[0] );
		point.posLocal[1] = objB.mXForm.transformPositionInv( point.pos[1] );
		return true;
	}

	bool CollisionAlgoBoxCircle::test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c, ManifoldPoint& point)
	{
		if (shapeA.getType() == Shape::eCircle)
		{
			if (test(objB, shapeB, objA, shapeA, c, point))
			{
				std::swap(c.object[0], c.object[1]);
				std::swap(point.posLocal[0], point.posLocal[1]);
				std::swap(point.pos[0], point.pos[1]);
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
			
			point.posLocal[0] = Vector2(Math::Sign(offset.x) * boxHalfSize.x , Math::Sign(offset.y) * boxHalfSize.y);
			normalLocal = offset - point.posLocal[0];
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
			point.depth  = radius - len;
		}
		else
		{
			Vector2 depthV = boxHalfSize + Vector2(radius, radius) - offsetAbs;

			bool bHorizonal;	
			if (offsetAbs.x > boxHalfSize.x)
			{
				CHECK(depthV.x >= 0);
				bHorizonal = true;
			}
			else if (offsetAbs.y > boxHalfSize.y)
			{
				CHECK(depthV.y >= 0);
				bHorizonal = false;
			}
			else
			{
				CHECK(depthV.x >= 0 && depthV.y >= 0);
				bHorizonal = depthV.x <= depthV.y;
			}

			if (bHorizonal)
			{
				point.depth = depthV.x;
				normalLocal = Vector2(Math::Sign(offset.x), 0);
				point.posLocal[0] = Vector2(normalLocal.x * boxHalfSize.x, offset.y);
			}
			else
			{
				point.depth = depthV.y;
				normalLocal = Vector2(0, Math::Sign(offset.y));
				point.posLocal[0] = Vector2(offset.x, normalLocal.y * boxHalfSize.y);
			}
		}

		c.object[0] = &objA;
		c.object[1] = &objB;
		c.normal = objA.mXForm.transformVector(normalLocal);
		point.pos[0] = objA.mXForm.transformPosition(point.posLocal[0]);
		point.pos[1] = objA.mXForm.transformPosition(offset - radius * normalLocal);
		point.posLocal[1] = objB.mXForm.transformPositionInv(point.pos[1]);
		return true;
	}

	// Helper struct for BoxBox collision internal data
	struct BoxBoxCollisionData
	{
		Vector2 axes[4];
		Vector2 normal;
		float minOverlap;
		int bestAxis;
		bool refIsA;
		Vector2 corners[4];
		int cornerIndices[4];
		int numIncidentCorners;
	};

	// Common SAT test for box-box, returns collision data
	static bool BoxBoxSATTest(CollideObject& objA, BoxShape& boxA, CollideObject& objB, BoxShape& boxB, BoxBoxCollisionData& data)
	{
		XForm2D const& xfA = objA.mXForm;
		XForm2D const& xfB = objB.mXForm;

		Vector2 hA = boxA.mHalfExt;
		Vector2 hB = boxB.mHalfExt;

		Vector2 pA = objA.getPos();
		Vector2 pB = objB.getPos();
		Vector2 d = pB - pA;

		// Axes: A's x,y and B's x,y
		data.axes[0] = xfA.transformVector(Vector2(1, 0));
		data.axes[1] = xfA.transformVector(Vector2(0, 1));
		data.axes[2] = xfB.transformVector(Vector2(1, 0));
		data.axes[3] = xfB.transformVector(Vector2(0, 1));

		data.minOverlap = std::numeric_limits<float>::max();
		data.bestAxis = -1;

		for (int i = 0; i < 4; ++i)
		{
			Vector2 const& L = data.axes[i];
			float projA = Math::Abs(data.axes[0].dot(L)) * hA.x + Math::Abs(data.axes[1].dot(L)) * hA.y;
			float projB = Math::Abs(data.axes[2].dot(L)) * hB.x + Math::Abs(data.axes[3].dot(L)) * hB.y;

			float dist = Math::Abs(d.dot(L)) - (projA + projB);

			if (dist > 0)
				return false;

			// Stability Bias: Prefer separation axes aligned with gravity (vertical Y axis)
			// Aggressively bias towards vertical (0.8 factor means we prefer vertical axis
			// even if the penetration on tilted axis is 20% smaller).
			// This effectively locks the contact normal to vertical for stable stacking.
			float verticality = Math::Abs(L.y);
			if (verticality > 0.98f)
			{
				dist *= 0.80f; // Strongly prefer this axis
			}

			if (dist > -data.minOverlap)
			{
				data.minOverlap = -dist;
				data.bestAxis = i;
			}
		}

		// Collision detected
		data.normal = data.axes[data.bestAxis];
		if (d.dot(data.normal) < 0)
			data.normal = -data.normal;

		// Identify Reference and Incident
		data.refIsA = (data.bestAxis < 2);
		BoxShape* incBox = data.refIsA ? &boxB : &boxA;
		XForm2D const& incXf = data.refIsA ? xfB : xfA;

		// Find incident vertices
		Vector2 searchDir = data.refIsA ? -data.normal : data.normal;

		Vector2 allCorners[4] = {
			incXf.transformPosition(Vector2(incBox->mHalfExt.x, incBox->mHalfExt.y)),
			incXf.transformPosition(Vector2(-incBox->mHalfExt.x, incBox->mHalfExt.y)),
			incXf.transformPosition(Vector2(-incBox->mHalfExt.x, -incBox->mHalfExt.y)),
			incXf.transformPosition(Vector2(incBox->mHalfExt.x, -incBox->mHalfExt.y))
		};

		// Find max projection
		float maxProj = -std::numeric_limits<float>::max();
		for (int i = 0; i < 4; ++i)
		{
			float proj = allCorners[i].dot(searchDir);
			if (proj > maxProj)
				maxProj = proj;
		}


		// Collect vertices close to max projection (these form the incident edge/face)
		// For a box of size 4x4, edge length is 4, so we need a reasonable tolerance
		// Use 1% of typical box dimension as slop (0.04)
		float const kFeatureSlop = 0.04f;  // Restored to allow stable 2-point face contact
		data.numIncidentCorners = 0;
		for (int i = 0; i < 4; ++i)
		{
			float proj = allCorners[i].dot(searchDir);
			if (proj >= maxProj - kFeatureSlop)
			{
				data.corners[data.numIncidentCorners] = allCorners[i];
				data.cornerIndices[data.numIncidentCorners] = i;
				data.numIncidentCorners++;
			}
		}

		return true;
	}

	bool BoxBoxAlgo::test(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, Contact& c, ManifoldPoint& point)
	{
		assert(shapeA.getType() == Shape::eBox && shapeB.getType() == Shape::eBox);

		BoxShape& boxA = static_cast<BoxShape&>(shapeA);
		BoxShape& boxB = static_cast<BoxShape&>(shapeB);

		BoxBoxCollisionData data;
		if (!BoxBoxSATTest(objA, boxA, objB, boxB, data))
			return false;

		c.normal = data.normal;
		c.object[0] = &objA;
		c.object[1] = &objB;
		point.depth = data.minOverlap;

		// Average the incident corners for single-point output
		Vector2 accumPos = Vector2::Zero();
		for (int i = 0; i < data.numIncidentCorners; ++i)
		{
			accumPos += data.corners[i];
		}
		Vector2 cp = accumPos * (1.0f / data.numIncidentCorners);

		if (data.refIsA)
		{
			point.pos[1] = cp;
			point.pos[0] = cp - data.normal * data.minOverlap;
		}
		else
		{
			point.pos[0] = cp;
			point.pos[1] = cp + data.normal * data.minOverlap;
		}

		point.posLocal[0] = objA.mXForm.transformPositionInv(point.pos[0]);
		point.posLocal[1] = objB.mXForm.transformPositionInv(point.pos[1]);

		return true;
	}

	bool BoxBoxAlgo::testManifold(CollideObject& objA, Shape& shapeA, CollideObject& objB, Shape& shapeB, ContactManifold& manifold)
	{
		assert(shapeA.getType() == Shape::eBox && shapeB.getType() == Shape::eBox);

		BoxShape& boxA = static_cast<BoxShape&>(shapeA);
		BoxShape& boxB = static_cast<BoxShape&>(shapeB);

		BoxBoxCollisionData data;
		if (!BoxBoxSATTest(objA, boxA, objB, boxB, data))
			return false;


		manifold.mContact.normal = data.normal;
		manifold.mContact.object[0] = &objA;
		manifold.mContact.object[1] = &objB;
		manifold.mAge = 0;
		// Store local normal for Position Solver
		manifold.refIsA = data.refIsA;
		if (data.refIsA)
			manifold.localNormal = objA.mXForm.transformVectorInv(data.normal);
		else
			manifold.localNormal = objB.mXForm.transformVectorInv(data.normal);

		// Create contact points for each incident corner (up to MaxManifoldPoints)
		int numPoints = Math::Min(data.numIncidentCorners, MaxManifoldPoints);
		manifold.mNumContacts = numPoints;

		for (int i = 0; i < numPoints; ++i)
		{
			ManifoldPoint& point = manifold.mPoints[i];
			Vector2 cp = data.corners[i];

			point.depth = data.minOverlap;

			if (data.refIsA)
			{
				point.pos[1] = cp;
				// Fix: Add normal * overlap to get point on reference surface (A)
				// Since normal points A->B, and cp (on B) is inside A due to penetration
				point.pos[0] = cp + data.normal * data.minOverlap;
			}
			else
			{
				point.pos[0] = cp;
				// Fix: Subtract normal * overlap to get point on reference surface (B)
				// Since normal points A->B, and cp (on A) is inside B
				point.pos[1] = cp - data.normal * data.minOverlap;
			}

			point.posLocal[0] = objA.mXForm.transformPositionInv(point.pos[0]);
			point.posLocal[1] = objB.mXForm.transformPositionInv(point.pos[1]);
			
			// CRITICAL: Reset impulses for new contact points
			// Without warm starting contact ID matching, we reset each frame
			point.normalImpulse = 0.0f;
			point.tangentImpulse = 0.0f;
		}

		return true;
	}


}//namespace Phy2D
