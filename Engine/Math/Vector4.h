#ifndef Vector4_h__
#define Vector4_h__

#include "Base.h"
#include "Vector3.h"
#include "Vector2.h"

namespace Math
{

	class Vector4
	{
	public:
		using ScaleType = float;

		Vector4() = default;
		Vector4(Vector4 const& rhs) = default;

		Vector4( float x , float y , float z , float w )
			:x(x),y(y),z(z),w(w){}
		explicit Vector4( float const v[] )
			:x(v[0]),y(v[1]), z(v[2]), w(v[3]){ }
		explicit Vector4( Vector3 const& v , float w  = 1.0 )
			:x(v.x),y(v.y),z(v.z),w(w){}
		explicit Vector4(Vector2 const& u, Vector2 const& v)
			:x(u.x), y(u.y), z(v.x), w(v.y){}

		static Vector4 Zero() { return Vector4(0,0,0,0); }
		static Vector4 Fill(float s) { return Vector4(s, s, s, s); }

		void    setValue( float inX , float inY , float inZ , float inW ){  x = inX; y = inY ; z = inZ ; w = inW;  }


		float   dot( Vector4 const& rhs ) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }
		Vector4 mul( Vector4 const& rhs ) const { return Vector4( x * rhs.x , y * rhs.y , z * rhs.z , w * rhs.w ); }

		void    divideW(){ assert( w != 0 ); float wf = 1.0f / w ; x *= wf ; y *= wf; z *= wf ; w = 1; }
		Vector3 xyz() const { return Vector3(x,y,z); }
		Vector2 xy() const { return Vector2(x, y); }

		Vector3 dividedVector() const { float wf = 1.0f / w ; return Vector3(x*wf,y*wf,z*wf); }

		Vector4& operator *= ( float v ) { x *= v ; y *= v; z *= v; w *= v; return *this; }
		Vector4& operator /= ( float v ) { x /= v; y /= v; z /= v; w /= v; return *this; }

		bool operator == (Vector4 const& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
		bool operator != (Vector4 const& rhs) const { return !this->operator==(rhs); }

		Vector4 operator + (Vector4 const& rhs) const { return Vector4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
		Vector4 operator - (Vector4 const& rhs) const { return Vector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
		operator float*() { return &x; }
		operator float const*() const { return &x; }



		float x , y , z , w;
	};


	FORCEINLINE Vector4 operator-(Vector4 const& a)
	{
		return Vector4(-a.x, -a.y, -a.z, -a.w);
	}

	FORCEINLINE Vector4 operator * (float s, Vector4 const& v)
	{
		return Vector4(s*v.x, s*v.y, s*v.z , s*v.w);
	}

	FORCEINLINE Vector4 operator*(Vector4 const& v, float s)
	{
		return Vector4(s*v.x, s*v.y, s*v.z, s*v.w);
	}

	FORCEINLINE Vector4 operator/(Vector4 const& v, float s)
	{
		return Vector4(v.x / s, v.y / s, v.z / s , v.w / s);
	}

	FORCEINLINE  float  Dot(Vector4 const& a, Vector4 const& b)
	{
		return a.dot(b);
	}


}//namespace Math

#endif // Vector4_h__
