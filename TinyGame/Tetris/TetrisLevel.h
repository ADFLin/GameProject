#ifndef TetrisCore_h__
#define TetrisCore_h__

#include <cassert>
#include <algorithm>
#include <vector>
#include "IntegerType.h"

namespace Tetris
{
	int const gTetrisMaxPlayerNum = 5;

	typedef short BlockType;

	class Block
	{
	public:
		BlockType getType() const { return type; }
		int       getX() const { return x; }
		int       getY() const { return y; }
	private:
		void      transformPos( unsigned char* trans , int size );
		void      setPos( int idx , int size );
		friend class Piece;
		char      x;
		char      y;
		BlockType type;
		char      index;
	};

	struct PieceTemplate
	{
		char     size;
		char     dirNum;
		char     color;
		uint32   map;
	};

	class Piece
	{
	public:
		Piece();

		enum
		{
			COLOR_MASK  = 0x00ff,
			FUNC_MASK   = 0xff00,
		};

		static short getColor( BlockType data )
		{
			return data & Piece::COLOR_MASK;
		}


		void         rotate( int time );
		int          getColor()   const {  return getTemplate().color; }
		int          getMapSize() const {  return getTemplate().size;  }
		int          getDirNum()  const {  return getTemplate().dirNum; }
		int          getDir()     const {  return mDir; }

		void         setBlockType( int idx , BlockType type ){ mBlock[ idx ].type = type; }

		int          getBlockNum() const { return mNumBlock; }
		Block const& getBlock( unsigned idx ) const
		{
			assert( idx < mNumBlock );
			return mBlock[idx];
		}

		Piece&  operator = ( Piece const& piece )
		{
			mNumBlock = piece.mNumBlock;
			mDir      = piece.mDir;
			mTemp     = piece.mTemp;
			std::copy( piece.mBlock , piece.mBlock + piece.mNumBlock , mBlock );
			return *this;
		}

	private:
		friend class PieceTemplateSet;
		void         setTemplate( PieceTemplate& temp );
		Block&       _getBlock( unsigned idx ){  return mBlock[idx];  }
		PieceTemplate const&  getTemplate() const { assert( mTemp ); return *mTemp; }
		PieceTemplate const* mTemp;
		unsigned char  mNumBlock;
		char           mDir;
		Block          mBlock[ 16 ];

	};

	class PieceTemplateSet
	{
	public:
		PieceTemplateSet( PieceTemplate temp[] , int num );
		int   getTemplateNum() const { return mNum; }
		void  setTemplate( int idx , Piece& piece ){ piece.setTemplate( mTemps[ idx ] ); }

		static PieceTemplateSet& getClassicSet();
	private:
		PieceTemplate* mTemps;
		int            mNum;
	};

	class BlockStorage
	{
	public:
		BlockStorage( int sizeX , int sizeY );
		~BlockStorage();

		void cleanupData();

		void reset( int sizeX , int sizeY );

		void storeData();

		void        clearBlock();
		void        markPiece  ( Piece& piece , int cx, int cy );
		int         movePiece  ( Piece& piece , int& cx , int& cy , int ox , int oy );
		int         rotatePiece( Piece& piece , int& cx , int& cy , bool beCW );
		void        fixPiecePos( Piece& piece , int& cx , int& cy , int dy );

		enum
		{
			HIT_LEFT_SIDE  = 1 << 0,
			HIT_RIGHT_SIDE = 1 << 1,
			HIT_BOTTOM_SIDE= 1 << 2,
			HIT_BLOCK      = 1 << 3,
		};
		int              testCollision( Piece& piece , int cx, int cy );

		int              getSizeX() const { return mSizeX; }
		int              getSizeY() const { return mSizeY; }
		int              getExtendSizeY() const { return mSizeY + 5; }

		BlockType        getBlock( int x , int y ) const {  return getLayer(y)[x];  }
		void             setBlock( int x , int y , BlockType val );
		void             emptyBlock( int x , int y );
		BlockType*       getLayer( int y )       { return mLayerMap[y]->blocks; }
		BlockType const* getLayer( int y ) const { return mLayerMap[y]->blocks; }

		void             clearLayer( int y );
		bool             isLayerFilled( int y );
		bool             addLayer( int y , unsigned leakBit , BlockType block );
		bool             isEmptyLayer( int y ) const { return mLayerMap[y]->isEmpty();  }

		int              removeConnect();
		int              scanConnect( int cx , int cy );

		int              scanConnectRecursive( int cx , int cy , short color );
		int              scanFilledLayer( int yMax , int yMin , int removeLayer[] );

		inline bool isInExtendRange(int cx,int cy)
		{
			return 0 <= cx && cx < mSizeX &&
				   0 <= cy && cy < getExtendSizeY();
		}
		inline bool isInRange(int cx,int cy)
		{
			return 0 <= cx && cx < mSizeX &&
			       0 <= cy && cy < mSizeY;
		}

		
	private:

		unsigned getFilledBits() const { return ( 1 << mSizeX ) - 1;  }

		struct Layer
		{
			unsigned   markBit;
			BlockType* blocks;
			bool  isEmpty(){  return markBit == 0;  }
		};

		friend class Scene;
		int     mSizeX;
		int     mSizeY;
		Layer*  mLayerStorage;
		std::vector< Layer* >  mLayerMap;
		std::vector< int* >    mConMap;

	};


	enum LevelState
	{
		LVS_NORMAL ,
		LVS_REMOVE_MAPLINE ,
		LVS_REMOVE_CONNECT ,
		LVS_ENTRY_DELAY ,
		LVS_OVER ,
	};

	class EventListener;

	class LevelRule
	{
	public:
		LevelRule();
		static int const DefaultTimeClearLayer;
		static int const DefaultTimeLockPiece;
		static int const DefaultTimeEntryDelay;

		int            getClearLayerTime() const { return mTimeClearLayer;  }
		int            getLockPieceTime()  const { return mTimeLockPiece;  }
		int            getEntryDelayTime() const { return mTimeEntryDelay; }

		void           setClearLayerTime( int time ){  mTimeClearLayer = time;  }
		void           setLockPieceTime( int time ) {  mTimeLockPiece = time;  }
		void           setEntryDelayTime( int time ){  mTimeEntryDelay = time; }

		PieceTemplateSet* getPieceTemplateSet(){ return mPieceTempSet; }

		
	protected:

		int          mTimeClearLayer;
		int          mTimeLockPiece;
		int          mTimeEntryDelay;

		PieceTemplateSet* mPieceTempSet;

	};

	class Level : public LevelRule
	{
	public:
		class EventListener
		{
		public:
			virtual ~EventListener(){}
			virtual void onMarkPiece  ( Level* level , int layer[] , int numLayer ){} 
			virtual void onRemoveLayer( Level* level , int layer[] , int numLayer ){}
			virtual void onChangePiece( Level* level ){}
		};

		Level( EventListener* listener );

		void           resetMap( int sizeX , int sizeY );

		void           restart();
		void           update( long time );

		void           movePiece( int ox , int oy ){  mStorage.movePiece( getMovePiece() , mXPosMP , mYPosMP , ox , oy );  }
		void           rotatePiece( bool beCW )    {  mStorage.rotatePiece( getMovePiece() , mXPosMP , mYPosMP , beCW );  }
		void           fixPiecePos( int dy )       {  mStorage.fixPiecePos( getMovePiece() , mXPosMP , mYPosMP , dy ); }
		void           moveDown();
		void           changeNextPiece();
		void           holdPiece();
		void           fallPiece();
		int            calcFallPosY();

		BlockStorage&  getBlockStorage(){ return mStorage; }
		LevelState     getState(){ return mState; }
		Piece&         getMovePiece(){  assert( mMovePiece ); return *mMovePiece; }
		void           setMovePiece( Piece& piece , int x , int y );
		Piece&         getNextPiece( int idx = 0 );
		Piece&         getHoldPiece(){  return *mHoldPiece;  }

		long           getTimeDuration()  const { return mTimeDuration; }
		long           getStateDuration() const { return mStateDuration; }
		int            getTotalRemoveLayerNum() const { return mTotalLayerRemove; }

		int            getUsePieceNum()   const { return mTotalPieceUse; }
		int            getGravityValue()  const { return mGravityValue; }
		void           setGravityValue( int value );

		int            getLastRemoveLayerNum()  const { return mRemoveLayerNum; }
		int            getLastRemoveLayerIndex( int n ){ return mRemoveLayer[ n ]; }

		void           getPiecePos( int& nx , int& ny ){  nx = mXPosMP; ny = mYPosMP;  }

		BlockType        getBlock( int x , int y ) const { return mStorage.getBlock( x , y ); }
		BlockType const* getLayer( int y ) const  { return mStorage.getLayer( y );  }
		bool             isEmptyLayer( int y ) const { return mStorage.isEmptyLayer( y ); }


		static int const GravityUnit;

		void           produceRandomPiece( Piece& piece );

		void        setUserData( void* data ){ mUserData = data; }
		void*       getUserData(){ return mUserData ; }


	protected:

		LevelState    doMoveDown( bool beGravity );
		void          changeState( LevelState state );

		BlockStorage      mStorage;
		void*             mUserData;

		LevelState   mState;
		long         mStateDuration;
		int          mIndexPiece;

		Piece*       mHoldPiece;
		Piece*       mMovePiece;

		static int const NumStoragePiece = 5;
		Piece        mPiece[ NumStoragePiece ];

		int          mTiggerLockPiece;

		int          mTotalPieceUse;
		int          mTotalLayerRemove;
		long         mTimeDuration;

		static int const MaxRemoveLayerNum = 10;
		int         mRemoveLayer[ MaxRemoveLayerNum ];
		int         mRemoveLayerNum;

		friend class Scene;

		int            mGravityValue;
		int            mCountGravity;
		int            mCountMove;
		EventListener* mListener;

		int         mXPosMP;
		int         mYPosMP;


	};

}// namespace Tetris

#endif // TetrisCore_h__