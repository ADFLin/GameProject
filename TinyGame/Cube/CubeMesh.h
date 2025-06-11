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

		int  getVertexNum(){ return (int)mVertices.size(); }
		void clearBuffer()
		{
			mVertices.clear();
			mIndices.clear();
		}

		struct Vertex
		{
			Vec3f     pos;
			Color4ub  color;
			Vec3f     normal;
			uint32    meta;
			Vec2f     uv;
		};

		TArray< Vertex > mVertices;
		TArray< uint32 > mIndices;
	};
}
#endif // CubeMesh_h__
