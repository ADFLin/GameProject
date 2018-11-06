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

#include "Core/Color.h"

#include <cassert>

namespace Render
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

	inline bool isNormalize(Vector3 const& v)
	{
		return Math::Abs(1.0 - v.length2()) < 1e-6;
	}

	class LookAtMatrix : public Matrix4
	{
	public:
		LookAtMatrix(Vector3 const& eyePos, Vector3 const& lookDir, Vector3 const& upDir)
		{
			Vector3 zAxis = -Math::GetNormal(lookDir);
			Vector3 xAxis = Math::GetNormal(upDir.cross(zAxis));
			Vector3 yAxis = zAxis.cross(xAxis);
			setValue(
				xAxis.x, yAxis.x, zAxis.x,
				xAxis.y, yAxis.y, zAxis.y,
				xAxis.z, yAxis.z, zAxis.z,
				-eyePos.dot(xAxis), -eyePos.dot(yAxis), -eyePos.dot(zAxis));
		}

		LookAtMatrix( Vector3 const& lookDir, Vector3 const& upDir)
		{
			Vector3 zAxis = -Math::GetNormal(lookDir);
			Vector3 xAxis = Math::GetNormal(upDir.cross(zAxis));
			Vector3 yAxis = zAxis.cross(xAxis);
			setValue(
				xAxis.x, yAxis.x, zAxis.x,
				xAxis.y, yAxis.y, zAxis.y,
				xAxis.z, yAxis.z, zAxis.z,
				0, 0, 0);
		}
	};

	class PerspectiveMatrix : public Matrix4
	{
	public:
		PerspectiveMatrix(float yFov, float aspect, float zNear, float zFar)
		{
			float f = 1.0 / Math::Tan(yFov / 2);
			float dz = zNear - zFar;
			setValue(
				f / aspect, 0, 0, 0,
				0, f, 0, 0,
				0, 0, (zFar + zNear) / dz, -1,
				0, 0, 2 * zFar * zNear / dz, 0);
		}

		PerspectiveMatrix(float left, float right, float bottom, float top, float zNear, float zFar)
		{
			float dz = zNear - zFar;
			float xFactor = 1 / (right - left);
			float yFactor = 1 / (top - bottom);
			float n2 = 2 * zNear;
			setValue(
				n2 * xFactor, 0, 0, 0,
				0, n2 * yFactor, 0, 0,
				(right + left) * xFactor, (top + bottom) * yFactor, (zFar + zNear) / dz, -1,
				0, 0, 2 * zFar * zNear / dz, 0);
		}

		static float GetNearZ(Matrix4 const& m)
		{
			return  m[14] / (m[10] - 1);
		}
	};
	class OrthoMatrix : public Matrix4
	{
	public:
		OrthoMatrix(float left, float right, float bottom, float top, float zNear, float zFar)
		{
			float xFactor = 1 / (right - left);
			float yFactor = 1 / (top - bottom);
			float zFactor = 1 / (zFar - zNear);

			setValue(
				2 * xFactor, 0, 0, 0,
				0, 2 * yFactor, 0, 0,
				0, 0, -2 * zFactor, 0,
				-(left + right) * xFactor, -(top + bottom) * yFactor, -(zFar + zNear) * zFactor, 1);
		}

		OrthoMatrix(float width, float height, float zNear, float zFar)
		{
			float xFactor = 2 / (width);
			float yFactor = 2 / (height);
			float zFactor = 1 / (zFar - zNear);

			setValue(xFactor, 0, 0, 0,
					 0, yFactor, 0, 0,
					 0, 0, -2 * zFactor, 0,
					 0, 0, -(zFar + zNear) * zFactor, 1);
		}

	};

	class BasisMaterix : public Matrix4
	{
	public:
		static Matrix4 FromX(Vector3 const& axis)
		{
			Vector3 x0 = axis;
			Vector3 x1 = AnyOrthoVector(x0);
			Vector3 x2 = x0.cross(x1);
			return BasisMaterix(x0, x1, x2);
		}
		static Matrix4 FromY(Vector3 const& axis)
		{
			Vector3 x0 = axis;
			Vector3 x1 = AnyOrthoVector(x0);
			Vector3 x2 = x0.cross(x1);
			return BasisMaterix(x2, x0, x1);
		}
		static Matrix4 FromZ(Vector3 const& axis)
		{
			Vector3 x0 = axis;
			Vector3 x1 = AnyOrthoVector(x0);
			Vector3 x2 = x0.cross(x1);
			return BasisMaterix(x1, x2, x0);
		}
		static Matrix4 FromXY(Vector3 const& xAxis, Vector3 const& yAxis)
		{
			Vector3 x0 = xAxis;
			Vector3 x1 = yAxis;
			x1 -= Porjection(x1, x0);
			Vector3 x2 = x0.cross(x1);
			return BasisMaterix(x0, x1, x2);
		}

		static Matrix4 FromYZ(Vector3 const& yAxis, Vector3 const& zAxis)
		{
			Vector3 x0 = yAxis;
			Vector3 x1 = zAxis;
			x1 -= Porjection(x1, x0);
			Vector3 x2 = x0.cross(x1);
			return BasisMaterix(x2, x0, x1);
		}

		static Matrix4 FromZX(Vector3 const& zAxis, Vector3 const& xAxis)
		{
			Vector3 x0 = zAxis;
			Vector3 x1 = xAxis;
			x1 -= Porjection(x1, x0);
			Vector3 x2 = x0.cross(x1);
			return BasisMaterix(x1, x2, x0);
		}
	private:
		//#MOVE
		static Vector3 Porjection(Vector3 v, Vector3 dir)
		{
			return v - (dir.dot(v) / dir.length2()) * dir;
		}

		static Vector3 AnyOrthoVector(Vector3 v)
		{
			Vector3 temp = v.cross(Vector3(0, 0, 1));
			if( temp.length2() > 1e-5 )
				return temp;
			return v.cross(Vector3(1, 0, 0));
		}
		BasisMaterix(Vector3 xAxis, Vector3 yAxis, Vector3 zAxis)
		{
			float s;
			s = xAxis.normalize(); assert(s != 0);
			s = yAxis.normalize(); assert(s != 0);
			s = zAxis.normalize(); assert(s != 0);
			setValue(xAxis.x, xAxis.y, xAxis.z,
					 yAxis.x, yAxis.y, yAxis.z,
					 zAxis.x, zAxis.y, zAxis.z);
		}
	};


	class ReflectMatrix : public Matrix4
	{
	public:
		//plane: r * n + d = 0
		ReflectMatrix(Vector3 const& n, float d)
		{
			assert(isNormalize(n));
			// R = I - 2 n * nt
			// [  I 0 ] * [ R 0 ] * [ I 0 ] = [  R 0 ]
			// [ -D 1 ]   [ 0 1 ]   [ D 1 ]   [-2D 1 ]
			float xx = -2 * n.x * n.x;
			float yy = -2 * n.y * n.y;
			float zz = -2 * n.z * n.z;
			float xy = -2 * n.x * n.y;
			float xz = -2 * n.x * n.z;
			float yz = -2 * n.y * n.z;

			Vector3 off = -2 * d * n;

			setValue(
				1 + xx, xy, xz,
				xy, 1 + yy, yz,
				xz, yz, 1 + zz,
				off.x, off.y, off.z);
		}
	};


}//namespace Render

#endif // BaseType_H_555613C2_A1A0_4F14_8A87_AEB8AD797FF6
