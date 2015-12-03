#ifndef CollisionSystem_h__
#define CollisionSystem_h__

#include "CommonFwd.h"
#include <algorithm>

namespace Shoot2D
{
	class CollisionSystem
	{
	public:
		static void processCollision(Object& obj1 , Object& obj2);
		static bool isNeedTestCollision(Object& obj1 , Object& obj2 );
		static bool testCollisionFlag( unsigned flag1, unsigned flag2 ,bool isEmpty );
		static bool testCollision(Object& obj1 , Object& obj2 );
	};

	inline bool testInterection( Vec2D const& p1 , float r1 ,Vec2D const& p2 , float r2 )
	{
		Vec2D dr = p1 - p2;
		float r = r1 + r2;
		return dr.length2() < r * r;
	}

	inline bool testInterection( Vec2D const& p1 , Vec2D& len1 ,Vec2D const& p2 , Vec2D& len2 )
	{
		float Xmax = std::min( p1.x + len1.x , p2.x + len2.x );
		float Xmin = std::max( p1.x , p2.x );

		if ( Xmax <= Xmin ) 
			return false;

		float Ymax = std::min( p1.y + len1.y , p2.y + len2.y );
		float Ymin = std::max( p1.y , p2.y );

		if ( Ymax <= Ymin ) 
			return false;

		return true;
	}

	inline bool testInterection( Vec2D const& p1 , Vec2D& len1 ,Vec2D const& p2 , float r2 )
	{
		return p1.x - r2 < p2.x && p2.x < p1.x + len1.x + r2 &&
			p1.y - r2 < p2.y && p2.y < p1.y + len1.y + r2;
	}

}//namespace Shoot2D

#endif // CollisionSystem_h__