#ifndef FreeCell_H
#define FreeCell_H

#include "PokerBase.h"

#include "DataStructure/FixedArray.h"

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

		int    getCardNum() const { return (isEmpty()) ? 0 : 1; }
		void   clear() final { mCard = Card::None(); }
		bool   isEmpty() const final { return  mCard == Card::None();  }
		Card const& getCard() const final { return mCard; }
		void   push(Card const& card) final { mCard = card; }

	protected:
		Card mCard;
	};


	class StackCell final : public Cell
	{
	public:
		StackCell():Cell( Cell::eSTACK ){ mCards.reserve( 20 ); }

		bool  isEmpty()    const override { return mCards.empty(); }
		int   getCardNum() const { return (int)mCards.size(); }
		void  pop() override{ mCards.pop_back(); }
		Card const& getCard() const override {  return mCards.back(); }
		
		void  push( Card const& card ) override{	mCards.push_back( card );  }
		void  clear() override{ mCards.clear(); }

		Card const& getCard(size_t idx) const {  return mCards[idx]; }

		void  moveCard(StackCell& to,int num);
		bool  testRule( Card const& card ) override;

		TArray< Card >   mCards;
	};

	class FreeCell final : public SingleCell
	{
	public:
		FreeCell();

		void   pop() override { mCard = Card::None(); }
		bool   testRule( Card const& card ) override;

	};

	class GoalCell final : public SingleCell
	{
	public:
		GoalCell();

		void   pop() override;
		bool   testRule( Card const& card ) override;
	};

	class FreeCellLevel
	{
	public:

		FreeCellLevel();
		virtual ~FreeCellLevel();
		using Suit = Card::Suit;

		void      setupGame( int seed );
		bool      tryMoveToFreeCell( StackCell& sCell );
		bool      tryMoveToGoalCell( StackCell& cell );
		bool      tryMoveToStackCell( FreeCell& fCell );
		bool      tryMoveCard( Cell& from , Cell& to );
		bool      moveToGoalCellAuto();

		Cell&        getCell( int index ){ assert( 0 <= index && index < TotalCellNum ); return *mCells[index]; }
		template< class T >
		T&           getCellT(int index) { return static_cast<T&>(getCell(index)); }
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

		void getMinGoalCardRank(int& blackRank, int& redRank);
		bool processMoveCard( Cell& from , Cell& to , int num );
		bool tryMoveToGoalCellInternal( Cell& cell , int blackRank , int redRank );

	private:
		StackCell  mSCells[ SCellNum ];
		FreeCell   mFCells[ FCellNum ];
		GoalCell   mGCells[ 4 ];
		Cell*      mCells[ TotalCellNum ];

	};


}//namespace Poker

#endif //FreeCell_H