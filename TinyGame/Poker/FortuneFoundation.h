#pragma once
#ifndef FortuneFoundation_H_1D52A8B0_B10A_497A_B579_D24D25915F0C
#define FortuneFoundation_H_1D52A8B0_B10A_497A_B579_D24D25915F0C

#include "PokerBase.h"

#include <array>
#include <vector>

namespace Poker::FortuneFoundation
{
	struct CompactState;
	class Solver;

	class State
	{
	public:
		static int constexpr TableauNum = 11;
		static int constexpr FoundationNum = 4;
		static int constexpr TarotFoundationNum = 2;
		static int constexpr EmptyTableauIndex = TableauNum / 2;

		enum class PileType
		{
			None,
			Tableau,
			Foundation,
			TarotFoundation,
			PokerSlot,
		};

		struct PileRef
		{
			PileType type = PileType::None;
			int index = INDEX_NONE;

			bool isValid() const { return type != PileType::None && index != INDEX_NONE; }
			bool operator == (PileRef const& rhs) const
			{
				return type == rhs.type && index == rhs.index;
			}
		};

		struct MoveInfo
		{
			int fromCardIndex = INDEX_NONE;
			int numCards = 0;
		};

		bool isWin() const;
		bool isMovableSequence(int tableauIndex, int cardIndex) const;
		bool canMove(PileRef from, PileRef to) const;
		bool canMove(PileRef from, PileRef to, int& outCardIndex) const;

		TArray<Card> const& getTableau(int index) const { return mTableaus[index]; }
		Card const& getFoundation(int index) const { return mFoundations[index]; }
		Card const& getTarotFoundation(int index) const { return mTarotFoundations[index]; }
		Card const& getPokerSlot() const { return mPokerSlot; }
		int getFoundationProgress(int index) const;
		int getTarotFoundationProgress(int index) const;
		int getTarotFoundationDrawOrder(int index) const { return mTarotFoundationDrawOrder[index]; }

	protected:
		friend struct CompactState;
		friend class Solver;
		friend class Level;

		TArray<Card>& getPile(PileRef ref);
		TArray<Card> const& getPile(PileRef ref) const;
		int getPileSize(PileRef ref) const;
		Card const& getPileCard(PileRef ref, int cardIndex) const;
		bool getMoveInfo(PileRef from, int fromCardIndex, MoveInfo& outInfo) const;
		int  getMaxTableauMoveNum(PileRef to) const;
		bool isTopOnlyPile(PileType type) const;
		bool canMoveWithCardIndex(PileRef from, PileRef to, int fromCardIndex = INDEX_NONE) const;
		bool canMoveWithCardIndex(PileRef from, PileRef to, int fromCardIndex, MoveInfo& outMoveInfo) const;
		bool canMove(PileRef from, PileRef to, MoveInfo& outMoveInfo) const;
		bool canMoveWithInfo(PileRef from, PileRef to, MoveInfo const& moveInfo) const;
		bool getPreviousTopOnlyCard(PileRef ref, Card const& card, Card& outCard) const;
		bool canMoveCardToPile(Card const& card, Card const& topCard, int numCards, PileRef to) const;
		bool shouldReverseMove(PileRef from, PileRef to, int fromCardIndex, int numCards) const;
		bool canRelayReverseMove(PileRef from, PileRef to, int fromCardIndex, int numCards) const;
		void moveChecked(PileRef from, PileRef to, int numCards, bool bReverse = false);

		std::array<TArray<Card>, TableauNum> mTableaus;
		std::array<Card, FoundationNum> mFoundations;
		std::array<Card, TarotFoundationNum> mTarotFoundations;
		std::array<int, TarotFoundationNum> mTarotFoundationDrawOrder;
		Card mPokerSlot = Card::None();
		bool bFastMove = true;
	};

	class Level : public State
	{
	public:
		using PileType = State::PileType;
		using PileRef = State::PileRef;
		static int constexpr TableauNum = State::TableauNum;
		static int constexpr FoundationNum = State::FoundationNum;
		static int constexpr TarotFoundationNum = State::TarotFoundationNum;
		static int constexpr EmptyTableauIndex = State::EmptyTableauIndex;

		struct MoveRecord
		{
			PileRef from;
			PileRef to;
			int numCards = 0;
			bool bReverse = false;
			bool bAutoMove = false;
			std::array<int, TarotFoundationNum> tarotDrawOrder;
		};

		void setupGame(int seed);
		bool autoMove(PileRef from);
		bool undoMove();

		int getMoveCount() const { return mMoveCount; }

	protected:
		void moveChecked(PileRef from, PileRef to, int numCards, bool bRecord, bool bReverse = false, bool bAutoMove = false);

	private:
		friend class Solver;

		TArray<MoveRecord> mUndoStack;
		int mMoveCount = 0;
	};

}//namespace Poker::FortuneFoundation

#endif // FortuneFoundation_H_1D52A8B0_B10A_497A_B579_D24D25915F0C
