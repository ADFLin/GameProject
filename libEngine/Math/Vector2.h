#ifndef Vector2_h__
#define Vector2_h__

#include "Math/Base.h"
#include "TVector2.h"

namespace Math
{
	class Vector2D : public TVector2< float >
	{
	public:
		Vector2D() {}
		template< class T >
		Vector2D(TVector2< T > const& rhs) :TVector2<float>(rhs) {}
		Vector2D(float x, float y) :TVector2<float>(x, y) {}
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
		bool isNormalize() const
		{
			return std::abs(1 - length2()) < 1e-5;
		}


		static inline Vector2D Cross(float value, Vector2D const& v)
		{
			return Vector2D(-value * v.y, value * v.x);
		}
	};

	inline Vector2D Normalize(Vector2D const& v)
	{
		Vector2D result = v;
		result.normalize();
		return result;
	}

	inline float Distance(Vector2D const& a, Vector2D const& b)
	{
		return Vector2D(a-b).length();
	}
	// ( a x b ) x c = (a.c)b - (b.c) a
	inline Vector2D TripleProduct(Vector2D const& a, Vector2D const& b, Vector2D const& c)
	{
		return a.dot(c) * b - b.dot(c) * a;
	}
}

#endif // Vector2_h__


