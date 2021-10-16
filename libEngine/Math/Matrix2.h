#ifndef Matrix2_H_DC434CB1_AA9F_4F30_8D03_F5EC8510BCFE
#define Matrix2_H_DC434CB1_AA9F_4F30_8D03_F5EC8510BCFE

#include "Math/Base.h"
#include "Math/Vector2.h"

#if USE_MATH_SIMD
#include "Math/SIMD.h"
#endif

namespace Math
{
	class Matrix2
	{
	public:
		Matrix2() = default;

		FORCEINLINE Matrix2(float const values[])
		{
			for (int i = 0; i < 4; ++i)
				mValues[i] = values[i];
		}
		FORCEINLINE Matrix2(float a0, float a1, float a2,float a3)
		{
			setValue(a0, a1, a2, a3);
		}

		FORCEINLINE Matrix2(Vector2 const& axisX, Vector2 const& axisY)
		{
			setBasis(axisX, axisY);
		}

		FORCEINLINE void setBasis(Vector2 const& axisX, Vector2 const& axisY)
		{
			mValues[0] = axisX.x; mValues[1] = axisX.y;
			mValues[2] = axisY.x; mValues[3] = axisY.y; 
		}

		FORCEINLINE void setValue(float a0, float a1, float a2, float a3)
		{
			mValues[0] = a0; mValues[1] = a1; mValues[2] = a2; mValues[3] = a3;
		}

		static Matrix2 const& Identity();
		static Matrix2 const& Zero();

		FORCEINLINE static Matrix2 Rotate(float angle)
		{
			Matrix2 m; m.setRotation(angle); return m;
		}
		FORCEINLINE static Matrix2 Scale(Vector2 const& scale)
		{
			Matrix2 m; m.setScale(scale); return m;
		}

		FORCEINLINE static Matrix2 ScaleThenRotate(Vector2 const& scale, float angle)
		{
			float c, s;
			Math::SinCos(angle, s, c);
			return Matrix2(scale.x * c, scale.x * s, -scale.y * s, scale.y * c);

		}

		FORCEINLINE static Matrix2 RotateThenScale(float angle, Vector2 const& scale)
		{
			float c, s;
			Math::SinCos(angle, s, c);
			return Matrix2(scale.x * c, scale.y * s, -scale.x * s, scale.y * c);
		}

		operator float* () { return mValues; }
		operator const float* () const { return mValues; }

		FORCEINLINE void setScale(Vector2 const& factor)
		{
			setValue(factor.x, 0,
				     0, factor.y);
		}

		FORCEINLINE void setRotation(float angle)
		{ 
			float s, c;
			Math::SinCos(angle, s, c);
			setValue(c, s, -s, c);
		}

		FORCEINLINE void setIdentity() { setValue(1, 0, 0 , 1); }

		FORCEINLINE Vector2 leftMul(Vector2 const& v) const
		{
#if USE_MATH_SIMD
			__m128 lv = _mm_setr_ps(v.x, v.x, v.y, v.y);
			__m128 mv = _mm_loadu_ps(mValues);
			__m128 xv = _mm_dp_ps(lv, mv, 0x51);
			__m128 yv = _mm_dp_ps(lv, mv, 0xa1);
			return Vector2(xv.m128_f32[0], yv.m128_f32[0]);

#else	
#define MAT_MUL( m , index )\
	( v.x * m[ index ] + v.y * m[ index + 2 ] )
			return Vector2(MAT_MUL(mValues, 0), MAT_MUL(mValues, 1));
#undef MAT_MUL
#endif
		}

		FORCEINLINE Vector2 mul(Vector2 const& v) const
		{
#if USE_MATH_SIMD
			__m128 rv = _mm_setr_ps(v.x, v.y, v.x, v.y);
			__m128 mv = _mm_loadu_ps(mValues);
			__m128 xv = _mm_dp_ps(mv, rv, 0x31);
			__m128 yv = _mm_dp_ps(mv, rv, 0xc1);
#else
#define MAT_MUL( m , index )\
	( v.x * m[ 2 * index ] + v.y * m[ 2 * index + 1 ] )
			return Vector2(MAT_MUL(mValues, 0), MAT_MUL(mValues, 1));
#undef MAT_MUL
#endif
		}

		FORCEINLINE friend Vector2 operator * (Vector2 const& lhs, Matrix2 const& rhs)
		{
			return rhs.leftMul(lhs);
		}

		FORCEINLINE friend Vector2 operator * (Matrix2 const& lhs, Vector2 const& rhs)
		{
			return lhs.mul(rhs);
		}

		FORCEINLINE float deter() const
		{
			return mValues[0] * mValues[3] - mValues[1] * mValues[2];
		}

		bool  inverse(Matrix2& m, float& det) const
		{
			det = deter();
			if (Math::Abs(det) < 1e-6)
				return false;

			float invDet = 1.0f / det;
			m.setValue( mValues[3] * invDet, -mValues[1] * invDet, -mValues[2] * invDet , mValues[0] * invDet );
			return true;
		}

		FORCEINLINE void leftScale(Vector2 const& s)
		{
#if USE_MATH_SIMD
			__m128 value = _mm_loadu_ps(mValues);
			__m128 scale = { s.x , s.x , s.y , s.y };
			__m128 result = _mm_mul_ps(value, scale);
			_mm_store_ps(mValues, result);
#else
			mValues[0] *= s.x;
			mValues[1] *= s.x;
			mValues[2] *= s.y;
			mValues[3] *= s.y;
#endif
		}

		FORCEINLINE void rightScale(Vector2 const& s)
		{
#if USE_MATH_SIMD
			__m128 value = _mm_loadu_ps(mValues);
			__m128 scale = { s.x , s.y , s.x , s.y };
			__m128 result = _mm_mul_ps(value, scale);
			_mm_store_ps(mValues, result);
#else
			mValues[0] *= s.x;
			mValues[1] *= s.y;
			mValues[2] *= s.x;
			mValues[3] *= s.y;
#endif
		}

		Matrix2  operator * (Matrix2 const& rhs) const;

		float& operator()(int idx) { return mValues[idx]; }
		float  operator()(int idx) const { return mValues[idx]; }

		float& operator()(int i, int j) { return mM[i][j]; }
		float  operator()(int i, int j) const { return mM[i][j]; }

	private:
		union
		{
			float mValues[4];
			float mM[2][2];
		};
	};

	FORCEINLINE Matrix2 Matrix2::operator * (Matrix2 const& rhs) const
	{
#if USE_MATH_SIMD
		__m128 lv = _mm_loadu_ps(mValues);
		__m128 rv = _mm_loadu_ps(rhs.mValues);
		__m128 r02 = _mm_shuffle_ps(rv, rv, _MM_SHUFFLE(2, 0, 2, 0));
		__m128 r13 = _mm_shuffle_ps(rv, rv, _MM_SHUFFLE(3, 1, 3, 1));
		return Matrix2(
			_mm_dp_ps(lv, r02, 0x31).m128_f32[0],
			_mm_dp_ps(lv, r13, 0x31).m128_f32[0],
			_mm_dp_ps(lv, r02, 0xc1).m128_f32[0],
			_mm_dp_ps(lv, r13, 0xc1).m128_f32[0]);
#else	
#define MAT_MUL( v1 , v2 , idx1 , idx2 )\
	( v1[2*idx1]*v2[idx2] + v1[2*idx1+1]*v2[idx2+2] )
		return Matrix2(
			MAT_MUL(mValues, rhs.mValues, 0, 0),
			MAT_MUL(mValues, rhs.mValues, 0, 1),

			MAT_MUL(mValues, rhs.mValues, 1, 0),
			MAT_MUL(mValues, rhs.mValues, 1, 1));
#undef MAT_MUL
#endif
	}

}

#endif // Matrix2_H_DC434CB1_AA9F_4F30_8D03_F5EC8510BCFE