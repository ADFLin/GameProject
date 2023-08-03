#pragma once
#ifndef Base_H_B94F26D1_65CF_41DC_8CED_C603C6AB1349
#define Base_H_B94F26D1_65CF_41DC_8CED_C603C6AB1349

#include <cmath>
#include <cassert>
#include <cfloat>
#include <type_traits>

#include "PlatformConfig.h"
#include "CompilerConfig.h"
#include "EnumCommon.h"

float constexpr FLOAT_DIV_ZERO_EPSILON = 1e-6f;
float constexpr FLOAT_EPSILON = 1e-5f;
namespace Math
{
	float constexpr MaxFloat = FLT_MAX;
	float constexpr MinFloat = FLT_MIN;
	float constexpr PI = 3.141592654f;

	FORCEINLINE int FloorToInt(float val) { return (int)::floor(val); }
	FORCEINLINE int CeilToInt(float val) { return (int)::ceil(val); }
	FORCEINLINE int RoundToInt(float val) { return (int)::round(val); }

	FORCEINLINE float Floor(float val) { return ::floor(val); }
	FORCEINLINE float Ceil(float val) { return ::ceil(val); }

	FORCEINLINE float Sqrt( float val ){ return ::sqrt( val ); }
	FORCEINLINE float InvSqrt( float val ){ return 1.0f / Sqrt( val ); }
	FORCEINLINE float Sin( float val ){ return ::sin( val ); }
	FORCEINLINE float Cos( float val ){ return ::cos( val ); }
	FORCEINLINE float Tan( float val ){ return ::tan( val ); }
	FORCEINLINE void  SinCos( float val , float& s , float& c  ){ s = ::sin( val ); c = ::cos( val ); }
	FORCEINLINE float ACos( float val){ return ::acos( val ); }
	FORCEINLINE float ASin( float val){ return ::asin( val ); }
	FORCEINLINE float ATan( float val ){ return ::atan( val ); }
	FORCEINLINE float Abs( float val ){ return ::fabs( val ); }
	FORCEINLINE float ATan2( float y , float x ){ return ::atan2( y ,x ); }
	FORCEINLINE float Frac(float val) { float temp; return ::modf(val, &temp); }
	FORCEINLINE float Log(float val) { return ::logf(val); }
	FORCEINLINE float LogX(float base, float value) { return Log(value) / Log(base); }

	template <typename T>
	FORCEINLINE float Sign(T x)
	{
		return (T(0) < x) - (x < T(0));
	}

	FORCEINLINE double FloorToInt(double val) { return ::floor(val); }
	FORCEINLINE double Abs(double val) { return ::abs(val); }
	FORCEINLINE double Tanh(double val) { return ::tanh(val); }
	FORCEINLINE double Exp(double val) { return ::exp(val); }

	FORCEINLINE float Round( float value ){ return ::floor( value + 0.5f ); }
	FORCEINLINE float Fmod( float v1 , float v2 ){ return ::fmod( v1 , v2 ); }
	FORCEINLINE float DegToRad( float val ){ return val * Math::PI / 180.0f; }
	FORCEINLINE float RadToDeg( float val ){ return val * 180.0f / Math::PI; }
	FORCEINLINE float Pow(float base, float exp) { return ::pow(base, exp); }
	FORCEINLINE float Lerp(float form, float to, float alpha) { return form * (1 - alpha) + to * alpha;  }

	template< class T >
	FORCEINLINE T Abs(T val) { return (val >= 0) ? val : -val; }
	template< class T >
	FORCEINLINE T Square(T val) { return val * val; }

	template< class T >
	FORCEINLINE T Min(T v1, T v2) { return (v1 < v2) ? v1 : v2; }
	template< class T >
	FORCEINLINE T Max(T v1, T v2) { return (v1 > v2) ? v1 : v2; }

	template< class T >
	FORCEINLINE T Clamp(T val, T minVal, T maxVal)
	{
		if( val < minVal )
			return minVal;
		if( val > maxVal )
			return maxVal;
		return val;
	}

	template< class T >
	FORCEINLINE T LinearLerp(T const& p0, T const& p1, float alpha)
	{
		return (1 - alpha) * p0 + alpha * p1;
	}

	template< class T >
	FORCEINLINE T BezierLerp(T const& p0, T const& p1, T const& p2, float alpha)
	{
		float frac = 1 - alpha;
		return (frac * frac) * p0 + (2 * frac * alpha) * p1 + (alpha * alpha) * p2;
	}

	template< class T >
	FORCEINLINE T BezierLerp(T const& p0, T const& p1, T const& p2, T const& p3, float alpha)
	{
		float frac = 1 - alpha;
		float frac2 = frac * frac;
		float alpha2 = alpha * alpha;
		return (frac * frac2) * p0 + (3 * frac2 * alpha) * p1 + (3 * frac * alpha2) * p2 + (alpha * alpha2) * p3;
	}

	template< class T >
	FORCEINLINE bool Barycentric(T const& p, T const& a, T const& b, T const& c, float coord[])
	{
		T v0 = b - a, v1 = c - a, v2 = p - a;
		float d00 = v0.dot(v0);
		float d01 = v0.dot(v1);
		float d11 = v1.dot(v1);
		float d20 = v2.dot(v0);
		float d21 = v2.dot(v1);
		float denom = d00 * d11 - d01 * d01;

		if( Math::Abs(denom) < FLOAT_DIV_ZERO_EPSILON )
			return false;

		coord[1] = (d11 * d20 - d01 * d21) / denom;
		coord[2] = (d00 * d21 - d01 * d20) / denom;
		coord[0] = 1.0f - coord[1] - coord[2];

		return true;
	}

	template< class T >
	constexpr T ToTileValue(T value, T tileLength)
	{
		if constexpr (std::is_signed_v< T >)
		{
			if (value < 0)
				value -= tileLength - 1;
		}
		return value / tileLength;
	}

	template< typename T >
	constexpr T PairingFunction(T a, T b)
	{
		T p = a + b;
		return (p + 1) * p / 2 + b;
	}
}//namespace Math


#endif // Base_H_B94F26D1_65CF_41DC_8CED_C603C6AB1349
