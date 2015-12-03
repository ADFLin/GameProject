#include "FreeCell.h"


#include <cassert>

namespace Poker
{


	Card const SingleCell::NoCard(-1);


	bool Cell::testStackRule( Card const& top,Card const& bottom )
	{
		return  ( top.getFaceRank() == bottom.getFaceRank() + 1 ) && 
			!Card::isSameColorSuit( top , bottom );
	}

	bool Cell::testGoalRule( Card const& top,Card const& bottom )
	{
		return  ( top.getFaceRank() + 1 == bottom.getFaceRank() ) && 
			top.getSuit() == bottom.getSuit();
	}


	void SCell::moveCard( SCell& to ,int num )
	{
		assert( num <= getCardNum() );

		for(iterator iter = mCards.end() - num; 
			iter != mCards.end() ; ++iter )
		{	
			to.push(*iter);	
		}
		deleteCard(num);
	}


	void SCell::deleteCard( int num )
	{
		assert( num <= getCardNum() );
		mCards.erase( mCards.end() - num , mCards.end() );
	}


	FreeCell::FreeCell()
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

	FreeCell::~FreeCell()
	{

	}

	void FreeCell::setupGame( int seed )
	{
		for ( int i = 0 ; i < TotalCellNum ; ++i )
			mCells[i]->clear();

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
			SCell& cell = mSCells[ i % SCellNum ];
			cell.push( Card( Card::Suit( card[i] / 13 ) , card[i] % 13 ) );
		}
	}


	int FreeCell::evalMoveCardNum( Cell& from , Cell& to )
	{
		assert( !from.isEmpty() );

		if ( isStackCell( from ) && isStackCell( to ) )
		{
			int num = evalMoveCardNum( static_cast< SCell& >( from ) , static_cast< SCell& >( to )  );

			if ( num == 0 ) 
				return 0;

			int emptyFCNum = getEmptyFCellNum();
			int emptySCNum = getEmptySCellNum();

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

	int FreeCell::evalMoveCardNum( SCell& from , SCell& to )
	{
		if ( to.isEmpty() )
			return getRuleCardNum( from );

		Card const& lastToCard = to.getCard();

		int index = from.getCardNum() - 1;
		while( index >= 0 )
		{
			Card const& testCard = from.getCard(index);
			if ( Cell::testStackRule( lastToCard , testCard ) )
			{
				break;
			}

			if ( index == 0 )
				return 0;

			Card const& nextCard = from.getCard( index -1 );
			if ( ! Cell::testStackRule( nextCard , testCard ) )
			{
				return 0;
			}
			--index;
		}

		return from.getCardNum() - index;
	}

	int FreeCell::getRuleCardNum( SCell& cell )
	{
		assert( !cell.isEmpty() );
		int index = cell.getCardNum() - 1;

		while( index  >  0)
		{
			Card const& testCard = cell.getCard(index);
			Card const& nextCard = cell.getCard(index -1);

			if ( !Cell::testStackRule( nextCard , testCard ) )
				break;

			--index;
		}
		return cell.getCardNum() - index;
	}



	int FreeCell::countEmptyCellNum( int index ,int num )
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


	Cell* FreeCell::findEmptyCell( int index ,int num )
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

	GCell* FreeCell::getGCell( Card::Suit suit )
	{
		for( int i = 0; i < 4 ; ++i )
		{
			GCell& cell = mGCells[i];
			if ( !cell.isEmpty() )
			{
				if ( cell.getCard().getSuit() == suit )
					return &cell;
			}
		}
		return NULL;
	}

	bool FreeCell::autoMoveToGCell()
	{
		int rank[4]={0,0,0,0};
		for( int i = 0; i < 4 ; ++i )
		{
			GCell& cell = mGCells[i];
			if ( !cell.isEmpty() )
			{
				Card const& card = cell.getCard();
				rank[card.getSuit()] = card.getFaceRank();
			}
		}

		int blackRank = std::min( rank[ Card::eCLUBS ] , rank[ Card::eSPADES ] );
		int redRank   = std::min( rank[ Card::eHEARTS ] , rank[ Card::eDIAMONDS ] );

		for (int i = 0; i < SCellNum ; ++i )
		{
			if ( autoToGCell( mSCells[i] , blackRank , redRank  ) )
				return true;
		}

		for (int i = 0; i < FCellNum ; ++i )
		{
			if ( autoToGCell( mFCells[i] , blackRank , redRank  ) )
				return true;
		}

		return false;
	}

	bool FreeCell::autoToGCell( Cell& cell , int blackRank , int redRank )
	{
		if ( cell.isEmpty() ) 
			return false;

		Card const& card = cell.getCard();

		if ( card.getFace() == Card::eACE )
		{
			SingleCell* emptyCell = getEmptyGCell();
			assert( emptyCell );
			return processMoveCard( cell , *emptyCell  , 1 );
		}

		SingleCell* fCell = getGCell( card.getSuit() );
		if ( fCell )
		{
			if ( Cell::testGoalRule( fCell->getCard(), card ) )
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



	bool FreeCell::processMoveCard( Cell& from , Cell& to , int num )
	{
		assert( num != 0 );

		if ( !preMoveCard( from , to , num ) )
			return false;

		if ( num > 1 )
		{
			assert ( isStackCell(from)  && isStackCell( to ) );
			moveCard( static_cast< SCell& >( from ), static_cast< SCell& >( to ) , num );
		}
		else if ( num == 1 )
		{
			moveSingleCard( from , to );
		}
		postMoveCard();
		return true;
	}

	bool FreeCell::tryMoveToSCell( FCell& fc )
	{
		if ( fc.isEmpty() )
			return false;

		Card const& card = fc.getCard();

		Cell* emptyCell = NULL;
		for ( int i = 0 ; i < SCellNum ; ++i )
		{
			SCell& cell = getSCell( i );
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

	bool FreeCell::tryMoveToFCell( SCell& mc )
	{
		if ( mc.isEmpty() )
			return false;
		

		FCell* cell = getEmptyFCell();
		if ( cell == NULL )
			return false;

		return processMoveCard( mc , *cell , 1 );
	}

	bool FreeCell::tryMoveCard( Cell& from , Cell& to )
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


	void FreeCell::moveSingleCard( Cell& from ,Cell& to )
	{
		Card card = from.getCard();
		from.pop();
		to.push( card );
	}

	void FreeCell::moveCard( SCell& form , SCell& to , int num )
	{
		form.moveCard( to , num );
	}

	int FreeCell::getGCardNum()
	{
		int result = 0;
		for(int i=0 ; i< 4 ; ++i)
		{
			GCell& cell = mGCells[i];
			if ( !cell.isEmpty() )
				result += cell.getCard().getFaceRank() + 1;
		}
		return result;
	}

	bool FreeCell::isStackCell( Cell& cell )
	{
		return cell.getType() == Cell::eSTACK;
	}

	bool FreeCell::isFreeCell( Cell& cell )
	{
		return cell.getType() == Cell::eFREE;
	}

	bool FreeCell::isGoalCell( Cell& cell )
	{
		return cell.getType() == Cell::eGOAL;
	}

	int FreeCell::getEmptySCellNum()
	{
		return countEmptyCellNum( SCellIndex , SCellNum );
	}

	int FreeCell::getEmptyFCellNum()
	{
		return countEmptyCellNum( FCellIndex , FCellNum );
	}

	GCell* FreeCell::getEmptyGCell()
	{
		Cell* cell = findEmptyCell( GCellIndex , 4 );
		assert( !cell || isGoalCell( *cell ) );
		return static_cast< GCell* >( cell );
	}

	FCell* FreeCell::getEmptyFCell()
	{
		Cell* cell = findEmptyCell( FCellIndex , FCellNum );
		assert( !cell || isFreeCell( *cell ) );
		return static_cast< FCell*>( cell );
	}

	int FreeCell::calcPossibleMoveNum()
	{
		int result = 0;
		for( int i = SCellIndex ; i < SCellIndex + SCellNum ; ++i )
		{
			Cell& testCell = getCell( i );
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
			Cell& testCell = getCell( i );
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