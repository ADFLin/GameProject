#ifndef TTLevel_h__
#define TTLevel_h__

#include "TGrid2D.h"
#include "TVector2.h"
#include "TTable.h"
#include "IntrList.h"

namespace TripleTown
{

	enum ObjectId
	{
		OBJ_ACTOR = -1,
		OBJ_NULL  = 0 ,

		//BASIC
		OBJ_BASIC_START ,
		OBJ_GRASS = OBJ_BASIC_START ,
		OBJ_BUSH ,
		OBJ_TREE ,
		OBJ_HUT ,
		OBJ_HOUSE ,
		OBJ_MANSION ,
		OBJ_CASTLE ,
		OBJ_FLOATING_CASTLE ,
		OBJ_TRIPLE_CASTLE ,
		OBJ_TOMBSTONE ,
		OBJ_CHURCH ,
		OBJ_CATHEDRAL ,
		OBJ_CHEST ,
		OBJ_LARGE_CHEST ,
		OBJ_ROCK     ,
		OBJ_MOUNTAIN ,
		OBJ_BASIC_END ,

		//FUN LAND
		OBJ_FUNLAND_START = OBJ_BASIC_END ,
		OBJ_STOREHOUSE    = OBJ_FUNLAND_START ,
		OBJ_CRATE ,
		OBJ_TIME_MACHINE ,
		OBJ_FUNLAND_END  ,

		//ACTOR
		OBJ_ACTOR_START = OBJ_FUNLAND_END ,
		OBJ_BEAR        = OBJ_ACTOR_START ,
		OBJ_NINJA      ,
		OBJ_ACTOR_END ,

		//TOOL
		OBJ_TOOL_START = OBJ_ACTOR_END ,
		OBJ_CRYSTAL    = OBJ_TOOL_START ,
		OBJ_ROBOT ,
		OBJ_UNDO_MOVE ,
		OBJ_FORTUNE_COOKIE ,
		OBJ_TOOL_END ,
		
	};

	int const NUM_OBJ     = OBJ_TOOL_END;
	int const NUM_BASIC   = OBJ_BASIC_END - OBJ_BASIC_START;
	int const NUM_ACTOR   = OBJ_ACTOR_END - OBJ_ACTOR_START;
	int const NUM_FUNLAND = OBJ_FUNLAND_END - OBJ_FUNLAND_START;
	int const NUM_TOOL    = OBJ_TOOL_END - OBJ_TOOL_START;

	typedef TVector2< int > TilePos;
	static int const DirNum = 4;

	enum LandType
	{
		LT_STANDARD  ,
		LT_PEACEFUL  ,
		LT_DANGERUOS ,
		LT_LAKE      ,
	};

	enum TerrainType
	{
		TT_LAND  = 0 ,
		TT_WATER ,
		///////////
		TT_ROAD  ,
		TT_GRASS ,
	};
	enum ObjectType
	{
		OT_NULL   ,
		OT_BASIC  ,
		OT_ACTOR ,
		OT_TOOL ,
		OT_FUN_LAND ,
	};

	enum ObjectState
	{
		STATE_ALIVE ,
		STATE_DEAD ,
	};

	struct ActorData
	{
		ObjectId    id;
		TilePos     pos;
		intptr_t    userData;
		ObjectState state;
		int         stepBorn;
		int         stepUpdate;
	};

	struct Tile
	{
		ObjectId    id;
		int         meta;
		unsigned    checkCount;
		union
		{
			//for Object
			int     link;
			//for Actor
			int     checkResult;
		};
		TerrainType terrainBase;

		TerrainType getTerrain() const 
		{
			if ( terrainBase == TT_LAND )
				return ( id == OBJ_NULL || id == OBJ_ACTOR ) ? TT_ROAD : TT_GRASS;
			return terrainBase;
		}
		bool     isEmpty()   const { return terrainBase == TT_LAND && id == OBJ_NULL; }
		bool     haveActor() const { return id == OBJ_ACTOR; }
		
	};

	typedef TGrid2D< Tile > TileMap;

	inline unsigned ACTOR_MASK( ObjectId id )
	{
		assert( id >= OBJ_ACTOR_START ); 
		return BIT( id - OBJ_ACTOR_START );
	}

	class Level;
	class ObjectClass
	{
	public:
		ObjectClass( ObjectType type ):mType( type ){}
		ObjectType getType(){ return mType; }
		virtual void setup( Level& level , Tile& tile , TilePos const& pos , ObjectId id ){}
		virtual bool use( Level& level , Tile& tile , TilePos const& pos , ObjectId id ){ return false; }
		virtual void play( Level& level , Tile& tile , TilePos const& pos , ObjectId id ){}
		virtual int  peek( Level& level , Tile& tile , TilePos const& pos , ObjectId id , TilePos posRemove[] ){ return 0; }
		virtual bool effect( Level& level , Tile& tile , TilePos const& pos ){ return false; }
		virtual void onRemove( Level& level , TilePos const& pos , ObjectId id ){}
	private:
		ObjectType mType;
	};


	class AnimManager
	{
	public:
		virtual void addBasicObject( TilePos const& pos , ObjectId id ){}
		virtual void removeObject( TilePos const& pos , ObjectId id ){}
		virtual void addActor( TilePos const& pos , ObjectId id ){}
		virtual void prevRemoveActor( Tile const& tile , ActorData const& actor ){}
		virtual void removeActor( TilePos const& pos , ObjectId id ){}
		virtual void moveActor( ObjectId id , TilePos const& posFrom , TilePos const& posTo ){}

		virtual void prevCreateLand(){}
		virtual void postCreateLand(){}

	};

	struct ObjectInfo
	{
		ObjectId     id;
		ObjectClass* typeClass;
		ObjectId     idUpgrade;
		int          numUpgrade;
		int          randRate;
	};

	class World
	{



	};
	
	class Level
	{
	public:
		Level();

		void         create( LandType type );
		void         restart();
		
		AnimManager& getAnimManager(){ return *mAnimMgr; }
		void         setAnimManager( AnimManager* manager );

		int          getUpgradeNum( ObjectId id );
		ObjectId     getUpgradeId( ObjectId id );

		ObjectId     getQueueObject( int order = 0 );
		void         setQueueObject( ObjectId id , int order = 0 );
		void         nextQueueObject();

		void         clickTile( TilePos const& pos , bool canUse = true );

		bool         useObject( TilePos const& pos , ObjectId id );
		int          peekObject( TilePos const& pos , ObjectId id , TilePos posRemove[] );
		void         addObject( TilePos const& pos , ObjectId id );
		bool         isVaildMapRange( TilePos const& pos ) const {  return mMap.checkRange( pos.x , pos.y );  }

		int          getEmptyTileNum() const { return mNumEmptyTile; }


		void         moveActor( ActorData& e , TilePos const& posTo );
		void         killActor( ActorData& e ){  killActor( getTile( e.pos ) , e );  }
		int          killConnectActor( ActorData& e , unsigned bitMask , TilePos& posNew ){  return killConnectActor( getTile( e.pos ) , e.pos , bitMask , posNew );  }

		
		TileMap const& getMap(){ return mMap; }
		Tile const&    getTile( TilePos const& pos ) const { return mMap.getData( pos.x , pos.y ); }
		Tile&          getTile( TilePos const& pos )       { return mMap.getData( pos.x , pos.y ); }


		int        addActor( Tile& tile , TilePos const& pos , ObjectId id );
		ActorData& getActor( Tile const& tile ){  assert( tile.haveActor() );  return mActorStorage[ tile.meta ]; }
		void       removeObject( Tile& tile , TilePos const& pos );

		

		ObjectId   getObjectId( Tile const& tile ){ return  tile.haveActor() ? getActor( tile ).id : tile.id;  }

		bool       checkEffect( TilePos const& pos );
	private:


		int     countConnectLink( TilePos const& pos , ObjectId id , unsigned dirBitSkip , unsigned& dirBitUsed );

		bool    useObjectImpl( Tile& tile , TilePos const& pos , ObjectId id );
		
		void    runStep( Tile& tile , TilePos const& pos );

		int     getLinkNum( TilePos const& pos );
		int     getLinkRoot( TilePos const& pos );
		int     linkTile( TilePos const& pos , ObjectId id  , int& idxRoot );
		void    relinkNeighborTile( TilePos const& pos , ObjectId id );

		void    rebuildLink( TilePos const& pos , ObjectId id );
		int     relink_R( TilePos const& pos , ObjectId id , int idxRoot );
		

		bool   testCheckCount( Tile& tile );
		void   checkActor( Tile& tile , TilePos const& pos );

		int    getConnectObjectPos( TilePos const& pos , ObjectId id , TilePos posConnect[] );
		int    getConnectObjectPos_R( TilePos const& pos , ObjectId id , TilePos posConnect[] );

		void   removeConnectObject( Tile& tile , TilePos const& pos );
		void   removeConnectObject_R( TilePos const& pos , ObjectId id );
		void   removeConnectObjectImpl_R( Tile& tile , TilePos const& pos , ObjectId id );

		int    markObjectWithLink( Tile& tile , TilePos const& pos , ObjectId id );
		void   markObject( Tile& tile , TilePos const& pos , ObjectId id );

		TerrainType getTerrain( TilePos const& pos )
		{
			if ( !isVaildMapRange( pos ) )
				return mBGTerrain;
			return getTile( pos ).getTerrain();
		}
		void    setTerrain( TilePos const& pos , TerrainType type );
		bool    updateActorState( ActorData& e );
		int     killConnectActor( Tile& tile , TilePos const& pos , unsigned bitMask , TilePos& posNew );

		struct KillInfo
		{
			TilePos pos;
			int step;
		};
		int     killConnectActor_R( TilePos const& pos , unsigned bitMask , KillInfo& info );
		void    killActor( Tile& tile , ActorData& e );

		

		LandType    mLandType;
		int         mStep;
		int         mNumEmptyTile;
		TileMap     mMap;
		
		Tile*   getConnectTile( TilePos const& pos , int dir );


		
		void      resetObjectQueue();
		static int const ObjQueueSize = 5;
		ObjectId  mObjQueue[ ObjQueueSize ];
		int       mIdxNextUse;

		AnimManager* mAnimMgr;

		
		void    removeActor( Tile& tile );
		void    moveActor( Tile& tile , ActorData& e , TilePos const& toPos );
		
		

		typedef std::vector< int >       IdxList;
		typedef std::vector< ActorData > EntityMap;
		EntityMap  mActorStorage;
		IdxList    mIdxUpdateQueue;
		int        mIdxFreeEntity;

		struct ProduceInfo
		{
			ObjectId id;
			int      rate;
		};
		void      setupRandProduce( ProduceInfo const replace[] , int num );
		ObjectId  randomObject();
		
		TerrainType mBGTerrain;
		ProduceInfo mRandProduceMap[ NUM_OBJ ];
		int         mNumRandProduces;
		int         mTotalRate;

		void        increaseCheckCount();
		unsigned      mCheckCount;
		static ObjectInfo const& getInfo( ObjectId id );
		


		friend class Scene;

		friend class ECFunLand;
		friend class ECBasicLand;
		friend class ECActor;
		friend class ECTool;
		friend class ECCrystal;
		friend class ECBear;
	};

}

#endif // TTLevel_h__
