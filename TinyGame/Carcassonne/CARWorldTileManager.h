#ifndef CARWorldTileManager_h__fddd9821_8488_4cd0_b838_a9ed1684a993
#define CARWorldTileManager_h__fddd9821_8488_4cd0_b838_a9ed1684a993

#include "CARCommon.h"
#include "CARLevelActor.h"

#include <map>
#include <set>

namespace CAR
{
	class TilePiece;
	class MapTile;

	struct TileType
	{
		enum Enum
		{
			eNormal ,
			eDouble ,
			eHalfling ,
		};
	};

	struct TileSet
	{
		enum EGroup
		{
			eCommon ,
			eRiver ,
			eGermanCastle ,

			NumGroup ,
		};
		Expansion      expansions;
		TileType::Enum type;
		int            idxDefine;
		TilePiece*     tiles;
		EGroup         group;
		uint32         tag;
		int            numPiece;
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
		TileIdVec const& getGroup( TileSet::EGroup group ){ return mSetMap[ group ]; }


		void     cleanup();

	private:
		void     setupTile( TilePiece& tile , TileDefine const& tileDef );
		TileSet& createSingleTileSet( TileDefine const& tileDef );
		TileSet& createDoubleTileSet(TileDefine const tileDef[] , TileSet::EGroup group );
		unsigned calcFarmSideLinkMask( unsigned linkMask );
		

		TileIdVec mSetMap[ TileSet::NumGroup ];
		std::vector< TileSet > mTileMap;
	};



	struct PutTileParam
	{
		PutTileParam(){ init(); }

		void init()
		{
			usageBridge = false;
			checkRiverConnect = false;
			checkRiverDirection = true;
			dirNeedUseBridge = -1;
			idxTileUseBridge = -1;
		}

		uint8 usageBridge : 1;
		uint8 checkRiverConnect : 1;
		uint8 checkRiverDirection: 1;

		int   dirNeedUseBridge;
		int   idxTileUseBridge;
	};

	class WorldTileManager
	{
	public:
		WorldTileManager();
		void       restart();

		TilePiece const& getTile( TileId id , int idx = 0 ) const;
		int         placeTile( TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param , MapTile* outMapTile[] );
		int         placeTileNoCheck( TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param , MapTile* outMapTile[] );
		bool        canPlaceTile( TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param );
		int         getPosibleLinkPos( TileId tileId , std::vector< Vec2i >& outPos , PutTileParam& param );
		MapTile*    findMapTile( Vec2i const& pos );
		bool        isEmptyLinkPos( Vec2i const& pos );
	private:
		bool        canPlaceTileList( TileId tileId , int numTile , Vec2i const& pos , int rotation , PutTileParam& param );
		MapTile*    placeTileInternal( TilePiece const& tile , Vec2i const& pos , int rotation , PutTileParam& param );


		//
		struct PlaceInfo
		{
			int  numTileLink;
			int  numRiverConnect;
			int  dirRiverLink;
			bool bNeedCheckRiverConnectDir;
			MapTile* riverLink;
		};
		bool        canPlaceTileInternal( TilePiece const& tile , Vec2i const& pos , int rotation , PutTileParam& param , PlaceInfo& info );
		bool        checkRiverLinkDirection( Vec2i const& pos , int dirLink , int dir );
		void        incCheckCount();

		struct VecCmp
		{
			bool operator() ( Vec2i const& v1 , Vec2i const& v2 ) const
			{
				return ( v1.x < v2.x ) || ( v1.x == v2.x && v1.y < v2.y );
			}
		};

	public:
		typedef std::map< Vec2i , MapTile , VecCmp > WorldTileMap;

		WorldTileMap    mMap;
		TileSetManager* mTileSetManager;
		uint32          mCheckCount;
		typedef std::set< Vec2i , VecCmp > PosSet;
		PosSet          mEmptyLinkPosSet;
		
	};



}

#endif // CARWorldTileManager_h__fddd9821_8488_4cd0_b838_a9ed1684a993