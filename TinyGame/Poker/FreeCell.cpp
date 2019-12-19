#include "FreeCell.h"


#include <cassert>

namespace Poker
{

	bool Cell::TestStackRule( Card const& top,Card const& bottom )
	{
		return  ( top.getFaceRank() == bottom.getFaceRank() + 1 ) && 
			!Card::isSameColorSuit( top , bottom );
	}

	bool Cell::TestGoalRule( Card const& top,Card const& bottom )
	{
		return  ( top.getFaceRank() + 1 == bottom.getFaceRank() ) && 
			top.getSuit() == bottom.getSuit();
	}


	GoalCell::GoalCell() :SingleCell(Cell::eGOAL)
	{

	}

	void GoalCell::pop()
	{
		if( mCard.getFace() == Card::eACE )
			mCard = Card::None();
		else
			mCard = Card(mCard.getSuit(), mCard.getFaceRank() - 1);
	}

	bool GoalCell::testRule(Card const& card)
	{
		if( isEmpty() )
			return card.getFace() == Card::eACE;
		return Cell::TestGoalRule(getCard(), card);
	}

	FreeCell::FreeCell() :SingleCell(Cell::eFREE)
	{

	}

	bool FreeCell::testRule(Card const& card)
	{
		return isEmpty();
	}

	void StackCell::moveCard(StackCell& to, int num)
	{
		assert( num <= getCardNum() );
		auto from = mCards.end() - num;
		to.mCards.insert(to.mCards.end(), from, mCards.end());
		mCards.erase(from, mCards.end());
	}

	bool StackCell::testRule(Card const& card)
	{

		if( isEmpty() )
			return true;
		return Cell::TestStackRule(getCard(), card);
	}

	FreeCellLevel::FreeCellLevel()
	{
		for ( int i = 0 ; i < SCellNum ; ++i )
		{
			mCells[ SCellIndex + i] = &mSCells[i];
			mSCells[i].setIndex( SCellIndex + i );
		}
		for ( int i = 0; i < FCellNum ; ++i )
		{
			mCells[ FCellIndex + i ] = &mFCells[ i ];
			mFCells[i].setIndex( FCellIndex + i );
		}
		for ( int i = 0; i < 4 ; ++i )
		{
			mCells[ GCellIndex + i ] = &mGCells[ i ];
			mGCells[i].setIndex( GCellIndex + i );
		}
	}

	FreeCellLevel::~FreeCellLevel()
	{

	}

	void FreeCellLevel::setupGame( int seed )
	{
		for (auto* cell : mCells)
			cell->clear();

		int card[52];
		if ( seed == 0 )
		{
			for( int i = 0 ; i < 52 ; ++i )
			{
				card[i] = 51 - i;
			}
		}
		else
		{
			class MicrosoftRandom
			{
			public:
				MicrosoftRandom( long seed ):mSeed( seed ){}
				int next()
				{
					mSeed = mSeed * 214013 + 2531011;
					return ( mSeed >> 16) & 0x7fff; 
				}
			private:
				long mSeed;
			};

			MicrosoftRandom random( seed );
			int const NumCardOfDeck = 52;
			int pos = 0;
			
			int tempDeck[ NumCardOfDeck ];

			for( int i = 0 ; i < NumCardOfDeck ; ++i )
				card[i] = i;

			//put the cards in "microsoft representation"
			for(int i=0;i<13;i++)
			{
				for(int j=0;j<4;j++)
				{
					tempDeck[pos] = card[i+j*13];
					pos++;
				}
			}

			int wLeft = NumCardOfDeck;
			for (int i = 0; i < NumCardOfDeck; i++)
			{
				int j = random.next() % wLeft;
				card[i] = tempDeck[j];
				tempDeck[j] = tempDeck[--wLeft];
			}
		}

		for(int i = 0; i < 52 ;++i)
		{
			StackCell& cell = mSCells[ i % SCellNum ];
			cell.push( Card( Card::Suit( card[i] / 13 ) , card[i] % 13 ) );
		}
	}


	int FreeCellLevel::evalMoveCardNum( Cell& from , Cell& to )
	{
		assert( !from.isEmpty() );

		if ( isStackCell( from ) && isStackCell( to ) )
		{
			int num = evalMoveCardNum( static_cast< StackCell& >( from ) , static_cast< StackCell& >( to )  );

			if ( num == 0 ) 
				return 0;

			int emptyFCNum = getEmptyFreeCellNum();
			int emptySCNum = getEmptyStackCellNum();

			if ( to.isEmpty() ) 
				--emptySCNum;

			//canMoveNum = ( emptyFCNum + 1 ) * 2 ^( emptySCNum )
			int canMoveNum = emptyFCNum + 1;
			canMoveNum <<= emptySCNum;

			if ( num <= canMoveNum )
				return num;
			else if ( to.isEmpty() ) 
				return canMoveNum;
			else 
				return 0;
		}
		else if ( to.testRule( from.getCard() ) )
		{
			return 1;
		}
		return 0;
	}

	int FreeCellLevel::evalMoveCardNum( StackCell& from , StackCell& to )
	{
		if ( to.isEmpty() )
			return getRuleCardNum( from );

		Card const& lastToCard = to.getCard();

		int index = from.getCardNum() - 1;
		while( index >= 0 )
		{
			Card const& testCard = from.getCard(index);
			if ( Cell::TestStackRule( lastToCard , testCard ) )
			{
				break;
			}

			if ( index == 0 )
				return 0;

			Card const& nextCard = from.getCard( index -1 );
			if ( ! Cell::TestStackRule( nextCard , testCard ) )
			{
				return 0;
			}
			--index;
		}

		return from.getCardNum() - index;
	}

	int FreeCellLevel::getRuleCardNum( StackCell& cell )
	{
		assert( !cell.isEmpty() );
		int index = cell.getCardNum() - 1;

		while( index  >  0)
		{
			Card const& testCard = cell.getCard(index);
			Card const& nextCard = cell.getCard(index -1);

			if ( !Cell::TestStackRule( nextCard , testCard ) )
				break;

			--index;
		}
		return cell.getCardNum() - index;
	}



	int FreeCellLevel::countEmptyCellNum( int index ,int num )
	{
		int result = 0;
		int end = index + num;
		for(int i = index ;i < end ; ++i )
		{
			Cell& cell = getCell( i );
			if ( cell.isEmpty() )
				++result;
		}
		return result;
	}




	Cell* FreeCellLevel::findEmptyCell(int index, int num)
	{
		int to = index + num;
		for(int i = index ; i < to ; ++i )
		{
			Cell* cell = mCells[ i ];
			if ( cell->isEmpty() )
				return cell;
		}
		return NULL;
	}

	GoalCell* FreeCellLevel::getGoalCell( Card::Suit suit )
	{
		for( int i = 0; i < 4 ; ++i )
		{
			GoalCell& cell = mGCells[i];
			if ( !cell.isEmpty() )
			{
				if ( cell.getCard().getSuit() == suit )
					return &cell;
			}
		}
		return NULL;
	}

	void FreeCellLevel::getMinGoalCardRank(int& blackRank, int& redRank)
	{
		int rank[4] = { 0,0,0,0 };
		for( int i = 0; i < 4; ++i )
		{
			GoalCell& cell = mGCells[i];
			if( !cell.isEmpty() )
			{
				Card const& card = cell.getCard();
				rank[card.getSuit()] = card.getFaceRank();
			}
		}
		blackRank = std::min(rank[Card::eCLUBS], rank[Card::eSPADES]);
		redRank = std::min(rank[Card::eHEARTS], rank[Card::eDIAMONDS]);
	}

	bool FreeCellLevel::moveToGoalCellAuto()
	{
		int blackRank , redRank;
		getMinGoalCardRank(blackRank, redRank);
		for (int i = 0; i < SCellNum ; ++i )
		{
			if ( tryMoveToGoalCellInternal( mSCells[i] , blackRank , redRank  ) )
				return true;
		}

		for (int i = 0; i < FCellNum ; ++i )
		{
			if ( tryMoveToGoalCellInternal( mFCells[i] , blackRank , redRank  ) )
				return true;
		}

		return false;
	}

	bool FreeCellLevel::tryMoveToGoalCellInternal( Cell& cell , int blackRank , int redRank )
	{
		if ( cell.isEmpty() ) 
			return false;

		Card const& card = cell.getCard();

		if ( card.getFace() == Card::eACE )
		{
			SingleCell* emptyCell = getEmptyGoalCell();
			assert( emptyCell );
			return processMoveCard( cell , *emptyCell  , 1 );
		}

		SingleCell* fCell = getGoalCell( card.getSuit() );
		if ( fCell )
		{
			if ( Cell::TestGoalRule( fCell->getCard(), card ) )
			{
				int rank = Card::isBlackSuit( card ) ?  redRank : blackRank ;

				if ( card.getFaceRank() <= rank + 1 )
				{
					return processMoveCard( cell ,*fCell , 1 );
				}
			}
		}
		return false;
	}



	bool FreeCellLevel::processMoveCard( Cell& from , Cell& to , int num )
	{
		assert( num != 0 );

		if ( !preMoveCard( from , to , num ) )
			return false;

		if ( num > 1 )
		{
			assert ( isStackCell(from)  && isStackCell( to ) );
			moveCard( static_cast< StackCell& >( from ), static_cast< StackCell& >( to ) , num );
		}
		else if ( num == 1 )
		{
			moveSingleCard( from , to );
		}
		postMoveCard();
		return true;
	}

	bool FreeCellLevel::tryMoveToStackCell( FreeCell& fc )
	{
		if ( fc.isEmpty() )
			return false;

		Card const& card = fc.getCard();

		Cell* emptyCell = NULL;
		for ( int i = 0 ; i < SCellNum ; ++i )
		{
			StackCell& cell = getStackCell( i );
			if ( cell.isEmpty() )
			{
				if ( !emptyCell )
					emptyCell = &cell;
				continue;
			}
			if ( cell.testRule( card ) )
				return processMoveCard( fc , cell , 1 );
		}

		if ( emptyCell )
			return processMoveCard( fc , *emptyCell , 1 );

		return false;
	}

	bool FreeCellLevel::tryMoveToFreeCell( StackCell& mc )
	{
		if ( mc.isEmpty() )
			return false;
		
		FreeCell* cell = getEmptyFreeCell();
		if ( cell == NULL )
			return false;

		return processMoveCard( mc , *cell , 1 );
	}

	bool FreeCellLevel::tryMoveToGoalCell( StackCell& mc )
	{
		if( mc.isEmpty() )
			return false;

		int blackRank, redRank;
		getMinGoalCardRank(blackRank, redRank);

		return tryMoveToGoalCellInternal( mc , blackRank, redRank);
	}

	bool FreeCellLevel::tryMoveCard(Cell& from, Cell& to)
	{
		if ( from.isEmpty() ) 
			return false;

		if ( isGoalCell( from ) )
			return false;

		int num = evalMoveCardNum( from , to );

		if ( num == 0 )
			return false;

		return processMoveCard( from , to , num );
	}


	void FreeCellLevel::moveSingleCard( Cell& from ,Cell& to )
	{
		Card card = from.getCard();
		from.pop();
		to.push( card );
	}

	void FreeCellLevel::moveCard( StackCell& form , StackCell& to , int num )
	{
		form.moveCard( to , num );
	}

	int FreeCellLevel::getGoalCardNum()
	{
		int result = 0;
		for(int i=0 ; i< 4 ; ++i)
		{
			GoalCell& cell = mGCells[i];
			if ( !cell.isEmpty() )
				result += cell.getCard().getFaceRank() + 1;
		}
		return result;
	}

	bool FreeCellLevel::isStackCell( Cell& cell )
	{
		return cell.getType() == Cell::eSTACK;
	}

	bool FreeCellLevel::isFreeCell( Cell& cell )
	{
		return cell.getType() == Cell::eFREE;
	}

	bool FreeCellLevel::isGoalCell( Cell& cell )
	{
		return cell.getType() == Cell::eGOAL;
	}

	int FreeCellLevel::getEmptyStackCellNum()
	{
		return countEmptyCellNum( SCellIndex , SCellNum );
	}

	int FreeCellLevel::getEmptyFreeCellNum()
	{
		return countEmptyCellNum( FCellIndex , FCellNum );
	}

	GoalCell* FreeCellLevel::getEmptyGoalCell()
	{
		Cell* cell = findEmptyCell( GCellIndex , 4 );
		assert( !cell || isGoalCell( *cell ) );
		return static_cast< GoalCell* >( cell );
	}

	FreeCell* FreeCellLevel::getEmptyFreeCell()
	{
		Cell* cell = findEmptyCell( FCellIndex , FCellNum );
		assert( !cell || isFreeCell( *cell ) );
		return static_cast< FreeCell*>( cell );
	}

	int FreeCellLevel::calcPossibleMoveNum()
	{
		int result = 0;
		for( int i = SCellIndex ; i < SCellIndex + SCellNum ; ++i )
		{
			StackCell& testCell = getCellT<StackCell>( i );
			if ( testCell.isEmpty() )
				continue;

			for( int j = 0; j < TotalCellNum ; ++j )
			{
				if ( evalMoveCardNum( testCell , getCell(j)) )
					++result;
			}
		}
		for( int i = FCellIndex ; i < FCellIndex + FCellNum ; ++i )
		{
			FreeCell& testCell = getCellT<FreeCell>( i );
			if ( testCell.isEmpty() )
				continue;

			for( int j = 0; j < TotalCellNum ; ++j )
			{
				if ( evalMoveCardNum( testCell , getCell(j)) )
					++result;
			}
		}
		return result;
	}


}//namespace Poker