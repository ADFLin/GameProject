#ifndef CubeBlockRenderer_h__
#define CubeBlockRenderer_h__

#include "CubeBase.h"

namespace Cube
{
	class IBlockAccess;
	class Mesh;

	class BlockRenderer
	{
	public:
		void     draw( int ox , int oy , int oz , BlockId id );
		void     drawUnknown( unsigned faceMask  );
		void     setBasePos( Vec3i const& pos ){ mBasePos = pos; }
		Mesh&    getMesh(){ return *mMesh; }
		IBlockAccess* replaceBlockAccess( IBlockAccess& blockAccess );


		Vec3i         mBasePos;
		IBlockAccess* mBlockAccess;
		Mesh*         mMesh;
	};

}//namespace Cube

#endif // CubeBlockRenderer_h__
