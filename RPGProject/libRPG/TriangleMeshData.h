#ifndef TriangleMeshData_h__
#define TriangleMeshData_h__

#include "common.h"
#include <vector>

//struct ObjTriInfo
//{
//	void init( OBJECTid id )
//	{
//		FnObject obj;
//		obj.Object( id );
//
//		vertexLen = * (int*) obj.GetTriangleData( TRIANGLE_VERTEX_LENGTH );
//		numFace   =  * (int*) obj.GetTriangleData( TRIANGLE_FACE_NUMBER );
//		face      = (int*) obj.GetTriangleData( TRIANGLE_FACE );
//		numVertices = *(int*) obj.GetTriangleData( TRIANGLE_VERTEX_NUMBER );
//		vertex    = (float*) obj.GetTriangleData( TRIANGLE_VERTEX );
//	}
//
//	int       vertexLen;
//	int       numFace;
//	int*      face;
//	int       numVertices;
//	float*    vertex;
//};

struct PhyShapeParams;


class TriangleMeshData
{
public:

	void add( CFly::Object* object );
	
	bool save( char const* name );
	bool load( char const* name );

	bool saveFileOBJFormat( char const* name ,float scale = 1.0f );
	int  createGeom( CFly::Object* obj , CFly::Material* mat );
	int  createDebugMesh( CFly::Object* obj , CFly::Material* mat );

	void fillShapeParams( PhyShapeParams& info );
public:
	
	struct TriangleIndex
	{
		int v[3];
	};
	//void build();
	std::vector< TriangleIndex > m_triVec;
	std::vector< Vec3D > m_vertexVec;
};

#endif // TriangleMeshData_h__
