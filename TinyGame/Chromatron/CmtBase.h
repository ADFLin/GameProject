#ifndef CmtBase_h__
#define CmtBase_h__

#include <cassert>

#include "Core/IntegerType.h"

#include "TVector2.h"
#include "TCycleNumber.h"
#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif


namespace Chromatron
{
	typedef TVector2< int > Vec2i;
	typedef unsigned DeviceId;
	typedef unsigned Color;
	DeviceId const ErrorDeviceId = DeviceId( -1 );

	inline bool isInRect( Vec2i const& pt , Vec2i const& rectPos , Vec2i const& rectSize )
	{
		return rectPos.x <= pt.x && pt.x < rectPos.x + rectSize.x && 
			   rectPos.y <= pt.y && pt.y < rectPos.y + rectSize.y;
	}

	enum
	{
		COLOR_NULL = 0x00000,
		COLOR_R    = 0x00001,
		COLOR_G    = 0x00002,
		COLOR_B    = 0x00004,
		COLOR_W    = COLOR_R | COLOR_G | COLOR_B,
		COLOR_CR   = COLOR_W & (~COLOR_R),
		COLOR_CG   = COLOR_W & (~COLOR_G),
		COLOR_CB   = COLOR_W & (~COLOR_B),
		
	};

	inline Color complementary( Color color ){ return Color( COLOR_W & (~color) );	 }

	int const NumDir = 8;
	typedef TCycleNumber< NumDir , int > Dir;


}//namespace Chromatron




#endif // CmtBase_h__
