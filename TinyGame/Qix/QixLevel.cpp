#include "TinyGamePCH.h"
#include "QixLevel.h"


namespace Qix
{
	Region::Region( Vec2i const& from , Vec2i const& to )
	{
		assert( from.x < to.x && from.y < to.y );
		CLine* lines[4];
		for ( int i = 0 ; i < 4 ; ++i )
			lines[i] = new CLine;

		//ccw
		lines[0]->conFront( *lines[1] , from );
		lines[1]->conFront( *lines[2] , Vec2i( from.y , to.x ) );
		lines[2]->conFront( *lines[3] , to );
		lines[3]->conFront( *lines[0] , Vec2i( to.x , from.y ) );

		mBounds = lines[0];
		Vec2i dif = to - from;
		mArea = dif.x * dif.y;
	}

	Region::~Region()
	{
		CLine* cur = mBounds;
		do 
		{
			CLine* nt = cur->next();
			delete cur;
			cur = nt;
		} while ( cur != mBounds );
	}

	int Region::addAreaSide( Vec2i pts[] , int num , CLine& sideTouch )
	{
		int area = pts[ num ].x * pts[ 0 ].y - pts[ num ].y * pts[ 0 ].x;
		for ( int i = 1 ; i < num; ++i )
		{
			area += pts[i-1].x * pts[i].y - pts[i-1].y * pts[i].x;
		}
		area /= 2;

		CLine* incBound[2];
		if ( area < 0 )
		{
			area = -area;
			CLine* cur = new CLine;
			incBound[0] = cur;
			for ( int i = 1 ; i < num ; ++i )
			{
				CLine* con = new CLine;
				cur->conFront( *con , pts[i] );
				cur = con;
			}
			incBound[1] = cur;
		}
		else
		{

			CLine* cur = new CLine;
			incBound[1] = cur;
			for ( int i = 1 ; i < num ; ++i )
			{
				CLine* con = new CLine;
				cur->conBack( *con , pts[i] );
				cur = con;
			}
			incBound[0] = cur;
		}

		return area;
	}

}//namespace Qix