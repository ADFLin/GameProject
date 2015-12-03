#include "Quaternion.h"
#include "Math/MatrixUtility.h"

#include "Math/Matrix3.h"
#include "Math/Matrix4.h"

namespace Math
{
	void Quaternion::setMatrix( Matrix3 const& m )
	{
		MatrixUtility::toQuaternion( m , *this );
	}

	void Quaternion::setMatrix( Matrix4 const& m )
	{
		MatrixUtility::toQuaternion( m , *this );
	}

	void Quaternion::setEulerZYX(float yaw,float pitch,float roll)
	{
		float sy , cy;
		Math::SinCos( yaw*float(0.5f) , sy , cy );
		float sp , cp;
		Math::SinCos( pitch*float(0.5f) , sp , cp );
		float sr , cr;
		Math::SinCos( roll*float(0.5f) , sr , cr );

		float cpcy = cp*cy;
		float spsy = sp*sy;
		float spcy = sp*cy;
		float cpsy = cp*sy;

		setValue(
			sr*cpcy - cr*spsy ,
			cr*spcy + sr*cpsy ,
			cr*cpsy - sr*spcy , 
			cr*cpcy + sr*spsy );
	}

	Vector3 Quaternion::getEulerZYX()
	{
		Vector3 result;
		result.x = Math::ATan2(  w*x + y*z , 0.5 - ( x*x + y*y ) );
		result.y = Math::ASin( 2*( w*y - z*x ) );
		result.z = Math::ATan2(  w*z + x*y , 0.5 - ( y*y + z*z ) );
		return result;
	}


	Vector3 Quaternion::rotate( Vector3 const& v ) const
	{
#if 1
		// nVidia impl
		Vector3 qv(x, y, z);
		Vector3 uv = qv.cross(v);
		Vector3 uuv = qv.cross(uv);
		uv  *= (2.0f * w);
		uuv *= 2.0f;
		return v + uv + uuv;
#else
		Vector3 qv(x, y, z);
		return ( 2.0f * qv.dot(v) ) * qv + 
			( w*w - qv.dot(qv) ) * v + 
			( 2.0f * w ) * qv.cross( v );
#endif
	}

	Vector3 Quaternion::rotateInverse(Vector3 const& v) const
	{
		//TODO
		Quaternion q = this->inverse();
		return q.rotate( v );
	}

	void Quaternion::setRotation( Vector3 const& axis , float angle )
	{
		float d = 1.0f / axis.length();

		assert( d != float(0.0f) );
		float k = angle * 0.5f;
		float s = Math::Sin(k) * d;

		x = axis.x * s;
		y = axis.y * s;
		z = axis.z * s;
		w = Math::Cos(k);
	}

	Quaternion slerp( Quaternion const& from , Quaternion const& to , float t )
	{

#define  THRESHOLD  1e-3
		Quaternion to1;
		float fCos = from.dot( to );

		if ( fCos < 0 )
		{
			fCos = -fCos;
			to1[0] = -to.x;
			to1[1] = -to.y;
			to1[2] = -to.z;
			to1[3] = -to.w;
		}
		else
		{
			to1 = to;
		}

		if ( Math::Abs(fCos) < 1 - THRESHOLD )
		{

			float fSin = Math::Sqrt( 1 -  fCos * fCos );
			float fAngle = Math::ATan2( fSin, fCos );
			float fInvSin = 1.0f / fSin;

			float scale0 = Math::Sin((1.0f - t) * fAngle) * fInvSin;
			float scale1 = Math::Sin(t * fAngle) * fInvSin;

			return Quaternion( 
				scale0 * from.x + scale1 * to1[0] ,
				scale0 * from.y + scale1 * to1[1] ,
				scale0 * from.z + scale1 * to1[2] ,
				scale0 * from.w + scale1 * to1[3] );
		}
		else
		{
			float scale0 = 1.0 -t;
			float scale1 = t;
			Quaternion result( 
				scale0 * from.x + scale1 * to1[0] ,
				scale0 * from.y + scale1 * to1[1] ,
				scale0 * from.z + scale1 * to1[2] ,
				scale0 * from.w + scale1 * to1[3] );

			result.normalize();
			return result;
		}
	}




}//namespace Math