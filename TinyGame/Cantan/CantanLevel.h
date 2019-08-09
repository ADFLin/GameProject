#include "TCycleNumber.h"
#include "TVector2.h"

#include "CppVersion.h"
#include "Core/IntegerType.h"

#include <algorithm>
#include <cassert>
#include <vector>
#include <map>

namespace Cantan
{
	int const DirNum = 6;
	typedef TCycleNumber< DirNum > Dir;
	typedef TVector2< int > Vec2i;

	class HexCellUtility
	{
	public:
		static bool IsCell( Vec2i const& p )
		{
			int u = 2 * p.x - p.y;
			int v = 2 * p.y - p.x;
			return ( u % 3 == 0 ) && ( v % 3  == 0 );
		}

		static bool IsVertex( Vec2i const& p )
		{
			return !IsCell( p );
		}

		static int  Distance( Vec2i const& a , Vec2i const& b )
		{
			Vec2i dif = a - b;
			int x = std::abs( dif.x );
			int y = std::abs( dif.y );
			if ( dif.x * dif.y > 0 )
			{
				return std::max( x , y );
			}
			return x + y;
		}

		static Vec2i const& VertexNeighborOffset( Dir dir )
		{
			static Vec2i const VertexPosOffset[] = 
			{
				Vec2i(1,0),Vec2i(1,1),Vec2i(0,1),
				Vec2i(-1,0),Vec2i(-1,-1),Vec2i(0,-1),
			};
			static_assert( sizeof( VertexPosOffset ) / sizeof( VertexPosOffset[0] ) == DirNum , "Error VertexPosOffset Num");
			return VertexPosOffset[ dir ];
		}

		static Vec2i CellNeighborOffset( Dir dir )
		{
			static Vec2i const CellPosOffset[] = 
			{
				Vec2i(2,1),Vec2i(1,2),Vec2i(-1,1),
				Vec2i(-2,-1),Vec2i(-1,-2),Vec2i(1,-1),
			};
			static_assert( sizeof( CellPosOffset ) / sizeof( CellPosOffset[0] ) == DirNum , "Error CellPosOffset Num" );
			return CellPosOffset[ dir ];
		}

		static Vec2i CellNeighborPos( Vec2i const& p , Dir dir )
		{
			assert( IsCell( p ) );
			return p + CellNeighborOffset( dir );
		}
	};


	class MapNode
	{
	public:
		Vec2i    pos;
		bool  isCell() const { return HexCellUtility::IsCell( pos );  }
	};

	enum ResType
	{
		RES_IRON ,
		RES_SHEEP ,
		RES_RICE ,
		RES_WOOD ,
		RES_BRICK ,
	};

	class CellClass
	{
	public:



	};

	class MapCell : public MapNode
	{
	public:
		int        value;
		unsigned   flag;
		CellClass* mClass;

		struct Vertex;
		struct Edge
		{
			Vertex* v[2];
			void getLinkCellPos( Vec2i outPos[2] )
			{
				Vec2i diff = v[0]->pos - v[1]->pos;
				Vec2i offset;
				if ( diff.y == 0 )
				{
					offset = Vec2i(1,0);
				}
				else
				{
					offset = Vec2i(1,0);
				}


				if ( HexCellUtility::IsCell( v[0]->pos + offset ) )
				{
					outPos[0] = v[0]->pos + offset;
					outPos[1] = v[1]->pos - offset;
				}
				else
				{
					outPos[0] = v[0]->pos - offset;
					outPos[1] = v[1]->pos + offset;
				}

				assert( HexCellUtility::IsCell( outPos[0] ) &&
					    HexCellUtility::IsCell( outPos[1] ) );
			}
		};

		struct Vertex : public MapNode
		{
			static int const NumEdge = 3;
			static int const NumCell = 3;

			uint16   idx;
			Edge*    edges[NumEdge];
			MapCell* cells[NumCell];


			Vertex()
			{
				std::fill_n( cells , NumCell , (MapCell*)0 );
				std::fill_n( edges , NumEdge , (Edge*)0 );
			}


			void     removeEdge( Edge* edge )
			{
				int i = 0;
				for( ; i < NumEdge ; ++i )
				{
					if ( edges[i] == edge )
					{
						edges[i] = nullptr;
						break;
					}
				}
				assert( i != NumEdge );
				for( ; i < NumEdge - 1 ; ++i )
					edges[i] = edges[i+1];
			}

			Vertex*  getLinkVertex( int idx )
			{
				Edge* edge = edges[idx];
				if ( edge == nullptr )
					return nullptr;
				return ( edge->v[0] == this ) ? edge->v[1] : edge->v[0];
			}
		};
	};

	class MapCellManager
	{
	public:

		~MapCellManager();
		void     buildIsotropicMap( int size );


		MapCell* constructCell( Vec2i const& pos , int size );
		void     cleanup();

		typedef std::vector< MapCell::Vertex* > CellVertexVec;
		CellVertexVec mCellVertices;
		typedef std::vector< MapCell::Edge* > CellEdgeVec;
		CellEdgeVec   mCellEdges;
		typedef std::vector< MapCell* > CellVec;
		CellVec   mCells;

		MapNode* getNode( Vec2i const& pos )
		{
			PosNodeMap::iterator iter = mPosMap.find( pos );
			if ( iter == mPosMap.end() )
				return nullptr;
			return iter->second;
		}
		void removeEdge( MapCell::Edge* edge )
		{
			assert( edge );
			edge->v[0]->removeEdge( edge );
			edge->v[1]->removeEdge( edge );
		}
		struct VecCmp
		{
			bool operator() ( Vec2i const& v1 , Vec2i const& v2 ) const
			{
				return ( v1.x < v2.x ) || ( !(v2.x < v1.x) && v1.y < v2.y );
			}
		};
		typedef std::map< Vec2i , MapNode* , VecCmp > PosNodeMap;
		PosNodeMap mPosMap;
	};

}//namespace Cantan