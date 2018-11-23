#ifndef Vector2_h__
#define Vector2_h__

#include "Math/Base.h"
#include "TVector2.h"

namespace Math
{
	class Vector2 : public TVector2< float >
	{
	public:
		Vector2() {}
		template< class T >
		Vector2(TVector2< T > const& rhs) :TVector2<float>(rhs) {}
		Vector2(float x, float y) :TVector2<float>(x, y) {}
		float normalize()
		{
			float len = Math::Sqrt(length2());
			if( len < FLT_DIV_ZERO_EPSILON )
				return 0.0;
			*this *= (1 / len);
			return len;
		}
		float length() const
		{
			return Math::Sqrt(length2());
		}
		bool isNormalized() const
		{
			return Math::Abs(1 - length2()) < 1e-5;
		}


		static inline Vector2 Cross(float value, Vector2 const& v)
		{
			return Vector2(-value * v.y, value * v.x);
		}
	};

	inline Vector2 GetNormal(Vector2 const& v)
	{
		Vector2 result = v;
		result.normalize();
		return result;
	}

	inline float Distance(Vector2 const& a, Vector2 const& b)
	{
		return Vector2(a - b).length();
	}
	// ( a x b ) x c = (a.c)b - (b.c) a
	inline Vector2 TripleProduct(Vector2 const& a, Vector2 const& b, Vector2 const& c)
	{
		return a.dot(c) * b - b.dot(c) * a;
	}


}

#endif // Vector2_h__


