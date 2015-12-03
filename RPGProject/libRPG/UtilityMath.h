#ifndef UtilityMath_h__
#define UtilityMath_h__

#include "common.h"

bool   UM_CheckFloatState();
Quat   UM_RoationQuat( Vec3D const& from, Vec3D const& to );
Quat   UM_FrontUpToQuat( CFly::AxisEnum frontAxis , Vec3D const& front , CFly::AxisEnum upAxis , Vec3D const& up );
void   UM_VecToYawPitch( Vec3D const& dir , float& yaw , float& pitch );
Quat   UM_Slerp( Quat const& from , Quat const& to , float t );

inline 
float  UM_Angle( Vec3D const& v1 , Vec3D const& v2 )
{
	float len = sqrt( v1.length2() * v2.length2() );
	float val = TMin( v1.dot( v2 )/len , 1.0f );
	
	return acos( val );
}

inline 
Vec3D  UM_ProjectVec( Vec3D const& org  , Vec3D& project )
{
	return ( project.dot( org ) /project.length2() ) * project; 
}

float UM_PlaneAngle( Vec3D const& faceDir , Vec3D const& pos , Vec3D const& destPos );
#endif // UtilityMath_h__