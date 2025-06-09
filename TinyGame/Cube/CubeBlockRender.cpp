#include "CubePCH.h"
#include "CubeBlockRender.h"

#include "CubeBlock.h"
#include "CubeBlockType.h"

#include "CubeMesh.h"

#include "WindowsHeader.h"
#include <gl/GL.h>
#include <gl/GLU.h>


namespace Cube
{
	void BlockRenderer::draw(Vec3i const& offset, BlockId id )
	{
		Block* block = Block::Get( id );
		unsigned faceMask = block->calcRenderFaceMask( *mBlockAccess , mBasePos + offset);
		if ( !faceMask )
			return;

		getMesh().setVertexOffset(Vec3f(offset));

		switch( id )
		{
		case BLOCK_DIRT:
		default:
			drawUnknown( faceMask );
		}
	}

	void BlockRenderer::drawUnknown(unsigned faceMask)
	{
		Mesh& mesh = getMesh();

		mesh.setColor(mDebugColor);
		//mesh.setColor( 0 , 255 , 0 );

		for (int face = 0; face < FaceSide::COUNT; ++face)
		{
			if (faceMask & BIT(face))
			{
				mesh.setIndexBase(mesh.getVertexNum());
				mesh.setNormal(GetFaceNoraml(FaceSide(face)));
				auto faceVertices = GetFaceVertices(FaceSide(face));
				mesh.addVertex(faceVertices[0]);
				mesh.addVertex(faceVertices[1]);
				mesh.addVertex(faceVertices[2]);
				mesh.addVertex(faceVertices[3]);

				mesh.addQuad(0, 1, 2, 3);
			}
		}
	}

}//namespace Cube