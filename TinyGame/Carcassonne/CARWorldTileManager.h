#ifndef CARWorldTileManager_h__fddd9821_8488_4cd0_b838_a9ed1684a993
#define CARWorldTileManager_h__fddd9821_8488_4cd0_b838_a9ed1684a993

#include "CARCommon.h"
#include "CARLevelActor.h"
#include "CARMapTile.h"

#include "Core/TypeHash.h"

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace CAR
{
	class TilePiece;
	class MapTile;

	struct TileDefine;
	struct ExpansionContent;

	struct TileType
	{
		enum Enum
		{
			eSimple ,
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
			eTest ,
			NumGroup ,
		};
		Expansion      expansion;
		TileType::Enum type;
		int            idxDefine;
		TilePiece*     tiles;
		EGroup         group;
		uint32         tag;
		int            numPieces;
	};

	typedef std::vector< TileId > TileIdVec;


	class TileSetManager
	{
	public:
		TileSetManager() = default;
		~TileSetManager();

		void  addExpansion(Expansion exp);
		void  importData(ExpansionContent const& content);
		
		TileSet const& getTileSet( TileId tileId ) const {  return mTileMap[ tileId ];  }

		unsigned getTileTag( TileId id )
		{
			return getTileSet( id ).tag;
		}
		int  getRegisterTileNum() const { return (int)mTileMap.size(); }
		TileIdVec const& getGroup( TileSet::EGroup group ){ return mSetMap[ group ]; }

		void     cleanup();

	private:
		void     setupTile( TilePiece& tile , TileDefine const& tileDef );
		TileSet& createSingleTileSet( TileDefine const& tileDef , TileSet::EGroup group = TileSet::eCommon );
		TileSet& createDoubleTileSet( TileDefine const tileDef[] , TileSet::EGroup group );
		unsigned calcFarmSideLinkMask( unsigned linkMask );
		

		TileIdVec mSetMap[ TileSet::NumGroup ];
		std::vector< TileSet > mTileMap;
	};



	struct PlaceTileParam
	{
		PlaceTileParam(){ init(); }

		void init()
		{
			canUseBridge = false;
			checkRiverConnect = false;
			checkRiverDirection = true;
			checkDontNearSameTag = false;
			dirNeedUseBridge = -1;
			idxTileUseBridge = -1;
		}

		uint8 canUseBridge : 1;
		uint8 checkRiverConnect : 1;
		uint8 checkRiverDirection: 1;
		uint8 checkDontNearSameTag : 1;
		uint8 checkNeighborCloisterOrShrine : 1;

		int   dirNeedUseBridge;
		int   idxTileUseBridge;
	};


	struct TilePlacePos
	{
		Vec2i location;
		uint8 dirMask;
		uint8 requireBridgeMask;
	};

	class WorldTileManager
	{
	public:
		WorldTileManager( TileSetManager& manager );
		void       restart();

		TilePiece const& getTile( TileId id , int idx = 0 ) const;
		int         placeTile( TileId tileId , Vec2i const& pos , int rotation , PlaceTileParam& param , MapTile* outMapTile[] );
		int         placeTileNoCheck( TileId tileId , Vec2i const& pos , int rotation , PlaceTileParam& param , MapTile* outMapTile[] );
		bool        canPlaceTile( TileId tileId , Vec2i const& pos , int rotation , PlaceTileParam& param );
		int         getPosibleLinkPos( TileId tileId , std::vector< Vec2i >& outPos , PlaceTileParam& param );
		MapTile*    findMapTile( Vec2i const& pos );
		bool        isEmptyLinkPos( Vec2i const& pos );


		bool        isLinkTilePosible( Vec2i const& pos , TileId id );
	
		auto        createWorldTileIterator() const { return MakeIterator(mMap); }
		auto        createEmptyLinkPosIterator() const { return MakeIterator(mEmptyLinkPosSet); }

		int         getNeighborCount(Vec2i const& pos, TileContentMask content);

	private:
		bool        canPlaceTileList( TileId tileId , int numTile , Vec2i const& pos , int rotation , PlaceTileParam& param );
		
		MapTile*    placeTileInternal( TilePiece const& tile , Vec2i const& pos , int rotation , PlaceTileParam& param );


		//
		struct PlaceResult
		{
			int  numTileLink;
			int  numRiverConnect;
			int  dirRiverLink;
			bool bNeedCheckRiverConnectDir;
			MapTile* riverLink;
		};
		bool        canPlaceTileInternal( TilePiece const& tile , Vec2i const& pos , int rotation , PlaceTileParam& param , PlaceResult& result );
		bool        canMergeHalflingTile(TilePiece const& tile, MapTile* halflingTile, PlaceTileParam& param);
		bool        checkRiverLinkDirection( Vec2i const& pos , int dirLink , int dir );


		void        incCheckCount();


		struct VecCmp
		{
			bool operator() ( Vec2i const& v1 , Vec2i const& v2 ) const
			{
				return ( v1.x < v2.x ) || ( !(v2.x < v1.x) && v1.y < v2.y );
			}
		};
		struct VecHasher
		{
			size_t operator()( Vec2i const& v) const
			{
				uint32 result = HashValue(v.x);
				HashCombine(result, v.y);
				return result;
			}
		};

		std::vector< MapTile* > mHalflingTiles;
#if 0
		typedef std::map< Vec2i , MapTile , VecCmp > WorldTileMap;
		typedef std::set< Vec2i , VecCmp > PosSet;
#else
		typedef std::unordered_map< Vec2i, MapTile, VecHasher > WorldTileMap;
		typedef std::unordered_set< Vec2i, VecHasher > PosSet;
#endif

		WorldTileMap    mMap;

		PosSet          mEmptyLinkPosSet;

		TileSetManager* mTileSetManager;
		uint32          mCheckCount;
	};



}

#endif // CARWorldTileManager_h__fddd9821_8488_4cd0_b838_a9ed1684a993