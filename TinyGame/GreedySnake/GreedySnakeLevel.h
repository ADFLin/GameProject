#ifndef GreedySnakeLevel_h__
#define GreedySnakeLevel_h__

#include "Math/TVector2.h"
#include "DataStructure/Grid2D.h"

#include <vector>
#include <list>

typedef TVector2< int > Vec2i;

#ifndef BIT
#define BIT(i) ( 1 << (i) )
#endif

namespace GreedySnake
{
	int const gMaxPlayerNum = 4;

	enum DirEnum
	{
		DIR_EAST = 0,
		DIR_SOUTH = 1,
		DIR_WEST = 2,
		DIR_NORTH = 3,
	};

	typedef int DirType;
	inline Vec2i getDirOffset( DirType dir )
	{
		Vec2i const offset[] = { Vec2i(1,0) , Vec2i(0,1), Vec2i(-1,0), Vec2i(0,-1) };
		return offset[dir];
	}
	inline DirType InverseDir( DirType dir )
	{
		dir += 2;
		if ( dir >= 4 )
			dir -= 4;
		return dir; 
	}

	class SnakeBody
	{
		
	public:
		struct Element
		{
			Vec2i   pos;
			DirType dir;
		};

		SnakeBody(){}
		SnakeBody( Vec2i const& pos , DirType dir, size_t length = 1 )
		{
			Elements.reserve( 16 );
			reset( pos , dir , length );  
		}

		void init(Vec2i const& pos, DirType dir, size_t length)
		{
			Elements.reserve(16);
			reset(pos, dir , length);
		}

		
		size_t  getLength() { return Elements.size(); }

		void    moveStep(DirType moveDir);
		void    growBody( size_t num = 1 );
		Element const& getHead(){ return Elements[ mIdxHead ];  }
		Element const& getTail(){ return Elements[ mIdxTail ];  } 

		typedef std::vector< Element > ElementVec;
		class Iterator
		{
		public:
			Element const& getElement(){ return mBodies[mIdxCur];  }
			bool haveMore(){  return mCount < mBodies.size();  }
			void goNext()
			{
				++mCount;
				if ( mIdxCur == 0 )
					mIdxCur = ( unsigned )mBodies.size();
				--mIdxCur;	
			}
		private:
			Iterator( ElementVec const& bodies , unsigned idxHead )
				:mBodies( bodies ) , mIdxCur( idxHead ),mCount(0){}
			ElementVec const& mBodies;
			size_t   mCount;
			unsigned mIdxCur;
			friend class SnakeBody;
		};

		Iterator       createIterator(){  return Iterator( Elements , mIdxHead ); }
		Element const& getElementByIndex( unsigned idx ){  return Elements[ idx ];  }
		Element const* hitTest( Vec2i const& pos );
		void        warpHeadPos( int w , int h );

		void       reset( Vec2i const& pos , DirType dir, size_t length );
	private:	
		ElementVec  Elements;
		unsigned mIdxTail;
		unsigned mIdxHead;
	};

	struct FoodInfo
	{
		Vec2i pos;
		int   type;
	};

	enum SnakeState
	{
		SS_DEAD        = BIT(0),
		SS_INC_SPEED   = BIT(1),
		SS_DEC_SPEED   = BIT(2),
		SS_CONFUSE     = BIT(3),
		SS_FREEZE      = BIT(4),
		SS_INVINCIBLE  = BIT(5),
		SS_TRANSPARENT = BIT(7),
	};

	enum TerrainType
	{
		TT_NONE  ,
		TT_BLOCK ,
		TT_SLOW  ,
		TT_SIDE  ,
	};

	class Snake
	{
	public:
		void init(Vec2i const& pos, DirType dir, size_t length);

		bool canMove() const
		{
			return (stateBit & (SS_FREEZE | SS_DEAD) )== 0;
		}
		unsigned   id;
		unsigned   stateBit;
		int        moveSpeed;
		int        moveCountAcc;

		int        frameMoveCount;

		bool       changeMoveDir(DirType dir);
		SnakeBody& getBody() { return mBody; }
		DirType    getMoveDir() const { return mMoveDir; }
	private:
		DirType    mMoveDir;
		SnakeBody  mBody;
	};

	struct MapTileData 
	{
		uint8    terrain;
		uint8    snakeMask;
		uint8    itemMask;
		uint8    dummy;
	};

	enum class MapBoundType
	{
		WarpXY,
		WarpX,
		WarpY,
		Cliff,
	};

	class Level
	{
	public:

		class Listener
		{
		public:
			virtual ~Listener(){}
			virtual void onEatFood( Snake& snake , FoodInfo& food ){}
			virtual void onCollideSnake( Snake& snake , Snake& colSnake ){}
			virtual void onCollideTerrain( Snake& snake , int type ){}
		};

		Level( Listener* listener );

		bool       isMapRange( Vec2i const& pos ){ return mMap.checkRange( pos.x , pos.y ); }
		void       setupMap( Vec2i const& size , MapBoundType type );
		void       setTerrain( Vec2i const& pos , int type ){ mMap.getData( pos.x , pos.y ).terrain = type;  }
		int        getTerrain( Vec2i const& pos ){ return mMap.getData( pos.x , pos.y ).terrain;  }
		Vec2i      getMapSize(){ return Vec2i( mMap.getSizeX() , mMap.getSizeY() );  }
		static int const DefaultMoveSpeed = 50;
		static int const MoveCount        = 500;

		void       removeFood( Vec2i const& pos );
		void       addFood( Vec2i const& pos , int type );
		Snake*     addSnake( Vec2i const& pos , DirType dir , size_t length );

		bool       addSnakeState( unsigned id , SnakeState state );

		void       removeAllFood();
		void       removeAllSnake();

		Snake&  getSnake( unsigned id )
		{ 
			assert( (int)id < mNumSnakePlay );
			return mSnakes[ id ];
		}

		enum HitMask
		{
			eNO_HIT_MASK  = 0,
			eTERRAIN_MASK = BIT(0),
			eSNAKE_MASK   = BIT(1),
			eFOOD_MASK    = BIT(2),
			eALL_MASK     = eTERRAIN_MASK | eSNAKE_MASK | eFOOD_MASK ,
		};

		HitMask   hitTest( Vec2i const& pos , unsigned mask , int& hitResult );
		void      tick();

		

		template< class Visitor >
		void      visitFood( Visitor& visitor )
		{
			for( auto& food : mFoodVec)
			{
				visitor( food );
			}
		}
		int       getSnakeNum() const { return mNumSnakePlay;  }
		MapBoundType   getMapType() const { return mMapBoundType; }

		bool      getMapPos( Vec2i const& pos , DirType dir , Vec2i& result );


	private:
		void      checkMapBoundCondition(Snake& snake);
		void      detectMoveSnakeCollision( Snake& snake );
		void      addSnakeMark( Snake& snake);
		void      addSnakeBodyElementMark( unsigned id , Vec2i const& pos );
		void      removeSnakeMark( Snake& snake);
		void      removeSnakeBodyElementMark( unsigned id , Vec2i const& pos );
		
		typedef std::list< FoodInfo > FoodVec;
		FoodVec    mFoodVec;
		Listener*  mListener;
		int        mNumSnakePlay;
		Snake      mSnakes[ gMaxPlayerNum ];
		int        mMoveSpeed;

		typedef TGrid2D< MapTileData > WorldMap;
		WorldMap     mMap;
		MapBoundType mMapBoundType;

		friend class Scene;
	};


}

#endif // GreedySnakeLevel_h__
