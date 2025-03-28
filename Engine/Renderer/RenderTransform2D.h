#pragma once
#ifndef RenderTransform2D_H_F3F42C19_2E67_4C07_A6AD_15FC522AD4F0
#define RenderTransform2D_H_F3F42C19_2E67_4C07_A6AD_15FC522AD4F0

#include "MarcoCommon.h"
#include "Math/Math2D.h"
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

		explicit RenderTransform2D(Math::Rotation2D const& rotation, Vector2 const& inOffset = Vector2::Zero())
			:M(rotation.toMatrix())
			,P(inOffset)
		{
		}

		explicit RenderTransform2D(float angle, Vector2 const& inOffset = Vector2::Zero())
			:M(Matrix2::Rotate(angle))
			, P(inOffset)
		{
		}

		explicit RenderTransform2D(Vector2 const& scale, Vector2 const& inOffset = Vector2::Zero())
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

		FORCEINLINE static RenderTransform2D Identity() { return { Matrix2::Identity() , Vector2::Zero() }; }
		FORCEINLINE static RenderTransform2D Scale(float scale)
		{
			return RenderTransform2D(Vector2(scale,scale));
		}
		FORCEINLINE static RenderTransform2D Scale(Vector2 const& scale)
		{
			return RenderTransform2D(scale);
		}
		FORCEINLINE static RenderTransform2D TranslateThenScale(Vector2 const& offset, Vector2 const& scale)
		{
			return RenderTransform2D(scale, scale * offset);
		}
		FORCEINLINE static RenderTransform2D TranslateThenRotate(Vector2 const& offset, float angle)
		{
			RenderTransform2D result;
			result.M = Matrix2::Rotate(angle);
			result.P = offset * result.M;
			return result;
		}
		FORCEINLINE static RenderTransform2D Transform(Vector2 const& pos, Vector2 const& dir)
		{
			RenderTransform2D result;
			result.P = pos;
			float c = dir.x;
			float s = dir.y;
			result.M = Matrix2(c, s, -s, c);
			return result;
		}

		FORCEINLINE static RenderTransform2D Translate(Vector2 const& pos)
		{
			RenderTransform2D result;
			result.M = Matrix2::Identity();
			result.P = pos;
			return  result;
		}

		FORCEINLINE static RenderTransform2D Translate(float x, float y)
		{
			RenderTransform2D result;
			result.M = Matrix2::Identity();
			result.P = Vector2(x, y);
			return  result;
		}

		FORCEINLINE static RenderTransform2D LookAt(Vector2 screenSize, Vector2 const& lookPos, Math::Rotation2D const& rotation, float zoom, bool bFlipX = false)
		{
			return TranslateThenScale(-lookPos, bFlipX ? Vector2(-zoom, zoom) : Vector2(zoom, zoom)) * RenderTransform2D(rotation, 0.5 * screenSize);
		}

		FORCEINLINE static RenderTransform2D LookAt(Vector2 screenSize, Vector2 const& lookPos, float theta, float zoom, bool bFlipX = false)
		{
			return LookAt(screenSize, lookPos, Math::Rotation2D::Angle(theta), zoom, bFlipX);
		}

		FORCEINLINE static RenderTransform2D LookAt(Vector2 screenSize, Vector2 const& lookPos, Vector2 const& upDir, float zoom, bool bFlipX = false)
		{
			Math::Rotation2D rotation;
			rotation.setYDir(upDir);
			return LookAt(screenSize, lookPos, rotation, zoom, bFlipX);
		}

		FORCEINLINE static RenderTransform2D Sprite(Vector2 const& pos, Vector2 const& pivotOffset, float rotation)
		{
			RenderTransform2D result;
			result.M = Matrix2::Rotate(rotation);
			result.P = MulOffset( -pivotOffset , result.M , pos + pivotOffset );
			return result;
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
			CHECK(!hadSacled());
			return M.mul(v);
		}

		FORCEINLINE Vector2 transformInvPositionAssumeNoScale(Vector2 const& pos) const
		{
			CHECK(!hadSacled());
			return M.mul(pos - P);
		}

		RenderTransform2D inverse() const;

		bool isUniformScale() const
		{
			Vector2 scale = getScale();
			return Math::Abs(1 - (scale.x / scale.y)) < 1.0e-5;
		}

		bool hadSacled() const
		{
			Vector2 scale = getScale();
			return Math::Abs(scale.x - 1) > 1e-5 || Math::Abs(scale.x - 1) > 1e-5;
		}

		Vector2 getScale() const
		{
			Vector2 xAxis = Vector2(M[0], M[1]);
			Vector2 yAxis = Vector2(M[2], M[3]);
			return Vector2(xAxis.length(), yAxis.length());
		}

		bool hadScaledOrRotation() const
		{
			return Math::Abs(M[0] - 1) > 1e-5 || Math::Abs(M[3] - 1) > 1e-5;
		}

		Vector2 removeScale();
		void    removeRotation();
		void    removeScaleAndRotation();

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

		static Vector2 MulOffset(Vector2 const& v, Matrix2 const& m, Vector2 const& offset);
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
			CHECK(!mStack.empty());
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