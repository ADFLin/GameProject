#include "CubePCH.h"
#include "CubeBlockRenderer.h"

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

	void BlockRenderer::drawUnknown( unsigned faceMask )
	{
		Mesh& mesh = getMesh();

		mesh.setColor(mDebugColor);
		//mesh.setColor( 0 , 255 , 0 );
		if ( faceMask & BIT( FACE_Z ) )
		{
			mesh.setIndexBase( mesh.getVertexNum() );

			mesh.addVertex( 0 , 0 , 1 );
			mesh.addVertex( 1 , 0 , 1 );
			mesh.addVertex( 1 , 1 , 1 );
			mesh.addVertex( 0 , 1 , 1 );

			mesh.addQuad( 0 , 1 , 2 , 3 );
		}

		if ( faceMask & BIT( FACE_NZ ) )
		{
			mesh.setIndexBase( mesh.getVertexNum() );
			mesh.addVertex( 0 , 1 , 0 );
			mesh.addVertex( 1 , 1 , 0 );
			mesh.addVertex( 1 , 0 , 0 );
			mesh.addVertex( 0 , 0 , 0 );
			mesh.addQuad( 0 , 1 , 2 , 3 );
		}

		if ( faceMask & BIT( FACE_X ) )
		{
			mesh.setIndexBase( mesh.getVertexNum() );
			mesh.addVertex( 1 , 0 , 0 );
			mesh.addVertex( 1 , 1 , 0 );
			mesh.addVertex( 1 , 1 , 1 );
			mesh.addVertex( 1 , 0 , 1 );
			mesh.addQuad( 0 , 1 , 2 , 3 );
		}

		if ( faceMask & BIT( FACE_NX ) )
		{
			mesh.setIndexBase( mesh.getVertexNum() );
			mesh.addVertex( 0 , 0 , 1 );
			mesh.addVertex( 0 , 1 , 1 );
			mesh.addVertex( 0 , 1 , 0 );
			mesh.addVertex( 0 , 0 , 0 );
			mesh.addQuad( 0 , 1 , 2 , 3 );
		}

		if ( faceMask & BIT( FACE_Y ) )
		{
			mesh.setIndexBase( mesh.getVertexNum() );
			mesh.addVertex( 0 , 1 , 1 );
			mesh.addVertex( 1 , 1 , 1 );
			mesh.addVertex( 1 , 1 , 0 );
			mesh.addVertex( 0 , 1 , 0 );
			mesh.addQuad( 0 , 1 , 2 , 3 );
		}

		if ( faceMask & BIT( FACE_NY ) )
		{
			mesh.setIndexBase( mesh.getVertexNum() );
			mesh.addVertex( 0 , 0 , 0 );
			mesh.addVertex( 1 , 0 , 0 );
			mesh.addVertex( 1 , 0 , 1 );
			mesh.addVertex( 0 , 0 , 1 );
			mesh.addQuad( 0 , 1 , 2 , 3 );
		}

	}

	IBlockAccess* BlockRenderer::replaceBlockAccess( IBlockAccess& blockAccess )
	{
		IBlockAccess* oldAccess = mBlockAccess;
		mBlockAccess = &blockAccess;
		return oldAccess;
	}

}//namespace Cube