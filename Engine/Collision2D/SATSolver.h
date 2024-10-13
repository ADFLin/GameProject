#pragma once
#ifndef SATSolver_H_9EA83323_CBDD_4954_A099_EC014B17CBD2
#define SATSolver_H_9EA83323_CBDD_4954_A099_EC014B17CBD2

#include "Math/Math2D.h"

typedef Math::Vector2 Vector2;

class SATSolver
{
public:
	bool     haveSA;
	Vector2  vResult;
	float    fResult;

	bool testIntersect(Vector2 const& pA, Vector2 vA[], int nA, Vector2 const& pB, float radius);
	bool testIntersect(Vector2 const& pA, Vector2 vA[], int nA, Vector2 const& pB, Vector2 vB[], int nB);
	bool testBoxIntersect(Vector2 const& posA, Vector2 const& sizeA, Vector2 const& posB, Vector2 const& sizeB, Math::XForm2D const& xform);
	static bool IsOverlap(float rangeA[], float rangeB[])
	{
		return (rangeA[0] <= rangeB[1]) &&
			(rangeB[0] <= rangeA[1]);
	}

	static float CalcOverlapDepth(float rangeA[], float rangeB[])
	{
		assert(IsOverlap(rangeA, rangeB));
		float vMin = Math::Max(rangeA[0], rangeB[0]);
		float vMax = Math::Min(rangeA[1], rangeB[1]);
		return vMax - vMin;
	}

	static float CalcDistance(float rangeA[], float rangeB[])
	{
		assert(!IsOverlap(rangeA, rangeB));
		float vMin = Math::Max(rangeA[0], rangeB[0]);
		float vMax = Math::Min(rangeA[1], rangeB[1]);
		return vMin - vMax;
	}

	static void CalcRange(float range[], Vector2 const& axis, Vector2 const v[], int num)
	{
		float vMax, vMin;
		vMax = vMin = axis.dot(v[0]);
		for( int i = 1; i < num; ++i )
		{
			float value = axis.dot(v[i]);
			if( value < vMin )
				vMin = value;
			else if( value > vMax )
				vMax = value;
		}
		range[0] = vMin;
		range[1] = vMax;
	}

};
#endif // SATSolver_H_9EA83323_CBDD_4954_A099_EC014B17CBD2
