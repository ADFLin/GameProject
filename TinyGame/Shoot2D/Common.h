#ifndef Common_H
#define Common_H

#include "Commonfwd.h"

namespace Shoot2D
{
	float const PI = 3.141592653589793238462643383279f;
	enum ObjStats
	{
		STATS_DEAD = 0,
		STATS_LIVE,
		STATS_DISAPPER,
		STATS_CLEAR,
	};

	enum ObjTeam
	{
		TEAM_FRIEND,
		TEAM_EMPTY,
	};

	enum GeomType
	{
		GEOM_RECT ,
		GEOM_CIRCLE ,


		NUM_GEOM_TYPE ,
	};

	enum ColFlag
	{
		COL_NO_NEED        = 0,
		COL_IS_BULLET      = 1 << 0,
		COL_IS_VEHICLE     = 1 << 1,
		COL_EMPTY_BULLET   = 1 << 2,
		COL_EMPTY_VEHICLE  = 1 << 3,
		COL_FRIEND_BULLET  = 1 << 4,
		COL_FRIEND_VEHICLE = 1 << 5,

		COL_EMPTY = COL_EMPTY_BULLET | COL_EMPTY_VEHICLE ,

		COL_DEFAULT_VEHICLE = COL_IS_VEHICLE | COL_EMPTY ,
		COL_DEFAULT_BULLET  = COL_IS_BULLET  | COL_EMPTY_VEHICLE ,
		COL_BORKEN_BULLET = COL_DEFAULT_BULLET | COL_EMPTY_BULLET ,
	};

	inline float Random( float min , float max , int step = 1000)
	{
		return min + ( max - min )*( rand() % step ) / step ;
	}

}//namespace Shoot2D

#endif Common_H