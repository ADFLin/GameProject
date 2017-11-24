#include "ProjectPCH.h"
#include "UtilityMath.h"

#include  <cfloat>
bool UM_CheckFloatState()
{
	unsigned val = _clear87();
	return  val & ( _EM_UNDERFLOW  |  _EM_OVERFLOW   | 
		            _EM_ZERODIVIDE |  _EM_INVALID    |
		            _EM_DENORMAL );
}

void UM_VecToYawPitch( Vec3D const& dir , float& yaw , float& pitch )
{
	float len = dir.length();
	Vec3D lookDir( dir.x , dir.y , 0 );
	float len2 = lookDir.length();
	
	if ( len2 < 1e-8 )
	{
		yaw = 0;
	}
	else
	{
		yaw = Math::ACos( lookDir.dot( Vec3D(1,0,0) ) / len2 );
		if ( lookDir.y < 0 )
		{
			yaw = -yaw;
		}
	}

	pitch = - Math::ASin( dir.z / len );
}

Quat UM_Slerp( Quat const& from , Quat const& to , float t )
{
	return Math::Slerp( from , to , t );
}


Quat UM_RoationQuat(Vec3D const& from, Vec3D const& to )
{
	float theta = Math::ACos( from.dot(to) );
	if ( Math::Abs( theta ) < 1e-8 )
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
	return Quat::Rotate( axis , theta );
}

Quat UM_FrontUpToQuat(CFly::AxisEnum frontAxis , Vec3D const& front , CFly::AxisEnum upAxis , Vec3D const& up /*= Vec3D(0,0,1)*/)
{
	Vec3D upAxisDir = CFly::getAxisDirection( upAxis );
	Vec3D fixFront = front - front.dot( upAxisDir ) * upAxisDir;

	float length = fixFront.normalize();
	float yaw = 0;
	if ( length > 1e-5 )
	{
		Vec3D frontAxisDir = CFly::getAxisDirection( frontAxis );
		//yaw = Math::ASin( upAxisDir.dot( frontAxisDir.cross( fixFront ) ) );

		yaw = Math::ACos( fixFront.dot( frontAxisDir ) );
		if ( fixFront.cross( frontAxisDir ).dot( upAxisDir ) < 0 )
			yaw = -yaw;
	}
	float pitch = Math::ACos( up.dot( upAxisDir ) );
	Quat quat;
	quat.setEulerZYX( yaw , pitch , 0 );
	return quat;
}

float UM_PlaneAngle( Vec3D const& faceDir , Vec3D const& pos , Vec3D const& destPos )
{
	Vec3D dir = destPos -  pos;
	dir[2] = 0;
	float result = UM_Angle( dir , faceDir );

	if ( dir.cross( faceDir ).z < 0 )
		result = - result;

	return result;
}