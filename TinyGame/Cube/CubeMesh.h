#ifndef CubeMesh_h__
#define CubeMesh_h__

#include "CubeBase.h"

#include "Core/Color.h"
#include "DataStructure/Array.h"

namespace Cube
{

	class Mesh
	{
	public:
		Mesh();

		void setVertexOffset( Vec3f const& offset ){ mVertexOffset = offset; }
		int  getVertexNum(){ return (int)mVtxBuffer.size(); }
		void addVertex( Vec3f const& pos );
		void addVertex( float x , float y , float z );
		void setIndexBase( int index );
		void addTriangle( int o1 , int o2 , int o3  );
		void addQuad( int o1 , int o2 , int o3 , int o4 );

		void setColor( uint8 r , uint8 g , uint8 b );
		void setColor(Color4ub const& color)
		{
			mCurVertex.color = color;
		}

		void clearBuffer()
		{
			mVtxBuffer.clear();
			mIndexBuffer.clear();
		}

		struct Vertex
		{
			float pos[3];
			Color4ub  color;
		};

		TArray< Vertex > mVtxBuffer;
		TArray< uint32 >  mIndexBuffer;

		Vec3f   mVertexOffset;
		int     mIndexBase;
		Vertex  mCurVertex;


	};
}
#endif // CubeMesh_h__
