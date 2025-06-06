#pragma once
#ifndef GeometryPrimitive_H_FABFC52A_8FFA_4419_B6C7_5A8B4C09CB4F
#define GeometryPrimitive_H_FABFC52A_8FFA_4419_B6C7_5A8B4C09CB4F

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "PrimitiveTest.h"
#include <type_traits>
#include "Template/ArrayView.h"


namespace Math
{
	template< class VectorType >
	struct TAABBox
	{
		using ScalarType = typename VectorType::ScalarType;


		VectorType min;
		VectorType max;

		void invalidate()
		{
			min = VectorType::Fill(std::numeric_limits<ScalarType>::max());
			max = VectorType::Fill(std::numeric_limits<ScalarType>::lowest());
		}

		bool isIntersect(TAABBox const& rhs) const
		{
			return Math::BoxBoxTest(min, max, rhs.min, rhs.max);
		}

		bool isValid() const { return min <= max; }
		bool isEmpty() const { return max <= min; }
		bool isPoint() const { return min == max; }

		bool isInside(VectorType const& p) const
		{
			return min <= p && p <= max;
		}
		ScalarType getArea() const
		{
			VectorType size = getSize();
			return size.x * size.y;
		}

		VectorType getSize() const { return max - min; }
		VectorType getCenter() const { return 0.5 * (max + min); }

		void setSize(VectorType const& size)
		{
			Vector2 center = getCenter();
			setCenterAndSize(getCenter(), size);
		}

		void setCenterAndSize(VectorType const& center, VectorType const& size)
		{
			min = center - 0.5 * size;
			max = center + 0.5 * size;
		}

		void translate(VectorType const& offset)
		{
			min += offset;
			max += offset;
		}

		void expand(VectorType const& dv)
		{
			min -= dv;
			max += dv;
		}

		void addPoint(VectorType const& v)
		{
			min.setMin(v);
			max.setMax(v);
		}

		TAABBox& operator += (TAABBox const& rhs)
		{
			assert(rhs.isValid());
			min.setMin(rhs.min);
			max.setMax(rhs.max);
			return *this;
		}

		TAABBox& operator += (VectorType const& v)
		{
			addPoint(v);
			return *this;
		}

		void setZero()
		{
			min = VectorType::Zero();
			max = VectorType::Zero();
		}

		TAABBox intersection(TAABBox const& rhs) const
		{
			TAABBox result;
			result.min = min.max(rhs.min);
			result.max = max.min(rhs.max);
			return result;
		}

		TAABBox() {}
		explicit TAABBox(EForceInit)
		{
			setZero();
		}
		TAABBox(VectorType const& inMin, VectorType const& inMax)
			:min(inMin), max(inMax)
		{
		}
	};

	FORCEINLINE float DistanceSqure(TAABBox<Vector2> const& box, Vector2 const& pos)
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

	FORCEINLINE float DistanceSqure(TAABBox<Vector3> const& box, Vector3 const& pos)
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

		VectorType getPosition(float dist) const { return pos + dist * dir; }
		int     testIntersect(TAABBox<VectorType> const& bound, float outDists[2]) const
		{
			if (!Math::LineAABBTest(pos, dir, bound.min, bound.max, outDists))
				return 0;

			if (outDists[0] < 0)
			{
				if (outDists[1] < 0)
					return 0;
				outDists[0] = outDists[1];
				return 1;
			}
			return 2;
		}

	};

	namespace EPlaneSide
	{
		enum Type
		{
			Front = 1,
			In = 0,
			Back = -1,
		};
	}

	// N.dot( V ) + d = 0
	//#MOVE
	template< class VectorType >
	class TPlane
	{
	public:
		TPlane() = default;
		TPlane(TPlane const& rhs) = default;
		TPlane& operator = (TPlane const& rhs) = default;

		TPlane(VectorType const& n, float d)
		{
			mNormal = n;
			mNormal.normalize();
			mArgD = d;
		}

		TPlane(VectorType const& normal, VectorType const& pos)
		{
			mNormal = normal;
			mNormal.normalize();
			mArgD = -mNormal.dot(pos);
		}

		VectorType const& getNormal() const { return mNormal; }
		float getArgD() const { return mArgD; }

		VectorType getAnyPoint() const
		{
			return (-mArgD) * mNormal;
		}

		float calcDistance(VectorType const& p) const
		{
			return mNormal.dot(p) + mArgD;
		}

		EPlaneSide::Type testSide(VectorType const& p, float thinkness, float& dist) const
		{
			dist = calcDistance(p);
			if (dist > thinkness)
				return EPlaneSide::Front;
			else if (dist < -thinkness)
				return EPlaneSide::Back;
			return EPlaneSide::In;
		}

		EPlaneSide::Type testSide(VectorType const& p, float thinkness) const
		{
			float dist;
			return testSide(p, thinkness, dist);
		}

		VectorType getIntersection(VectorType const& p1, VectorType const& p2) const
		{
			VectorType dir = p2 - p1;
			float dist = calcDistance(p1);
			float t = -dist / mNormal.dot(dir);
			return p1 + t * dir;
		}

		void inverse() { mNormal = -mNormal; }
	protected:
		VectorType mNormal;
		float      mArgD;
	};

	class Plane : public TPlane<Vector3>
	{
	public:
		Plane() = default;
		using TPlane<Vector3>::TPlane;

		Plane(Vector3 const& v0, Vector3 const& v1, Vector3 const& v2)
		{
			Vector3 d1 = v1 - v0;
			Vector3 d2 = v2 - v0;
			mNormal = d1.cross(d2);
			mNormal.normalize();

			mArgD = -mNormal.dot(v0);
		}

		static Plane FromVector4(Vector4 const& v)
		{
			Vector3 n = v.xyz();
			float mag = n.normalize();
			return Plane(n, v.w / mag);
		}


		operator Vector4() const { return Vector4(mNormal, mArgD); }
#if 0
		void swap(Plane& p)
		{
			using std::swap;

			swap(mNormal, p.mNormal);
			swap(mArgD, p.mArgD);
		}
#endif
	};

	class Plane2D : public TPlane<Vector2>
	{
	public:
		Plane2D() = default;
		using TPlane<Vector2>::TPlane;

		static Plane2D FromPosition(Vector2 const& posA, Vector2 const& posB)
		{
			Vector2 offset = posB - posA;
			return Plane2D(Vector2(offset.y, -offset.x), posA);
		}

		Vector2 getDirection() const
		{
			return Vector2(-mNormal.y, mNormal.x);
		}
	};

	class BoundSphere
	{
	public:
		BoundSphere(Vector3 const& c = Vector3(0, 0, 0), float r = 0)
			:center(c), radius(r)
		{
			assert(radius >= 0);
		}
		BoundSphere(Vector3 const& max, Vector3 const& min)
			:center(0.5f * (max + min))
		{
			Vector3 extrent = max - min;
			radius = 0.5f * Math::Max(extrent.x, Math::Max(extrent.y, extrent.z));
			assert(radius >= 0);
		}

		void merge(BoundSphere const& sphere)
		{
			Vector3 dir = sphere.center - center;
			float   dist = dir.length();

			if( radius > dist + sphere.radius )
				return;
			if( sphere.radius > dist + radius )
			{
				radius = sphere.radius;
				center = sphere.center;
				return;
			}

			float r = 0.5f * (radius + sphere.radius + dist);

			center += ((r - radius) / dist) * dir;
			radius = r;
		}

		void addPoint(Vector3 const& p)
		{
			Vector3 dir = p - center;
			float   dist2 = dir.length2();
			if( dist2 < radius * radius )
				return;

			radius = Math::Sqrt(dist2);
			//radius = 0.5f * ( radius + dist );
			//center += ( ( radius - dist ) / dist ) * dir;
		}
		bool testIntersect(Plane const& plane) const
		{
			float dist = plane.calcDistance(center);
			return Math::Abs(dist) < dist;
		}
		Vector3 const& getCenter() const { return center; }
		float          getRadius() const { return radius; }

		Vector3 center;
		float   radius;
	private:


	};


	FORCEINLINE Vector2 CalcPolygonCentroid(TArrayView<Vector2 const> posList)
	{
		int indexPrev = posList.size() - 1;
		float totalWeight = 0;
		Vector2 pos = Vector2::Zero();
		for (int index = 0; index < posList.size(); ++index)
		{
			Vector2 const& p1 = posList[indexPrev];
			Vector2 const& p2 = posList[index];
			float area = p1.cross(p2);
			pos += (p1 + p2) * area;
			totalWeight += area;
			indexPrev = index;
		}

		return pos / (3 * totalWeight);
	}

}

#endif // GeometryPrimitive_H_FABFC52A_8FFA_4419_B6C7_5A8B4C09CB4F