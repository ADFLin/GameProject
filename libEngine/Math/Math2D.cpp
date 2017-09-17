#include "Math2D.h"

namespace Math2D
{
	Rotation Rotation::Make(Vector2D const& from, Vector2D const& to)
	{
		assert(from.isNormalize() && to.isNormalize());

		Rotation result;
		result.c = from.dot(to);
		result.s = from.cross(to);
		return result;
	}

}//namespace Math2D

