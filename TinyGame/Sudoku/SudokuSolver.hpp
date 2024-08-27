template < class T , int BS>
int SudokuSolverT<T,BS>::sGroupNextIndex[3][ MaxIndex ];

template < class T , int BS>
bool SudokuSolverT<T,BS>::evalSolvedValueMethod( int index , Group group , int idxGroup )
{
	(void) idxGroup;

	if ( mCurSolution[index] )
		return false;

	unsigned posible = mPosibleBitsCell[index];
	if ( isOneBitSet( posible ) )
	{
		if ( _this()->onPrevEvalMethod(EMethod::SolvedValue , group , index , posible ) )
		{
			fillNumber( index , posible );
			_this()->onPostEvalMethod(EMethod::SolvedValue , group , index , posible );
			return true;
		}
	}
	return false;
}


template < class T , int BS>
bool SudokuSolverT<T,BS>::evalSingleValueMethod( int index , Group group , int idxGroup )
{
	if ( mCurSolution[index] )
		return false;

	unsigned pBit = mPosibleBitsCell[ index ];
	while( pBit )
	{
		unsigned numBits = FBitUtility::ExtractTrailingBit(pBit);
		pBit -= numBits;

		if ( calcGroupNumBitCount( group , idxGroup , numBits ) == 1 )
		{
			if ( _this()->onPrevEvalMethod(EMethod::SingleValue , group , index , numBits ) )
			{
				fillNumber( index , numBits );
				_this()->onPostEvalMethod(EMethod::SingleValue , group , index , numBits );
				return true;
			}
		}
	}

	return false;
}


template < class T , int BS>
int SudokuSolverT<T,BS>::calcGroupNumBitCount( Group group , int idxGroup , unsigned numBits )
{
	int result = 0;

	Iterator iter;
	for( iter.setGroupIndex( group , idxGroup ); iter ; ++iter )
	{
		int idx = iter.getCellIndex();
		if ( mPosibleBitsCell[idx] & numBits )
			++result;
	}

	return result;
}

template < class T , int BS>
bool SudokuSolverT<T,BS>::evalNakedMethod(  int index , Group group , int idxGroup  )
{
	if ( mCurSolution[index] )
		return false;

	switch( bitCount( mPosibleBitsCell[ index ] ) )
	{
	case 2:
		if ( !evalNakedPairOrderInternal( index , group ,idxGroup ) )
			return evalNakedTripleOrderInternal( index , group , idxGroup , 2 );
		break;
	case 3:
		return evalNakedTripleOrderInternal( index , group , idxGroup , 3 );
	}
	return false;
}


template < class T , int BS>
bool SudokuSolverT<T,BS>::evalNakedPairOrderInternal(  int index , Group group , int idxGroup  )
{
	assert( !mCurSolution[index] );

	unsigned posible = mPosibleBitsCell[index];
	assert( bitCount( posible ) == 2 );

	Iterator iter1;
	for( iter1.setNextIndex( group , index ); iter1; ++iter1 )
	{
		int idx1 = iter1.getCellIndex();

		if ( mCurSolution[idx1] )
			continue;

		if ( mPosibleBitsCell[idx1] == posible )
		{
			if ( _this()->onPrevEvalMethod(EMethod::NakedPair , group , index  , posible ) )
			{
				Iterator iter2;
				for( iter2.setGroupIndex( group , idxGroup ); iter2; ++iter2 )
				{
					int idx2 = iter2.getCellIndex();
					if ( idx2 != idx1 && idx2 != index )
						removeNumBit( idx2 , posible );
				}

				_this()->onPostEvalMethod(EMethod::NakedPair , group , index  , posible );
				return true;
			}
			break;
		}
	}
	return false;
}

template < class T , int BS>
void SudokuSolverT<T,BS>::removeGroupNumBit( Group group , int idxGroup , unsigned numBits , int* skipIndex , int numSkip )
{
	Iterator iter;
	iter.setGroupIndex( group , idxGroup );
	removeGroupNumBit( iter , numBits , skipIndex , numSkip );
}

template < class T , int BS>
void SudokuSolverT<T,BS>::removeGroupNumBit( Iterator& iter , unsigned numBits , int* skipIndex , int numSkip )
{
	for( ; iter; ++iter )
	{
		int idx = iter.getCellIndex();
		for( int i = 0 ; i < numSkip ; ++i )
		{
			if ( idx == skipIndex[i] )
				goto skip;
		}
		removeNumBit( idx , numBits );
skip:
		;
	}
}

template < class T , int BS>
bool SudokuSolverT<T,BS>::evalNakedOrderInternal(  int index , Group group , int idxGroup , int count )
{
	assert( !mCurSolution[index] );

	unsigned numBits = mPosibleBitsCell[index];

	assert( bitCount( numBits ) == count );
	
	unsigned curNumBit = numBits;
	int      curCount  = count;
	int      acc       = 0;
	int      useIdx[ NumberNum ];

	useIdx[ acc++ ] = index;

	Iterator iter1;
	for( iter1.setNextIndex( group , index ); iter1 ; ++iter1 )
	{
		int idx1 = iter1.getCellIndex();

		assert( idx1 != index );

		if ( mCurSolution[idx1] )
			continue;

		unsigned numBit1 = mPosibleBitsCell[idx1];
		if ( numBit1 != ( numBit1 & curNumBit ) && 
			 curCount == 3 )
			 continue;

		curCount += bitCount( numBit1 & ~curNumBit );
		useIdx[ acc++ ] = idx1;
		++acc;

		if ( acc == 3 )
		{
			if ( _this()->onPrevEvalMethod(EMethod::NakedTriple , group , index  , numBits ) )
			{
				removeGroupNumBit( group , idxGroup , numBits , useIdx , 3 );
				_this()->onPostEvalMethod(EMethod::NakedTriple , group , index  , numBits );
				return true;
			}
		}
	}

	return false;
}



template < class T , int BS>
bool SudokuSolverT<T,BS>::evalNakedTripleOrderInternal(  int index , Group group , int idxGroup , int count )
{
	assert( !mCurSolution[index] );

	unsigned numBits = mPosibleBitsCell[index];

	enum
	{
		TYPE_33X = 1, // 123 123 123 or 123 123 12
		TYPE_32X ,    // 123 12 23 or 123 12 123 
		TYPE_222 ,    // 12 23 31
		TYPE_23X ,    // 12 123 23 or 12 123 123 
	};
	Iterator iter1;
	for( iter1.setNextIndex( group , index ); iter1 ; ++iter1 )
	{
		int idx1 = iter1.getCellIndex();

		if ( mCurSolution[idx1] )
			continue;

		int type = 0;
		unsigned numBit1 = mPosibleBitsCell[idx1];
		unsigned testBit;

		switch( count )
		{
		case 3:
			if ( numBit1 == numBits )
			{
				type = TYPE_33X;
			} 
			// 123 12 23
			else if ( bitCount( numBit1 ) == 2 )
			{
				if ( bitCount( numBits & numBit1 ) == 2 )
					type = TYPE_32X;
			}
			break;
		case 2: 
			switch( bitCount( numBit1 ) )
			{
			case 2: // 12 23 31
				testBit = numBit1 ^ numBits;
				if ( bitCount( testBit ) == 2 )
					type = TYPE_222;
				break;
			case 3: //12 123 23 or 12 123 123 
				if ( bitCount( numBits & numBit1 ) == 2 )
					type = TYPE_23X;
			}
		}

		if ( !type )
			continue;

		Iterator iter2;
		for( iter2.setNextIndex( group , idx1 ); iter2; ++iter2 )
		{
			int idx2 = iter2.getCellIndex();

			if ( mCurSolution[idx2] )
				continue;

			unsigned numBit2 = mPosibleBitsCell[idx2];

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
				if ( numBit2 == numBits )
					match = true;
				// 123 12 23
				else if ( bitCount( numBit2 ) == 2 && numBits == (numBit1 | numBit2) )
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
				else if ( bitCount( numBit2 ) == 2 &&  numBit1 == (numBits | numBit2) )
					match = true;
				break;
			}

			if ( !match )
				continue;

			if ( _this()->onPrevEvalMethod(EMethod::NakedTriple , group , index  , numBits ) )
			{
				Iterator iter;
				for( iter.setGroupIndex( group , idxGroup ); iter; ++iter )
				{
					int idx = iter.getCellIndex();
					if ( idx != idx2 && idx != idx1 && idx != index )
						removeNumBit( idx , numBits );
				}
				_this()->onPostEvalMethod(EMethod::NakedTriple , group , index  , numBits );
				return true;
			}
		}
	}
	return false;
}


template < class T , int BS>
bool SudokuSolverT<T,BS>::evalPointingMethod( int index , Group  group , int idxGroup )
{
	assert( group == BOX );

	bool result = false;
	unsigned pBit = mPosibleBitsCell[ index ];
	while( pBit )
	{
		unsigned numBits = FBitUtility::ExtractTrailingBit(pBit);
		pBit -= numBits;
		result |= evalPointingInternal( index ,  idxGroup , numBits );
	}
	return result;
}

template < class T , int BS>
bool SudokuSolverT<T,BS>::evalPointingInternal( int index , int idxGroup , unsigned numBits )
{
	int count = 0;

	int   lineAt ;
	Group lineGroup = BOX;

	Iterator iter;
	for ( iter.setGroupIndex( BOX , idxGroup ); iter ; ++iter )
	{
		int idx = iter.getCellIndex();

		if ( mCurSolution[idx] )
			continue;

		if ( idx == index )
			continue;

		if ( !( mPosibleBitsCell[idx] & numBits ) )
			continue;

		++count;

		if ( count > BoxSize )
			return false;

		if ( count == 1 )
		{
			if ( ( lineAt = RowIndex( index ) ) == RowIndex( idx ) )
				lineGroup = ROW;
			else if ( ( lineAt = ColIndex( index ) ) == ColIndex( idx ) )
				lineGroup = COL;
			else
				return false;
		}
		else
		{
			if ( lineGroup == ROW && lineAt != RowIndex( idx ) )
				return false;
			if ( lineGroup == COL && lineAt != ColIndex( idx ) )
				return false;
		}
	}

	if ( lineGroup == BOX )
		return false;

	if ( _this()->onPrevEvalMethod(EMethod::Pointing , BOX , index , numBits ) )
	{
		for ( iter.setGroupIndex( lineGroup , lineAt ); iter; ++iter )
		{
			int idx = iter.getCellIndex();
			if ( BoxIndex( idx ) != idxGroup )
				removeNumBit( idx ,numBits );
		}
		_this()->onPostEvalMethod(EMethod::Pointing , BOX , index , numBits );
		return true;
	}
	return false;
}


template < class T , int BS>
bool SudokuSolverT<T,BS>::checkNumInBox(  Group group , int idxGroup , int box , unsigned numBits )
{
	for( Iterator iter = Iterator::FromGroupIndex( group , idxGroup ); 
		 iter; ++iter )
	{
		int idx = iter.getCellIndex();
		if ( mPosibleBitsCell[ idx ] & numBits )
		{
			if ( BoxIndex( idx ) != box )
				return false;
		}
	}
	return true;
}

template < class T , int BS>
bool SudokuSolverT<T,BS>::evalBoxLineMethod( int index , Group group , int idxGroup )
{
	assert( group == BOX );
	int row = RowIndex( index );
	int col = ColIndex( index );

	bool result = false;

	unsigned pBit = mPosibleBitsCell[ index ];
	while( pBit )
	{
		unsigned numBits = FBitUtility::ExtractTrailingBit(pBit);
		pBit -= numBits;

		if ( checkNumInBox( COL , col , idxGroup , numBits ) )
		{
			if ( _this()->onPrevEvalMethod(EMethod::BoxLine , BOX , index , numBits ) )
			{
				Iterator iter;
				for ( iter.setGroupIndex( BOX , idxGroup ) ; iter ; ++iter )
				{
					int idx = iter.getCellIndex();
					if( ColIndex(idx) != col )
					{
						removeNumBit(idx, numBits);
					}
				}
				_this()->onPostEvalMethod(EMethod::BoxLine , BOX , index , numBits );

				result |= true;
			}
		}
		if ( checkNumInBox( ROW , row , idxGroup , numBits ) )
		{
			if ( _this()->onPrevEvalMethod(EMethod::BoxLine , BOX , index , numBits ) )
			{
				Iterator iter;
				for ( iter.setGroupIndex( BOX , idxGroup ) ; iter ; ++iter )
				{
					int idx = iter.getCellIndex();
					if ( RowIndex( idx ) != row )
					{
						removeNumBit( idx ,numBits );
					}
				}
				_this()->onPostEvalMethod(EMethod::BoxLine , BOX , index , numBits );

				result |= true;
			}
		}
	}

	return result;
}

template < class T , int BS>
bool SudokuSolverT<T,BS>::evalXWingMethod(  int index , Group group , int idxGroup )
{
	bool result = false;

	unsigned pBit = mPosibleBitsCell[ index ];
	while( pBit )
	{
		unsigned numBits = FBitUtility::ExtractTrailingBit(pBit);
		pBit -= numBits;

		result |= evalXWingInternal( index , group , idxGroup , numBits );
	}

	return true;
}

template < class T , int BS>
bool SudokuSolverT<T,BS>::evalXWingInternal(  int index , Group group , int idxGroup , unsigned numBits )
{
	int  count = 0;

	int  cIndex[2][2];
	cIndex[0][0] = index;

	int  indexP;

	Iterator iter;
	for( iter.setGroupIndex( group , idxGroup ); iter ; ++iter )
	{
		int idx = iter.getCellIndex();

		if ( idx == index )
			continue;

		if ( mPosibleBitsCell[idx] & numBits )
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

	int maxOffset = NumberNum - idxGroup;
	Group rGroup = ( group == COL ) ? ROW : COL;

	for( int offset = 1 ; offset < maxOffset ; ++offset )
	{
		if ( calcGroupNumBitCount( group , idxGroup + offset , numBits ) != 2 )
			continue;

		cIndex[0][1] = OffsetCellIndex( cIndex[0][0] , rGroup , offset );
		cIndex[1][1] = OffsetCellIndex( cIndex[1][0] , rGroup , offset );

		if ( ( mPosibleBitsCell[ cIndex[0][1] ] & numBits ) &&
			 ( mPosibleBitsCell[ cIndex[1][1] ] & numBits ) )
		{
			if ( _this()->onPrevEvalMethod(EMethod::XWing , group , index , numBits ) )
			{
				for( int i = 0 ; i < 2 ; ++i )
				{
					for( iter.setCellIndex( rGroup , cIndex[i][0] ); iter; ++iter )
					{
						int idx = iter.getCellIndex();
						if ( idx != cIndex[i][0] && idx != cIndex[i][1] )
							removeNumBit( idx , numBits );
					}
				}
				_this()->onPostEvalMethod(EMethod::XWing , group , index , numBits );
				return true;
			}
		}
	}

	return false;
}


template < class T , int BS>
bool SudokuSolverT<T,BS>::evalSwordFishMethod(  int index , Group group , int idxGroup )
{
	bool result = false;

	unsigned pBit = mPosibleBitsCell[ index ];
	while( pBit )
	{
		unsigned numBits = pBit & -pBit;
		pBit -= numBits;

		result |= evalSwordFishInternal( index , group , idxGroup , numBits );
	}

	return true;
}

template < class T , int BS>
bool SudokuSolverT<T,BS>::evalSwordFishInternal(  int index , Group group , int idxGroup , unsigned numBits )
{
	int  count = 0;

	int  cIndex[3][3];
	cIndex[0][0] = index;

	int  indexP;

	Iterator iter;
	int* iterator = sGroupNextIndex[ group ];
	for( iter.setGroupIndex( group , idxGroup ); iter ; ++iter )
	{
		int idx = iter.getCellIndex();

		if ( idx == index )
			continue;

		if ( mPosibleBitsCell[idx] & numBits )
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

	int maxOffset = NumberNum - idxGroup;
	Group rGroup = ( group == COL ) ? ROW : COL;


	int offset;
	for( offset = 1 ; offset < maxOffset ; ++offset )
	{
		if ( calcGroupNumBitCount( group , idxGroup + offset , numBits ) != 3 )
			continue;

		cIndex[0][1] = OffsetCellIndex( cIndex[0][0] , rGroup , offset );
		cIndex[1][1] = OffsetCellIndex( cIndex[1][0] , rGroup , offset );
		cIndex[2][1] = OffsetCellIndex( cIndex[2][0] , rGroup , offset );

		if ( ( mPosibleBitsCell[ cIndex[0][1] ] & numBits ) &&
			 ( mPosibleBitsCell[ cIndex[1][1] ] & numBits ) && 
			 ( mPosibleBitsCell[ cIndex[2][1] ] & numBits ) )
			break;
	}


	for( ; offset < maxOffset ; ++offset )
	{
		if ( calcGroupNumBitCount( group , idxGroup + offset , numBits ) != 3 )
			continue;

		cIndex[0][2] = OffsetCellIndex( cIndex[0][0] , rGroup , offset );
		cIndex[1][2] = OffsetCellIndex( cIndex[1][0] , rGroup , offset );
		cIndex[2][2] = OffsetCellIndex( cIndex[2][0] , rGroup , offset );

		if ( ( mPosibleBitsCell[ cIndex[0][2] ] & numBits ) &&
			 ( mPosibleBitsCell[ cIndex[1][2] ] & numBits ) && 
			 ( mPosibleBitsCell[ cIndex[2][2] ] & numBits ) )
		{
			if ( _this()->onPrevEvalMethod( EMethod::SwordFish , group , index , numBits ) )
			{
				for( int i = 0 ; i < 3 ; ++i )
				{
					iter.setCellIndex( rGroup , cIndex[i][0] );
					removeGroupNumBit( iter , numBits , cIndex[i] , 3 );
				}
				_this()->onPostEvalMethod(EMethod::SwordFish , group , index , numBits );
				return true;
			}
		}
	}
	return false;
}


template < class T , int BS>
bool SudokuSolverT<T,BS>::evalHiddenMethod(  int index , Group group , int idxGroup  )
{
	if ( mCurSolution[index] )
		return false;

	unsigned posibleNumBit[ NumberNum ];
	int num = 0;

	unsigned posible = mPosibleBitsCell[index];
	while( posible )
	{
		unsigned bit = FBitUtility::ExtractTrailingBit(posible);
		posibleNumBit[num++] = bit;
		posible -= bit;
	}

	if ( num >= 3 )
	{
		for( int i = 0 ; i < num ; ++i )
		{
			for( int j = i + 1 ; j < num ; ++j )
			{
				unsigned numBits = posibleNumBit[i] | posibleNumBit[j];
				if ( evalHiddenMethodInternal( index , group , idxGroup , 1 , numBits , EMethod::HiddenPair ) )
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
					unsigned numBits = posibleNumBit[i] | posibleNumBit[j] | posibleNumBit[k];
					if ( evalHiddenMethodInternal( index , group , idxGroup , 1 , numBits , EMethod::HiddenTriple ) )
						return true;
				}
			}
		}
	}
	return false;
}

template < class T , int BS>
bool SudokuSolverT<T,BS>::evalHiddenMethodInternal(  int index , Group group , int idxGroup , int num , unsigned numBits , EMethod method  )
{
	int* iterator = sGroupNextIndex[ group ];
	int count = 0;
	int saveIndex[ NumberNum ];

	int idx1 = GetIteratorBeginFromGroupIndex( group , idxGroup );
	for( ; idx1 != -1 ; idx1 = iterator[idx1] )
	{
		if ( mCurSolution[idx1] || index == idx1 )
			continue;

		if ( mPosibleBitsCell[idx1] & numBits )
		{
			saveIndex[ count++ ] = idx1;
			if ( count > num )
				break;
		}
	}

	if ( count == num )
	{
		if ( _this()->onPrevEvalMethod( method , group , index  , numBits ) )
		{
			unsigned invBit = ~numBits;

			removeNumBit(  index  , invBit );

			for( int i = 0 ; i < num ; ++i )
			{
				removeNumBit( saveIndex[i] , invBit );
			}
			
			int idx2 = GetIteratorBeginFromGroupIndex(  group , idxGroup );
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

				removeNumBit( idx2 , numBits );
			}

			_this()->onPostEvalMethod( method , group , index  , numBits );
			return true;
		}
	}

	return false;
}

template < class T , int BS>
bool SudokuSolverT<T,BS>::evalYWingMethod( int index , Group group , int idxGroup )
{
	assert( group != BOX );

	if ( bitCount( mPosibleBitsCell[index] ) != 2 )
		return false;

	bool result = false;

	int* iterator = sGroupNextIndex[ group ];
	int  box = BoxIndex( index );
	for( int idx = GetIteratorBeginFromGroupIndex( group , idxGroup ) ; idx != -1 ; idx = iterator[idx] )
	{
		if ( mCurSolution[idx] )
			continue;
		if ( idx == index )
			continue;
		if ( BoxIndex( idx ) == box )
			continue;


		if ( bitCount( mPosibleBitsCell[ idx ] & mPosibleBitsCell[index] ) != 1 )
			continue;

		static Group const nextGroup[] = { ROW , BOX , COL };

		Group nGroup = group;
		for ( int i = 0 ; i < 2 ; ++i )
		{
			nGroup = nextGroup[ nGroup ];
			result |= evalYWingInternal( index , group , idxGroup , nGroup , idx );
		}
	}
	return result;
}

template < class T , int BS>
bool SudokuSolverT<T,BS>::evalYWingInternal( int index , Group group , int idxGroup , Group nGroup , int nIdx )
{
	int* iterator = sGroupNextIndex[ nGroup ];

	unsigned numBits = mPosibleBitsCell[ nIdx ] ^ mPosibleBitsCell[index];
	unsigned removeBit = mPosibleBitsCell[ nIdx ] &  numBits;

	bool result = false;

	Iterator iter;
	for( iter.setCellIndex( nGroup , index ); iter; ++iter )
	{
		int idx = iter.getCellIndex();

		if ( mCurSolution[idx] )
			continue;

		if ( idx == index )
			continue;

		if ( nGroup == BOX )
		{
			int testAt = ( group == COL ) ? ColIndex( idx ) : RowIndex( idx );
			if ( testAt == idxGroup )
				continue;
		}

		if ( numBits != mPosibleBitsCell[idx] )
			continue;

		if ( _this()->onPrevEvalMethod(EMethod::YWing , group , index  , mPosibleBitsCell[ index ] | removeBit  ) )
		{
			switch( nGroup )
			{
			case COL:
				assert( group == ROW );
				removeNumBit( GetCellIndex( ColIndex(idx) , RowIndex(index) ) , removeBit );
				break;
			case ROW:
				assert( group == COL );
				removeNumBit( GetCellIndex( ColIndex(index) , RowIndex(idx) ) , removeBit );
				break;
			case BOX:
				{
					int box;
					Iterator iter1;
					int* iterator = sGroupNextIndex[ group ];

					box = BoxIndex( nIdx );
					for( iter1.setCellIndex( group , idx ); iter1; ++iter1 )
					{
						int idx1 = iter1.getCellIndex();
						if ( BoxIndex( idx1) == box  )
							removeNumBit( idx1 , removeBit );
					}

					box = BoxIndex( index );
					for( iter1.setGroupIndex( group , idxGroup ); iter1; ++iter1 )
					{
						int idx1 = iter1.getCellIndex();
						if ( BoxIndex( idx1) == box && idx1 != index )
							removeNumBit( idx1 , removeBit );
					}
				}
				break;
			}
			_this()->onPostEvalMethod(EMethod::YWing , group , index  , mPosibleBitsCell[ index ] | removeBit );
			result = true;
		}
	}
	return result;
}

template < class T , int BS>
bool SudokuSolverT<T,BS>::evalSimpleColourMethod( int index , Group group  , int idxGroup  )
{
	(void) group;
	(void) idxGroup;
	bool result = false;

	unsigned pBit = mPosibleBitsCell[ index ];
	while( pBit )
	{
		unsigned numBits = FBitUtility::ExtractTrailingBit(pBit);
		pBit -= numBits;
		result |= evalSimpleColourInternal( index , numBits );
	}

	return result;
}

template < class T , int BS>
bool SudokuSolverT<T,BS>::evalSimpleColourInternal( int index , unsigned numBits )
{
	enum
	{
		RED        =  1,
		BLUE       = -1,
		BOTH_COLOR =  2,
	};


	if ( index == 19 && numBits & BIT( 2 - 1 ) )
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
	int col = ColIndex( index );
	int row = RowIndex( index );
	int box = BoxIndex( col , row );

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
			if ( generateGroupNumBitIndex(  Group(g) , testIndex , numBits , pIndex , 1 ) != 1 )
				continue;

			int pairIndex = pIndex[0];

			if ( color[ pairIndex ] )
			{
				if ( color[testIndex] == color[ pairIndex ] )
				{
					if ( _this()->onPrevEvalMethod(EMethod::SimpleColour , NO_GROUP , index , numBits ) )
					{
						removeNumBit( pairIndex , numBits );
						_this()->onPostEvalMethod(EMethod::SimpleColour , NO_GROUP , index , numBits );
						result |= true;
					}
				}
			}
			else
			{
				color[ pairIndex ] = -color[ testIndex ];

				int gAt[3];
				gAt[COL] = ColIndex( pairIndex );
				gAt[ROW] = RowIndex( pairIndex );
				gAt[BOX] = BoxIndex( gAt[COL] , gAt[ROW] );

				for( int g = 0 ; g < 3 ; ++g )
				{
					int idxGroup = gAt[g];
					if ( !groupColor[ g ][ idxGroup ] )
					{
						groupColor[ g ][ idxGroup ] = color[ pairIndex ];
					}
					else if (  groupColor[ g ][ idxGroup ] == BOTH_COLOR )
					{
						removeColor = color[ pairIndex ];
					}
					else
					{
						if ( groupColor[ g ][ idxGroup ] == color[ pairIndex ] )
						{
							removeColor = color[ pairIndex ];
						}
						else
						{
							groupColor[ g ][ idxGroup ] = BOTH_COLOR;
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
		if ( _this()->onPrevEvalMethod(EMethod::SimpleColour , NO_GROUP , index , numBits ) )
		{
			for( int i = 0 ; i < numUsed ; ++i )
			{
				int idx = usedIndex[ i ];
				if ( color[idx] == removeColor )
					removeNumBit( idx , numBits );
			}
			_this()->onPostEvalMethod(EMethod::SimpleColour , NO_GROUP , index , numBits );
			result |= true;
		}
	}

	return result;
}


template < class T , int BS>
void SudokuSolverT<T,BS>::doSolveR( int idx )
{
	if ( idx == MaxIndex )
	{
		if ( numSolution == 16 )
			return;

		for( int i = 0 ; i < MaxIndex ; ++i )
			solution[ numSolution ][i] = mCurSolution[i];

		++numSolution;
		return;
	}

	if ( problem[ idx ] )
	{
		mCurSolution[idx] = 1 << ( problem[idx] - 1 );

		doSolveR( idx + 1 );
		return;
	}

	int col = ColIndex( idx );
	int row = RowIndex( idx );
	int box = BoxIndex( col , row );

	unsigned posible = mPosibleBitsGroup[COL][col] & mPosibleBitsGroup[ROW][row] & mPosibleBitsGroup[BOX][box];

	while ( posible )
	{
		int pn = posible & -posible;

		mCurSolution[idx] = pn;

		posible &= ~pn;

		mPosibleBitsGroup[COL][col] &= ~pn;
		mPosibleBitsGroup[ROW][row] &= ~pn;
		mPosibleBitsGroup[BOX][box] &= ~pn;

		doSolveR( idx + 1 );

		mPosibleBitsGroup[COL][col] |= pn;
		mPosibleBitsGroup[ROW][row] |= pn;
		mPosibleBitsGroup[BOX][box] |= pn;
	}
}

template < class T , int BS >
int SudokuSolverT<T,BS>::GetIteratorBeginFromGroupIndex( Group group , int idxGroup )
{
	switch( group )
	{
	case COL:  return idxGroup;
	case ROW:  return idxGroup * NumberNum;
	case BOX:  return BoxSize * ( idxGroup % BoxSize ) + (NumberNum * BoxSize) * ( idxGroup / BoxSize );
	default:
		return -1;
	}
}

template < class T , int BS >
int SudokuSolverT<T,BS>::GetIteratorBeginFromCellIndex( Group group , int idxCell )
{
	switch( group )
	{
	case COL: return ColIndex( idxCell );
	case ROW: return RowIndex( idxCell ) * NumberNum;
	case BOX: 
		{
			int idxGroup = BoxIndex( idxCell );
			return BoxSize * ( idxGroup % BoxSize ) + (NumberNum * BoxSize) * ( idxGroup / BoxSize );
		}
	}
	assert( 0 );
	return -1;
}


template < class T , int BS>
void SudokuSolverT<T,BS>::InitIterator()
{
	static bool isContructed = false;
	if ( isContructed )
		return;

	isContructed = true;

	for( int i = 0 ; i < MaxIndex ; ++i )
	{
		sGroupNextIndex[ COL ][ i ] = i + NumberNum;
		sGroupNextIndex[ ROW ][ i ] = i + 1;
		sGroupNextIndex[ BOX ][ i ] = i + 1;
	}

	for ( int i = 0 ; i < NumberNum ; ++ i )
	{
		sGroupNextIndex[ ROW ][ ( i + 1 ) * NumberNum - 1] = -1;
		sGroupNextIndex[ COL ][ ( NumberNum - 1 ) * NumberNum + i] = -1;

		for ( int n = 0 ; n < BoxSize ; ++ n )
		{
			int idx = NumberNum * i + ( n + 1 ) * BoxSize - 1;
			if ( i % BoxSize == BoxSize - 1 )
				sGroupNextIndex[BOX][idx] = -1;
			else
				sGroupNextIndex[BOX][idx] = idx + NumberNum - ( BoxSize - 1 );
		}
	}

#ifdef _DEBUG
	PrintIterator( sGroupNextIndex[ROW] );
	PrintIterator( sGroupNextIndex[COL] );
	PrintIterator( sGroupNextIndex[BOX] );

	for ( int group = 0 ; group < 3 ; ++group )
	{
		for( int idxGroup = 0; idxGroup < NumberNum; ++idxGroup )
		{
			int count = 0;
			int idx = GetIteratorBeginFromGroupIndex(Group(group), idxGroup);
			for( ; idx != -1; idx = sGroupNextIndex[group][idx] )
				++count;

			assert(count == 9);
		}
	}
#endif //_DEBUG
}

template < class T, int BS >
int SudokuSolverT<T, BS>::OffsetCellIndex(int index, Group group, int num)
{
	switch( group )
	{
	case ROW: return index + num;
	case COL: return index + num * NumberNum;
	case BOX:
		{
			int* iterator = sGroupNextIndex[group];
			for( int i = 0; i < num; ++i )
			{
				index = iterator[index];
				if( index == -1 )
					return -1;
			}
			return index;
		}
	}
	return -1;
}


template < class T , int BS>
int  SudokuSolverT<T,BS>::generateGroupNumBitIndex( Group group , int index , unsigned numBits , int pIndex[] , int maxNum )
{
	int count = 0;
	assert( mPosibleBitsCell[index] & numBits );
	Iterator iter;
	for( iter.setCellIndex( group , index ); iter; ++iter )
	{
		int idx = iter.getCellIndex();

		if ( idx == index )
			continue;

		if ( mPosibleBitsCell[idx] & numBits )
		{
			if ( count > maxNum )
				return ++count;

			pIndex[ count++ ] = idx;
		}
	}
	return count;
}

template < class T , int BS >
bool SudokuSolverT<T,BS>::setupProbInternal( int const* prob )
{
	numSolution = 0;
	for( int n = 0; n < NumberNum ; ++n )
	{
		mPosibleBitsGroup[COL][n] = NumberBitFill;
		mPosibleBitsGroup[ROW][n] = NumberBitFill;
		mPosibleBitsGroup[BOX][n] = NumberBitFill;
	}

	std::copy(prob, prob + MaxIndex, problem);

	for ( int n = 0 ; n < MaxIndex ; ++n )
	{
		indexCheck[ n ] = true;

		if ( problem[n] == 0 )
			continue;

		unsigned bit = ~BIT( problem[n] - 1 );

		int col = ColIndex( n );
		int row = RowIndex( n );
		int box = BoxIndex( col , row );

		//if ( ( groupPosible[COL][col] & bit ) == 0 ||
		//	 ( groupPosible[ROW][row] & bit ) == 0 ||
		//	 ( groupPosible[BOX][box] & bit ) == 0 )
		//{
		//	return false;
		//}

		mPosibleBitsGroup[COL][ col ] &= bit;
		mPosibleBitsGroup[ROW][ row ] &= bit;
		mPosibleBitsGroup[BOX][ box ] &= bit;

		mCurSolution[n] = 0;
		indexCheck[ n ] = false;
	}


	return true;
}

