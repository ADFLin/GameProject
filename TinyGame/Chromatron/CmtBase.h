#ifndef CmtBase_h__
#define CmtBase_h__

#include <cassert>

#include "Core/IntegerType.h"

#include "Math/TVector2.h"
#include "CycleValue.h"
#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif


namespace Chromatron
{
	using Vec2i = TVector2< int >;
	using DeviceId = unsigned;
	
	DeviceId const ErrorDeviceId = DeviceId( -1 );

	int const NumDir = 8;
	using Dir = TCycleValue< NumDir, int >;

	inline bool IsInRect( Vec2i const& pt , Vec2i const& rectPos , Vec2i const& rectSize )
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
	using Color = unsigned;
	inline Color Complementary( Color color ){ return Color( COLOR_W & (~color) );	 }

}//namespace Chromatron




#endif // CmtBase_h__
