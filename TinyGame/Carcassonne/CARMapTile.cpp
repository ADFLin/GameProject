#include "CAR_PCH.h"
#include "CARMapTile.h"

#include <algorithm>

namespace CAR
{
	MapTile::MapTile( TilePiece const& tile , int rotation) 
		:mTile(&tile)
		,rotation(rotation)
		,userData(nullptr)
		,towerHeight(0)
		,bridgeMask(0)
		,goldPices(0)
		,group(INDEX_NONE)
		,haveHill( false )
		,mergedTileId { FAIL_TILE_ID , FAIL_TILE_ID }
	{
		if( tile.isHalflingType() )
		{
			for( int i = 0; i < TilePiece::NumSide; ++i )
			{
				SideNode& node = sideNodes[i];
				node.index = i;
				node.outConnect = nullptr;
				node.group = INDEX_NONE;
				int lDir = FDir::ToLocal(i, rotation);
				if( i < TilePiece::NumSide / 2 )
				{
					node.type = mTile->getLinkType(lDir);
					node.linkMask = LocalToWorldSideLinkMask( mTile->getSideLinkMask(lDir), rotation);
				}
				else
				{
					node.type = ESide::Empty;
					node.linkMask = 0;
				}
			}
			for( int i = 0; i < TilePiece::NumFarm; ++i )
			{
				FarmNode& node = farmNodes[i];
				node.index = i;
				node.outConnect = nullptr;
				node.group = INDEX_NONE;
			}
		}
		else
		{
			for( int i = 0; i < TilePiece::NumSide; ++i )
			{
				SideNode& node = sideNodes[i];
				node.index = i;
				node.outConnect = nullptr;
				node.group = INDEX_NONE;
				int lDir = FDir::ToLocal(i, rotation);
				node.type = mTile->getLinkType(lDir);
				node.linkMask = LocalToWorldSideLinkMask( mTile->getSideLinkMask(lDir) , rotation);
			}
			for( int i = 0; i < TilePiece::NumFarm; ++i )
			{
				FarmNode& node = farmNodes[i];
				node.index = i;
				node.outConnect = nullptr;
				node.group = INDEX_NONE;
			}
		}
	}

	void MapTile::connectSide(int dir , MapTile& data)
	{
		sideNodes[dir].connectNode( data.sideNodes[ FDir::Inverse(dir)] );
	}

	void MapTile::connectFarm(int idx , MapTile& data)
	{
		farmNodes[idx].connectNode( data.farmNodes[ TilePiece::FarmIndexConnect( idx ) ] );
	}

	ESide::Type MapTile::getLinkType(int dir) const
	{
		return sideNodes[ dir ].type;
	}

	int MapTile::getFarmGroup(int idx) const
	{ 
		return farmNodes[idx].group; 
	}

	int MapTile::getSideGroup(int dir) const
	{ 
		return sideNodes[dir].group; 
	}

	unsigned MapTile::getCityLinkFarmMask(int idx) const
	{
		assert(!mTile->isHalflingType());
		int idxLocal = TilePiece::ToLocalFarmIndex( idx , rotation );

#if 0
		if( mTile->isHalflingType() && lDir >= TilePiece::NumSide / 2 )
		{

		}
		else
		{

		}
#endif

		unsigned mask = 0;
		unsigned sideMask = mTile->farms[ idxLocal ].sideLinkMask;
		for( auto iter = TBitMaskIterator< FDir::TotalNum >(sideMask); iter; ++iter )
		{
			if( mTile->canLinkCity(iter.index) )
				mask |= BIT(iter.index);
		}
		return LocalToWorldSideLinkMask(mask, rotation);
	}

	unsigned MapTile::getSideLinkMask(int dir) const
	{
		return sideNodes[dir].linkMask;
	}

	unsigned MapTile::getRoadLinkMask(int dir) const
	{
		int dirLocal = FDir::ToLocal(dir, rotation);
		unsigned mask;
		if ( mTile->isHalflingType() && dirLocal >= TilePiece::NumSide / 2 )
		{
			mask = 0;
		}
		else
		{
			mask = mTile->getRoadLinkMask(dirLocal);
		}

		return LocalToWorldRoadLinkMask(mask, rotation);
	}

	unsigned MapTile::calcSideRoadLinkMeskToCenter() const
	{
		unsigned roadMask = 0;
		int sideNum = (mTile->isHalflingType()) ? FDir::TotalNum/2 : FDir::TotalNum;
		for( int i = 0 ; i < sideNum; ++i)
		{
			if ( mTile->sides[i].roadLinkDirMask & TilePiece::CenterMask )
				roadMask |= mTile->sides[i].roadLinkDirMask;
		}
		return LocalToWorldRoadLinkMask( roadMask , rotation );
	}

	Vec2i MapTile::getAircaftDirOffset() const
	{
		assert( getTileContent() & BIT(TileContent::eAircraft ));
		Vec2i localOffset = Vec2i(0,0);
		for( int i = 0; i < TilePiece::NumSide; ++i )
		{
			if( mTile->sides[i].contentFlag & BIT(SideContent::eAircraftDirMark) )
				localOffset += FDir::LinkOffset(i);
		}
		return FDir::ToWorld(localOffset , rotation);
	}

	unsigned MapTile::LocalToWorldRoadLinkMask(unsigned roadMask, int rotation)
	{
		unsigned bitCenter = roadMask & TilePiece::CenterMask;
		return bitCenter | FBitUtility::RotateLeft(roadMask & TilePiece::AllSideMask, rotation, TilePiece::NumSide );
	}

	unsigned MapTile::LocalToWorldSideLinkMask(unsigned mask, int rotation)
	{
		assert((mask & ~TilePiece::AllSideMask) == 0);
		return FBitUtility::RotateLeft(mask, rotation, TilePiece::NumSide );
	}

	unsigned MapTile::LocalToWorldFarmLinkMask(unsigned mask, int rotation)
	{
		assert((mask & ~TilePiece::AllFarmMask) == 0);
		return FBitUtility::RotateLeft(mask, 2 * rotation, TilePiece::NumFarm );
	}

	unsigned MapTile::getFarmLinkMask(int idx) const
	{
		int idxLocal = TilePiece::ToLocalFarmIndex(idx, rotation);
		unsigned mask;
		if( mTile->isHalflingType() && idxLocal >= TilePiece::NumFarm / 2 )
		{
			mask = 0;
		}
		else
		{
			mask = mTile->farms[idxLocal].farmLinkMask;
		}
		return LocalToWorldFarmLinkMask( mask , rotation );
	}

	void MapTile::addActor(LevelActor& actor)
	{
		if ( actor.mapTile != nullptr )
		{
			actor.mapTile->removeActor( actor );
		}
		actor.mapTile = this;
		mActors.push_back( &actor );
	}

	void MapTile::removeActor(LevelActor& actor)
	{
		assert( actor.mapTile == this );
		actor.mapTile = nullptr;
		mActors.erase( std::find( mActors.begin() , mActors.end() , &actor ) );
	}

	SideContentMask MapTile::getSideContnet(int dir) const
	{
		int dirLocal = FDir::ToLocal(dir, rotation);
		if( mTile->isHalflingType() && dirLocal >= TilePiece::NumSide / 2 )
			return 0;
		
		return mTile->sides[dirLocal].contentFlag;
	}

	TileContentMask MapTile::getTileContent() const
	{
		return mTile->contentFlag;
	}

	void MapTile::addBridge(int dir)
	{
		int invDir = FDir::Inverse( dir );
		assert( getLinkType( dir ) == ESide::Field && getLinkType( FDir::Inverse( invDir ) ) == ESide::Field);

		unsigned linkMask = BIT( dir ) | BIT( invDir );
		sideNodes[dir].type = ESide::Road;
		removeLinkMask( dir );
		sideNodes[dir].linkMask = linkMask;

		sideNodes[invDir].type = ESide::Road;
		removeLinkMask( invDir );
		sideNodes[invDir].linkMask = linkMask;

		bridgeMask |= linkMask;
	}

	void MapTile::margeHalflingTile(TilePiece const& tile, TilePiece& mergedTile)
	{
		assert(mTile->isHalflingType() && tile.isHalflingType());

		mergedTileId[0] = mTile->id;
		mergedTileId[1] = tile.id;

		int const Rotation = TilePiece::NumSide / 2;

		//build temp TilePiece
		for( int i = 0; i < TilePiece::NumSide / 2; ++i )
		{
			mergedTile.sides[i] = mTile->sides[i];
			int idx = i + TilePiece::NumSide / 2;
			mergedTile.sides[idx] = tile.sides[i];
			mergedTile.sides[idx].linkDirMask = LocalToWorldSideLinkMask(tile.sides[i].linkDirMask, Rotation);
			mergedTile.sides[idx].roadLinkDirMask = LocalToWorldSideLinkMask(tile.sides[i].roadLinkDirMask, Rotation);
		}
		for( int i = 0; i < TilePiece::NumFarm / 2; ++i )
		{
			mergedTile.farms[i] = mTile->farms[i];
			int idx = i + TilePiece::NumFarm / 2;
			mergedTile.farms[idx] = tile.farms[i];
			mergedTile.farms[idx].sideLinkMask = LocalToWorldSideLinkMask(tile.farms[i].sideLinkMask, Rotation);
			mergedTile.farms[idx].farmLinkMask = LocalToWorldFarmLinkMask(tile.farms[i].farmLinkMask, Rotation);
		}

		{
			unsigned sideMask = mTile->sides[IndexHalflingSide].linkDirMask |
				LocalToWorldSideLinkMask(tile.sides[IndexHalflingSide].linkDirMask, Rotation);

			for(auto iter = TBitMaskIterator<FDir::TotalNum>(sideMask); iter; ++iter )
			{
				mergedTile.sides[iter.index].linkDirMask |= sideMask;
			}
		}

		for( int i = 0 ; i < 2 ; ++i )
		{
			unsigned farmMask = mTile->farms[2 * IndexHalflingSide + i].farmLinkMask | 
				LocalToWorldFarmLinkMask( tile.farms[2 * IndexHalflingSide + (1 - i)].farmLinkMask , Rotation);
			unsigned sideMask = mTile->farms[2 * IndexHalflingSide + i].sideLinkMask |
				LocalToWorldFarmLinkMask( tile.farms[2 * IndexHalflingSide + (1 - i)].sideLinkMask , Rotation);

			for( auto iter = TBitMaskIterator<TilePiece::NumFarm>(farmMask); iter; ++iter )
			{
				mergedTile.farms[iter.index].farmLinkMask |= farmMask;
				mergedTile.farms[iter.index].sideLinkMask |= sideMask;
			}
		}
		mergedTile.contentFlag = ( BIT(TileContent::eTemp) | ( mTile->contentFlag | tile.contentFlag ) ) & ~BIT(TileContent::eHalfling);
		mergedTile.id = TEMP_TILE_ID;

		//update
		mTile = &mergedTile;
		for( int i = 0; i < TilePiece::NumSide; ++i )
		{
			SideNode& node = sideNodes[i];
			int lDir = FDir::ToLocal(i, rotation);
			node.type = mTile->getLinkType(lDir);
			node.linkMask = LocalToWorldSideLinkMask(mTile->getSideLinkMask(lDir), rotation);
		}
	}

	void MapTile::updateSideLink(unsigned linkMask)
	{
		for( auto iter = TBitMaskIterator< FDir::TotalNum >( linkMask ) ; iter ; ++iter )
		{
			sideNodes[iter.index].linkMask = linkMask;
		}
	}

	bool MapTile::isSemiCircularCity(int dir)
	{
		int dirLocal = FDir::ToLocal(dir, rotation);
		unsigned mask;
		if( mTile->isHalflingType() && dirLocal >= TilePiece::NumFarm / 2 )
			return false;
		
		return mTile->isSemiCircularCity(dirLocal);
	}

	int MapTile::getFeatureGroup(ActorPos const& pos)
	{
		switch( pos.type )
		{
		case ActorPos::eSideNode:
			return sideNodes[pos.meta].group;
		case ActorPos::eFarmNode:
			return farmNodes[pos.meta].group;
		}

		return ERROR_GROUP_ID;
	}

	int MapTile::SideNode::getLocalDir() const
	{
		return FDir::ToLocal( index , getMapTile()->rotation );
	}

	int MapTile::FarmNode::getLocalIndex() const
	{
		return FDir::ToLocal( index , getMapTile()->rotation );
	}


	bool TilePiece::isSemiCircularCity(int lDir) const
	{
		SideData const& sideData = sides[lDir];
		assert(sideData.linkType == ESide::City);
		return (sideData.linkDirMask == FBitUtility::ExtractTrailingBit(sideData.linkDirMask)) &&
			((sideData.contentFlag & BIT(SideContent::eNotSemiCircularCity)) == 0);
	}

	bool TilePiece::haveSideType(ESide::Type type) const
	{
		for(auto const& side : sides)
		{
			if( side.linkType == type )
				return true;
		}
		return false;
	}

	bool TilePiece::CanLink(ESide::Type typeA, ESide::Type typeB)
	{
		if( typeA == typeB )
			return true;
		unsigned const AbbeyLinkMask = BIT(ESide::Abbey) | BIT(ESide::City) | BIT(ESide::Road) | BIT(ESide::Field);
		if( typeA == ESide::Abbey && (AbbeyLinkMask & BIT(typeB)) )
			return true;
		if( typeB == ESide::Abbey && (AbbeyLinkMask & BIT(typeA)) )
			return true;
		return false;
	}

}//namespace CAR
