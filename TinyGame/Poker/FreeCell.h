#ifndef FreeCell_H
#define FreeCell_H

#include "PokerBase.h"

#include "FixVector.h"

#include <vector>
#include <cstdlib>
#include <ctime>
#include <typeinfo>
#include <algorithm>

namespace Poker
{

	class Cell
	{
	public:
		static bool testStackRule( Card const& top , Card const& bottom );
		static bool testGoalRule( Card const& top , Card const& bottom );

		enum Type
		{
			eFREE  ,
			eGOAL  ,
			eSTACK ,
		};
		Cell( Type type ):mType( type ){}

		virtual ~Cell(){}
		virtual void clear() = 0;
		virtual void push( Card const& card ) = 0;
		virtual void pop() = 0;
		virtual bool isEmpty() const = 0;
		virtual Card const& getCard() const = 0;
		virtual bool testRule( Card const& card ) = 0;

		Type getType() const { return mType; }
		int  getIndex() const { return mIndex; }
		void setIndex( int idx ){ mIndex = idx;  }
		
	private:
		Type mType;
		int  mIndex;
	};

	class SingleCell : public Cell
	{
	public:
		SingleCell( Type type )
			:Cell( type )
			,mCard(Card::None())
		{

		}

		void   clear(){ mCard = Card::None(); }
		int    getCardNum() const { return (isEmpty())? 0 : 1 ; }
		bool   isEmpty() const { return  mCard == Card::None();  }
		Card const& getCard() const { return mCard; }
		void   push(Card const& card ){ mCard = card; }
		void   pop(){ mCard = Card::None(); }
	protected:
		Card mCard;
	};


	class SCell : public Cell 
	{
	public:
		SCell():Cell( Cell::eSTACK ){ mCards.reserve( 20 ); }

		bool  isEmpty()    const { return mCards.empty(); }
		int   getCardNum() const { return (int)mCards.size(); }
		void  pop(){ deleteCard(1); }
		Card const& getCard() const {  return mCards.back(); }
		
		void  push( Card const& card ){	mCards.push_back( card );  }
		void  clear(){ mCards.clear(); }

		Card const& getCard(size_t idx) const {  return mCards[idx]; }

		void moveCard(SCell& to,int num);
		void deleteCard(int num);

		bool   testRule( Card const& card )
		{ 
			if ( isEmpty() ) 
				return true; 
			return Cell::testStackRule( getCard() , card );
		}


		typedef std::vector< Card > CGroup;
		typedef CGroup::iterator iterator;
		CGroup   mCards;
	};

	class FCell : public SingleCell
	{
	public:
		FCell():SingleCell( Cell::eFREE ){}
		bool   testRule( Card const& card )
		{
			return isEmpty();
		}
	};

	class GCell : public SingleCell
	{
	public:
		GCell():SingleCell( Cell::eGOAL ){}
		void   pop()
		{
			if ( mCard.getFace() == Card::eACE )
				mCard = Card::None();
			else
				mCard = Card( mCard.getSuit() , mCard.getFaceRank() - 1 );
		}

		bool   testRule( Card const& card )
		{
			if ( isEmpty() )
				return card.getFace() == Card::eACE;
			return Cell::testGoalRule( getCard() , card );
		}
	};

	class FreeCell
	{
	public:

		FreeCell();
		virtual ~FreeCell();
		typedef   Card::Suit Suit;

		void      setupGame( int seed );
		bool      tryMoveToFCell( SCell& mc );
		bool      tryMoveToSCell( FCell& fc );
		bool      tryMoveCard( Cell& from , Cell& to );
		bool      autoMoveToGCell();

		Cell&     getCell( int index ){ assert( 0 <= index && index < TotalCellNum ); return *mCells[index]; }
		GCell*    getGCell( Card::Suit suit );
		SCell&    getSCell( int index ){ return mSCells[ index ]; }

		int       getGCardNum();
		int       evalMoveCardNum( Cell& from , Cell& to );

		GCell*    getEmptyGCell();
		FCell*    getEmptyFCell();

		int       getEmptyFCellNum();
		int       getEmptySCellNum();


		int       calcPossibleMoveNum();

		static int getRuleCardNum( SCell& cell );
		static int evalMoveCardNum( SCell& from , SCell& to);

		static bool isStackCell( Cell& cell );
		static bool isFreeCell( Cell& cell );
		static bool isGoalCell( Cell& cell );

		static int const SCellNum = 8;
		static int const FCellNum = 4;
		static int const TotalCellNum = SCellNum + FCellNum + 4;

		static int const SCellIndex = 0;
		static int const FCellIndex = SCellNum;
		static int const GCellIndex = SCellNum + FCellNum;

	private:
		

		Cell*   findEmptyCell( int index ,int num );
		int     countEmptyCellNum( int index ,int num );

	protected:
		virtual bool preMoveCard( Cell& from , Cell& to , int num ){ return true; }
		virtual void postMoveCard(){}
		virtual void moveSingleCard( Cell& from ,Cell& to );
		virtual void moveCard( SCell& form , SCell& to , int num );

		bool processMoveCard( Cell& from , Cell& to , int num );
		bool autoToGCell( Cell& cell , int blackRank , int redRank );

	private:
		SCell   mSCells[ SCellNum ];
		FCell   mFCells[ FCellNum ];
		GCell   mGCells[4];
		Cell*   mCells[ TotalCellNum ];

	};


}//namespace Poker

#endif //FreeCell_H