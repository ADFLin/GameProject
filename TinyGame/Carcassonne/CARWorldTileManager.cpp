#include "CAR_PCH.h"
#include "CARWorldTileManager.h"

#include "CARMapTile.h"
#include "CARDebug.h"
#include "CARExpansion.h"

#include <algorithm>

namespace CAR
{

	WorldTileManager::WorldTileManager(TileSetManager& manager)
		:mTileSetManager(&manager)
	{

	}

	WorldTileManager::~WorldTileManager()
	{
		cleanup();
	}

	void WorldTileManager::restart()
	{
		mMap.clear();
		mEmptyLinkPosSet.clear();
		cleanup();
	}

	void WorldTileManager::cleanup()
	{
		for (auto tilePiece : mTempTilePieces)
		{
			delete tilePiece;
		}
		mTempTilePieces.clear();
	}

	int WorldTileManager::placeTile(TileId tileId, Vec2i const& pos, int rotation, PlaceTileParam& param, MapTile* outMapTile[])
	{
		if ( !canPlaceTile( tileId , pos , rotation , param ) )
			return 0;
		return placeTileNoCheck( tileId , pos , rotation , param , outMapTile );
	}

	bool WorldTileManager::canPlaceTile( TileId tileId , Vec2i const& pos , int rotation , PlaceTileParam& param )
	{
		TileSet const& tileSet = mTileSetManager->getTileSet( tileId );

		switch( tileSet.type )
		{
		case ETileClass::Simple:
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
						int dirRiverLinkLocal = FDir::ToLocal(result.dirRiverLink, rotation);
						unsigned linkMask = MapTile::LocalToWorldSideLinkMask(tile.sides[dirRiverLinkLocal].linkDirMask, rotation);
						linkMask &= ~BIT( result.dirRiverLink );
						if ( linkMask && FBitUtility::ExtractTrailingBit( linkMask ) == linkMask )
						{
							if ( !checkRiverLinkDirection( result.riverLink->pos , FDir::Inverse( result.dirRiverLink ) , FBitUtility::ToIndex4( linkMask ) ) )
								return false;
						}
					}
				}
			}
			return true;
		case ETileClass::Double:
			{
				//
				param.checkRiverConnect = false;
				return canPlaceTileList(tileId, 2, pos, rotation, param);
			}
		case ETileClass::Halfling:
			{
				param.checkRiverConnect = false;

				PlaceResult result;
				if( canPlaceTileInternal(getTile(tileId), pos, rotation, param, result) == false )
					return false;
			}
			return true;
		}

		assert(0);
		return true;
	}

	bool WorldTileManager::canPlaceTileInternal( TilePiece const& tile , Vec2i const& pos , int rotation , PlaceTileParam& param , PlaceResult& result  )
	{
		int numSideCheck = FDir::TotalNum;
		int deltaDir = 0;
		if( tile.isHalflingType() )
		{
			assert(tile.sides[IndexHalflingSide].linkType != ESide::Abbey &&
				   tile.sides[IndexHalflingSide].linkType != ESide::Empty);

			MapTile* mapTile = findMapTile(pos);
			if( mapTile )
			{
				if( mapTile->isHalflingType() == false )
					return false;
				if( FDir::Inverse(rotation) != mapTile->rotation )
					return false;
				if( !TilePiece::CanLink(mapTile->getHalflingSideData().linkType, tile.sides[IndexHalflingSide].linkType) )
					return false;
			}
			else
			{
				param.canUseBridge = false;
			}
		}
		else
		{
			if( findMapTile(pos) != nullptr )
				return false;
		}
		
		int linkCount = 0;
		bool checkRiverConnect = false;
		int  numRiverConnect = 0;
		MapTile* riverLink = nullptr;
		int  dirRiverLink = -1;
		for( int i = 0 ; i < numSideCheck ; ++i )
		{
			int lDir = i + deltaDir;
			int dir = FDir::ToWorld(lDir, rotation);

			if ( param.checkRiverConnect )
			{
				if ( tile.getLinkType(lDir) == ESide::River )
				{
					checkRiverConnect = true;
				}
			}
			Vec2i linkPos = FDir::LinkPos( pos , dir );
			MapTile* dataCheck = findMapTile( linkPos );
			if ( dataCheck )
			{
				++linkCount;
				int lDirCheck = FDir::ToLocal( FDir::Inverse( dir ) , dataCheck->rotation );
				TilePiece const& tileCheck = getTile( dataCheck->getId() );
				if ( !TilePiece::CanLink( tileCheck , lDirCheck , tile , lDir ) )
				{
					if ( param.canUseBridge )
					{
						if ( !tileCheck.canLinkRoad( lDirCheck ) )
							return false;

						if ( tile.getLinkType(lDirCheck) != ESide::Field ||
							tile.getLinkType(lDir) == ESide::Field  )
							return false;

						param.dirNeedUseBridge = dir;
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
					dirRiverLink = dir;
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

	bool WorldTileManager::canPlaceTileList(TileId tileId, int numTile, Vec2i const& pos, int rotation, PlaceTileParam& param)
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

			if ( param.canUseBridge && param.idxTileUseBridge != -1 )
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
			if ( FBitUtility::ExtractTrailingBit( mask ) != mask )
				return true;

			int dirCheck = FBitUtility::ToIndex4( mask );
			if ( dirCheck == dir )
				return false;

			dirLink = FDir::Inverse( dirCheck );
			posLink = FDir::LinkPos( posLink , dirCheck );
		}
		return true;
	}

	int WorldTileManager::placeTileNoCheck( TileId tileId , Vec2i const& pos , int rotation , PlaceTileParam& param , MapTile* outMapTile[] )
	{
		TileSet const& tileSet = mTileSetManager->getTileSet( tileId );
		switch( tileSet.type )
		{
		case ETileClass::Simple:
			outMapTile[0] = placeTileInternal( getTile( tileId ) , pos , rotation , param );
			return 1;
		case ETileClass::Double:
			{
				Vec2i curPos = pos;
				for( int i = 0 ; i < 2 ; ++i )
				{
					outMapTile[i] = placeTileInternal( getTile( tileId , i ) , curPos , rotation , param );
					curPos += FDir::LinkOffset( rotation );
				}
			}
			return 2;
		case ETileClass::Halfling:
			outMapTile[0] = placeTileInternal(getTile(tileId), pos, rotation, param);
			return 1;
		}
		assert(0);
		return 0;
	}


	MapTile* WorldTileManager::placeTileInternal( TilePiece const& tile , Vec2i const& pos , int rotation , PlaceTileParam& param )
	{

		MapTile* mapTilePlace = nullptr;
		int numSideLink = FDir::TotalNum;
		int deltaDir = 0;

		if( tile.isHalflingType() )
		{
			numSideLink = FDir::TotalNum / 2;

			auto iter = mMap.find(pos);
			if( iter != mMap.end() )
			{
				mapTilePlace = &iter->second;
				assert(mapTilePlace->isHalflingType());

				TilePiece* mergedTilePiece = new TilePiece;

				mapTilePlace->margeHalflingTile(tile, *mergedTilePiece);

				mTempTilePieces.push_back(mergedTilePiece);

				deltaDir = FDir::TotalNum / 2;
				mHalflingTiles.erase(std::find(mHalflingTiles.begin(), mHalflingTiles.end(), mapTilePlace));
			}
		}
		
		if ( mapTilePlace == nullptr )
		{
			auto iter = mMap.insert(std::make_pair(pos, MapTile(tile, rotation))).first;
			assert(iter != mMap.end());

			{
				auto iter = mEmptyLinkPosSet.find(pos);
				if( iter != mEmptyLinkPosSet.end() )
					mEmptyLinkPosSet.erase(iter);
			}

			mapTilePlace = &iter->second;
			mapTilePlace->checkCount = mCheckCount;
			mapTilePlace->pos = pos;
			mapTilePlace->rotation = rotation;

			if( param.canUseBridge && param.dirNeedUseBridge != -1 )
			{
				mapTilePlace->addBridge(param.dirNeedUseBridge);
			}

			if( tile.isHalflingType() )
			{
				mHalflingTiles.push_back(mapTilePlace);
			}
		}	
		
		for( int i = 0 ; i < numSideLink ; ++i )
		{
			int lDir = i + deltaDir;
			int dir = FDir::ToWorld( lDir , rotation );

			Vec2i posLink = FDir::LinkPos( pos , dir );
			MapTile* mapTileCheck = findMapTile( posLink );

			if ( mapTileCheck != nullptr )
			{
				//link node
				assert( TilePiece::CanLink( tile , lDir , *mapTileCheck->mTile , 
					    FDir::ToLocal( FDir::Inverse(dir) , mapTileCheck->rotation ) ) );

				if ( tile.getLinkType(lDir) != ESide::Empty )
				{
					mapTilePlace->connectSide(dir, *mapTileCheck);

					//link farm
					if( tile.canLinkFarm(lDir) )
					{
						int idx = TilePiece::DirToFarmIndexFirst(dir);
						mapTilePlace->connectFarm(idx, *mapTileCheck);
						mapTilePlace->connectFarm(idx + 1, *mapTileCheck);
					}
				}
			}
			else
			{
				mEmptyLinkPosSet.insert( posLink );
			}
		}

		return mapTilePlace;
	}


	TilePiece const& WorldTileManager::getTile( TileId id , int idx ) const
	{
		TileSet const& tileSet = mTileSetManager->getTileSet( id );
		return tileSet.tiles[idx];
	}

	int WorldTileManager::getPosibleLinkPos( TileId tileId , TArray< Vec2i >& outPos , PlaceTileParam& param )
	{
		TileSet const& tileSet = mTileSetManager->getTileSet( tileId );

		int idxStart = outPos.size();
		switch ( tileSet.type )
		{
		case ETileClass::Simple:
			for( Vec2i const& pos : mEmptyLinkPosSet )
			{
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
		case ETileClass::Double:
			for( Vec2i const& pos : mEmptyLinkPosSet )
			{
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
			outPos.makeUnique(idxStart);
			break;
		case ETileClass::Halfling:
			for( Vec2i const& pos : mEmptyLinkPosSet )
			{
				for( int i = 0; i < FDir::TotalNum; ++i )
				{
					if( canPlaceTile(tileId, pos, i, param) )
					{
						outPos.push_back(pos);
					}
				}
			}
			for( auto mapTile : mHalflingTiles )
			{
				if( canPlaceTile(tileId, mapTile->pos, FDir::Inverse( mapTile->rotation ) , param) )
				{
					outPos.push_back(mapTile->pos);
				}
			}
			break;
		default:
			assert(0);
		}

		return outPos.size() - idxStart;
	}

	MapTile* WorldTileManager::findMapTile(Vec2i const& pos)
	{
		auto iter = mMap.find( pos );
		if ( iter == mMap.end() )
			return nullptr;
		return &iter->second;
	}

	void WorldTileManager::incCheckCount()
	{
		++mCheckCount;
		if ( mCheckCount == 0 )
		{
			for(auto& pair : mMap)
			{
				pair.second.checkCount = 0;
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
		case ETileClass::Double:
			for( int i = 0 ; i < FDir::TotalNum ; ++i )
			{
				if ( isEmptyLinkPos( pos + FDir::LinkOffset(i) ) )
					return true;
			}
			return false;
		
		case ETileClass::Simple:
			return isEmptyLinkPos( pos );
		case ETileClass::Halfling:
			{
				MapTile* mapTile = findMapTile(pos);
				if( mapTile && mapTile->isHalflingType() )
					return true;
			}
			return isEmptyLinkPos(pos);
		}

		assert(0);
		return false;
	}

	int WorldTileManager::getNeighborCount(Vec2i const& pos, TileContentMask content)
	{
		int result = 0;
		for( int i = 0; i < FDir::NeighborNum; ++i )
		{
			Vec2i nPos = pos + FDir::NeighborOffset(i);
			MapTile* mapTile = findMapTile(nPos);
			if( mapTile )
			{
				if( mapTile->getTileContent() & content )
				{
					++result;
				}
			}
		}
		return result;
	}

	TileSetManager::~TileSetManager()
	{
		cleanup();
	}

	void TileSetManager::addExpansion(Expansion exp)
	{
		for( int idx = 0; idx < gAllExpansionTileContents.size() ; ++idx )
		{
			ExpansionContent const& content = gAllExpansionTileContents[idx];
			if( content.exp == exp )
			{
				importData(content);
				return;
			}
		}
	}

	void TileSetManager::cleanup()
	{
		for(TileSet& tileSet : mTileMap)
		{
			switch( tileSet.type )
			{
			case ETileClass::Halfling:
			case ETileClass::Simple:
				delete tileSet.tiles;
				break;
			case ETileClass::Double: 
				delete [] tileSet.tiles;
				break;
			default:
				assert(0);
			}
		}

		for(auto& set : mSetMap)
		{
			set.clear();
		}
		mTileMap.clear();
	}

	void TileSetManager::importData( ExpansionContent const& content )
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
				tileSet = &createSingleTileSet( tileDef , (content.exp == EXP_TEST ) ? TileSet::eTest : TileSet::eCommon );
			}
			tileSet->expansion = content.exp;
			tileSet->idxDefine = idxDefine;
			++idxDefine;
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
		tileSet.numPieces = tileDef[0].numPieces;
		tileSet.tiles = tiles;
		tileSet.tag = tileDef[0].tag;
		tileSet.type = ETileClass::Double;
		mTileMap.push_back( tileSet );

		tileSet.group = group;
		mSetMap[ group ].push_back( id );

		return mTileMap[ id ];
	}

	TileSet& TileSetManager::createSingleTileSet( TileDefine const& tileDef , TileSet::EGroup group )
	{
		TilePiece* tile = new TilePiece;
		
		TileId id = (TileId)mTileMap.size();
		tile->id = id;
		setupTile( *tile , tileDef );

		TileSet tileSet;
		tileSet.numPieces = tileDef.numPieces;
		tileSet.tiles = tile;
		tileSet.tag = tileDef.tag;
		tileSet.type = ETileClass::Simple;
		if ( tile->haveRiver() )
		{
			group = TileSet::eRiver;
		}

		tileSet.group = group;

		mTileMap.push_back(tileSet);
		mSetMap[ group ].push_back( id );

		return mTileMap[ id ];
	}

	void TileSetManager::setupTile(TilePiece& tile , TileDefine const& tileDef)
	{
		bool isHalflingType = !!(tileDef.content & BIT(TileContent::eHalfling) );
		for( int i = 0; i < TilePiece::NumSide; ++i )
		{
			tile.sides[i].linkType = (ESide::Type)tileDef.linkType[i];
			tile.sides[i].contentFlag = tileDef.sideContent[i];
			tile.sides[i].linkDirMask = BIT(i);
			tile.sides[i].roadLinkDirMask = 0;
		}

		tile.contentFlag = tileDef.content;

		for( int i = 0; i < ARRAY_SIZE(tileDef.sideLink); ++i )
		{
			if( tileDef.sideLink[i] == 0 )
				break;
			assert((tileDef.sideLink[i] & ~0xf) == 0);
			unsigned linkMask = tileDef.sideLink[i];
#if _DEBUG
			int idxStart = -1;
#endif
			for( auto iter = TBitMaskIterator< 8 >(linkMask); iter; ++iter )
			{
				int idx = iter.index;
#if _DEBUG
				if( idxStart == -1 ) 
				{ 
					idxStart = idx; 
				}
				assert(TilePiece::CanLink(tile.sides[idx].linkType, tile.sides[idxStart].linkType));
#endif
				tile.sides[idx].linkDirMask = tileDef.sideLink[i];
			}
		}

		for( int i = 0; i < ARRAY_SIZE(tileDef.roadLink); ++i )
		{
			if( tileDef.roadLink[i] == 0 )
				break;

			unsigned linkMask = tileDef.roadLink[i] & ~TilePiece::CenterMask;
			for( auto iter = TBitMaskIterator< 8 >(linkMask) ; iter ; ++iter )
			{
				tile.sides[iter.index].roadLinkDirMask |= tileDef.roadLink[i];
			}
		}

		int numSide;
		int numFarm;
		unsigned farmLinkRe;
		if( isHalflingType )
		{
			for( int i = 0; i < TilePiece::NumSide / 2; ++i )
			{
				tile.sides[i].linkDirMask &= ~BIT(IndexHalflingSide);
				tile.sides[i].roadLinkDirMask &= ~BIT(IndexHalflingSide);
			}

			numSide = TilePiece::NumSide / 2 + 1;
			numFarm = TilePiece::NumFarm / 2 + 2;
			farmLinkRe = BIT(6) - 1;
		}
		else
		{
			numSide = TilePiece::NumSide;
			numFarm = TilePiece::NumFarm;
			farmLinkRe = TilePiece::AllFarmMask;
		}

		unsigned noFarmIndexMask = 0;
		for( int i = 0; i < numSide; ++i )
		{
			if( tile.canRemoveFarm(i) )
			{
				int idx = TilePiece::DirToFarmIndexFirst(i);
				noFarmIndexMask |= BIT(idx) | BIT(idx + 1);
			}
		}

		noFarmIndexMask &= ~tileDef.centerFarmMask;
		if( isHalflingType )
			noFarmIndexMask |= ( BIT(6) | BIT(7) );


		for( int i = 0; i < numFarm; ++i )
		{
			tile.farms[i].sideLinkMask = 0;
			if( noFarmIndexMask & BIT(i) )
				tile.farms[i].farmLinkMask = 0;
			else
				tile.farms[i].farmLinkMask = BIT(i);
		}

		for( int i = 0; i < ARRAY_SIZE(tileDef.farmLink); ++i )
		{
			if( tileDef.farmLink[i] == 0 )
				break;

			unsigned usageFarmLink = tileDef.farmLink[i];
			if( usageFarmLink == 0xff )
			{
				usageFarmLink = farmLinkRe;
			}
			farmLinkRe &= ~tileDef.farmLink[i];

			unsigned linkMask = usageFarmLink & ~noFarmIndexMask;
			unsigned farmSideLink = calcFarmSideLinkMask(usageFarmLink);
			for( auto iter = TBitMaskIterator< TilePiece::NumFarm >(linkMask); iter; ++iter )
			{
				int idx = iter.index;
				tile.farms[idx].farmLinkMask = linkMask;
				tile.farms[idx].sideLinkMask = farmSideLink;
			}
		}

		if( isHalflingType )
		{
			for( int i = 0; i < TilePiece::NumFarm / 2; ++i )
			{
				tile.farms[i].farmLinkMask &= ~(BIT(4) | BIT(5));
				tile.farms[i].sideLinkMask &= ~BIT(IndexHalflingSide);
			}

		}
	}

	unsigned TileSetManager::calcFarmSideLinkMask(unsigned linkMask)
	{
		unsigned result = 0;
		while( linkMask )
		{
			unsigned bit = FBitUtility::ExtractTrailingBit( linkMask );
			int idx = FBitUtility::ToIndex8( bit );
			result |= BIT( TilePiece::FarmSideDir( idx ) );
			linkMask &= ~bit;
		}
		return result;
	}


}//namespace CAR

