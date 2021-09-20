#include "CubePCH.h"
#include "CubeBlock.h"

#include "CubeBlockRenderer.h"
#include "CubeBlockType.h"
#include "CubeWorld.h"

namespace Cube
{
	static Block* sBlockMap[ 256 ] = { 0 };


	void Block::InitList()
	{
		(*new Block( BLOCK_DIRT )).setSolid( true );
		(*new LiquidBlock( BLOCK_WATER ) ).setSolid( false );
	}

	Block* Block::Get( BlockId id )
	{
		return sBlockMap[ id ];
	}

	Block::Block( BlockId id ) :mId( id )
		,mbSolid( true )
	{
		if ( sBlockMap[ id ] )
		{
			delete sBlockMap[id];
		}
		sBlockMap[ id ] = this;

	}

	unsigned Block::calcRenderFaceMask( IBlockAccess& blockAccess , int bx , int by , int bz  )
	{
		unsigned mask = 0;

		if ( !blockAccess.isOpaqueBlock( bx + 1 , by , bz ) )
			mask |= BIT( FACE_X );
		if ( !blockAccess.isOpaqueBlock( bx  , by + 1 , bz ) )
			mask |= BIT( FACE_Y );
		if ( !blockAccess.isOpaqueBlock( bx  , by , bz + 1 ) )
			mask |= BIT( FACE_Z );
		if ( !blockAccess.isOpaqueBlock( bx - 1 , by , bz ) )
			mask |= BIT( FACE_NX );
		if ( !blockAccess.isOpaqueBlock( bx , by - 1 , bz ) )
			mask |= BIT( FACE_NY );
		if ( !blockAccess.isOpaqueBlock( bx , by , bz - 1 ) )
			mask |= BIT( FACE_NZ );

		return mask;
	}


	static AABB const gDefaultAABB( Vec3f(0,0,0) , Vec3f(1,1,1) );
	AABB const* Block::getCollisionBox( AABB& aabb )
	{
		return &gDefaultAABB;
	}


	unsigned LiquidBlock::calcRenderFaceMask( IBlockAccess& blockAccess , int bx , int by , int bz )
	{
		unsigned mask = 0;
		if ( blockAccess.getBlockId( bx + 1 , by , bz ) != getId() )
			mask |= BIT( FACE_X );
		if ( blockAccess.getBlockId( bx , by + 1 , bz ) != getId() )
			mask |= BIT( FACE_Y );
		if ( blockAccess.getBlockId( bx , by , bz + 1 ) != getId() )
			mask |= BIT( FACE_Z );
		if ( blockAccess.getBlockId( bx - 1 , by , bz ) != getId() )
			mask |= BIT( FACE_NX );
		if ( blockAccess.getBlockId( bx , by - 1 , bz ) != getId() )
			mask |= BIT( FACE_NY );
		if ( blockAccess.getBlockId( bx , by , bz - 1 ) != getId() )
			mask |= BIT( FACE_NZ );

		return mask;
	}

	void LiquidBlock::onNeighborBlockModify( IBlockAccess& blockAccess , int bx , int by , int bz , FaceSide face )
	{
		unsigned meta = blockAccess.getBlockMeta( bx , by , bz );
	}

}//namespace Cube