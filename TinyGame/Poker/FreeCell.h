#ifndef FreeCell_H
#define FreeCell_H

#include "PokerBase.h"

#include "DataStructure/FixVector.h"

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
		static bool TestStackRule( Card const& top , Card const& bottom );
		static bool TestGoalRule( Card const& top , Card const& bottom );

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


	class StackCell : public Cell 
	{
	public:
		StackCell():Cell( Cell::eSTACK ){ mCards.reserve( 20 ); }

		bool  isEmpty()    const { return mCards.empty(); }
		int   getCardNum() const { return (int)mCards.size(); }
		void  pop(){ deleteCard(1); }
		Card const& getCard() const {  return mCards.back(); }
		
		void  push( Card const& card ){	mCards.push_back( card );  }
		void  clear(){ mCards.clear(); }

		Card const& getCard(size_t idx) const {  return mCards[idx]; }

		void moveCard(StackCell& to,int num);
		void deleteCard(int num);

		bool   testRule( Card const& card );


		typedef std::vector< Card > CGroup;
		typedef CGroup::iterator iterator;
		CGroup   mCards;
	};

	class FreeCell : public SingleCell
	{
	public:
		FreeCell();
		bool   testRule( Card const& card );
	};

	class GoalCell : public SingleCell
	{
	public:
		GoalCell();

		void   pop();
		bool   testRule( Card const& card );
	};

	class FreeCellLevel
	{
	public:

		FreeCellLevel();
		virtual ~FreeCellLevel();
		typedef   Card::Suit Suit;

		void      setupGame( int seed );
		bool      tryMoveToFreeCell( StackCell& mc );
		bool      tryMoveToStackCell( FreeCell& fc );
		bool      tryMoveCard( Cell& from , Cell& to );
		bool      moveToGoalCellAuto();

		Cell&        getCell( int index ){ assert( 0 <= index && index < TotalCellNum ); return *mCells[index]; }
		GoalCell*    getGoalCell( Card::Suit suit );
		StackCell&   getStackCell( int index ){ return mSCells[ index ]; }

		int       getGoalCardNum();
		int       evalMoveCardNum( Cell& from , Cell& to );

		GoalCell*    getEmptyGoalCell();
		FreeCell*    getEmptyFreeCell();

		int       getEmptyFreeCellNum();
		int       getEmptyStackCellNum();


		int       calcPossibleMoveNum();

		static int getRuleCardNum( StackCell& cell );
		static int evalMoveCardNum( StackCell& from , StackCell& to);

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
		virtual void moveCard( StackCell& form , StackCell& to , int num );

		bool processMoveCard( Cell& from , Cell& to , int num );
		bool moveToGoalCellAuto( Cell& cell , int blackRank , int redRank );

	private:
		StackCell  mSCells[ SCellNum ];
		FreeCell   mFCells[ FCellNum ];
		GoalCell   mGCells[4];
		Cell*      mCells[ TotalCellNum ];

	};


}//namespace Poker

#endif //FreeCell_H