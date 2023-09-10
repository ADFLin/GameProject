#include "TinyGamePCH.h"
#include "Bsp2D.h"
#include "Math/PrimitiveTest.h"

#include "DebugDraw.h"

namespace Bsp2D
{
	float const WallThin = 1e-3f;


	void Plane::init( Vector2 const& v1 , Vector2 const& v2 )
	{
		Vector2 offset = v2 - v1;
		float len = sqrt( offset.length2() );
		normal.x = offset.y / len;
		normal.y = -offset.x / len;
		d = -normal.dot( v1 );
	}

	Side Plane::splice( Vector2 v[2] , Vector2 vSplit[2] )
	{
		float dist;
		Side s0 = testSide( v[0] , dist );
		Side s1 = testSide( v[1] , dist );
		if ( s0 + s1 != 0 )
			return ( s0 ) ? ( s0 ) : s1;

		if ( s0 == SIDE_IN )
			return SIDE_IN;

		Vector2 dir = v[0] - v[1];

		//plane.normal( v[1] + dir * t ) + d = 0;
		float t = - dist / normal.dot( dir );
		Vector2 p = v[1] + t * dir;

		if ( s0 == SIDE_FRONT )
		{
			vSplit[0] = p;
			vSplit[1] = v[1];
			v[1] = p;
		}
		else
		{
			vSplit[0] = v[0];
			vSplit[1] = p;
			v[0] = p;
		}

		return SIDE_SPLIT;
	}

	Side Plane::testSide( Vector2 const& p , float& dist )
	{
		dist = calcDistance( p );
		if ( dist > WallThin )
			return SIDE_FRONT;
		else if ( dist < -WallThin )
			return SIDE_BACK;
		return SIDE_IN;
	}

	Side Plane::testSegment( Vector2 const& v0 , Vector2 const& v1 )
	{
		float dist;
		Side s0 = testSide( v0 , dist );
		Side s1 = testSide( v1 , dist );

		if ( s0 + s1 != 0 )
			return ( s0 ) ? ( s0 ) : s1;

		if ( s0 == SIDE_IN )
			return SIDE_IN;

		return SIDE_SPLIT;
	}



	bool Plane::getIntersectPos( Vector2 const& v1 , Vector2 const& v2 , Vector2& out )
	{


		return false;
	}



	void Tree::clear()
	{
		if ( mRoot )
		{
			struct DeleteFun
			{
				void operator() ( Node* node ){ delete node; }
			};
			std::for_each( mNodes.begin() , mNodes.end() , DeleteFun() );
			//deleteNode_R( mRoot );
			mNodes.clear();
			mRoot = nullptr;
		}
		mEdges.clear();
		mLeaves.clear();
	}

	void Tree::build( PolyArea* polys[] , int num , Vector2 const& bbMin , Vector2 const& bbMax )
	{
		clear();


		int numEdge = 0;
		for( int i = 0 ; i < num ; ++i )
		{
			PolyArea* poly = polys[i];

			numEdge += poly->getVertexNum();

			int prev = poly->getVertexNum() - 1;
			for( int cur = 0 ;cur < poly->getVertexNum() ; ++cur )
			{
				assert( isInRect( poly->getVertex( prev ) , bbMin , bbMax ) );
				assert( isInRect( poly->getVertex( cur ) , bbMin , bbMax ) );

				addEdge( poly->getVertex( prev ) , poly->getVertex( cur ) , i );
				prev = cur;
			}
		}


		Vector2 edgeBB[2] = { Vector2( bbMin.x , bbMax.y ) , Vector2( bbMax.x , bbMin.y ) };

		addEdge( bbMin , edgeBB[0] , -1 );
		addEdge( edgeBB[0] , bbMax , -1 );
		addEdge( bbMax , edgeBB[1] , -1 );
		addEdge( edgeBB[1] , bbMin , -1 );

		numEdge += 4;
		struct CountFun
		{
			CountFun(){ current = 0; }
			int current;
			int operator()() {return current++;}
		};
		IndexVec idxEdges( numEdge );
		std::generate_n( idxEdges.begin() , numEdge , CountFun() );

		assert( mRoot == nullptr );

		mRoot = contructTree_R( idxEdges );
		mRoot->parent = nullptr;
	}

	Tree::Node* Tree::contructTree_R( IndexVec& idxEdges )
	{
		if ( idxEdges.empty() )
			return nullptr;

		int idx = choiceBestSplitEdge( idxEdges );

		Node* node = new Node;

		node->idxEdge   = idx;

		if ( idx < 0 ) // leaf
		{
			node->tag   = ~uint32( mLeaves.size() );
			node->front = nullptr;
			node->back  = nullptr;

			mLeaves.push_back( Leaf() );
			Leaf& data = mLeaves.back();
			data.node = node;
			data.edges.swap( idxEdges );
			return node;
		}
		else
		{
			node->tag = uint32( mNodes.size() );
			mNodes.push_back( node );
		}

		//triList.erase( cIter );

		IndexVec idxFronts;
		IndexVec idxBacks;

		Plane& plane = mEdges[ idx ].plane;

		for( IndexVec::iterator iter( idxEdges.begin() ) , itEnd( idxEdges.end() ) ; 
			iter != itEnd ; ++iter  )
		{
			int idxTest = *iter;
			Edge& edgeTest = mEdges[ idxTest ];

			Vector2 vSplit[2];
			switch ( plane.splice( edgeTest.v , vSplit ) )
			{
			case SIDE_FRONT:
			case SIDE_IN:
				idxFronts.push_back( idxTest );
				break;
			case SIDE_BACK:
				idxBacks.push_back( idxTest );
				break;
			case SIDE_SPLIT:
				{
					idxFronts.push_back( idxTest );
					idxBacks.push_back( (int)mEdges.size() );
					mEdges.push_back( Edge() );
					Edge& edge = mEdges.back();
					edge.v[0] = vSplit[0];
					edge.v[1] = vSplit[1];
					edge.plane = edgeTest.plane;
					edge.idx   = edgeTest.idx;
				}
				break;
			}				
		}

		node->front = contructTree_R( idxFronts );
		if ( node->front )
			node->front->parent = node;

		node->back  = contructTree_R( idxBacks );
		if ( node->back )
			node->back->parent = node;

		//node->tag   = 0;

		return node;
	}

	int Tree::choiceBestSplitEdge( IndexVec const& idxEdges )
	{
		int minVal = INT_MAX;
		int result = -1;

		//return result;
		for( IndexVec::const_iterator iter ( idxEdges.begin() ) , itEnd( idxEdges.end() ) ; 
			iter != itEnd ; ++iter  )
		{
			int numFront = 0;
			int numBack = 0;
			int numSplit = 0;
			int numOnPlane = 0;

			Edge& edge = mEdges[ *iter ];
			Plane& plane = edge.plane;

			for( IndexVec::const_iterator iter2 ( idxEdges.begin() )  ; 
				iter2 != itEnd ; ++iter2  )
			{
				Edge& edgeTest = mEdges[ *iter2 ];
				switch( plane.testSegment( edgeTest.v ) )
				{
				case SIDE_FRONT: ++numFront; break;
				case SIDE_BACK:  ++numBack;  break;
				case SIDE_SPLIT: ++numSplit; break;
				case SIDE_IN:    ++numOnPlane; break;
				}
			}

			if ( ( numSplit == 0 ) &&
				( numBack == 0 || numFront == 0) )
				continue;

			int val = abs( numFront - numBack ) + 8 * numSplit;
			if ( val < minVal )
			{
				result = *iter;
				minVal = val;
			}
		}
		return result;
	}

	class Tree::SegmentColSolver
	{
	public:
		bool solve( Tree& tree , Vector2 const& start , Vector2 const& end , ColInfo& info )
		{
			if ( !tree.getRoot() )
				return false;

			mTree  = &tree;
			mInfo  = &info;
			mStart = start;
			mEnd   = end;
			return solve_R( mTree->getRoot() );
		}

	private:

		bool solve_R( Node* node )
		{
			assert( node );

			if ( node->isLeaf() )
			{
				Leaf& leaf = mTree->getLeaf( node );
				for( int i = 0 ; i < leaf.edges.size(); ++i )
				{
					int idxEdge = leaf.edges[i];
					Edge& edge = mTree->getEdge( idxEdge );

					float frac;
					if ( Math::SegmentInterection( mStart , mEnd , edge.v[0] , edge.v[1] , frac ) )
					{
						mInfo->indexEdge = idxEdge;
						mInfo->frac = frac;
						return true;
					}

				}
				return false;
			}

			Edge& edge = mTree->getEdge( node->idxEdge );

			float dist;
			Side s0 = edge.plane.testSide( mStart , dist );
			Side s1 = edge.plane.testSide( mEnd , dist );

			if ( s0 == SIDE_IN )
			{
				if ( solve_R( node->front ) )
					return true;
				if ( s1 == SIDE_BACK )
					return solve_R( node->back );
			}
			else if ( s0 == s1 )
			{
				if ( s0 == SIDE_FRONT )
				{
					return solve_R( node->front );
				}
				else
				{
					return solve_R( node->back  );
				}
			}
			else
			{
				if ( s0 == SIDE_FRONT )
				{
					if ( solve_R( node->front ) )
						return true;
					if ( solve_R( node->back ) )
						return true;
				}
				else
				{
					if ( solve_R( node->back ) )
						return true;
					if ( solve_R( node->front ) )
						return true;
				}
			}

			return false;
		}

		Tree::ColInfo* mInfo;
		Tree*  mTree;
		Vector2  mStart;
		Vector2  mEnd;
	};

	bool Tree::segmentTest( Vector2 const& start , Vector2 const& end , ColInfo& info )
	{
		SegmentColSolver solver;
		return solver.solve( *this , start , end , info );
	}


	void Tree::PortalBuilder::build( Vector2 const& max , Vector2 const& min )
	{
		int numNodes = mTree->mNodes.size();
		for( int i = 0 ; i < numNodes ; ++i )
		{
			Node* node = mTree->mNodes[i];

			if ( node->isLeaf() )
				continue;

			SuperPlane* splane = createSuperPlane( node , max , min );
			generatePortal_R( node , splane );
		}
	}

	Tree::PortalBuilder::SuperPlane* Tree::PortalBuilder::createSuperPlane( Node* node , Vector2 const& max , Vector2 const& min )
	{
		assert( !node->isLeaf() );

		SuperPlane* splane = new SuperPlane;
		splane->node  = node;
		splane->plane = mTree->getEdge( node->idxEdge ).plane;
		splane->tag   = mSPlanes.size();

		Portal portal;
		portal.con[0] = 0;
		portal.con[1] = 0;

		Vector2 v[4] = { max , Vector2( min.x , max.y ) , min , Vector2( max.x , min.y ) };
		for( int i = 0 , prev = 3 ; i < 4 ; prev = i++ )
		{
			Vector2 dir = v[ i ] - v[ prev ];
			float dot = dir.dot( splane->plane.normal );
			if ( abs( dot ) < FLOAT_EPSILON )
				continue;
			float t = ( v[i].dot( splane->plane.normal ) + splane->plane.d ) / dot;
			Vector2 p = v[i] + t * dir;
		}
	


		Node* cur = node->parent;
		while( cur )
		{




			cur = cur->parent;
		}

		return splane;
	}

}//namespace Bsp2D