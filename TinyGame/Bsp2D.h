#ifndef Bsp2D_h__
#define Bsp2D_h__

#include "Math/TVector2.h"
#include "Math/Vector2.h"
#include "Core/IntegerType.h"
#include "DataStructure/Array.h"
#include "Math/GeometryPrimitive.h"

#include <algorithm>


namespace Bsp2D
{

	typedef ::Math::Vector2 Vector2;

	class PolyArea
	{
	public:
		PolyArea( Vector2 const& v0 )
		{
			mVtx.reserve( 4 );
			mVtx.push_back( v0 );
		}

		void         shift( Vector2 const& offset )
		{
			for( int i = 0 ; i < mVtx.size() ; ++i )
				mVtx[i] += offset;
		}
		int          getVertexNum() const { return (int)mVtx.size(); }
		Vector2 const& getVertex( int idx ) const { return mVtx[ idx ]; }
		//void insertVertex( int idx , Vector2 const& v );
		void         pushVertex( Vector2 const& v ){ mVtx.push_back( v ); }


		TArray< Vector2 > mVtx;
		//int mSolidType;
	};


	namespace EPlaneSide = ::Math::EPlaneSide;
	enum Side
	{
		SIDE_FRONT = EPlaneSide::Front,
		SIDE_IN    = EPlaneSide::In,
		SIDE_BACK  = EPlaneSide::Back,
		SIDE_SPLIT = 2,
	};

	class Plane : public ::Math::Plane2D
	{
	public:
		using ::Math::Plane2D::Plane2D;

		void  init( Vector2 const& v1 , Vector2 const& v2 );
		bool  getIntersectPos( Vector2 const& v1 , Vector2 const& v2 , Vector2& out );
		Side  testSegment( Vector2 const& v0 , Vector2 const& v1 );
		Side  testSegment( Vector2 const v[2] ){  return testSegment( v[0] , v[1] );  }
		Side  split( Vector2 v[2] , Vector2 vSplit[2] );
	};


	class Tree
	{
	public:

		struct Edge
		{
			int   idx;
			Vector2 v[2];
			Plane plane;
		};



		typedef TArray< Edge > EdgeVec;
		typedef TArray< int > IndexVec;


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
			Vector2  v[2];
			uint32 con[2];
		};

		typedef TArray< Leaf > LeafVec;
		typedef TArray< Node* > NodeVec;

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

		bool segmentTest( Vector2 const& start , Vector2 const& end , ColInfo& info );



		template < class Visitor >
		void traverse( Vector2 const& pos , Visitor& visitor ){ traverse_R( mRoot , pos , visitor ); }

		template < class Visitor >
		void traverse_R( Node* node , Vector2 const& pos , Visitor& visitor )
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
				traverse_R( node->front , pos , visitor );
				visitor.visit( *node );
				traverse_R( node->back , pos , visitor );
				break;
			case SIDE_BACK:
				traverse_R( node->back , pos , visitor );
				visitor.visit( *node );
				traverse_R( node->front , pos , visitor );
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

		static bool isInRect( Vector2 const& p , Vector2 const& min , Vector2 const& max )
		{
			return min.x <= p.x && p.x < max.x &&
				min.y <= p.y && p.y < max.y;
		}

		void addEdge( Vector2 const& from , Vector2 const& to , int idx )
		{
			Edge edge;
			edge.v[0] = from;
			edge.v[1] = to;
			edge.plane.init( edge.v[0] , edge.v[1] );
			edge.idx  = idx;
			mEdges.push_back( edge );
		}
		void  build( PolyArea* polys[] , int num , Vector2 const& bbMin , Vector2 const& bbMax );

		
		

	private:
		Node* contructTree_R( IndexVec& idxEdges );
		int   choiceBestSplitEdge( IndexVec const& idxEdges  );

		class SegmentColSolver;

		class PortalBuilder
		{

			typedef TArray< Portal > PortalVec;
			struct SuperPlane
			{
				uint32    tag;
				Node*     node;
				Plane     plane;
				PortalVec portals;
			};

			typedef TArray< SuperPlane* > SPlaneVec;

			SPlaneVec mSPlanes;

			void build( Vector2 const& max , Vector2 const& min );

			SuperPlane* createSuperPlane( Node* node , Vector2 const& max , Vector2 const& min );


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
