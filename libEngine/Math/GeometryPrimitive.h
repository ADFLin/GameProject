#pragma once
#ifndef GeometryPrimitive_H_FABFC52A_8FFA_4419_B6C7_5A8B4C09CB4F
#define GeometryPrimitive_H_FABFC52A_8FFA_4419_B6C7_5A8B4C09CB4F

#include <type_traits>

namespace Math
{
	template< class VectorType >
	struct TBoundBox
	{
		VectorType min;
		VectorType max;


		bool isIntersect(TBoundBox const& rhs) const
		{
			return Math::BoxBoxTest(min, max, rhs.min, rhs.max);
		}

		bool isValid() const
		{
			return min <= max;
		}

		TBoundBox& operator += (TBoundBox const& rhs)
		{
			assert(rhs.isValid());
			min.min(rhs.min);
			max.max(rhs.max);
			return *this;
		}

		TBoundBox& operator += (VectorType const& pos)
		{
			min.min(pos);
			max.max(pos);
			return *this;
		}


		TBoundBox() {}
		TBoundBox(EForceInit)
		{
			min = VectorType::Zero();
			max = VectorType::Zero();
		}
	};

	FORCEINLINE float DistanceSqure(TBoundBox<Vector2> const& box, Vector2 const& pos)
	{
		Vector2 dMin = pos - box.min;
		Vector2 dMax = box.max - pos;

		float dx, dy;
		dx = Math::Abs(dMin.x) < Math::Abs(dMax.x) ? dMin.x : dMax.x;
		dy = Math::Abs(dMin.y) < Math::Abs(dMax.y) ? dMin.y : dMax.y;

		float result = 0;
		if( dx < 0 ) result += dx * dx;
		if( dy < 0 ) result += dy * dy;
		return result;
	}

	FORCEINLINE float DistanceSqure(TBoundBox<Vector3> const& box, Vector3 const& pos)
	{
		Vector3 dMin = pos - box.min;
		Vector3 dMax = box.max - pos;

		float dx, dy, dz;
		dx = Math::Abs(dMin.x) < Math::Abs(dMax.x) ? dMin.x : dMax.x;
		dy = Math::Abs(dMin.y) < Math::Abs(dMax.y) ? dMin.y : dMax.y;
		dz = Math::Abs(dMin.z) < Math::Abs(dMax.z) ? dMin.z : dMax.z;

		float result = 0;
		if( dx < 0 ) result += dx * dx;
		if( dy < 0 ) result += dy * dy;
		if( dz < 0 ) result += dz * dz;
		return result;
	}

	template< class VectorType >
	struct TRay
	{
		VectorType pos;
		VectorType dir;

		Vector2 getPosition(float dist) const { return pos + dist * dir; }
		int     testIntersect(TBoundBox<VectorType> const& bound, float outDists[2]) const
		{
			if( !Math::LineBoxTest(pos, dir, bound.min, bound.max, outDists) )
				return 0;

			if( outDists[0] < 0 )
			{
				if( outDists[1] < 0 )
					return 0;
				outDists[0] = outDists[1];
				return 1;
			}
			return 2;
		}

	};


}

#endif // GeometryPrimitive_H_FABFC52A_8FFA_4419_B6C7_5A8B4C09CB4F