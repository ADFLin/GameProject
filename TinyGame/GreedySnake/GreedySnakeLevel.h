#ifndef GreedySnakeLevel_h__
#define GreedySnakeLevel_h__

#include "TVector2.h"
#include <vector>
#include <list>
#include "TGrid2D.h"

typedef TVector2< int > Vec2i;

#ifndef BIT
#define BIT(i) ( 1 << (i) )
#endif

namespace GreedySnake
{
	int const gMaxPlayerNum = 4;
	typedef int DirType;
	inline Vec2i getDirOffset( DirType dir )
	{
		Vec2i const offset[] = { Vec2i(1,0) , Vec2i(0,1), Vec2i(-1,0), Vec2i(0,-1) };
		return offset[dir];
	}
	inline DirType inverseDir( DirType dir )
	{
		dir += 2;
		if ( dir >= 4 )
			dir -= 4;
		return dir; 
	}

	class Snake
	{
		
	public:
		struct Body
		{
			Vec2i   pos;
			DirType dir;
		};
		Snake( Vec2i const& pos , DirType dir , size_t length = 1 )
		{
			mBodies.reserve( 16 );
			_reset( pos , dir , length );  
		}

		DirType getMoveDir() const { return mMoveDir; }
		bool    changeMoveDir( DirType dir );
		size_t  getBodyLength(){ return mBodies.size(); }

		void    moveStep();
		void    growBody( size_t num = 1 );
		Body const& getHead(){ return mBodies[ mIdxHead ];  }
		Body const& getTail(){ return mBodies[ mIdxTail ];  } 

		typedef std::vector< Body > BodyVec;
		class Iterator
		{
		public:
			Body const& getElement(){ return mBodies[mIdxCur];  }
			bool haveMore(){  return mCount < mBodies.size();  }
			void goNext()
			{
				++mCount;
				if ( mIdxCur == 0 )
					mIdxCur = ( unsigned )mBodies.size();
				--mIdxCur;	
			}
		private:
			Iterator( BodyVec const& bodies , unsigned idxHead )
				:mBodies( bodies ) , mIdxCur( idxHead ),mCount(0){}
			BodyVec const& mBodies;
			size_t   mCount;
			unsigned mIdxCur;
			friend class Snake;
		};

		Iterator    getBodyIterator(){  return Iterator( mBodies , mIdxHead ); }
		Body const& getBodyByIndex( unsigned idx ){  return mBodies[ idx ];  }
		Body const* hitTest( Vec2i const& pos );
		void        warpHeadPos( int w , int h );

		void       _reset( Vec2i const& pos , DirType dir , size_t length );
	private:	
		BodyVec  mBodies;
		DirType  mMoveDir;
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
		SS_DEAD = BIT(0) ,
	};

	enum TerrainType
	{
		TT_BLOCK ,
		TT_SLOW  ,
		TT_SIDE  ,

	};
	struct SnakeInfo
	{
		unsigned id;
		Snake*   snake;
		unsigned stateBit;
		int      moveSpeed;
		int      curMoveCount;
	};

	struct MapData 
	{
		int      terrain ;
		unsigned snake   ;
	};


	struct LevelVisitor
	{
		void visitFood( FoodInfo const& info );
	};
	class Level
	{
	public:

		class Listener
		{
		public:
			virtual ~Listener(){}
			virtual void onEatFood( SnakeInfo& snake , FoodInfo& food ){}
			virtual void onCollideSnake( SnakeInfo& snake , SnakeInfo& colSnake ){}
			virtual void onCollideTerrain( SnakeInfo& snake , int type ){}
		};

		Level( Listener* listener );
		enum MapType
		{
			eMAP_WARP_XY ,
			eMAP_WARP_X  ,
			eMAP_WARP_Y  ,
			eMAP_CLIFF   ,
		};
		bool       isVaildMapRange( Vec2i const& pos ){ return mMap.checkRange( pos.x , pos.y ); }
		void       setupMap( int w , int h , MapType type );
		void       setTerrain( Vec2i const& pos , int type ){ mMap.getData( pos.x , pos.y ).terrain = type;  }
		int        getTerrain( Vec2i const& pos ){ return mMap.getData( pos.x , pos.y ).terrain;  }
		Vec2i      getMapSize(){ return Vec2i( mMap.getSizeX() , mMap.getSizeY() );  }
		static int const DefaultMoveSpeed = 50;
		static int const MoveCount        = 500;

		void       removeFood( Vec2i const& pos );
		void       addFood( Vec2i const& pos , int type );
		SnakeInfo* addSnake( Vec2i const& pos , DirType dir , size_t length );

		bool       addSnakeState( unsigned id , SnakeState state );

		void       removeAllFood();
		void       removeAllSnake();

		SnakeInfo&  getSnakeInfo( unsigned id )
		{ 
			assert( (int)id < mNumSnake );
			return mSnakeInfo[ id ];
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
			for( FoodVec::iterator iter = mFoodVec.begin();
				iter != mFoodVec.end() ; ++iter )
			{
				visitor.visit( *iter );
			}
		}
		int       getSnakeNum() const { return mNumSnake;  }
		MapType   getMapType() const { return mMapType; }

		bool      getMapPos( Vec2i const& pos , DirType dir , Vec2i& result );


	private:
		void      detectSnakeCollision( SnakeInfo& info );
		void      addSnakeMark( SnakeInfo& info );
		void      addSnakeBodyMark( unsigned id , Vec2i const& pos );
		void      removeSnakeMark( SnakeInfo& info );
		void      removeSnakeBodyMark( unsigned id , Vec2i const& pos );
		
		typedef std::list< FoodInfo > FoodVec;
		FoodVec    mFoodVec;
		Listener*  mListener;
		int        mNumSnake;
		SnakeInfo  mSnakeInfo[ gMaxPlayerNum ];
		int        mMoveSpeed;
		int        mCurMoveCount;

		typedef TGrid2D< MapData > WorldMap;
		WorldMap   mMap;
		MapType    mMapType;

		friend class Scene;
	};


}

#endif // GreedySnakeLevel_h__
