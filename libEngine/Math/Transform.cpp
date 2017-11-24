#include "Transform.h"

namespace Math
{
	Transform Transform::inverse() const
	{
		Transform result;
		result.scale = Vector3(1.0 / scale.x, 1.0 / scale.y, 1.0 / scale.z);
		result.rotation = rotation.inverse();
		result.location = result.rotation.rotate(result.scale * location);
		return result;
	}


	Math::Matrix4 Transform::toMatrix()
	{
		Matrix4 result;
		result.setQuaternion(rotation);
		result(0, 0) *= scale.x; result(0, 1) *= scale.x; result(0, 2) *= scale.x;
		result(1, 0) *= scale.y; result(1, 1) *= scale.y; result(1, 2) *= scale.y;
		result(2, 0) *= scale.z; result(2, 1) *= scale.z; result(2, 2) *= scale.z;
		result.modifyTranslation(location);
		return result;
	}

	Math::Matrix4 Transform::toMatrixNoScale()
	{
		Matrix4 result;
		result.setQuaternion(rotation);
		result.modifyTranslation(location);
		return result;
	}

}//namespace Math


