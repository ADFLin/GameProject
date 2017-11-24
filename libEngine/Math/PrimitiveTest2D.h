#pragma once
#ifndef PrimitiveTest2D_H_713D2BCD_4AC3_4B4C_99C5_4BEBB85721AE
#define PrimitiveTest2D_H_713D2BCD_4AC3_4B4C_99C5_4BEBB85721AE

#include "Math/Math2D.h"

namespace Math2D
{
	bool LineBoxTest(Vector2 const& rayPos, Vector2 const& rayDir, Vector2 const& boxMin, Vector2 const& boxMax, float outDistances[]);

	bool BoxBoxTest(Vector2 const& aMin, Vector2 const& aMax, Vector2 const& bMin, Vector2 const& bMax);


	bool LineCircleTest(Vector2 const& rPos, Vector2 const& rDir,
						Vector2 const& cPos, float cRadius, float t[]);
}

#endif // PrimitiveTest2D_H_713D2BCD_4AC3_4B4C_99C5_4BEBB85721AE