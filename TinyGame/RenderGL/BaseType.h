#pragma once
#ifndef BaseType_H_555613C2_A1A0_4F14_8A87_AEB8AD797FF6
#define BaseType_H_555613C2_A1A0_4F14_8A87_AEB8AD797FF6

#include "Core/IntegerType.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4.h"
#include "Math/Matrix3.h"
#include "Math/Quaternion.h"
#include "Math/Transform.h"
#include "TVector2.h"
#include "TVector3.h"
#include "ReflectionCollect.h"

namespace RenderGL
{
	using IntVector2 = TVector2<int>;
	using IntVector3 = TVector3<int>;


	using ::Math::Vector2;
	using ::Math::Vector3;
	using ::Math::Vector4;
	using ::Math::Quaternion;
	using ::Math::Matrix4;
	using ::Math::Matrix3;
	using ::Math::Matrix3;

}//namespace RenderGL

#endif // BaseType_H_555613C2_A1A0_4F14_8A87_AEB8AD797FF6
