#include "Shape.h"

namespace Phy2D
{

	struct AABBUpdater
	{
		AABBUpdater( AABB& aabb ):aabb(aabb)
		{
			aabb.min.x = std::numeric_limits< float >::max();
			aabb.min.y = std::numeric_limits< float >::max();
			aabb.max.x = std::numeric_limits< float >::min();
			aabb.max.y = std::numeric_limits< float >::min();
#if _DEBUG
			count = 0;
#endif
			
		}
		~AABBUpdater()
		{
			assert( count >= 2 );
		}

		void add( Vector2 const& p )
		{
			if ( aabb.min.x > p.x )
				aabb.min.x = p.x;
			else if ( aabb.max.x < p.x )
				aabb.max.x = p.x;
			if ( aabb.min.y > p.y )
				aabb.min.y = p.x;
			else if ( aabb.max.y < p.y )
				aabb.max.y = p.y;

#if _DEBUG
			++count;
#endif

		}
#if _DEBUG
		int count;
#endif
		AABB& aabb;
	};

	void BoxShape::calcAABB(XForm2D const& xform , AABB& aabb)
	{
		Vector2 len = xform.transformVector( mHalfExt );
		Vector2 dir = xform.getRotation().getXDir();
		dir.x = Math::Abs( dir.x );
		dir.y = Math::Abs( dir.y );
		len.x = mHalfExt.x * dir.x + mHalfExt.y * dir.y;
		len.y = mHalfExt.x * dir.y + mHalfExt.y * dir.x;
		aabb.min = xform.getPos() - len;
		aabb.max = xform.getPos() + len;
	}

	void BoxShape::calcAABB(  AABB& aabb )
	{
		aabb.min = -mHalfExt;
		aabb.max = mHalfExt;
	}

	void BoxShape::calcMass(MassInfo& info)
	{
		info.m =  mHalfExt.x * mHalfExt.y * 4;
		info.I = ( 4 / 12.0f ) * info.m * ( mHalfExt.length2() );
		info.center = Vector2(0,0);
	}

	Vector2 BoxShape::getSupport(Vector2 const& dir)
	{
		return Vector2(
			( dir.x >= 0 ) ? mHalfExt.x : -mHalfExt.x ,
			( dir.y >= 0 ) ? mHalfExt.y : -mHalfExt.y );
	}

	void CircleShape::calcAABB(XForm2D const& xform , AABB& aabb)
	{
		aabb.min = xform.getPos() - Vector2( mRadius , mRadius );
		aabb.max = xform.getPos() + Vector2( mRadius , mRadius );
	}

	void CircleShape::calcAABB(AABB& aabb)
	{
		aabb.min = Vector2( -mRadius , -mRadius );
		aabb.max = Vector2( mRadius , mRadius );
	}

	void CircleShape::calcMass(MassInfo& info)
	{
		float r2 = mRadius * mRadius;
		info.m = Math::PI * r2;
		info.I = ( 1 / 2.0f ) * info.m * r2;
		info.center = Vector2(0,0);
	}

	Vector2 CircleShape::getSupport(Vector2 const& dir)
	{
		Vector2 d = dir;
		d.normalize();
		return mRadius * d;	
	}


	void PolygonShape::calcAABB(XForm2D const& xform , AABB& aabb)
	{
		AABBUpdater updater( aabb );
		for( int i = 0 ; i < getVertexNum() ; ++i )
		{
			Vector2 v = xform.transformPosition( mVertices[i] );
			updater.add( v );
		}
	}

	void PolygonShape::calcAABB(AABB& aabb)
	{
		AABBUpdater updater( aabb );
		for( int i = 0 ; i < getVertexNum() ; ++i )
		{
			updater.add( mVertices[i] );
		}
	}

	Vector2 PolygonShape::getSupport(Vector2 const& dir)
	{
		float vMax = dir.dot( mVertices[0] );
		int idx = 0;
		for( int i = 1 ; i < getVertexNum() ; ++i )
		{
			float val = dir.dot( mVertices[i] );

			if ( val > vMax )
			{
				idx = i;
				vMax = val;
			}
		}
		return mVertices[idx];
	}

	void PolygonShape::calcMass(MassInfo& info )
	{
		int prev = getVertexNum();
		info.I = 0;
		info.m = 0;
		info.center = Vector2::Zero();
		for( int i = 0 ; i < getVertexNum() ; prev = i++ )
		{
			Vector2 const& v = mVertices[i];
			Vector2 const& vP = mVertices[ prev ];

			float area = 0.5f * vP.cross( v );
			float b2 = vP.length2();
			float h2  = 4 * area * area / b2;
			info.m += area;
			info.I += ( area ) * ( 11 * b2 / 4 +  h2 ) / 9;
			info.center += v;
		}
		info.center /= (float)getVertexNum();
	}

	void CapsuleShape::calcAABB(AABB& aabb)
	{
		aabb.min = -mHalfExt;
		aabb.min.y += -mHalfExt.x; 
		aabb.max = mHalfExt;
		aabb.min.y += +mHalfExt.x;
	}

	void CapsuleShape::calcAABB(XForm2D const& xform , AABB& aabb)
	{
		Vector2 offset = mHalfExt.y * xform.getRotation().getYDir() + Vector2( getRadius() , getRadius() );
		aabb.min = -offset;
		aabb.max = offset; 
	}

	void CapsuleShape::calcMass(MassInfo& info )
	{
		info.center = Vector2::Zero();
		info.m      = ( 4 * mHalfExt.x * mHalfExt.y + Math::PI * getRadius() );
		//FIXME
		info.I      = info.m * mHalfExt.x;
	}

	Vector2 CapsuleShape::getSupport(Vector2 const& dir)
	{
		if ( Math::Abs( mHalfExt.y * dir.x ) < Math::Abs( mHalfExt.x * dir.y ) )
		{
			return Vector2( dir.x >= 0 ? mHalfExt.x : -mHalfExt.x ,
				          dir.y >= 0 ? mHalfExt.y : -mHalfExt.y );
		}

		Vector2 result = getRadius() * dir / dir.length();
		result.y += ( dir.y > 0 ) ? mHalfExt.y : -mHalfExt.y;
		return result;
	}

}//namespace Phy2D

