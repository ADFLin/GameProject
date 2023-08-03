#ifndef Quaternion_h__
#define Quaternion_h__

#include "Math/Base.h"
#include "Math/Vector3.h"

namespace Math
{

	class Matrix4;
	class Matrix3;

	class Quaternion
	{
	public:
		float x,y,z,w;

		Quaternion(){ setValue( 0,0,0,1); }
		Quaternion(EForceInit) { setValue(0, 0, 0, 1); }
		Quaternion(Matrix3 const& m) { setMatrix(m); }
		Quaternion( float sx , float sy, float sz , float sw ){ setValue( sx , sy , sz , sw ); }

		static Quaternion Identity() { return Quaternion(0, 0, 0, 1); }
		static Quaternion Value( Vector3 const& v , float w );
		static Quaternion Rotate( Vector3 const& axis , float angle );
		static Quaternion EulerZYX(Vector3 const& v);

		void    setValue( float sx,float sy,float sz , float sw ){	x=sx; y=sy; z=sz; w=sw;  }
		void    setRotation( Vector3 const& axis , float angle );
		// Yaw -> z   Pitch -> y  Roll -> x;
		void    setEulerZYX( float yaw, float pitch , float roll );
		Vector3 getEulerZYX() const;

		void    setMatrix( Matrix4 const& m );
		void    setMatrix( Matrix3 const& m );

		float   length() const;
		float   length2() const{ return dot( *this ); }

		float   dot( Quaternion const& q ) const;
		//v' = q v q*
		Vector3 rotate( Vector3 const& v ) const;
		Vector3 rotateInverse( Vector3 const& v ) const;

		Quaternion inverse() const;

		Quaternion conjugation() const   { return Quaternion( -x , -y , -z , w ); }
		float      normalize();

		Quaternion& operator *= ( Quaternion const& rhs );
		Quaternion& operator *= (float scale)
		{
			x *= scale; y *= scale; z *= scale; w *= scale;
			return *this;
		}

		operator       float* ()       { return &x; }
		operator const float* () const { return &x; }

	private:
		struct NoInit{ };
		Quaternion( NoInit const& ){}


	};

	Quaternion Slerp( Quaternion const& from , Quaternion const& to , float t );

	inline Quaternion operator * ( float s , Quaternion const& q )
	{
		return Quaternion( s * q.x , s * q.y , s * q.z , s * q.w );
	}
	inline Quaternion operator * ( Quaternion const& a, Quaternion const& b )
	{
		// a * b = ( va.cross( vb ) + wa * vb + wb * va , wa * wb - va.dot( vb ) )
		return Quaternion(
			a.y*b.z - a.z*b.y + a.x*b.w + a.w*b.x  ,
			a.z*b.x - a.x*b.z + a.y*b.w + a.w*b.y  ,
			a.x*b.y - a.y*b.x + a.z*b.w + a.w*b.z  ,
			a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z );
	}

	inline Quaternion operator + ( Quaternion const& a, Quaternion const& b )
	{
		return Quaternion( a.x+b.x , a.y+b.y , a.z+b.z , a.w+b.w );
	}

	inline Quaternion operator - (Quaternion const& a, Quaternion const& b)
	{
		return Quaternion(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
	}

	inline Quaternion& Quaternion::operator *= (Quaternion const& rhs)
	{
		*this = (*this) * rhs;
		return *this;
	}

	inline Quaternion Quaternion::Value( Vector3 const& v , float w )
	{
		Quaternion q = NoInit();
		q.x = v.x;
		q.y = v.y;
		q.z = v.z;
		q.w = w;
		return q;
	}
	inline Quaternion Quaternion::Rotate( Vector3 const& axis , float angle )
	{
		Quaternion q = NoInit();
		q.setRotation( axis , angle );
		return q;
	}

	inline Quaternion Quaternion::inverse() const
	{
		return ( 1.0f / length2() ) * conjugation();
	}

	inline float Quaternion::dot( Quaternion const& q ) const
	{
		return x * q.x + y * q.y + z * q.z + w * q.w;
	}

	inline float Quaternion::length() const
	{
		return Math::Sqrt( length2() );
	}

	inline float Quaternion::normalize()
	{
		float len = length();
		float s = 1.0f / len;
		w*=s; x*=s; y*=s; z*=s;
		return len;
	}

}//namespace Math

#endif // Quaternion_h__
