#ifndef CARLevel_h__5285fbff_0e2e_44e4_94bc_34ed6719cb50
#define CARLevel_h__5285fbff_0e2e_44e4_94bc_34ed6719cb50

#include "CARCommon.h"
#include "CARLevelActor.h"

#include <map>
#include <set>

namespace CAR
{
	class Tile;
	class MapTile;


	struct TileSet
	{
		enum EGroup
		{
			eCommon ,
			eRiver ,

			NumGroup ,
		};
		Expansion expansions;
		int    idxDefine;
		Tile*  tile;
		EGroup group;
		uint32 tag;
		int    numPiece;
	};

	typedef std::vector< TileId > TileIdVec;
	class TileSetManager
	{
	public:
		TileSetManager();
		~TileSetManager();

		void  import( ExpansionTileContent const& content );
		TileSet const& getTileSet( TileId tileId ) const {  return mTileMap[ tileId ];  }

		int  getReigsterTileNum() const { return (int)mTileMap.size(); }
		TileIdVec const& getSetGroup( TileSet::EGroup group ){ return mSetMap[ group ]; }


		void     cleanup();

	private:
		void     setupTile( Tile& tile , TileDefine const& tileDef );
		TileSet& createTileSet( TileDefine const& tileDef );
		unsigned calcFarmSideLinkMask( unsigned linkMask );
		

		TileIdVec mSetMap[ TileSet::NumGroup ];
		std::vector< TileSet > mTileMap;
	};




	class Level
	{
	public:
		Level();
		void restart();

		Tile const& getTile( TileId id ) const;
		enum
		{
			eDontCheckRiverConnect = BIT(0),
			eUsageBridage     = BIT(1),
		};
		MapTile*    placeTile( TileId tileId , Vec2i const& pos , int rotation , unsigned flag = 0 );
		MapTile*    placeTileNoCheck( TileId tileId , Vec2i const& pos , int rotation , unsigned flag = 0 );
		MapTile*    findMapTile( Vec2i const& pos );
		bool        canPlaceTile( TileId tileId , Vec2i const& pos , int rotation , unsigned flag = 0 );
		int         getPosibleLinkPos( TileId tileId  , std::vector< Vec2i >& outPos );


		void  DBGPutAllTile( int rotation );

		
		void incCheckCount();

		struct VecCmp
		{
			bool operator() ( Vec2i const& v1 , Vec2i const& v2 ) const
			{
				return ( v1.x < v2.x ) || ( v1.x == v2.x && v1.y < v2.y );
			}
		};
		typedef std::map< Vec2i , MapTile , VecCmp > LevelTileMap;

		LevelTileMap    mMap;
		TileSetManager* mTileSetManager;
		uint32          mCheckCount;
		typedef std::set< Vec2i , VecCmp > PosSet;
		PosSet          mEmptyLinkPosSet;
		
	};



}

#endif // CARLevel_h__5285fbff_0e2e_44e4_94bc_34ed6719cb50