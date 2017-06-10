#ifndef Base_h__C8B87C85_21EC_4C14_B747_F8B15EE26510
#define Base_h__C8B87C85_21EC_4C14_B747_F8B15EE26510

#include "Math/Base.h"
#include "TVector2.h"
#include "IntegerType.h"
#include <limits>
#include "Math/Math2D.h"

#define PHY2D_DEBUG 1

#ifndef PHY2D_DEBUG
#define PHY2D_DEBUG 0
#endif//PHY2D_DEBUG

namespace Phy2D
{
	typedef Math::Vector2D Vec2f;
	using namespace Math2D;

	class AABB
	{
	public:
		Vector2D min;
		Vector2D max;

		void  expend(Vec2f const& offset)
		{
			if( offset.x > 0 )
				max.x += offset.x;
			else
				min.x += offset.x;

			if( offset.y > 0 )
				max.y += offset.y;
			else
				min.y += offset.y;
		}
		bool isInterect(AABB const& other)
		{
			if( min.x > other.max.x || min.y > other.max.y )
				return false;
			if( max.x < other.min.x || max.y < other.min.y )
				return false;
			return true;
		}
	};

	typedef void(*DebugJumpFun)();
	extern DebugJumpFun GDebugJumpFun;

	class Shape;
	class CollideObject;
	class RigidBody;

}//namespace Phy2D

#endif // Base_h__C8B87C85_21EC_4C14_B747_F8B15EE26510