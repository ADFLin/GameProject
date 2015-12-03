#include "MathUtility.h"

#include  <cfloat>
bool checkFloatState()
{
	unsigned val = _clear87();
	return  val & ( _EM_UNDERFLOW  |  _EM_OVERFLOW   | 
		            _EM_ZERODIVIDE |  _EM_INVALID    |
		            _EM_DENORMAL );
}

void Vec3DToYawPitch( Vec3D const& dir , float& yaw , float& pitch )
{
	Float len = dir.length();
	Vec3D lookDir( dir.x() , dir.y() , 0 );
	Float len2 = lookDir.length();
	
	if ( len2 < 1e-8 )
	{
		yaw = 0;
	}
	else
	{
		yaw = acos( lookDir.dot( Vec3D(1,0,0) ) / len2 );
		if ( lookDir.y() < 0 )
		{
			yaw = -yaw;
		}
	}

	pitch = - asin( dir.z() / len );
}

Quat Slerp( Quat const& from , Quat const& to , float t )
{
#define  DELTA  1e-2
	Quat to1;
	double omega , cosom , sinom , scale0 , scale1;

	cosom = from.dot( to );

	if ( cosom < 0 )
	{
		cosom = -cosom;
		to1[0] = -to.x();
		to1[1] = -to.y();
		to1[2] = -to.z();
		to1[3] = -to.w();
	}
	else
	{
		to1 = to;
	}

	if ( 1 - cosom > DELTA )
	{
		omega = acos( cosom );
		sinom = sin( omega );

		scale0 = sin( (1.0 - t) * omega ) / sinom;
		scale1 = sin( t * omega ) / sinom;
	}
	else
	{
		scale0 = 1.0 -t;
		scale1 = t;
	}


	return Quat( scale0 * from.x() + scale1 * to1[0] ,
		         scale0 * from.y() + scale1 * to1[1] ,
				 scale0 * from.z() + scale1 * to1[2] ,
				 scale0 * from.w() + scale1 * to1[3] );

}


Quat GetRoationQuat(Vec3D const& from, Vec3D const& to )
{
	Float theta = acos( from.dot(to) );
	if ( fabs( theta ) < 1e-8 )
		return Quat( 1 ,0, 0 ,0 );

	Vec3D axis = from.cross(to);
	if ( axis.length2() < 1e-16 )
	{
		axis = Vec3D( -from[1] , from[0] , from[2] );
		if ( axis == from )
		{
			axis = Vec3D( from[0] , from[2] , -from[1] );
		}
	}
	return Quat( axis , theta );
}

Quat FrontUpToQuat( Vec3D const& front , Vec3D const& up /*= Vec3D(0,0,1)*/ )
{
	Vec3D vFront( front.x() , front.y() , 0 );
	vFront.normalize();
	float yaw = acos( vFront.dot( Vec3D(1,0,0) ) );
	if ( front.y() < 0 )
		yaw = - yaw;
	float pitch = acos( up.dot( Vec3D(0,0,1) ) );

	Quat quat;
	quat.setEulerZYX( yaw , pitch , 0 );
	return quat;
}