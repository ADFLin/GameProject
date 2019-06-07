#include "Transform.h"

namespace Math
{
	Transform Transform::inverse() const
	{
		Transform result;
		result.scale = getSafeInvScale();
		result.rotation = rotation.inverse();
		result.location = result.rotation.rotate(result.scale * location);
		return result;
	}

	Transform Transform::getRelTransform(Transform const& rhs) const
	{
		// M(T) * M(Rel) = M(Rhs)
		return inverse() * rhs;	
	}

	Transform Transform::getLocalRelTransform(Transform const& rhs) const
	{
		// M(Rel)  * M(T)  = M(Rhs)
		return rhs * inverse();
	}

	Matrix4 Transform::toMatrix()
	{
		Matrix4 result;
		result.setTransform(location, rotation);
		result(0, 0) *= scale.x; result(0, 1) *= scale.x; result(0, 2) *= scale.x;
		result(1, 0) *= scale.y; result(1, 1) *= scale.y; result(1, 2) *= scale.y;
		result(2, 0) *= scale.z; result(2, 1) *= scale.z; result(2, 2) *= scale.z;
		return result;
	}

	Matrix4 Transform::toMatrixNoScale()
	{
		Matrix4 result;
		result.setTransform(location, rotation);
		return result;
	}

	Vector3 Transform::getSafeInvScale() const
	{
		float const Tolerance = 1e-8;
		Vector3 result;
		if( scale.x < Tolerance )
		{
			result.x = 0;
		}
		else
		{
			result.x = 1.0 / scale.x;
		}
		if( scale.y < Tolerance )
		{
			result.y = 0;
		}
		else
		{
			result.y = 1.0 / scale.y;
		}
		if( scale.z < Tolerance )
		{
			result.z = 0;
		}
		else
		{
			result.z = 1.0 / scale.z;
		}
		return result;
	}

}//namespace Math


