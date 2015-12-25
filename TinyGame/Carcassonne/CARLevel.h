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

		int  getRegisterTileNum() const { return (int)mTileMap.size(); }
		TileIdVec const& getSetGroup( TileSet::EGroup group ){ return mSetMap[ group ]; }


		void     cleanup();

	private:
		void     setupTile( Tile& tile , TileDefine const& tileDef );
		TileSet& createTileSet( TileDefine const& tileDef );
		unsigned calcFarmSideLinkMask( unsigned linkMask );
		

		TileIdVec mSetMap[ TileSet::NumGroup ];
		std::vector< TileSet > mTileMap;
	};



	struct PutTileParam
	{
		PutTileParam()
		{
			usageBridge = 0;
			checkRiverConnect = 0;
			dirNeedUseBridge = -1;
		}
		uint8 usageBridge : 1;
		uint8 checkRiverConnect : 1;

		int   dirNeedUseBridge;
	};

	class Level
	{
	public:
		Level();
		void restart();

		Tile const& getTile( TileId id ) const;
		MapTile*    placeTile( TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param );
		MapTile*    placeTileNoCheck( TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param );
		bool        canPlaceTile( TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param );
		int         getPosibleLinkPos( TileId tileId , std::vector< Vec2i >& outPos , PutTileParam& param );
		MapTile*    findMapTile( Vec2i const& pos );
		bool        isEmptyLinkPos( Vec2i const& pos );

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