#include "CubePCH.h"
#include "CubeMesh.h"

namespace Cube
{

	Mesh::Mesh()
	{
		mVertices.reserve( 4 * 1000 );
		mIndices.reserve( 3 * 2 * 1000 );
	}


}//namespace Cube
