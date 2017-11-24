#ifndef CFGeomUtility_h__
#define CFGeomUtility_h__

#include "CFBase.h"

namespace CFly
{



	class  Plane
	{
	public:
		Plane(){}

		Plane( Vector3 const& v0 , Vector3 const& v1 , Vector3 const&  v2 )
		{
			Vector3 d1 = v1 - v0;
			Vector3 d2 = v2 - v0;
			mNormal = d1.cross( d2 );
			mNormal.normalize();

			mDistance = mNormal.dot( v0 );
		}

		Plane( Vector3 const& normal , Vector3 const& pos )
			:mNormal( normal )
		{
			mNormal.normalize();
			mDistance = mNormal.dot( pos );
		}

		Plane( Vector3 const& n , float d )
			:mNormal( n )
			,mDistance( d )
		{
			mNormal.normalize();
		}

		void swap( Plane& p )
		{
			using std::swap;

			swap( mNormal , p.mNormal );
			swap( mDistance , p.mDistance );
		}

		float calcDistance( Vector3 const& p ) const
		{
			return p.dot( mNormal ) - mDistance;
		}
	private:
		Vector3 mNormal;
		float   mDistance;
	};


	class BoundSphere
	{
	public:
		BoundSphere( Vector3 const& c = Vector3(0,0,0), float r = 0 )
			:center( c ) , radius( r ){ assert( radius >= 0 ); }
		BoundSphere( Vector3 const& max , Vector3 const& min )
			:center( 0.5f * ( max + min ) )
		{
			Vector3 extrent = max - min;
			radius = 0.5f * Math::Max( Math::Max( extrent.x , extrent.y ) , extrent.z );
			assert( radius >= 0 );
		}

		void merge( BoundSphere const& sphere )
		{
			Vector3 dir = sphere.center - center;
			float   dist = dir.length();

			if ( radius > dist + sphere.radius )
				return;
			if ( sphere.radius > dist + radius )
			{
				radius = sphere.radius;
				center = sphere.center;
				return;
			}

			float r = 0.5f * ( radius + sphere.radius + dist );

			center += ( ( r - radius ) / dist ) * dir;
			radius = r;
		}

		void addPoint( Vector3 const& p )
		{
			Vector3 dir = p - center;
			float   dist2 = dir.length2();
			if ( dist2 < radius * radius )
				return;

			radius = Math::Sqrt( dist2);
			//radius = 0.5f * ( radius + dist );
			//center += ( ( radius - dist ) / dist ) * dir;
		}
		bool testIntersect( Plane const& plane ) const
		{
			float dist = plane.calcDistance( center );
			return Math::Abs( dist ) < dist;
		}
		Vector3 const& getCenter() const { return center; }
		float          getRadius() const { return radius; }

		Vector3 center;
		float   radius;
	private:


	};

}//namespace CFly


#endif // CFGeomUtility_h__
