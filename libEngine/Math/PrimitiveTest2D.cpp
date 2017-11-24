#include "PrimitiveTest2D.h"

#include <algorithm>

namespace Math2D
{

	bool LineBoxTest(Vector2 const& rayPos, Vector2 const& rayDir, Vector2 const& boxMin, Vector2 const& boxMax, float outDistances[])
	{
		assert(rayDir.isNormalize());

		float dMin = Math::MinFloat; // 0 for ray
		float dMax = Math::MaxFloat;
		for( int i = 0; i < 2; ++i )
		{
			if( Math::Abs(rayDir[i]) < FLT_DIV_ZERO_EPSILON )
			{
				if( rayPos[i] < boxMin[i] || rayPos[i] > boxMax[i] )
					return false;
			}
			else
			{
				float invD = 1.0 / rayDir[i];
				float d1 = (boxMin[i] - rayPos[i]) * invD;
				float d2 = (boxMax[i] - rayPos[i]) * invD;

				if( d1 > d2 )
					std::swap(d1, d2);

				if( d1 > dMin )
					dMin = d1;
				if( d2 < dMax )
					dMax = d2;
				if( dMin > dMax )
					return false;
			}
		}

		outDistances[0] = dMin;
		outDistances[1] = dMax;
		return true;
	}

	static bool IsIntersect(float minA, float maxA, float minB, float maxB)
	{
		if( maxA < minB )
			return false;
		if( maxB < minA )
			return false;
		return true;
	}


	bool BoxBoxTest(Vector2 const& aMin, Vector2 const& aMax, Vector2 const& bMin, Vector2 const& bMax)
	{
		return IsIntersect(aMin.x, aMax.x, bMin.x, bMax.x) && IsIntersect(aMin.y, aMax.y, bMin.y, bMax.y);
	}

	bool LineCircleTest(Vector2 const& rPos, Vector2 const& rDir, Vector2 const& cPos, float cRadius, float t[])
	{
		assert(rDir.isNormalize());

		Vector2 offset = cPos - rPos;
		float B = offset.dot(rDir);
		float C = offset.length2() - cRadius * cRadius;

		float D = B * B - C;

		if( D < 0 )
			return false;

		D = Math::Sqrt(D);

		t[0] = B - D;
		t[1] = B + D;

		return  true;
	}

}//namespace Math2D

