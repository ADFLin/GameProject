#pragma once
#ifndef DualQuat_H_D354AE63_25B1_4672_B7E9_482EB9B79905
#define DualQuat_H_D354AE63_25B1_4672_B7E9_482EB9B79905

#include "Math/Quaternion.h"

namespace Math

{
	template< class T >
	class TDualValue
	{
	public:
		T const& getRealPart() const { return mReal; }
		T const& getDualPart() const { return mDual; }

		TDualValue operator + (TDualValue<T> const& rhs) const
		{
			return TDualValue(mReal + rhs.mReal, mDual + rhs.mDual);
		}
		TDualValue operator - (TDualValue<T> const& rhs) const
		{
			return TDualValue(mReal + rhs.mReal, mDual + rhs.mDual);
		}
		TDualValue operator * (TDualValue<T> const& rhs) const
		{
			return TDualValue(mReal * rhs.mReal, mDual * rhs.mReal + mReal * rhs.mDual);
		}

	protected:
		T mReal;
		T mDual;
	};

	class DualQuat : public TDualValue< Quaternion >
	{

	public:
		static DualQuat FromTransform(Vector3 const& translation, Quaternion const& rotation)
		{
			DualQuat result;
			result.mReal = rotation;
			result.mDual = 0.5 * ( Quaternion::Value(translation, 0) * result.mReal );
			return result;
		}

		float normalize()
		{
			float mag = mReal.dot(mReal);
			if( mag != 0 )
			{
				float temp = 1.0 / mag;
				mReal *= temp;
				mDual *= temp;
			}
			return mag;
		}
	};
}

#endif // DualQuat_H_D354AE63_25B1_4672_B7E9_482EB9B79905
