#pragma once
#ifndef RenderTransform2D_H_F3F42C19_2E67_4C07_A6AD_15FC522AD4F0
#define RenderTransform2D_H_F3F42C19_2E67_4C07_A6AD_15FC522AD4F0

#include "Math/Matrix2.h"
#include "Math/Vector2.h"
#include "Math/Matrix4.h"

#include <vector>

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

		static RenderTransform2D Identity() { return { Matrix2::Identity() , Vector2::Zero() }; }
		static RenderTransform2D TranslateThenScale(Vector2 const& offset, Vector2 const& scale)
		{
			return RenderTransform2D(scale, scale * offset);
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

		FORCEINLINE Vector2 transformVector(Vector2 const& v) const
		{
			return M.leftMul(v);
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

		FORCEINLINE void rotateWorld(float angle)
		{
			M = M * Matrix2::Rotate(angle);
		}

		FORCEINLINE void rotateLocal(float angle)
		{
			//[ R  0 ] [ M 0 ]  =  [ R * M  0 ]
			//[ 0  1 ] [ P 1 ]     [ P      1 ]
			M = Matrix2::Rotate(angle) * M;
		}

		FORCEINLINE void scaleWorld(Vector2 const& scale)
		{
			M.rightScale(scale);
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
				0, 0, 1, 0,
				P.x, P.y, 0, 1);
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
			mCurrent.rotateWorld(angle);
		}

		FORCEINLINE void scale(Vector2 const& scale)
		{
			mCurrent.scaleWorld(scale);
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
		std::vector< RenderTransform2D > mStack;
	};
}//namespace Render

#endif // RenderTransform2D_H_F3F42C19_2E67_4C07_A6AD_15FC522AD4F0