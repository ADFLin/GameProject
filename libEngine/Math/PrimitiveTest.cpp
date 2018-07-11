#include "PrimitiveTest.h"

#include <algorithm>

namespace Math
{


	template< class VectorType >
	bool LineBoxTestT(VectorType const& rayPos, VectorType const& rayDir, VectorType const& boxMin, VectorType const& boxMax, float outDistances[])
	{
		assert(rayDir.isNormalize());

		float dMin = Math::MinFloat; // 0 for ray
		float dMax = Math::MaxFloat;
		for( int i = 0; i < GetDim<VectorType>::Result; ++i )
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


	bool LineBoxTest(Vector2 const& rayPos, Vector2 const& rayDir, Vector2 const& boxMin, Vector2 const& boxMax, float outDistances[])
	{
		return LineBoxTestT(rayPos, rayDir, boxMin, boxMax, outDistances);
	}

	bool LineBoxTest(Vector3 const& rayPos, Vector3 const& rayDir, Vector3 const& boxMin, Vector3 const& boxMax, float outDistances[])
	{
		return LineBoxTestT(rayPos, rayDir, boxMin, boxMax, outDistances);
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

	bool BoxBoxTest(Vector3 const& aMin, Vector3 const& aMax, Vector3 const& bMin, Vector3 const& bMax)
	{
		return IsIntersect(aMin.x, aMax.x, bMin.x, bMax.x) && IsIntersect(aMin.y, aMax.y, bMin.y, bMax.y) && IsIntersect(aMin.z, aMax.z, bMin.z, bMax.z);;
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

	Vector3 PointToTriangleClosestPoint(Vector3 const& p, Vector3 const& a, Vector3 const& b, Vector3 const& c, float& outSide)
	{
		Vector3 ab = b - a;
		Vector3 ac = c - a;
		Vector3 bc = c - b;

		Vector3 ap = p - a;
		Vector3 bp = p - b;
		Vector3 cp = p - c;

		Vector3 n = Math::Cross(ab, ac);
		outSide = (Math::Dot(n, ap) >= 0) ? 1 : -1;

		// Compute parametric position s for projection P・ of P on AB,
		// P・ = A + s*AB, s = snom/(snom+sdenom)
		float snom = Math::Dot(ap, ab), sdenom = -Math::Dot(bp, ab);
		// Compute parametric position t for projection P・ of P on AC,
		// P・ = A + t*AC, s = tnom/(tnom+tdenom)
		float tnom = Math::Dot(ap, ac), tdenom = -Math::Dot(cp, ac);
		if( snom <= 0.0f && tnom <= 0.0f )
			return a;  // Vertex region early out

					   // Compute parametric position u for projection P・ of P on BC,
					   // P・ = B + u*BC, u = unom/(unom+udenom)
		float unom = Math::Dot(bp, bc), udenom = -Math::Dot(cp, bc);
		if( sdenom <= 0.0f && unom <= 0.0f )
			return b; // Vertex region early out
		if( tdenom <= 0.0f && udenom <= 0.0f )
			return c; // Vertex region early out

					  // P is outside (or on) AB if the triple scalar product [N PA PB] <= 0

		float vc = Math::Dot(n, Math::Cross(ap, bp));
		// If P outside AB and within feature region of AB,
		// return projection of P onto AB
		if( vc <= 0.0f && snom >= 0.0f && sdenom >= 0.0f )
			return a + snom / (snom + sdenom) * ab;
		// P is outside (or on) BC if the triple scalar product [N PB PC] <= 0
		float va = Math::Dot(n, Math::Cross(bp, cp));
		// If P outside BC and within feature region of BC,
		// return projection of P onto BC
		if( va <= 0.0f && unom >= 0.0f && udenom >= 0.0f )
			return b + unom / (unom + udenom) * bc;
		// P is outside (or on) CA if the triple scalar product [N PC PA] <= 0
		float vb = Math::Dot(n, Math::Cross(cp, ap));
		// If P outside CA and within feature region of CA,
		// return projection of P onto CA
		if( vb <= 0.0f && tnom >= 0.0f && tdenom >= 0.0f )
			return a + tnom / (tnom + tdenom) * ac;

		// P must project inside face region. Compute Q using barycentric coordinates
		float u = va / (va + vb + vc);
		float v = vb / (va + vb + vc);
		float w = 1.0f - u - v; // = vc / (va + vb + vc)
		return u * a + v * b + w * c;
	}

}//namespace Math2D

