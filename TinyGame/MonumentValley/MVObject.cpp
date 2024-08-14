#include "MVObject.h"

namespace MV
{
	BlockSurface* NavNode::getSurface()
	{
		return BlockSurface::Get(*this);
	}

	void NavNode::connect(NavNode& other)
	{
		assert(link == NULL && other.link == NULL);
		link = &other;
		other.link = this;
		if (getSurface()->getBlock()->group == other.getSurface()->getBlock()->group)
		{
			flag |= eStatic;
			other.flag |= eStatic;
		}
	}

	BlockSurface* BlockSurface::Get(NavNode& node)
	{
		static_assert(offsetof(BlockSurface, nodes[1][0]) == offsetof(BlockSurface, nodes) + (FACE_NAV_LINK_NUM * 1 + 0) * sizeof(NavNode));
		static_assert(offsetof(BlockSurface, nodes[1][1]) == offsetof(BlockSurface, nodes) + (FACE_NAV_LINK_NUM * 1 + 1) * sizeof(NavNode));
		static_assert(offsetof(BlockSurface, nodes[2][0]) == offsetof(BlockSurface, nodes) + (FACE_NAV_LINK_NUM * 2 + 0) * sizeof(NavNode));
		static_assert(offsetof(BlockSurface, nodes[3][1]) == offsetof(BlockSurface, nodes) + (FACE_NAV_LINK_NUM * 3 + 1) * sizeof(NavNode));

		ptrdiff_t offset = offsetof(BlockSurface, nodes) + (FACE_NAV_LINK_NUM * node.type + node.idxDir) * sizeof(NavNode);
		return reinterpret_cast<BlockSurface*>(intptr_t(&node) - offset);
	}

	Block* BlockSurface::getBlock()
	{
		return Block::Get(*this);
	}

	Block* Block::Get(BlockSurface& surface)
	{
		ptrdiff_t offset = offsetof(Block, surfaces) + surface.face * sizeof(NavNode);
		return reinterpret_cast<Block*>(intptr_t(&surface) - offset);
	}

}//namespace MV
