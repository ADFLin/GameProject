#ifndef CARMapTile_h__a22f3df0_8ee3_4ee1_be77_0ae02aaea8c3
#define CARMapTile_h__a22f3df0_8ee3_4ee1_be77_0ae02aaea8c3

#include "CARCommon.h"
#include "CARLevelActor.h"

namespace CAR
{
	
	int const IndexHalflingSide = 2;

	class TilePiece
	{
	public:
		static int const NumSide = FDir::TotalNum;
		static int const NumFarm = FDir::TotalNum * 2;
		static unsigned const AllSideMask = 0xf;
		static unsigned const AllFarmMask = 0xff;
		static unsigned const CenterMask = BIT( FDir::TotalNum );

		struct SideData
		{
			SideType linkType;
			uint32   contentFlag;
			uint32   linkDirMask;
			uint32   roadLinkDirMask;
		};

		struct FarmData
		{
			uint32  farmLinkMask;
			uint32  sideLinkMask;
		};

		TileId    id;
		SideData  sides[ NumSide ];
		FarmData  farms[ NumFarm ];
		uint32    contentFlag;

		SideType getLinkType( int lDir ) const{  return sides[lDir].linkType;  }

		unsigned getSideLinkMask( int lDir ) const {  return sides[lDir].linkDirMask;;  }
		unsigned getRoadLinkMask( int lDir ) const {  return sides[lDir].roadLinkDirMask;  }
		unsigned getFarmLinkMask( int idx  ) const {  return farms[idx].farmLinkMask;  }

		bool     canRemoveFarm( int lDir ) const {  return CanRemoveFarm(sides[lDir].linkType);  }
		bool     isSideLinked( int lDirA , int lDirB ) const {  return !!( sides[lDirA].linkDirMask & BIT(lDirB ) );  }
		bool     canLinkCity(int lDir) const { return CanLinkCity(sides[lDir].linkType); }
		bool     canLinkRiver( int lDir ) const { return CanLinkRiver(sides[lDir].linkType); }
		bool     canLinkRoad( int lDir ) const { return CanLinkRoad(sides[lDir].linkType); }
		bool     canLinkFarm( int lDir ) const {  return CanLinkFarm( sides[lDir].linkType );  }
		bool     isSemiCircularCity( int lDir ) const;
		bool     isHalflingType() const {  return !!(contentFlag & TileContent::eHalfling);  }
		bool     haveSideType( SideType type ) const;
		bool     haveRiver() const { return haveSideType( SideType::eRiver ); }

		static bool CanLink( SideType typeA , SideType typeB );

		static bool CanLink( TilePiece const& tileA , int lDirA , TilePiece const& tileB , int lDirB )
		{
			return CanLink( tileA.sides[lDirA].linkType , tileB.sides[lDirB].linkType );
		}

		static bool CanLinkCity(SideType type)
		{
			return type == SideType::eCity;
		}
		static bool CanLinkRiver(SideType type)
		{
			return type == SideType::eRiver;
		}
		static bool CanLinkRoad(SideType type)
		{
			return type == SideType::eRoad;
		}
		static bool CanLinkFarm( SideType type )
		{
			unsigned const FarmLinkMask = BIT( SideType::eField ) | BIT( SideType::eRoad ) | BIT( SideType::eRiver ) | BIT( SideType::eGermanCastle );
			return !!( FarmLinkMask & BIT( type ) );
		}

		static bool CanRemoveFarm(SideType type)
		{
			return !!(BIT(type) & (BIT(SideType::eCity) | BIT(SideType::eGermanCastle)));
		}

		static bool CanLinkBridge( SideType type )
		{
			return !!(BIT(type) & (BIT(SideType::eField) | BIT(SideType::eRoad)));
		}

		static int FarmSideDir( int idx ){ return idx / 2; }
		static int DirToFarmIndexFrist( int dir ){ return 2 * dir; }

		static int ToLocalFarmIndex( int idx , int roatation )
		{
			return ( idx - 2 * roatation + NumFarm ) % NumFarm;
		}
		static int FarmIndexConnect( int idx )
		{
			static int const conMap[] = { 5 , 4 , 7 , 6 , 1 , 0 , 3 , 2 };
			return conMap[idx];
		}

	};


	class MapTile : public ActorContainer
	{
	public:
		MapTile( TilePiece const& tile , int rotation );

		TileId getId() const { return mTile->id;  }
		bool   isHalflingType() const { return mTile->isHalflingType(); }

		TilePiece::SideData const& getHalflingSideData() { assert(mTile->isHalflingType()); return mTile->sides[IndexHalflingSide]; }

		void   addActor( LevelActor& actor );
		void   removeActor(LevelActor& actor);

		void connectSide( int dir , MapTile& data );
		void connectFarm( int idx , MapTile& data );

		SideType getLinkType( int dir ) const;

		unsigned getSideLinkMask( int dir ) const;
		unsigned getRoadLinkMask( int dir ) const;
		unsigned getFarmLinkMask( int idx ) const;

		unsigned getCityLinkFarmMask(int idx) const;

		int      getSideGroup( int dir ) const;
		int      getFarmGroup( int idx ) const;

		bool     have(TileContent::Enum context) const
		{
			return !!(mTile->contentFlag & context);
		}
		unsigned getSideContnet( int dir ) const;
		unsigned getTileContent() const;

		unsigned calcSideRoadLinkMeskToCenter() const;

		static unsigned LocalToWorldRoadLinkMask( unsigned mask , int rotation );
		static unsigned LocalToWorldSideLinkMask( unsigned mask , int rotation );
		static unsigned LocalToWorldFarmLinkMask( unsigned mask , int rotation );

		void     addBridge( int dir );
		void     margeHalflingTile(TilePiece const& tile);
		void     updateSideLink( unsigned linkMask );
		bool     isSemiCircularCity( int dir );

		void    removeLinkMask( int dir )
		{
			for( auto iter = TBitMaskIterator< FDir::TotalNum >( sideNodes[dir].linkMask ); iter ; ++iter )
			{
				sideNodes[iter.index].linkMask &= ~BIT(dir);
			}
		}

		struct SideNode
		{
			int       index;
			int       group;
			SideNode* outConnect;
			SideType  type;
			unsigned  linkMask;

			int      getLocalDir() const;

			void     connectNode( SideNode& node )
			{
				node.outConnect = this;
				this->outConnect = &node;
			}
			MapTile const* getMapTile() const
			{
				ptrdiff_t offset = offsetof( MapTile , sideNodes ) + index * sizeof( SideNode );
				return reinterpret_cast< MapTile* >( intptr_t( this ) - offset );
			}
			MapTile* getMapTile()
			{
				ptrdiff_t offset = offsetof( MapTile , sideNodes ) + index * sizeof( SideNode );
				return reinterpret_cast< MapTile* >( intptr_t( this ) - offset );
			}
		};

		struct FarmNode
		{
			int       index;
			int       group;
			FarmNode* outConnect;

			int      getLocalIndex() const;

			void     connectNode( FarmNode& node )
			{
				node.outConnect = this;
				this->outConnect = &node;
			}
			MapTile const* getMapTile() const
			{
				ptrdiff_t offset = offsetof( MapTile , farmNodes ) + index * sizeof( FarmNode );
				return reinterpret_cast< MapTile* >( intptr_t( this ) - offset );
			}
			MapTile* getMapTile()
			{
				ptrdiff_t offset = offsetof(MapTile, farmNodes) + index * sizeof(FarmNode);
				return reinterpret_cast<MapTile*>(intptr_t(this) - offset);
			}
		};

		Vec2i       pos;
		int         rotation;
		SideNode    sideNodes[ TilePiece::NumSide ];
		FarmNode    farmNodes[ TilePiece::NumFarm ];
		int         group;
		int         towerHeight;
		uint8       bridgeMask;
		void*       userData;
		bool        haveHill;
		//HalflingTile
		TileId      mergedTileId[2];

		//////////
		TilePiece const* mTile;
		unsigned    checkCount;
	};


}//namespace CAR

#endif // CARMapTile_h__a22f3df0_8ee3_4ee1_be77_0ae02aaea8c3
