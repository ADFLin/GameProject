#ifndef BJLevel_h__
#define BJLevel_h__

#include <vector>

namespace Bejeweled
{
	enum DestroyMethod
	{
		RM_NORMAL ,
		RM_LINE   ,
	};

	typedef char GemType;
	class Level
	{
	public:

		static int const BoardSize = 8;
		static int const BoardStorageSize  = BoardSize * BoardSize;
		static int const MaxGemTypeNum = 7;
		static int const MinRmoveCount = 3;

		enum Dir
		{
			eRIGHT ,
			eDOWN  ,
			eLEFT  ,
			eTOP   ,
		};
		static bool isVerticalDir( Dir dir ){ return ( dir & 0x1 ) != 0;   }
		static int  getAdjoinedIndex( int idx , Dir dir )
		{
			int const dirOffset[] = { 1 , BoardSize , -1 , -BoardSize };
			return idx + dirOffset[ dir ];
		}
		static bool isNeighboring( int idx1 , int idx2 )
		{
			int offset = abs( idx2 - idx1 );
			if ( offset == 1 && ( idx1 / BoardSize ) == ( idx2 / BoardSize ) )
				return true;
			if ( offset == BoardSize )
				return true;
			return false;
		}
		static int  getIndex( int x , int y ){ return y * BoardSize + x;  }

		class Listener
		{
		public:
			virtual void          prevCheckGems(){}
			virtual DestroyMethod onFindSameGems( int idxStart , int num , bool beV ) = 0;
			virtual void          postCheckGems(){}
			virtual void          onDestroyGem( int idx , GemType type ){}
			virtual void          onFillGem( int idx , GemType type ){}
			virtual void          onMoveDownGem( int idxFrom , int idxTo ){}
		};

		Level( Listener* listener );

		GemType  getBoardGem( int idx ) const { return mBoard[idx]; }
		int      getGemTypeNum() const { return mNumGemType;  }
		void     setGemTypeNum( int num ){  mNumGemType = num;  }
		void     setBoardGem( int idx , GemType type ){ mBoard[idx] = type; }

		void     generateRandGems();
		bool     swapSequence( int idx1 , int idx2 );
		void     stepSwap( int idx1 , int idx2 );
		size_t   stepCheckSwap( int idx1 , int idx2 );
		void     stepFill();
		size_t   setepCheckFill();
		void     stepDestroy();

		bool     testRemoveGeom( int idx1 );

		bool     testSwapPairHaveRemove( int idx1 , int idx2 );
	private:
		void   checkFillGem( int x , int maxIdx );
		void   checkGemVertical( int idx );
		void   checkGemHorizontal( int idx );
		void   fillGem( int x , int idxMax );
		int    produceGem( int idx );
		int    countGemsVertical( int idx , int& idxStart );
		int    countGemsHorizontal( int idx ,  int& idxStart );

		struct LineRange
		{
			int  idxStart;
			int  num;
			bool beVertical;
			DestroyMethod method;
		};

		typedef std::vector< LineRange > LineRangeVec;
		LineRangeVec mDestroyRangeVec;
		Listener*    mListener;

		int     mNumGemType;
		int     mIndexMax[ BoardSize ];
		GemType mBoard[ BoardSize * BoardSize ];
	};

}//namespace Bejeweled

#endif // BJLevel_h__
