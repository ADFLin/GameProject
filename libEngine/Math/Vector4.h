#ifndef Vector4_h__
#define Vector4_h__

#include "Base.h"

namespace Math
{

	class Vector4
	{
	public:
		Vector4(){}
		Vector4( float x , float y , float z , float w )
			:x(x),y(y),z(z),w(w){}
		explicit Vector4( Vector3 const& v , float w  = 1.0 )
			:x(v.x),y(v.y),z(v.z),w(w){}

		void    setValue( float inX , float inY , float inZ , float inW ){  x = inX; y = inY ; z = inZ ; w = inW;  }

		float   dot( Vector4 const& rhs ) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }
		Vector4 mul( Vector4 const& rhs ) const { return Vector4( x * rhs.x , y * rhs.y , z * rhs.z , w * rhs.w ); }

		void    divideW(){ assert( w != 0 ); float wf = 1.0f / w ; x *= wf ; y *= wf; z *= wf ; w = 1; }
		Vector3 xyz() const { return Vector3(x,y,z); }
		Vector3 dividedVector() const { float wf = 1.0f / w ; return Vector3(x*wf,y*wf,z*wf); }

		Vector4& operator *= ( float v ){ x *= v ; y *= v; z *= v; w *= v; return *this;}

		operator float const*() const { return &x; }

		float x , y , z , w;
	};

}//namespace Math

#endif // Vector4_h__
