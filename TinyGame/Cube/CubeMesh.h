#ifndef CubeMesh_h__
#define CubeMesh_h__

#include "CubeBase.h"

#include "Core/Color.h"
#include "Core/HalfFlot.h"
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
			int16     normal[4];
			uint32    meta;
			HalfFloat uv[2];
		};

		TArray< Vertex > mVertices;
		TArray< uint32 > mIndices;
	};
}
#endif // CubeMesh_h__
