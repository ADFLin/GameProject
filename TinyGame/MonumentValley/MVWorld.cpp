#include "MVWorld.h"

#include <algorithm>

namespace MV
{

	struct NavBuildContext
	{
		Dir   faceDirL;
		Dir   faceDir;
		Vec3i faceOffset;
		Dir   linkDirL;
		Dir   linkDir;
		int   nodeType;
		bool  bParallaxTest;

		Block* block;
		NavNode* node;
		BlockSurface* surface;

		void  setupNodeFromDir( int type , Dir dirL )
		{
			nodeType = type;
			linkDirL = dirL;
			node = &surface->nodes[ nodeType ][ FDir::NeighborIndex( faceDirL , linkDirL ) ];
		}
		void  setupNodeFromIndex( int type , int idx )
		{
			nodeType = type;
			linkDirL = FDir::Neighbor( faceDirL , idx );
			node = &surface->nodes[ nodeType ][ idx ];
		}
	};


	World::World() 
		:mMapOffset( 0 , 0 , 0 )
		,mIdxParallaxDir(0)
	{
		mParallaxOffset = FDir::ParallaxOffset( mIdxParallaxDir );
		mFreeBlockId = 0;
		mUpdateCount = 0;
	}

	World::~World()
	{
		cleanup( true );
	}

	void World::init( Vec3i const& mapSize )
	{
		mMapSize = mapSize;
		mBlocks.push_back( nullptr );
		int num = mMapSize.x * mMapSize.y * mMapSize.z;
		mBlockMap.resize( num , 0 );
	}

	void World::cleanup( bool beDestroy )
	{
		if ( beDestroy )
		{
			for( int i = 0 ; i < mBlocks.size() ; ++i )
				delete mBlocks[i];
			mBlocks.clear();
		}
		else
		{
			mFreeBlockId = 0;
			for( int i = 1 ; i < mBlocks.size() ; ++i )
			{
				Block* block = mBlocks[i];
				if ( block->id > 0 )
				{
					block->group->remove( *block );
					//block->mapHook.unlink();
					removeNavNode( *block );
				}
				block->id = mFreeBlockId;
				mFreeBlockId = -i;
			}

			std::fill( mBlockMap.begin() , mBlockMap.end() , 0 );
			mUpdateCount = -1;
		}

		for(auto* group : mGroups)
		{
			delete group;
		}
		mGroups.clear();
	}

	bool World::putActor(Actor& actor , Vec3i const& pos , Dir faceDir )
	{
		int blockId = getBlock( pos );
		if ( blockId == 0 )
			return false;
		setActorBlock( actor , blockId , faceDir );
		return true;
	}

	void World::removeMap( ObjectGroup& group )
	{
		for (ObjectGroup* child : group.children)
		{
			removeMap( *child );
		}

		for (Block* block : group.blocks)
		{
			//#TODO:map only 1 block
			if ( getBlock( block->pos ) == block->id )
				setBlock( block->pos , 0 );
		}

	}

	void World::updateMap( ObjectGroup& group )
	{
		for (ObjectGroup* child : group.children)
		{
			updateMap( *child );
		}

		for (Block* block : group.blocks)
		{
			setBlock( block->pos , block->id );
		}
	}

	void World::prevEditBlock( Block& block )
	{
		removeNavNode( block );
	}

	void World::postEditBlock( Block& block )
	{
		increaseUpdateCount();
		updateNeighborNavNode( block.pos );
		updateNavNode( block );
	}

	Block* World::createBlock( Vec3i const& pos , 
		int idMesh , SurfaceDef surfaceDef[] , bool updateNav  , 
		ObjectGroup* group , Dir dirZ , Dir dirX  )
	{
		assert( FDir::Axis( dirZ ) != FDir::Axis( dirX ) );
		if ( getBlock( pos ) )
			return nullptr;

		int id;
		if ( mFreeBlockId )
		{
			id = -mFreeBlockId;
			mFreeBlockId = mBlocks[id]->id;
		}
		else
		{
			id = mBlocks.size();
			mBlocks.push_back( new Block );
		}
		Block* block = mBlocks[ id ];
		block->id = id;
		block->rotation.set( dirX , dirZ );
		block->pos = pos;

		for( int i = 0 ; i < BLOCK_FACE_NUM ; ++i )
		{
			block->surfaces[i].func = surfaceDef[i].func;
			block->surfaces[i].meta = surfaceDef[i].meta;
		}
		block->idMesh = idMesh;
		block->updateCount = 0;
		if ( group == nullptr )
			group = &mRootGroup;

		group->add( *block );

		setBlock( pos , block->id );

		if ( updateNav )
		{
			increaseUpdateCount();
			updateNeighborNavNode( block->pos );
			updateNavNode( *block );
		}

		return block;
	}

	bool World::destroyBlock( Vec3i const& pos )
	{
		if ( !checkPos( pos ) )
			return false;

		int idx = getBlockIndex( pos );
		int id = mBlockMap[ idx ];
		if ( id == 0 )
			return false;

		mBlockMap[ idx ] = 0;
		destroyBlockInternal( mBlocks[id] );
		return true;
	}

	void World::destroyBlock( Block* block )
	{
		assert( block && block->id > 0 );
		int idx = getBlockIndex( block->pos );
		//#TODO:map only 1 block
		if ( mBlockMap[ idx ] == block->id )
		{
			mBlockMap[idx] = 0;
		}
		destroyBlockInternal( block );
	}

	void World::destroyBlockInternal( Block* block )
	{
		assert( block && block->id > 0 );
		if ( block->group )
			block->group->remove( *block );

		int id = block->id;
		block->id = mFreeBlockId;
		mFreeBlockId = -id;

		removeNavNode( *block );

		increaseUpdateCount();
		updateNeighborNavNode( block->pos );
	}

	void World::setActorBlock( Actor& actor , int blockId , Dir faceDir , bool bUpdateRender )
	{
		Block* block = mBlocks[ blockId ];
		if ( actor.actBlockId )
			mBlocks[ actor.actBlockId ]->group->remove( actor );
		actor.actBlockId = blockId;
		actor.pos = block->pos;
		actor.actFaceDir = faceDir;
		if ( bUpdateRender )
		{
			actor.renderPos = Vec3f( block->pos ) + 0.5 * FDir::OffsetF( faceDir );
		}
		block->group->add( actor );
	}


	void World::removeActor( Actor& actor )
	{
		if ( actor.actBlockId )
		{
			actor.group->remove( actor );
			actor.actBlockId = 0;
		}
	}

	void World::updateNavNode_R( ObjectGroup& group )
	{
		updateBlockNavNode( group );
		for(ObjectGroup* child : group.children)
		{
			updateNavNode_R( *child );
		}
	}

	void World::updateBlockNavNode( ObjectGroup& group )
	{
		for(Block* block : group.blocks)
		{
			updateNavNode( *block );
		}
	}

	void World::connectBlockNavNode( Block& block , Dir dirL , Dir linkDirL )
	{
		BlockSurface& surface = block.getLocalFace( dirL );
		int idx = FDir::NeighborIndex( dirL , linkDirL );
		NavNode& node = surface.nodes[ NODE_DIRECT ][ idx ];
		if ( node.link )
			return;

		BlockSurface& destSurface = block.getLocalFace( linkDirL );
		int idxDest = FDir::NeighborIndex( linkDirL , dirL );
		node.connect( destSurface.nodes[ NODE_DIRECT ][ idxDest ] );
	}


	class ActionNavFunc
	{
	public:
		virtual int precessObstacleBlock( NavBuildContext const& context , Block* destBlock , BlockSurface& destSurface , Dir destFaceDirL ) = 0;
		virtual int precessNeighborBlock( NavBuildContext const& context , Block* destBlock , BlockSurface& destSurface , Dir destFaceDirL ) = 0;
	};

	class MoveActionNavFunc : public ActionNavFunc
	{
	public:
		int precessObstacleBlock( NavBuildContext const& context , Block* destBlock , BlockSurface& destSurface , Dir destFaceDirL )
		{
			switch ( destSurface.func )
			{
			case NFT_PLANE_LADDER:
			case NFT_LADDER:
				{
					Dir destLinkDirL = destBlock->rotation.toLocal( FDir::Inverse( context.faceDir ) );
					int idxLink = FDir::NeighborIndex( destFaceDirL , destLinkDirL );
					if ( destSurface.meta == ( idxLink / 2 ) )
					{
						NavNode& linkNode = destSurface.nodes[ context.nodeType ][ idxLink ];
						linkNode.checkDisconnect();
						context.node->checkDisconnect();
						context.node->connect( linkNode );
						return 1;
					}
				}
				return -1;
			case NFT_SUPER_PLANE:
			case NFT_ROTATOR_NC:
			case NFT_STAIR:
				{
					Dir destUpDirL = destBlock->rotation.toLocal( context.faceDir );
					if ( destSurface.meta == destUpDirL )
					{
						int idx = FDir::NeighborIndex( destFaceDirL , FDir::Inverse( destUpDirL ) );
						context.node->connect( destSurface.nodes[ context.nodeType ][idx] );
						return 1;
					}
				}
				return -1;
			case NFT_PLANE:
				if ( context.surface->func == NFT_STAIR )
				{
					if ( context.block->rotation.toWorld( Dir( context.surface->meta ) ) == destBlock->rotation.toWorld( destFaceDirL ) )
					{
						Dir destLinkDirL = destBlock->rotation.toLocal( FDir::Inverse( context.faceDir ) );
						int idx = FDir::NeighborIndex( destFaceDirL , destLinkDirL );
						context.node->connect( destSurface.nodes[ context.nodeType ][idx] );
						return 1;
					}
				}
				return -1;
			case NFT_BLOCK:
			default:
				return -1;	
			}
			return 0;
		}

		int precessNeighborBlock( NavBuildContext const& context , Block* destBlock , BlockSurface& destSurface , Dir destFaceDirL )
		{
			switch( destSurface.func )
			{
			case NFT_PLANE:
			case NFT_PLANE_LADDER:
			case NFT_SUPER_PLANE:
				{
					Dir destNDirL = destBlock->rotation.toLocal( FDir::Inverse( context.linkDir ) );
					int idx = FDir::NeighborIndex( destFaceDirL , destNDirL );
					NavNode& linkNode = destSurface.nodes[ context.nodeType ][ idx ];
					context.node->connect( linkNode );
					return 1;
				}
				break;
			case NFT_ROTATOR_C:
				{
					Dir linkDirLD = destBlock->rotation.toLocal( context.linkDir );
					if ( destSurface.meta == linkDirLD )
					{
						int idx = FDir::NeighborIndex( destFaceDirL , FDir::Inverse( linkDirLD ) );
						context.node->connect( destSurface.nodes[ context.nodeType ][idx] );
						return 1;
					}
				}
				return -1;
			}
			return 0;
		}
	};

	class ClimbActionNavFunc : public ActionNavFunc
	{
	public:
		int precessObstacleBlock( NavBuildContext const& context , Block* destBlock , BlockSurface& destSurface , Dir destFaceDirL )
		{
			switch ( destSurface.func )
			{

			case NFT_PLANE_LADDER:
			case NFT_LADDER:
				return -1;
#if 0
			case NFT_STAIR:

				Dir destUpDirL = destBlock->rotation.toLocal( context.faceDir );
				if ( destSurface.meta == destUpDirL )
				{
					int idx = FDir::NeighborIndex( destFaceDirL , invertDir( destUpDirL ) );
					context.node->connect( destSurface.node[ context.nodeType ][idx] , context.block->group == destBlock->group );
					return 1;
				}
				return -1;
			case NFT_PLANE:
			case NFT_BLOCK:
				return -1;	
#endif		
			}
			return 0;
		}

		int precessNeighborBlock( NavBuildContext const& context , Block* destBlock , BlockSurface& destSurface , Dir destFaceDirL )
		{
			switch( destSurface.func )
			{
			case NFT_LADDER:
			case NFT_PLANE_LADDER:
				{
					Dir destNDirL = destBlock->rotation.toLocal( FDir::Inverse( context.linkDir ) );
					int idx = FDir::NeighborIndex( destFaceDirL , destNDirL );
					NavNode& linkNode = destSurface.nodes[ context.nodeType ][ idx ];
					context.node->connect( linkNode );
					return 1;
				}
				break;
			}
			return 0;
		}
	};


	class RotateActionNavFunc : public ActionNavFunc
	{
	public:
		RotateActionNavFunc( bool beConvex ):beConvex( beConvex ){}

		int precessObstacleBlock( NavBuildContext const& context , Block* destBlock , BlockSurface& destSurface , Dir destFaceDirL )
		{
			if ( beConvex )
			{
				switch( destSurface.func )
				{

				}
			}
			else
			{
				switch( destSurface.func )
				{

				}
			}
			return 0;
		}

		int precessNeighborBlock( NavBuildContext const& context , Block* destBlock , BlockSurface& destSurface , Dir destFaceDirL )
		{
			if ( beConvex )
			{
				switch( destSurface.func )
				{

				}
			}
			else
			{
				switch( destSurface.func )
				{
				case NFT_PLANE:
				case NFT_PLANE_LADDER:
					return 0;
				}
			}
			return 0;
		}

		bool beConvex;
	};

	void World::updateNavNode( Block& block )
	{
		//#TODO:map only 1 block
		if ( getBlock( block.pos ) != block.id )
			return;

		//check parallax block Obstacle 
		Vec3i blockPosBlockObstacle;
		int blockIdBlockObstacle = getTopParallaxBlock( block.pos , blockPosBlockObstacle );
		bool bParallaxTest = ( blockIdBlockObstacle == block.id );
		if ( bParallaxTest )
		{
			//remove Face Nav Link if Find Obstacle
			Vec3i tempPos = blockPosBlockObstacle;
			while( 1 )
			{
				tempPos -= mParallaxOffset;
				if ( !checkPos( tempPos ) )
					break;
				int id = getBlock( tempPos );
				if ( id )
				{
					for( int i = 0 ; i < BLOCK_FACE_NUM ; ++i )
					{
						BlockSurface& surface = mBlocks[id]->surfaces[i];
						for( int idx = 0 ; idx < FACE_NAV_LINK_NUM ; ++idx )
						{
							surface.nodes[ NODE_PARALLAX ][idx].checkDisconnect();
						}
					}
					break;
				}
			}
		}

		for( int i = 0 ; i < BLOCK_FACE_NUM ; ++i )
		{
			updateSurfaceNavNode( block , Dir(i) , bParallaxTest );
		}
	}

	void World::updateSurfaceNavNode( Block& block , Dir faceDirL , bool bParallaxTest )
	{
		NavBuildContext context;
		context.bParallaxTest = bParallaxTest;
		context.block    = &block;
		context.faceDirL = faceDirL;
		context.faceDir  = block.rotation.toWorld( context.faceDirL );

		assert( context.faceDirL == block.rotation.toLocal( context.faceDir ) );
		context.surface = &block.getLocalFace( context.faceDirL );

		if ( context.surface->func == NFT_NULL )
			return;

		context.faceOffset = FDir::Offset( context.faceDir );

		Vec3i posTest = block.pos + context.faceOffset;

		int destBlockId = getBlockCheck( posTest );
		if ( destBlockId )
		{

			Block* destBlock = mBlocks[ destBlockId ];
			Dir destFaceDirL = destBlock->rotation.toLocal( FDir::Inverse( context.faceDir ) );
			BlockSurface& destSurface = destBlock->getLocalFace( destFaceDirL );
			if ( destSurface.func != NFT_NULL )
			{
				for( int idx = 0 ; idx < 4 ; ++idx )
				{
					destSurface.nodes[ NODE_DIRECT ][idx].checkDisconnect();
				}
				return;
			}
			else
			{
				//#TODO

				return;
			}
		}

		if ( bParallaxTest )
		{
			if ( context.faceOffset.dot( mParallaxOffset ) > 0 )
			{
				//check parallax face Obstacle 
				Vec3i blockPosFaceObstacle;
				int blockIdFaceObstacle = getTopParallaxBlock( posTest , blockPosFaceObstacle );
				if ( blockIdFaceObstacle != 0 && comparePosFromView( context.faceDir , block.pos , blockPosFaceObstacle ) < 0 )
				{
					context.bParallaxTest = false;
				}
			}
		}
		buildSurfaceNavNode(context);
	}

	void World::buildSurfaceNavNode(NavBuildContext& context)
	{
		Block& block = *context.block;
		switch( context.surface->func )
		{
		case NFT_PLANE:
			for( int idx = 0 ; idx < 4 ; ++idx )
			{
				buildNavLinkFromIndex(context, MoveActionNavFunc(), idx);
			}
			break;
		case NFT_SUPER_PLANE:
			for( int idx = 0 ; idx < 4 ; ++idx )
			{
				if ( buildNavLinkFromIndex(context, MoveActionNavFunc(), idx) == 0 )
				{
					connectBlockNavNode(block, context.faceDirL, context.linkDirL);
				}
			}
			break;
		case NFT_ROTATOR_C:
		case NFT_ROTATOR_NC:
			{
				Dir dirL = Dir( context.surface->meta );
				connectBlockNavNode(block, context.faceDirL, dirL );
				buildNavLinkFromDir(context , RotateActionNavFunc( false ), FDir::Inverse( dirL ));
			}
			break;
		case NFT_STAIR:
			{
				Dir dirL = Dir( context.surface->meta );
				connectBlockNavNode(block, context.faceDirL, dirL );
				buildNavLinkFromDir(context, MoveActionNavFunc(), FDir::Inverse( dirL ));
			}
			break;
		case NFT_LADDER:
			for( int i = 0 ; i < 2 ; ++i )
			{
				int idx = 2 * context.surface->meta + i;
				if ( buildNavLinkFromIndex(context, ClimbActionNavFunc(), idx) == 0 )
				{
					BlockSurface& surfaceN = block.getLocalFace( context.linkDirL );

					if ( surfaceN.func == NFT_PLANE || 
						 surfaceN.func == NFT_PLANE_LADDER )
					{
						connectBlockNavNode(block, context.faceDirL, context.linkDirL);
					}
				}
			}
			break;
		}
	}

	int World::buildNavLinkFromIndex(NavBuildContext& context, ActionNavFunc& func, int idx)
	{
		Block& block = *context.block;
		context.setupNodeFromIndex(NODE_DIRECT, idx );
		context.linkDir = block.rotation.toWorld( context.linkDirL );
		int result = buildNeighborNavLink(context, func);
		if ( context.bParallaxTest )
		{
			context.setupNodeFromIndex(NODE_PARALLAX, idx );
			buildParallaxNavLink(context, func);
		}
		return result;
	}

	int World::buildNavLinkFromDir(NavBuildContext& context, ActionNavFunc& func, Dir dirL)
	{
		Block& block = *context.block;
		context.setupNodeFromDir(NODE_DIRECT, dirL);
		context.linkDir = block.rotation.toWorld(context.linkDirL);
		int result = buildNeighborNavLink(context, func);
		if ( context.bParallaxTest )
		{
			context.setupNodeFromDir(NODE_PARALLAX, dirL);
			buildParallaxNavLink(context, func);
		}
		return result;
	}

	int World::buildNeighborNavLink(NavBuildContext const& context, ActionNavFunc& func)
	{
		Block& block = *context.block;

		if ( context.node->link )
			return 2;

		Vec3i linkOffset = FDir::Offset( context.linkDir );
		//       | faceDir
		//       |___
		//       | O |
		//   ___ |___|___ linkDir
		//   | x | L |
		//   |___|___|

		Vec3i destblockPos;
		int   destBlockId;
		// Block O
		Vec3i destPos = block.pos + linkOffset + context.faceOffset;
		destBlockId = getBlockCheck( destPos );
		if ( destBlockId )
		{
			Block* destBlock = mBlocks[ destBlockId ];
			Dir destFaceDirL = destBlock->rotation.toLocal( FDir::Inverse( context.linkDir ) );
			BlockSurface& destSurface = destBlock->getLocalFace( destFaceDirL );
			int ret = func.precessObstacleBlock( context , destBlock , destSurface , destFaceDirL );
			if ( ret )
			{
				return ret;
			}
		}

		// Block L
		destBlockId = getBlockCheck( block.pos + linkOffset );
		if ( destBlockId )
		{
			Block* destBlock = mBlocks[ destBlockId ];
			Dir destFaceDirL = destBlock->rotation.toLocal( context.faceDir );
			BlockSurface& destSurface = destBlock->getLocalFace( destFaceDirL );
			int ret = func.precessNeighborBlock( context , destBlock , destSurface , destFaceDirL );
			if ( ret )
			{
				return ret; 
			}
		}

		return 0;
	}

	int World::buildParallaxNavLink(NavBuildContext const& context, ActionNavFunc& func)
	{
		Block& block = *context.block;
		if ( context.node->link )
			return 2;

		Vec3i linkOffset = FDir::Offset( context.linkDir );
		//       | faceDir
		//       |___
		//       | O |
		//   ___ |___|___ linkDir
		//   | x | L |
		//   |___|___|


		// Block O
		Vec3i destPos = block.pos + linkOffset + context.faceOffset;

		Vec3i linkBlockPos = block.pos + linkOffset;
		int idLO = getBlockCheck( destPos );

		Vec3i destblockPos;
		int destBlockId = getTopParallaxBlock( destPos , destblockPos );
		int idL  = getBlockCheck( linkBlockPos );

		Vec3i tempBlockPos;
		int tempBlockId = getTopParallaxBlock( linkBlockPos , tempBlockPos );

		if ( destBlockId )
		{
			if ( destBlockId == idLO || 
				comparePosFromView( context.linkDir , block.pos , destblockPos ) >= 0 ||
				//check O not obstacle X and L
				( comparePosFromView( context.faceDir , destblockPos , block.pos ) <= 0 &&
				  comparePosFromView( context.faceDir , destblockPos , tempBlockPos ) <= 0 ) )
			{
				destBlockId = 0;
			}
		}

		if ( tempBlockId == idL )
		{
			tempBlockId = 0;
		}

		if ( destBlockId )
		{
			Block* destBlock = mBlocks[ destBlockId ];
			Dir destFaceDirL = destBlock->rotation.toLocal( FDir::Inverse( context.linkDir ) );
			BlockSurface& destSurface = destBlock->getLocalFace( destFaceDirL );
			int ret = func.precessObstacleBlock( context , destBlock , destSurface , destFaceDirL );
			if ( ret )
				return ret; 
		}

		// Block L
		int specialTest = 0;

		if ( tempBlockId )
		{
			destBlockId = tempBlockId;
			destblockPos = tempBlockPos;
			specialTest = comparePosFromView( context.linkDir , block.pos , destblockPos );
			if ( specialTest >= 0 )
			{
				//check 
				BlockSurface& surf = ( FDir::IsPositive( context.linkDir ) ) ? 
					context.block->getFace( context.linkDir ) :
				mBlocks[ destBlockId ]->getFace( FDir::Inverse( context.linkDir ) );
				if ( surf.func != NFT_PASS_VIEW )
				{
					destBlockId = 0;
					specialTest = 0;
				}
			}
		}
		if ( destBlockId )
		{
			Block* destBlock = mBlocks[ destBlockId ];
			Dir destFaceDirL = destBlock->rotation.toLocal( context.faceDir );
			BlockSurface& destSurface = destBlock->getLocalFace( destFaceDirL );
			int ret = func.precessNeighborBlock( context , destBlock , destSurface , destFaceDirL );
			if ( ret )
			{
				if ( ret == 1 && specialTest )
				{
					NavNode* node = ( specialTest == 1 ) ? context.node : context.node->link;
					node ->flag |= NavNode::eFixPos;
					mFixNodes.push_back( node );
				}
				return ret; 
			}
		}
		return 0;
	}

	int World::getTopParallaxBlock(Vec3i const& pos , Vec3i const& parallaxOffset , Vec3i& outPos )
	{
		Vec3i testPos;

		Vec3i posToMap = pos - mMapOffset;

		int diff[3];
		diff[0] = ( parallaxOffset.x > 0 ) ? ( mMapSize.x - 1 - posToMap.x ) : ( posToMap.x );
		diff[1] = ( parallaxOffset.y > 0 ) ? ( mMapSize.y - 1 - posToMap.y ) : ( posToMap.y );
		diff[2] = ( parallaxOffset.z > 0 ) ? ( mMapSize.z - 1 - posToMap.z ) : ( posToMap.z );

		//int idxMin = ( diff[0] < diff[1] )
		int minOff = std::min( diff[0], std::min( diff[1] , diff[2] ) );
		testPos = pos + ( minOff ) * parallaxOffset;

		for(;;)
		{
			if ( !checkPos( testPos ) )
				break;

			int blockId = getBlock( testPos ); 
			if ( blockId )
			{
				outPos = testPos;
				return blockId;
			}

			testPos -= parallaxOffset;

		}
		outPos = testPos;
		return 0;
	}

	int World::comparePosFromView( Dir dir , Vec3i const& posA , Vec3i const& posB )
	{
		int axis = FDir::Axis( dir );
		if ( posA[axis] == posB[axis] )
			return 0;
		int result = ( FDir::IsPositive( dir ) ) ? 1 : -1;
		result *= (  posA[axis] > posB[ axis ] ) ? 1 : -2;
		return result;
	}

	void World::removeDynamicNavNode_R( ObjectGroup& group )
	{
		for( BlockList::iterator iter = group.blocks.begin() , itEnd = group.blocks.end();
			iter != itEnd ; ++iter )
		{
			Block* block = *iter;

			for( int i = 0 ; i < BLOCK_FACE_NUM ; ++i )
			{
				BlockSurface& surface = block->getLocalFace( Dir(i) );
				for( int n = 0 ; n < FACE_NAV_LINK_NUM ; ++n )
				{
					if ( surface.nodes[ NODE_DIRECT ][n].flag & NavNode::eStatic )
						surface.nodes[ NODE_DIRECT ][n].checkDisconnect();
					
					surface.nodes[ NODE_PARALLAX ][n].checkDisconnect();
				}
			}
		}

		for(ObjectGroup* child : group.children)
		{
			removeDynamicNavNode_R( *child );
		}
	}

	void World::removeParallaxNavNode_R( ObjectGroup& group )
	{
		for(Block* block : group.blocks)
		{
			for( int i = 0 ; i < BLOCK_FACE_NUM ; ++i )
			{
				BlockSurface& surface = block->getLocalFace( Dir(i) );
				for( int n = 0 ; n < FACE_NAV_LINK_NUM ; ++n )
				{
					surface.nodes[ NODE_PARALLAX ][n].checkDisconnect();
				}
			}
		}

		for(ObjectGroup* child : group.children)
		{
			removeParallaxNavNode_R( *child );
		}
	}

	void World::action( Actor& actor , int idxNode )
	{
		if ( actor.actBlockId == 0 )
			return;
		Block* block = mBlocks[ actor.actBlockId ];
		BlockSurface& surface = block->getFace( actor.actFaceDir );

		NavNode* node = &surface.nodes[ NODE_DIRECT ][ idxNode ];
		if ( node->link == nullptr )
		{
			node = &surface.nodes[ NODE_PARALLAX ][ idxNode ];
			if ( node->link == nullptr )
				return;
		}

		NavNode* destNode = node->link;
		BlockSurface* destSurface = destNode->getSurface();
		Block* destBlock = destSurface->getBlock();
		Dir dirFaceL = Block::LocalDir( *destSurface );
		setActorBlock(actor, destBlock->id, destBlock->rotation.toWorld( dirFaceL ));
	}

	void World::removeNavNode(Block& block)
	{
		for( int i = 0 ; i < BLOCK_FACE_NUM ; ++i )
		{
			BlockSurface& surface = block.surfaces[i];
			for( int idx = 0 ; idx < FACE_NAV_LINK_NUM ; ++idx )
			{
				surface.nodes[ NODE_DIRECT ][idx].checkDisconnect();
				surface.nodes[ NODE_PARALLAX ][idx].checkDisconnect();
			}
		}
	}

	void World::updateNeighborNavNode( Vec3i const& pos , ObjectGroup* skipGroup )
	{
		Block* updateBlock[ 30 ];
		int numBlock = 0;

		//#TODO: optimize code
		for( int i = -1 ; i <= 1 ; ++i )
		{
			for( int j = -1 ; j <= 1 ; ++j )
			{
				for( int k = -1 ; k <= 1 ; ++k )
				{
					if ( i == 0 && j == 0 && k == 0 )
						continue;

					Vec3i nPos = pos + Vec3i(i,j,k);
					int idNBlock = getBlockCheck( nPos );
					if ( idNBlock == 0 )
						continue;

					Block* block = mBlocks[ idNBlock ];

					if ( !checkUpdateCount( *block ) )
						continue;

					ObjectGroup* group = block->group;
					if ( group == skipGroup )
						continue;

					removeNavNode( *block );
					updateBlock[ numBlock ] = block;
					++numBlock;
				}
			}
		}

		for( int i = 0 ; i < numBlock ; ++i )
		{
			updateNavNode( *updateBlock[i] );
		}
	}

	void World::increaseUpdateCount()
	{
		++mUpdateCount;
		if ( mUpdateCount == 0 )
		{
			for( int i = 1 ; i < mBlocks.size() ; ++i )
			{
				mBlocks[i]->updateCount = 0;
			}
			++mUpdateCount;
		}
		mSkipCount = 0;
	}

	bool World::checkUpdateCount( Block& block )
	{
		if ( mUpdateCount == block.updateCount )
		{
			++mSkipCount;
			return false;
		}
		block.updateCount = mUpdateCount;
		return true;
	}

	ObjectGroup* World::createGroup(ObjectGroup* parent /*= nullptr */)
	{
		ObjectGroup* group = new ObjectGroup;
		mGroups.push_back( group );
		if ( parent == nullptr )
			parent = &mRootGroup;
		parent->add( *group );
		group->idx = mGroups.size();
		return group;
	}

	struct DeleteFunc
	{
		DeleteFunc( World& world ):world(world){}
		void operator()( Block* obj )
		{ 
			world.destroyBlock( obj ); 
		}
		void operator()( ObjectGroup* obj )
		{ 
			obj->removeAll( *this );
		}
		template< class T >
		void operator()( T* obj ){ }
		World& world;
	};

	struct ModifyGroupFunc
	{
		ModifyGroupFunc( ObjectGroup& group ):group(group){}
		template< class T >
		void operator()( T* obj )
		{
			group.add( *obj );
		}
		ObjectGroup& group;
	};

	void World::destroyGroup( ObjectGroup* group , bool bDeleteObj )
	{
		assert( group && std::find( mGroups.begin() , mGroups.end() , group ) != mGroups.end() );
		if ( bDeleteObj )
		{
			group->removeAll( DeleteFunc( *this ) );
		}
		else
		{
			group->removeAll( ModifyGroupFunc( mRootGroup ) );
		}
		mGroups.erase( std::find( mGroups.begin() , mGroups.end() , group ) );
	}

	void World::refreshFixNode()
	{
		int num = mFixNodes.size();
		for( int i = 0; i != num ; )
		{
			NavNode* node = mFixNodes[i];
			if ( ( node->flag & NavNode::eFixPos ) == 0 )
			{
				--num;
				std::swap( mFixNodes[i] , mFixNodes[num] );
				continue;
			}
			++i;
		}
		if ( num != mFixNodes.size() )
			mFixNodes.resize( num );
	}

	int World::calcFixParallaxPosOffset( Block& block , Dir face )
	{
		int const MaxFixDist = 4;
		Vec3i temp;
		if ( getTopParallaxBlock( block.pos , temp ) != block.id )
			return 0;

		int axis = FDir::Axis( face );
		Vec3i posP = calcParallaxPos( block.pos , axis );

		int maxOffset = 0;
		int num = mFixNodes.size();
		for( int i = 0; i != num ; ++i )
		{
			NavNode* node = mFixNodes[i];

			if (  face != Block::WorldDir( *node->getSurface() ) )
				continue;

			Block* sBlock = node->getSurface()->getBlock();

			if ( comparePosFromView( face , block.pos , sBlock->pos ) >= 0 )
				continue;

			Vec3i posPS = calcParallaxPos( sBlock->pos , axis );
			Vec3i diff = posP - posPS;
			if ( abs(diff.x) + abs(diff.y) + abs(diff.z) > MaxFixDist )
				continue;

			int offset = std::abs( sBlock->pos[axis] - block.pos[axis] );
			if ( maxOffset > offset )
				continue;
			maxOffset = offset;
		}

		return maxOffset;
	}

}//namespace MV