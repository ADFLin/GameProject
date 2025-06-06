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

	bool IsOpaqueBlock(BlockId id)
	{
		return id != BLOCK_NULL;
	}

	unsigned Block::calcRenderFaceMask( IBlockAccess& blockAccess , Vec3i const& blockPos)
	{
		unsigned mask = 0;

		BlockId ids[FaceSide::COUNT];
		blockAccess.getNeighborBlockIds(blockPos, ids);
		for (int i = 0; i < FaceSide::COUNT; ++i)
		{
			if (!IsOpaqueBlock(ids[i]))
			{
				mask |= BIT(i);
			}
		}	 
		return mask;
	}


	static AABB const gDefaultAABB( Vec3f(0,0,0) , Vec3f(1,1,1) );
	AABB const* Block::getCollisionBox( AABB& aabb )
	{
		return &gDefaultAABB;
	}

	unsigned LiquidBlock::calcRenderFaceMask( IBlockAccess& blockAccess , Vec3i const& blockPos)
	{
		unsigned mask = 0;

		BlockId ids[FaceSide::COUNT];
		blockAccess.getNeighborBlockIds(blockPos, ids);
		for (int i = 0; i < FaceSide::COUNT; ++i)
		{
			if (ids[i] != getId())
			{
				mask |= BIT(i);
			}
		}
		return mask;
	}

	void LiquidBlock::onNeighborBlockModify( IBlockAccess& blockAccess , Vec3i const& blockPos, FaceSide face )
	{
		unsigned meta = blockAccess.getBlockMeta(blockPos.x , blockPos.y , blockPos.z );
	}

}//namespace Cube