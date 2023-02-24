#ifndef BMLevel_h__
#define BMLevel_h__

#include "Math/TVector2.h"
#include "DataStructure/Grid2D.h"
#include "Flag.h"

#include "FastDelegate/FastDelegate.h"
#include "Core/IntegerType.h"

#include "DataStructure/Array.h"

#include <list>
#include <cmath>

namespace BomberMan
{
	class World;
	class Player;
	class ColObject;
	class IAnimManager;

	typedef TVector2< int >   Vec2i;
	typedef TVector2< float > Vector2;

	struct CollisionInfo;

	int const gMaxPlayerNum = 8;

	int const gBombFireFrame = 160;
	int const gFireMoveTime  = 12;
	int const gFireMoveSpeed = 4;
	int const gFireHoldFrame = 30;


	enum Dir
	{
		DIR_EAST  = 0,
		DIR_SOUTH = 1,
		DIR_WEST  = 2,
		DIR_NORTH = 3,
	};

	Dir getInvDir( Dir dir );

	inline bool isVertical( Dir dir ){ return (dir & 0x1) != 0; }
	static Vec2i const& getDirOffset( int dir )
	{
		static Vec2i const dirOffset[] =
		{
			Vec2i(1,0) , Vec2i(0,1) , Vec2i(-1,0), Vec2i(0,-1)
		};
		return dirOffset[ dir ];
	}

	enum BombFlag
	{
		BF_UNMOVABLE      = BIT(0) ,
		BF_CONTROLLABLE   = BIT(1) ,
		BF_DANGEROUS      = BIT(2) ,
		BF_BOUNCING       = BIT(3) ,
		BF_PIERCE         = BIT(4) ,
	};

	class Player;
	class BombObject;

	struct Bomb
	{
		enum State
		{
			eONTILE = -1 ,
			eMOVE   = -2 ,
			eJUMP   = -3 ,
			eTAKE   = -4 ,
			eDESTROY= -5 ,
		};

		int         index;
		short       state;
		short       power;
		Vec2i       pos;		
		unsigned    flag;
		Player*     owner;
		long        timer;
		union
		{
			short  fireLength[4];
			BombObject* obj;
		};

		bool isFire() const { return state >= 0; }
		int  getFireLength( Dir dir ) const 
		{ 
			assert( isFire() ); 
			return fireLength[ dir ] != -1 ? fireLength[ dir ] : state; 
		}

		int         next;
	};

	enum MapTile
	{
		MT_DIRT = 0 ,
		MT_ARROW  ,
		MT_ARROW_END = MT_ARROW + 4 ,
		MT_ARROW_CHANGE ,
		MT_ARROW_CHANGE_END = MT_ARROW_CHANGE + 4 ,
		MT_CONVEYER_BELT ,
		MT_CONVEYER_BELT_END = MT_CONVEYER_BELT + 4 ,
		MT_RAIL ,
		MT_RAIL_END = MT_RAIL + 6 ,
	};

	inline int makeRailOffset( Dir dir1 , Dir dir2 )
	{
		assert( dir1 != dir2 );
		static int const offsetMap[4][4] =
		{    // E    S    W    N
		/*E*/{ -1 ,  0 ,  4 ,  3 } ,
		/*S*/{  0 , -1 ,  1 ,  5 } ,
		/*W*/{  4 ,  1 , -1 ,  2 } ,
		/*N*/{  3 ,  5 ,  2 , -1 } ,
		};
		return offsetMap[dir1][dir2];
	}

	inline bool isRailConnect( int railID , Dir dir , int& other )
	{
		assert( MT_RAIL <= railID && railID < MT_RAIL_END );

		static int const map[6][4] =
		{   // E    S    W    N
			{  1 ,  0 , -1 , -1 } ,
			{ -1 ,  2 ,  1 , -1 } ,
			{ -1 , -1 ,  3 ,  2 } ,
			{  3 , -1 , -1 ,  0 } ,
			{  2 , -1 ,  0 , -1 } ,
			{ -1 ,  3 , -1 ,  1 } ,
		};
		
		int offset = railID - MT_RAIL;
		other = map[ offset ][ dir ];
		return other != -1;
	}

	enum TileObject
	{
		MO_NULL = 0 ,
		MO_BLOCK ,
		MO_BOMB  ,
		MO_ITEM  ,
		MO_WALL  ,
	};


	struct TileData
	{
		char  terrain;
		char  obj;
		short meta;

		void setValue( uint16 id )
		{
			terrain = char( id & 0xff );
			obj     = char( id >> 4 );
		}

		bool isBlocked()
		{
			return obj == MO_BLOCK || obj == MO_WALL || obj == MO_BOMB;
		}
	};

	class TileClass
	{
	public:
		virtual void onFire( World const& world , TileData& tile , Vec2i const& pos , Bomb& bomb , int dir ){}
	};

	enum Item
	{
		ITEM_SPEED ,
		ITEM_POWER ,
		ITEM_FULL_POWER ,
		ITEM_BOMB_UP  ,
		ITEM_LINE_BOMB ,
		ITEM_SKULL ,
		ITEM_RUBBER_BOMB ,
		ITEM_LIQUID_BOMB ,
		ITEM_PIERCE_BOMB ,
		ITEM_POWER_GLOVE ,
		ITEM_BOXING_GLOVE ,
		ITEM_BOMB_PASS ,
		ITEM_BOMB_KICK ,
		ITEM_WALL_PASS ,
		ITEM_HEART ,
		ITEM_REMOTE_CTRL ,
		ITEM_INVICIBLE_SUIT ,
		ITEM_ROOEY ,

		NUM_ITEM_TYPE ,
	};

	enum SkullType
	{
		SKULL_NULL ,
		SKULL_FAST_RUNNER  , // The player moves extremely fast.
		SKULL_SLOW_FOOT    , // The player move extremely slow
		SKULL_LOW_POWER    , // The player's bombs explode at minimum blast radius
		SKULL_DIARRHEA     , // The player continuously sets bombs when possible.
		SKULL_INVISIBILITY , // The player disappears from the screen a number of times.
		SKULL_NO_STOP      , // The player cannot stop moving.
		SKULL_REVERSE      , // The player's directional inputs on the controller are reversed.
		SKULL_HASTY        , // The player's bombs explode faster.
		SKULL_INABILITY    , // The player cannot drop bombs.
		SKULL_LEISURE      , // The player's bombs take longer to explode.
		SKULL_HICCUP       , // The player drops random items at regular intervals.
		NumSkullType ,
	};

	enum ColType
	{
		COL_FIRE   ,
		COL_OBJECT ,
		COL_TILE   ,
	};

	enum ObjectType
	{
		OT_PLAYER  ,
		OT_BOMB    ,
		OT_MONSTER ,
		OT_BULLET  ,
		OT_TRIGGER  ,
		OT_ITEM    ,
		OT_MINE_CAR,

		NumObjectType ,
	};

	enum RooeyType
	{
		RT_NONE       ,
		RT_GYAROOEY   , //She can kick soft blocks away
		RT_HANEROOEY  , //He can hop over blocks
		RT_KEROOEY    , //He can kick bombs
		RT_MAGICAROOEY, //He can create a line of bombs depending on how many the player has.
		RT_MAROOEY    , //A morbidly obese rooey that can roll forward at great speeds
		RT_NAGUROOEY  , //He can punch the other players, stunning them momentarily
		RT_WAROOEY    , //These rooeys can only be ridden by bosses. They have no special abilities.
	};

	class Entity
	{
	public:
		Entity( ObjectType type );
		virtual ~Entity(){}
		ObjectType   getType() const { return mType; }

		virtual void update() = 0;
		virtual void postUpdate(){}
		virtual void onSpawn(){}
		virtual void onDestroy(){}
		virtual void onCollision( ColObject* objPart , CollisionInfo const& info ){}

		Vector2 const& getPos()  const { return mPos; }
		Vector2 const& getPrevPos() const { return mPrevPos; }
		void         setPos( Vector2 const& pos ){ mPos = pos; }
		void         setPosWithTile( Vec2i const& tPos );
		World*       getWorld(){ return mWorld; }

	private:

		friend class World;
		bool       mNeedRemove;
		World*     mWorld;
		ObjectType mType;
		Vector2      mPos;
		Vector2      mPrevPos;
	};

	struct TileData;
	class  ColObject;


	enum ColMaskBit
	{
		CMB_SOILD   = BIT( 0 ),
		CMB_FIRE    = BIT( 1 ),
		CMB_BULLET  = BIT( 2 ),
		CMB_TRIGGER = BIT( 3 ),
		CMB_ALL     = 0xfffffff,
	};


	struct CollisionInfo
	{
		ColType type;
		union
		{
			Bomb*      bomb;
			ColObject* obj;
			struct
			{
				TileData* data;
				short     x,y;
			}tile;	
		};
	};

	class ColObject
	{
	public:
		ColObject( Entity* e )
			:mClient( e )
		{

		}
		Vector2 const& getHalfBoundSize() const { return mHalfBSize; }
		void         setHalfBoundSize( Vector2 const& size ){ mHalfBSize = size; }
		unsigned     getColMask() const { return mColMask; }
		void         setColMask( unsigned mask ){ mColMask = mask; }
		void         notifyCollision( CollisionInfo const& info )
		{
			getClient()->onCollision( this , info );
		}
		Entity*      getClient(){ return mClient; }

	private:
		Entity*    mClient;
		Vector2      mHalfBSize;
		unsigned   mColMask;
		friend class World;
	};

	class ColEntity : public Entity
		            , public ColObject
	{
		typedef Entity BaseClass;
	public:
		ColEntity( ObjectType type );

		virtual void update(){}
		virtual void onSpawn();
		virtual void onDestroy();	
	};

	class Trigger : public ColEntity
	{
		typedef ColEntity BaseClass;
	public:
		Trigger():BaseClass( OT_TRIGGER ){}
		typedef fastdelegate::FastDelegate< void ( ColObject* , CollisionInfo const& ) > CollisionFun;
		typedef fastdelegate::FastDelegate< void () >   UpdateFun;

		virtual void update()
		{ 
			if ( updateFun ) updateFun();
		}
		virtual void onCollision( ColObject* objPart , CollisionInfo const& info )
		{ 
			if ( collisionFun ) collisionFun( objPart , info ); 
		}

		CollisionFun collisionFun;
		UpdateFun    updateFun;
	};

	class BombObject : public ColEntity
	{
		typedef ColEntity BaseClass;
	public:
		BombObject( int idx );
		bool  isStopMotion(){ return mbStopMotion; }
		int   getIndex(){ return mIndex; }
		Bomb& getData();
	protected:
		bool mbStopMotion;
	private:
		int  mIndex;
	};

	class MoveBomb : public BombObject
	{
		typedef BombObject BaseClass;
	public:
		MoveBomb( int idx , Vec2i const& pos , Dir dir );

		void update();
		void postUpdate();
		void onSpawn();
		void onDestroy();
		void onCollision( ColObject* objPart , CollisionInfo const& info );
		void setMoveSpeed( float speed ){ mMoveSpeed = speed; }

	private:
		void onTiggerCollision( ColObject* objPart , CollisionInfo const& info );
		void onTiggerUpdate();
	protected:
		Trigger mTrigger;
		float   mMoveSpeed;
		bool    mStopReady;
		Dir     mMoveDir;
	};

	class JumpBomb : public BombObject
	{
		typedef BombObject BaseClass;
	public:
		JumpBomb( int idx , Vec2i const& pos , Dir dir , int jumpDist );

		void update();
		void postUpdate();
		void onCollision( ColObject* objPart , CollisionInfo const& info);
		
		Vec2i   mCurTilePos;
		int     mDistance;
		int     mJumpDist;
		int     mTimer;
		Dir     mMoveDir;
		bool    mHaveBlocked;

	};

	class Actor : public ColEntity
	{
		typedef ColEntity BaseClass;
	public:
		Actor( ObjectType type );

		enum ActionKey
		{
			ACT_MOVE     = BIT(0),
			ACT_BOMB     = BIT(1),
			ACT_FUN      = BIT(2),
			ACT_MOVE_INV = BIT(3),
		};

		void     clearActionKey(){ mActionFlag.clear(); }
		void     addActionKey( ActionKey action ){ mActionFlag.addBits( action ); }
		bool     checkActionKey( ActionKey action ){ return mActionFlag.checkBits( action ); }


	protected:
		
		bool    moveOnTile( Dir moveDir , float offset , Vector2& goalPos , TileData* colTile[] , float margin );
		virtual TileData* testBlocked( Vector2 const& pos );
		FlagValue< unsigned > mActionFlag;
	};

	struct PlayerStatus 
	{
		int      health;
		int      numUseBomb;
		int      maxNumBomb;
		int      power;
		int      tools[ NUM_ITEM_TYPE ];
	};


	class MineCar : public ColEntity
	{
		typedef ColEntity BaseClass;
	public:

		MineCar( Vec2i const& tPos );
		enum State
		{
			eMOVE ,
			eWAIT_IN ,
			eWAIT_OUT ,
		};

		bool isIdle()
		{
			return mRider == NULL;
		}
		void update();
		void postUpdate();
		void onCollision( ColObject* objPart , CollisionInfo const& info );

		State   mState;
		Player* mRider;
		Vec2i   mGoalTilePos;
		float   mOffset;
		Dir     mMoveDir;
		Dir     mNextDir;
	};

	class MineCar;

	class Player : public Actor 
		         , private PlayerStatus
	{
		typedef Actor BaseClass;
	public:
		Player( unsigned id );

		PlayerStatus& getStatus(){ return *this; }


		bool     isVisible();
		bool     isAlive();
		unsigned getId(){ return mId; }
		void     reset();
		void     update();

		void     updateAction( Vector2& nextPos );

		void     postUpdate();
		void     addItem( Item tool );
		bool     haveItem( Item tool ){ return ( mToolFlag & BIT( tool ) ) != 0;  }

		void     move( Dir dir );

		void     spawn( bool beInvicible );

		void     onCollision( ColObject* objPart , CollisionInfo const& info );
		void     onTouchTile( TileData& tile );
		void     onTravelStart( MineCar& car );
		void     onTravelEnd( MineCar& car );

		enum BombEvent
		{
			eFIRE ,
			eDESTROY ,
		};

		void     onBombEvent( Bomb& bomb , BombEvent event );
		void     onFireCollision( Bomb& bomb , ColObject& obj );

		Dir      getFaceDir() const { return mFaceDir; }
		void     setFaceDir( Dir dir ){ mFaceDir = dir; }

		void     takeDamage();
		void     catchDisease( SkullType disease );
		int      getTakeBombIndex(){ return mIdxBombTake; }
		//virtual 
		TileData* testBlocked( Vector2 const& pos );
		SkullType getDisease(){ return mDisease; }

		enum State
		{
			STATE_NORMAL ,
			STATE_DIE   ,
			STATE_INVICIBLE ,
			STATE_GHOST ,
		};

		enum Action
		{
			AS_IDLE      ,
			AS_JUMP      ,
			AS_ENTER_CAR ,
			AS_RIDING    ,
			AS_LEAVE_CAR ,
			AS_FAINT     ,
		};

		void      changeAction( Action act );

		int       getActionTime() const { return mTimeAction; }

		Action    getAction(){ return mAction ; }
		State     getState() const { return mState; }
		void      changeState( State state )
		{
			mState = state;
			mTimeState = 0;
		}

		RooeyType getOwnRooey() const { return mRooeyOwned; }

	private:
		RooeyType mRooeyOwned;
		Action    mAction;
		int       mTimeAction;
		State     mState;
		int       mTimeState;
		int       mIdxBombTake;
		unsigned  mId;
	private:
		int       checkTouchTileInternal( Vector2 const& pos , TileData* tiles[] , int numTile );

		unsigned  mPlayerColBit;
		unsigned  mPrevPlayerColBit;
		SkullType mDisease;
		int       mDiseaseTimer;

		Dir       mFaceDir;
		unsigned  mToolFlag;
	};

	struct MonsterStatus
	{

	};

	class Monster : public Actor
	{
	public:
		Monster():Actor( OT_MONSTER ){}
	};


	class AIQuery
	{
	public:
		void setup( World& world );
		void updateMapInfo();

		World* mWorld;
		TGrid2D< int > mFireCount;
		TGrid2D< int > mDangerCount;
	};

	enum EventId
	{
		EVT_ACTOR_DIE ,
	};

	class WorldListener
	{
	public:
		virtual void onPlayerEvent( EventId id , Player& player ){}
		virtual void onMonsterEvent( EventId id , Monster& monster ){}
		virtual void onBombEvent( EventId id , Bomb& bom ){}
		virtual void onBreakTileObject( Vec2i const& pos , int prevId , int prevMeta  ){}
	};


	struct Rule
	{
		Rule()
		{
			haveGhost = true;
			haveDengerousBomb = true;
		}
		bool haveGhost;
		bool haveDengerousBomb;
	};


	class MoveItem : public Entity
	{
		typedef Entity BaseClass;
	public:
		MoveItem( Item item , Vector2 const& from , Vector2 const& to );

		void update();

	private:
		Vector2 mDest;
		Item  mItem;
	};

	class World
	{
	public:
		World();

		void           tick();
		void           restart();

		int            getRandom( int value );

		void           clearAction();
		void           setListener( WorldListener* listener ){ mListener = listener; }
		WorldListener* getListener(){ return mListener; }
		SkullType      getAvailableDisease();
		RooeyType      getAvailableRooey();
		IAnimManager*  getAnimManager(){ return mAimManager; }
		void           setAnimManager( IAnimManager* manager );

		Rule           getRule(){ return mRule; }

		int            getPlayerNum()     {  return mPlayerNum; }
		Player*        getPlayer( int id ){  return mPlayerMap[ id ];  }
		Player*        addPlayer( Vec2i const& pos );

		void           setupMap( Vec2i const& size , uint16 const* id = NULL );
		void           warpTilePos( Vec2i& pos );
		Vec2i          getMapSize(){  return Vec2i( mMap.getSizeX() , mMap.getSizeY() );  }
		TileData&      getTile( Vec2i const& pos ){  return mMap.getData( pos.x , pos.y );  }

		void           breakTileObj( Vec2i const& pos );
		TileData&      getTileWarp( Vec2i const& pos );
		TileData*      getTileWithPos( Vector2 const& pos , Vec2i& tPos );
		class BombIterator
		{
		public:
			Bomb&  getBomb() { return mLevel.mBombList[ mIdx ]; }
			bool   haveMore(){ return mIdx != -1; }
			void   goNext()  {  mIdx = mLevel.mBombList[ mIdx ].next;  }
		private:
			friend class World;
			BombIterator( World& world , int idx )
				:mLevel( world ),mIdx( idx ){}
			World& mLevel;
			int    mIdx;
		};

		BombIterator   getBombs(){  return BombIterator( *this , mUsingIndex ); }
		int            addBomb( Player& player , Vec2i const& pos , int power , int time , unsigned flag );
		void           moveBomb( int index , Dir dir , float speed , bool beForce );
		void           throwBomb( int index , Vec2i const& pos , Dir dir , int dist );
		Bomb&          getBomb( int index ){  return mBombList[index];  }
	private:
		bool           updateBomb( Bomb& bomb );
		void           checkFireBomb( Bomb &bomb );
		void           fireBomb( Bomb &bomb );
		int            getBombIndex( Bomb& bomb );

		TArray< Bomb >  mBombList;
		int mFreeIndex;
		int mUsingIndex;

	public:

		bool          testCollision( ColObject& obj1 , ColObject& obj2 );
		void          addColObject( ColObject& obj );
		void          removeColObject( ColObject& obj );

		template< class TFunc >
		void         visitColObjects( TFunc func )
		{
			for ( ColObjectList::iterator iter = mColObjList.begin();
				iter != mColObjList.end() ; ++iter )
			{
				ColObject* obj = iter->obj;
				func( obj );
			}
		}

	private:
		struct ColObjectData
		{
			ColObject* obj;
			Vector2      bMin;
			Vector2      bMax;
		};
		typedef std::list< ColObjectData > ColObjectList;
		ColObjectList mColObjList;

		void testFireCollision( Bomb& bomb );
		void resolveCollision( ColObjectData& data1 , ColObjectData& data2 );
		void resolvePlayerCollision( ColObjectData& data1 , ColObjectData& data2 );

		bool fireTile( Vec2i const& pos , Bomb& bomb , int dir );

	public:
		void addEntity( Entity* entity , bool beManaged = false );
		void removeEntity( Entity* entity );

		bool checkEntitiesModified()
		{
			bool result = mIsEntitiesDirty;
			mIsEntitiesDirty = false;
			return result;
		}

		template< class TFunc >
		void visitEntities( TFunc func )
		{
			for ( EntityList::iterator iter = mEnities.begin();
				iter != mEnities.end() ; ++iter )
			{
				Entity* entity = iter->entity;
				if ( entity->mNeedRemove )
					continue;
				func( entity );
			}
		}
		
	private:
		void  cleanupEntity();
		struct EntityData
		{
			Entity* entity;
			bool    beManaged;
		};
		typedef std::list< EntityData > EntityList;

		EntityList::iterator findEntity( Entity* entity );

		bool       mIsEntitiesDirty;
		EntityList mEnities;

	private:
		Rule mRule;
		WorldListener* mListener;
		IAnimManager*  mAimManager;

		Player* mPlayerMap[ gMaxPlayerNum ];
		int mPlayerNum;


		TGrid2D< TileData >  mMap;
	};

}//namespace BomberMan

#endif // BMLevel_h__
