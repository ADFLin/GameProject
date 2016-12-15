#ifndef CARWorldTileManager_h__fddd9821_8488_4cd0_b838_a9ed1684a993
#define CARWorldTileManager_h__fddd9821_8488_4cd0_b838_a9ed1684a993

#include "CARCommon.h"
#include "CARLevelActor.h"
#include "CARExpansion.h"
#include "CARMapTile.h"

#include <map>
#include <unordered_map>
#include <set>

namespace CAR
{
	class TilePiece;
	class MapTile;
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

		void  addExpansion(Expansion exp);

		bool  haveUse(Expansion exp) const { return (mUseExpansionMask & BIT(exp)) != 0; }
		void  import( ExpansionContent const& content );
		
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
		TileSet& createSingleTileSet( TileDefine const& tileDef );
		TileSet& createDoubleTileSet(TileDefine const tileDef[] , TileSet::EGroup group );
		unsigned calcFarmSideLinkMask( unsigned linkMask );
		

		TileIdVec mSetMap[ TileSet::NumGroup ];
		std::vector< TileSet > mTileMap;

		unsigned mUseExpansionMask;
	};



	struct PutTileParam
	{
		PutTileParam(){ init(); }

		void init()
		{
			usageBridge = false;
			checkRiverConnect = false;
			checkRiverDirection = true;
			checkDontNearSameTag = false;
			dirNeedUseBridge = -1;
			idxTileUseBridge = -1;
		}

		uint8 usageBridge : 1;
		uint8 checkRiverConnect : 1;
		uint8 checkRiverDirection: 1;
		uint8 checkDontNearSameTag : 1;

		int   dirNeedUseBridge;
		int   idxTileUseBridge;
	};

	class WorldTileManager
	{
	public:
		WorldTileManager( TileSetManager& manager );
		void       restart();

		TilePiece const& getTile( TileId id , int idx = 0 ) const;
		int         placeTile( TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param , MapTile* outMapTile[] );
		int         placeTileNoCheck( TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param , MapTile* outMapTile[] );
		bool        canPlaceTile( TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param );
		int         getPosibleLinkPos( TileId tileId , std::vector< Vec2i >& outPos , PutTileParam& param );
		MapTile*    findMapTile( Vec2i const& pos );
		bool        isEmptyLinkPos( Vec2i const& pos );


		bool        isLinkTilePosible( Vec2i const& pos , TileId id );
	
		auto        createWorldTileIterator() const { return MakeIterator(mMap); }
		auto        createEmptyLinkPosIterator() const { return MakeIterator(mEmptyLinkPosSet); }

	private:
		bool        canPlaceTileList( TileId tileId , int numTile , Vec2i const& pos , int rotation , PutTileParam& param );
		
		MapTile*    placeTileInternal( TilePiece const& tile , Vec2i const& pos , int rotation , PutTileParam& param );


		//
		struct PlaceResult
		{
			int  numTileLink;
			int  numRiverConnect;
			int  dirRiverLink;
			bool bNeedCheckRiverConnectDir;
			MapTile* riverLink;
		};
		bool        canPlaceTileInternal( TilePiece const& tile , Vec2i const& pos , int rotation , PutTileParam& param , PlaceResult& result );
		bool        canMergeHalflingTile(TilePiece const& tile, MapTile* halflingTile, PutTileParam& param);
		bool        checkRiverLinkDirection( Vec2i const& pos , int dirLink , int dir );
		void        incCheckCount();


		struct VecCmp
		{
			bool operator() ( Vec2i const& v1 , Vec2i const& v2 ) const
			{
				return ( v1.x < v2.x ) || ( v1.x == v2.x && v1.y < v2.y );
			}
		};
		struct VecHasher
		{
			static inline uint32 HashCombine(uint32 A, uint32 C)
			{
				uint32 B = 0x9e3779b9;
				A += B;

				A -= B; A -= C; A ^= (C >> 13);
				B -= C; B -= A; B ^= (A << 8);
				C -= A; C -= B; C ^= (B >> 13);
				A -= B; A -= C; A ^= (C >> 12);
				B -= C; B -= A; B ^= (A << 16);
				C -= A; C -= B; C ^= (B >> 5);
				A -= B; A -= C; A ^= (C >> 3);
				B -= C; B -= A; B ^= (A << 10);
				C -= A; C -= B; C ^= (B >> 15);

				return C;
			}
			size_t operator()( Vec2i const& v) const
			{
				return HashCombine(v.x, v.y);
			}
		};
		typedef std::unordered_map< Vec2i , MapTile , VecHasher > WorldTileMap;
		//typedef std::map< Vec2i , MapTile , VecCmp > WorldTileMap;

		std::vector< MapTile* > mHalflingTiles;
		WorldTileMap    mMap;
		TileSetManager* mTileSetManager;
		uint32          mCheckCount;
		typedef std::set< Vec2i , VecCmp > PosSet;
		PosSet          mEmptyLinkPosSet;

	};



}

#endif // CARWorldTileManager_h__fddd9821_8488_4cd0_b838_a9ed1684a993