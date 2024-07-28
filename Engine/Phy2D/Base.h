#ifndef Base_h__C8B87C85_21EC_4C14_B747_F8B15EE26510
#define Base_h__C8B87C85_21EC_4C14_B747_F8B15EE26510

#include "Math/Base.h"
#include "Math/TVector2.h"
#include "Core/IntegerType.h"
#include <limits>
#include "Math/Math2D.h"
#include "Math/GeometryPrimitive.h"

#define PHY2D_DEBUG 1

#ifndef PHY2D_DEBUG
#define PHY2D_DEBUG 0
#endif//PHY2D_DEBUG

namespace Phy2D
{
	typedef Math::Vector2 Vector2;

	using namespace Math;

	using AABB = Math::TAABBox< Vector2 >;

	typedef void(*DebugJumpFunc)();
	extern DebugJumpFunc GDebugJumpFun;

	class Shape;
	class CollideObject;
	class RigidBody;

}//namespace Phy2D

#endif // Base_h__C8B87C85_21EC_4C14_B747_F8B15EE26510