#include "MVPathFinder.h"

#include "Algo/AStar.h"

namespace MV
{
	struct AStarNode : AStar::NodeBaseT< AStarNode , FindState , int >
	{



	};

	class AStarFinder : public AStar::AStarT< AStarFinder , AStarNode >
	{
	public:
		typedef FindState StateType;
		typedef AStarNode NodeType;
		ScoreType calcHeuristic( StateType& state )
		{  
			return calcDistance( state , mGoal );
		}
		ScoreType calcDistance( StateType& a, StateType& b)
		{  
			Vec3i offset = a.block->pos - b.block->pos;
			int factor = ( a.faceDirL == b.faceDirL ) ? 1 : 2;
			return factor * ( abs( offset.x ) + abs( offset.y ) + abs( offset.z ) ); 
		}
		bool      isEqual(StateType& a ,StateType& b )
		{  
			return a.faceDirL == b.faceDirL && a.block->pos == b.block->pos;
		}
		bool      isGoal (StateType& state )
		{ 
			return isEqual( mGoal , state );
		}

		//  call addSreachNode for all possible next state
		void      processNeighborNode( NodeType& aNode )
		{  
			Block* block = aNode.state.block;

			BlockSurface& surface = block->getLocalFace( aNode.state.faceDirL );

			for( int idxNode = 0 ; idxNode < 4 ; ++idxNode )
			{
				for( int n = 0 ; n < 2 ; ++n )
				{
					NavNode* node = &surface.nodes[n][ idxNode ];
					if ( node->link == NULL )
						continue;
					if ( aNode.state.prevBlockNode == node->link )
						continue;

					NavNode* destNode = node->link;
					BlockSurface* destSurface = destNode->getSurface();

					BlockSurface* srcSurface = node->getSurface(); 

					FindState state;
					state.isPrevParallax = ( n == 1 ) ;
					state.upDir = aNode.state.upDir;

					if ( srcSurface->fun != destSurface->fun )
					{
						switch( srcSurface->fun )
						{
						case NFT_LADDER :
							{
								Dir destFaceDir = Block::WorldDir( *destSurface );
								if ( destSurface->fun != NFT_PLANE_LADDER ||
									destFaceDir != Block::WorldDir( *srcSurface ) )
								{
									if ( destFaceDir != aNode.state.upDir )
										continue;
								}
							}
							break;
						case NFT_STAIR:
							{
								if(  Block::WorldDir( *destSurface ) != aNode.state.upDir )
										continue;
							}
							break;
						case NFT_ROTATOR_C:
							state.upDir = Block::WorldDir( *destSurface );
							break;
						case NFT_ROTATOR_NC:
							state.upDir = FDir::Inverse( Block::WorldDir( *destSurface ) );
							break;
						}
					}

					state.block = destSurface->block;
					state.prevBlockNode  = node;
					state.faceDirL = Block::LocalDir( *destSurface );
					addSreachNode( state , aNode , 1 );
				}
			}
		}

		FindState mGoal;
	};

	AStarFinder gFinderImpl;

	bool PathFinder::find(FindState const& from , FindState const& to )
	{
		gFinderImpl.mGoal = to;
		if ( !gFinderImpl.sreach( from ) )
			return false;
		return true;
	}

	static void addPathNode( std::vector< PathNode >& pathNodes  , PointVec&  points ,  
		FindState& state , FindState* nextState , Dir inDir , Dir outDir , bool isParallax  )
	{
		PathNode pathNode;

		pathNode.block = state.block;
		pathNode.upDir = state.upDir;
		pathNode.faceDir = pathNode.block->rotation.toWorld( state.faceDirL );
		
		//pathNode.frontDir = outDir;

		BlockSurface* surf = &state.block->getLocalFace( state.faceDirL );

		Vec3f faceOffset = 0.5 * FDir::OffsetF( pathNode.faceDir );
		Vec3f faceCenterPos = Vec3f( state.block->pos ) + faceOffset;

		switch( surf->fun )
		{
		case NFT_STAIR:
			points.push_back( Vec3f( state.block->pos ) );
			pathNode.numPos = 1;
			pathNode.moveFactor = 1.4f;
			break;
		case NFT_ROTATOR_C:
		case NFT_ROTATOR_NC:
		default:
			pathNode.moveFactor = 1.0f;
			points.push_back( faceCenterPos );
			pathNode.numPos =  1;
		}

		if ( nextState == NULL )
		{
			pathNode.parallaxDir = 0;
			pathNode.link = NULL;
			pathNode.moveFactor *= 0.5;
		}
		else
		{
			Vec3f outOffset = 0.5 *  FDir::OffsetF( outDir );

			++pathNode.numPos;
			pathNode.link = nextState->prevBlockNode;
			Vec3f outPos;
			if ( isParallax )
			{
				BlockSurface* destSurf = pathNode.link->link->getSurface();
				Block* destBlock = destSurf->block;
				Vec3f destPos = Vec3f( destBlock->pos ) + 0.5 * ( FDir::OffsetF( Block::WorldDir( *destSurf ) ) ) - outOffset;

				points.push_back( faceCenterPos + 0.5 * FDir::OffsetF( outDir ) );
				points.push_back( destPos );
				if ( faceOffset.dot( destPos - faceCenterPos ) > 0.0 )
				{
					pathNode.parallaxDir = 1;
				}
				else
				{
					pathNode.parallaxDir = -1;
				}
			}
			else
			{
				pathNode.parallaxDir = 0;
				points.push_back( faceCenterPos + 0.5 * FDir::OffsetF( outDir ) );
			}
		}

		pathNodes.push_back( pathNode );
	}

	void PathFinder::contructPath( Path& path , PointVec& points , World const& world )
	{
		path.mNodes.clear();

		AStarFinder::NodeType* aNode = gFinderImpl.getPath();
		AStarFinder::NodeType* aNodeNext; 

		Block*   prevBlock = NULL;
		Dir      prevDir;
		Dir      dir;
		Block*   block;
		FindState* state;
		FindState* nextState;

		{
			FindState& state = aNode->state;
			block = state.block;

			aNodeNext = aNode->child;
			if ( aNodeNext )
			{
				nextState = ( aNodeNext ) ? &aNodeNext->state : NULL;
				dir =  block->rotation.toWorld( FDir::Neighbor( state.faceDirL , nextState->prevBlockNode->getDirIndex() ) );
			}
			else
			{
				nextState = NULL;
				dir = Dir::X;
			}
			addPathNode( path.mNodes , points , state , nextState , dir , dir , false );
			
		}

		while( aNodeNext )
		{
			prevDir = dir;
			prevBlock = block;

			aNode = aNodeNext;
			FindState& state = aNode->state;
			block = state.block;
			aNodeNext = aNode->child;
			nextState = ( aNodeNext ) ? &aNodeNext->state : NULL;

			bool needAddNode = true;

			if ( nextState )
			{
				NavNode* node = nextState->prevBlockNode;
				BlockSurface* surf = node->getSurface();
				BlockSurface* nextSurf = node->link->getSurface();
				switch ( surf->fun )
				{
				case NFT_ROTATOR_C:
				case NFT_ROTATOR_NC:
				case NFT_STAIR:
					if ( surf->fun == nextSurf->fun && surf->block == nextSurf->block )
						needAddNode = false;
					break;
				}
			}

			if ( needAddNode )
			{
				dir = ( nextState ) ? block->rotation.toWorld( FDir::Neighbor( state.faceDirL , nextState->prevBlockNode->getDirIndex() ) ) : Dir::X;
				addPathNode( path.mNodes , points , state , nextState , prevDir , dir , ( nextState ) ? nextState->isPrevParallax : false );
			}
		}
	}

	void Navigator::update(float dt)
	{
		switch( mState )
		{
		case eDisconnect:
			{


			}
			break;
		case eRun:
			{
				while ( dt > mMoveTime )
				{
					dt -= mMoveTime;
					PathNode const& node = mPath.getNode( mIdxNode );
					
					++mNodePosCount;
					if ( mNodePosCount == node.numPos )
					{
						++mIdxNode;
						if ( mIdxNode == mPath.getNodeNum() )
						{
							mState = eFinish;
							return;
						}

						if ( node.link->link == NULL )
						{
							mState = eDisconnect;
							return;
						}
						if ( node.parallaxDir )
						{
							++mIdxNextPos;
						}

						setupNodeParam();
						setupActorOnNode();
					}

					mPrevPos = mMovePoints[ mIdxNextPos ];
					++mIdxNextPos;

					mMoveOffset = mMovePoints[ mIdxNextPos ] - mPrevPos;
					mMoveTime = mMoveDuration;

				}

				assert( mMoveTime >= dt );
				mMoveTime -= dt;
				mHost->renderPos = mPrevPos + ( 1 -  mMoveTime / mMoveDuration ) * mMoveOffset;
			}
			break;
		}
	}

	void Navigator::moveToPos( Block* block , Dir faceDir )
	{
		if ( mHost->actBlockId == 0 )
			return;

		if ( mState != eFinish && mState != eDisconnect )
			return;

		PathFinder pathFinder;
		FindState from , to;
		to.block     = block;
		to.faceDirL  = to.block->rotation.toLocal( faceDir );

		from.upDir = mHost->rotation[2];
		from.block = mWorld->getBlock( mHost->actBlockId );
		from.faceDirL = from.block->rotation.toLocal( mHost->actFaceDir );

		if ( !pathFinder.find( from , to ) )
			return;

		mMovePoints.clear();
		pathFinder.contructPath( mPath , mMovePoints , *mWorld );
		fixPathParallaxPosition( 0 );

		mState = eRun;
		mIdxNode = 0;
		mIdxNextPos = 0;
		setupNodeParam();
		mPrevPos = mMovePoints[ mIdxNextPos ];
		++mNodePosCount;
		++mIdxNextPos;
		if ( mIdxNextPos == mMovePoints.size() )
		{
			mHost->renderPos = mPrevPos;
			mState = eFinish;
			return;
		}
		mMoveOffset = mMovePoints[ mIdxNextPos ] - mPrevPos;
		mMoveTime = mMoveDuration;
	}

	void Navigator::fixPathParallaxPosition(int idxStart)
	{
		mWorld->refrechFixNode();

		int idx = idxStart;
		PathNode* prevNode = NULL;
		for( int i = 0 ; i < mPath.getNodeNum() ; ++i )
		{
			PathNode& node = mPath.getNode(i);
			node.fixPosOffset = mWorld->calcFixParallaxPosOffset( *node.block , node.faceDir );
			if ( node.fixPosOffset )
			{
				Vec3f fixOffset = Vec3f( node.fixPosOffset * mWorld->mParallaxOffset );
				if ( prevNode && prevNode->parallaxDir )
				{
					mMovePoints[idx] += fixOffset;
					++idx;
				}
				for( int n = 0 ; n < node.numPos ; ++n )
				{
					mMovePoints[idx] += fixOffset;
					++idx;
				}
			}
			else
			{
				if ( prevNode && prevNode->parallaxDir )
					++idx;
				idx += node.numPos;
			}

			prevNode = &node;
		}
	}

	void Navigator::setupNodeParam()
	{
		PathNode const& node = mPath.getNode( mIdxNode );
		mNodePosCount = 0;
		mMoveDuration = 0.3 * node.moveFactor / node.numPos;
	}

	void Navigator::setupActorOnNode()
	{
		PathNode const& node = mPath.getNode( mIdxNode );
		mWorld->setActorBlock( *mHost , node.block->id , node.faceDir , false);

		switch( node.block->getFace( node.faceDir ).fun )
		{
		case NFT_LADDER:
			mHost->actState = Actor::eActClimb;
			break;
		default:
			mHost->actState = Actor::eActMove;
		}

		//mHost->rotation.set( node.frontDir , node.upDir );
	}

}//namespace MV