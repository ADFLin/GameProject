#ifndef CubeBlockRenderer_h__
#define CubeBlockRenderer_h__

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
		IBlockAccess* replaceBlockAccess( IBlockAccess& blockAccess );



		Color4ub      mDebugColor;
		Vec3i         mBasePos;
		IBlockAccess* mBlockAccess;
		Mesh*         mMesh;
	};

}//namespace Cube

#endif // CubeBlockRenderer_h__
