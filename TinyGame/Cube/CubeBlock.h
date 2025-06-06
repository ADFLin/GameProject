#ifndef CubeBlock_h__
#define CubeBlock_h__

#include "CubeBase.h"

namespace Cube
{
	class IBlockAccess;

	struct AABB
	{
		AABB( Vec3f const& max , Vec3f const& min )
			:max( max ), min ( min ){}
		Vec3f max;
		Vec3f min;
	};

	class Block
	{
	public:
		Block( BlockId id );
		BlockId getId(){ return mId; }


		static void   InitList();
		static Block* Get( BlockId id );


		bool    isSolid(){ return mbSolid; }
		Block&  setSolid( bool beS = true ){ mbSolid = beS; return *this; }

		virtual unsigned    calcRenderFaceMask( IBlockAccess& blockAccess , Vec3i const& blockPos);
		virtual bool        canPlaceItem( FaceSide face , ItemId itemId ){ return false; }
		virtual void        onNeighborBlockModify( IBlockAccess& blockAccess , Vec3i const& blockPos, FaceSide face ){}
		virtual AABB const* getCollisionBox( AABB& aabb );
	private:
		BlockId mId;
		bool    mbSolid;
		
	};

	class LiquidBlock : public Block
	{
		typedef Block BaseClass;
	public:
		LiquidBlock( BlockId id )
			:BaseClass( id ){}

		unsigned calcRenderFaceMask( IBlockAccess& blockAccess , Vec3i const& blockPos);
		virtual void onNeighborBlockModify( IBlockAccess& blockAccess , Vec3i const& blockPos, FaceSide face );
	};

}//namespace Cube

#endif // CubeBlock_h__
