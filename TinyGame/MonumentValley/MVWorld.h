#ifndef MVWorld_h__
#define MVWorld_h__

#include "MVCommon.h"
#include "MVObject.h"

#include <vector>

namespace MV
{

	struct NavLinkInfo;
	class  ActionNavFun;

	class World
	{
	public:
		World();
		~World();

		void   init( Vec3i const& mapSize );
		void   cleanup( bool beDestroy );
		Block* createBlock( 
			Vec3i const& pos , int idMesh , 
			SurfaceDef surfaceDef[] , bool updateNav = true , 
			ObjectGroup* group = NULL , Dir dirZ = eDirZ , Dir dirX = eDirX );
		void   destroyBlock( Block* block );
		bool   destroyBlock( Vec3i const& pos );

		ObjectGroup* createGroup( ObjectGroup* parent = NULL );
		void         destroyGroup( ObjectGroup* group , bool bDeleteObj );

		bool putActor( Actor& actor , Vec3i const& pos , Dir faceDir = eDirZ );
		void removeActor( Actor& actor );

		void action( Actor& actor , int idxNode );
		void setActorBlock( Actor& actor , int blockId , Dir faceDir , bool updateRender = true );

		Block* getBlock( int id ){ return mBlocks[id]; }

		bool checkPosVaild( Vec3i const& pos )
		{
			return mMapOffset.x <= pos.x && pos.x < mMapSize.x + mMapOffset.x &&
				mMapOffset.y <= pos.y && pos.y < mMapSize.y + mMapOffset.y &&
				mMapOffset.z <= pos.z && pos.z < mMapSize.z + mMapOffset.z ;
		}

		int  getBlockCheck( Vec3i const& pos )
		{
			if ( !checkPosVaild( pos ) )
				return 0;
			return getBlock( pos );
		}

		int getBlockIndex( Vec3i const & pos )
		{
			assert( checkPosVaild( pos ) );
			Vec3i temp = pos - mMapOffset;
			return ( temp.z  * mMapSize.y + temp.y ) * mMapSize.x + temp.x ;
		}

		void setBlock( Vec3i const& pos , int id )
		{
			int idx = getBlockIndex( pos );
			//#TODO:map only 1 block
			if ( id && mBlockMap[ idx ] )
			{
				return;
			}
			mBlockMap[ idx ] = id;
		}

		int getBlock( Vec3i const& pos )
		{
			int idx = getBlockIndex( pos );
			return mBlockMap[ idx ];
		}

		void prevEditBlock( Block& block );
		void postEditBlock( Block& block );

		int  getTopParallaxBlock( Vec3i const& pos , Vec3i const& parallaxOffset , Vec3i& outPos );
		int  getTopParallaxBlock( Vec3i const& pos , Vec3i& outPos ){ return getTopParallaxBlock( pos , mParallaxOffset , outPos ); }
		void updateAllNavNode(){  updateNavNode_R( mRootGroup ); }
		void removeAllParallaxNavNode()
		{ 
			removeDynamicNavNode_R( mRootGroup ); 
			mFixNodes.clear();
		}

		void updateNavNode( ObjectGroup& group ){ updateNavNode_R( group ); }
		void removeDynamicNavNode( ObjectGroup& group ){ removeDynamicNavNode_R( group ); }
		void removeParallaxNavNode( ObjectGroup& group ){ removeParallaxNavNode_R( group ); }


		void  updateNeighborNavNode( Vec3i const& pos , ObjectGroup* skipGroup = NULL );

		void updateNavNode_R( ObjectGroup& group );
		void removeDynamicNavNode_R( ObjectGroup& group );
		void removeParallaxNavNode_R( ObjectGroup& group );

		void  updateNavNode( Block& block );
		void  removeNavNode( Block& block );

		void updateBlockNavNode( ObjectGroup& group );
		void updateSurfaceNavNode( Block& block , Dir faceDirL , bool testParallax );
		void buildSurfaceNavNode( Block &block, NavLinkInfo &info );
		int  buildNavLinkFromIndex( Block& block , NavLinkInfo& info , ActionNavFun& fun , int idx );
		int  buildNavLinkFromDir( Block& block , NavLinkInfo& info , ActionNavFun& fun , Dir dir );
		int  buildNeighborNavLink( Block& block , NavLinkInfo& info , ActionNavFun& fun );
		int  buildParallaxNavLink( Block& block , NavLinkInfo& info , ActionNavFun& fun );

		void connectBlockNavNode( Block& block , Dir dirL , Dir linkDirL );

		void removeMap( ObjectGroup& group );
		void updateMap( ObjectGroup& group );
		void destroyBlockInternal( Block* block );


		void increaseUpdateCount();
		bool checkUpdateCount( Block& block );
		int  comparePosFromView( Dir dir , Vec3i const& posA , Vec3i const& posB );


		static Vec3i calcParallaxPos( Vec3i const& pos , Vec3i const& parallaxOffset , int axis )
		{
			int offset = parallaxOffset[axis] * pos[axis];
			return pos - offset * parallaxOffset; 
		}
		Vec3i calcParallaxPos( Vec3i const& pos , int axis )
		{
			return calcParallaxPos( pos , mParallaxOffset , axis );
		}

		void refrechFixNode();
		int  calcFixParallaxPosOffset( Block& block , Dir face );

		int  raycastTest( Vec3f const& startPos , Vec3f endPos )
		{




		}


		void setParallaxDir( int idx )
		{
			mIdxParallaxDir = idx;
			mParallaxOffset = FDir::ParallaxOffset( idx );
		}

		Vec3i const& getParallOffset() const { return mParallaxOffset; }
	
		int        mIdxParallaxDir;
		Vec3i      mParallaxOffset;
		Vec3i      mMapSize;
		Vec3i      mMapOffset;
		typedef std::vector< int > IntMap;
		IntMap     mBlockMap;

		uint32     mUpdateCount;
		uint32     mSkipCount;

		typedef std::vector< ObjectGroup* > GroupVec;
		ObjectGroup mRootGroup;
		GroupVec    mGroups;




		std::vector< NavNode* > mFixNodes;

		int       mFreeBlockId;
		std::vector< Block* > mBlocks;

	};

}//namespace MV

#endif // MVWorld_h__
