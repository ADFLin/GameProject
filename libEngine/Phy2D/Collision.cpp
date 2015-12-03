#include "Collision.h"

#include "Phy2D/RigidBody.h"

namespace Phy2D
{
	class GJKColAlgo : public ColAlgo
	{
	public:
		virtual bool test( CollideObject* objA , CollideObject* objB , Contact& c );
	};

	class CircleCircleAlgo : public ColAlgo
	{	
	public:
		virtual bool test( CollideObject* objA , CollideObject* objB , Contact& c );
	};

	class BoxCircleAlgo : public ColAlgo
	{
	public:
		virtual bool test( CollideObject* objA , CollideObject* objB , Contact& c );
	};

	class BoxBoxAlgo : public ColAlgo
	{
	public:
		virtual bool test( CollideObject* objA , CollideObject* objB , Contact& c );

	};


	CollisionManager::CollisionManager()
	{
		static GJKColAlgo       sGjkAlgo;
		static CircleCircleAlgo sCircleAlgo;
		static BoxCircleAlgo    sBoxCircleAlgo;

		std::fill_n( &mMap[0][0] , Shape::NumShape * Shape::NumShape , &sGjkAlgo );
		mMap[ Shape::eCircle ][ Shape::eCircle ] = &sCircleAlgo;
		mMap[ Shape::eBox ][ Shape::eCircle ] = &sBoxCircleAlgo;
		mMap[ Shape::eCircle ][ Shape::eBox ] = &sBoxCircleAlgo;

	}

	void CollisionManager::preocss(float dt)
	{
		mBroadphase.process( mPairManager , dt );

		for( ProxyPairList::iterator iter = mPairManager.mProxyList.begin() , itEnd = mPairManager.mProxyList.end();
			iter != itEnd ; ++iter )
		{
			ProxyPair* pair = *iter;
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
		for( ProxyPairList::iterator iter = mPairManager.mProxyList.begin() , itEnd = mPairManager.mProxyList.end();
			iter != itEnd ; ++iter )
		{
			ProxyPair* pair = *iter;
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
			Vec2f dp = Vec2f( mContactBreakThreshold + 0.5  , mContactBreakThreshold + 0.5 );
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

	GJK gGJK;
	void GJK::init(CollideObject* objA , CollideObject* objB)
	{
		mObj[0] = objA;
		mObj[1] = objB;
		mB2AWorld = objA->mXForm.mulInv( objB->mXForm );
		mSv[0] = mStorage + 0;
		mSv[1] = mStorage + 1;
		mSv[2] = mStorage + 2;
		mNumEdge = 0;
		mNumSimplex = 3;
#if PHY2D_DEBUG
		mDBG.clear();
#endif
	}

	bool GJK::test(Vec2f const& initDir)
	{
		Vec2f dir = initDir;
		Vec2f d;
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
		dir = tripleProduct( sv->v , d , d );

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

			Vec2f d0 = mSv[0]->v - sv->v;
			Vec2f d1 = mSv[1]->v - sv->v;

			Vec2f d0Prep = tripleProduct( d1 , d0 , d0 );
			Vec2f d1Prep = tripleProduct( d0 , d1 , d1 );

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

	void GJK::buildContact( Edge* e, Contact &c )
	{
		Edge* next = e->next;

		Vec2f d = e->depth * e->normal;
		float p[2];
		p[1] = e->sv->v.cross( e->normal );
		p[0] = e->normal.cross( next->sv->v );
		float sum = p[0] + p[1];
		if ( Math::Abs( sum ) < FLT_DIV_ZERO_EPSILON )
		{
			p[0] = p[1] = 0.5;
		}
		else
		{
			p[0] /= sum;
			p[1] /= sum;
		}
		XForm const& worldTrans = mObj[0]->mXForm;
		c.normal = worldTrans.rotateVector( e->normal );
		c.depth  = e->depth;

		c.object[0] = mObj[0];
		c.object[1] = mObj[1];
		c.posLocal[0] = p[0] * ( mObj[0]->mShape->getSupport( e->sv->d ) ) +
			            p[1] * ( mObj[0]->mShape->getSupport( next->sv->d ) );
		c.posLocal[1] = mB2AWorld.mulInv( c.posLocal[0] - d );
		c.pos[0] = mObj[0]->mXForm.mul( c.posLocal[0] );
		c.pos[1] = mObj[1]->mXForm.mul( c.posLocal[1] );
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

	GJK::Edge* GJK::addEdge( Simplex* sv , Vec2f const& b )
	{
		Edge* e = mEdges + mNumEdge;
		++mNumEdge;
		e->sv  = sv;
		updateEdge( e , b );
		return e;
	}

	void GJK::updateEdge( Edge* e ,Vec2f const& b )
	{
		assert( e );
		Simplex* sv = e->sv;
		Vec2f d = b - sv->v;
		e->normal = tripleProduct( d , sv->v , d );
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

	GJK::Simplex* GJK::calcSupport(Simplex* sv , Vec2f const& dir)
	{
		assert( sv );
		Vec2f v0 = mObj[0]->mShape->getSupport( dir );
		Vec2f v1 = mObj[1]->mShape->getSupport( mB2AWorld.rotateVectorInv(-dir) );
#if PHY2D_DEBUG
		sv->vObj[0] = v0;
		sv->vObj[1] = v1;
#endif
		sv->v = v0 - mB2AWorld.mul( v1 );
		sv->d = dir;
		return  sv;
	}

	bool GJKColAlgo::test(CollideObject* objA , CollideObject* objB , Contact& c )
	{
		gGJK.init( objA , objB );
		Vec2f dir = objB->getPos() - objA->getPos();
		if ( gGJK.test( objA->mXForm.rotateVectorInv( dir ) ) )
		{
			gGJK.generateContact( c );
			return true;
		}
		return false;
	}

	bool CircleCircleAlgo::test(CollideObject* objA , CollideObject* objB , Contact& c)
	{
		assert( objA->mShape->getType() == Shape::eCircle &&  
			    objB->mShape->getType() == Shape::eCircle );

		CircleShape* ca = static_cast< CircleShape* >( objA->mShape );
		CircleShape* cb = static_cast< CircleShape* >( objA->mShape );

		Vec2f offset = objB->getPos() - objA->getPos();
		float r = ca->getRadius() + cb->getRadius();
		float len2 = offset.length2();
		if ( len2 > r * r )
			return false;

		float len = Math::Sqrt( len2 );
		if ( len < FLT_DIV_ZERO_EPSILON )
		{
			c.depth = r;
			c.normal = Vec2f(1,0);
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
		c.posLocal[0] = objA->mXForm.mulInv( c.pos[0] );
		c.posLocal[1] = objB->mXForm.mulInv( c.pos[1] );
		return true;
	}

	bool BoxCircleAlgo::test( CollideObject* objA , CollideObject* objB , Contact& c)
	{
		if ( objA->mShape->getType() == Shape::eCircle )
			std::swap( objA , objB );

		assert( objA->mShape->getType() == Shape::eBox && objB->mShape->getType() == Shape::eCircle );

		Vec2f offset = objA->mXForm.rotateVector( objB->getPos() - objA->getPos() );
		Vec2f const& half  = static_cast< BoxShape* >( objA->mShape )->mHalfExt;
		float radius = static_cast< CircleShape* >( objB->mShape )->getRadius();

		int idx = 0;
		if ( offset.x > half.x )
			idx += 1;
		else if ( offset.x < -half.x )
			idx -= 1;

		if ( offset.y > half.y )
			idx += 3;
		else if ( offset.y < -half.y )
			idx -= 3;

		bool isEdge = true;
		Vec2f vCol;
		if ( idx == 0 )
		{
			float ox = Math::Abs( offset.x );
			float oy = Math::Abs( offset.y );
#if 0
			if ( Math::Abs( ox - oy ) < FLT_ZERO_EPSILON )
			{
				c.normal = normalize( Vec2f( offset.x > 0 ? 1 : -1 , offset.y > 0 ? 1 : -1 ) );
				c.depth  = Sqrt( )

				isEdge = false;
			}
			else 
#endif
			if ( half.x - ox < half.y - oy )
			{
				if ( offset.x > 0 )
				{
					c.depth = radius + half.x - offset.x; 
					c.normal = Vec2f::PositiveX(); 
				}
				else
				{
					c.depth = radius + half.x + offset.x; 
					c.normal = Vec2f::NegativeX();
				}
			}
			else // half.x - ox > half.y - oy 
			{
				if ( offset.y > 0 )
				{
					c.depth = radius + half.y - offset.y; 
					c.normal = Vec2f::PositiveY(); 
				}
				else
				{
					c.depth = radius + half.y + offset.y; 
					c.normal = Vec2f::NegativeY();
				}
			}
		}
		else if ( ( idx % 2 )== 0 )
		{
			switch ( idx )
			{
			case  4: vCol = half; break;
			case  2: vCol = Vec2f( -half.x , half.y ); break;
			case -2: vCol = Vec2f( half.x , -half.y ); break;
			case -4: vCol = -half; break;
			}

			Vec2f diff = offset - vCol;
			float len = diff.length2();
			if ( len > radius * radius )
				return false;
			len = Math::Sqrt( len );
			c.depth = radius - len;
			if ( len < FLT_DIV_ZERO_EPSILON )
			{
				c.normal = normalize( Vec2f( vCol.x > 0 ? 1 : -1 , vCol.y > 0 ? 1 : -1 ) );
			}
			else
			{
				c.normal = diff / len;
			}

			isEdge = false;
		}
		else
		{
			switch ( idx )
			{
			case  1: 
				if ( offset.x > half.x + radius ) 
					return false; 
				c.depth = ( half.x + radius ) - offset.x; 
				c.normal = Vec2f::PositiveX(); 
				break;
			case -1: 
				if ( -offset.x > ( half.x + radius ) ) 
					return false;
				c.depth = ( half.x + radius ) + offset.x; 
				c.normal = Vec2f::NegativeX();
				break;
			case  3: 
				if ( offset.y > half.y + radius ) 
					return false; 
				c.depth = ( half.y + radius ) - offset.y; 
				c.normal = Vec2f::PositiveY();
				break;
			case -3: 
				if ( -offset.y > ( half.y + radius ) ) 
					return false;
				c.depth = ( half.y + radius ) + offset.y; 
				c.normal = Vec2f::NegativeY(); 
				break;
			}	
		}

		c.object[0] = objA;
		c.object[1] = objB;
		if ( isEdge )
		{
			Vec2f pB = offset - radius * c.normal; 
			c.posLocal[0] = pB + c.depth * c.normal;
			c.pos[1] = objA->mXForm.mul( pB );
		}
		else
		{
			c.posLocal[0] = vCol;
			c.pos[1] = objA->mXForm.mul( vCol - c.depth * c.normal );
		}
		c.pos[0] = objA->mXForm.mul( c.posLocal[0] );
		c.posLocal[1] = objB->mXForm.mulInv( c.pos[1] );
		return true;
	}

	bool BoxBoxAlgo::test( CollideObject* objA , CollideObject* objB , Contact& c)
	{

		return false;
		
	}

}//namespace Phy2D
