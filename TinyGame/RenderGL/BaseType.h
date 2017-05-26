#pragma once
#ifndef BaseType_H_555613C2_A1A0_4F14_8A87_AEB8AD797FF6
#define BaseType_H_555613C2_A1A0_4F14_8A87_AEB8AD797FF6

#include "IntegerType.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4.h"
#include "Math/Matrix3.h"
#include "Math/Quaternion.h"
#include "Math/Transform.h"

#include "ReflectionCollect.h"

namespace RenderGL
{
	typedef ::Math::Vector3 Vector3;
	typedef ::Math::Vector4 Vector4;
	typedef ::Math::Quaternion Quaternion;
	typedef ::Math::Matrix4 Matrix4;
	typedef ::Math::Matrix3 Matrix3;
	typedef ::Math::Matrix3 Transform;

}//namespace RenderGL

#endif // BaseType_H_555613C2_A1A0_4F14_8A87_AEB8AD797FF6
