#include "Math/Base.h"
#include "Math/Vector2.h"

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

		operator float* () { return mValues; }
		operator const float* () const { return mValues; }

		void setScale(Vector2 const& factor)
		{
			setValue(factor.x, 0,
				     0, factor.y);
		}

		void setRotation(float angle)
		{ 
			float s, c;
			Math::SinCos(angle, s, c);
			setValue(c, s, -s, c);
		}

		void setIdentity() { setValue(1, 0, 0 , 1); }

		Vector2 leftMul(Vector2 const& v) const
		{
#define MAT_MUL( m , index )\
	( v.x * m[ index ] + v.y * m[ index + 2 ] )
			return Vector2(MAT_MUL(mValues, 0), MAT_MUL(mValues, 1));
#undef MAT_MUL
		}

		Vector2 rightMul(Vector2 const& v) const
		{
#define MAT_MUL( m , index )\
	( v.x * m[ 2 * index ] + v.y * m[ 2 * index + 1 ] )
			return Vector2(MAT_MUL(mValues, 0), MAT_MUL(mValues, 1));
#undef MAT_MUL
		}

		float deter() const
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
#define MAT_MUL( v1 , v2 , idx1 , idx2 )\
	( v1[2*idx1]*v2[idx2] + v1[2*idx1+1]*v2[idx2+2] )
		return Matrix2(
			MAT_MUL(mValues, rhs.mValues, 0, 0),
			MAT_MUL(mValues, rhs.mValues, 0, 1),

			MAT_MUL(mValues, rhs.mValues, 1, 0),
			MAT_MUL(mValues, rhs.mValues, 1, 1));
#undef MAT_MUL

	}

}