#ifndef GoCore_h__
#define GoCore_h__

#include <list>
#include <vector>

class SocketBuffer;

namespace Go
{
	class Board
	{
	public:
		class Pos
		{
		public:
			Pos(){}
			explicit Pos( int idx ):index( idx ){}
			int toIndex() const { return index; }
		private:
			friend class Board;
			int index;
		};
		enum DataType
		{
			eEmpty = 0,
			eBlack = 1,
			eWhite = 2,
		};
		enum Dir
		{
			eLeft  = 0,
			eRight  ,
			eTop    ,
			eBottom ,

			NumDir ,
		};

		Board();
		~Board();


		void     setup( int size );
		void     clear();


		bool     checkRange( int x , int y ) const;
		int      getSize() const { return mSize; }

		Pos      getPos( int x , int y ) const { assert( checkRange( x, y ) ); return Pos( getDataIndex( x , y ) );}
		Pos      getPos( int idx ) const { return Pos( idx ); }

		bool     getConPos( Pos const& pos , int dir , Pos& result );

		DataType getData( int x , int y ) const;
		DataType getData( Pos const& p ) const { return DataType( getData( p.toIndex() ) ); }
		
		int      getLife( Pos const& p ) const { return getLife( p.toIndex() ); }
		int      getLife( int x , int y ) const;

		void     putStone( Pos const& p , DataType color ){ putStone( p.toIndex()  , color ); }
		int      fillStone( Pos const& p , DataType color );
		void     removeStone( Pos const& p );
		int      captureStone( Pos const& p );

	private:
		typedef short LinkType;

		int      calcConIndex( int idx , int dir ) const { return idx + mIndexOffset[ dir ]; }
		int      offsetIndex( int idx , int ox , int oy ){  return idx + ox + oy * getDataSizeX(); }

		char     getData( int idx )  const  { return mData[ idx ]; }
		int      getDataSize()  const { return getDataSizeX() * getDataSizeY(); }
		int      getDataSizeY() const { return mSize + 2; }
		int      getDataSizeX() const { return mSize + 1; }
		int      getDataIndex( int x , int y ) const {  return x + getDataSizeX() * ( y + 1 );  }

		void     putStone( int idx, DataType color );

		int      getLife( int index ) const;
		void     addRootLife( int idxRoot , int val );

		int      relink( int idx );
		void     linkRoot( int idxRoot );
		int      getLinkRoot( int idx ) const;

		int      relink_R( int idx );
		int      fillStone_R( int idx );
		int      captureStone_R( int idx );

		void     removeVisitedMark_R( int idx );


		int       mIndexOffset[ NumDir ];
		DataType  mColorR;
		int       mIdxConRoot;

		int       mSize;
		LinkType* mLinkIndex;
		char*     mData;
	};

	struct GameRule
	{
		int   sec; // 25 hand
		int   komi; //¶K¥Ø
		float handicapNum;
	};
	class Game
	{
	public:
		typedef Board::DataType DataType;
		typedef Board::Pos      Pos;

		Game();

		void    setup( int size );
		void    restart();
		bool    play( int x , int y );
		bool    play( Pos const & pos );
		void    pass();
		void    undo();
		
		DataType getNextPlayColor() { return mNextPlayColor; }
		DataType getFristPlayColor() { return mNextPlayColor; }
		Board const& getBoard() const { return mBoard; }

		int     getBlackRemovedNum() const { return mNumBlackRemoved; }
		int     getWhiteRemovedNum() const { return mNumWhiteRemoved; }

		void    print( int x , int y );

		bool    save( char const* path );

	private:

		void     doRestart( bool beClearBoard );
		

		Pos      getFristConPos( Board::Pos const& pos , unsigned bitDir );

		int      captureStone( Board::Pos const& pos , unsigned& bitDir );

		
		int       mCurHand;
		int       mNumBlackRemoved;
		int       mNumWhiteRemoved;
		Board     mBoard;
		int       mIdxKoPos;
		DataType  mFristPlayColor;
		DataType  mNextPlayColor;

		struct StepInfo
		{
			int       idxPlay;
			int       idxKO;
			unsigned  bitCaptureDir;
		};

		typedef std::vector< StepInfo > StepVec;
		StepVec   mStepVec;

		void takeData( SocketBuffer& buffer );
		bool restoreData( SocketBuffer& buffer );
		
	};

}//namespace Go

#endif // GoCore_h__