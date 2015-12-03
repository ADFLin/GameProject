#include "RichPCH.h"
#include "RichWorld.h"

#include "RichCell.h"

#include <algorithm>

namespace Rich
{

	EmptyCell gEmptyCell( EMPTY_CELL_ID );

	World::World( IObjectQuery* query )
		:mQuery( query )
	{
		restCellMap();	
	}

	int World::getMoveDir( MapCoord const& posCur , MapCoord const& posPrev , DirType dir[] )
	{
		int result = 0;

		DirType dirPrev = NoDir;
		for( int i = 0 ; i < DirNum ; ++i )
		{
			MapCoord nextPos = posCur + dirOffset( i );

			if ( nextPos == posPrev )
			{
				dirPrev = i;
				continue;
			}
			if ( getTile( nextPos ) )
			{
				dir[ result ] = i;
				++result;
			}
		}

		dir[ result ] = dirPrev;
		return result;
	}

	void World::clearCell( bool beDelete )
	{
		if ( beDelete )
		{
			int idx = mIdxFreeCellId;
			while( mIdxFreeCellId != -1 )
			{
				int next = intptr_t( mCellMap[ mIdxFreeCellId ] );
				mCellMap[ mIdxFreeCellId ] = nullptr;
			}
			for( int i = 1 ; i < mIdxLastCell; ++i )
			{
				delete mCellMap[ i ];
			}
		}
		restCellMap();
	}

	void World::restCellMap()
	{
		std::fill_n( mCellMap , MaxCellNum , static_cast< Cell* >( 0 ) );
		mIdxLastCell = 1;
		mCellMap[ EMPTY_CELL_ID ] = &gEmptyCell;
		mIdxFreeCellId = -1;
	}

	Rich::CellId World::registerCell( Cell* cell , CellId idReg /*= BLOCK_CELL_ID */ )
	{
		assert( cell );
		assert( mIdxLastCell < MaxCellNum );

		CellId id;
		if ( idReg != ERROR_CELL_ID )
		{
			id = idReg;
		}
		else if ( mIdxFreeCellId != -1 )
		{
			id = CellId( mIdxFreeCellId );
			mIdxFreeCellId = intptr_t( mCellMap[ mIdxFreeCellId ] );
		}
		else
		{
			id = CellId( mIdxLastCell );
		}

		if ( mIdxLastCell == id )
			++mIdxLastCell;

		mCellMap[ id ]= cell;
		
		cell->setId( id );
		cell->install( *this );

		return id;
	}

	void World::unregisterCell( Cell* cell )
	{
		CellId id = cell->getId();
		assert( mCellMap[ id ] == cell );
		cell->uninstall( *this );
		mCellMap[ id ] = reinterpret_cast< Cell* >( mIdxFreeCellId );
		mIdxFreeCellId = id;
		cell->setId( ERROR_CELL_ID );
	}

	void World::setupMap( int w , int h )
	{
		mMapData.resize( w , h );
		clearMap();
	}

	void World::clearMap()
	{
		mMapData.fillValue( ERROR_TILE_ID );
		for( TileVec::iterator iter = mTileMap.begin() , end = mTileMap.end();
			iter != end ; ++iter )
		{
			delete *iter;
		}
		mTileMap.clear();
	}


	bool World::addTile( MapCoord const& pos , CellId id )
	{
		if ( !mMapData.checkRange( pos.x , pos.y ) )
			return false;
		if ( mMapData.getData( pos.x , pos.y ) != ERROR_TILE_ID )
			return false;

		Tile* tile = new Tile;
		tile->cell = id;
		tile->pos  = pos;

		mMapData( pos.x , pos.y ) = ( TileId )mTileMap.size();
		mTileMap.push_back( tile );

		return true;
	}

	bool World::removeTile( MapCoord const& pos )
	{
		if ( !mMapData.checkRange( pos.x , pos.y ) )
			return false;

		TileId id = mMapData.getData( pos.x , pos.y );
		if  ( id == ERROR_TILE_ID )
			return false;

		Tile* tile = mTileMap[ id ];
		mMapData( pos.x , pos.y ) = ERROR_TILE_ID;
		delete tile;

		if ( id != mTileMap.size() - 1 )
		{
			Tile* tileLast = mTileMap.back();
			mMapData( tileLast->pos.x , tileLast->pos.y ) = id;
			mTileMap[ id ] = tileLast;
		}
		mTileMap.pop_back();

		return true;
	}

	Tile* World::getTile( MapCoord const& pos )
	{
		if ( !mMapData.checkRange( pos.x , pos.y ) )
			return nullptr;
		TileId id = mMapData.getData( pos.x , pos.y );
		
		if ( id == ERROR_TILE_ID )
			return nullptr;

		return mTileMap[ id ];
	}


	class CRandom : public IRandom
	{
	public:
		virtual int getInt()
		{
			return ::rand();
		}
	} gRandom;

	IRandom& World::getRandom()
	{
		return gRandom;
	}

	void World::sendMsg( WorldMsg const& event )
	{
		for( EventListerVec::iterator iter = mEvtListers.begin() , itEnd = mEvtListers.end() ;
			 iter != itEnd ; ++iter )
		{
			IMsgListener* listener = *iter;
			listener->onWorldMsg( event );
		}
	}

	void World::addMsgListener( IMsgListener& listener )
	{
		assert( std::find( mEvtListers.begin() , mEvtListers.end() , &listener ) == mEvtListers.end() );
		mEvtListers.push_back( &listener );
	}

	void World::removeMsgListener( IMsgListener& listener )
	{
		EventListerVec::iterator iter = std::find( mEvtListers.begin() , mEvtListers.end() , &listener );
		if ( iter == mEvtListers.end() )
			return;
		mEvtListers.erase( iter );
	}

	int World::findCell( MapCoord const& pos , DistType dist , Cell* cellFound[] )
	{
		Vec2i min = pos - Vec2i( dist , dist );
		Vec2i max = pos + Vec2i( dist , dist );

		int numOut = 0;
		for( int i = min.x ; i <= max.x ; ++i )
		{
			if ( i < 0 || i >= mMapData.getSizeX() )
				continue;
			for( int j = min.y ; j <= max.y ; ++j )
			{
				if ( j < 0 || j >= mMapData.getSizeY() )
					continue;
				TileId id = mMapData.getData( i , j );
				if ( id == ERROR_TILE_ID )
					continue;
				Tile* tile = mTileMap[ id ];

				if ( tile->cell == ERROR_CELL_ID )
					continue;

				Cell* cell = mCellMap[ tile->cell ];
				int n = 0;
				for ( ; i < numOut ; ++n )
				{
					if ( cellFound[n] == cell )
						break;
				}
				if ( n == numOut )
				{
					cellFound[ numOut ] = cell;
					++numOut;
				}
			}
		}
		return numOut;
	}

}//namespace Rich
