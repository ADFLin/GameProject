#ifndef CARMapTile_h__a22f3df0_8ee3_4ee1_be77_0ae02aaea8c3
#define CARMapTile_h__a22f3df0_8ee3_4ee1_be77_0ae02aaea8c3

#include "CARCommon.h"
#include "CARLevelActor.h"

namespace CAR
{



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

		bool canRemoveFarm( int lDir ) const
		{
			return sides[lDir].linkType == SideType::eCity;
		}

		bool     isSideLinked( int lDirA , int lDirB ) const
		{  return ( sides[lDirA].linkDirMask & BIT(lDirB ) ) != 0;  }
		bool     canLinkCity( int lDir ) const
		{  return sides[lDir].linkType == SideType::eCity;  }
		bool     canLinkRiver( int lDir ) const
		{  return sides[lDir].linkType == SideType::eRiver;  }
		bool     canLinkRoad( int lDir ) const
		{  return sides[lDir].linkType == SideType::eRoad;  }
		bool     canLinkFarm( int lDir ) const {  return CanLinkFarm( sides[lDir].linkType );  }
		bool     isSemiCircularCity( int lDir ) const
		{
			SideData const& sideData = sides[lDir];
			assert( sideData.linkType == SideType::eCity );
			return ( sideData.linkDirMask == FBit::Extract( sideData.linkDirMask ) ) && 
				   ( ( sideData.contentFlag & SideContent::eNotSemiCircularCity ) == 0 );
		}
		bool  haveSideType( SideType type )
		{
			for( int i = 0 ; i < NumSide ; ++i )
			{
				if ( sides[i].linkType == type )
					return true;
			}
			return false;
		}

		bool  haveRiver(){ return haveSideType( SideType::eRiver ); }

		static bool CanLink( SideType typeA , SideType typeB )
		{
			if ( typeA == typeB )
				return true;
			unsigned AbbeyLinkMask = BIT( SideType::eAbbey ) | BIT( SideType::eCity ) | BIT( SideType::eRoad ) | BIT( SideType::eField );
			if ( typeA == SideType::eAbbey && ( AbbeyLinkMask & BIT(typeB)) != 0 )
				return true;
			if ( typeB == SideType::eAbbey && ( AbbeyLinkMask & BIT(typeA)) != 0 )
				return true;
			return false;
		}
		static bool CanLink( TilePiece const& tileA , int lDirA , TilePiece const& tileB , int lDirB )
		{
			return CanLink( tileA.sides[lDirA].linkType , tileB.sides[lDirB].linkType );
		}
		static bool CanLinkFarm( SideType type )
		{
			return type == SideType::eField || type == SideType::eRoad || type == SideType::eRiver;
		}
		static bool CanLinkBridge( SideType type )
		{
			return type == SideType::eField || type == SideType::eRoad;
		}

		static int FarmSideDir( int idx ){ return idx / 2; }
		static int DirToFramIndexFrist( int dir ){ return 2 * dir; }

		static int ToLocalFramIndex( int idx , int roatation )
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
		MapTile();

		TileId getId() const { return mTile->id;  }
		void addActor( LevelActor& actor );
		void removeActor(LevelActor& actor);

		void connectSide( int dir , MapTile& data );
		void connectFarm( int idx , MapTile& data );

		SideType getLinkType( int dir ) const;

		unsigned getSideLinkMask( int dir ) const;
		unsigned getRoadLinkMask( int dir ) const;
		unsigned getFarmLinkMask( int idx ) const;
		unsigned getCityLinkFarmMask( int idx ) const;

		int      getSideGroup( int dir );
		int      getFarmGroup( int idx );

		unsigned getSideContnet( int dir ) const;
		unsigned getTileContent() const;

		unsigned calcRoadMaskLinkCenter() const;

		void     addBridge( int dir );

		void     updateSideLink( unsigned linkMask );

		void    removeLinkMask( int dir )
		{
			unsigned mask = sideNodes[dir].linkMask;
			int linkDir;
			while( FBit::MaskIterator< FDir::TotalNum >( mask , linkDir ) )
			{
				sideNodes[linkDir].linkMask &= ~BIT(dir);
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
				ptrdiff_t offset = offsetof( MapTile , farmNodes ) + index * sizeof( FarmNode );
				return reinterpret_cast< MapTile* >( intptr_t( this ) - offset );
			}
		};

		Vec2i       pos;
		int         rotation;
		SideNode    sideNodes[ TilePiece::NumSide ];
		FarmNode    farmNodes[ TilePiece::NumFarm ];
		int         group;
		int         towerHeight;
		uint8       bridgeMask;
		void*       renderData;
		bool        haveHill;

		//////////
		TilePiece const* mTile;
		unsigned    checkCount;
	};


}//namespace CAR

#endif // CARMapTile_h__a22f3df0_8ee3_4ee1_be77_0ae02aaea8c3
