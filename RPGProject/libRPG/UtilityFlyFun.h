#ifndef UtilityFlyFun_h__
#define UtilityFlyFun_h__

#include "common.h"


float* UF_SetXYZW_RGB( float* vertex , Vec3D const& pos , float w , float* color , float tu, float tv );
float* UF_SetXYZ_RGB( float* vertex , Vec3D const& pos , Vec3D const& color , float tu, float tv );
float* UF_SetXYZ_RGB( float* vertex , Vec3D const& pos , Vec3D const& color , float tu1 , float tv1 , float tu2 , float tv2);
float* UF_SetXYZ_RGB( float* vertex , Vec3D const& pos , Vec3D const& color );
float* UF_SetXYZ( float* vertex , Vec3D const& pos , float tu, float tv );

int  UF_CreateLine( CFObject* obj , Vec3D const& from , Vec3D const& to , Material* mat );
void UF_CreateBoxLine( CFObject* obj , float x, float y , float z , Material* mat );
int  UF_CreateCube( CFObject* obj , float x , float y , float z , Vec3D const& color , Material* mat );
int  UF_CreateSquare3D( CFObject* obj, Vec3D const& pos , int w, int h, Vec3D const& color , Material* mat);
#endif // UtilityFlyFun_h__
