#ifndef TTLevel_h__
#define TTLevel_h__

#include "Math/TVector2.h"
#include "DataStructure/Grid2D.h"
#include "DataStructure/TTable.h"
#include "DataStructure/IntrList.h"

#include "Serialize/StreamBuffer.h"
#include "DataStreamBuffer.h"

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

		//FUNC LAND
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
		LT_TEST      ,
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
		OT_FUNC_LAND ,
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
		int         stepEvolve;
	};

	struct Tile
	{
		ObjectId    id;
		int         meta;
		unsigned    checkCount;
		bool        bSpecial;
		union
		{
			//for Object
			int     link;
			//for Actor
			int     checkResult;
		};
		TerrainType terrainBase;

		static Tile InitTile()
		{
			Tile tile;
			tile.id = OBJ_NULL;
			tile.meta = 0;
			tile.link = 0;
			tile.bSpecial = false;
			tile.terrainBase = TT_LAND;
			tile.checkCount = 0;
			return tile;
		}

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


	class LevelListener
	{
	public:
		virtual void notifyObjectAdded( TilePos const& pos , ObjectId id ){}
		virtual void notifyObjectRemoved( TilePos const& pos , ObjectId id ){}
		virtual void nodifyActorAdded( TilePos const& pos , ObjectId id ){}

		virtual void notifyActorMoved( ObjectId id , TilePos const& posFrom , TilePos const& posTo ){}

		virtual void notifyWorldRestore(){}

		virtual void prevRemoveActor(Tile const& tile, ActorData const& actor) {}
		virtual void postRemoveActor(TilePos const& pos, ObjectId id) {}
		virtual void prevPrevSetupLand(){}
		virtual void postSetupMap(){}
		virtual void postSetupLand(){}

		virtual void notifyAddPoints( int points ){}
		virtual void notifyAddCoins(int coins) {}

		virtual void notifyStateChanged(){}
	};

	struct ObjectInfo
	{
		ObjectId     id;
		ObjectClass* typeClass;
		ObjectId     idUpgrade;
		int          numUpgrade;
		float        randRate;
		int          points;
	};

	class World
	{



	};

	struct LevelState
	{
	public:

		TileMap     mMap;
		int         mNumEmptyTile;
		int         mStep;


		static int const ObjQueueSize = 5;
		ObjectId    mObjQueue[ObjQueueSize];
		int         mIdxNextUse;


		typedef TArray< int >       IdxList;
		typedef TArray< ActorData > EntityMap;
		EntityMap  mActorStorage;
		IdxList    mIdxUpdateQueue;
		int        mIdxFreeEntity;

		template< class OP >
		void serialize(OP& op)
		{
			uint32 version = 0;
			op & version;

			uint8 sizeX, sizeY;
			if (OP::IsSaving)
			{
				sizeX = mMap.getSizeX();
				sizeY = mMap.getSizeY();
				op & sizeX & sizeY;
			}
			else
			{
				op & sizeX & sizeY;
				mMap.resize(sizeX, sizeY);
			}
			op & IStreamSerializer::MakeSequence(mMap.getRawData(), mMap.getRawDataSize());

			op & mNumEmptyTile & mStep;
			op & mObjQueue & mIdxNextUse;
			op & mActorStorage & mIdxUpdateQueue & mIdxFreeEntity;
		}

	};
	struct PlayerData
	{
	public:
		virtual int  getPoints() const { return 0; }
		virtual int  getCoins() const { return 0; }
		virtual void addPoints(int points) {}
		virtual void addCoins(int coins) {}
	};
	
	class Level : public LevelState
	{
	public:
		Level();

		void         setupLand( LandType type, bool bTestMode = false);
		void         restart();
		
		LevelListener& getListener(){ return *mListener; }
		void           setListener( LevelListener* listener );

		PlayerData&  getPlayerData() { return *mPlayerData; }
		void         setPlayerData(PlayerData& playerData)
		{
			mPlayerData = &playerData;
		}

		int          getUpgradeNum( ObjectId id );
		ObjectId     getUpgradeId( ObjectId id );

		ObjectId     getQueueObject( int order = 0 );
		void         setQueueObject( ObjectId id , int order = 0 );
		void         nextQueueObject();

		void         clickTile( TilePos const& pos , bool canUse = true );

		bool         useObject( TilePos const& pos , ObjectId id );
		int          peekObject( TilePos const& pos , ObjectId id , TilePos posRemove[] );
		void         addObject( TilePos const& pos , ObjectId id , bool bInit = false );
		bool         isMapRange( TilePos const& pos ) const {  return mMap.checkRange( pos.x , pos.y );  }

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

		int    getConnectObjectPos( TilePos const& pos , ObjectId id , TilePos posConnect[] );
		int    getConnectObjectPos_R( TilePos const& pos , ObjectId id , TilePos posConnect[] );

		void   removeConnectObject( Tile& tile , TilePos const& pos );
		void   removeConnectObject_R( TilePos const& pos , ObjectId id );
		void   removeConnectObjectImpl_R( Tile& tile , TilePos const& pos , ObjectId id );

		int    markObjectWithLink( Tile& tile , TilePos const& pos , ObjectId id );
		void   markObject( Tile& tile , TilePos const& pos , ObjectId id );

		TerrainType getTerrain( TilePos const& pos )
		{
			if ( !isMapRange( pos ) )
				return mBGTerrain;
			return getTile( pos ).getTerrain();
		}
		void    setTerrain( TilePos const& pos , TerrainType type );
		int     killConnectActor( Tile& tile , TilePos const& pos , unsigned bitMask , TilePos& posNew );
		int     killAllActors(unsigned bitMask);

		struct KillInfo
		{
			TilePos pos;
			int     step;
		};
		int     killConnectActor_R( TilePos const& pos , unsigned bitMask , KillInfo& info );
		void    killActor( Tile& tile , ActorData& e );

		void     addPoints(ObjectId id);
		LandType    mLandType;

		Tile*   getConnectTile( TilePos const& pos , int dir );

		TerrainType mBGTerrain;

		void      resetObjectQueue();


		LevelListener* mListener;
		PlayerData*    mPlayerData;

		
		void    removeActor( Tile& tile );
		void    moveActor( Tile& tile , ActorData& e , TilePos const& toPos );
	
	public:
		bool redo()
		{
			int index = mIndexLastStepState + 1;
			if (index == ARRAY_SIZE(mStepStates))
			{
				index = 0;
			}
			auto& state = mStepStates[index];
			if (state.step != mStep + 1)
				return false;

			loadState(state);
			mIndexLastStepState = index;
			return true;
		}

		bool undo()
		{
			int index = mIndexLastStepState - 1;
			if (index < 0)
			{
				index += (int)ARRAY_SIZE(mStepStates);
			}
			auto& state = mStepStates[index];
			if (state.step != mStep - 1)
				return false;

			loadState(state);
			mIndexLastStepState = index;
			return true;
		}

		struct  StateData
		{
			int step;
			TArray<uint8> buffer;
		};

		void recordStepState()
		{
			++mIndexLastStepState;
			if (mIndexLastStepState == ARRAY_SIZE(mStepStates))
				mIndexLastStepState = 0;

			saveState(mStepStates[mIndexLastStepState]);
		}
		void saveState(StateData& state)
		{
			state.step = mStep;
			state.buffer.clear();
			auto bufferSerializer = CreateBufferSerializer<ArrayWriteBuffer>(state.buffer);
			serialize( IStreamSerializer::WriteOp(bufferSerializer));
		}

		void loadState(StateData& state)
		{
			auto bufferSerializer = CreateBufferSerializer<SimpleReadBuffer>(MakeConstView(state.buffer));
			serialize(IStreamSerializer::ReadOp(bufferSerializer));

			mListener->notifyStateChanged();
		}

		int mIndexLastStepState = 0;
		StateData mStepStates[32];


		struct ProduceInfo
		{
			ObjectId id;
			float    rate;
		};
		enum EProduceOverrideMode
		{
			ReplaceObject ,
			ReplaceAll ,
		};
		void      setupRandProduce(EProduceOverrideMode mode , ProduceInfo const producesOverrided[] , int num );
		ObjectId  randomObject();
		

		ProduceInfo mRandProduceMap[ NUM_OBJ ];
		int         mNumRandProduces;
		float       mTotalRate;

		bool        testCheckCount(Tile& tile);
		void        increaseCheckCount();
		unsigned    mCheckCount;
		bool        mbIdxUpdateQueueModified;

		template< typename TFunc >
		void updateActors(TFunc func)
		{
			mbIdxUpdateQueueModified = false;
			unsigned idxCheck = 0;
			while (idxCheck < mIdxUpdateQueue.size())
			{
				ActorData& e = mActorStorage[mIdxUpdateQueue[idxCheck]];

				if (func(e))
				{
					if (mbIdxUpdateQueueModified == true)
					{
						idxCheck = 0;
						mbIdxUpdateQueueModified = false;
					}
					else
					{
						++idxCheck;
					}
				}
				else
				{
					++idxCheck;
				}
			}

		}
		static ObjectInfo const& GetInfo( ObjectId id );
		


		friend class Scene;

		friend class ECFuncLand;
		friend class ECBasicLand;
		friend class ECActor;
		friend class ECToolBase;
		friend class ECCrystal;
		friend class ECBear;
		friend class ECNinja;
	};

}

#endif // TTLevel_h__
