#ifndef Bsp2D_h__
#define Bsp2D_h__

#include "TVector2.h"
#include "IntegerType.h"
#include "CppVersion.h"

#include <algorithm>

namespace Bsp2D
{
	float const FLOAT_EPSILON = 1e-5f;

	typedef TVector2< float > Vec2f;

	class PolyArea
	{
	public:
		PolyArea( Vec2f const& v0 )
		{
			mVtx.reserve( 4 );
			mVtx.push_back( v0 );
		}

		void         shift( Vec2f const& offset )
		{
			for( int i = 0 ; i < mVtx.size() ; ++i )
				mVtx[i] += offset;
		}
		int          getVertexNum() const { return (int)mVtx.size(); }
		Vec2f const& getVertex( int idx ) const { return mVtx[ idx ]; }
		//void insertVertex( int idx , Vec2f const& v );
		void         pushVertex( Vec2f const& v ){ mVtx.push_back( v ); }


		std::vector< Vec2f > mVtx;
		//int mSolidType;
	};

	enum Side
	{
		SIDE_FRONT = 1 ,
		SIDE_IN    = 0 ,
		SIDE_BACK  = -1,
		SIDE_SPLIT = 2,
	};

	struct Plane
	{
		Vec2f normal;
		float d;

		void  init( Vec2f const& v1 , Vec2f const& v2 );
		float calcDistance( Vec2f const& p ){  return normal.dot( p ) + d;  }
		bool  getInteractPos( Vec2f const& v1 , Vec2f const& v2 , Vec2f& out );
		Side  testSide( Vec2f const& p , float& dist );
		Side  testSegment( Vec2f const& v0 , Vec2f const& v1 );
		Side  testSegment( Vec2f const v[2] ){  return testSegment( v[0] , v[1] );  }
		Side  splice( Vec2f v[2] , Vec2f vSplit[2] );
	};


	class Tree
	{
	public:

		struct Edge
		{
			int   idx;
			Vec2f v[2];
			Plane plane;
		};



		typedef std::vector< Edge > EdgeVec;
		typedef std::vector< int > IndexVec;


		struct Node
		{
			bool isLeaf(){ return ( tag & 0x80000000 ) != 0; }
			uint32    tag;
			int       idxEdge;
			Node*     parent;
			Node*     front;
			Node*     back;
		};

		struct Leaf
		{
			Node*     node;
			IndexVec  edges;
		};

		struct Portal
		{
			Vec2f  v[2];
			uint32 con[2];
		};

		typedef std::vector< Leaf > LeafVec;
		typedef std::vector< Node* > NodeVec;

		NodeVec   mNodes;
		EdgeVec   mEdges;
		LeafVec   mLeaves;
		Node*     mRoot;
		int       mNumSplitNode;


		Tree()
		{
			mRoot = nullptr;
		}

		~Tree()
		{
			if ( mRoot )
				deleteNode_R( mRoot );
		}

		Node* getRoot(){ return mRoot; }

		Edge& getEdge( int idx )
		{
			return mEdges[ idx ];
		}

		Leaf& getLeaf( Node* node )
		{
			assert( node && node->isLeaf() );
			return  mLeaves[ ~node->tag ];
		}

		void clear();

		struct ColInfo
		{
			float frac;
			float depth;
			int   indexEdge;
		};

		bool segmentTest( Vec2f const& start , Vec2f const& end , ColInfo& info );



		template < class Visitor >
		void treasure( Vec2f const& pos , Visitor& visitor ){ treasure_R( mRoot , pos , visitor ); }

		template < class Visitor >
		void treasure_R( Node* node , Vec2f const& pos , Visitor& visitor )
		{
			if ( !node )
				return;

			if ( node->isLeaf() )
			{
				visitor.visit( mLeaves[ ~node->tag ] );
				return;
			}

			Edge& edge = mEdges[ node->idxEdge ];
			switch( edge.plane.testSide( pos ) )
			{
			case SIDE_FRONT:
			case SIDE_IN:
				treasure_R( node->front , pos , visitor );
				visitor.visit( *node );
				treasure_R( node->back , pos , visitor );
				break;
			case SIDE_BACK:
				treasure_R( node->back , pos , visitor );
				visitor.visit( *node );
				treasure_R( node->front , pos , visitor );
				break;
			}
		}


		template< class Visitor >
		void visit( Visitor& visitor ){ visit_R( mRoot , visitor ); }

		template< class Visitor >
		void visit_R( Node* node , Visitor& visitor )
		{
			if ( !node )
				return;

			if ( node->isLeaf() )
			{
				visitor.visit( mLeaves[ ~node->tag ] );
				return;
			}

			visit_R( node->front , visitor );
			visitor.visit( *node );
			visit_R( node->back , visitor );
		}

		static void deleteNode_R( Node* node )
		{
			if ( node->front )
				deleteNode_R( node->front );
			if ( node->back )
				deleteNode_R( node->back );
			delete node;
		}

		static bool isInRect( Vec2f const& p , Vec2f const& min , Vec2f const& max )
		{
			return min.x <= p.x && p.x < max.x &&
				min.y <= p.y && p.y < max.y;
		}

		void addEdge( Vec2f const& from , Vec2f const& to , int idx )
		{
			Edge edge;
			edge.v[0] = from;
			edge.v[1] = to;
			edge.plane.init( edge.v[0] , edge.v[1] );
			edge.idx  = idx;
			mEdges.push_back( edge );
		}
		void  build( PolyArea* polys[] , int num , Vec2f const& bbMin , Vec2f const& bbMax );

		
		

	private:
		Node* contructTree_R( IndexVec& idxEdges );
		int   choiceBestSplitEdge( IndexVec const& idxEdges  );

		class SegmentColSolver;

		class PortalBuilder
		{

			typedef std::vector< Portal > PortalVec;
			struct SuperPlane
			{
				uint32    tag;
				Node*     node;
				Plane     plane;
				PortalVec portals;
			};

			typedef std::vector< SuperPlane* > SPlaneVec;

			SPlaneVec mSPlanes;

			void build( Vec2f const& max , Vec2f const& min );

			SuperPlane* createSuperPlane( Node* node , Vec2f const& max , Vec2f const& min );


			void generatePortal_R( Node* node , SuperPlane* splane )
			{
				if ( node->isLeaf() )
				{
					Leaf& leaf = mTree->getLeaf( node );

					for( int i = 0 ; i < leaf.edges.size() ; ++i )
					{
						Edge& edge = mTree->getEdge( leaf.edges[i] );



					}

				}


			}
			Tree*  mTree;
		};


	};


}//namespace Bsp2D

#endif // Bsp2D_h__
