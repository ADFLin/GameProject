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
		,group(-1)
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
				node.group = -1;
				int lDir = FDir::ToLocal(i, rotation);
				if( i < TilePiece::NumSide / 2 )
				{
					node.type = mTile->getLinkType(lDir);
					node.linkMask = LocalToWorldSideLinkMask( mTile->getSideLinkMask(lDir), rotation);
				}
				else
				{
					node.type = SideType::eEmptySide;
					node.linkMask = 0;
				}
			}
			for( int i = 0; i < TilePiece::NumFarm; ++i )
			{
				FarmNode& node = farmNodes[i];
				node.index = i;
				node.outConnect = nullptr;
				node.group = -1;
			}
		}
		else
		{
			for( int i = 0; i < TilePiece::NumSide; ++i )
			{
				SideNode& node = sideNodes[i];
				node.index = i;
				node.outConnect = nullptr;
				node.group = -1;
				int lDir = FDir::ToLocal(i, rotation);
				node.type = mTile->getLinkType(lDir);
				node.linkMask = LocalToWorldSideLinkMask( mTile->getSideLinkMask(lDir) , rotation);
			}
			for( int i = 0; i < TilePiece::NumFarm; ++i )
			{
				FarmNode& node = farmNodes[i];
				node.index = i;
				node.outConnect = nullptr;
				node.group = -1;
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

	CAR::SideType MapTile::getLinkType(int dir) const
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

	unsigned MapTile::LocalToWorldRoadLinkMask(unsigned roadMask, int rotation)
	{
		unsigned bitCenter = roadMask & TilePiece::CenterMask;
		return bitCenter | FBit::RotateLeft(roadMask & TilePiece::AllSideMask, rotation, TilePiece::NumSide );
	}

	unsigned MapTile::LocalToWorldSideLinkMask(unsigned mask, int rotation)
	{
		assert((mask & ~TilePiece::AllSideMask) == 0);
		return FBit::RotateLeft(mask, rotation, TilePiece::NumSide );
	}

	unsigned MapTile::LocalToWorldFarmLinkMask(unsigned mask, int rotation)
	{
		assert((mask & ~TilePiece::AllFarmMask) == 0);
		return FBit::RotateLeft(mask, 2 * rotation, TilePiece::NumFarm );
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

	unsigned MapTile::getSideContnet(int dir) const
	{
		int dirLocal = FDir::ToLocal(dir, rotation);
		if( mTile->isHalflingType() && dirLocal >= TilePiece::NumSide / 2 )
			return 0;
		
		return mTile->sides[dirLocal].contentFlag;
	}

	unsigned MapTile::getTileContent() const
	{
		return mTile->contentFlag;
	}

	void MapTile::addBridge(int dir)
	{
		int invDir = FDir::Inverse( dir );
		assert( getLinkType( dir ) == eField && getLinkType( FDir::Inverse( invDir ) ) == eField );

		unsigned linkMask = BIT( dir ) | BIT( invDir );
		sideNodes[dir].type = SideType::eRoad;
		removeLinkMask( dir );
		sideNodes[dir].linkMask = linkMask;

		sideNodes[invDir].type = SideType::eRoad;
		removeLinkMask( invDir );
		sideNodes[invDir].linkMask = linkMask;

		bridgeMask |= linkMask;
	}

	void MapTile::margeHalflingTile(TilePiece const& tile)
	{
		assert(mTile->isHalflingType() && tile.isHalflingType());

		mergedTileId[0] = mTile->id;
		mergedTileId[1] = tile.id;

		int const Rotation = TilePiece::NumSide / 2;

		//build temp TilePiece
		TilePiece* tileMerge = new TilePiece;
		for( int i = 0; i < TilePiece::NumSide / 2; ++i )
		{
			tileMerge->sides[i] = mTile->sides[i];
			int idx = i + TilePiece::NumSide / 2;
			tileMerge->sides[idx] = tile.sides[i];
			tileMerge->sides[idx].linkDirMask = LocalToWorldSideLinkMask(tile.sides[i].linkDirMask, Rotation);
			tileMerge->sides[idx].roadLinkDirMask = LocalToWorldSideLinkMask(tile.sides[i].roadLinkDirMask, Rotation);
		}
		for( int i = 0; i < TilePiece::NumFarm / 2; ++i )
		{
			tileMerge->farms[i] = mTile->farms[i];
			int idx = i + TilePiece::NumFarm / 2;
			tileMerge->farms[idx] = tile.farms[i];
			tileMerge->farms[idx].sideLinkMask = LocalToWorldSideLinkMask(tile.farms[i].sideLinkMask, Rotation);
			tileMerge->farms[idx].farmLinkMask = LocalToWorldFarmLinkMask(tile.farms[i].farmLinkMask, Rotation);
		}

		{
			unsigned sideMask = mTile->sides[IndexHalflingSide].linkDirMask |
				LocalToWorldSideLinkMask(tile.sides[IndexHalflingSide].linkDirMask, Rotation);

			for(auto iter = TBitMaskIterator<FDir::TotalNum>(sideMask); iter; ++iter )
			{
				tileMerge->sides[iter.index].linkDirMask |= sideMask;
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
				tileMerge->farms[iter.index].farmLinkMask |= farmMask;
				tileMerge->farms[iter.index].sideLinkMask |= sideMask;
			}
		}
		tileMerge->contentFlag = ( TileContent::eTemp | ( mTile->contentFlag | tile.contentFlag ) ) & ~TileContent::eHalfling;
		tileMerge->id = TEMP_TILE_ID;

		//update
		mTile = tileMerge;
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
		assert(sideData.linkType == SideType::eCity);
		return (sideData.linkDirMask == FBit::Extract(sideData.linkDirMask)) &&
			((sideData.contentFlag & SideContent::eNotSemiCircularCity) == 0);
	}

	bool TilePiece::haveSideType(SideType type) const
	{
		for( int i = 0; i < NumSide; ++i )
		{
			if( sides[i].linkType == type )
				return true;
		}
		return false;
	}

	bool TilePiece::CanLink(SideType typeA, SideType typeB)
	{
		if( typeA == typeB )
			return true;
		unsigned const AbbeyLinkMask = BIT(SideType::eAbbey) | BIT(SideType::eCity) | BIT(SideType::eRoad) | BIT(SideType::eField);
		if( typeA == SideType::eAbbey && (AbbeyLinkMask & BIT(typeB)) )
			return true;
		if( typeB == SideType::eAbbey && (AbbeyLinkMask & BIT(typeA)) )
			return true;
		return false;
	}

}//namespace CAR
