#include "RichWorld.h"

#include "RichArea.h"

#include <algorithm>

namespace Rich
{

	EmptyArea GEmptyArea( EMPTY_AREA_ID );

	World::World(IObjectQuery* objectQuery, IActorFactory* actorFactory)
		:mObjectQuery(objectQuery)
		,mActorFactory(actorFactory)
	{
		restAreaMap();	
	}

	int World::getMovableLinks( MapCoord const& posCur , MapCoord const& posPrev , LinkHandle outLinks[] )
	{
		int result = 0;

		LinkHandle dirPrev = EmptyLinkHandle;
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
				outLinks[ result ] = i;
				++result;
			}
		}

		outLinks[ result ] = dirPrev;
		return result;
	}

	int World::getLinks(MapCoord const& posCur, LinkHandle outLinks[])
	{
		int result = 0;

		for (int i = 0; i < DirNum; ++i)
		{
			MapCoord nextPos = posCur + dirOffset(i);
			if (getTile(nextPos))
			{
				outLinks[result] = i;
				++result;
			}
		}

		return result;
	}

	void World::clearArea(bool beDelete)
	{
		if ( beDelete )
		{
			int idx = mIdxFreeAreaId;
			while( mIdxFreeAreaId != -1 )
			{
				int next = intptr_t( mAreaMap[ mIdxFreeAreaId ] );
				mAreaMap[ mIdxFreeAreaId ] = nullptr;
			}
			for( int i = 1 ; i < mIdxLastArea; ++i )
			{
				delete mAreaMap[ i ];
			}
		}
		restAreaMap();
	}

	void World::restAreaMap()
	{
		std::fill_n( mAreaMap , MaxAreaNum , static_cast< Area* >( 0 ) );
		mIdxLastArea = 1;
		mAreaMap[ EMPTY_AREA_ID ] = &GEmptyArea;
		mIdxFreeAreaId = INDEX_NONE;
	}


	MapCoord const UninitPos = MapCoord(MaxInt32, MaxInt32);
	AreaId World::registerArea( Area* area , AreaId idReg /*= BLOCK_AREA_ID */ )
	{
		assert( area );
		assert( mIdxLastArea < MaxAreaNum );

		AreaId id;
		if ( idReg != ERROR_AREA_ID )
		{
			id = idReg;
		}
		else if ( mIdxFreeAreaId != INDEX_NONE)
		{
			id = AreaId( mIdxFreeAreaId );
			mIdxFreeAreaId = intptr_t( mAreaMap[ mIdxFreeAreaId ] );
		}
		else
		{
			id = AreaId( mIdxLastArea );
		}

		if ( mIdxLastArea == id )
			++mIdxLastArea;

		mAreaMap[ id ]= area;
		
		area->setId( id );
		area->setPos(UninitPos);
		area->install( *this );

		return id;
	}

	void World::unregisterArea( Area* area )
	{
		AreaId id = area->getId();
		assert( mAreaMap[ id ] == area );
		area->uninstall( *this );
		mAreaMap[ id ] = reinterpret_cast< Area* >( mIdxFreeAreaId );
		mIdxFreeAreaId = id;
		area->setId( ERROR_AREA_ID );
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


	bool World::addTile( MapCoord const& pos , AreaId id )
	{
		if ( !mMapData.checkRange( pos.x , pos.y ) )
			return false;
		if ( mMapData.getData( pos.x , pos.y ) != ERROR_TILE_ID )
			return false;

		Tile* tile = new Tile;
		tile->areaId = id;
		tile->pos  = pos;

		if (mAreaMap[id]->getPos() == UninitPos)
		{
			mAreaMap[id]->setPos(pos);
		}

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

	void World::dispatchMessage( WorldMsg const& msg )
	{
		for( IWorldMessageListener* listener : mEvtListers )
		{
			listener->onWorldMsg( msg );
		}
	}

	void World::addMsgListener( IWorldMessageListener& listener )
	{
		assert( std::find( mEvtListers.begin() , mEvtListers.end() , &listener ) == mEvtListers.end() );
		mEvtListers.push_back( &listener );
	}

	void World::removeMsgListener( IWorldMessageListener& listener )
	{
		mEvtListers.remove(&listener);
	}

	int World::findAreas( MapCoord const& pos , DistType dist , Area* outAreasFound[] )
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

				if ( tile->areaId == ERROR_AREA_ID )
					continue;

				Area* area = mAreaMap[ tile->areaId ];
				int n = 0;
				for ( ; i < numOut ; ++n )
				{
					if ( outAreasFound[n] == area )
						break;
				}
				if ( n == numOut )
				{
					outAreasFound[ numOut ] = area;
					++numOut;
				}
			}
		}
		return numOut;
	}

}//namespace Rich
