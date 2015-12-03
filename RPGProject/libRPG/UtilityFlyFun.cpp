#include "ProjectPCH.h"
#include "UtilityFlyFun.h"

//#include "UtilityMath.h"
//#include "TGame.h"
//#include "FyVec3D.h"

#include "PhysicsSystem.h"

#include "CFObject.h"
#include "CFMaterial.h"
#include "CFMeshBuilder.h"


float* UF_SetXYZW_RGB( float* vertex , Vec3D const& pos , float w , float* color , float tu, float tv )
{
	*(vertex++) = pos[0]; 
	*(vertex++) = pos[1]; 
	*(vertex++) = pos[2];

	*(vertex++) = w;

	*(vertex++) = color[0];  
	*(vertex++) = color[1];  
	*(vertex++) = color[2];

	*(vertex++) = tu;       
	*(vertex++) = tv;

	return vertex;
}

float* UF_SetXYZ_RGB( float* vertex , Vec3D const& pos , Vec3D const& color , float tu, float tv )
{
	*(vertex++) = pos[0]; 
	*(vertex++) = pos[1]; 
	*(vertex++) = pos[2];
	*(vertex++) = color[0];  
	*(vertex++) = color[1];  
	*(vertex++) = color[2];
	*(vertex++) = tu;       
	*(vertex++) = tv;

	return vertex;
}

float* UF_SetXYZ_RGB( float* vertex , Vec3D const& pos , Vec3D const& color , 
					 float tu1 , float tv1 , float tu2 , float tv2)
{
	*(vertex++) = pos[0]; 
	*(vertex++) = pos[1]; 
	*(vertex++) = pos[2];
	*(vertex++) = color[0];  
	*(vertex++) = color[1];  
	*(vertex++) = color[2];
	*(vertex++) = tu1;
	*(vertex++) = tv1;
	*(vertex++) = tu2;
	*(vertex++) = tv2;

	return vertex;
}

float* UF_SetXYZ_RGB( float* vertex , Vec3D const& pos , Vec3D const& color )
{
	*(vertex++) = pos[0]; 
	*(vertex++) = pos[1]; 
	*(vertex++) = pos[2];
	*(vertex++) = color[0];  
	*(vertex++) = color[1];  
	*(vertex++) = color[2];
	return vertex;
}

float* UF_SetXYZ( float* vertex , Vec3D const& pos , float tu, float tv )
{
	*(vertex++) = pos[0]; 
	*(vertex++) = pos[1]; 
	*(vertex++) = pos[2];
	*(vertex++) = tu;       
	*(vertex++) = tv;
	return vertex;
}


void UF_CreateBoxLine( CFObject* obj , float x, float y , float z , Material* mat )
{

	float fV[] = {  x, y, z,  x,-y, z,  x,-y,-z,  x, y,-z };
	float bV[] = { -x, y, z, -x,-y, z, -x,-y,-z, -x, y,-z };
	float lV[] = {  x, y, z, -x, y, z, -x, y,-z,  x, y,-z };
	float rV[] = {  x,-y, z, -x,-y, z, -x,-y,-z,  x,-y,-z };

	obj->createLines( mat , CFly::CF_CLOSE_POLYLINE , fV  , 4 );
	obj->createLines( mat , CFly::CF_CLOSE_POLYLINE , bV , 4 );
	obj->createLines( mat , CFly::CF_CLOSE_POLYLINE , lV , 4 );
	obj->createLines( mat , CFly::CF_CLOSE_POLYLINE , rV , 4 );
}


int UF_CreateLine( CFObject* obj , Vec3D const& from , Vec3D const& to , Material* mat )
{
	Vec3D vertex[2] = { from , to };
	return obj->createLines( mat , CFly::CF_CLOSE_POLYLINE , &vertex[0].x  ,  2  );
}


int UF_CreateCircle( CFObject* obj , float r , int num , Material* mat  )
{
	Vec3D* vertex = new Vec3D[ num+1 ];

	for (int i =0;i < num ; ++i )
	{
		float theta = (2 * Math::PI * i) / num;
		vertex[i].x = r*cos(theta);
		vertex[i].y = r*sin(theta);  
		vertex[i].z = 0;
	}

	vertex[num] = vertex[0];
	int gemoID = obj->createLines( mat , CFly::CF_OPEN_POLYLINE , &vertex[0].x  ,  num+1 );

	delete [] vertex;

	return gemoID;
}

//int UF_CreateSquare2D( IObject* obj, Vec3D const& pos , int w, int h, float *color , IMaterial* mat )
//{
//	float v[36], z;
//	// setup the z
//	z = pos.z();
//	// upper-left corner
//	UF_SetXYZW_RGB( &v[ 0] , pos                      , 1 , color , 0.0f , 1.0f );
//	UF_SetXYZW_RGB( &v[ 9] , pos + Vec3D( 0 , h , 0 ) , 1 , color , 0.0f , 0.0f );
//	UF_SetXYZW_RGB( &v[18] , pos + Vec3D( w , h , 0 ) , 1 , color , 1.0f , 0.0f );
//	UF_SetXYZW_RGB( &v[27] , pos + Vec3D( w , 0 , 0 ) , 1 , color , 1.0f , 1.0f );
//
//	int texLen = 2;
//	WORD tri[6];
//	tri[0] = 0; tri[1] = 1; tri[2] = 3;
//	tri[3] = 1; tri[4] = 2; tri[5] = 3;
//
//	return  obj->createIndexedIndexedTriangle(
//		CFly::CFVT_XYZW_RGB , mat , 1, 4, 1 , &texLen, v, 2, tri, TRUE);
//}


int UF_CreateSquare3D( CFObject* obj, Vec3D const& pos , int w, int h, Vec3D const& color , Material* mat)
{
	float v[ 8 * 4 ];

	float* vtx = v;
	vtx = UF_SetXYZ_RGB( vtx , pos                      , color , 0.0f , 0.0f );
	vtx = UF_SetXYZ_RGB( vtx , pos + Vec3D( w , 0 , 0 ) , color , 1.0f , 0.0f );
	vtx = UF_SetXYZ_RGB( vtx , pos + Vec3D( w , h , 0 ) , color , 1.0f , 1.0f );
	vtx = UF_SetXYZ_RGB( vtx , pos + Vec3D( 0 , h , 0 ) , color , 0.0f , 1.0f );
	
	int tri[6];
	tri[0] = 0; tri[1] = 1; tri[2] = 3;
	tri[3] = 1; tri[4] = 2; tri[5] = 3;

	using namespace CFly;
	return  obj->createIndexedTriangle( mat , CFVT_XYZ_CF1 | CFVF_TEX1( 2 ) , v , 4 , tri , 2  );
}


int* UF_SetIndex( int* tri , int v1, int v2 , int v3 )
{
	*(tri++) = v1; *(tri++)  = v2; *(tri++) = v3;
	return tri;
}

int UF_CreateCube( CFObject* obj , float x , float y , float z , Vec3D const& color , Material* mat )
{

	float v[ 8 * 20 ];

	float* fill = v;

	fill = UF_SetXYZ_RGB( fill, Vec3D(-x, -y, -z),color , 1.0000, 0.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D(-x,  y, -z),color , 1.0000, 1.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D( x,  y, -z),color , 0.0000, 1.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D( x, -y, -z),color , 0.0000, 0.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D(-x, -y,  z),color , 0.0000, 0.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D( x, -y,  z),color , 1.0000, 0.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D( x,  y,  z),color , 1.0000, 1.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D(-x,  y,  z),color , 0.0000, 1.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D(-x, -y, -z),color , 0.0000, 0.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D( x, -y, -z),color , 1.0000, 0.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D( x, -y,  z),color , 1.0000, 1.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D(-x, -y,  z),color , 0.0000, 1.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D( x,  y, -z),color , 1.0000, 0.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D( x, -y,  z),color , 0.0000, 1.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D( x,  y, -z),color , 0.0000, 0.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D(-x,  y, -z),color , 1.0000, 0.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D(-x,  y,  z),color , 1.0000, 1.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D( x,  y,  z),color , 0.0000, 1.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D(-x,  y, -z),color , 0.0000, 0.0000);
	fill = UF_SetXYZ_RGB( fill, Vec3D(-x, -y,  z),color , 1.0000, 1.0000);

	int tri[ 3 * 12 ];

	int* idx = tri;
	idx = UF_SetIndex( idx ,  0,  1,  2 );
	idx = UF_SetIndex( idx ,  2,  3,  0 );
	idx = UF_SetIndex( idx ,  4,  5,  6 );
	idx = UF_SetIndex( idx ,  6,  7,  4 );
	idx = UF_SetIndex( idx ,  8,  9, 10 );
	idx = UF_SetIndex( idx , 10, 11,  8 );
	idx = UF_SetIndex( idx ,  3, 12,  6 );
	idx = UF_SetIndex( idx ,  6, 13,  3 );
	idx = UF_SetIndex( idx , 14, 15, 16 );
	idx = UF_SetIndex( idx , 16, 17, 14 );
	idx = UF_SetIndex( idx , 18,  0, 19 );
	idx = UF_SetIndex( idx , 19,  7, 18 );

	using namespace CFly;
	return  obj->createIndexedTriangle( mat , CFVT_XYZ_CF1 | CFVF_TEX1( 2 ) , v  , 20 , tri , 12  );
}


