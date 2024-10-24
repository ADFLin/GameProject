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
	constexpr float WallThickness = 1e-3f;
	typedef ::Math::Vector2 Vector2;

	static bool IsInRect(Vector2 const& p, Vector2 const& min, Vector2 const& max)
	{
		return min.x - WallThickness <= p.x && p.x <= max.x + WallThickness &&
			   min.y - WallThickness <= p.y && p.y <= max.y + WallThickness;
	}

	static bool IsEqual(Vector2 const& v1, Vector2 const& v2)
	{
		Vector2 diff = (v1 - v2).abs();
		return diff.x < WallThickness && diff.y < WallThickness;
	}

	bool IsContain(Vector2 const a[], Vector2 const b[]);

	class PolyArea
	{
	public:
		PolyArea() = default;
		PolyArea( Vector2 const& v0 )
		{
			mVertices.reserve( 4 );
			mVertices.push_back( v0 );
		}

		void         shift( Vector2 const& offset )
		{
			for( int i = 0 ; i < mVertices.size() ; ++i )
				mVertices[i] += offset;
		}
		int          getVertexNum() const { return (int)mVertices.size(); }
		Vector2 const& getVertex( int idx ) const { return mVertices[ idx ]; }
		//void insertVertex( int idx , Vector2 const& v );
		void         pushVertex( Vector2 const& v ){ mVertices.push_back( v ); }


		TArray< Vector2 > mVertices;
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

		Side  testSegment( Vector2 const& v0 , Vector2 const& v1 ) const;
		Side  testSegment( Vector2 const v[2] ) const {  return testSegment( v[0] , v[1] );  }

		Side  split( Vector2 v[2] , Vector2 vSplit[2] ) const;
		int   clip(bool bFront, Vector2 inoutV[2]) const;
	};


	class Tree
	{
	public:

		struct Edge
		{
			int   polyIndex;
			Vector2 v[2];
			Plane plane;
		};



		typedef TArray< Edge > EdgeVec;
		typedef TArray< int > IndexList;


		static bool IsLeafTag(uint32 tag) { return (tag & 0x80000000) != 0; }

		struct Node
		{
			bool isLeaf() { return IsLeafTag(tag); }
			uint32    tag;
			int       idxEdge;
			Node*     parent;
			Node*     front;
			Node*     back;
		};

		struct Leaf
		{
			Node*     node;
			IndexList  edges;

			Vector2  debugPos;
		};

		struct Portal
		{
			Vector2  v[2];
			uint32   frontTag;
			uint32   backTag;
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

		Leaf& getLeafByTag(uint32 tag)
		{
			assert(IsLeafTag(tag));
			return  mLeaves[~tag];
		}

		void clear();

		struct ColInfo
		{
			float depth;
			int   indexEdge;
			float frac;
		};

		bool segmentTest( Vector2 const& start , Vector2 const& end , ColInfo& info );

		Node* getLeaf(Vector2 const& pos);



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

		void addEdge( Vector2 const& from , Vector2 const& to , int polyIndex )
		{
			Edge edge;
			edge.v[0] = from;
			edge.v[1] = to;
			edge.plane.init( edge.v[0] , edge.v[1] );
			edge.polyIndex  = polyIndex;
			mEdges.push_back( edge );
		}
		void  build( PolyArea* polys[] , int num , Vector2 const& bbMin , Vector2 const& bbMax );

		
		

	private:
		Node* contructTree_R( IndexList& edgeIndices);
		int   choiceBestSplitEdge( IndexList const& edgeIndices);

		class SegmentColSolver;
		class LeafFinder;

	};

	class PortalBuilder
	{
	public:
		using Node = Tree::Node;
		using Leaf = Tree::Leaf;
		using Edge = Tree::Edge;
		using Portal = Tree::Portal;

		using PortalVec = TArray< Portal >;
		struct SuperPlane
		{
			uint32    tag;
			Node*     node;
			Plane     plane;
			PortalVec portals;

			Vector2   debugPos[2];
		};

		using SPlaneVec = TArray< SuperPlane* >;
		SPlaneVec mSPlanes;

		~PortalBuilder()
		{
			cleanup();
		}

		void cleanup()
		{
			for (auto splane : mSPlanes)
			{
				delete splane;
			}
			mSPlanes.clear();
		}
		void build(Vector2 const& min, Vector2 const& max);

		SuperPlane* createSuperPlane(Node* node, Vector2 const& min, Vector2 const& max);

		void generatePortal_R(Node* node, SuperPlane* splane);
		Tree*  mTree;
	};

}//namespace Bsp2D

#endif // Bsp2D_h__
