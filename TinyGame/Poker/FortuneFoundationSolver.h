#pragma once
#ifndef FortuneFoundationSolver_H_8E97D1CE_07A7_4426_BC2E_DFFDF4EA41A8
#define FortuneFoundationSolver_H_8E97D1CE_07A7_4426_BC2E_DFFDF4EA41A8

#include "FortuneFoundation.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Poker::FortuneFoundation
{
	struct CompactState
	{
		using PileRef = State::PileRef;
		using PileType = State::PileType;
		using CardId = uint8_t;
		static CardId constexpr NoneCard = 0xff;
		static int constexpr MaxTableauCards = Card::TotalCardNum;

		std::array<CardId, MaxTableauCards> cards = {};
		std::array<uint8_t, State::TableauNum> offsets = {};
		std::array<uint8_t, State::TableauNum> sizes = {};
		std::array<uint8_t, State::FoundationNum> foundationProgress = {};
		std::array<uint8_t, State::TarotFoundationNum> tarotProgress = {};
		CardId pokerSlot = NoneCard;
		bool bFastMove = true;

		static CompactState Make(State const& state);
		static int getCardKey(Card const& card);
		static CardId getCardId(Card const& card);
		static bool isNoneCard(CardId cardId);
		static bool isStandardCard(CardId cardId);
		static bool isTarotCard(CardId cardId);
		static int getCardSuit(CardId cardId);
		static int getCardRank(CardId cardId);
		static int getTarotIndex(CardId cardId);
		static bool isAdjacentSameFamily(CardId lhs, CardId rhs);
		static CardId getFoundationCard(int index, int progress);
		static CardId getTarotFoundationCard(int index, int progress);

		int getFoundationProgress(int index) const;
		int getTarotFoundationProgress(int index) const;
		int getPileSize(PileRef ref) const;
		CardId getPileCard(PileRef ref, int cardIndex) const;
		bool isWin() const;
	};

	class Solver
	{
	public:
		using PileRef = State::PileRef;
		using PileType = State::PileType;

		struct Move
		{
			PileRef from;
			PileRef to;
			int fromCardIndex = INDEX_NONE;
			int numCards = 0;
		};

		struct Config
		{
			int maxVisited = 4000000;
			int maxGenerated = 4000000;
			int maxDepth = 40000;
			std::atomic_bool const* cancelFlag = nullptr;
		};

		struct Result
		{
			bool bSolved = false;
			bool bAborted = false;
			bool bCancelled = false;
			int visitedCount = 0;
			int generatedCount = 0;
			double elapsedMS = 0.0;
			int maxOpenNodes = 0;
			int maxVisitedDepth = 0;
			int invalidSolutionCount = 0;
			TArray<Move> moves;
		};

		Result solve(State const& state);
		Result solve(State const& state, Config const& config);

	private:
		struct StateKey
		{
			uint64_t hash[2] = { 0, 0 };

			bool operator == (StateKey const& rhs) const
			{
				return hash[0] == rhs.hash[0] && hash[1] == rhs.hash[1];
			}
		};

		struct StateKeyHasher
		{
			size_t operator()(StateKey const& key) const;
		};

		struct MoveContext
		{
			std::array<uint8_t, State::TableauNum> firstMovableIndex = {};
			std::array<uint8_t, State::TableauNum> maxMoveToTableau = {};
			std::array<uint8_t, State::TableauNum> hasEmptyTableauExcept = {};
			int emptyTableaus = 0;
		};

		struct SearchNode
		{
			CompactState state;
			int parent = INDEX_NONE;
			Move move;
			int depth = 0;
			int score = 0;
		};

		struct QueueEntry
		{
			int nodeIndex = INDEX_NONE;
			int score = 0;
			int depth = 0;
			int order = 0;
		};

		struct QueueEntryCompare
		{
			bool operator()(QueueEntry const& lhs, QueueEntry const& rhs) const
			{
				if (lhs.score != rhs.score)
					return lhs.score < rhs.score;
				if (lhs.depth != rhs.depth)
					return lhs.depth > rhs.depth;
				return lhs.order > rhs.order;
			}
		};

		Result solveInternal(State const& state, Config const& config);
		static StateKey makeStateKey(CompactState const& state);
		static int evalScore(CompactState const& state, int depth);
		static void appendCardKey(StateKey& key, CompactState::CardId cardId);
		static void appendHashByte(StateKey& key, uint8_t value);
		static TArray<Move> buildMovePath(TArray<SearchNode> const& nodes, int nodeIndex);
		static bool expandMovePath(State const& state, TArray<Move> const& moves, TArray<Move>& outMoves);
		static bool validateMovePath(State const& state, TArray<Move> const& moves);
		static bool isCancellationRequested(Config const& config);

		static void buildMoveContext(CompactState const& state, MoveContext& outContext);
		static bool isMovableSequence(CompactState const& state, MoveContext const& context, int tableauIndex, int cardIndex);
		static bool getMoveInfo(CompactState const& state, MoveContext const& context, PileRef from, int fromCardIndex, int numCards, int& outFromCardIndex, int& outNumCards);
		static int getMoveCardNum(CompactState const& state, MoveContext const& context, PileRef from, int fromCardIndex);
		static bool shouldReverseMove(CompactState const& state, PileRef from, PileRef to, int fromCardIndex, int numCards);
		static bool canRelayReverseMove(CompactState const& state, MoveContext const& context, PileRef from, PileRef to, int fromCardIndex, int numCards);
		static bool canMoveCardToPile(CompactState const& state, CompactState::CardId cardId, CompactState::CardId topCardId, int numCards, PileRef to);
		static bool canMove(CompactState const& state, MoveContext const& context, PileRef from, PileRef to, int fromCardIndex);
		static bool canMove(CompactState const& state, MoveContext const& context, Move& move);
		static bool applyMove(CompactState const& state, MoveContext const& context, Move const& move, CompactState& outState);
		static bool isImmediateUndo(Move const& move, Move const& prevMove);

		static void generateMoves(CompactState const& state, MoveContext const& context, Move const& prevMove, TArray<Move>& outMoves);
		static void addAutoMoves(CompactState const& state, MoveContext const& context, Move const& prevMove, TArray<Move>& outMoves);
		static void addTableauMoves(CompactState const& state, MoveContext const& context, Move const& prevMove, TArray<Move>& outMoves);
		static void addPokerSlotMoves(CompactState const& state, MoveContext const& context, Move const& prevMove, TArray<Move>& outMoves);
	};

}//namespace Poker::FortuneFoundation

#endif // FortuneFoundationSolver_H_8E97D1CE_07A7_4426_BC2E_DFFDF4EA41A8
