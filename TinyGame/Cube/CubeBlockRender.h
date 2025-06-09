#pragma once
#ifndef CubeBlockRender_H_40E60BB2_467B_44A1_98F1_D8076A6291FA
#define CubeBlockRender_H_40E60BB2_467B_44A1_98F1_D8076A6291FA

#include "CubeBase.h"
#include "Core/Color.h"

namespace Cube
{
	class IBlockAccess;
	class Mesh;

	class BlockRenderer
	{
	public:
		void     draw(Vec3i const& offset, BlockId id );
		void     drawUnknown( unsigned faceMask  );
		void     setBasePos( Vec3i const& pos ){ mBasePos = pos; }
		Mesh&    getMesh(){ return *mMesh; }

		Color4ub      mDebugColor;
		Vec3i         mBasePos;
		IBlockAccess* mBlockAccess;
		Mesh*         mMesh;
	};

}//namespace Cube

#endif // CubeBlockRender_H_40E60BB2_467B_44A1_98F1_D8076A6291FA
