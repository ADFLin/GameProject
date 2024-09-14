#pragma once
#ifndef PrimitiveTest2D_H_713D2BCD_4AC3_4B4C_99C5_4BEBB85721AE
#define PrimitiveTest2D_H_713D2BCD_4AC3_4B4C_99C5_4BEBB85721AE

#include "Math/Vector2.h"
#include "Math/Vector3.h"

namespace Math
{
	template< class VectorType >
	struct GetDim {};
	template<>
	struct GetDim< Vector2 > { static int constexpr Result = 2; };
	template<>
	struct GetDim< Vector3 > { static int constexpr Result = 3; };

	bool LineAABBTest(Vector2 const& rayPos, Vector2 const& rayDir, Vector2 const& boxMin, Vector2 const& boxMax, float outDistances[]);
	bool LineAABBTest(Vector3 const& rayPos, Vector3 const& rayDir, Vector3 const& boxMin, Vector3 const& boxMax, float outDistances[]);
	bool BoxBoxTest(Vector2 const& aMin, Vector2 const& aMax, Vector2 const& bMin, Vector2 const& bMax);
	bool BoxBoxTest(Vector3 const& aMin, Vector3 const& aMax, Vector3 const& bMin, Vector3 const& bMax);

	bool RaySphereTest(Vector3 const& pos, Vector3 const& dirNormalized, Vector3 const& center, float radius, float& outT);
	bool LineSphereTest(Vector3 const& pos, Vector3 const& dirNormalized, Vector3 const& center, float radius, float outDistance[2]);

	bool LineLineTest(Vector2 const& posA, Vector2 const& dirA, Vector2 const& posB, Vector2 const& dirB, Vector2& outPos);
	bool SegmentSegmentTest(Vector2 const& posA1, Vector2 const& posA2, Vector2 const& posB1, Vector2 const& posB2, Vector2& outPos);
	bool LineCircleTest(Vector2 const& rPos, Vector2 const& rDir,
						Vector2 const& cPos, float cRadius, float t[]);

	bool BoxCircleTest(Vector2 const& boxCenter, Vector2 const& boxHalfSize, Vector2 const& circlePos, float circleRadius);

	Vector3 PointToTriangleClosestPoint(Vector3 const& p, Vector3 const& a, Vector3 const& b, Vector3 const& c, float& outSide);

	bool SegmentInterection(Vector2 const& a1, Vector2 const& a2, Vector2 const& b1, Vector2 const& b2, float& fracA);


	Vector2 GetCircumcirclePoint(Vector2 const& a, Vector2 const& b, Vector2 const& c);

	bool IsInsideCircumcircle(Vector2 const& v0, Vector2 const& v1, Vector2 const& v2, Vector2 const& p);;
}

#endif // PrimitiveTest2D_H_713D2BCD_4AC3_4B4C_99C5_4BEBB85721AE