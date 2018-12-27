#include "CantanLevel.h"

#include "Core/IntegerType.h"

#include <limits>
#include <algorithm>

namespace Cantan
{

	MapCellManager::~MapCellManager()
	{
		cleanup();
	}

	struct DeleteOp
	{
		template< class T >
		void operator()( T* ptr ){ delete ptr; }
	};

	void MapCellManager::cleanup()
	{
		std::for_each( mCellVertices.begin() , mCellVertices.end() , DeleteOp() );
		std::for_each( mCellEdges.begin() , mCellEdges.end() , DeleteOp() );
		std::for_each( mCells.begin() , mCells.end() , DeleteOp() );

		mCellVertices.clear();
		mCellEdges.clear();
		mCells.clear();
		mPosMap.clear();
	}

	void MapCellManager::buildIsotropicMap(int size)
	{
		cleanup();

		MapCell* cellCenter = constructCell( Vec2i(0,0) , size );

		for( int i = 1 ; i < size ; ++i )
		{
			Vec2i pos = i * HexCellUtility::CellNeighborOffset( Dir::ValueChecked(4) );
			for( int d = 0 ; d < DirNum; ++d )
			{
				Vec2i offset = HexCellUtility::CellNeighborOffset( Dir::ValueChecked(d) );
				for( int n = 0 ; n < i ; ++n )
				{
					MapCell* cell = constructCell( pos , size );
					pos += offset;
				}
			}
		}

		typedef std::map< uint32 , MapCell::Edge* > EdgeMap;
		EdgeMap edgeMap;
		//
		for( CellVertexVec::iterator iter = mCellVertices.begin() , itEnd = mCellVertices.end();
			iter != itEnd ; ++iter )
		{
			MapCell::Vertex* v = *iter;

			int nC = 0;
			int nE = 0;
			for( int i = 0 ; i < DirNum ; ++i )
			{
				Vec2i nPos = v->pos + HexCellUtility::VertexNeighborOffset( Dir::ValueChecked(i) );
				PosNodeMap::iterator iter = mPosMap.find( nPos );
				if ( iter == mPosMap.end() )
					continue;

				if ( HexCellUtility::IsCell( nPos ) )
				{
					v->cells[ nC++ ] = static_cast< MapCell* >( iter->second );
					assert( nC <= MapCell::Vertex::NumCell );
				}
				else
				{
					MapCell::Vertex* vLink = static_cast< MapCell::Vertex* >( iter->second );
					uint16 idx1 = v->idx;
					uint16 idx2 = vLink->idx;
					if ( idx1 < idx2 )
						std::swap( idx1 , idx2 );
					uint32 key = ( uint32( idx1 ) << 16 ) | uint32( idx2 );
					EdgeMap::iterator iterEdge = edgeMap.lower_bound( key );
					MapCell::Edge* edge;
					if ( iterEdge == edgeMap.end() || edgeMap.key_comp()( key , iterEdge->first ) )
					{
						edge = new MapCell::Edge;
						edge->v[0] = v;
						edge->v[1] = vLink;
						edgeMap.insert( iterEdge , std::make_pair( key , edge ) );
						mCellEdges.push_back( edge );
					}
					else
					{
						edge = iterEdge->second;
					}

					v->edges[nE++] = edge;
					assert( nE <= MapCell::Vertex::NumEdge );
				}
			}
		}
		
	}

	MapCell* MapCellManager::constructCell( Vec2i const& pos , int size )
	{
		assert( HexCellUtility::IsCell( pos ) );
		MapCell* cell = new MapCell;
		mCells.push_back( cell );
		cell->pos = pos;
		assert( mPosMap[ pos ] == nullptr );
		mPosMap[ pos ] = cell;
		for( int i = 0 ; i < DirNum ; ++i )
		{
			Vec2i vPos = pos + HexCellUtility::VertexNeighborOffset( Dir::ValueChecked( i ) );
			assert( HexCellUtility::IsVertex( vPos ) );

			int dist = HexCellUtility::Distance( vPos , Vec2i(0,0) );
			if ( dist > size + 1 )
				continue;
			if ( dist == size + 1 && ( vPos.x == 0 || vPos.y == 0 || vPos.x == vPos.y ) )
				continue;

			PosNodeMap::iterator iter = mPosMap.lower_bound( vPos );
			if ( iter == mPosMap.end() || mPosMap.key_comp()( vPos , iter->first ) )
			{
				MapCell::Vertex* v = new MapCell::Vertex;
				v->idx = mCellVertices.size();
				assert( v->idx < std::numeric_limits< uint16 >::max() );
				v->pos = vPos;
				mCellVertices.push_back( v );
				mPosMap.insert( iter , std::make_pair( vPos , v ) );
			}	
		}
		return cell;
	}

}//namespace Cantan
