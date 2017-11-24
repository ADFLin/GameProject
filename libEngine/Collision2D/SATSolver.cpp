#include "SATSolver.h"

#include <limits>

bool SATSolver::testIntersect(Vector2 const& pA, Vector2 vA[], int nA, Vector2 const& pB, float radius)
{
	Vector2 posRel = pA - pB;
	float rangeB[2] = { -radius , radius };
	float depthMin = std::numeric_limits< float >::max();
	int   idx = -1;
	for( int i = 0, prev = nA - 1; i < nA; ++i )
	{
		{
			Vector2 edge = vA[i] - vA[prev];
			Vector2 axis = Vector2(edge.y, -edge.x);
			
			float len = axis.normalize();

			float rangeA[2];
			CalcRange(rangeA, axis, vA, nA);
			float offset = axis.dot(posRel);
			rangeA[0] += offset;
			rangeA[1] += offset;

			if( !IsOverlap(rangeA, rangeB) )
			{
				fResult = CalcDistance(rangeA, rangeB);
				vResult = axis;
				haveSA = true;
				return haveSA;
			}

			float depth = CalcOverlapDepth(rangeA, rangeB);
			if( depth < depthMin )
			{
				depthMin = depth;
				idx = i;
			}
		}

		{
			Vector2 axis = vA[i] + posRel;

			float len = axis.normalize();

			float rangeA[2];
			CalcRange(rangeA, axis, vA, nA);
			float offset = axis.dot(posRel);
			rangeA[0] += offset;
			rangeA[1] += offset;

			if( !IsOverlap(rangeA, rangeB) )
			{
				fResult = CalcDistance(rangeA, rangeB);
				vResult = axis;
				haveSA = true;
				return haveSA;
			}

			float depth = CalcOverlapDepth(rangeA, rangeB);
			if( depth < depthMin )
			{
				depthMin = depth;
				idx = i;
			}
		}
		prev = i;
	}

	fResult = depthMin;
	haveSA = false;
	return haveSA;
}

bool SATSolver::testIntersect(Vector2 const& pA, Vector2 vA[], int nA, Vector2 const& pB, Vector2 vB[], int nB)
{
	Vector2 posRel = pB - pA;

	float depthMin = std::numeric_limits< float >::max();
	int   idx = -1;
	for( int cur = 0, prev = nA - 1; cur < nA; prev = cur++ )
	{
		Vector2 edge = vA[cur] - vA[prev];
		Vector2 axis = Vector2(edge.y, -edge.x);

		float rangeA[2];
		CalcRange(rangeA, axis, vA, nA);
		float rangeB[2];
		CalcRange(rangeB, axis, vB, nB);

		float offset = axis.dot(posRel);
		rangeB[0] += offset;
		rangeB[1] += offset;

		if( !IsOverlap(rangeA, rangeB) )
		{
			fResult = CalcDistance(rangeA, rangeB);
			vResult = axis;
			haveSA = true;
			return haveSA;
		}

		float depth = CalcOverlapDepth(rangeA, rangeB) / axis.length();
		if( depth < depthMin )
		{
			depthMin = depth;
			idx = cur;
		}
	}

	for( int cur = 0, prev = nB - 1; cur < nB; prev = cur++ )
	{
		Vector2 edge = vB[cur] - vB[prev];
		Vector2 axis = Vector2(edge.y, -edge.x);

		float rangeA[2];
		CalcRange(rangeA, axis, vA, nA);
		float rangeB[2];
		CalcRange(rangeB, axis, vB, nB);


		float offset = axis.dot(posRel);
		rangeB[0] += offset;
		rangeB[1] += offset;

		if( !IsOverlap(rangeA, rangeB) )
		{
			fResult = CalcDistance(rangeA, rangeB);
			vResult = axis;
			haveSA = true;
			return haveSA;
		}

		float depth = CalcOverlapDepth(rangeA, rangeB) / axis.length();
		if( depth < depthMin )
		{
			depthMin = depth;
			idx = -cur;
		}
		prev = cur;
	}

	fResult = depthMin;
	haveSA = false;
	return haveSA;
}

bool SATSolver::testBoxIntersect(Vector2 const& pA, Vector2 const& sizeA, Vector2 const& pB, Vector2 const& sizeB, Math2D::Rotation const& rotation)
{
	assert(0);
	return false;
}
