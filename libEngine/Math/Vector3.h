#ifndef Vector3_h__
#define Vector3_h__

#include "Math/Base.h"

namespace Math
{
	class  Vector3
	{
	public:
		Vector3(){}
		Vector3( float const v[] ){ setValue( v[0] , v[1] , v[2] ); }
		Vector3(float sx,float sy,float sz);
		Vector3(float value):x(value),y(value),z(value){}

		float normalize()
		{  
			float length = length2();
			if( length < 1e-8 )
				return 0;
			length =  Math::Sqrt(length);
			this->operator /= (length );
			return length;
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

		void    max( Vector3 const& rhs )
		{
			if ( x < rhs.x ) x = rhs.x;
			if ( y < rhs.y ) y = rhs.y;
			if ( z < rhs.z ) z = rhs.z;
		}

		void    min( Vector3 const& rhs )
		{
			if ( x > rhs.x ) x = rhs.x;
			if ( y > rhs.y ) y = rhs.y;
			if ( z > rhs.z ) z = rhs.z;
		}

		static Vector3 Zero(){ return Vector3(0,0,0); }
	public:
		float x,y,z;

	private:
		void operator + ( int ) const;
		void operator - ( int ) const;
		void operator +=( int ) const;
		void operator -=( int ) const;
	};


	inline float  dot( Vector3 const& a, Vector3 const& b )
	{ 
		return a.dot( b ); 
	}

	inline Vector3 cross( Vector3 const& a, Vector3 const& b )
	{
		return a.cross( b );
	}


	inline Vector3::Vector3( float sx , float sy , float sz ) 
		:x(sx),y(sy),z(sz)
	{

	}

	inline float Vector3::dot( Vector3 const& b ) const
	{
		return x*b.x + y*b.y + z*b.z;
	}

	inline Vector3 Vector3::cross( Vector3 const& b ) const
	{
		return Vector3( y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x );
	}

	inline Vector3& Vector3::operator += ( Vector3 const& v )
	{
		x += v.x; y += v.y; z += v.z;
		return *this;
	}

	inline Vector3& Vector3::operator -= ( Vector3 const& v )
	{
		x -= v.x; y -= v.y; z -= v.z;
		return *this;
	}

	inline Vector3& Vector3::operator *= ( float s )
	{
		x *= s; y *= s; z *= s;
		return *this;
	}

	inline Vector3& Vector3::operator /= (float s)
	{
		x /= s; y /= s; z /= s;
		return *this;
	}

	inline Vector3 operator + ( Vector3 const& a , Vector3 const& b )
	{
		return Vector3(a.x+b.x,a.y+b.y,a.z+b.z);
	}

	inline Vector3 operator-(Vector3 const& a,Vector3 const& b)
	{
		return Vector3(a.x-b.x,a.y-b.y,a.z-b.z);
	}

	inline Vector3 operator*(Vector3 const& v1, Vector3 const& v2)
	{
		return v1.mul(v2);
	}

	inline Vector3 operator/(Vector3 const& v1, Vector3 const& v2)
	{
		return v1.div(v2);
	}

	inline Vector3 operator*( float a, Vector3 const& b )
	{
		return Vector3(a*b.x,a*b.y,a*b.z);
	}

	inline Vector3 operator*( Vector3 const& b , float a )
	{
		return Vector3(a*b.x,a*b.y,a*b.z);
	}

	inline Vector3 operator / (Vector3 const& b, float a)
	{
		return ( 1.0f / a ) * b;
	}

	inline Vector3 operator-( Vector3 const& a )
	{
		return Vector3(-a.x,-a.y,-a.z);
	}


	inline float distance2( Vector3 const& v1 , Vector3 const& v2 )
	{ 
		Vector3 diff = v1 - v2;
		return diff.length2();
	}

	inline float distance( Vector3 const& v1 , Vector3 const& v2 )
	{ 
		Vector3 diff = v1 - v2;
		return diff.length();
	}

	inline Vector3 lerp( Vector3 const& a , Vector3 const& b , float t )
	{
		return ( 1.0f - t ) * a + t * b; 
	}

	inline Vector3 Normalize( Vector3 const& v )
	{
		float len = v.length();
		if ( len < 1e-5 )
			return Vector3::Zero();
		return ( 1 / len ) * v;
	}


}//namespace Math
#endif // Vector3_h__
