#ifndef CubeMesh_h__
#define CubeMesh_h__

#include "CubeBase.h"

#include "Core/Color.h"
#include "DataStructure/Array.h"
#include "RHI/RHICommon.h"
#include "Math/GeometryPrimitive.h"

namespace Cube
{
	struct MeshData
	{
		Render::RHIBufferRef vertexBuffer;
		Render::RHIBufferRef indexBuffer;
	};


	class Mesh
	{
	public:
		Mesh();

		void setVertexOffset( Vec3f const& offset ){ mVertexOffset = offset; }
		int  getVertexNum(){ return (int)mVertices.size(); }
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
		void setNormal(Vec3f const& noraml)
		{
			mCurVertex.normal = noraml;
		}

		void clearBuffer()
		{
			mVertices.clear();
			mIndices.clear();
			bound.invalidate();
		}

		struct Vertex
		{
			Vec3f     pos;
			Color4ub  color;
			Vec3f     normal;

			bool operator == (Vertex const& rhs) const
			{
				return pos == rhs.pos && /*color == rhs.color &&*/ normal == rhs.normal;
			}

			uint32 getTypeHash() const 
			{
				uint32 result = HashValues(pos.x, pos.y, pos.z, /*color.r, color.g, color.b, color.a ,*/ normal.x, normal.y , normal.z);
				return result;
			}
		};

		Math::TAABBox< Vec3f > bound;

		TArray< Vertex > mVertices;
		TArray< uint32 > mIndices;

		Vec3f   mVertexOffset;
		int     mIndexBase;
		Vertex  mCurVertex;


		void merge();
	};
}
#endif // CubeMesh_h__
