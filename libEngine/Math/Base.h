#pragma once
#ifndef Base_H_B94F26D1_65CF_41DC_8CED_C603C6AB1349
#define Base_H_B94F26D1_65CF_41DC_8CED_C603C6AB1349

#include <cmath>
#include <cassert>
#include <cfloat>

#include "CompilerConfig.h"
#include "EnumCommon.h"

float constexpr FLT_DIV_ZERO_EPSILON = 1e-6f;

namespace Math
{
	float constexpr MaxFloat = FLT_MAX;
	float constexpr MinFloat = FLT_MIN;
	float constexpr PI = 3.141592654f;

	inline int FloorToInt(float val) { return (int)::floor(val); }
	inline int CeilToInt(float val) { return (int)::ceil(val); }

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
	inline float Frac(float val) { float temp; return ::modf(val, &temp); }
	inline float Log(float val) { return ::logf(val); }

	inline double Abs(double val) { return ::abs(val); }
	inline double Tanh(double val) { return ::tanh(val); }
	inline double Exp(double val) { return ::exp(val); }


	inline float Round( float value ){ return ::floor( value + 0.5f ); }
	inline float Fmod( float v1 , float v2 ){ return ::fmod( v1 , v2 ); }
	inline float Deg2Rad( float val ){ return val * Math::PI / 180.0f; }
	inline float Rad2Deg( float val ){ return val * 180.0f / Math::PI; }
	inline float Pow(float base, float exp) { return ::pow(base, exp); }
	inline float Lerp(float form, float to, float alpha) { return form * (1 - alpha) + to * alpha;  }

	template< class T >
	inline T Squre(T val) { return val * val; }

	template< class T >
	inline T Min(T v1, T v2) { return (v1 < v2) ? v1 : v2; }
	template< class T >
	inline T Max(T v1, T v2) { return (v1 > v2) ? v1 : v2; }
	template< class T >
	inline T Clamp(T val, T minVal, T maxVal)
	{
		return Min(Max(minVal, val), maxVal);
	}

	template< class T >
	static T LinearLerp(T const& p0, T const& p1, float alpha)
	{
		return (1 - alpha) * p0 + alpha * p1;
	}

	template< class T >
	static T BezierLerp(T const& p0, T const& p1, T const& p2, float alpha)
	{
		float frac = 1 - alpha;
		return (frac * frac) * p0 + (2 * frac * alpha) * p1 + (alpha * alpha) * p2;
	}

	template< class T >
	static T BezierLerp(T const& p0, T const& p1, T const& p2, T const& p3, float alpha)
	{
		float frac = 1 - alpha;
		float frac2 = frac * frac;
		float alpha2 = alpha * alpha;
		return (frac * frac2) * p0 + (3 * frac2 * alpha) * p1 + (3 * frac * alpha2) * p2 + (alpha * alpha2) * p3;
	}

	template< class T >
	static bool Barycentric(T const& p, T const& a, T const& b, T const& c, float coord[])
	{
		T v0 = b - a, v1 = c - a, v2 = p - a;
		float d00 = v0.dot(v0);
		float d01 = v0.dot(v1);
		float d11 = v1.dot(v1);
		float d20 = v2.dot(v0);
		float d21 = v2.dot(v1);
		float denom = d00 * d11 - d01 * d01;

		if( Math::Abs(denom) < FLT_DIV_ZERO_EPSILON )
			return false;

		coord[1] = (d11 * d20 - d01 * d21) / denom;
		coord[2] = (d00 * d21 - d01 * d20) / denom;
		coord[0] = 1.0f - coord[1] - coord[2];

		return true;
	}
}//namespace Math


#endif // Base_H_B94F26D1_65CF_41DC_8CED_C603C6AB1349
