#include "PrimitiveTest.h"

#include <algorithm>

namespace Math
{


	template< class VectorType >
	bool LineBoxTestT(VectorType const& rayPos, VectorType const& rayDir, VectorType const& boxMin, VectorType const& boxMax, float outDistances[])
	{
		assert(rayDir.isNormalized());

		float dMin = -Math::MaxFloat; // 0 for ray
		float dMax = Math::MaxFloat;
		for( int i = 0; i < GetDim<VectorType>::Result; ++i )
		{
			if( Math::Abs(rayDir[i]) < FLOAT_DIV_ZERO_EPSILON )
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


	bool LineAABBTest(Vector2 const& rayPos, Vector2 const& rayDir, Vector2 const& boxMin, Vector2 const& boxMax, float outDistances[])
	{
		return LineBoxTestT(rayPos, rayDir, boxMin, boxMax, outDistances);
	}

	bool LineAABBTest(Vector3 const& rayPos, Vector3 const& rayDir, Vector3 const& boxMin, Vector3 const& boxMax, float outDistances[])
	{
		return LineBoxTestT(rayPos, rayDir, boxMin, boxMax, outDistances);
	}


	inline bool IsInside(Vector2 const& min, Vector2 const& max, Vector2 const& p)
	{
		return min.x < p.x && p.x < max.x &&
			min.y < p.y && p.y < max.y;
	}

	static bool RayAABB(Vector2 const& org, Vector2 const& dir, Vector2 const& min, Vector2 const& max, float& outT)
	{
		float factors[2];
		for( int i = 0; i < 2; ++i )
		{
			if( dir[i] > FLOAT_DIV_ZERO_EPSILON )
			{
				factors[i] = (min[i] - org[i]) / dir[i];
			}
			else if( dir[i] < -FLOAT_DIV_ZERO_EPSILON )
			{
				factors[i] = (max[i] - org[i]) / dir[i];
			}
			else
			{
				factors[i] = std::numeric_limits<float>::lowest();
			}
		}

		int idx = (factors[0] > factors[1]) ? (0) : (1);
		if( factors[idx] < 0 )
			return false;

		Vector2 p = org + dir * factors[idx];
		int idx1 = (idx + 1) % 2;
		if( min[idx1] > p[idx1] || p[idx1] > max[idx1] )
		{
			return false;
		}

		outT = factors[idx];
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

	bool BoxBoxTest(Vector3 const& aMin, Vector3 const& aMax, Vector3 const& bMin, Vector3 const& bMax)
	{
		return IsIntersect(aMin.x, aMax.x, bMin.x, bMax.x) && IsIntersect(aMin.y, aMax.y, bMin.y, bMax.y) && IsIntersect(aMin.z, aMax.z, bMin.z, bMax.z);;
	}

	bool RaySphereTest(Vector3 const& pos, Vector3 const& dirNormalized, Vector3 const& center, float radius, float& outT)
	{
		// ( ld + vd * t ) ^ 2 = r^2 ; ld = pos - center
		// t^2 + 2 ( ld * vd ) t + ( ld^2 - r^2 ) = 0
		Vector3 offset = pos - center;
		float b = offset.dot(dirNormalized);
#if 1
		float radius2 = radius * radius;
		// d = b * b - c = ( ld * vd )^2 - ld^2 + r^2
		//   = r^2 - (ld - ( ld * vd ) vd)^2
		float d = radius2 - (offset - b * dirNormalized).length2();
#else
		float c = offset.length2() - radius * radius;
		float d = b * b - c;
#endif

		if (d < 0)
		{
			outT = (offset - b * dirNormalized).length();
			return false;
		}
		outT = -b - Math::Sqrt(d);
		return true;
	}

	bool LineSphereTest(Vector3 const& pos, Vector3 const& dirNormalized, Vector3 const& center, float radius, float outDistance[2])
	{
		Vector3 offset = pos - center;
		float b = offset.dot(dirNormalized);
#if 1
		float radius2 = radius * radius;
		float d = radius2 - (offset - b * dirNormalized).length2();
#else
		float c = offset.length2() - radius * radius;
		float d = b * b - c;
#endif
		if (d < 0)
			return false;

		d = Math::Sqrt(d);
		outDistance[0] = b - d;
		outDistance[1] = b + d;
		return true;
	}

	// pA + dA * ta = pB + dB * tb,  d = pB - pA
	// A = [ dax -dbx ], T = [ta], D = [ dx ],  A * T = D  => T = A(-1) * D = (1/det) * [-dby  dbx][dx]
	//     [ day -dby ]      [tb]      [ dy ]                                           [-day  dax][dy]
	bool LineLineTest(Vector2 const& posA, Vector2 const& dirA, Vector2 const& posB, Vector2 const& dirB, Vector2& outPos)
	{
		float det = dirA.y * dirB.x - dirA.x * dirB.y;
		if( Math::Abs(det) < FLOAT_DIV_ZERO_EPSILON )
			return false;

		Vector2 d = posB - posA;
		float t = (d.y * dirB.x - d.x * dirB.y) / det;
		outPos = posA + t * dirA;
		return true;
	}

	bool SegmentSegmentTest(Vector2 const& posA1, Vector2 const& posA2, Vector2 const& posB1, Vector2 const& posB2, Vector2& outPos)
	{
		Vector2 dirA = posA2 - posA1;
		Vector2 dirB = posB2 - posB1;

		float det = dirA.y * dirB.x - dirA.x * dirB.y;
		if (Math::Abs(det) < FLOAT_DIV_ZERO_EPSILON)
			return false;

		Vector2 d = posB1 - posA1;
		float tA = (d.y * dirB.x - d.x * dirB.y) / det;
		if (tA > 1 || tA < 0)
			return false;
		float tB = (d.y * dirA.x - d.x * dirA.y) / det;
		if (tB > 1 || tB < 0)
			return false;

		outPos = posA1 + tA * dirA;
		return true;
	}

	bool LineCircleTest(Vector2 const& rPos, Vector2 const& rDir, Vector2 const& cPos, float cRadius, float t[])
	{
		assert(rDir.isNormalized());

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

	bool BoxCircleTest(Vector2 const& boxCenter, Vector2 const& boxHalfSize, Vector2 const& circlePos, float circleRadius)
	{
		Vector2 offset = (boxCenter - circlePos).abs();

		if (offset.x > boxHalfSize.x + circleRadius)
			return false;
		if (offset.y > boxHalfSize.y + circleRadius)
			return false;
		
		if (offset.x > boxHalfSize.x && offset.y > boxHalfSize.y)
		{
			if ((offset - boxHalfSize).length2() > Math::Square(circleRadius))
				return false;
		}

		return true;
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

	bool SegmentInterection(Vector2 const& a1, Vector2 const& a2, Vector2 const& b1, Vector2 const& b2, float& fracA)
	{
		float x1 = a1.x, x2 = a2.x, x3 = b1.x, x4 = b2.x;
		float y1 = a1.y, y2 = a2.y, y3 = b1.y, y4 = b2.y;

		float denom = x1 * (y4 - y3) + x2 * (y3 - y4) + x4 * (y2 - y1) + x3 * (y1 - y2);

		//Segments are parallel
		if (Math::Abs(denom) < FLOAT_DIV_ZERO_EPSILON)
			return false;

		//Compute numerators
		float numer1 = x1 * (y4 - y3) + x3 * (y1 - y4) + x4 * (y3 - y1);
		float numer2 = -(x1 * (y3 - y2) + x2 * (y1 - y3) + x3 * (y2 - y1));

		double s = numer1 / denom;
		double t = numer2 / denom;

		if (s < -FLOAT_EPSILON || s > 1 + FLOAT_EPSILON ||
			t < -FLOAT_EPSILON || t > 1 + FLOAT_EPSILON)
			return  false;

		fracA = s;
		return true;
	}

	Vector2 GetCircumcirclePoint(Vector2 const& a, Vector2 const& b, Vector2 const& c)
	{
		Vector2 ab = a - b;
		Vector2 bc = b - c;
		Vector2 ca = c - a;
		Vector2 d = 0.5 * (a.length2()*bc + b.length2()*ca + c.length2()*ab);

		Vector2 result;
		result.x = d.y / (a.x*bc.y + b.x*ca.y + c.x*ab.y);
		result.y = d.x / (a.y*bc.x + b.y*ca.x + c.y*ab.x);
		return result;
	}

	bool IsInsideCircumcircle(Vector2 const& v0, Vector2 const& v1, Vector2 const& v2, Vector2 const& p)
	{
		Vector2 a = v0 - p;
		Vector2 b = v1 - p;
		Vector2 c = v2 - p;
		float det = a.length2()*b.cross(c) + b.length2()*c.cross(a) + c.length2()*a.cross(b);
		return det > 0;
	}

}//namespace Math2D

