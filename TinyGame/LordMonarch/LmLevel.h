#ifndef LmLevel_h__
#define LmLevel_h__

#include "TGrid2D.h"

namespace LordMonarch
{
	class Unit
	{
	public:
		virtual void tick() = 0;
	};

	enum TerrainID
	{
		TID_DIRT  = 0 ,

		TID_VILLAGE   ,
		TID_TERRITORY ,
		TID_GRASS     ,
		TID_CAVE      ,
		TID_CAVE_BLOCKED ,
		TID_BRIDGE ,

		TID_FOREST ,
		TID_RIVER  ,
		TID_HILL   ,
	};

	bool isBlocked( TerrainID id )
	{
		switch( id )
		{
		case TID_CAVE_BLOCKED:
		case TID_RIVER:
		case TID_FOREST:
		case TID_HILL:
			return true;
		}
		return false;
	}


	struct Tile
	{
		int id;
		int meta;
	};

	struct UnitData
	{
		int   score;
		int   owner;
		Vec2i pos;
		int   next;
	};

	class IPathFinder
	{
	public:
		virtual IPath* findPath( Vec2i const& start , Vec2i const& goal ) = 0;
		virtual IPath* findTile( Vec2i const& pos , TerrainID id , int maxDist ) = 0;
	};

	class CPathFinder
	{
	public:
		virtual IPath* findPath( Vec2i const& start , Vec2i const& goal )
		{

		}
		virtual IPath* findTile( Vec2i const& start , TerrainID id , int maxDist )
		{




		}

		
		bool findTileImpl( int x , int y , int dir , FindInfo const info )
		{
			if ( mTestCount.getData( x , y ) == mCurCount )
				return;

			struct FindNode
			{
				Vec2i pos;
			};
		}


		struct FindInfo
		{
			int maxDist;
			int id;
			std::vector< Vec2i > nodes;
		};

		TGrid2D<Tile>& getMap();
		int     mCurCount;

		TGrid2D mTestCount;
	};
	class IPath
	{
	public:
		virtual int   getNodeNum();
		virtual Vec2i getNode();
	};

	template< class T >
	class TArrayList
	{
		typedef int IndexType;

		

		void      remove( IndexType idx )
		{
		}
		IndexType fetchFree()
		{
			IndexType idx;
			if ( mIdxFree == -1 )
			{
				idx = mNodeStorage.size();
				mNodeStorage.push_back( Node() );
			}
			else
			{
				idx = mIdxFree;
				mIdxFree = mNodeStorage[ idx ].next;

			}
			Node& node = mNodeStorage[ idx ];

			mIdxUse   = idx;
			node.next = mIdxUse;

			return idx;
		}
		struct Node
		{
			T value;
			IndexType next;
		};

		std::vector< Node > mNodeStorage;
		IndexType mIdxUse;
		IndexType mIdxFree;
	}
	struct Action
	{
		enum Id
		{
			eAuto    ,
			eStandBy ,
			eDefend  ,
			eDestroy ,
			eBuildVillage   ,
			eBuildBarricade ,
			eBuildBridge    ,
			eClearLand ,
			eClearPath ,
			eDismantle ,
			eStealCave ,
		};
		Id    id;
		Vec2i posDest;
	};
	struct ActorData : public UnitData
	{
		Action::Id comAction;
		Action::Id curAction;
	};

	class Kingdom
	{
	public:
		struct Status
		{
			int countTerritory;
			int countVillege;
			int totalPoplution;
			int momey;
		};

		Status& getStatus(){ return mStatus; }

		void setTaxRate( int rate )
		{
			mTaxRate = rate;
			if ( mTaxRate > 30 )
				mTaxRate = 30;
		}
		int  getTaxRate() const { mTaxRate; }
		int  getBaseDecRate( int population , int numTerritory )
		{
			//IMPL
			return 0;
		}

		int  getId(){ return mId; }
		void update()
		{
			mStatus.momey += mTaxRate * ( mStatus.countTerritory + mStatus.countVillege ) / 100;
		}

		void addTerritory( Tile& tile )
		{
			assert( tile.id == TID_DIRT );
			tile.id   = TID_TERRITORY;
			tile.meta = getId(); 
			getStatus().countTerritory += 1;
		}

		void removeTerritory( Tile& tile )
		{
			assert( tile.id == TID_TERRITORY );
			tile.id  = TID_DIRT;
			getStatus().countTerritory -= 1;
		}

		void addVillage( Tile& tile )
		{
			assert( tile.id == TID_DIRT );
			tile.id = TID_VILLAGE;
			tile.meta = getId();
			getStatus().countVillege += 1;
		}

		void removeVillage( Tile& tile )
		{
			assert( tile.id == TID_VILLAGE );
			tile.id  = TID_DIRT;
			getStatus().countVillege -= 1;
		}

	
		int    mId;
		Status mStatus;
		int    mTaxRate;
	};
	Vec2i const& linkOffset( int dir )
	{
		static Vec2i const offset[] =
		{
			Vec2i(1,0) , Vec2i(1,1) , Vec2i(0,1) , Vec2i(-1,1) , 
			Vec2i(-1,0) ,Vec2i(-1,-1) , Vec2i(0,-1) ,Vec2i(1,-1) ,
		};
		return offset[dir];
	}
	class Level
	{
	public:

		Tile& getTile( Vec2i const& pos ){  return mMap.getData( pos.x , pos.y );  }

		bool  produceUnit( Vec2i const& pos , int power )
		{

			return false;
		}

		bool  canBuildVillage( Vec2i const& pos )
		{
			if ( getTile( pos ).id != TID_DIRT )
				return false;

			for( int i = 0 ; i < 8 ; ++i )
			{
				if ( getTile( pos + linkOffset( i ) ).id == TID_VILLAGE )
					return false;
			}
			return true;
		}
		void  updateUnit( ActorData& actorData )
		{
			Tile& curTile = getTile( actorData.pos );
			if ( curTile.id != TID_VILLAGE )
			{
				if ( actorData.score >= 128 )
				{

				}


			}

			switch( actorData.curAction )
			{
			case Action::eAuto:
				if ( canBuildVillage( actorData.pos ) )
				{
				
				}
			}
		}

		bool  isAllies( int id1 , int id2 )
		{
			return false;
		}
		bool  onUnitMoveTile( ActorData& actorData )
		{
			Tile& tile = getTile( actorData.pos );
			switch( tile.id )
			{
			case TID_TERRITORY:
				if ( tile.meta != actorData.owner &&
					 !isAllies( tile.meta , actorData.owner ) )
				{
					Kingdom& other = getPlayer( tile.meta );
					other.removeTerritory( tile );
				}
			case TID_VILLAGE:
				{
					UnitData& villageData = getUnitData( tile.meta );
					if ( actorData.score > villageData.score )
					{
						Kingdom& other = getPlayer( villageData.owner );
						other.removeVillage( tile );
						actorData.score -= villageData.score;
					}
					else
					{

					}
				}
			}

		}

		UnitData& getUnitData( int id ){  return mUnitData[ id ]; }

		void updateUnit( ActorData& actorData )
		{

		}
		void update()
		{
			int  index = mIdxUnitUse;
			int* prevIndex = &mIdxUnitUse;
			while( index != -1 )
			{
				UnitData& bomb = mUnitData[ index ];
				if ( !updateBomb( bomb ) )
				{
					int next = bomb.next;
					bomb.next    = mIdxUnitFree;
					mIdxUnitFree = index;
					*prevIndex = next;
					index = next;
				}
				else
				{
					index = bomb.next;
					prevIndex = &bomb.next;
				}
			}
		}
		std::vector< UnitData > mUnitData;
		int mIdxUnitFree;
		int mIdxUnitUse;

		void  updateVillage( Tile& tile , UnitData& unitData )
		{
			assert( tile.id == TID_VILLAGE );

			Kingdom& player = getPlayer( unitData.owner );

			int numTerritory = 0;
			int tileId[8];
			for( int i = 0 ; i < 8; ++i )
			{
				Vec2i pos = unitData.pos + linkOffset( i );
				tileId[i] = getTile( pos ).id;
				if ( getTile( pos ).id == TID_TERRITORY )
					++numTerritory;
			}
			
			unitData.score += ( numTerritory + 1 ) - player.getBaseDecRate( unitData.score , numTerritory );
			int const TerritoryDirMap[] =
			{
				4 , 0 , 6 , 2 ,
				5 , 3 , 7 , 1 ,
			};
			if ( unitData.score > 16 * ( numTerritory + 1 ) )
			{
				int idx = 0;
				for( ; idx < 8 ; ++idx )
				{
					if ( tileId[ TerritoryDirMap[idx] ] == TID_DIRT )
						break;
				}
				if ( idx != 8 )
				{
					Vec2i pos = unitData.pos + linkOffset( TerritoryDirMap[ idx ] );
					player.addTerritory( tile );
					++numTerritory;
				}
				else if ( produceUnit( unitData.pos , unitData.score ) )
				{
					unitData.score = 0;
				}
				else if ( unitData.score > 255 )
				{
					unitData.score = 255;
				}
			}
		}
		Kingdom& getPlayer( int id ){ return mPlayers[id]; }
		TGrid2D< Tile > mMap;
		Kingdom mPlayers[ 4 ];
	};




}//namespace LordMonarch

#endif // LmLevel_h__
