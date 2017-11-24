#include "Phy2D.h"

#ifdef min
#undef min
#undef max
#endif // min

namespace Phy2D
{

	template< class T >
	static bool Barycentric( T const& p , T const& a, T const& b, T const& c, float coord[] )
	{
		T v0 = b - a, v1 = c - a, v2 = p-a;
		float d00 = v0.dot(v0);
		float d01 = v0.dot(v1);
		float d11 = v1.dot(v1);
		float d20 = v2.dot(v0);
		float d21 = v2.dot(v1);
		float denom = d00 * d11 - d01 * d01;

		if ( Math::Abs( denom ) < FLT_DIV_ZERO_EPSILON )
			return false;

		coord[1] = (d11 * d20 - d01 * d21) / denom;
		coord[2] = (d00 * d21 - d01 * d20) / denom;
		coord[0] = 1.0f - coord[1] - coord[2];

		return true;
	}

	inline bool isInside( Vector2 const& min , Vector2 const& max , Vector2 const& p )
	{
		return min.x < p.x && p.x < max.x &&
			   min.y < p.y && p.y < max.y ;
	}
	static bool raycastAABB( Vector2 const& org , Vector2 const& dir , Vector2 const& min , Vector2 const& max , float& outT )
	{
		if ( isInside( min , max , org ) )
		{
			outT = 0;
			return true;
		}

		float factor[2];
		for( int i = 0 ; i < 2 ; ++i )
		{
			if ( dir[i] > FLT_DIV_ZERO_EPSILON )
			{
				factor[i] = ( min[i] - org[i] ) / dir[i];
			}
			else if ( dir[i] < -FLT_DIV_ZERO_EPSILON )
			{
				factor[i] = ( max[i] - org[i] ) / dir[i];
			}
			else
			{
				factor[i] = std::numeric_limits<float>::min();
			}
		}

		int idx = ( factor[0] > factor[1] ) ? ( 0 ) : ( 1 );
		if ( factor[idx] < 0 )
			return false;

		Vector2 p = org + dir * factor[idx];
		int idx1 = ( idx + 1 ) % 2;
		if ( min[idx1] > p[idx1] || p[idx1] > max[idx1] )
		{
			return false;
		}

		outT = factor[idx];
		return true;

	}



}//namespace Phy2D