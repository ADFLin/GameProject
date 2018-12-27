#include "TinyGamePCH.h"
#include "Big2Utility.h"

#include <algorithm>

template <typename Iter>
inline bool NextCombination(const Iter first, Iter k, const Iter last)
{
	if ((first == last) || (first == k) || (last == k))
		return false;
	Iter itr1 = first;
	Iter itr2 = last;
	++itr1;
	if (last == itr1)
		return false;
	itr1 = last;
	--itr1;
	itr1 = k;
	--itr2;
	while (first != itr1)
	{
		if (*--itr1 < *itr2)
		{
			Iter j = k;
			while (!(*itr1 < *j)) ++j;
			std::iter_swap(itr1,j);
			++itr1;
			++j;
			itr2 = k;
			std::rotate(itr1,j,last);
			while (last != j)
			{
				++j;
				++itr2;
			}
			std::rotate(k,itr2,last);
			return true;
		}
	}
	std::rotate(first,k,last);
	return false;
}

void test()
{
	int n[] = { 0 , 1 , 2 , 3 , 4 };
	//NextCombination( n , n + 3 , n )
}

namespace Poker { namespace Big2 {

	bool const SupportTwoPair     = false;
	bool const SupportThreeOfKind = false;
	bool const SupportFlush       = false;


	void TrickHelper::init()
	{
		mParseCards = NULL;

		int* curPtr = mIndexStorge;
#define  SET_POS( POS , NUM )\
	mDataGroups[ POS ].index = curPtr; curPtr += NUM;

		SET_POS( POS_SINGLE        ,13 )
		SET_POS( POS_PAIR          , 6 )
		SET_POS( POS_THREE_OF_KIND , 4 )
		SET_POS( POS_FOUR_OF_KIND  , 3 )
		SET_POS( POS_STRAIGHT      ,10 )
		SET_POS( POS_FLUSH         , 4 )
		SET_POS( POS_STRAIGHT_FLUSH, 2 )

#undef  SET_POS
	}

	int const OnePairPosMap[] = { POS_PAIR , POS_THREE_OF_KIND , POS_FOUR_OF_KIND };
	int const ThreeOfKindPosMap[] = { POS_THREE_OF_KIND , POS_FOUR_OF_KIND };

	void TrickHelper::parse( Card sortedCards[] , int numCard )
	{
		mParseCards = sortedCards;

		for( int i = 0 ; i < 4 ; ++i )
			mSuitGroup[i].num = 0;

		for( int i = 0 ; i < NumGroupPos ; ++i )
			mDataGroups[i].num = 0;

		if ( numCard )
		{
			mFaceGroups[ 0 ].rank  = sortedCards[0].getFaceRank();
			mFaceGroups[ 0 ].num   = 1; 
			mFaceGroups[ 0 ].index = 0;
			mFaceGroups[ 0 ].suitMask = BIT( sortedCards[0].getSuit() );

			int idxGroup = 0;
			for ( int i = 1 ; i < numCard ; ++i )
			{
				Card const& card = sortedCards[i];
				int rank = card.getFaceRank();

				addSuitGroup( card , i );

				if ( mFaceGroups[ idxGroup ].rank != rank )
				{
					++idxGroup;
					mFaceGroups[ idxGroup ].index = i;
					mFaceGroups[ idxGroup ].rank = rank;
					mFaceGroups[ idxGroup ].num  = 1;
					mFaceGroups[ idxGroup ].suitMask = BIT( card.getSuit() );
				}
				else
				{
					mFaceGroups[ idxGroup ].num += 1;
					mFaceGroups[ idxGroup ].suitMask |= BIT( card.getSuit() );
				}
			}

			mNumFaceGroup = idxGroup + 1;
		}
		else
		{
			mNumFaceGroup = 0;
		}

		for( int i = 0 ; i < mNumFaceGroup ; ++i )
		{
			switch ( mFaceGroups[i].num )
			{
			case 2: addDataGroup( POS_PAIR          , i ); break;
			case 3: addDataGroup( POS_THREE_OF_KIND , i ); break;
			case 4: addDataGroup( POS_FOUR_OF_KIND  , i ); break;
			}
		}


		if ( mNumFaceGroup >= 5 )
		{
			int idx  = mFaceGroups[0].index;
			int rank = mFaceGroups[0].rank + 1;
			int numSquence = 1;
			for( int i = 0 ; i < mNumFaceGroup ; ++i )
			{
				if ( mFaceGroups[i].rank == rank )
				{
					rank += 1;
					numSquence += 1;
					if ( numSquence >= 5 )
					{
						int idxStr = i - 4;
						addDataGroup( POS_STRAIGHT , idxStr );
						unsigned mask = mFaceGroups[ idxStr ].suitMask;
						for( int i = 1 ; i < 5 ; ++i )
						{
							mask &= mFaceGroups[ idxStr + i ].suitMask;
							if ( !mask )
								break;
						}
						if ( mask )
						{
							for( int i = 0 ; i < 4 ; ++i )
							{
								if ( BIT(i) & mask )
								{
									addDataGroup( POS_STRAIGHT_FLUSH , ( idxStr << 2 ) + i );
								}
							}
						}
					}
				}
				else
				{
					rank = mFaceGroups[i].rank + 1;
					numSquence = 1;
				}
			}

			if ( numSquence >= 4 && rank == 13 )
			{
				if ( mFaceGroups[0].rank == 0 )
				{
					addDataGroup( POS_STRAIGHT , mNumFaceGroup - 4 );

					int idxStr = mNumFaceGroup - 4;
					unsigned mask = mFaceGroups[ 0 ].suitMask;
					for( int i = 0 ; i < 4 ; ++i )
					{
						mask &= mFaceGroups[ idxStr + i ].suitMask;
						if ( !mask )
							break;
					}
					if ( mask )
					{
						for( int i = 0 ; i < 4 ; ++i )
						{
							if ( BIT(i) & mask )
							{
								addDataGroup( POS_STRAIGHT_FLUSH , ( idxStr << 2 ) + i );
							}
						}
					}
				}
			}
		}

		for( int i = 0 ; i < 4 ; ++i )
		{
			if ( mSuitGroup[i].num >= 5 )
			{
				addDataGroup( POS_FLUSH , i );
			}
		}
	}

	bool TrickHelper::initIterator( CardGroup group , IterData& iterData )
	{
		iterData.condition = -1;

		switch( group )
		{
		case CG_SINGLE:
			if ( initIteratorSingle( iterData.face[0] ) )
			{
				iterData.condition = 0;
				return true;
			}
			break;
		case CG_ONE_PAIR:
			if ( initIteratorOnePair( iterData.face[0] ) )
			{
				iterData.condition = 0;
				return true;
			}
			break;
		case CG_THREE_OF_KIND:
			if ( initIteratorThreeOfKind( iterData.face[0] ) )
			{
				iterData.condition = 0;
				return true;
			}
			break;
		case CG_TWO_PAIR:
			break;
		case CG_STRAIGHT:
			{
				IterStraight& straight = iterData.straight;
				if ( getData( POS_STRAIGHT ).num )
				{
					iterData.condition = 0;
					for( int i = 0 ; i < 5 ; ++i )
						straight.index[i] = 0;
					straight.cur = 4;
					return true;
				}
			}
			break;
		case CG_FLUSH:
			{
				IterFlush& data = iterData.flush;
				if ( getData( POS_FLUSH ).num )
				{
					iterData.condition = 0;
					data.count = 0;
					data.cur = 0;
					int idxGroup = getData( POS_FLUSH ).index[0];
					data.initCombine( mSuitGroup[ idxGroup ].num );
					return true;
				}
			}
			break;
		case CG_FULL_HOUSE:
			if ( initIteratorThreeOfKind( iterData.face[0] ) &&
				 initIteratorOnePair( iterData.face[1] ) )
			{
				iterData.condition = 0;
				if ( iterData.face[1].cur == iterData.face[0].cur + 1 )
				{
					iterData.face[2] = iterData.face[1];
					do 
					{
						if ( !nextIteratorOnePair( iterData.face[1] ) )
							return false;
					} while ( iterData.face[1].cur  == iterData.face[0].cur + 1 );
					iterData.condition = 1;
				}
				return true;
			}
			break;
		case CG_FOUR_OF_KIND:
			{
				if ( !getData( POS_FOUR_OF_KIND ).num )
					return false;

				iterData.face[0].cur   = 0;
				iterData.face[0].count = 0;

				if ( mNumFaceGroup < 2 )
					return false;

				iterData.face[1].cur   = 0;
				iterData.face[1].count = 0;
				if ( iterData.face[1].cur == getData( POS_FOUR_OF_KIND ).index[ 0 ] )
					iterData.face[1].cur = 1;

				iterData.condition = 0;
				return true;
			}
			break;
		case CG_STRAIGHT_FLUSH:
			{
				if ( !getData( POS_STRAIGHT_FLUSH ).num )
					return false;

				iterData.face[0].cur   = 0;
				iterData.face[0].count = 0;

				iterData.condition = 0;
				return true;
			}
		default:
			break;
		}
		return false;
	}

	bool TrickHelper::nextIterator( CardGroup group , IterData& iterData )
	{
		if ( iterData.condition == -1 )
			return false;

		switch( group )
		{
		case CG_SINGLE:        return nextIteratorSingle( iterData.face[0] );
		case CG_ONE_PAIR:      return nextIteratorOnePair( iterData.face[0] );
		case CG_THREE_OF_KIND: return nextIteratorThreeOfKind( iterData.face[0] );

		case CG_STRAIGHT:
			{
				IterStraight& straight = iterData.straight;

				bool needRest = false;

				while ( 1 )
				{
					straight.index[ straight.cur ] += 1;
					int idxGroup = getData( POS_STRAIGHT ).index[ iterData.condition ];
					if ( straight.cur == 4 && mFaceGroups[ idxGroup ].rank == 9 )
					{
						idxGroup = 0;
					}
					else
					{
						idxGroup += straight.cur;
					}

					if (  straight.index[ straight.cur ] < mFaceGroups[ idxGroup ].num )
					{
						if ( needRest )
							straight.cur = 4;
						break;
					}

					straight.cur -= 1;
					if ( straight.cur == -1 )
					{
						iterData.condition += 1;
						if(  iterData.condition < getData( POS_STRAIGHT ).num )
						{
							for( int i = 0 ; i < 4 ; ++i )
								straight.index[i] = 0;
							straight.cur = 4;
							return true;
						}
						else
						{
							iterData.condition = -1;
							return false;
						}
					}
					else
					{
						for ( int i = 4; i > straight.cur ; --i )
						{
							straight.index[i] = 0;
						}
						needRest = true;
					}
				}
				return true;
			}
			break;
		case CG_FLUSH:
			{
				IterFlush& data = iterData.flush;
				if ( data.count < mDataGroups[ CG_FLUSH ].num )
				{
					int idxGroup = mDataGroups[POS_FLUSH].index[ data.count ];
					if ( data.nextCombine( mSuitGroup[ idxGroup ].num , 5 ) )
						return true;
					data.initCombine( mSuitGroup[ idxGroup ].num );
					data.count += 1;
				}
				if ( data.count >= mDataGroups[ CG_FLUSH ].num )
				{
					data.cur = -1;
					return false;
				}
				return true;
			}
			break;

		case CG_FULL_HOUSE:
			{
				if ( iterData.condition == 1 )
				{
					iterData.face[1] = iterData.face[2];
					iterData.condition = 0;
				}

				if( !nextIteratorThreeOfKind( iterData.face[0] ) )
				{
					if ( !nextIteratorOnePair( iterData.face[1] ) )
					{
						iterData.condition = -1;
						return false;
					}
					else
					{
						initIteratorThreeOfKind( iterData.face[0] );
					}
				}

				if ( iterData.face[1].cur == iterData.face[0].cur + 1 &&
					iterData.face[0].count == iterData.face[1].count )
				{
					iterData.face[2] = iterData.face[1];
					do
					{
						if ( !nextIteratorOnePair( iterData.face[1] ) )
						{
							iterData.condition = -1;
							return false;
						}
					}
					while( iterData.face[1].cur == iterData.face[0].cur + 1 &&
						  iterData.face[0].count == iterData.face[1].count );

					if ( getData( POS_THREE_OF_KIND ).num > 1 )
						iterData.condition = 1;
				}
				return true;
			}
			break;
		case CG_FOUR_OF_KIND:
			{
				if ( iterData.condition == 1 )
				{
					iterData.face[1] = iterData.face[2];
					iterData.condition = 0;
				}

				iterData.face[0].count += 1;
				if ( iterData.face[0].count >= getData( POS_FOUR_OF_KIND ).num )
				{
					if ( !nextIteratorSingle( iterData.face[1] ) )
					{
						iterData.condition = -1;
						return false;
					}
					else
					{
						iterData.face[0].cur   = 0;
						iterData.face[0].count = 0;
					}
				}

				if ( getData( POS_FOUR_OF_KIND ).index[ iterData.face[0].count ] == iterData.face[1].cur )
				{
					iterData.face[2] = iterData.face[1];
					do
					{
						if ( !nextIteratorSingle( iterData.face[1] ) )
						{
							iterData.condition = -1;
							return false;
						}
					}
					while( getData( POS_FOUR_OF_KIND ).index[ iterData.face[0].count ] ==  iterData.face[1].cur );

					if ( getData( POS_FOUR_OF_KIND ).num > 1 )
						iterData.condition = 1;
				}
				return true;
			}
			break;
		case CG_STRAIGHT_FLUSH:
			{
				IterFace& data = iterData.face[0];
				data.count += 1;
				if ( data.count < mDataGroups[ CG_STRAIGHT_FLUSH ].num )
					return true;

				iterData.condition = -1;
				return false;
			}
			break;
		}
		return false;
	}

	void TrickHelper::valueIterator( CardGroup group , IterData& iterData , int outIndex[] , int* power )
	{
		switch( group )
		{
		case CG_SINGLE:
			valueIteratorSingle( iterData.face[0] , outIndex , power );
			break;
		case CG_ONE_PAIR:
			valueIteratorOnePair( iterData.face[0] , outIndex , power );
			break;
		case CG_THREE_OF_KIND:
			valueIteratorThreeOfKind( iterData.face[0] , outIndex , power );
			break;
		case CG_TWO_PAIR:
			break;
		case CG_STRAIGHT:
			{
				IterStraight& straight = iterData.straight;
				int idxGroup = getData( POS_STRAIGHT ).index[ iterData.condition ];

				for( int i = 0 ; i < 4 ; ++i )
					outIndex[i] = mFaceGroups[ idxGroup + i ].index + straight.index[i];

				if ( mFaceGroups[ idxGroup ].rank == Card::ToRank( Card::eN10 ) )
					outIndex[4] = mFaceGroups[ 0 ].index + straight.index[4];
				else
					outIndex[4] = mFaceGroups[ idxGroup + 4 ].index + straight.index[4];

				if ( power )
				{
					Card cards[5];
					for( int i = 0 ; i < 5 ; ++ i )
						cards[i] = mParseCards[ outIndex[i] ];
					*power = FTrick::CalcPower( CG_STRAIGHT , cards );
				}
			}
			break;
		case CG_FLUSH:
			{
				IterFlush& data = iterData.flush;
				int idxGroup = getData( POS_FLUSH ).index[ data.count ];
				for( int i = 0 ; i < 5 ; ++i )
					outIndex[i] = mSuitGroup[ idxGroup ].index[ data.combine[i] ];

				if ( power )
				{
					Card cards[5];
					for( int i = 0 ; i < 5 ; ++ i )
						cards[i] = mParseCards[ outIndex[i] ];
					*power = FTrick::CalcPower( CG_FLUSH , cards );
				}
			}	
			break;

		case CG_FULL_HOUSE:
			{
				valueIteratorOnePair( iterData.face[1] , outIndex , NULL );
				valueIteratorThreeOfKind( iterData.face[0] , outIndex + 2 , NULL );
				
				if ( power )
				{
					Card cards[5];
					for( int i = 0 ; i < 5 ; ++ i )
						cards[i] = mParseCards[ outIndex[i] ];
					*power = FTrick::CalcPower( CG_FULL_HOUSE , cards );
				}
			}
			break;
		case CG_FOUR_OF_KIND:
			{
				int idxGroup = getData( POS_FOUR_OF_KIND ).index[ iterData.face[0].count ];

				valueIteratorSingle( iterData.face[1] , outIndex , NULL );
				for( int i = 0 ; i < 4 ; ++i )
					outIndex[i + 1] = mFaceGroups[ idxGroup ].index + i;

				if ( power )
				{
					Card cards[5];
					for( int i = 0 ; i < 5 ; ++ i )
						cards[i] = mParseCards[ outIndex[i] ];
					*power = FTrick::CalcPower( CG_FOUR_OF_KIND , cards );
				}	
			}
			break;
		case CG_STRAIGHT_FLUSH:
			{
				int valueGroup = getData( POS_STRAIGHT_FLUSH ).index[ iterData.face[0].count ];
				int idxGroup = valueGroup >> 2;
				int suit     = valueGroup & 0x3;

				for( int i = 0 ; i < 4 ; ++i )
				{
					int idx = idxGroup + i;
					int count = 0;
					for( int n = 0 ; n < suit ; ++n )
					{
						if ( BIT( n ) & mFaceGroups[ idx ].suitMask )
							++count;
					}
					outIndex[i] = mFaceGroups[ idx ].index + count;
				}
				{
					int idx = ( mFaceGroups[ idxGroup ].rank == Card::ToRank( Card::eN10 ) ) ? 0 : idxGroup + 4;
					int count = 0;
					for( int n = 0 ; n < suit ; ++n )
					{
						if ( BIT( n ) & mFaceGroups[ idx ].suitMask )
							++count;
					}
					outIndex[4] = mFaceGroups[ idx ].index + count;
				}

				if ( power )
				{
					Card cards[5];
					for( int i = 0 ; i < 5 ; ++ i )
						cards[i] = mParseCards[ outIndex[i] ];
					*power = FTrick::CalcPower( CG_STRAIGHT_FLUSH , cards );
				}	
			}
			break;
		}
	}

	bool TrickHelper::initIteratorSingle( IterFace& data  )
	{
		if ( mNumFaceGroup == 0 )
			return false;
		data.cur   = 0;
		data.count = 0;
		return true;
	}

	bool TrickHelper::nextIteratorSingle( IterFace& data )
	{
		data.count += 1;
		if ( data.count < mFaceGroups[ data.cur ].num )
			return true;
		data.cur += 1;
		if ( data.cur < mNumFaceGroup )
		{
			data.count = 0;
			return true;
		}
		return false;
	}

	void TrickHelper::valueIteratorSingle( IterFace& data , int outIndex[] , int* power )
	{
		outIndex[0] = mFaceGroups[ data.cur ].index + data.count;
		if ( power )
		{
			*power = FTrick::CalcPower( CG_SINGLE , mParseCards + outIndex[0] );
		}
	}

	bool TrickHelper::initIteratorOnePair( IterFace& data  )
	{
		data.cur = -1;
		for( int i = 0 ;  i < ARRAY_SIZE( OnePairPosMap ) ; ++i )
		{
			if (  mDataGroups[ OnePairPosMap[ i ] ].num  )
			{
				data.cur = i;
				break;
			}
		}
		if ( data.cur == -1 )
			return false;
	
		data.count = 0;
		switch( data.cur )
		{
		case 1: data.initCombine( 3 ); break;
		case 2: data.initCombine( 4 ); break;
		}
		return true;
	}

	bool TrickHelper::nextIteratorOnePair( IterFace& data )
	{
		if ( data.cur == -1 )
			return false;
		if ( data.count < mDataGroups[ OnePairPosMap[ data.cur ]  ].num )
		{
			switch( data.cur )
			{
			case 0:
				data.count += 1;
				break;
			case 1:
				if ( data.nextCombine( 3 , 2 ) )
					return true;
				data.initCombine( 3 );
				data.count += 1;
				break;
			case 2:
				if ( data.nextCombine( 4 , 2 ) )
					return true;
				data.initCombine( 4 );
				data.count += 1;
				break;
			}
		}

		if ( data.count >= mDataGroups[ OnePairPosMap[ data.cur ]  ].num )
		{
			while( data.cur < ARRAY_SIZE( OnePairPosMap ) )
			{
				++data.cur;
				if ( mDataGroups[ OnePairPosMap[data.cur] ].num )
					break;
			}
			if ( data.cur < ARRAY_SIZE( OnePairPosMap ) )
			{
				data.count = 0;
				switch( data.cur )
				{
				case 1: data.initCombine( 3 ); break;
				case 2: data.initCombine( 4 ); break;
				}
				return true;
			}
			else
			{
				data.cur = -1;
				return false;
			}
		}

		return true;
	}


	void TrickHelper::valueIteratorOnePair( IterFace& data , int outIndex[] , int* power )
	{
		int idxGroup = mDataGroups[ OnePairPosMap[ data.cur ] ].index[ data.count ];
		int idx = mFaceGroups[ idxGroup  ].index;
		switch( data.cur )
		{
		case 0:
			outIndex[0] = idx;
			outIndex[1] = idx + 1;
			break;
		case 1:
		case 2:
			outIndex[0] = idx + data.combine[0];
			outIndex[1] = idx + data.combine[1];
			break;
		default:
			return;
		}

		if ( power )
		{
			Card cards[2] = 
			{ 
				mParseCards[ outIndex[0] ] , 
				mParseCards[ outIndex[1] ] ,
			};
			*power = FTrick::CalcPower( CG_THREE_OF_KIND , cards );
		}
	}

	bool TrickHelper::initIteratorThreeOfKind( IterFace &data )
	{
		data.cur = -1;
		for( int i = 0 ;  i < ARRAY_SIZE( ThreeOfKindPosMap ) ; ++i )
		{
			if (  mDataGroups[ ThreeOfKindPosMap[ i ] ].num  )
			{
				data.cur = i;
				break;
			}
		}

		if ( data.cur == -1 )
			return false;
		data.count = 0;
		switch( data.cur )
		{
		case 1: data.initCombine( 4 ); break;
		}
		return true;
	}

	bool TrickHelper::nextIteratorThreeOfKind( IterFace& data )
	{
		if ( data.cur == -1 )
			return false;

		if ( data.count < mDataGroups[ ThreeOfKindPosMap[ data.cur ]  ].num )
		{
			switch( data.cur )
			{
			case 0:
				data.count += 1;
				break;
			case 1:
				if ( data.nextCombine( 4 , 3 ) )
					return true;
				data.initCombine( 4 );
				data.count += 1;
				break;
			}
		}

		if ( data.count >= mDataGroups[ ThreeOfKindPosMap[ data.cur ]  ].num )
		{
			while( data.cur < ARRAY_SIZE( ThreeOfKindPosMap ) )
			{
				++data.cur;
				if ( mDataGroups[ ThreeOfKindPosMap[data.cur] ].num )
					break;
			}
			if ( data.cur < ARRAY_SIZE( ThreeOfKindPosMap ) )
			{
				data.count = 0;
				switch( data.cur )
				{
				case 1: data.initCombine( 4 ); break;
				}
				return true;
			}
			else
			{
				data.cur = -1;
				return false;
			}
		}
		return true;
	}

	void TrickHelper::valueIteratorThreeOfKind( IterFace& data, int outIndex[] , int* power )
	{
		int idxGroup = mDataGroups[ ThreeOfKindPosMap[ data.cur ] ].index[ data.count ];
		int idx = mFaceGroups[ idxGroup ].index;
		
		switch( data.cur )
		{
		case 0:
			outIndex[0] = idx;
			outIndex[1] = idx + 1;
			outIndex[2] = idx + 2;
			break;
		case 1:
			outIndex[0] = idx + data.combine[0];
			outIndex[1] = idx + data.combine[1];
			outIndex[2] = idx + data.combine[2];
			break;
		default:
			return;
		}
		if ( power )
		{
			Card cards[3] = 
			{ 
				mParseCards[ outIndex[0] ] , 
				mParseCards[ outIndex[1] ] ,
				mParseCards[ outIndex[2] ] 
			};
			*power = FTrick::CalcPower( CG_THREE_OF_KIND , cards );
		}
	}

	TrickIterator TrickHelper::getIterator( CardGroup group )
	{
		return TrickIterator( *this , group );
	}

	void TrickHelper::addSuitGroup( Card const& card , int index )
	{
		SuitGroup& group = mSuitGroup[ card.getSuit() ];
		group.index[ group.num++ ] = index;
	}

	void TrickHelper::addDataGroup( GroupPos pos , int index )
	{
		DataGroup& group = mDataGroups[ pos ];
		group.index[ group.num++ ] = index;
	}

	bool TrickHelper::haveGroup( CardGroup group )
	{
		IterData iterData;
		return initIterator( group , iterData );
	}

	int TrickHelper::getFaceNum( int faceRank )
	{
		FaceGroup findGroup;
		findGroup.rank = faceRank;
		struct Cmp
		{
			bool operator()( FaceGroup const& g1 , FaceGroup const& g2 )
			{
				return g1.rank < g2.rank;
			}
		};
		FaceGroup* end = mFaceGroups + mNumFaceGroup;
		FaceGroup* find = std::lower_bound( mFaceGroups , end , findGroup , Cmp() );
		if ( find == end )
			return 0;

		return find->num;
	}

	TrickIterator::TrickIterator( TrickHelper& helper , CardGroup group ) 
		:mHelper( &helper ),mGroup( group )
	{
		reset();
	}

	TrickIterator::TrickIterator()
	{
		mHelper = NULL;
		mGroup  = CG_NONE;
		mbOK = false;
	}

	void TrickIterator::goNext()
	{
		mbOK = mHelper->nextIterator( mGroup , mIterData );
		if ( mbOK )
			mHelper->valueIterator( mGroup , mIterData , mIndex , NULL );
	}

	void TrickIterator::goNext( int power )
	{
		while( 1 )
		{
			mbOK = mHelper->nextIterator( mGroup , mIterData );
			if ( !mbOK )
				break;

			int curPower;
			mHelper->valueIterator( mGroup , mIterData , mIndex , &curPower );
			if ( curPower > power )
				break;
		}
	}

	void TrickIterator::reset()
	{
		mbOK = mHelper->initIterator( mGroup , mIterData );
		if ( mbOK )
			mHelper->valueIterator( mGroup , mIterData , mIndex , NULL );
	}

	bool TrickIterator::reset( int power )
	{
		mbOK = mHelper->initIterator( mGroup , mIterData );
		if ( !mbOK )
			return false;

		while( 1 )
		{
			int curPower;
			mHelper->valueIterator( mGroup , mIterData , mIndex , &curPower );
			if ( curPower > power )
				break;
			mbOK = mHelper->nextIterator( mGroup , mIterData );
			if ( !mbOK )
				break;
		}

		return true;
	}

	int* TrickIterator::getIndex( int& num )
	{
		num = FTrick::GetGroupNum( mGroup ); 
		return mIndex;
	}

	bool TrickHelper::IterFace::nextCombine( int m , int n )
	{
		return NextCombination( combine , combine + n , combine + m );
	}

	void TrickHelper::IterFace::initCombine( int num )
	{
		for( int i = 0 ; i < num ; ++i )
			combine[i] = i;
	}

	bool TrickHelper::IterFlush::nextCombine( int m , int n )
	{
		return NextCombination( combine , combine + n , combine + m );
	}

	void TrickHelper::IterFlush::initCombine( int num )
	{
		for( int i = 0 ; i < num ; ++i )
			combine[i] = i;
	}

	bool FTrick::CanSuppress( TrickInfo const& info , TrickInfo const& prevInfo )
	{
		switch ( info.group )
		{
		case CG_STRAIGHT_FLUSH:
			if ( prevInfo.group != CG_STRAIGHT_FLUSH || info.power > prevInfo.power )
				return true;
		case CG_FOUR_OF_KIND:
			if ( prevInfo.group != CG_STRAIGHT_FLUSH )
			{
				if ( prevInfo.group != CG_FOUR_OF_KIND || info.power > prevInfo.power )
					return true;
			}
		}

		if ( info.group == prevInfo.group && info.power > prevInfo.power )
			return true;

		return false;
	}

	int FTrick::GetRankPower( int rank )
	{
		int const big2Power[13] = { 12 , 13 , 1 , 2 , 3, 4 , 5 , 6 , 7 , 8 , 9 , 10 , 11 };
		return big2Power[ rank ];
	}

	int FTrick::CalcPower( Card const& card )
	{
		return 4 * GetRankPower( card.getFaceRank() ) + card.getSuit();
	}

	int FTrick::CalcPower( CardGroup group , Card cards[] )
	{
		switch( group )
		{		
		case CG_SINGLE:
			return CalcPower( cards[0] );
		case CG_ONE_PAIR: 
			return CalcPower( cards[1] );
		case CG_TWO_PAIR:
			return CalcPower( cards[4] );
		case CG_THREE_OF_KIND:
			return CalcPower( cards[2] );
		case CG_STRAIGHT:
			if ( cards[0].getFace() == Card::eN2 )
				return CalcPower( cards[0] );
			else
				return CalcPower( cards[4] );
		case CG_FLUSH:
			return CalcPower( cards[4] );
		case CG_FULL_HOUSE:
			return CalcPower( cards[4] );
		case CG_FOUR_OF_KIND:
			return 100000 + CalcPower( cards[4] );
		case CG_STRAIGHT_FLUSH:
			if ( cards[0].getFace() == Card::eN2 )
				return 200000 + CalcPower( cards[0] );
			else
				return 200000 + CalcPower( cards[4] );
		}
		return 0;
	}

	bool FTrick::CheckCard5( TrickInfo& info )
	{
		int groupIndex[5];
		int groupNum[5];
		groupIndex[0] = 0;
		groupNum[0]   = 1;
		int idxGroup = 0;
		int rank = info.card[0].getFaceRank();
		for( int i = 1 ; i < 5 ; ++i )
		{
			if ( info.card[i].getFaceRank() == rank )
			{
				groupNum[ idxGroup ] += 1;
			}
			else
			{
				idxGroup += 1;
				groupIndex[ idxGroup ] = i;
				groupNum[ idxGroup ] = 1;
				rank = info.card[i].getFaceRank();
			}
		}

		int cond = -1;
		switch( idxGroup + 1 )
		{
		case 2:
			if ( groupNum[0] == 3 && groupNum[1] == 2 ) 
			{
				std::rotate( info.card , info.card + 3 , info.card + 5 );
				cond = 0; 
			}
			else if ( groupNum[0] == 2 && groupNum[1] == 3 )
			{
				cond = 1;
			}

			if ( cond != -1)
			{
				info.group = CG_FULL_HOUSE;
				return true;
			}
			if ( groupNum[0] == 4 && groupNum[1] == 1 )
			{
				std::rotate( info.card , info.card + 1 , info.card + 5 );
				cond = 0;
			}
			else if ( groupNum[0] == 1 && groupNum[1] == 4 )
			{
				cond = 1;
			}

			if ( cond != -1 )
			{
				info.group = CG_FOUR_OF_KIND;
				return true;
			}
			break;
		case 3:
			if ( SupportTwoPair )
			{
				if ( groupNum[0] == 2 && groupNum[1] == 2 )
				{
					std::rotate( info.card , info.card + 4 , info.card + 5 );
					cond = 0;
				}
				else if ( groupNum[1] == 2 && groupNum[2] == 2 )
				{
					cond = 1;
				}
				else if ( groupNum[2] == 2 && groupNum[0] == 2 ) 
				{
					std::rotate( info.card , info.card + 2 , info.card + 3 );
					cond = 2;
				}

				if ( cond != -1 )
				{
					info.group = CG_TWO_PAIR;
					return true;
				}
			}
			break;
		case 5:
			{
				bool bSquence = true;
				int rank = info.card[1].getFaceRank() + 1;
				for( int i = 2 ; i < 5 ; ++i )
				{
					if ( rank != info.card[i].getFaceRank() )
					{
						bSquence = false;
						break;
					}
					++rank;
				}
				if ( bSquence )
				{
					if (info.card[0].getFaceRank() + 1 != info.card[1].getFaceRank() )
					{
						if ( info.card[4].getFace() == Card::eKING && 
							 info.card[0].getFace() == Card::eACE )
						{
							std::rotate( info.card , info.card + 1 , info.card + 5 );
						}
						else
						{
							bSquence = false;
						}
					}
				}

				int suit = info.card[0].getSuit();
				bool bSameSuit = true;
				for( int i = 1 ; i < 5 ; ++i )
				{
					if ( suit != info.card[i].getSuit() )
					{
						bSameSuit = false;
						break;
					}
				}

				if ( bSameSuit )
				{
					if ( bSquence )
					{
						info.group = CG_STRAIGHT_FLUSH;
						return true;
					}
					else if ( SupportFlush )
					{
						info.group = CG_FLUSH;
						return true;
					}
				}
				else if ( bSquence )
				{
					info.group = CG_STRAIGHT;
					return true;
				}
			}
			break;
		}
		return false;
	}

	bool FTrick::CheckCard( Card const cards[] , int numCard , int index[] , int num , TrickInfo& info )
	{
		if ( num > numCard )
			return false;
		for( int i = 0 ; i < num ; ++i )
		{
			if ( index[i] >= numCard )
				return false;

			for( int j = i + 1 ; j < num ; ++j )
			{
				if ( index[i] == index[j] )
					return false;
			}
		}

		switch( num )
		{
		case 1:
			{
				Card const& c = cards[ index[0] ];
				info.group   = CG_SINGLE;
				info.card[0] = cards[ index[0] ];
				return true;
			}
			break;
		case 2:
			{
				Card const& c0 = cards[ index[0] ];
				Card const& c1 = cards[ index[1] ];
				if ( c0.getFaceRank() != c1.getFaceRank() )
					return false;
				info.group = CG_ONE_PAIR;
				for ( int i = 0 ; i < 2 ; ++i )
					info.card[i] = cards[ index[i] ];
				std::sort( info.card , info.card + 2 , CardSortCmp() );
				return true;
			}
			break;
		case 3:
			if ( SupportThreeOfKind )
			{
				Card const& c0 = cards[ index[0] ];
				Card const& c1 = cards[ index[1] ];
				Card const& c2 = cards[ index[2] ];

				if ( c0.getFaceRank() != c1.getFaceRank() ||
					c0.getFaceRank() != c2.getFaceRank() )
					return false;
				for ( int i = 0 ; i < 3 ; ++i )
					info.card[i] = cards[ index[i] ];
				std::sort( info.card , info.card + 3 , CardSortCmp() );
				return true;
			}
			break;
		case 5:
			{
				for( int i = 0 ; i < 5 ; ++i )
					info.card[i] = cards[ index[i] ];
				std::sort( info.card , info.card + 5 , CardSortCmp() );
				if( CheckCard5( info ) )
					return true;
			}
			break;
		}

		return false;
	}

	int FTrick::GetGroupNum( CardGroup group )
	{
		int const numMap[] = { 1 , 2 , 5 , 3 , 5 , 5 , 5 , 5 , 5 , 0 };
		return numMap[ group ];
	}

}//namespace Big2
}//namespace Poker