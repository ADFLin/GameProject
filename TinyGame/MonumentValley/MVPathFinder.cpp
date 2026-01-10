#include "MVPathFinder.h"

#include "Algo/AStar.h"

namespace MV
{
	struct AStarNode : AStar::NodeBaseT< AStarNode , FindState , float >
	{
		AStarNode* child;

		BlockSurface& getSurface()
		{
			return state.block->getLocalFace(state.faceDirL);
		}
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
		bool      isGoal(StateType& state )
		{ 
			return isEqual( mGoal , state );
		}

		//  call addSearchNode for all possible next state
		void      processNeighborNode( NodeType& aNode )
		{  
			BlockSurface& surface = aNode.getSurface();

			for( int idxNode = 0 ; idxNode < FACE_NAV_LINK_NUM; ++idxNode )
			{
				for( int nodeType = 0 ; nodeType < NUM_NODE_TYPE; ++nodeType )
				{
					NavNode* node = &surface.nodes[nodeType][ idxNode ];
					if ( node->link == nullptr )
						continue;
					if ( aNode.state.prevBlockNode == node->link )
						continue;

					NavNode* destNode = node->link;
					BlockSurface* destSurface = destNode->getSurface();

					BlockSurface* srcSurface = node->getSurface(); 

					FindState state;
					state.bParallaxLink = nodeType == NODE_PARALLAX;
					state.upDir = aNode.state.upDir;

					if ( srcSurface->func != destSurface->func )
					{
						switch( srcSurface->func )
						{
						case NFT_LADDER :
							{
								Dir destFaceDir = Block::WorldDir( *destSurface );
								if ( destSurface->func != NFT_PLANE_LADDER ||
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

					state.block = destSurface->getBlock();
					state.prevBlockNode  = node;
					state.faceDirL = Block::LocalDir( *destSurface );
					addSearchNode( state , aNode , 1 );
				}
			}
		}

		SearchResult mSearchResult;
		FindState    mGoal;
	};

	AStarFinder GFinderImpl;

	bool PathFinder::find(FindState const& from , FindState const& to )
	{
		GFinderImpl.mGoal = to;
		if ( !GFinderImpl.search( from , GFinderImpl.mSearchResult) )
			return false;

		return true;
	}

	void AddPathNode( TArray< PathNode >& pathNodes, PointVec& points, FindState& state, FindState* nextState, Dir inDir, Dir outDir, bool bParallaxLink)
	{
		PathNode pathNode;

		pathNode.block = state.block;
		pathNode.upDir = state.upDir;
		pathNode.faceDir = pathNode.block->rotation.toWorld( state.faceDirL );
		pathNode.posCount = 0;
		pathNode.surface = &state.block->getLocalFace(state.faceDirL);
		//pathNode.frontDir = outDir;

		BlockSurface* surf = &state.block->getLocalFace(state.faceDirL);

		Vec3f faceOffset = 0.5 * FDir::OffsetF( pathNode.faceDir );
		Vec3f faceCenterPos = Vec3f( state.block->pos ) + faceOffset;

		static const float Sqrt2 = Math::Sqrt(2);

		switch( surf->func )
		{
		case NFT_STAIR:
			pathNode.moveFactor = Sqrt2;
			points.push_back( Vec3f( state.block->pos ) );
			pathNode.posCount += 1;
			break;
		case NFT_ROTATOR_C:
		case NFT_ROTATOR_NC:
		default:
			pathNode.moveFactor = 1.0f;
			points.push_back( faceCenterPos );
			pathNode.posCount += 1;
		}

		if ( nextState == nullptr )
		{
			pathNode.parallaxDir = 0;
			pathNode.link = nullptr;
			pathNode.moveFactor *= 0.5;
		}
		else
		{
			Vec3f outOffset = 0.5 *  FDir::OffsetF( outDir );

			pathNode.link = nextState->prevBlockNode;
			Vec3f outPos;
			if ( bParallaxLink )
			{
				BlockSurface* destSurf = pathNode.link->link->getSurface();
				Block* destBlock = destSurf->getBlock();
				Vec3f destPos = Vec3f( destBlock->pos ) + 0.5 * ( FDir::OffsetF( Block::WorldDir( *destSurf ) ) ) - outOffset;

				points.push_back( faceCenterPos + 0.5 * FDir::OffsetF( outDir ) );
				points.push_back( destPos );
				pathNode.posCount += 1;
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
				switch (surf->func)
				{
				case NFT_STAIR:
					if (state.block->pos != nextState->block->pos)
						points.push_back(faceCenterPos + outOffset);
					else
						points.push_back(faceCenterPos + outOffset - 2 * faceOffset);
					break;
				case NFT_ROTATOR_C:
				case NFT_ROTATOR_NC:
				default:
					pathNode.moveFactor = 1.0f;
					points.push_back(faceCenterPos + outOffset);
				}

				pathNode.posCount += 1;
			}
		}

		pathNodes.push_back( pathNode );
	}

	void PathFinder::buildPath(Path& path , PointVec& points , World const& world )
	{
		path.mNodes.clear();

		GFinderImpl.mSearchResult.globalNode->child = nullptr;
		GFinderImpl.constructPath(GFinderImpl.mSearchResult.globalNode,
			[](AStarFinder::NodeType* node)
			{
				if(node->parent)
				{
					node->parent->child = node;
				}
			}
		);

		AStarFinder::NodeType* aNode = GFinderImpl.mSearchResult.startNode;
		AStarFinder::NodeType* aNodeNext; 

		Block*   prevBlock = nullptr;
		Block*   block;
		Dir      prevDir;
		Dir      dir;

		FindState* state;
		FindState* nextState;

		{
			FindState& state = aNode->state;
			block = state.block;

			aNodeNext = aNode->child;
			if ( aNodeNext )
			{
				nextState = &aNodeNext->state;
				NavNode* node = nextState->prevBlockNode;
				dir =  block->rotation.toWorld( FDir::Neighbor( state.faceDirL , node->getDirIndex() ) );
			}
			else
			{
				nextState = nullptr;
				dir = Dir::X;
			}
			prevDir = dir;
			BlockSurface* surf = &aNode->getSurface();

			bool needAddNode = true;
			if (needAddNode)
			{
				AddPathNode(path.mNodes, points, state, nextState, prevDir, dir, false);
			}
		}

		while( aNodeNext )
		{
			prevDir = dir;
			prevBlock = block;

			aNode = aNodeNext;
			FindState& state = aNode->state;
			block = state.block;
			aNodeNext = aNode->child;
			nextState = ( aNodeNext ) ? &aNodeNext->state : nullptr;

			bool needAddNode = true;

			if ( nextState )
			{
				NavNode* node = nextState->prevBlockNode;
				BlockSurface* surf = node->getSurface();
				BlockSurface* nextSurf = node->link->getSurface();
				switch ( surf->func )
				{
				case NFT_ROTATOR_C:
				case NFT_ROTATOR_NC:
				case NFT_STAIR:
					if (Block::WorldDir(*surf) == dir || Block::WorldDir(*surf) == FDir::Inverse(dir))
						needAddNode = false;
					break;
				}
			}

			if ( needAddNode )
			{
				dir = ( nextState ) ? block->rotation.toWorld( FDir::Neighbor( state.faceDirL , nextState->prevBlockNode->getDirIndex() ) ) : Dir::X;
				AddPathNode( path.mNodes , points , state , nextState , prevDir, dir , ( nextState ) ? nextState->bParallaxLink : false );
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
					
					++mIndexNodePos;
					if ( mIndexNodePos == node.posCount )
					{
						++mIdxNode;
						if ( mIdxNode == mPath.getNodeNum() )
						{
							mState = eFinish;
							return;
						}

						if ( node.link->link == nullptr )
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
		pathFinder.buildPath( mPath , mMovePoints , *mWorld );
		fixPathParallaxPosition( 0 );

		initMove();
		return;
	}

	void Navigator::initMove()
	{
		mState = eRun;
		mIdxNode = 0;
		mIdxNextPos = 0;
		setupNodeParam();
		mPrevPos = mMovePoints[mIdxNextPos];
		++mIndexNodePos;
		++mIdxNextPos;
		if (mIdxNextPos == mMovePoints.size())
		{
			mHost->renderPos = mPrevPos;
			mState = eFinish;
			return;
		}
		mMoveOffset = mMovePoints[mIdxNextPos] - mPrevPos;
		mMoveTime = mMoveDuration;
	}

	void Navigator::moveReplay()
	{
		initMove();
		setupActorOnNode();
	}

	void Navigator::fixPathParallaxPosition(int idxStart)
	{
		mWorld->refreshFixNode();

		int idx = idxStart;
		PathNode* prevNode = nullptr;
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
				for( int n = 0 ; n < node.posCount ; ++n )
				{
					mMovePoints[idx] += fixOffset;
					++idx;
				}
			}
			else
			{
				if ( prevNode && prevNode->parallaxDir )
					++idx;
				idx += node.posCount;
			}

			prevNode = &node;
		}
	}

	void Navigator::setupNodeParam()
	{
		PathNode const& node = mPath.getNode( mIdxNode );
		mIndexNodePos = 0;
		mMoveDuration = 0.3 * node.moveFactor / node.posCount;
	}

	void Navigator::setupActorOnNode()
	{
		PathNode const& node = mPath.getNode( mIdxNode );
		mWorld->setActorBlock( *mHost , node.block->id , node.faceDir , false);

		switch( node.block->getFace( node.faceDir ).func )
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