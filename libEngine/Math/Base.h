#ifndef Base_h__
#define Base_h__

#include <cmath>
#include <cassert>

namespace Math
{

	float const PI = 3.141592654f;
	inline float Floor( float val ){ return ::floor( val ); }
	inline float Sqrt( float val ){ return ::sqrt( val ); }
	inline float InvSqrt( float val ){ return 1.0f / Sqrt( val ); }
	inline float Sin( float val ){ return ::sin( val ); }
	inline float Cos( float val ){ return ::cos( val ); }
	inline float Tan( float val ){ return ::tan( val ); }
	inline void  SinCos( float val , float& s , float& c  ){ s = ::sin( val ); c = ::cos( val ); }
	inline float ACos( float val){ return ::acos( val ); }
	inline float ASin( float val){ return ::asin( val ); }
	inline float ATan( float val ){ return ::atan( val ); }
	inline float Abs( float val ){ return ::fabs( val ); }
	inline float ATan2( float y , float x ){ return ::atan2( y ,x ); }
	inline float Min( float v1 , float v2 ){ return ( v1 < v2 ) ? v1 : v2; }
	inline float Max( float v1 , float v2 ){ return ( v1 > v2 ) ? v1 : v2; }
	inline float Clamp( float val , float minVal , float maxVal )
	{
		return Min( Max(  minVal , val ) , maxVal );
	}
	inline float Round( float value ){ return ::floor( value + 0.5f ); }
	inline float Fmod( float v1 , float v2 ){ return ::fmod( v1 , v2 ); }
	inline float Deg2Rad( float val ){ return val * Math::PI / 180.0f; }
	inline float Rad2Deg( float val ){ return val * 180.0f / Math::PI; }

}//namespace Math

#endif // Base_h__
