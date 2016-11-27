#include "CAR_PCH.h"
#include "CARMapTile.h"

#include <algorithm>

namespace CAR
{
	MapTile::MapTile( TilePiece const& tile , int rotation) 
		:mTile(&tile)
		,rotation(rotation)
		,renderData(nullptr)
		,towerHeight(0)
		,bridgeMask(0)
		,group(-1)
		,haveHill( false )
	{

		for( int i = 0 ; i < TilePiece::NumSide ; ++i )
		{
			SideNode& node = sideNodes[i];
			node.index      = i;
			node.outConnect = nullptr;
			node.group  = -1;
			int lDir = FDir::ToLocal( i , rotation );
			node.type = mTile->getLinkType( lDir );
			unsigned mask = mTile->getSideLinkMask( lDir );
			assert(( mask & ~TilePiece::AllSideMask ) == 0 );
			node.linkMask = FBit::RotateLeft( mask , rotation , 4 );
		}
		for( int i = 0 ; i < TilePiece::NumFarm ; ++i )
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
		int lDir = TilePiece::ToLocalFramIndex( idx , rotation );

		unsigned mask = 0;
		unsigned sideMask = mTile->farms[ lDir ].sideLinkMask;
		int lDirTest;
		while( FBit::MaskIterator< FDir::TotalNum >( sideMask , lDirTest ) )
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
		unsigned bitCenter = mask & TilePiece::CenterMask;
		return bitCenter | FBit::RotateLeft( mask & TilePiece::AllSideMask , rotation , 4 );
	}

	unsigned MapTile::calcRoadMaskLinkCenter() const
	{
		unsigned roadMask = 0;
		for( int i = 0 ; i < FDir::TotalNum ; ++i)
		{
			if ( mTile->sides[i].roadLinkDirMask & TilePiece::CenterMask )
				roadMask |= mTile->sides[i].roadLinkDirMask;
		}
		unsigned bitCenter = roadMask & TilePiece::CenterMask;
		return bitCenter | FBit::RotateLeft( roadMask & TilePiece::AllSideMask , rotation , 4 );
	}

	unsigned MapTile::getFarmLinkMask(int idx) const
	{
		unsigned mask = mTile->farms[ TilePiece::ToLocalFramIndex( idx , rotation ) ].farmLinkMask;
		assert(( mask & ~TilePiece::AllFarmMask ) == 0 );
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

	void MapTile::updateSideLink(unsigned linkMask)
	{
		int dir;
		unsigned mask = linkMask;
		while ( FBit::MaskIterator< FDir::TotalNum >( mask , dir ) )
		{
			sideNodes[dir].linkMask = linkMask;
		}
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
