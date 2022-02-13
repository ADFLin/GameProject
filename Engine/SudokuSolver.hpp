template < class T , int N >
int TSudokuSolver<T,N>::sGroupIterator[3][ MaxIndex ];

template < class T , int N >
bool TSudokuSolver<T,N>::evalSolvedValueMethod( int index , Group group , int at )
{
	(void) at;

	if ( curSolution[index] )
		return false;

	unsigned posible = idxPosible[index];
	if ( bitCount( posible ) == 1 )
	{
		if ( _this()->OnPrevEvalMethod( eSolvedValue , group , index , posible ) )
		{
			fillNumber( index , posible );
			_this()->OnPostEvalMethod( eSolvedValue , group , index , posible );
			return true;
		}
	}
	return false;
}


template < class T , int N >
bool TSudokuSolver<T,N>::evalSingleValueMethod( int index , Group group , int at )
{
	if ( curSolution[index] )
		return false;

	unsigned pBit = idxPosible[ index ];
	while( pBit )
	{
		unsigned numBit = pBit & -pBit;
		pBit -= numBit;

		if ( calcGroupNumBitCount( group , at , numBit ) == 1 )
		{
			if ( _this()->OnPrevEvalMethod( eSingleValue , group , index , numBit ) )
			{
				fillNumber( index , numBit );
				_this()->OnPostEvalMethod( eSingleValue , group , index , numBit );
				return true;
			}
		}
	}

	return false;
}


template < class T , int N >
int TSudokuSolver<T,N>::calcGroupNumBitCount( Group group , int at , unsigned numBit )
{
	int result = 0;

	Iterator iter;
	for( iter.setGroupAt( group , at ); iter.haveMore(); ++iter )
	{
		int idx = iter.getIndex();
		if ( idxPosible[idx] & numBit )
			++result;
	}

	return result;
}

template < class T , int N >
bool TSudokuSolver<T,N>::evalNakedMethod(  int index , Group group , int at  )
{
	if ( curSolution[index] )
		return false;

	switch( bitCount( idxPosible[ index ] ) )
	{
	case 2:
		if ( !evalNakedPairOrderInternal( index , group ,at ) )
			return evalNakedTripleOrderInternal( index , group , at , 2 );
		break;
	case 3:
		return evalNakedTripleOrderInternal( index , group , at , 3 );
	}
	return false;
}


template < class T , int N >
bool TSudokuSolver<T,N>::evalNakedPairOrderInternal(  int index , Group group , int at  )
{
	assert( !curSolution[index] );

	unsigned posible = idxPosible[index];
	assert( bitCount( posible ) == 2 );

	Iterator iter1;
	for( iter1.setNextIndex( group , index ); iter1.haveMore(); ++iter1 )
	{
		int idx1 = iter1.getIndex();

		if ( curSolution[idx1] )
			continue;

		if ( idxPosible[idx1] == posible )
		{
			if ( _this()->OnPrevEvalMethod( eNakedPair , group , index  , posible ) )
			{
				Iterator iter2;
				for( iter2.setGroupAt( group , at ); iter2.haveMore(); ++iter2 )
				{
					int idx2 = iter2.getIndex();
					if ( idx2 != idx1 && idx2 != index )
						removeNumBit( idx2 , posible );
				}

				_this()->OnPostEvalMethod( eNakedPair , group , index  , posible );
				return true;
			}
			break;
		}
	}
	return false;
}

template < class T , int N >
void TSudokuSolver<T,N>::removeGroupNumBit( Group group , int at , unsigned numBit , int* skipIndex , int numSkip )
{
	Iterator iter;
	iter.setGroupAt( group , at );
	removeGroupNumBit( iter , numBit , skipIndex , numSkip );
}

template < class T , int N >
void TSudokuSolver<T,N>::removeGroupNumBit( Iterator& iter , unsigned numBit , int* skipIndex , int numSkip )
{
	for( ; iter.haveMore(); ++iter )
	{
		int idx = iter.getIndex();
		for( int i = 0 ; i < numSkip ; ++i )
		{
			if ( idx == skipIndex[i] )
				goto skip;
		}
		removeNumBit( idx , numBit );
skip:
		;
	}
}

template < class T , int N >
bool TSudokuSolver<T,N>::evalNakedOrderInternal(  int index , Group group , int at , int count )
{
	assert( !curSolution[index] );

	unsigned numBit = idxPosible[index];

	assert( bitCount( numBit ) == count );
	
	unsigned curNumBit = numBit;
	int      curCount  = count;
	int      acc       = 0;
	int      useIdx[ NumberNum ];

	useIdx[ acc++ ] = index;

	Iterator iter1;
	for( iter1.setNextIndex( group , index ); iter1.haveMore() ; ++iter1 )
	{
		int idx1 = iter1.getIndex();

		assert( idx1 != index );

		if ( curSolution[idx1] )
			continue;

		unsigned numBit1 = idxPosible[idx1];
		if ( numBit1 != ( numBit1 & curNumBit ) && 
			 curCount == 3 )
			 continue;

		curCount += bitCount( numBit1 & ~curNumBit );
		useIdx[ acc++ ] = idx1;
		++acc;

		if ( acc == 3 )
		{
			if ( _this()->OnPrevEvalMethod( eNakedTriple , group , index  , numBit ) )
			{
				removeGroupNumBit( group , at , numBit , useIdx , 3 );
				_this()->OnPostEvalMethod( eNakedTriple , group , index  , numBit );
				return true;
			}
		}
	}

	return false;
}



template < class T , int N >
bool TSudokuSolver<T,N>::evalNakedTripleOrderInternal(  int index , Group group , int at , int count )
{
	assert( !curSolution[index] );

	unsigned numBit = idxPosible[index];

	enum
	{
		TYPE_33X = 1, // 123 123 123 or 123 123 12
		TYPE_32X ,    // 123 12 23 or 123 12 123 
		TYPE_222 ,    // 12 23 31
		TYPE_23X ,    // 12 123 23 or 12 123 123 
	};
	Iterator iter1;
	for( iter1.setNextIndex( group , index ); iter1.haveMore() ; ++iter1 )
	{
		int idx1 = iter1.getIndex();

		if ( curSolution[idx1] )
			continue;

		int type = 0;
		unsigned numBit1 = idxPosible[idx1];
		unsigned testBit;

		switch( count )
		{
		case 3:
			if ( numBit1 == numBit )
			{
				type = TYPE_33X;
			} 
			// 123 12 23
			else if ( bitCount( numBit1 ) == 2 )
			{
				if ( bitCount( numBit & numBit1 ) == 2 )
					type = TYPE_32X;
			}
			break;
		case 2: 
			switch( bitCount( numBit1 ) )
			{
			case 2: // 12 23 31
				testBit = numBit1 ^ numBit;
				if ( bitCount( testBit ) == 2 )
					type = TYPE_222;
				break;
			case 3: //12 123 23 or 12 123 123 
				if ( bitCount( numBit & numBit1 ) == 2 )
					type = TYPE_23X;
			}
		}

		if ( !type )
			continue;

		Iterator iter2;
		for( iter2.setNextIndex( group , idx1 ); iter2.haveMore(); ++iter2 )
		{
			int idx2 = iter2.getIndex();

			if ( curSolution[idx2] )
				continue;

			unsigned numBit2 = idxPosible[idx2];

			bool match = false;
			switch( type )
			{
			case TYPE_33X:
				// 123 123 123
				if ( numBit2 == numBit1 )
					match = true;
				// 123 123 12
				else if ( bitCount( numBit2 ) == 2 && 
					      bitCount( numBit2 & numBit1 ) == 2 )
					match = true;
				break;
			case TYPE_32X: 
				// 123 12 123 
				if ( numBit2 == numBit )
					match = true;
				// 123 12 23
				else if ( bitCount( numBit2 ) == 2 &&
					      numBit == numBit1 | numBit2 )
					match = true;
				break;
			case TYPE_222:
				// 12 23 31
				if ( numBit2 == testBit )
					match = true;
				break;
			case TYPE_23X:
				// 12 123 123
				if ( numBit2 == numBit1 )
					match = true;
				//  12 123 23 
				else if ( bitCount( numBit2 ) == 2 &&
					      numBit1 == numBit | numBit2 )
					match = true;
				break;
			}

			if ( !match )
				continue;

			if ( _this()->OnPrevEvalMethod( eNakedTriple , group , index  , numBit ) )
			{
				Iterator iter;
				for( iter.setGroupAt( group , at ); iter.haveMore(); ++iter )
				{
					int idx = iter.getIndex();
					if ( idx != idx2 && idx != idx1 && idx != index )
						removeNumBit( idx , numBit );
				}
				_this()->OnPostEvalMethod( eNakedTriple , group , index  , numBit );
				return true;
			}
		}
	}
	return false;
}


template < class T , int N >
bool TSudokuSolver<T,N>::evalPointingMethod( int index , Group  group , int at )
{
	assert( group == BOX );

	bool result = false;
	unsigned pBit = idxPosible[ index ];
	while( pBit )
	{
		unsigned numBit = pBit & -pBit;
		pBit -= numBit;
		result |= evalPointingInternal( index ,  at , numBit );
	}
	return result;
}

template < class T , int N >
bool TSudokuSolver<T,N>::evalPointingInternal( int index , int at , unsigned numBit )
{
	int count = 0;
	int* iterator = sGroupIterator[BOX]; 

	int   lineAt ;
	Group lineGroup = BOX;

	Iterator iter;
	for ( iter.setGroupAt( BOX , at ); iter.haveMore() ; ++iter )
	{
		int idx = iter.getIndex();

		if ( curSolution[idx] )
			continue;

		if ( idx == index )
			continue;

		if ( !( idxPosible[idx] & numBit ) )
			continue;

		++count;

		if ( count > BoxSize )
			return false;

		if ( count == 1 )
		{
			if ( ( lineAt = getRow( index ) ) == getRow( idx ) )
				lineGroup = ROW;
			else if ( ( lineAt = getCol( index ) ) == getCol( idx ) )
				lineGroup = COL;
			else
				return false;
		}
		else
		{
			if ( lineGroup == ROW && lineAt != getRow( idx ) )
				return false;
			if ( lineGroup == COL && lineAt != getCol( idx ) )
				return false;
		}
	}

	if ( lineGroup == BOX )
		return false;

	if ( _this()->OnPrevEvalMethod( ePointing , BOX , index , numBit ) )
	{
		for ( iter.setGroupAt( lineGroup , lineAt ); iter.haveMore(); ++iter )
		{
			int idx = iter.getIndex();
			if ( getBox( idx ) != at )
				removeNumBit( idx ,numBit );
		}
		_this()->OnPostEvalMethod( ePointing , BOX , index , numBit );
		return true;
	}
	return false;
}


template < class T , int N >
bool TSudokuSolver<T,N>::checkNumInBox(  Group group , int at , int box , unsigned numBit )
{
	Iterator iter;
	for( iter.setGroupAt( group , at ); iter.haveMore(); ++iter )
	{
		int idx = iter.getIndex();
		if ( idxPosible[ idx ] & numBit )
		{
			if ( getBox( idx ) != box )
				return false;
		}
	}
	return true;
}

template < class T , int N >
bool TSudokuSolver<T,N>::evalBoxLineMethod( int index , Group group , int at )
{
	assert( group == BOX );
	int row = getRow( index );
	int col = getCol( index );

	bool result = false;

	unsigned pBit = idxPosible[ index ];
	while( pBit )
	{
		unsigned numBit = pBit & -pBit;
		pBit -= numBit;

		if ( checkNumInBox( COL , col , at , numBit ) )
		{
			if ( _this()->OnPrevEvalMethod( eBoxLine , BOX , index , numBit ) )
			{
				Iterator iter;
				for ( iter.setGroupAt( BOX , at ) ; iter.haveMore(); ++iter )
				{
					int idx = iter.getIndex();
					if ( getCol( idx ) != col )
						removeNumBit( idx ,numBit );
				}
				_this()->OnPostEvalMethod( eBoxLine , BOX , index , numBit );

				result |= true;
			}
		}
		if ( checkNumInBox( ROW , row , at , numBit ) )
		{
			if ( _this()->OnPrevEvalMethod( eBoxLine , BOX , index , numBit ) )
			{
				Iterator iter;
				for ( iter.setGroupAt( BOX , at ) ; iter.haveMore(); ++iter )
				{
					int idx = iter.getIndex();
					if ( getRow( idx ) != row )
						removeNumBit( idx ,numBit );
				}
				_this()->OnPostEvalMethod( eBoxLine , BOX , index , numBit );

				result |= true;
			}
		}
	}

	return result;
}

template < class T , int N >
bool TSudokuSolver<T,N>::evalXWingMethod(  int index , Group group , int at )
{
	bool result = false;

	unsigned pBit = idxPosible[ index ];
	while( pBit )
	{
		unsigned numBit = pBit & -pBit;
		pBit -= numBit;

		result |= evalXWingInternal( index , group , at , numBit );
	}

	return true;
}

template < class T , int N >
bool TSudokuSolver<T,N>::evalXWingInternal(  int index , Group group , int at , unsigned numBit )
{
	int  count = 0;

	int  cIndex[2][2];
	cIndex[0][0] = index;

	int  indexP;

	Iterator iter;
	int* iterator = sGroupIterator[ group ];
	for( iter.setGroupAt( group , at ); iter.haveMore() ; ++iter )
	{
		int idx = iter.getIndex();

		if ( idx == index )
			continue;

		if ( idxPosible[idx] & numBit )
		{
			++count;
			if ( count >= 2 )
				break;
			cIndex[1][0] = idx;
			indexP = idx;
		}
	}

	if ( count != 1 )
		return false;

	int maxOffset = NumberNum - at;
	Group rGroup = ( group == COL ) ? ROW : COL;

	for( int offset = 1 ; offset < maxOffset ; ++offset )
	{
		if ( calcGroupNumBitCount( group , at + offset , numBit ) != 2 )
			continue;

		cIndex[0][1] = offsetIndex( cIndex[0][0] , rGroup , offset );
		cIndex[1][1] = offsetIndex( cIndex[1][0] , rGroup , offset );

		if ( ( idxPosible[ cIndex[0][1] ] & numBit ) &&
			 ( idxPosible[ cIndex[1][1] ] & numBit ) )
		{
			if ( _this()->OnPrevEvalMethod( eXWing , group , index , numBit ) )
			{
				for( int i = 0 ; i < 2 ; ++i )
				{
					for( iter.setGroupIndex( rGroup , cIndex[i][0] ); iter.haveMore(); ++iter )
					{
						int idx = iter.getIndex();
						if ( idx != cIndex[i][0] && idx != cIndex[i][1] )
							removeNumBit( idx , numBit );
					}
				}
				_this()->OnPostEvalMethod( eXWing , group , index , numBit );
				return true;
			}
		}
	}

	return false;
}


template < class T , int N >
bool TSudokuSolver<T,N>::evalSwordFishMethod(  int index , Group group , int at )
{
	bool result = false;

	unsigned pBit = idxPosible[ index ];
	while( pBit )
	{
		unsigned numBit = pBit & -pBit;
		pBit -= numBit;

		result |= evalSwordFishInternal( index , group , at , numBit );
	}

	return true;
}

template < class T , int N >
bool TSudokuSolver<T,N>::evalSwordFishInternal(  int index , Group group , int at , unsigned numBit )
{
	int  count = 0;

	int  cIndex[3][3];
	cIndex[0][0] = index;

	int  indexP;

	Iterator iter;
	int* iterator = sGroupIterator[ group ];
	for( iter.setGroupAt( group , at ); iter.haveMore() ; ++iter )
	{
		int idx = iter.getIndex();

		if ( idx == index )
			continue;

		if ( idxPosible[idx] & numBit )
		{
			if ( count >= 2 )
			{
				++count;
				break;
			}

			cIndex[count][0] = idx;
			++count;
		}
	}

	if ( count != 2 )
		return false;

	int maxOffset = NumberNum - at;
	Group rGroup = ( group == COL ) ? ROW : COL;


	int offset;
	for( offset = 1 ; offset < maxOffset ; ++offset )
	{
		if ( calcGroupNumBitCount( group , at + offset , numBit ) != 3 )
			continue;

		cIndex[0][1] = offsetIndex( cIndex[0][0] , rGroup , offset );
		cIndex[1][1] = offsetIndex( cIndex[1][0] , rGroup , offset );
		cIndex[2][1] = offsetIndex( cIndex[2][0] , rGroup , offset );

		if ( ( idxPosible[ cIndex[0][1] ] & numBit ) &&
			 ( idxPosible[ cIndex[1][1] ] & numBit ) && 
			 ( idxPosible[ cIndex[2][1] ] & numBit ) )
			break;
	}


	for( ; offset < maxOffset ; ++offset )
	{
		if ( calcGroupNumBitCount( group , at + offset , numBit ) != 3 )
			continue;

		cIndex[0][2] = offsetIndex( cIndex[0][0] , rGroup , offset );
		cIndex[1][2] = offsetIndex( cIndex[1][0] , rGroup , offset );
		cIndex[2][2] = offsetIndex( cIndex[2][0] , rGroup , offset );

		if ( ( idxPosible[ cIndex[0][2] ] & numBit ) &&
			 ( idxPosible[ cIndex[1][2] ] & numBit ) && 
			 ( idxPosible[ cIndex[2][2] ] & numBit ) )
		{
			if ( _this()->OnPrevEvalMethod( eSwordFish , group , index , numBit ) )
			{
				for( int i = 0 ; i < 3 ; ++i )
				{
					iter.setGroupIndex( rGroup , cIndex[i][0] );
					removeGroupNumBit( iter , numBit , cIndex[i] , 3 );
				}
				_this()->OnPostEvalMethod( eSwordFish , group , index , numBit );
				return true;
			}
		}
	}
	return false;
}


template < class T , int N >
bool TSudokuSolver<T,N>::evalHiddenMethod(  int index , Group group , int at  )
{
	if ( curSolution[index] )
		return false;

	unsigned posibleNumBit[ NumberNum ];
	int num = 0;

	unsigned posible = idxPosible[index];
	while( posible )
	{
		unsigned bit = posible & (-posible );
		posibleNumBit[num++] = bit;
		posible -= bit;
	}

	if ( num >= 3 )
	{
		for( int i = 0 ; i < num ; ++i )
		{
			for( int j = i + 1 ; j < num ; ++j )
			{
				unsigned numBit = posibleNumBit[i] | posibleNumBit[j];
				if ( evalHiddenMethodInternal( index , group , at , 1 , numBit , eHiddenPair ) )
					return true;
			}
		}
	}

	if ( num >= 4 )
	{
		for( int i = 0 ; i < num ; ++i )
		{
			for( int j = i + 1 ; j < num ; ++j )
			{
				for( int k = j + 1 ; k < num ; ++k )
				{
					unsigned numBit = posibleNumBit[i] | posibleNumBit[j] | posibleNumBit[k];
					if ( evalHiddenMethodInternal( index , group , at , 1 , numBit , eHiddenTriple ) )
						return true;
				}
			}
		}
	}
	return false;
}

template < class T , int N >
bool TSudokuSolver<T,N>::evalHiddenMethodInternal(  int index , Group group , int at , int num , unsigned numBit , Method method  )
{
	int* iterator = sGroupIterator[ group ];
	int count = 0;
	int saveIndex[ NumberNum ];

	int idx1 = getIteratorBegin( group , at );
	for( ; idx1 != -1 ; idx1 = iterator[idx1] )
	{
		if ( curSolution[idx1] || index == idx1 )
			continue;

		if ( idxPosible[idx1] & numBit )
		{
			saveIndex[ count++ ] = idx1;
			if ( count > num )
				break;
		}
	}

	if ( count == num )
	{
		if ( _this()->OnPrevEvalMethod( method , group , index  , numBit ) )
		{
			unsigned invBit = ~numBit;

			removeNumBit(  index  , invBit );

			for( int i = 0 ; i < num ; ++i )
			{
				removeNumBit( saveIndex[i] , invBit );
			}
			
			int idx2 = getIteratorBegin(  group , at );
			int next = 0;
			saveIndex[count] = -1;
			for( ; idx2 != -1 ; idx2 = iterator[idx2] )
			{
				if ( idx2 == index )
					continue;
				if ( idx2 == saveIndex[next] )
				{
					++next;
					continue;
				}

				removeNumBit( idx2 , numBit );
			}

			_this()->OnPostEvalMethod( method , group , index  , numBit );
			return true;
		}
	}

	return false;
}

template < class T , int N >
bool TSudokuSolver<T,N>::evalYWingMethod( int index , Group group , int at )
{
	assert( group != BOX );

	if ( bitCount( idxPosible[index] ) != 2 )
		return false;

	bool result = false;

	int* iterator = sGroupIterator[ group ];
	int  box = getBox( index );
	for( int idx = getIteratorBegin( group , at ) ; idx != -1 ; idx = iterator[idx] )
	{
		if ( curSolution[idx] )
			continue;
		if ( idx == index )
			continue;
		if ( getBox( idx ) == box )
			continue;


		if ( bitCount( idxPosible[ idx ] & idxPosible[index] ) != 1 )
			continue;

		static Group const nextGroup[] = { ROW , BOX , COL };

		Group nGroup = group;
		for ( int i = 0 ; i < 2 ; ++i )
		{
			nGroup = nextGroup[ nGroup ];
			result |= evalYWingInternal( index , group , at , nGroup , idx );
		}
	}
	return result;
}

template < class T , int N >
bool TSudokuSolver<T,N>::evalYWingInternal( int index , Group group , int at , Group nGroup , int nIdx )
{
	int* iterator = sGroupIterator[ nGroup ];

	unsigned numBit = idxPosible[ nIdx ] ^ idxPosible[index];
	unsigned removeBit = idxPosible[ nIdx ] &  numBit;

	bool result = false;

	Iterator iter;
	for( iter.setGroupIndex( nGroup , index ); iter.haveMore(); ++iter )
	{
		int idx = iter.getIndex();

		if ( curSolution[idx] )
			continue;

		if ( idx == index )
			continue;

		if ( nGroup == BOX )
		{
			int testAt = ( group == COL ) ? getCol( idx ) : getRow( idx );
			if ( testAt == at )
				continue;
		}

		if ( numBit != idxPosible[idx] )
			continue;

		if ( _this()->OnPrevEvalMethod( eYWing , group , index  , idxPosible[ index ] | removeBit  ) )
		{
			switch( nGroup )
			{
			case COL:
				assert( group == ROW );
				removeNumBit( getIndex( getCol(idx) , getRow(index) ) , removeBit );
				break;
			case ROW:
				assert( group == COL );
				removeNumBit( getIndex( getCol(index) , getRow(idx) ) , removeBit );
				break;
			case BOX:
				{
					int box;
					Iterator iter1;
					int* iterator = sGroupIterator[ group ];

					box = getBox( nIdx );
					for( iter1.setGroupIndex( group , idx ); iter1.haveMore(); ++iter1 )
					{
						int idx1 = iter1.getIndex();
						if ( getBox( idx1) == box  )
							removeNumBit( idx1 , removeBit );
					}

					box = getBox( index );
					for( iter1.setGroupAt( group , at ); iter1.haveMore(); ++iter1 )
					{
						int idx1 = iter1.getIndex();
						if ( getBox( idx1) == box && idx1 != index )
							removeNumBit( idx1 , removeBit );
					}
				}
				break;
			}
			_this()->OnPostEvalMethod( eYWing , group , index  , idxPosible[ index ] | removeBit );
			result = true;
		}
	}
	return result;
}

template < class T , int N >
bool TSudokuSolver<T,N>::evalSimpleColourMethod( int index , Group group  , int at  )
{
	(void) group;
	(void) at;
	bool result = false;

	unsigned pBit = idxPosible[ index ];
	while( pBit )
	{
		unsigned numBit = pBit & -pBit;
		pBit -= numBit;
		result |= evalSimpleColourInternal( index , numBit );
	}

	return result;
}

template < class T , int N >
bool TSudokuSolver<T,N>::evalSimpleColourInternal( int index , unsigned numBit )
{
	enum
	{
		RED        =  1,
		BLUE       = -1,
		BOTH_COLOR =  2,
	};


	if ( index == 19 && numBit & BIT( 2 - 1 ) )
	{
		int aa = 1;
	}

	int color[ MaxIndex ];
	std::fill_n( color , MaxIndex , 0 );
	int usedIndex[ MaxIndex ];
	std::fill_n( usedIndex , MaxIndex , -1 );
	int numUsed = 0;

	int  curTest = 0;

	unsigned groupColor[3][ NumberNum ];
	std::fill_n( &groupColor[0][0] , 3 * NumberNum , 0 );

	color[ index ] = RED;
	int col = getCol( index );
	int row = getRow( index );
	int box = getBox( col , row );

	groupColor[COL][col] = RED;
	groupColor[ROW][row] = RED;
	groupColor[BOX][box] = RED;
	usedIndex[ numUsed++ ] = index;

	bool result = false;

	int removeColor = 0;

	while( curTest < numUsed )
	{
		int testIndex = usedIndex[ curTest ];

		assert( testIndex != -1 );

		for( int g = 0 ; g < 3 ; ++g )
		{
			int pIndex[2];
			if ( generateGroupNumBitIndex(  Group(g) , testIndex , numBit , pIndex , 1 ) != 1 )
				continue;

			int pairIndex = pIndex[0];

			if ( color[ pairIndex ] )
			{
				if ( color[testIndex] == color[ pairIndex ] )
				{
					if ( _this()->OnPrevEvalMethod( eSimpleColour , NO_GROUP , index , numBit ) )
					{
						removeNumBit( pairIndex , numBit );
						_this()->OnPostEvalMethod( eSimpleColour , NO_GROUP , index , numBit );
						result |= true;
					}
				}
			}
			else
			{
				color[ pairIndex ] = -color[ testIndex ];

				int gAt[3];
				gAt[COL] = getCol( pairIndex );
				gAt[ROW] = getRow( pairIndex );
				gAt[BOX] = getBox( gAt[COL] , gAt[ROW] );

				for( int g = 0 ; g < 3 ; ++g )
				{
					int at = gAt[g];
					if ( !groupColor[ g ][ at ] )
					{
						groupColor[ g ][ at ] = color[ pairIndex ];
					}
					else if (  groupColor[ g ][ at ] == BOTH_COLOR )
					{
						removeColor = color[ pairIndex ];
					}
					else
					{
						if ( groupColor[ g ][ at ] == color[ pairIndex ] )
						{
							removeColor = color[ pairIndex ];
						}
						else
						{
							groupColor[ g ][ at ] = BOTH_COLOR;
						}
					}
				}
				usedIndex[ numUsed++ ] = pairIndex;
			}
		}

		++curTest;
	}

	if ( removeColor )
	{
		if ( _this()->OnPrevEvalMethod( eSimpleColour , NO_GROUP , index , numBit ) )
		{
			for( int i = 0 ; i < numUsed ; ++i )
			{
				int idx = usedIndex[ i ];
				if ( color[idx] == removeColor )
					removeNumBit( idx , numBit );
			}
			_this()->OnPostEvalMethod( eSimpleColour , NO_GROUP , index , numBit );
			result |= true;
		}
	}

	return result;
}


template < class T , int N >
void TSudokuSolver<T,N>::doSolve( int idx )
{
	if ( idx == MaxIndex )
	{
		if ( numSolution == 16 )
			return;

		for( int i = 0 ; i < MaxIndex ; ++i )
			solution[ numSolution ][i] = curSolution[i];

		++numSolution;
		return;
	}

	if ( problem[ idx ] )
	{
		curSolution[idx] = 1 << ( problem[idx] - 1 );

		doSolve( idx + 1 );
		return;
	}

	int col = getCol( idx );
	int row = getRow( idx );
	int box = getBox( col , row );

	unsigned posible = groupPosible[COL][col] & groupPosible[ROW][row] & groupPosible[BOX][box];

	while ( posible )
	{
		int pn = posible & -posible;

		curSolution[idx] = pn;

		posible &= ~pn;

		groupPosible[COL][col] &= ~pn;
		groupPosible[ROW][row] &= ~pn;
		groupPosible[BOX][box] &= ~pn;

		doSolve( idx + 1 );

		groupPosible[COL][col] |= pn;
		groupPosible[ROW][row] |= pn;
		groupPosible[BOX][box] |= pn;
	}
}

template < class T , int N >
int TSudokuSolver<T,N>::getIteratorBegin( Group group , int at )
{
	switch( group )
	{
	case COL:  return at;
	case ROW:  return at * NumberNum;
	case BOX:  return BoxSize * ( at % BoxSize ) + NumberNum * ( BoxSize * ( at / BoxSize ) );
	default:
		return -1;
	}
}

template < class T , int N >
int TSudokuSolver<T,N>::getIteratorIndexBegin( Group group , int index )
{
	switch( group )
	{
	case COL: return getCol( index );
	case ROW: return getRow( index ) * NumberNum;
	case BOX: 
		{
			int at = getBox( index );
			return BoxSize * ( at % BoxSize ) + NumberNum * ( BoxSize * ( at / BoxSize ) );
		}
	}
	assert( 0 );
	return -1;
}


template < class T , int N >
void TSudokuSolver<T,N>::contructIter()
{
	static bool isContructed = false;
	if ( isContructed )
		return;

	isContructed = true;

	for( int i = 0 ; i < MaxIndex ; ++i )
	{
		sGroupIterator[ COL ][ i ] = i + NumberNum;
		sGroupIterator[ ROW ][ i ] = i + 1;
		sGroupIterator[ BOX ][ i ] = i + 1;
	}

	for ( int i = 0 ; i < NumberNum ; ++ i )
	{
		sGroupIterator[ ROW ][ ( i + 1 ) * NumberNum - 1] = -1;
		sGroupIterator[ COL ][ ( NumberNum - 1 ) * NumberNum + i] = -1;

		for ( int n = 0 ; n < BoxSize ; ++ n )
		{
			int idx = NumberNum * i + ( n + 1 ) * BoxSize - 1;
			if ( i % BoxSize == BoxSize - 1 )
				sGroupIterator[BOX][idx] = -1;
			else
				sGroupIterator[BOX][idx] = idx + NumberNum - ( BoxSize - 1 );
		}
	}

#ifdef _DEBUG
	printIter( sGroupIterator[ROW] );
	printIter( sGroupIterator[COL] );
	printIter( sGroupIterator[BOX] );

	for ( int group = 0 ; group < 3 ; ++group )
	for ( int at = 0; at < NumberNum ; ++at )
	{
		int count = 0;
		int idx = getIteratorBegin( Group(group) , at );
		for( ; idx != -1 ; idx = sGroupIterator[ group ][ idx ] )
			++count;

		assert( count == 9 );
	}
#endif //_DEBUG
}


template < class T , int N >
int  TSudokuSolver<T,N>::generateGroupNumBitIndex( Group group , int index , unsigned numBit , int pIndex[] , int maxNum )
{
	int count = 0;
	assert( idxPosible[index] & numBit );
	Iterator iter;
	for( iter.setGroupIndex( group , index ); iter.haveMore(); ++iter )
	{
		int idx = iter.getIndex();

		if ( idx == index )
			continue;

		if ( idxPosible[idx] & numBit )
		{
			if ( count > maxNum )
				return ++count;

			pIndex[ count++ ] = idx;
		}
	}
	return count;
}

template < class T , int N >
void TSudokuSolver<T,N>::init( int* prob )
{
	numSolution = 0;
	for( int n = 0; n < NumberNum ; ++n )
	{
		groupPosible[COL][n] = NumberBitFill;
		groupPosible[ROW][n] = NumberBitFill;
		groupPosible[BOX][n] = NumberBitFill;
	}

	problem = prob;
	for ( int n = 0 ; n < MaxIndex ; ++n )
	{
		indexCheck[ n ] = true;


		if ( problem[n] == 0 )
			continue;

		unsigned bit = ~BIT( problem[n] - 1 );

		int col = getCol( n );
		int row = getRow( n );
		int box = getBox( col , row );

		groupPosible[COL][ col ] &= bit;
		groupPosible[ROW][ row ] &= bit;
		groupPosible[BOX][ box ] &= bit;

		curSolution[n] = 0;
		indexCheck[ n ] = false;
	}
}

