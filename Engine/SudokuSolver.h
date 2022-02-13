#ifndef SudokuSolver_h__
#define SudokuSolver_h__


#include <iostream>
#include <cassert>

#include "BitUtility.h"
using namespace std;

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
	int  getNumber(){ assert( size() == 1 ); return BitUtility::toNumber( mNumberBit ); }
	int  size(){ return BitUtility::count( mNumberBit ); }
	void add( unsigned number ){ mNumberBit |= BIT( number ); }
	void sub( unsigned number ){ mNumberBit &= ~BIT( number ); }

	void add( PASet const& rhs ){ mNumberBit |= rhs.mNumberBit; }
	void sub( PASet const& rhs ){ mNumberBit &= ~rhs.mNumberBit; }
private:
	unsigned mNumberBit;
};


template < class T , int BS = 3 >
class TSudokuSolver : public SudokuSolverBase
{
	T* _this(){ return static_cast< T* >( this ); }
public:
	static int const BoxSize    = BS;
	static int const NumberNum  = BS * BS;
	static int const MaxIndex   = NumberNum * NumberNum;
	static int const NumberBitFill = ( 1 << NumberNum ) - 1;



	enum Method
	{
		eSolvedValue ,
		eSingleValue ,
		eNakedPair ,
		eNakedTriple ,
		eHiddenPair ,
		eHiddenTriple ,
		ePointing ,
		eBoxLine ,
		eXWing ,
		eYWing ,
		eSimpleColour,
		eSwordFish ,
		eXCycle ,
		eXYChain ,
	};


	TSudokuSolver()
		:problem( 0 )
	{

	}



	void solve( int* prob )
	{
		init( prob );
		doSolve( 0 );
	}
	int  getNumSolution() const { return numSolution; }
	void printSolution( int nSol ){ print( solution[nSol] ); }


	bool OnPrevEvalMethod( Method method , Group group , int idx , unsigned numBit ){ return true; }
	void OnPostEvalMethod( Method method , Group group , int idx , unsigned numBit ){}




	void setProblem( int* prob )
	{
		init( prob );

		for( int i = 0 ; i < MaxIndex ; ++i )
		{
			int col = getCol(i);
			int row = getRow(i);
			int box = getBox( col , row );

			idxPosible[i] = groupPosible[COL][col] & groupPosible[ROW][row] & groupPosible[BOX][ box ];

			if ( problem[i] )
			{
				curSolution[i] = 1 << ( problem[i] - 1 );
				idxPosible[i] = 0;
			}
			else
			{
				curSolution[i] = 0;
			}
		}
	}

	int const* getProblem(){ return problem; }

	int  getSolution( int idx ) const
	{
		return curSolution[idx];
	}

	bool keepSolve;

	void solveLogicSolution()
	{
		assert( problem );

		contructIter();

		groupCheck[ ROW ] = NumberBitFill;
		groupCheck[ COL ] = NumberBitFill;
		groupCheck[ BOX ] = NumberBitFill;

		int numStep = 0;

		keepSolve = true;

		bool finish = false;

		while ( !finish && keepSolve )
		{
			finish = true;
CHECK_SIMPLE:
			bool reCheck = false;
			for( int index = 0 ; index < MaxIndex ; ++index )
			{
				if ( !keepSolve )
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
				if ( !keepSolve )
					break;

				if ( !indexCheck[index] )
					continue;

				int groupAt[3];
				groupAt[ COL ] = getCol( index );
				groupAt[ ROW ] = getRow( index );
				groupAt[ BOX ] = getBox( groupAt[ COL ] , groupAt[ ROW ] );

				if ( evalSingleValueMethod( index , COL , groupAt[ COL ] ) ||
					evalSingleValueMethod( index , ROW , groupAt[ ROW ] ) ||
					evalSingleValueMethod( index , BOX , groupAt[ BOX ] ) )
				{
					goto CHECK_SIMPLE;
				}
			}



			for( int index = 0 ; index < MaxIndex ; ++index )
			{
				if ( !keepSolve )
					break;

				if ( !indexCheck[index] )
					continue;

				finish = false;
				indexCheck[index] = false;


				int groupAt[3];

				groupAt[ COL ] = getCol( index );
				groupAt[ ROW ] = getRow( index );
				groupAt[ BOX ] = getBox( groupAt[ COL ] , groupAt[ ROW ] );

				if ( evalSingleValueMethod( index , COL , groupAt[ COL ] ) ||
					 evalSingleValueMethod( index , ROW , groupAt[ ROW ] ) ||
					 evalSingleValueMethod( index , BOX , groupAt[ BOX ] ) )
					 continue;


				int at = getBox( index );
				if ( !evalBoxLineMethod( index , BOX , at ) )
					evalPointingMethod( index , BOX , at );

#if 0
				for( int g = 0 ; g < 3 ; ++g )
				{
					Group group = Group(g);

					if ( groupCheck[group] == 0 )
						continue;

					at = groupAt[ group ];

					if ( !( groupCheck[group] & BIT(at) ) )
						continue;

					evalNakedMethod( index , group , at );
					evalHiddenMethod( index , group , at );
					if ( group != BOX )
					{
						bool s = evalXWingMethod( index , group , at );
						evalYWingMethod( index , group , at );
						if ( !s )
							evalSwordFishMethod( index , group , at );
					}
				}

				evalSimpleColourMethod( index );
#endif
				//BUG
				//evalXCycleMethod( index );
			}
		}

		print( curSolution );
	}

	int getIdxPosible( int idx ){ return idxPosible[idx]; }

	class Iterator
	{
	public:
		void setGroupAt( Group group , int at )
		{
			mIter = sGroupIterator[ group ];
			mIndex = getIteratorBegin( group , at );
		}
		void setGroupIndex( Group group , int index )
		{
			assert( index != -1 );
			mIter = sGroupIterator[ group ];
			mIndex = getIteratorIndexBegin( group , index );
		}
		void setNextIndex( Group group , int index )
		{
			assert( index != -1 );
			mIter = sGroupIterator[ group ];
			mIndex = mIter[ index ];
		}
		bool haveMore() const { return mIndex != -1; }

		int  getIndex() const { return mIndex; }
		int  operator *( void ) const { return getIndex(); }
		Iterator& operator ++ ( void ){ mIndex = mIter[mIndex]; return *this; }
		Iterator  operator ++ ( int )
		{ 
			Iterator temp(*this); 
			mIndex = mIter[mIndex]; 
			return temp; 
		}

	private:
		int  mIndex;
		int* mIter;
	};

protected:

	static int getBox( int col , int row ){ return ( col / BoxSize ) + BoxSize * ( row / BoxSize ); }
	static int getBox( int idx ){ return getBox( getCol(idx) ,getRow(idx) ); }
	static int getCol( int idx ){ return idx % NumberNum; }
	static int getRow( int idx ){ return idx / NumberNum; }

	void doSolve( int idx );
	void init( int* prob );

	static void print( int* sol )
	{
		for ( int i = 0 ; i < MaxIndex ; ++i )
		{
			cout << Bit2Num( sol[i] ) << " ";
			if ( i % NumberNum == NumberNum - 1 )
				cout << endl;
		}
	}



	void fillNumber( int idx , int numberBit )
	{
		curSolution[ idx ] = numberBit;
		idxPosible[ idx ] = 0;

		int col = getCol( idx );
		int row = getRow( idx );
		int box = getBox( col , row );

		groupPosible[COL][ col ] &= ~numberBit;
		removeGroupNumBit( COL , col , numberBit );

		groupPosible[ROW][ row ] &= ~numberBit;
		removeGroupNumBit( ROW , row , numberBit );

		groupPosible[BOX][ box ] &= ~numberBit;
		removeGroupNumBit( BOX , box , numberBit );
	}

	bool removeNumBit( int idx , int numBit )
	{
		if ( curSolution[idx] )
			return false;

		if ( idxPosible[idx] & numBit )
		{
			idxPosible[idx] &= ~numBit;
			addCheck( idx );
			return false;
		}
		return true;
	}

	void addCheck( int idx )
	{
		int col = getCol(idx);
		int row = getRow(idx);
		int box = getBox( col , row );

		groupCheck[COL] |= BIT( col );
		groupCheck[ROW] |= BIT( row );
		groupCheck[BOX] |= BIT( box );

		int* iterator ;
#define INDEX_CHECK( G , g )\
		iterator = sGroupIterator[ G ];\
		for( int index = getIteratorBegin( G , g ) ;\
			 index != idx ; index = iterator[index] )\
		{\
			indexCheck[g] = true;\
		}

		INDEX_CHECK( COL , col );
		INDEX_CHECK( ROW , row );
		INDEX_CHECK( BOX , box );

#undef INDEX_CHECK
		indexCheck[idx] = true;
	}

	void fillNumBit( int idx , int numBit )
	{
		idxPosible[idx] |=  numBit;

	}

	void removeGroupNumBit( Group group , int at , int numBit )
	{
		Iterator iter;
		for( iter.setGroupAt( group ,at ); iter.haveMore() ; ++iter  )
			removeNumBit( iter.getIndex() , numBit );
	}

	void removeGroupNumBit( Group group , int at , unsigned numBit  , int* skipIndex , int numSkip );
	void removeGroupNumBit( Iterator& iter ,  unsigned numBit  , int* skipIndex , int numSkip );

	void fillGroupNumBit( Group group , int at , int numBit )
	{
		Iterator iter;
		for( iter.setGroupAt( group ,at ); iter.haveMore() ; ++iter  )
			fillNumBit( iter.getIndex() , numBit );
	}

	bool evalNakedOrderInternal(  int index , Group group , int at , int count );

	bool evalSolvedValueMethod( int index , Group group = NO_GROUP , int at = -1 );
	bool evalSingleValueMethod( int index , Group group  , int at );
	bool evalNakedMethod( int index , Group group , int at );
	bool evalPointingMethod( int index , Group  group , int at );
	bool evalHiddenMethod( int index , Group group , int at );
	bool evalBoxLineMethod( int index , Group group , int at );
	bool evalXWingMethod( int index , Group group , int at );
	bool evalYWingMethod( int index , Group group , int at );
	bool evalSwordFishMethod(  int index , Group group , int at );
	bool evalSimpleColourMethod( int index , Group group = NO_GROUP , int at = -1 );

	bool evalXYChainMethod( int index , Group group = NO_GROUP , int at = -1 )
	{

		return false;
	}

	bool evalXCycleMethod( int index , Group group = NO_GROUP , int at = -1 )
	{
		(void) group;
		(void) at;
		bool result = false;

		unsigned pBit = idxPosible[ index ];
		while( pBit )
		{
			unsigned numBit = pBit & -pBit;
			pBit -= numBit;

			while( evalXCycleInternal( index ,  numBit ) )
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

	bool evalXCycleInternal( int index , unsigned numBit )
	{

		if ( !( idxPosible[index] & numBit ) )
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
			int num = generateGroupNumBitIndex( groupMap[g] , index , numBit , pIndex , 9 );

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

				int num = generateGroupNumBitIndex( curGroup , curIndex , numBit , pIndex , 9 );

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

						if ( _this()->OnPrevEvalMethod( eXCycle , NO_GROUP , index , numBit ) )
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
											for( iter.setGroupIndex( info.group , testIndex );
												iter.haveMore() ; ++iter )
											{
												if ( iter.getIndex() != testIndex )
													removeNumBit( iter.getIndex() , numBit );
											}
										}
										testIndex = info.prevIndex;
									}
									while ( testIndex != splitIndex );
								}
							}
							else if ( type == LINK_STRONG )
							{
								removeNumBit( splitIndex , numBit );

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

										removeNumBit( removeIndex , numBit );
										removeIndex = info.prevIndex;
									}
								}
							}
							else // type == LINK_WEAK
							{
								removeNumBit( idx , numBit );
							}

							_this()->OnPostEvalMethod( eXCycle , NO_GROUP , index , numBit );
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
		case COL: return getCol( idx1 ) == getCol( idx2 );
		case ROW: return getRow( idx1 ) == getRow( idx2 );
		case BOX: return getBox( idx1 ) == getBox( idx2 );
		}
		return false;
	}

	bool evalNakedPairOrderInternal( int index , Group group , int at );
	bool evalNakedTripleOrderInternal(  int index , Group group , int at , int count );
	bool evalPointingInternal( int index , int at , unsigned numBit );
	bool evalXWingInternal( int index , Group group , int at , unsigned numBit );
	bool evalYWingInternal( int index , Group group , int at , Group nGroup , int nIdx );
	bool evalSimpleColourInternal( int index , unsigned numBit );
	bool evalSwordFishInternal( int index , Group group , int at , unsigned numBit );

	int  calcGroupNumBitCount( Group group , int at , unsigned numBit );
	bool checkNumInBox( Group group , int at , int box , unsigned numBit );

	int  generateGroupNumBitIndex( Group group , int index , unsigned numBit , int pIndex[] , int maxNum );

	bool evalHiddenMethodInternal( int index , Group group , int at , int num , unsigned numBit , Method method  );

	bool      indexCheck[ MaxIndex ];
	unsigned  groupCheck[3];


	int  curSolution[ MaxIndex ];
	int* problem;
	int  numSolution;
	int  solution[ 16 ][ MaxIndex ];

	unsigned  idxPosible[ MaxIndex ];
	unsigned  groupPosible[3][ NumberNum ];

	static int offsetIndex( int index , Group group , int num )
	{
		switch( group )
		{
		case ROW: return index + num;
		case COL: return index + num * NumberNum;
		case BOX: 
			{
				int* iterator = sGroupIterator[group];
				for( int i = 0 ; i < num; ++i )
				{
					index = iterator[index];
					if ( index == -1 )
						return -1;
				}
				return index;
			}
		}
		return -1;
	}

	static int  getIndex( int col , int row ){ return col + NumberNum * row; }
	static int  getIteratorIndexBegin( Group group , int index );
	static int  getIteratorBegin( Group group , int at );
	static void contructIter();
	static void printIter( int*  iter )
	{
		for ( int i = 0 ; i < MaxIndex ; ++i )
		{
			cout << iter[i]  << " ";
			if ( i % NumberNum == NumberNum - 1 )
				cout << endl;
		}
	}

	static int  sGroupIterator[3][ MaxIndex ];
};

#endif // SudokuSolver_h__

#include "SudokuSolver.hpp"