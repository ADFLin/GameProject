#include "ProjectPCH.h"
#include "TriangleMeshData.h"

#include "CFObject.h"
using namespace CFly;

#include "PhysicsSystem.h"

#include <fstream>


using namespace CFly;
void TriangleMeshData::add( CFly::Object* object )
{
	Matrix4 const& m = object->getWorldTransform();
	Object::MeshIterator iter = object->getElements();

	for( ; iter.haveMore(); iter.next() )
	{
		MeshBase* mesh = iter.get()->getMesh();

		int startIndex = m_vertexVec.size();
		m_vertexVec.reserve( startIndex + mesh->getVertexNum() );

		unsigned offset;
		VertexBuffer* buffer = mesh->getVertexElement( CFV_XYZ , offset );

		if ( buffer )
		{
			uint8* ptr = static_cast< uint8* >( buffer->lock() ) + offset;
			unsigned strride = buffer->getVertexSize();

			for( int i = 0 ; i < mesh->getVertexNum() ; ++i )
			{
				m_vertexVec.push_back( Vec3D() );

				Vec3D& vtx = m_vertexVec.back();
				Vec3D& v = *reinterpret_cast< Vec3D* >( ptr );

				vtx = TransformUntility::mul( v , m );
				ptr += strride;
			}

			buffer->unlock();
		}
	}

}

//void TTerrainData::add( FnObject& obj )
//{
//	FyVec3D pos;
//	obj.GetWorldPosition( (float*)&pos );
//	float* m = obj.GetMatrix( TRUE );
//	assert( pos.x == m[12] );
//
//	int num = obj.GetGeometryNumber();
//	for (int j = 0; j < num ; ++j )
//	{
//		FnGeometry geom;
//		geom.Object( obj.Object() , j );
//
//		assert( geom.GetType() == 1 );
//
//		obj.GotoGeometryElement(j);
//
//		ObjTriInfo info;
//		info.init( obj.Object() );
//		int startIndex = m_vertexVec.size();
//		m_vertexVec.reserve( startIndex + info.numVertex );
//
//		for( int i = 0 ; i < info.numVertex ; ++i )
//		{
//			m_vertexVec.push_back( FyVec3D() );
//			FyVec3D& vtx = m_vertexVec.back();
//
//			float* pv = &info.vertex[ info.vertexLen * i ];
//
//			vtx.x = pv[0]*m[0] + pv[1]*m[4] + pv[2]*m[8 ] + m[12];
//			vtx.y = pv[0]*m[1] + pv[1]*m[5] + pv[2]*m[9 ] + m[13];
//			vtx.z = pv[0]*m[2] + pv[1]*m[6] + pv[2]*m[10] + m[14];
//
//		}
//
//		m_triVec.reserve( m_triVec.size() + info.numFace );
//
//		for( int i = 0 ; i < info.numFace ; ++i )
//		{
//			m_triVec.push_back( tri_t() );
//			tri_t& tri = m_triVec.back();
//			int* pv = &info.face[3*i];
//
//			tri.v[0] = startIndex + pv[0];
//			tri.v[1] = startIndex + pv[1];
//			tri.v[2] = startIndex + pv[2];
//		}
//	}
//}

bool TriangleMeshData::save( char const* name )
{
	using namespace std;

	fstream fs;
	fs.open( name , ios::out | ios::binary );

	if ( !fs )
		return false;

	int size;
	size = m_vertexVec.size();
	fs.write( reinterpret_cast<char *>(&size) , sizeof(int) );
	fs.write( (char*) &(m_vertexVec[0]) ,sizeof(Vec3D) * size );

	size = m_triVec.size();
	fs.write( reinterpret_cast<char *>(&size) , sizeof(int) );
	fs.write( (char*) &(m_triVec[0]) ,sizeof(TriangleIndex) * size );

	fs.close();
	return true;
}

bool TriangleMeshData::load( char const* name )
{
	using namespace std;

	ifstream fs;
	fs.open( name , ios::binary );

	if ( !fs )
		return false;

	int size;
	fs.read( reinterpret_cast<char *>(&size) , sizeof(int) );
	m_vertexVec.resize(size);
	fs.read((char*) &(m_vertexVec[0]) ,sizeof(Vec3D) * size );


	size = m_triVec.size();
	fs.read( reinterpret_cast<char *>(&size) , sizeof(int) );
	m_triVec.resize(size);
	fs.read( (char*) &(m_triVec[0]) , sizeof(TriangleIndex)* size );

	fs.close();
	return true;
}


int TriangleMeshData::createDebugMesh( CFly::Object* obj , CFly::Material* mat )
{
	struct vtx_t
	{
		vtx_t( Vec3D const& v , Vec3D const& c)
			:vtx(v),color(c){}
		Vec3D vtx;
		Vec3D color;
	};

	std::vector< vtx_t > vtxVec;
	std::vector< TriangleIndex > triVec;
	vtxVec.reserve( m_triVec.size() * 3 );
	triVec.reserve( m_triVec.size() );
	int index = -1;
	for (int i = 0; i < m_triVec.size(); ++i )
	{
		TriangleIndex& tri = m_triVec[i];
		Vec3D& v0 = m_vertexVec[ tri.v[0] ]; 
		Vec3D& v1 = m_vertexVec[ tri.v[1] ]; 
		Vec3D& v2 = m_vertexVec[ tri.v[2] ]; 
		Vec3D normal = ( v1 - v0 ).cross( v2 - v1 );

		float len = sqrt( normal.dot(normal) );
		float c = ( 2 + (normal.y + normal.z)/len )/ 4 ;

		Vec3D color( c ,c , c );
		vtxVec.push_back( vtx_t( v0 , color ) );
		vtxVec.push_back( vtx_t( v1 , color ) );
		vtxVec.push_back( vtx_t( v2 , color ) );

		TriangleIndex t;
		t.v[0] = ++index;
		t.v[1] = ++index;
		t.v[2] = ++index;

		triVec.push_back( t );
	}

	return obj->createIndexedTriangle( 
		mat , CFVT_XYZ_CF1 , 
		(float*) &vtxVec[0] , vtxVec.size() , 
		&( triVec[0].v[0] ) , triVec.size() );
}



int TriangleMeshData::createGeom( CFly::Object* obj , CFly::Material* mat  )
{
	struct vtx_t
	{
		vtx_t( Vec3D const& v )
			:vtx(v){}
		Vec3D vtx;
	};

	std::vector< vtx_t > vtxVec;
	std::vector< TriangleIndex > triVec;
	vtxVec.reserve( m_triVec.size() * 3 );
	triVec.reserve( m_triVec.size() );

	int index = -1;
	for (int i = 0; i < m_triVec.size(); ++i )
	{
		TriangleIndex& tri = m_triVec[i];
		Vec3D& v0 = m_vertexVec[ tri.v[0] ]; 
		Vec3D& v1 = m_vertexVec[ tri.v[1] ]; 
		Vec3D& v2 = m_vertexVec[ tri.v[2] ]; 


		vtxVec.push_back( vtx_t( v0 ) );
		vtxVec.push_back( vtx_t( v1 ) );
		vtxVec.push_back( vtx_t( v2 ) );

		TriangleIndex t;
		t.v[0] = ++index;
		t.v[1] = ++index;
		t.v[2] = ++index;

		triVec.push_back( t );
	}

	return obj->createIndexedTriangle( 
		mat , CFVT_XYZ  , 
		(float*) &vtxVec[0] ,vtxVec.size() , 
		 &( triVec[0].v[0] ) , triVec.size() );
}

bool TriangleMeshData::saveFileOBJFormat( char const* name ,float scale )
{
	using namespace std;

	fstream fs;
	fs.open( name , ios::out );

	if ( !fs )
		return false;

	int size;
	size = m_vertexVec.size();
	for( int i = 0 ; i < size ; ++ i )
	{
		Vec3D& vtx = m_vertexVec[i];
		fs << "v " 
		   << vtx.y * scale << " " 
		   << vtx.z * scale << " " 
		   << vtx.x * scale << "\n";
	}

	size = m_triVec.size();
	for( int i = 0 ; i < size ; ++ i )
	{
		TriangleIndex& tri = m_triVec[i];
		fs << "f " 
		   << tri.v[0] + 1 << " " 
		   << tri.v[1] + 1 << " " 
		   << tri.v[2] + 1 << "\n";
	}

	fs.close();
	return true;

}



void TriangleMeshData::fillShapeParams( PhyShapeParams& info )
{
	info.type = CSHAPE_TRIANGLE_MESH;
	info.triMesh.numTriangles        = m_triVec.size();
	info.triMesh.triangleIndex       = (int*) &m_triVec[0];
	info.triMesh.triangleIndexStride = sizeof( TriangleIndex );
	info.triMesh.numVertices         = m_vertexVec.size();
	info.triMesh.vertex              = (float*)&m_vertexVec[0];
	info.triMesh.vertexStride        = sizeof( Vec3D );
}
