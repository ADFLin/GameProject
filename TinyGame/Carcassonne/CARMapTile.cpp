#include "CARMapTile.h"

#include <algorithm>

namespace CAR
{
	MapTile::MapTile( Tile const& tile , int rotation) 
		:mTile(&tile)
		,rotation(rotation)
		,renderData(nullptr)
		,towerHeight(0)
		,group(-1)
	{

		for( int i = 0 ; i < Tile::NumSide ; ++i )
		{
			SideNode& node = sideNodes[i];
			node.index      = i;
			node.outConnect = nullptr;
			node.group  = -1;
			unsigned mask = mTile->getSideLinkMask( FDir::ToLocal( i , rotation ) );
			assert(( mask & ~Tile::AllSideMask ) == 0 );
			node.linkMask = FBit::RotateLeft( mask , rotation , 4 );
		}
		for( int i = 0 ; i < Tile::NumFarm ; ++i )
		{
			FarmNode& node = farmNodes[i];
			node.index      = i;
			node.outConnect = nullptr;
			node.group  = -1;
		}
	}

	void MapTile::connectSide(int dir , MapTile& data)
	{
		sideNodes[dir].connectNode( data.sideNodes[ FDir::Inverse(dir)] );
	}

	void MapTile::connectFarm(int idx , MapTile& data)
	{
		farmNodes[idx].connectNode( data.farmNodes[ Tile::FarmIndexConnect( idx ) ] );
	}

	CAR::SideType MapTile::getLinkType(int dir) const
	{
		return mTile->getLinkType( FDir::ToLocal( dir , rotation ) );
	}

	int MapTile::getFarmGroup(int idx)
	{ 
		return farmNodes[idx].group; 
	}

	int MapTile::getSideGroup(int dir)
	{ 
		return sideNodes[dir].group; 
	}

	unsigned MapTile::getCityLinkFarmMask(int idx) const
	{
		int lDir = Tile::ToLocalFramIndex( idx , rotation );

		unsigned mask = 0;
		unsigned sideMask = mTile->farms[ lDir ].sideLinkMask;
		int lDirTest;
		while( FBit::MaskIterator4( sideMask , lDirTest ) )
		{
			if ( mTile->canLinkCity( lDirTest ) )
				mask |= BIT( lDirTest );
		}
		return FBit::RotateLeft( mask , rotation , 4 );
	}

	unsigned MapTile::getSideLinkMask(int dir) const
	{
		return sideNodes[dir].linkMask;
	}

	unsigned MapTile::getRoadLinkMask(int dir) const
	{
		unsigned mask = mTile->getRoadLinkMask( FDir::ToLocal( dir , rotation ) );
		unsigned bitCenter = mask & Tile::CenterMask;
		return bitCenter | FBit::RotateLeft( mask & Tile::AllSideMask , rotation , 4 );
	}

	unsigned MapTile::calcRoadMaskLinkCenter() const
	{
		unsigned roadMask = 0;
		for( int i = 0 ; i < FDir::TotalNum ; ++i)
		{
			if ( mTile->sides[i].roadLinkDirMask & Tile::CenterMask )
				roadMask |= mTile->sides[i].roadLinkDirMask;
		}
		unsigned bitCenter = roadMask & Tile::CenterMask;
		return bitCenter | FBit::RotateLeft( roadMask & Tile::AllSideMask , rotation , 4 );
	}

	unsigned MapTile::getFarmLinkMask(int idx) const
	{
		unsigned mask = mTile->farms[ Tile::ToLocalFramIndex( idx , rotation ) ].farmLinkMask;
		assert(( mask & ~Tile::AllFarmMask ) == 0 );
		return FBit::RotateLeft( mask , 2 * rotation , 8 );
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
		return mTile->sides[ FDir::ToLocal( dir , rotation) ].contentFlag;
	}

	unsigned MapTile::getTileContent() const
	{
		return mTile->contentFlag;
	}

	int MapTile::SideNode::getLocalDir() const
	{
		return FDir::ToLocal( index , getMapTile()->rotation );
	}

	int MapTile::FarmNode::getLocalIndex() const
	{
		return FDir::ToLocal( index , getMapTile()->rotation );
	}


}//namespace CAR
