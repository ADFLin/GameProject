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
	bool LineCircleTest(Vector2 const& rPos, Vector2 const& rDir,
						Vector2 const& cPos, float cRadius, float t[]);

	Vector3 PointToTriangleClosestPoint(Vector3 const& p, Vector3 const& a, Vector3 const& b, Vector3 const& c, float& outSide);
}

#endif // PrimitiveTest2D_H_713D2BCD_4AC3_4B4C_99C5_4BEBB85721AE