#include "Math2D.h"

namespace Math
{
	Rotation2D Rotation2D::Make(Vector2 const& from, Vector2 const& to)
	{
		assert(from.isNormalize() && to.isNormalize());

		Rotation2D result;
		result.c = from.dot(to);
		result.s = from.cross(to);
		return result;
	}

}//namespace Math

