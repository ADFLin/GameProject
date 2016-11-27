#include "CAR_PCH.h"
#include "CARWorldTileManager.h"

#include "CARMapTile.h"
#include "CARDebug.h"

#include <algorithm>

namespace CAR
{
	static int const gBitIndexMap[ 9 ] = { 0 , 0 , 1 , 1 , 2 , 2 , 2 , 2 , 3 };

	int FBit::ToIndex4( unsigned bit )
	{
		assert( (bit&0xf) == bit );
		assert( ( bit & ( bit - 1 ) ) == 0 );
		return gBitIndexMap[ bit ];
	}
	int FBit::ToIndex8( unsigned bit )
	{
		assert( (bit&0xff) == bit );
		assert( ( bit & ( bit - 1 ) ) == 0 );
		int result = 0;
		if ( bit & 0xf0 ){ result += 4; bit >>= 4; }
		return result + gBitIndexMap[ bit ];
	}
	int FBit::ToIndex32( unsigned bit )
	{
		assert( (bit&0xffffffff) == bit );
		assert( ( bit & ( bit - 1 ) ) == 0 );
		int result = 0;
		if ( bit & 0xffff0000 ){ result += 16; bit >>= 16; }
		if ( bit & 0xff00 ){ result += 8; bit >>= 8; }
		if ( bit & 0xf0 ){ result += 4; bit >>= 4; }
		return result + gBitIndexMap[ bit ];
	}

	unsigned FBit::RotateRight(unsigned bits , unsigned offset , unsigned numBit)
	{
		assert( offset <= numBit );
		unsigned mask = ( 1 << numBit ) - 1;
		return ( ( bits >> offset ) | ( bits << ( numBit - offset ) ) ) & mask;
	}

	unsigned FBit::RotateLeft(unsigned bits , unsigned offset , unsigned numBit)
	{
		assert( offset <= numBit );
		unsigned mask = ( 1 << numBit ) - 1;
		return ( ( bits << offset ) | ( bits >> ( numBit - offset ) ) ) & mask;
	}

	WorldTileManager::WorldTileManager()
	{

	}

	void WorldTileManager::restart()
	{
		mMap.clear();
		mEmptyLinkPosSet.clear();
	}

	int WorldTileManager::placeTile(TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param , MapTile* outMapTile[] )
	{
		if ( !canPlaceTile( tileId , pos , rotation , param ) )
			return 0;
		return placeTileNoCheck( tileId , pos , rotation , param , outMapTile );
	}

	bool WorldTileManager::canPlaceTile( TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param )
	{
		TileSet const& tileSet = mTileSetManager->getTileSet( tileId );

		switch( tileSet.type )
		{
		case TileType::eSimple:
			{
				PlaceResult result;
				if ( canPlaceTileInternal( getTile( tileId ) , pos , rotation , param , result ) == false )
					return false;

				if ( result.numTileLink == 0 )
					return false;

				if ( result.bNeedCheckRiverConnectDir )
				{
					if (  result.numRiverConnect == 0 )
						return false;

					if ( param.checkRiverDirection && result.numRiverConnect == 1 )
					{
						TilePiece const& tile = getTile( tileId );

						unsigned linkMask = FBit::RotateLeft( tile.sides[ FDir::ToLocal( result.dirRiverLink , rotation ) ].linkDirMask , rotation , 4 );
						linkMask &= ~BIT( result.dirRiverLink );
						if ( linkMask && FBit::Extract( linkMask ) == linkMask )
						{
							if ( !checkRiverLinkDirection( result.riverLink->pos , FDir::Inverse( result.dirRiverLink ) , FBit::ToIndex4( linkMask ) ) )
								return false;
						}
					}
				}
			}
			return true;
		case TileType::eDouble:
			//
			param.checkRiverConnect = false;
			return canPlaceTileList( tileId , 2 , pos , rotation , param );
		case TileType::eHalfling:
			assert(0);
			return false;
		}

		assert(0);
		return true;
	}

	bool WorldTileManager::canPlaceTileInternal( TilePiece const& tile , Vec2i const& pos , int rotation , PutTileParam& param , PlaceResult& result  )
	{
		if ( findMapTile( pos ) != nullptr )
			return false;

		int linkCount = 0;
		bool checkRiverConnect = false;
		int  numRiverConnect = 0;
		MapTile* riverLink = nullptr;
		int  dirRiverLink = -1;
		for( int i = 0 ; i < FDir::TotalNum ; ++i )
		{
			int lDir = FDir::ToLocal( i , rotation );

			if ( param.checkRiverConnect )
			{
				if ( tile.getLinkType(lDir) == SideType::eRiver )
				{
					checkRiverConnect = true;
				}
			}
			Vec2i linkPos = FDir::LinkPos( pos , i );
			MapTile* dataCheck = findMapTile( linkPos );
			if ( dataCheck )
			{
				++linkCount;
				int lDirCheck = FDir::ToLocal( FDir::Inverse( i ) , dataCheck->rotation );
				TilePiece const& tileCheck = getTile( dataCheck->getId() );
				if ( !TilePiece::CanLink( tileCheck , lDirCheck , tile , lDir ) )
				{
					if ( param.usageBridge )
					{
						if ( !tileCheck.canLinkRoad( lDirCheck ) )
							return false;

						if ( tile.getLinkType(lDirCheck) != SideType::eField ||
							tile.getLinkType(lDir) == SideType::eField  )
							return false;

						param.dirNeedUseBridge = i;
					}
					else
					{
						return false;
					}
				}

				if ( checkRiverConnect && tileCheck.canLinkRiver( lDirCheck ) )
				{
					++numRiverConnect;
					riverLink = dataCheck;
					dirRiverLink = i;
				}
			}
		}

		if ( param.checkDontNearSameTag )
		{
			unsigned tag = mTileSetManager->getTileTag( tile.id );

			for( int i = 0 ; i < FDir::NeighborNum ; ++i )
			{
				MapTile* dataCheck = this->findMapTile( pos + FDir::NeighborOffset(i) );
				if ( dataCheck && mTileSetManager->getTileTag( dataCheck->getId() ) == tag )
					return false;
			}
		}

		result.numTileLink = linkCount;
		result.bNeedCheckRiverConnectDir = checkRiverConnect;
		result.dirRiverLink = dirRiverLink;
		result.numRiverConnect = numRiverConnect;
		result.riverLink = riverLink;
		return true;
	}

	bool WorldTileManager::canPlaceTileList( TileId tileId , int numTile , Vec2i const& pos , int rotation , PutTileParam& param )
	{
		assert( param.checkRiverConnect == false );
		assert( param.idxTileUseBridge == -1 );

		int const MaxUseBridgeNum = 1;

		Vec2i curPos = pos;
		int numTileLink = 0;
		for( int i = 0 ; i < numTile ; ++i )
		{
			int dirTemp = param.dirNeedUseBridge;
			PlaceResult result;
			if ( !canPlaceTileInternal( getTile( tileId , i ) , curPos , rotation , param , result ) )
				return false;

			if ( param.usageBridge && param.idxTileUseBridge != -1 )
			{
				if ( param.idxTileUseBridge != -1 )
					return false;
				param.idxTileUseBridge = i;
			}

			numTileLink += result.numTileLink;
			curPos += FDir::LinkOffset( rotation );
		}

		if ( numTileLink == 0 )
			return false;

		return true;
	}


	bool WorldTileManager::checkRiverLinkDirection( Vec2i const& pos , int dirLink , int dir )
	{
		Vec2i posLink = pos;
		for(;;)
		{
			MapTile* mapTile = findMapTile( posLink );
			if ( mapTile == nullptr )
				break;
			unsigned mask = mapTile->getSideLinkMask(dirLink);
			mask &= ~BIT( dirLink );
			if ( mask == 0 )
				return true;
			if ( FBit::Extract( mask ) != mask )
				return true;

			int dirCheck = FBit::ToIndex4( mask );
			if ( dirCheck == dir )
				return false;

			dirLink = FDir::Inverse( dirCheck );
			posLink = FDir::LinkPos( posLink , dirCheck );
		}
		return true;
	}

	int WorldTileManager::placeTileNoCheck( TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param , MapTile* outMapTile[] )
	{
		TileSet const& tileSet = mTileSetManager->getTileSet( tileId );
		switch( tileSet.type )
		{
		case TileType::eSimple:
			outMapTile[0] = placeTileInternal( getTile( tileId ) , pos , rotation , param );
			return 1;
		case TileType::eDouble:
			{
				Vec2i curPos = pos;
				for( int i = 0 ; i < 2 ; ++i )
				{
					outMapTile[i] = placeTileInternal( getTile( tileId , i ) , curPos , rotation , param );
					curPos += FDir::LinkOffset( rotation );
				}
			}
			return 2;
		case TileType::eHalfling:
			//TODO
			assert(0);
			return 0;
		}
		assert(0);
		return 0;
	}


	MapTile* WorldTileManager::placeTileInternal( TilePiece const& tile , Vec2i const& pos , int rotation , PutTileParam& param )
	{
		WorldTileMap::iterator iter = mMap.insert( std::make_pair( pos , MapTile( tile ,rotation ) ) ).first;
		assert( iter != mMap.end() );

		{
			PosSet::iterator iter = mEmptyLinkPosSet.find( pos );
			if ( iter != mEmptyLinkPosSet.end() )
				mEmptyLinkPosSet.erase( iter );
		}

		MapTile& mapData = iter->second;
		mapData.checkCount = mCheckCount;
		mapData.pos        = pos;
		mapData.rotation   = rotation;

		if ( param.usageBridge && param.dirNeedUseBridge != - 1 )
		{
			mapData.addBridge( param.dirNeedUseBridge );
		}

		for( int i = 0 ; i < FDir::TotalNum ; ++i )
		{
			Vec2i posLink = FDir::LinkPos( pos , i );
			
			MapTile* dataCheck = findMapTile( posLink );
			int lDir = FDir::ToLocal( i , rotation );

			if ( dataCheck != nullptr )
			{
				//link node
				assert( TilePiece::CanLink( tile , lDir , *dataCheck->mTile , 
					    FDir::ToLocal( FDir::Inverse( i ) , dataCheck->rotation ) ) );

				mapData.connectSide( i , *dataCheck );

				//link farm
				if ( tile.canLinkFarm( lDir ) )
				{
					int idx = TilePiece::DirToFramIndexFrist( i );
					mapData.connectFarm( idx , *dataCheck );
					mapData.connectFarm( idx + 1 , *dataCheck );
				}

			}
			else
			{
				mEmptyLinkPosSet.insert( posLink );
			}
		}


		return &mapData;
	}


	TilePiece const& WorldTileManager::getTile( TileId id , int idx ) const
	{
		TileSet const& tileSet = mTileSetManager->getTileSet( id );
		return tileSet.tiles[idx];
	}

	void makeValueUnique( std::vector< Vec2i >& outPos , int idxStart )
	{
		int len = outPos.size();
		for( int i = idxStart ; i < len ; ++i )
		{
			for( int j = i + 1 ; j < len ; ++j )
			{
				if ( outPos[i] == outPos[j] )
				{
					if ( j != len - 1 ) 
					{
						std::swap( outPos[j] , outPos[ len - 1 ] );
						--j;
					}
					--len;
				}
			}
		}
		if ( len != outPos.size() )
		{
			outPos.resize( len );
		}
	}

	int WorldTileManager::getPosibleLinkPos( TileId tileId , std::vector< Vec2i >& outPos , PutTileParam& param )
	{
		TileSet const& tileSet = mTileSetManager->getTileSet( tileId );

		int idxStart = outPos.size();
		switch ( tileSet.type )
		{
		case TileType::eSimple:
			for( PosSet::iterator iter = mEmptyLinkPosSet.begin() , itEnd = mEmptyLinkPosSet.end();
				iter != itEnd ; ++iter )
			{
				Vec2i const& pos = *iter;
				for(int i = 0 ; i < FDir::TotalNum ; ++i )
				{
					if ( canPlaceTile( tileId , pos , i , param ) )
					{
						outPos.push_back( pos );
						break;
					}
				}
			}
			break;
		case TileType::eDouble:
			
			for( PosSet::iterator iter = mEmptyLinkPosSet.begin() , itEnd = mEmptyLinkPosSet.end();
				iter != itEnd ; ++iter )
			{
				Vec2i const& pos = *iter;
				for( int n = 0 ; n < 2 ; ++n )
				{
					for(int i = 0 ; i < FDir::TotalNum ; ++i )
					{
						Vec2i testPos = pos - n * FDir::LinkOffset( i );
						if ( canPlaceTile( tileId , testPos , i , param ) )
						{
							outPos.push_back( testPos );
						}
					}
				}
			}
			makeValueUnique( outPos ,idxStart );
			break;
		default:
			assert(0);
		}

		return outPos.size() - idxStart;
	}

	MapTile* WorldTileManager::findMapTile(Vec2i const& pos)
	{
		WorldTileMap::iterator iter = mMap.find( pos );
		if ( iter == mMap.end() )
			return nullptr;
		return &iter->second;
	}

	void WorldTileManager::incCheckCount()
	{
		++mCheckCount;
		if ( mCheckCount == 0 )
		{
			for( WorldTileMap::iterator iter = mMap.begin() , itEnd = mMap.end();
				iter != itEnd ; ++iter )
			{
				iter->second.checkCount = 0;
			}
		}
	}

	bool WorldTileManager::isEmptyLinkPos(Vec2i const& pos)
	{
		return mEmptyLinkPosSet.find( pos ) != mEmptyLinkPosSet.end();
	}

	bool WorldTileManager::isLinkTilePosible(Vec2i const& pos , TileId id)
	{
		TileSet const& tileSet = mTileSetManager->getTileSet( id );
		switch( tileSet.type )
		{
		case TileType::eDouble:
			for( int i = 0 ; i < FDir::NeighborNum ; ++i )
			{
				if ( isEmptyLinkPos( pos + FDir::NeighborOffset(i) ) )
					return true;
			}
			return false;
		case TileType::eHalfling:
		case TileType::eSimple:
			return isEmptyLinkPos( pos );
		}

		assert(0);
		return false;
	}

	TileSetManager::TileSetManager()
	{

	}


	TileSetManager::~TileSetManager()
	{
		cleanup();
	}

	void TileSetManager::cleanup()
	{
		for( int i = 0 ; i < mTileMap.size(); ++i )
		{
			TileSet& tileSet = mTileMap[i];
			switch( tileSet.type )
			{
			case TileType::eDouble: 
				delete [] tileSet.tiles;
				break;
			default:
				delete tileSet.tiles;
			}
		}

		for( int i = 0 ; i < TileSet::NumGroup ; ++i )
		{
			mSetMap[i].clear();
		}
		mTileMap.clear();

	}

	void TileSetManager::import( ExpansionTileContent const& content )
	{
		TileId id = 0;
		int idxDefine = 0;
		for ( int i = 0 ; i < content.numDefines ; ++i )
		{
			TileDefine& tileDef = content.defines[i];

			TileSet* tileSet;
			if ( tileDef.tag == TILE_GERMAN_CASTLE_TAG )
			{
				if ( i + 1 >= content.numDefines )
				{
					CAR_LOG("Warning: Error Content Defines");
					break;
				}
				tileSet = &createDoubleTileSet( content.defines + i , TileSet::eGermanCastle );
				++i;
			}
			else
			{
				tileSet = &createSingleTileSet( tileDef );
			}
			tileSet->expansions = content.exp;
			tileSet->idxDefine = idxDefine++;
		}
	}

	TileSet& TileSetManager::createDoubleTileSet(TileDefine const tileDef[] , TileSet::EGroup group )
	{
		TilePiece* tiles = new TilePiece[2];

		TileId id = (TileId)mTileMap.size();

		for( int i = 0 ; i < 2 ; ++i )
		{
			tiles[i].id = id;
			setupTile( tiles[i] , tileDef[i] );
		}

		TileSet tileSet;
		tileSet.numPiece = tileDef[0].numPiece;
		tileSet.tiles = tiles;
		tileSet.tag = tileDef[0].tag;
		tileSet.type = TileType::eDouble;
		mTileMap.push_back( tileSet );

		tileSet.group = group;
		mSetMap[ group ].push_back( id );

		return mTileMap[ id ];
	}

	TileSet& TileSetManager::createSingleTileSet(TileDefine const& tileDef)
	{
		TilePiece* tile = new TilePiece;
		
		TileId id = (TileId)mTileMap.size();
		tile->id = id;
		setupTile( *tile , tileDef );

		TileSet tileSet;
		tileSet.numPiece = tileDef.numPiece;
		tileSet.tiles = tile;
		tileSet.tag = tileDef.tag;
		tileSet.type = TileType::eSimple;

		mTileMap.push_back( tileSet );

		TileSet::EGroup group = TileSet::eCommon;
		if ( tile->haveRiver() )
		{
			group = TileSet::eRiver;
		}

		tileSet.group = group;
		mSetMap[ group ].push_back( id );

		return mTileMap[ id ];
	}

	void TileSetManager::setupTile(TilePiece& tile , TileDefine const& tileDef)
	{
		for( int i = 0 ; i < TilePiece::NumSide ; ++i )
		{
			tile.sides[i].linkType = (SideType)tileDef.linkType[i];
			tile.sides[i].contentFlag = tileDef.sideContent[i];
			tile.sides[i].linkDirMask = BIT(i);
			tile.sides[i].roadLinkDirMask = 0;
		}

		tile.contentFlag = tileDef.content;

		for( int i = 0 ; i < ARRAY_SIZE( tileDef.sideLink ) ; ++i )
		{
			if ( tileDef.sideLink[i] == 0 )
				break;
			assert( ( tileDef.sideLink[i] & ~0xf ) == 0 );

			unsigned linkMask = tileDef.sideLink[i];
			
			int idx;
#if _DEBUG
			int idxStart = -1;
#endif
			while ( FBit::MaskIterator< 8 >( linkMask , idx ) )
			{
#if _DEBUG
				if ( idxStart == -1 ){  idxStart = idx;  }
				assert( TilePiece::CanLink( tile.sides[idx].linkType , tile.sides[idxStart].linkType ) );
#endif
				tile.sides[idx].linkDirMask = tileDef.sideLink[i];
			}
		}

		for( int i = 0 ; i < ARRAY_SIZE( tileDef.roadLink ) ; ++i )
		{
			if ( tileDef.roadLink[i] == 0 )
				break;

			unsigned linkMask = tileDef.roadLink[i] & ~TilePiece::CenterMask;
			int idx;
			while ( FBit::MaskIterator< 8 >( linkMask , idx ) )
			{
				tile.sides[idx].roadLinkDirMask |= tileDef.roadLink[i];
			}
		}

		unsigned noFramIndexMask = 0;
		for( int i = 0 ; i < FDir::TotalNum ; ++i )
		{
			if ( tile.canRemoveFarm(i) )
			{
				int idx = TilePiece::DirToFramIndexFrist( i );
				noFramIndexMask |= BIT(idx)|BIT(idx+1);
			}
		}
		noFramIndexMask &= ~tileDef.centerFarmMask;

		for( int i = 0 ; i < TilePiece::NumFarm ; ++i )
		{
			tile.farms[i].sideLinkMask = 0;
			if ( noFramIndexMask & BIT(i) )
				tile.farms[i].farmLinkMask = 0;
			else
				tile.farms[i].farmLinkMask = BIT(i);
		}

		unsigned farmLinkRe = TilePiece::AllFarmMask;
		for( int i = 0 ; i < ARRAY_SIZE( tileDef.farmLink ) ; ++i )
		{
			if ( tileDef.farmLink[i] == 0 )
				break;

			unsigned usageFarmLink = tileDef.farmLink[i];
			if ( usageFarmLink == 0xff )
			{
				usageFarmLink = farmLinkRe;
			}
			farmLinkRe &= ~tileDef.farmLink[i];

			unsigned linkMask = usageFarmLink & ~noFramIndexMask;
			unsigned farmSideLink = calcFarmSideLinkMask( usageFarmLink );
			unsigned mask = linkMask;
			int idx;
			while ( FBit::MaskIterator< TilePiece::NumFarm >( mask , idx ) )
			{
				tile.farms[idx].farmLinkMask = linkMask;
				tile.farms[idx].sideLinkMask = farmSideLink;
			}
		}
	}

	unsigned TileSetManager::calcFarmSideLinkMask(unsigned linkMask)
	{
		unsigned result = 0;
		while( linkMask )
		{
			unsigned bit = FBit::Extract( linkMask );
			int idx = FBit::ToIndex8( bit );
			result |= BIT( TilePiece::FarmSideDir( idx ) );
			linkMask &= ~bit;
		}
		return result;
	}


}//namespace CAR

