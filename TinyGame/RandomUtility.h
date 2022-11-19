#pragma once
#ifndef RandomUtility_H_4E694775_69BB_4AC7_AE40_EF527C781FF5
#define RandomUtility_H_4E694775_69BB_4AC7_AE40_EF527C781FF5

#include "Math/Vector3.h"

using Math::Vector3;

inline float RandFloat()
{
	return float(::rand()) / RAND_MAX;
}

inline float RandFloat(float min, float max)
{
	return min + (max - min) * RandFloat();
}

inline Vector3 RandVector()
{
	return Vector3(RandFloat(), RandFloat(), RandFloat());
}

inline Vector3 RandVector(Vector3 const& min, Vector3 const& max)
{
	return min + (max - min) * RandVector();
}

inline Vector3 RandDirection()
{
	float s1, c1;
	Math::SinCos(Math::PI * RandFloat(), s1, c1);
	float s2, c2;
	Math::SinCos(2 * Math::PI * RandFloat(), s2, c2);
	return Vector3(s1 * c2, s1 * s2, c1);
}

#endif // RandomUtility_H_4E694775_69BB_4AC7_AE40_EF527C781FF5
