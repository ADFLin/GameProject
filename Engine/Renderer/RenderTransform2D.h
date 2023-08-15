#pragma once
#ifndef RenderTransform2D_H_F3F42C19_2E67_4C07_A6AD_15FC522AD4F0
#define RenderTransform2D_H_F3F42C19_2E67_4C07_A6AD_15FC522AD4F0

#include "MarcoCommon.h"
#include "Math/Matrix2.h"
#include "Math/Vector2.h"
#include "Math/Matrix4.h"
#include "TransformPushScope.h"

#include "DataStructure/Array.h"

namespace Render
{
	using ::Math::Vector2;
	using ::Math::Matrix2;
	using ::Math::Matrix4;

	struct RenderTransform2D
	{
		Matrix2 M;
		Vector2 P;

		RenderTransform2D() = default;
		RenderTransform2D(Vector2 const& scale, float angle, Vector2 const& inOffset = Vector2::Zero())
			:M(Matrix2::ScaleThenRotate(scale, angle))
			, P(inOffset)
		{
		}

		RenderTransform2D(float angle, Vector2 const& inOffset = Vector2::Zero())
			:M(Matrix2::Rotate(angle))
			, P(inOffset)
		{
		}

		RenderTransform2D(Vector2 const& scale, Vector2 const& inOffset = Vector2::Zero())
			:M(Matrix2::Scale(scale))
			, P(inOffset)
		{
		}

		RenderTransform2D(Matrix2 const& inM, Vector2 const& inP)
			:M(inM), P(inP)
		{

		}

		void setIdentity()
		{
			M.setIdentity();
			P = Vector2::Zero();
		}

		void setPos(Vector2 const& pos) { P = pos; }

		static RenderTransform2D Identity() { return { Matrix2::Identity() , Vector2::Zero() }; }
		static RenderTransform2D Scale(Vector2 const& scale)
		{
			return RenderTransform2D(scale);
		}
		static RenderTransform2D TranslateThenScale(Vector2 const& offset, Vector2 const& scale)
		{
			return RenderTransform2D(scale, scale * offset);
		}

		static RenderTransform2D Transform(Vector2 const& pos, Vector2 const& dir)
		{
			RenderTransform2D result;
			result.P = pos;
			float c = dir.x;
			float s = dir.y;
			result.M = Matrix2(c, s, -s, c);
			return result;
		}

		static RenderTransform2D Translate(Vector2 const& pos)
		{
			RenderTransform2D result;
			result.M = Matrix2::Identity();
			result.P = pos;
			return  result;
		}

		static RenderTransform2D Translate(float x, float y)
		{
			RenderTransform2D result;
			result.M = Matrix2::Identity();
			result.P = Vector2(x, y);
			return  result;
		}

		FORCEINLINE RenderTransform2D operator * (RenderTransform2D const& rhs) const
		{
			//[ M  0 ] [ Mr 0 ]  =  [ M * Mr        0 ]
			//[ P  1 ] [ Pr 1 ]     [ P * Mr + Pr   1 ]
			RenderTransform2D result;
			result.M = M * rhs.M;
			result.P = MulOffset(P, rhs.M, rhs.P);
			return result;
		}

		FORCEINLINE Vector2 transformPosition(Vector2 const& pos) const
		{
			return MulOffset(pos, M, P);
		}

		FORCEINLINE Vector2 transformInvPosition(Vector2 const& pos) const
		{
			Matrix2 InvM;
			float det;
			M.inverse(InvM, det);
			return MulOffset(pos, InvM, -P * InvM);
		}

		FORCEINLINE Vector2 transformVector(Vector2 const& v) const
		{
			return M.leftMul(v);
		}

		FORCEINLINE Vector2 transformInvVector(Vector2 const& v) const
		{
			Matrix2 InvM;
			float det;
			M.inverse(InvM, det);
			return InvM.leftMul(v);
		}

		FORCEINLINE Vector2 transformInvVectorAssumeNoScale(Vector2 const& v) const
		{
			return M.mul(v);
		}

		FORCEINLINE Vector2 transformInvPositionAssumeNoScale(Vector2 const& pos) const
		{
			return M.mul(pos - P);
		}

		RenderTransform2D inverse() const
		{
			//[ M  0 ] [ Mr 0 ]  =  [ M * Mr        0 ] = [ 1  0 ]
			//[ P  1 ] [ Pr 1 ]     [ P * Mr + Pr   1 ] = [ 0  1 ]
			RenderTransform2D result;
			float det;
			M.inverse(result.M, det);
			result.P = -P * result.M;

			return result;
		}

		bool   isUniformScale() const
		{
			Vector2 scale = getScale();
			return Math::Abs(1 - (scale.x / scale.y)) < 1.0e-5;
		}

		Vector2 getScale() const
		{
			Vector2 xAxis = Vector2(M[0], M[1]);
			Vector2 yAxis = Vector2(M[2], M[3]);
			return Vector2(xAxis.length(), yAxis.length());
		}

		Vector2 removeScale()
		{
			Vector2 xAxis = Vector2(M[0], M[1]);
			float xScale = xAxis.normalize();
			Vector2 yAxis = Vector2(M[2], M[3]);
			float yScale = yAxis.normalize();

			M = Matrix2(xAxis, yAxis);
			return Vector2(xScale, yScale);
		}

		FORCEINLINE void translateWorld(Vector2 const& offset)
		{
			P += offset;
		}

		FORCEINLINE void translateLocal(Vector2 const& offset)
		{
			//[ 1  0 ] [ M 0 ]  =  [ M           0 ]
			//[ T  1 ] [ P 1 ]     [ T * M + P   1 ]
			P = MulOffset( offset , M , P );
		}

		FORCEINLINE void rotateLocal(float angle)
		{
			//[ R  0 ] [ M 0 ]  =  [ R * M  0 ]
			//[ 0  1 ] [ P 1 ]     [ P      1 ]
			Matrix2 R = Matrix2::Rotate(angle);
			M = R * M;
		}

		FORCEINLINE void rotateWorld(float angle)
		{
			//[ M  0 ] [ R 0 ]  =  [ M * R  0 ]
			//[ P  1 ] [ 0 1 ]     [ P * R  1 ]
			Matrix2 R = Matrix2::Rotate(angle);
			M = M * R;
			P = P * R;
		}

		FORCEINLINE void rotateLocal(Matrix2 const& R)
		{
			//[ R  0 ] [ M 0 ]  =  [ R * M  0 ]
			//[ 0  1 ] [ P 1 ]     [ P      1 ]
			M = R * M;
		}

		FORCEINLINE void rotateWorld(Matrix2 const& R)
		{
			//[ M  0 ] [ R 0 ]  =  [ M * R  0 ]
			//[ P  1 ] [ 0 1 ]     [ P * R  1 ]
			M = M * R;
			P = P * R;
		}

		FORCEINLINE void scaleWorld(Vector2 const& scale)
		{
			//[ M  0 ] [ S 0 ]  =  [ M * S  0 ]
			//[ P  1 ] [ 0 1 ]     [ P * S  1 ]
			M.rightScale(scale);
			P = P.mul(scale);
		}

		FORCEINLINE void scaleLocal(Vector2 const& scale)
		{
			//[ S  0 ] [ M 0 ]  =  [ S * M  0 ]
			//[ 0  1 ] [ P 1 ]     [ P      1 ]
			M.leftScale(scale);
		}

		FORCEINLINE Matrix4 toMatrix4() const
		{
			return Matrix4(
				M[0], M[1], 0, 0,
				M[2], M[3], 0, 0,
				   0,    0, 1, 0,
				 P.x,  P.y, 0, 1);
		}


		static Vector2 MulOffset(Vector2 const& v, Matrix2 const& m, Vector2 const& offset)
		{
#if USE_MATH_SIMD
			__m128 rv = _mm_setr_ps(v.x, v.x, v.y, v.y);
			__m128 mv = _mm_loadu_ps(m);
			__m128 xv = _mm_dp_ps(rv, mv, 0x51);
			__m128 yv = _mm_dp_ps(rv, mv, 0xa2);
			__m128 resultV = _mm_add_ps(_mm_add_ps(xv, yv), _mm_setr_ps(offset.x, offset.y, 0, 0));
			return Vector2(resultV.m128_f32[0], resultV.m128_f32[1]);
#else

			return v * m + offset;
#endif
		}
	};

	struct TransformStack2D
	{
		TransformStack2D()
		{
			mCurrent = RenderTransform2D::Identity();
		}
		void clear()
		{
			mStack.clear();
			mCurrent = RenderTransform2D::Identity();
		}

		FORCEINLINE void set(RenderTransform2D const& xform)
		{
			mCurrent = xform;
		}

		FORCEINLINE void setIdentity()
		{
			mCurrent = RenderTransform2D::Identity();
		}

		FORCEINLINE void transform(RenderTransform2D const& xform)
		{
			mCurrent = xform * mCurrent;
		}

		FORCEINLINE void translate(Vector2 const& offset)
		{
			mCurrent.translateLocal(offset);
		}

		FORCEINLINE void rotate(float angle)
		{
			mCurrent.rotateLocal(angle);
		}

		FORCEINLINE void scale(Vector2 const& scale)
		{
			mCurrent.scaleLocal(scale);
		}

		void pushTransform(RenderTransform2D const& xform, bool bApplyPrev = true)
		{
			mStack.push_back(mCurrent);
			if (bApplyPrev)
			{
				transform(xform);
			}
			else
			{
				set(xform);
			}
		}

		void push()
		{
			mStack.push_back(mCurrent);
		}

		void pop()
		{
			assert(!mStack.empty());
			mCurrent = mStack.back();
			mStack.pop_back();
		}

		RenderTransform2D const& get() const { return mCurrent; }
		RenderTransform2D&       get()       { return mCurrent; }
		RenderTransform2D mCurrent;
		TArray< RenderTransform2D > mStack;
	};

	template<>
	struct TTransformStackTraits< TransformStack2D >
	{
		using TransformType = RenderTransform2D;
	};

}//namespace Render

#endif // RenderTransform2D_H_F3F42C19_2E67_4C07_A6AD_15FC522AD4F0