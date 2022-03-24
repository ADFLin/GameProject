#ifndef SudokuSolver_h__
#define SudokuSolver_h__

#include <iostream>
#include <cassert>

#include "BitUtility.h"

#ifndef BIT
#define BIT( n ) ( 1 << (n) )
#endif

class SudokuSolverBase
{
public:


	enum Group
	{
		COL  ,
		ROW  ,
		BOX ,
		NO_GROUP ,
	};
	static bool isOneBitSet(unsigned n)
	{
		return !(n & (n - 1));
	}
	static unsigned Bit2Num( unsigned n )
	{
		unsigned result = 0;
		if ( n & 0xffff0000 ){ result += 16 ; n >>= 16; }
		if ( n & 0x0000ff00 ){ result +=  8 ; n >>=  8; }
		if ( n & 0x000000f0 ){ result +=  4 ; n >>=  4; }
		if ( n & 0x0000000c ){ result +=  2 ; n >>=  2; }
		if ( n & 0x00000002 ){ result +=  1 ; n >>=  1; }
		return result + (n & 0x00000001);
	}
	static int bitCount( int x )
	{
		int count = 0;
		for( ; x ; ++count )
			x &= x - 1; 
		return count;
	}
};

class PASet
{
public:
	void clear(){ mNumberBit = 0; }
	int  getNumber(){ assert( size() == 1 ); return FBitUtility::CountLeadingZeros(mNumberBit); }
	int  size(){ return FBitUtility::CountSet( mNumberBit ); }
	void add( int number ){ mNumberBit |= BIT( number ); }
	void sub( int number ){ mNumberBit &= ~BIT( number ); }

	void add( PASet const& rhs ){ mNumberBit |= rhs.mNumberBit; }
	void sub( PASet const& rhs ){ mNumberBit &= ~rhs.mNumberBit; }
private:
	unsigned mNumberBit;
};


template < class T , int BS = 3 >
class SudokuSolverT : public SudokuSolverBase
{
	T* _this(){ return static_cast< T* >( this ); }
public:
	static int const BoxSize    = BS;
	static int const NumberNum  = BS * BS;
	static int const MaxIndex   = NumberNum * NumberNum;
	static int const NumberBitFill = ( 1 << NumberNum ) - 1;



	enum EMethod
	{
		SolvedValue ,
		SingleValue ,
		NakedPair ,
		NakedTriple ,
		HiddenPair ,
		HiddenTriple ,
		Pointing ,
		BoxLine ,
		XWing ,
		YWing ,
		SimpleColour,
		SwordFish ,
		XCycle ,
		XYChain ,
	};

	struct MethodSolveResult
	{
		EMethod   method;
		Group    group;
		int      idx;
		unsigned numBits;
	};



	SudokuSolverT()
	{

	}



	bool solve( int const* prob )
	{
		if( !setupProbInternal(prob) )
			return false;
		doSolveR( 0 );
		return true;
	}
	int  getNumSolution() const { return numSolution; }
	void printSolution( int nSol ){ print( solution[nSol] ); }

	//
	bool onPrevEvalMethod( EMethod method, Group group, int idx, unsigned numBits ){ return true; }
	void onPostEvalMethod( EMethod method, Group group, int idx, unsigned numBits ){}

	struct RelatedCellInfo
	{
		bool bRemove;
		int  index;
		unsigned numBits;
	};
	bool needEnumRelatedCellInfo() { return true; }
	void doEnumRelatedCellInfo(RelatedCellInfo const& info){}
	//
	void enumRelatedCellInfo(RelatedCellInfo const& info)
	{
		if( _this()->needEnumRelatedCellInfo() )
		{
			_this()->doEnumRelatedCellInfo(info);
		}
	}

	bool setupProblem( int const* prob )
	{
		if( !setupProbInternal(prob) )
			return false;

		for( int i = 0 ; i < MaxIndex ; ++i )
		{
			int col = ColIndex(i);
			int row = RowIndex(i);
			int box = BoxIndex( col , row );

			mPosibleBitsCell[i] = mPosibleBitsGroup[COL][col] & mPosibleBitsGroup[ROW][row] & mPosibleBitsGroup[BOX][ box ];

			if ( problem[i] )
			{
				mCurSolution[i] = 1 << ( problem[i] - 1 );
				mPosibleBitsCell[i] = 0;
			}
			else
			{
				mCurSolution[i] = 0;
			}
		}
		return true;
	}

	int const* getProblem(){ return problem; }




	bool checkGroup( Group group ,  int idxGroup )
	{
		unsigned checkNum = 0;
		for( Iterator iter = Iterator::FromGroupIndex(group, idxGroup); iter.haveMore(); ++iter )
		{			
			int idx = iter.getCellIndex();
			if( mCurSolution[idx] )
			{
				if( mPosibleBitsCell[idx] != 0 )
					return false;
				checkNum |= mCurSolution[idx];
			}
		}

		checkNum |= mPosibleBitsGroup[group][idxGroup];
		if( checkNum != NumberBitFill )
			return false;

		return true;
	}

	bool  checkState()
	{
		for( int i = 0; i < 3; ++i )
		{
			for( int n = 0; n < NumberNum; ++n )
			{
				if( !checkGroup(Group(i), n) )
					return false;
			}
		}
		return true;
	}

	int  getSolution( int idx ) const
	{
		return mCurSolution[idx];
	}

	bool bKeepSolve;

	void solveLogic()
	{
		InitIterator();

		groupCheck[ ROW ] = NumberBitFill;
		groupCheck[ COL ] = NumberBitFill;
		groupCheck[ BOX ] = NumberBitFill;

		int numStep = 0;

		bKeepSolve = true;

		bool finish = false;

		while ( !finish && bKeepSolve )
		{
			finish = true;
CHECK_SIMPLE:
			bool reCheck = false;
			for( int index = 0 ; index < MaxIndex ; ++index )
			{
				if ( !bKeepSolve )
					break;

				if ( !indexCheck[index] )
					continue;

				if ( evalSolvedValueMethod( index ) )
				{
					reCheck = true;
				}
			}

			if ( reCheck )
				goto CHECK_SIMPLE;

			for( int index = 0 ; index < MaxIndex ; ++index )
			{
				if ( !bKeepSolve )
					break;

				if ( !indexCheck[index] )
					continue;

				int groupAt[3];
				groupAt[ COL ] = ColIndex( index );
				groupAt[ ROW ] = RowIndex( index );
				groupAt[ BOX ] = BoxIndex( groupAt[ COL ] , groupAt[ ROW ] );

				if ( evalSingleValueMethod( index , COL , groupAt[ COL ] ) ||
					 evalSingleValueMethod( index , ROW , groupAt[ ROW ] ) ||
					 evalSingleValueMethod( index , BOX , groupAt[ BOX ] ) )
				{
					goto CHECK_SIMPLE;
				}
			}



			for( int index = 0 ; index < MaxIndex ; ++index )
			{
				if ( !bKeepSolve )
					break;

				if ( !indexCheck[index] )
					continue;

				finish = false;
				indexCheck[index] = false;


				int groupAt[3];

				groupAt[ COL ] = ColIndex( index );
				groupAt[ ROW ] = RowIndex( index );
				groupAt[ BOX ] = BoxIndex( groupAt[ COL ] , groupAt[ ROW ] );

				if ( evalSingleValueMethod( index , COL , groupAt[ COL ] ) ||
					 evalSingleValueMethod( index , ROW , groupAt[ ROW ] ) ||
					 evalSingleValueMethod( index , BOX , groupAt[ BOX ] ) )
					 continue;


				int idxGroup = BoxIndex( index );
				if ( !evalBoxLineMethod( index , BOX , idxGroup ) )
					evalPointingMethod( index , BOX , idxGroup );

#if 1
				for( int g = 0 ; g < 3 ; ++g )
				{
					Group group = Group(g);

					if ( groupCheck[group] == 0 )
						continue;

					idxGroup = groupAt[ group ];

					if ( !( groupCheck[group] & BIT(idxGroup) ) )
						continue;


					evalNakedMethod( index , group , idxGroup );
					evalHiddenMethod( index , group , idxGroup );
					if ( group != BOX )
					{
						bool s = evalXWingMethod( index , group , idxGroup );
#if 0
						evalYWingMethod( index , group , idxGroup );
						if ( !s )
							evalSwordFishMethod( index , group , idxGroup );
#endif
					}
				}

				evalSimpleColourMethod( index );
#endif
				//BUG
				//evalXCycleMethod( index );
			}
		}

		print( mCurSolution );
	}

	int getIdxPosible( int idx ){ return mPosibleBitsCell[idx]; }

	class Iterator
	{
	public:
		static Iterator FromGroupIndex(Group group, int idxGroup)
		{
			Iterator iter;
			iter.setGroupIndex(group, idxGroup);
			return iter;
		}

		static Iterator FromCellIndex(Group group, int idxCell)
		{
			Iterator iter;
			iter.setCellIndex(group, idxCell);
			return iter;
		}
		void setGroupIndex( Group group , int idxGroup )
		{
			mNextIndex = sGroupNextIndex[ group ];
			mIndex = GetIteratorBeginFromGroupIndex( group , idxGroup );
		}
		void setCellIndex( Group group , int index )
		{
			assert( index != -1 );
			mNextIndex = sGroupNextIndex[ group ];
			mIndex = GetIteratorBeginFromCellIndex( group , index );
		}
		void setNextIndex( Group group , int index )
		{
			assert( index != -1 );
			mNextIndex = sGroupNextIndex[ group ];
			mIndex = mNextIndex[ index ];
		}
		bool haveMore() const { return mIndex != -1; }
		operator bool() const { return haveMore();  }

		int  getCellIndex() const { return mIndex; }
		int  operator *( void ) const { return getCellIndex(); }
		Iterator& operator ++ ( void )
		{ 
			mIndex = mNextIndex[mIndex]; 
			return *this; 
		}
		Iterator  operator ++ ( int )
		{ 
			Iterator temp(*this); 
			mIndex = mNextIndex[mIndex]; 
			return temp; 
		}

	private:
		int  mIndex;
		int const* mNextIndex;
	};

protected:

	static int BoxIndex( int col , int row ){ return ( col / BoxSize ) + BoxSize * ( row / BoxSize ); }
	static int BoxIndex( int idx ){ return BoxIndex( ColIndex(idx) ,RowIndex(idx) ); }
	static int ColIndex( int idx ){ return idx % NumberNum; }
	static int RowIndex( int idx ){ return idx / NumberNum; }
	static int ToCellIndex(int col, int row) { return row * NumberNum + col; }

	void doSolveR( int idx );
	bool setupProbInternal(int const* prob);
	

	static void print( int* sol )
	{
		for ( int i = 0 ; i < MaxIndex ; ++i )
		{
			std::cout << Bit2Num( sol[i] ) << " ";
			if ( i % NumberNum == NumberNum - 1 )
				std::cout << std::endl;
		}
	}



	void fillNumber( int idx , unsigned posNumber )
	{
		assert(isOneBitSet(posNumber));

		RelatedCellInfo info;
		info.bRemove = false;
		info.index = idx;
		info.numBits = posNumber;
		enumRelatedCellInfo(info);

		
		mCurSolution[idx] = posNumber;
		mPosibleBitsCell[idx] = 0;

		int col = ColIndex(idx);
		int row = RowIndex(idx);
		int box = BoxIndex(col, row);

		mPosibleBitsGroup[COL][col] &= ~posNumber;
		removeGroupNumBit(COL, col, posNumber);

		mPosibleBitsGroup[ROW][row] &= ~posNumber;
		removeGroupNumBit(ROW, row, posNumber);

		mPosibleBitsGroup[BOX][box] &= ~posNumber;
		removeGroupNumBit(BOX, box, posNumber);
	}


	bool removeNumBit( int idx , int numBits )
	{
		if( mCurSolution[idx] )
			return false;

		if( !(mPosibleBitsCell[idx] & numBits) )
			return false;

		RelatedCellInfo info;
		info.bRemove = true;
		info.index = idx;
		info.numBits = numBits;
		enumRelatedCellInfo(info);

		mPosibleBitsCell[idx] &= ~numBits;
		addCheck( idx );
		return true;
	}

	void addCheck( int idx )
	{
		int col = ColIndex(idx);
		int row = RowIndex(idx);
		int box = BoxIndex( col , row );

		groupCheck[COL] |= BIT( col );
		groupCheck[ROW] |= BIT( row );
		groupCheck[BOX] |= BIT( box );

#define INDEX_CHECK( G , g )\
		{\
			Iterator iter;\
			for( iter.setGroupIndex( G , g ); iter ; ++iter )\
			{\
				indexCheck[iter.getCellIndex()] = true;\
			}\
		}
		INDEX_CHECK( COL , col );
		INDEX_CHECK( ROW , row );
		INDEX_CHECK( BOX , box );

#undef INDEX_CHECK
		indexCheck[idx] = true;
	}

	void fillNumBit( int idx , int numBits )
	{
		mPosibleBitsCell[idx] |=  numBits;

	}

	void removeGroupNumBit( Group group , int idxGroup , int numBits )
	{
		Iterator iter;
		for( iter.setGroupIndex( group ,idxGroup ); iter ; ++iter  )
			removeNumBit( iter.getCellIndex() , numBits );
	}

	void removeGroupNumBit( Group group , int idxGroup , unsigned numBits  , int* skipIndex , int numSkip );
	void removeGroupNumBit( Iterator& iter ,  unsigned numBits  , int* skipIndex , int numSkip );

	void fillGroupNumBit( Group group , int idxGroup , int numBits )
	{
		Iterator iter;
		for( iter.setGroupIndex( group ,idxGroup ); iter.haveMore() ; ++iter  )
			fillNumBit( iter.getCellIndex() , numBits );
	}

	bool evalNakedOrderInternal(  int index , Group group , int idxGroup , int count );

	bool evalSolvedValueMethod( int index , Group group = NO_GROUP , int idxGroup = -1 );
	bool evalSingleValueMethod( int index , Group group  , int idxGroup );
	bool evalNakedMethod( int index , Group group , int idxGroup );
	bool evalPointingMethod( int index , Group  group , int idxGroup );
	bool evalHiddenMethod( int index , Group group , int idxGroup );
	bool evalBoxLineMethod( int index , Group group , int idxGroup );
	bool evalXWingMethod( int index , Group group , int idxGroup );
	bool evalYWingMethod( int index , Group group , int idxGroup );
	bool evalSwordFishMethod(  int index , Group group , int idxGroup );
	bool evalSimpleColourMethod( int index , Group group = NO_GROUP , int idxGroup = -1 );

	bool evalXYChainMethod( int index , Group group = NO_GROUP , int idxGroup = -1 )
	{

		return false;
	}

	bool evalXCycleMethod( int index , Group group = NO_GROUP , int idxGroup = -1 )
	{
		(void) group;
		(void) idxGroup;
		bool result = false;

		unsigned pBit = mPosibleBitsCell[ index ];
		while( pBit )
		{
			unsigned numBits = pBit & -pBit;
			pBit -= numBits;

			while( evalXCycleInternal( index ,  numBits ) )
			{
				result |= true;
			}
		}

		return result;
	}

	enum LinkType
	{
		LINK_STRONG = 1,
		LINK_WEAK   = 2,
		LINK_BOTH   = 3,
	};

	struct LinkInfo
	{
		int  index;
		int  prevIndex;
		LinkType type;
		Group    group;
	};


	static int  checkSplitIndex( LinkInfo linkInfo[] , int linkMap[] , int idx , int idx1 )
	{
		int  indexList[2][ MaxIndex ];
		int* pIndex[2] = { indexList[0] , indexList[1] };

		*( pIndex[0]++ ) = idx;
		*( pIndex[1]++ ) = idx1;

		for( int i = 0 ; i < 2 ; ++i )
		{
			int curIndex = indexList[i][0];
			while( linkMap[ curIndex ] != -1 )
			{
				LinkInfo& info = linkInfo[ linkMap[curIndex] ];
				assert( curIndex == info.index );
				*( pIndex[i]++ ) = info.prevIndex;

				curIndex = info.prevIndex;
			}
		}

		while( pIndex[0] !=  indexList[0] && 
			   pIndex[1] !=  indexList[1] )
		{
			--pIndex[0];
			--pIndex[1];
			if ( *pIndex[0] != *pIndex[1] )
			{
				if ( linkInfo[ linkMap[*pIndex[0]] ].type == 
					 linkInfo[ linkMap[*pIndex[0]] ].type )
					 return -1;
				return *( pIndex[0] + 1 );
			}
		}

		return -1;
	}

	bool evalXCycleInternal( int index , unsigned numBits )
	{

		if ( !( mPosibleBitsCell[index] & numBits ) )
			return false;

		LinkInfo linkInfo[ MaxIndex ];

		int linkMap[ MaxIndex ];
		std::fill_n( linkMap , MaxIndex , -1 );

		int numUsed = 0;
		int curTest = 0;

		int pIndex[9];

		Group groupMap[] = { BOX , COL , ROW };
		for( int g = 0 ; g < 3 ; ++g )
		{
			int num = generateGroupNumBitIndex( groupMap[g] , index , numBits , pIndex , 9 );

			LinkType type = ( num == 1 ) ? LINK_STRONG : LINK_WEAK;

			for( int i = 0 ; i < num ; ++i )
			{
				if ( linkMap[ pIndex[i] ] != -1 )
					continue;

				linkMap[ pIndex[i] ] = numUsed;

				LinkInfo& info = linkInfo[ numUsed ];
				info.index     = pIndex[i];
				info.prevIndex = index;
				info.type      = type;
				info.group     = groupMap[g];

				++numUsed;
			}
		}

		unsigned groupLink[3][ NumberNum ];
		std::fill_n( &groupLink[0][0] , 3 * NumberNum , 0 );

		while( curTest < numUsed )
		{
			LinkInfo& curInfo = linkInfo[ curTest ];
			int curIndex = curInfo.index;

			for( int g = 0 ; g < 3 ; ++g )
			{
				Group curGroup = groupMap[g];
				if ( curInfo.group == curGroup )
					continue;

				int num = generateGroupNumBitIndex( curGroup , curIndex , numBits , pIndex , 9 );

				if ( num == 0 )
					continue;

				LinkType type = ( num == 1 ) ? LINK_STRONG : LINK_WEAK;

				if ( curInfo.type == type )
					continue;

				for( int i = 0 ; i < num ; ++i )
				{
					int const& idx = pIndex[i];

					if ( isSameGroup( curInfo.group , idx , curIndex  ) )
						continue;

					if ( linkMap[idx] != -1 )
					{
						LinkInfo& testInfo = linkInfo[ linkMap[idx] ];

						int splitIndex = checkSplitIndex( linkInfo , linkMap , curIndex , idx );
						if ( splitIndex == -1 )
							continue;

						if ( _this()->onPrevEvalMethod( eXCycle , NO_GROUP , index , numBits ) )
						{
							if ( testInfo.type != type )
							{
								int rIndex[3] = { curIndex , idx , splitIndex };

								assert( curIndex != splitIndex );
								assert( idx != splitIndex );

								for ( int i = 0 ; i < 3 ; ++i )
								{
									int testIndex = rIndex[i];

									do
									{
										assert( linkMap[testIndex] != -1 );
										LinkInfo& info = linkInfo[ linkMap[testIndex] ];

										if ( info.type == LINK_WEAK )
										{
											Iterator iter;
											for( iter.setCellIndex( info.group , testIndex );
												iter.haveMore() ; ++iter )
											{
												if ( iter.getCellIndex() != testIndex )
													removeNumBit( iter.getCellIndex() , numBits );
											}
										}
										testIndex = info.prevIndex;
									}
									while ( testIndex != splitIndex );
								}
							}
							else if ( type == LINK_STRONG )
							{
								removeNumBit( splitIndex , numBits );

								int rIndex[2] = { curIndex , testInfo.prevIndex };
								for( int i = 0 ; i < 2 ; ++i )
								{
									int removeIndex = rIndex[i];
									removeIndex = curIndex;
									while( removeIndex != splitIndex )
									{
										assert( linkMap[removeIndex] != -1 );
										LinkInfo& info = linkInfo[ linkMap[removeIndex] ];
										assert( removeIndex == info.index );

										removeNumBit( removeIndex , numBits );
										removeIndex = info.prevIndex;
									}
								}
							}
							else // type == LINK_WEAK
							{
								removeNumBit( idx , numBits );
							}

							_this()->onPostEvalMethod( eXCycle , NO_GROUP , index , numBits );
							return true;
						}

						continue;
					}


					linkMap[ pIndex[i] ] = numUsed;

					LinkInfo& info = linkInfo[ numUsed ];
					info.index     = pIndex[i];
					info.prevIndex = curIndex;
					info.type      = type;
					info.group     = curGroup;

					++numUsed;
				} //for( int i = 0 ; i < num ; ++i )

			} //for( int g = 0 ; g < 3 ; ++g )

			++curTest;

		} //while( curTest < numUsed )

		return false;
	}


	bool  isSameGroup( Group group , int idx1 , int idx2 )
	{
		switch( group )
		{
		case COL: return ColIndex( idx1 ) == ColIndex( idx2 );
		case ROW: return RowIndex( idx1 ) == RowIndex( idx2 );
		case BOX: return BoxIndex( idx1 ) == BoxIndex( idx2 );
		}
		return false;
	}

	bool evalNakedPairOrderInternal( int index , Group group , int idxGroup );
	bool evalNakedTripleOrderInternal(  int index , Group group , int idxGroup , int count );
	bool evalPointingInternal( int index , int idxGroup , unsigned numBits );
	bool evalXWingInternal( int index , Group group , int idxGroup , unsigned numBits );
	bool evalYWingInternal( int index , Group group , int idxGroup , Group nGroup , int nIdx );
	bool evalSimpleColourInternal( int index , unsigned numBits );
	bool evalSwordFishInternal( int index , Group group , int idxGroup , unsigned numBits );

	int  calcGroupNumBitCount( Group group , int idxGroup , unsigned numBits );
	bool checkNumInBox( Group group , int idxGroup , int box , unsigned numBits );

	int  generateGroupNumBitIndex( Group group , int index , unsigned numBits , int pIndex[] , int maxNum );

	bool evalHiddenMethodInternal( int index , Group group , int idxGroup , int num , unsigned numBits , EMethod method  );

	bool      indexCheck[ MaxIndex ];
	unsigned  groupCheck[3];


	int  mCurSolution[ MaxIndex ];
	int  problem[ MaxIndex ];
	int  numSolution;
	int  solution[ 16 ][ MaxIndex ];

	unsigned  mPosibleBitsCell[ MaxIndex ];
	unsigned  mPosibleBitsGroup[3][ NumberNum ];

	static int OffsetCellIndex( int index , Group group , int num );

	static int  GetCellIndex( int col , int row ){ return col + NumberNum * row; }
	static int  GetIteratorBeginFromCellIndex(Group group, int idxCell );
	static int  GetIteratorBeginFromGroupIndex( Group group , int idxGroup );
	static void InitIterator();
	static void PrintIterator( int* iter )
	{
		for ( int i = 0 ; i < MaxIndex ; ++i )
		{
			std::cout << iter[i]  << " ";
			if ( i % NumberNum == NumberNum - 1 )
				std::cout << std::endl;
		}
	}

	static int  sGroupNextIndex[3][ MaxIndex ];
};



#endif // SudokuSolver_h__

#include "SudokuSolver.hpp"