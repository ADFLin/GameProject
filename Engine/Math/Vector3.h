#ifndef Vector3_h__
#define Vector3_h__

#include "Math/Base.h"
#include "TVector3.h"

namespace Math
{
	class  Vector3
	{
	public:
		typedef float ScalarType;

		Vector3() = default;
		constexpr explicit Vector3( float const v[] )
			:x(v[0]),y(v[1]),z(v[2])
		{

		}
		constexpr Vector3(float sx, float sy, float sz)
			:x(sx), y(sy), z(sz)
		{

		}
		constexpr explicit Vector3(float value)
			:x(value),y(value),z(value)
		{

		}
		template< class T >
		constexpr explicit Vector3( TVector3<T> const& rhs)
			:x(rhs.x),y(rhs.y),z(rhs.z)
		{

		}

		Vector3 getNormal() const;

		float normalize()
		{  
			float length = length2();
			if( length < 1e-8 )
				return 0;
			length =  Math::Sqrt(length);
			this->operator /= (length );
			return length;
		}

		bool isNormalized() const
		{
			float lengthSqr = length2();
			return Math::Abs(lengthSqr - 1) < 1e-6;
		}

		bool isUniform() const
		{
			return x == y && x == z;
		}

		void setValue(float sx,float sy,float sz)
		{ x=sx; y=sy; z=sz;}

		float length2() const { return dot( *this ); }
		float length() const  { return Math::Sqrt( length2() ); }

		operator float*(){ return &x; }
		operator float const*() const { return &x; }

		Vector3& operator += ( Vector3 const& v );
		Vector3& operator -= ( Vector3 const& v );
		Vector3& operator *= ( float s );
		Vector3& operator /= ( float s );

		Vector3 mul( Vector3 const& v ) const
		{
			return Vector3( x * v.x , y * v.y , z * v.z );
		}
		Vector3 div(Vector3 const& v) const
		{
			return Vector3(x / v.x, y / v.y, z / v.z);
		}

		float   dot(Vector3 const& b) const;
		Vector3 cross(Vector3 const& b) const;

		void setMax( Vector3 const& rhs )
		{
			if ( x < rhs.x ) x = rhs.x;
			if ( y < rhs.y ) y = rhs.y;
			if ( z < rhs.z ) z = rhs.z;
		}

		void setMin( Vector3 const& rhs )
		{
			if ( x > rhs.x ) x = rhs.x;
			if ( y > rhs.y ) y = rhs.y;
			if ( z > rhs.z ) z = rhs.z;
		}
		Vector3 max(Vector3 const& v) const { return Vector3(Math::Max(x, v.x), Math::Max(y, v.y), Math::Max(z, v.z)); }
		Vector3 min(Vector3 const& v) const { return Vector3(Math::Min(x, v.x), Math::Min(y, v.y), Math::Min(z, v.z)); }

		Vector3 abs() const { return Vector3(Math::Abs(x), Math::Abs(y), Math::Abs(z)); }
		Vector3 projectNormal(Vector3 const& dir) const;

		static Vector3 Zero(){ return Vector3(0,0,0); }
	public:
		float x,y,z;

	private:
		void operator + ( int ) const;
		void operator - ( int ) const;
		void operator +=( int ) const;
		void operator -=( int ) const;
	};

	FORCEINLINE Vector3 operator + (Vector3 const& a, Vector3 const& b)
	{
		return Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
	}

	FORCEINLINE Vector3 operator-(Vector3 const& a, Vector3 const& b)
	{
		return Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
	}

	FORCEINLINE Vector3 operator*(Vector3 const& v1, Vector3 const& v2)
	{
		return v1.mul(v2);
	}

	FORCEINLINE Vector3 operator/(Vector3 const& v1, Vector3 const& v2)
	{
		return v1.div(v2);
	}

	FORCEINLINE Vector3 operator*(float a, Vector3 const& b)
	{
		return Vector3(a*b.x, a*b.y, a*b.z);
	}

	FORCEINLINE Vector3 operator*(Vector3 const& b, float a)
	{
		return Vector3(a*b.x, a*b.y, a*b.z);
	}

	FORCEINLINE Vector3 operator / (Vector3 const& b, float a)
	{
		return (1.0f / a) * b;
	}

	FORCEINLINE Vector3 operator-(Vector3 const& a)
	{
		return Vector3(-a.x, -a.y, -a.z);
	}

	FORCEINLINE  float  Dot( Vector3 const& a, Vector3 const& b )
	{ 
		return a.dot( b ); 
	}

	FORCEINLINE  Vector3 Cross( Vector3 const& a, Vector3 const& b )
	{
		return a.cross( b );
	}

	FORCEINLINE  float Vector3::dot( Vector3 const& b ) const
	{
		return x*b.x + y*b.y + z*b.z;
	}

	FORCEINLINE  Vector3 Vector3::cross( Vector3 const& b ) const
	{
		return Vector3( y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x );
	}

	FORCEINLINE Vector3& Vector3::operator += ( Vector3 const& v )
	{
		x += v.x; y += v.y; z += v.z;
		return *this;
	}

	FORCEINLINE Vector3& Vector3::operator -= ( Vector3 const& v )
	{
		x -= v.x; y -= v.y; z -= v.z;
		return *this;
	}

	FORCEINLINE Vector3& Vector3::operator *= ( float s )
	{
		x *= s; y *= s; z *= s;
		return *this;
	}

	FORCEINLINE Vector3& Vector3::operator /= (float s)
	{
		x /= s; y /= s; z /= s;
		return *this;
	}

	FORCEINLINE Vector3 Vector3::getNormal() const
	{
		return Math::InvSqrt(length2()) * (*this);
	}

	FORCEINLINE Vector3 Vector3::projectNormal(Vector3 const& dir) const
	{
		assert(dir.isNormalized());
		return dot(dir) * dir;
	}
	FORCEINLINE float DistanceSqure( Vector3 const& v1 , Vector3 const& v2 )
	{ 
		Vector3 diff = v1 - v2;
		return diff.length2();
	}

	FORCEINLINE float Distance( Vector3 const& v1 , Vector3 const& v2 )
	{ 
		Vector3 diff = v1 - v2;
		return diff.length();
	}

	FORCEINLINE  Vector3 Lerp( Vector3 const& a , Vector3 const& b , float t )
	{
		return ( 1.0f - t ) * a + t * b; 
	}

	FORCEINLINE  Vector3 GetNormal( Vector3 const& v )
	{
		float len = v.length();
		if ( len < 1e-5 )
			return Vector3::Zero();
		return ( 1 / len ) * v;
	}

	FORCEINLINE Vector3 Clamp(Vector3 const& v, Vector3 const& min, Vector3 const& max)
	{
		return Vector3(
			Clamp(v.x, min.x, max.x),
			Clamp(v.y, min.y, max.y),
			Clamp(v.z, min.z, max.z));
	}


}//namespace Math
#endif // Vector3_h__
