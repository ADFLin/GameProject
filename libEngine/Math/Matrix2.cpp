#include "Matrix2.h"

namespace Math
{
	static Matrix2 const gIdentity(	1, 0, 0, 1);
	static Matrix2 const gZero(0, 0, 0, 0);

	Matrix2 const& Matrix2::Identity() { return gIdentity; }
	Matrix2 const& Matrix2::Zero() { return gZero; }
}