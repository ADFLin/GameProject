#ifndef MathUtility_h__
#define MathUtility_h__

#include "common.h"

bool checkFloatState();

Quat GetRoationQuat(Vec3D const& from, Vec3D const& to );
Quat FrontUpToQuat( Vec3D const& front , Vec3D const& up = Vec3D(0,0,1) );

inline 
Vec3D ProjectVec3D( Vec3D const& org  , Vec3D& project )
{
	return ( project.dot( org ) /project.length2() ) * project; 
}

void Vec3DToYawPitch( Vec3D const& dir , float& yaw , float& pitch );
Quat Slerp( Quat const& from , Quat const& to , float t );
#endif // MathUtility_h__