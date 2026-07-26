#include "TinyGamePCH.h"
#include "FortuneFoundationSolver.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <queue>
#include <unordered_set>
#include <utility>

namespace Poker::FortuneFoundation
{
	namespace
	{
		int constexpr InitialReserveLimit = 262144;

		bool IsSamePile(Solver::PileRef lhs, Solver::PileRef rhs)
		{
			return lhs.type == rhs.type && lhs.index == rhs.index;
		}

		bool IsValidPileRef(Solver::PileRef ref)
		{
			switch (ref.type)
			{
			case Solver::PileType::Tableau:
				return 0 <= ref.index && ref.index < State::TableauNum;
			case Solver::PileType::Foundation:
				return 0 <= ref.index && ref.index < State::FoundationNum;
			case Solver::PileType::TarotFoundation:
				return 0 <= ref.index && ref.index < State::TarotFoundationNum;
			case Solver::PileType::PokerSlot:
				return ref.index == 0;
			default:
				return false;
			}
		}

		double GetElapsedMS(std::chrono::steady_clock::time_point startTime)
		{
			return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count();
		}
	}

	Solver::Result Solver::solve(State const& state)
	{
		return solve(state, Config());
	}

	Solver::Result Solver::solve(State const& state, Config const& config)
	{
		auto totalStartTime = std::chrono::steady_clock::now();
		Result result = solveInternal(state, config);
		result.elapsedMS = GetElapsedMS(totalStartTime);
		return result;
	}

	Solver::Result Solver::solveInternal(State const& state, Config const& config)
	{
		auto passStartTime = std::chrono::steady_clock::now();
		Result result;
		if (state.isWin())
		{
			result.bSolved = true;
			result.elapsedMS = GetElapsedMS(passStartTime);
			return result;
		}

		CompactState rootState = CompactState::Make(state);
		std::priority_queue<QueueEntry, std::vector<QueueEntry>, QueueEntryCompare> openList;
		std::unordered_set<StateKey, StateKeyHasher> visited;

		int generatedLimit = config.maxGenerated > 0 ? config.maxGenerated : config.maxVisited;
		generatedLimit = Math::Max(generatedLimit, 1);
		int reserveCount = Math::Min(generatedLimit, InitialReserveLimit);
		visited.reserve(reserveCount);

		TArray<SearchNode> nodes;
		nodes.reserve(reserveCount);

		SearchNode root;
		root.state = rootState;
		root.score = evalScore(root.state, 0);

		visited.insert(makeStateKey(root.state));
		nodes.push_back(root);
		openList.push(QueueEntry{ 0, root.score, 0, 0 });
		result.generatedCount = 1;

		int nextOrder = 1;
		TArray<Move> moves;
		moves.reserve(128);
		int maxOpenNodes = 1;
		int maxVisitedDepth = 0;
		int invalidSolutionCount = 0;

		bool bStopSearch = false;
		while (!openList.empty() && !bStopSearch)
		{
			if (isCancellationRequested(config))
			{
				result.bCancelled = true;
				break;
			}

			if (result.visitedCount >= config.maxVisited)
			{
				result.bAborted = true;
				break;
			}

			QueueEntry entry = openList.top();
			openList.pop();
			SearchNode const& node = nodes[entry.nodeIndex];
			++result.visitedCount;
			maxVisitedDepth = Math::Max(maxVisitedDepth, node.depth);

			if (node.state.isWin())
			{
				TArray<Move> solution = buildMovePath(nodes, entry.nodeIndex);
				TArray<Move> expandedSolution;
				if (!expandMovePath(state, solution, expandedSolution) ||
				    !validateMovePath(state, expandedSolution))
				{
					++invalidSolutionCount;
					result.bAborted = true;
					result.elapsedMS = GetElapsedMS(passStartTime);
					result.maxOpenNodes = maxOpenNodes;
					result.maxVisitedDepth = maxVisitedDepth;
					result.invalidSolutionCount = invalidSolutionCount;
					return result;
				}

				result.bSolved = true;
				result.moves = std::move(expandedSolution);
				result.elapsedMS = GetElapsedMS(passStartTime);
				result.maxOpenNodes = maxOpenNodes;
				result.maxVisitedDepth = maxVisitedDepth;
				result.invalidSolutionCount = invalidSolutionCount;
				return result;
			}

			if (node.depth >= config.maxDepth)
				continue;

			MoveContext context;
			buildMoveContext(node.state, context);
			generateMoves(node.state, context, node.move, moves);

			for (Move const& move : moves)
			{
				if (isCancellationRequested(config))
				{
					result.bCancelled = true;
					bStopSearch = true;
					break;
				}

				if (result.generatedCount >= generatedLimit)
				{
					result.bAborted = true;
					bStopSearch = true;
					break;
				}

				CompactState nextState;
				if (!applyMove(node.state, context, move, nextState))
					continue;

				StateKey key = makeStateKey(nextState);
				if (!visited.insert(key).second)
					continue;

				SearchNode nextNode;
				nextNode.state = nextState;
				nextNode.parent = entry.nodeIndex;
				nextNode.move = move;
				nextNode.depth = node.depth + 1;
				nextNode.score = evalScore(nextNode.state, nextNode.depth);

				int nodeIndex = int(nodes.size());
				nodes.push_back(nextNode);
				openList.push(QueueEntry{ nodeIndex, nextNode.score, nextNode.depth, nextOrder++ });
				maxOpenNodes = Math::Max(maxOpenNodes, int(openList.size()));
				++result.generatedCount;
			}
		}

		result.elapsedMS = GetElapsedMS(passStartTime);
		result.maxOpenNodes = maxOpenNodes;
		result.maxVisitedDepth = maxVisitedDepth;
		result.invalidSolutionCount = invalidSolutionCount;
		return result;
	}

	CompactState CompactState::Make(State const& state)
	{
		CompactState compact;
		int writeIndex = 0;
		for (int pileIndex = 0; pileIndex < State::TableauNum; ++pileIndex)
		{
			compact.offsets[pileIndex] = uint8_t(writeIndex);
			compact.sizes[pileIndex] = uint8_t(state.mTableaus[pileIndex].size());
			for (Card const& card : state.mTableaus[pileIndex])
				compact.cards[writeIndex++] = getCardId(card);
		}

		for (int index = 0; index < State::FoundationNum; ++index)
			compact.foundationProgress[index] = uint8_t(state.getFoundationProgress(index));
		for (int index = 0; index < State::TarotFoundationNum; ++index)
			compact.tarotProgress[index] = uint8_t(state.getTarotFoundationProgress(index));

		compact.pokerSlot = getCardId(state.mPokerSlot);
		compact.bFastMove = state.bFastMove;
		return compact;
	}

	void Solver::generateMoves(CompactState const& state, MoveContext const& context, Move const& prevMove, TArray<Move>& outMoves)
	{
		outMoves.clear();
		addAutoMoves(state, context, prevMove, outMoves);
		if (!outMoves.empty())
			return;

		addTableauMoves(state, context, prevMove, outMoves);
		addPokerSlotMoves(state, context, prevMove, outMoves);
	}

	void Solver::addAutoMoves(CompactState const& state, MoveContext const& context, Move const& prevMove, TArray<Move>& outMoves)
	{
		auto tryAdd = [&](PileRef from) -> bool
		{
			int pileSize = state.getPileSize(from);
			if (!from.isValid() || pileSize == 0)
				return false;

			int topCardIndex = pileSize - 1;
			CompactState::CardId cardId = state.getPileCard(from, topCardIndex);
			if (CompactState::isStandardCard(cardId))
			{
				PileRef to{ PileType::Foundation, CompactState::getCardSuit(cardId) };
				if (from.type == PileType::Tableau && CompactState::isNoneCard(state.pokerSlot))
				{
					int numCards = 0;
					int progress = state.foundationProgress[to.index];
					for (int cardIndex = pileSize - 1; cardIndex >= 0; --cardIndex)
					{
						CompactState::CardId moveCardId = state.getPileCard(from, cardIndex);
						if (!CompactState::isStandardCard(moveCardId) ||
						    CompactState::getCardSuit(moveCardId) != to.index ||
						    CompactState::getCardRank(moveCardId) != progress)
						{
							break;
						}

						++numCards;
						++progress;
					}

					if (numCards > 1)
					{
						Move move{ from, to, pileSize - numCards, numCards };
						if (!isImmediateUndo(move, prevMove))
						{
							outMoves.push_back(move);
							return true;
						}
						return false;
					}
				}

				Move move{ from, to, INDEX_NONE, 1 };
				if (canMove(state, context, move) && !isImmediateUndo(move, prevMove))
				{
					outMoves.push_back(move);
					return true;
				}
			}
			else if (CompactState::isTarotCard(cardId))
			{
				PileRef lowTo{ PileType::TarotFoundation, 0 };
				if (from.type == PileType::Tableau)
				{
					int numCards = 0;
					int progress = state.tarotProgress[lowTo.index];
					for (int cardIndex = pileSize - 1; cardIndex >= 0; --cardIndex)
					{
						CompactState::CardId moveCardId = state.getPileCard(from, cardIndex);
						if (!CompactState::isTarotCard(moveCardId) ||
						    CompactState::getTarotIndex(moveCardId) != progress)
						{
							break;
						}

						++numCards;
						++progress;
					}

					if (numCards > 1)
					{
						Move move{ from, lowTo, pileSize - numCards, numCards };
						if (!isImmediateUndo(move, prevMove))
						{
							outMoves.push_back(move);
							return true;
						}
						return false;
					}
				}

				Move lowMove{ from, lowTo, INDEX_NONE, 1 };
				if (canMove(state, context, lowMove) && !isImmediateUndo(lowMove, prevMove))
				{
					outMoves.push_back(lowMove);
					return true;
				}

				PileRef highTo{ PileType::TarotFoundation, 1 };
				if (from.type == PileType::Tableau)
				{
					int numCards = 0;
					int progress = state.tarotProgress[highTo.index];
					for (int cardIndex = pileSize - 1; cardIndex >= 0; --cardIndex)
					{
						CompactState::CardId moveCardId = state.getPileCard(from, cardIndex);
						if (!CompactState::isTarotCard(moveCardId) ||
						    CompactState::getTarotIndex(moveCardId) != Card::TarotCardNum - 1 - progress)
						{
							break;
						}

						++numCards;
						++progress;
					}

					if (numCards > 1)
					{
						Move move{ from, highTo, pileSize - numCards, numCards };
						if (!isImmediateUndo(move, prevMove))
						{
							outMoves.push_back(move);
							return true;
						}
						return false;
					}
				}

				Move highMove{ from, highTo, INDEX_NONE, 1 };
				if (canMove(state, context, highMove) && !isImmediateUndo(highMove, prevMove))
				{
					outMoves.push_back(highMove);
					return true;
				}
			}

			return false;
		};

		for (int index = 0; index < State::TableauNum; ++index)
		{
			if (tryAdd(PileRef{ PileType::Tableau, index }))
				return;
		}

		tryAdd(PileRef{ PileType::PokerSlot, 0 });
	}

	void Solver::addTableauMoves(CompactState const& state, MoveContext const& context, Move const& prevMove, TArray<Move>& outMoves)
	{
		for (int fromIndex = 0; fromIndex < State::TableauNum; ++fromIndex)
		{
			int pileSize = state.sizes[fromIndex];
			if (pileSize == 0)
				continue;

			PileRef from{ PileType::Tableau, fromIndex };
			bool bAddedEmptyTarget = false;
			for (int toIndex = 0; toIndex < State::TableauNum; ++toIndex)
			{
				if (fromIndex == toIndex)
					continue;

				bool bEmptyTarget = state.sizes[toIndex] == 0;
				if (bEmptyTarget && bAddedEmptyTarget)
					continue;

				PileRef to{ PileType::Tableau, toIndex };
				if (state.bFastMove)
				{
					for (int cardIndex = 0; cardIndex < pileSize; ++cardIndex)
					{
						Move move{ from, to, cardIndex, pileSize - cardIndex };
						if (canMove(state, context, move))
						{
							if (bEmptyTarget && cardIndex == 0 &&
							    !shouldReverseMove(state, from, to, move.fromCardIndex, move.numCards))
							{
								bAddedEmptyTarget = true;
								break;
							}

							if (!isImmediateUndo(move, prevMove))
								outMoves.push_back(move);
							if (bEmptyTarget)
								bAddedEmptyTarget = true;
							break;
						}
					}
				}
				else
				{
					if (bEmptyTarget && pileSize == 1)
						continue;

					Move move{ from, to, INDEX_NONE, 1 };
					if (canMove(state, context, move))
					{
						if (!isImmediateUndo(move, prevMove))
							outMoves.push_back(move);
						if (bEmptyTarget)
							bAddedEmptyTarget = true;
					}
				}
			}

			PileRef pokerSlotTo{ PileType::PokerSlot, 0 };
			Move move{ from, pokerSlotTo, INDEX_NONE, 1 };
			if (canMove(state, context, move) && !isImmediateUndo(move, prevMove))
				outMoves.push_back(move);
		}
	}

	void Solver::addPokerSlotMoves(CompactState const& state, MoveContext const& context, Move const& prevMove, TArray<Move>& outMoves)
	{
		if (CompactState::isNoneCard(state.pokerSlot))
			return;

		PileRef from{ PileType::PokerSlot, 0 };
		bool bAddedEmptyTarget = false;
		for (int toIndex = 0; toIndex < State::TableauNum; ++toIndex)
		{
			bool bEmptyTarget = state.sizes[toIndex] == 0;
			if (bEmptyTarget && bAddedEmptyTarget)
				continue;

			PileRef to{ PileType::Tableau, toIndex };
			Move move{ from, to, INDEX_NONE, 1 };
			if (canMove(state, context, move))
			{
				if (!isImmediateUndo(move, prevMove))
					outMoves.push_back(move);
				if (bEmptyTarget)
					bAddedEmptyTarget = true;
			}
		}
	}

	Solver::StateKey Solver::makeStateKey(CompactState const& state)
	{
		StateKey key;
		struct TableauKey
		{
			uint8_t size = 0;
			std::array<CompactState::CardId, CompactState::MaxTableauCards> cards = {};
		};

		std::array<TableauKey, State::TableauNum> tableauKeys;

		for (int index = 0; index < State::TableauNum; ++index)
		{
			TableauKey pileKey;
			int offset = state.offsets[index];
			int size = state.sizes[index];
			pileKey.size = uint8_t(size);
			for (int cardIndex = 0; cardIndex < size; ++cardIndex)
				pileKey.cards[cardIndex] = state.cards[offset + cardIndex];
			tableauKeys[index] = pileKey;
		}

		std::sort(tableauKeys.begin(), tableauKeys.end(), [](TableauKey const& lhs, TableauKey const& rhs)
		{
			if (lhs.size != rhs.size)
				return lhs.size < rhs.size;

			for (int index = 0; index < lhs.size; ++index)
			{
				if (lhs.cards[index] != rhs.cards[index])
					return lhs.cards[index] < rhs.cards[index];
			}

			return false;
		});

		appendHashByte(key, 'B');
		for (TableauKey const& pileKey : tableauKeys)
		{
			appendHashByte(key, pileKey.size);
			for (int cardIndex = 0; cardIndex < pileKey.size; ++cardIndex)
				appendCardKey(key, pileKey.cards[cardIndex]);
			appendHashByte(key, 0xff);
		}

		appendHashByte(key, 'P');
		appendCardKey(key, state.pokerSlot);

		appendHashByte(key, 'F');
		for (uint8_t progress : state.foundationProgress)
			appendHashByte(key, progress);

		appendHashByte(key, 'T');
		for (uint8_t progress : state.tarotProgress)
			appendHashByte(key, progress);

		appendHashByte(key, state.bFastMove ? '1' : '0');
		return key;
	}

	int Solver::evalScore(CompactState const& state, int depth)
	{
		MoveContext context;
		buildMoveContext(state, context);

		int foundationScore = 0;
		for (uint8_t progress : state.foundationProgress)
			foundationScore += progress;
		for (uint8_t progress : state.tarotProgress)
			foundationScore += progress;

		int buriedPenalty = 0;
		int runScore = 0;
		for (int index = 0; index < State::TableauNum; ++index)
		{
			int pileSize = state.sizes[index];
			if (pileSize > 0)
			{
				buriedPenalty += pileSize - 1;
				runScore += pileSize - context.firstMovableIndex[index];
			}
		}

		auto scoreCardAccess = [&](CompactState::CardId cardId)
		{
			if (CompactState::isNoneCard(cardId))
				return 0;
			if (state.pokerSlot == cardId)
				return CompactState::isStandardCard(cardId) ? 18 : 42;

			for (int pileIndex = 0; pileIndex < State::TableauNum; ++pileIndex)
			{
				int offset = state.offsets[pileIndex];
				int size = state.sizes[pileIndex];
				for (int cardIndex = 0; cardIndex < size; ++cardIndex)
				{
					if (state.cards[offset + cardIndex] != cardId)
						continue;

					int coveredCards = size - 1 - cardIndex;
					int score = 48 - coveredCards * 14;
					if (isMovableSequence(state, context, pileIndex, cardIndex))
						score += 12;
					return score;
				}
			}

			return 0;
		};

		int accessScore = 0;
		for (int suit = 0; suit < State::FoundationNum; ++suit)
		{
			int progress = state.foundationProgress[suit];
			if (progress < 13)
				accessScore += scoreCardAccess(CompactState::CardId(progress * 4 + suit));
		}
		if (state.tarotProgress[0] < Card::TarotCardNum)
			accessScore += scoreCardAccess(CompactState::CardId(Card::StandardCardNum + state.tarotProgress[0]));
		if (state.tarotProgress[1] < Card::TarotCardNum)
			accessScore += scoreCardAccess(CompactState::CardId(Card::StandardCardNum + Card::TarotCardNum - 1 - state.tarotProgress[1]));

		int pokerSlotScore = 0;
		if (CompactState::isNoneCard(state.pokerSlot))
		{
			pokerSlotScore = 24;
		}
		else
		{
			PileRef from{ PileType::PokerSlot, 0 };
			bool bHasLanding = false;
			for (int toIndex = 0; toIndex < State::TableauNum; ++toIndex)
			{
				if (canMove(state, context, from, PileRef{ PileType::Tableau, toIndex }, 0))
				{
					bHasLanding = true;
					break;
				}
			}
			pokerSlotScore = bHasLanding ? 10 : -60;
		}

		return foundationScore * 1000 +
		       context.emptyTableaus * 36 +
		       pokerSlotScore +
		       runScore * 5 +
		       accessScore -
		       buriedPenalty * 4 -
		       depth * 2;
	}

	int CompactState::getCardKey(Card const& card)
	{
		return card.isNone() ? -1 : card.getIndex();
	}

	CompactState::CardId CompactState::getCardId(Card const& card)
	{
		int cardKey = getCardKey(card);
		return cardKey < 0 ? CompactState::NoneCard : CompactState::CardId(cardKey);
	}

	void Solver::appendCardKey(StateKey& key, CompactState::CardId cardId)
	{
		appendHashByte(key, cardId);
	}

	void Solver::appendHashByte(StateKey& key, uint8_t value)
	{
		key.hash[0] ^= uint64_t(value) + 0x9e3779b97f4a7c15ull + (key.hash[0] << 6) + (key.hash[0] >> 2);
		key.hash[1] ^= uint64_t(value + 0x51) + 0xbf58476d1ce4e5b9ull + (key.hash[1] << 7) + (key.hash[1] >> 3);
	}

	TArray<Solver::Move> Solver::buildMovePath(TArray<SearchNode> const& nodes, int nodeIndex)
	{
		TArray<Move> moves;
		for (int index = nodeIndex; index != INDEX_NONE && nodes[index].parent != INDEX_NONE; index = nodes[index].parent)
			moves.push_back(nodes[index].move);

		std::reverse(moves.begin(), moves.end());
		return moves;
	}

	bool Solver::expandMovePath(State const& state, TArray<Move> const& moves, TArray<Move>& outMoves)
	{
		outMoves.clear();
		outMoves.reserve(moves.size());

		State replayState = state;
		for (int moveIndex = 0; moveIndex < int(moves.size()); ++moveIndex)
		{
			Move const& move = moves[moveIndex];
			bool bPackedAutoMove = move.from.type == PileType::Tableau &&
			                       (move.to.type == PileType::Foundation || move.to.type == PileType::TarotFoundation) &&
			                       move.numCards > 1;
			int repeatCount = bPackedAutoMove ? move.numCards : 1;
			for (int repeatIndex = 0; repeatIndex < repeatCount; ++repeatIndex)
			{
				State::MoveInfo moveInfo;
				if (!replayState.canMove(move.from, move.to, moveInfo))
				{
					LogWarning(0, "[FortuneFoundationSolver] Can't expand move step=%d/%d repeat=%d/%d fromType=%d fromIndex=%d toType=%d toIndex=%d",
					           moveIndex + 1, int(moves.size()),
					           repeatIndex + 1, repeatCount,
					           int(move.from.type), move.from.index,
					           int(move.to.type), move.to.index);
					return false;
				}

				Move expandedMove{ move.from, move.to, moveInfo.fromCardIndex, moveInfo.numCards };
				outMoves.push_back(expandedMove);

				bool bReverse = replayState.shouldReverseMove(move.from, move.to, moveInfo.fromCardIndex, moveInfo.numCards);
				replayState.moveChecked(move.from, move.to, moveInfo.numCards, bReverse);
			}
		}

		return true;
	}

	bool Solver::validateMovePath(State const& state, TArray<Move> const& moves)
	{
		State replayState = state;
		for (int moveIndex = 0; moveIndex < int(moves.size()); ++moveIndex)
		{
			Move const& move = moves[moveIndex];

			State::MoveInfo moveInfo;
			moveInfo.fromCardIndex = move.fromCardIndex;
			moveInfo.numCards = move.numCards;
			if (moveInfo.fromCardIndex == INDEX_NONE && moveInfo.numCards > 0)
			{
				int fromPileSize = replayState.getPileSize(move.from);
				moveInfo.fromCardIndex = fromPileSize - moveInfo.numCards;
			}

			if (!replayState.canMoveWithInfo(move.from, move.to, moveInfo))
			{
				int fromSize = move.from.isValid() ? replayState.getPileSize(move.from) : 0;
				int toSize = move.to.isValid() ? replayState.getPileSize(move.to) : 0;
				LogWarning(0, "[FortuneFoundationSolver] Invalid solution move step=%d/%d fromType=%d fromIndex=%d fromSize=%d toType=%d toIndex=%d toSize=%d fromCardIndex=%d numCards=%d",
				           moveIndex + 1, int(moves.size()),
				           int(move.from.type), move.from.index, fromSize,
				           int(move.to.type), move.to.index, toSize,
				           moveInfo.fromCardIndex, moveInfo.numCards);
				return false;
			}

			State::MoveInfo publicMoveInfo;
			if (!replayState.canMove(move.from, move.to, publicMoveInfo) ||
			    publicMoveInfo.fromCardIndex != moveInfo.fromCardIndex ||
			    publicMoveInfo.numCards != moveInfo.numCards)
			{
				LogWarning(0, "[FortuneFoundationSolver] Solution move does not match Level playback step=%d/%d solverFromCardIndex=%d solverNumCards=%d levelFromCardIndex=%d levelNumCards=%d",
				           moveIndex + 1, int(moves.size()),
				           moveInfo.fromCardIndex, moveInfo.numCards,
				           publicMoveInfo.fromCardIndex, publicMoveInfo.numCards);
				return false;
			}

			bool bReverse = replayState.shouldReverseMove(move.from, move.to, moveInfo.fromCardIndex, moveInfo.numCards);
			replayState.moveChecked(move.from, move.to, moveInfo.numCards, bReverse);
		}

		if (!replayState.isWin())
		{
			LogWarning(0, "[FortuneFoundationSolver] Solution path replay did not reach win state. moves=%d", int(moves.size()));
			return false;
		}

		return true;
	}

	bool Solver::isCancellationRequested(Config const& config)
	{
		return config.cancelFlag && config.cancelFlag->load(std::memory_order_relaxed);
	}

	bool CompactState::isNoneCard(CardId cardId)
	{
		return cardId == CompactState::NoneCard;
	}

	bool CompactState::isStandardCard(CardId cardId)
	{
		return cardId < Card::StandardCardNum;
	}

	bool CompactState::isTarotCard(CardId cardId)
	{
		return Card::StandardCardNum <= cardId && cardId < Card::TotalCardNum;
	}

	int CompactState::getCardSuit(CardId cardId)
	{
		return int(cardId) % 4;
	}

	int CompactState::getCardRank(CardId cardId)
	{
		return CompactState::isTarotCard(cardId) ? CompactState::getTarotIndex(cardId) : int(cardId) / 4;
	}

	int CompactState::getTarotIndex(CardId cardId)
	{
		return int(cardId) - Card::StandardCardNum;
	}

	bool CompactState::isAdjacentSameFamily(CardId lhs, CardId rhs)
	{
		if (CompactState::isTarotCard(lhs) || CompactState::isTarotCard(rhs))
			return CompactState::isTarotCard(lhs) && CompactState::isTarotCard(rhs) && Math::Abs(CompactState::getTarotIndex(lhs) - CompactState::getTarotIndex(rhs)) == 1;

		return CompactState::isStandardCard(lhs) && CompactState::isStandardCard(rhs) &&
		       CompactState::getCardSuit(lhs) == CompactState::getCardSuit(rhs) &&
		       Math::Abs(CompactState::getCardRank(lhs) - CompactState::getCardRank(rhs)) == 1;
	}

	CompactState::CardId CompactState::getFoundationCard(int index, int progress)
	{
		if (progress <= 0)
			return CompactState::NoneCard;
		return CompactState::CardId((progress - 1) * 4 + index);
	}

	CompactState::CardId CompactState::getTarotFoundationCard(int index, int progress)
	{
		if (progress <= 0)
			return CompactState::NoneCard;
		if (index == 0)
			return CompactState::CardId(Card::StandardCardNum + progress - 1);
		return CompactState::CardId(Card::StandardCardNum + Card::TarotCardNum - progress);
	}

	int CompactState::getFoundationProgress(int index) const
	{
		if (index < 0 || index >= State::FoundationNum)
			return 0;
		return foundationProgress[index];
	}

	int CompactState::getTarotFoundationProgress(int index) const
	{
		if (index < 0 || index >= State::TarotFoundationNum)
			return 0;
		return tarotProgress[index];
	}

	int CompactState::getPileSize(PileRef ref) const
	{
		switch (ref.type)
		{
		case PileType::Tableau:
			return 0 <= ref.index && ref.index < State::TableauNum ? sizes[ref.index] : 0;
		case PileType::Foundation:
			return 0 <= ref.index && ref.index < State::FoundationNum && foundationProgress[ref.index] > 0 ? 1 : 0;
		case PileType::TarotFoundation:
			return 0 <= ref.index && ref.index < State::TarotFoundationNum && tarotProgress[ref.index] > 0 ? 1 : 0;
		case PileType::PokerSlot:
			return ref.index == 0 && !CompactState::isNoneCard(pokerSlot) ? 1 : 0;
		default: return 0;
		}
	}

	CompactState::CardId CompactState::getPileCard(PileRef ref, int cardIndex) const
	{
		switch (ref.type)
		{
		case PileType::Tableau:
			if (ref.index < 0 || ref.index >= State::TableauNum || cardIndex < 0 || cardIndex >= sizes[ref.index])
				return CompactState::NoneCard;
			return cards[offsets[ref.index] + cardIndex];
		case PileType::Foundation:
			if (ref.index < 0 || ref.index >= State::FoundationNum)
				return CompactState::NoneCard;
			return getFoundationCard(ref.index, foundationProgress[ref.index]);
		case PileType::TarotFoundation:
			if (ref.index < 0 || ref.index >= State::TarotFoundationNum)
				return CompactState::NoneCard;
			return getTarotFoundationCard(ref.index, tarotProgress[ref.index]);
		case PileType::PokerSlot:
			return ref.index == 0 ? pokerSlot : CompactState::NoneCard;
		default:
			return CompactState::NoneCard;
		}
	}

	bool CompactState::isWin() const
	{
		if (tarotProgress[0] + tarotProgress[1] != Card::TarotCardNum)
			return false;

		for (uint8_t progress : foundationProgress)
		{
			if (progress != 13)
				return false;
		}

		return true;
	}

	void Solver::buildMoveContext(CompactState const& state, MoveContext& outContext)
	{
		outContext = MoveContext();
		for (int pileIndex = 0; pileIndex < State::TableauNum; ++pileIndex)
		{
			int size = state.sizes[pileIndex];
			if (size == 0)
			{
				++outContext.emptyTableaus;
				outContext.firstMovableIndex[pileIndex] = 0;
				continue;
			}

			int firstMovable = size - 1;
			int direction = 0;
			int offset = state.offsets[pileIndex];
			for (int cardIndex = size - 2; cardIndex >= 0; --cardIndex)
			{
				CompactState::CardId bottom = state.cards[offset + cardIndex];
				CompactState::CardId top = state.cards[offset + cardIndex + 1];
				if (!CompactState::isAdjacentSameFamily(bottom, top))
					break;

				int step = CompactState::getCardRank(top) - CompactState::getCardRank(bottom);
				if (direction == 0)
				{
					direction = step;
				}
				else if (direction != step)
				{
					break;
				}

				firstMovable = cardIndex;
			}

			outContext.firstMovableIndex[pileIndex] = uint8_t(firstMovable);
		}

		for (int toIndex = 0; toIndex < State::TableauNum; ++toIndex)
		{
			int emptyTableaus = outContext.emptyTableaus - (state.sizes[toIndex] == 0 ? 1 : 0);
			bool bHasEmptyTableauExcept = emptyTableaus > 0;
			outContext.hasEmptyTableauExcept[toIndex] = bHasEmptyTableauExcept ? 1 : 0;
			outContext.maxMoveToTableau[toIndex] = bHasEmptyTableauExcept ? 255 : (CompactState::isNoneCard(state.pokerSlot) ? 2 : 1);
		}
	}

	bool Solver::isMovableSequence(CompactState const& state, MoveContext const& context, int tableauIndex, int cardIndex)
	{
		if (tableauIndex < 0 || tableauIndex >= State::TableauNum)
			return false;

		int size = state.sizes[tableauIndex];
		return 0 <= cardIndex && cardIndex < size && cardIndex >= context.firstMovableIndex[tableauIndex];
	}

	bool Solver::getMoveInfo(CompactState const& state, MoveContext const& context, PileRef from, int fromCardIndex, int numCards, int& outFromCardIndex, int& outNumCards)
	{
		outFromCardIndex = INDEX_NONE;
		outNumCards = 0;

		if (!from.isValid())
			return false;

		int fromPileSize = state.getPileSize(from);
		if (fromPileSize <= 0)
			return false;

		if (numCards > 0)
		{
			if (fromCardIndex == INDEX_NONE)
				fromCardIndex = fromPileSize - numCards;
		}
		else
		{
			if (fromCardIndex == INDEX_NONE)
				fromCardIndex = fromPileSize - 1;

			if (fromCardIndex < 0 || fromCardIndex >= fromPileSize)
				return false;

			switch (from.type)
			{
			case PileType::Tableau:
				numCards = fromPileSize - fromCardIndex;
				break;
			case PileType::Foundation:
			case PileType::TarotFoundation:
			case PileType::PokerSlot:
				numCards = 1;
				break;
			default:
				return false;
			}
		}

		if (fromCardIndex < 0 || fromCardIndex + numCards > fromPileSize)
			return false;

		switch (from.type)
		{
		case PileType::Tableau:
			if (numCards != fromPileSize - fromCardIndex)
				return false;
			if (!isMovableSequence(state, context, from.index, fromCardIndex))
				return false;
			break;
		case PileType::Foundation:
		case PileType::TarotFoundation:
		case PileType::PokerSlot:
			if (fromCardIndex != fromPileSize - 1 || numCards != 1)
				return false;
			break;
		default:
			return false;
		}

		outFromCardIndex = fromCardIndex;
		outNumCards = numCards;
		return true;
	}

	int Solver::getMoveCardNum(CompactState const& state, MoveContext const& context, PileRef from, int fromCardIndex)
	{
		int moveCardIndex = INDEX_NONE;
		int numCards = 0;
		return getMoveInfo(state, context, from, fromCardIndex, 0, moveCardIndex, numCards) ? numCards : 0;
	}

	bool Solver::shouldReverseMove(CompactState const& state, PileRef from, PileRef to, int fromCardIndex, int numCards)
	{
		if (from.type != PileType::Tableau || to.type != PileType::Tableau)
			return false;

		if (numCards <= 1)
			return true;

		if (state.sizes[to.index] == 0)
			return state.bFastMove;

		CompactState::CardId targetTop = state.getPileCard(to, state.sizes[to.index] - 1);
		CompactState::CardId firstCard = state.getPileCard(from, fromCardIndex);
		CompactState::CardId lastCard = state.getPileCard(from, fromCardIndex + numCards - 1);
		return !CompactState::isAdjacentSameFamily(targetTop, firstCard) && CompactState::isAdjacentSameFamily(targetTop, lastCard);
	}

	bool Solver::canRelayReverseMove(CompactState const& state, MoveContext const& context, PileRef from, PileRef to, int fromCardIndex, int numCards)
	{
		if (numCards <= 0)
			return false;

		if (!state.bFastMove || from.type != PileType::Tableau || to.type != PileType::Tableau)
			return false;

		if (state.sizes[to.index] == 0 || !context.hasEmptyTableauExcept[to.index])
			return false;

		CompactState::CardId firstCard = state.getPileCard(from, fromCardIndex);
		CompactState::CardId targetTop = state.getPileCard(to, state.sizes[to.index] - 1);
		return CompactState::isAdjacentSameFamily(targetTop, firstCard);
	}

	bool Solver::canMoveCardToPile(CompactState const& state, CompactState::CardId cardId, CompactState::CardId topCardId, int numCards, PileRef to)
	{
		switch (to.type)
		{
		case PileType::Tableau:
			{
				if (state.sizes[to.index] == 0)
					return true;

				CompactState::CardId bottom = state.getPileCard(to, state.sizes[to.index] - 1);
				return CompactState::isAdjacentSameFamily(bottom, cardId) || CompactState::isAdjacentSameFamily(bottom, topCardId);
			}
		case PileType::Foundation:
			{
				if (!CompactState::isNoneCard(state.pokerSlot))
					return false;

				if (numCards != 1 || !CompactState::isStandardCard(cardId))
					return false;
				if (to.index != CompactState::getCardSuit(cardId))
					return false;

				int progress = state.foundationProgress[to.index];
				if (progress == 0)
					return CompactState::getCardRank(cardId) == Card::eACE;

				return progress == CompactState::getCardRank(cardId);
			}
		case PileType::PokerSlot:
			return numCards == 1 && (CompactState::isStandardCard(cardId) || CompactState::isTarotCard(cardId)) && CompactState::isNoneCard(state.pokerSlot);
		case PileType::TarotFoundation:
			{
				if (numCards != 1 || !CompactState::isTarotCard(cardId))
					return false;
				if (to.index == 0)
					return CompactState::getTarotIndex(cardId) == state.tarotProgress[to.index];
				if (to.index == 1)
					return CompactState::getTarotIndex(cardId) == Card::TarotCardNum - 1 - state.tarotProgress[to.index];
				return false;
			}
		default:
			return false;
		}
	}

	bool Solver::canMove(CompactState const& state, MoveContext const& context, PileRef from, PileRef to, int fromCardIndex)
	{
		Move move{ from, to, fromCardIndex, 0 };
		return canMove(state, context, move);
	}

	bool Solver::canMove(CompactState const& state, MoveContext const& context, Move& move)
	{
		PileRef from = move.from;
		PileRef to = move.to;

		if (!IsValidPileRef(from) || !IsValidPileRef(to))
			return false;

		if (IsSamePile(from, to))
			return false;

		int fromCardIndex = INDEX_NONE;
		int numCards = 0;
		if (!getMoveInfo(state, context, from, move.fromCardIndex, move.numCards, fromCardIndex, numCards))
			return false;

		if (from.type == PileType::Tableau && to.type == PileType::Tableau &&
		    numCards > 1 && !state.bFastMove)
		{
			return false;
		}

		CompactState::CardId cardId = state.getPileCard(from, fromCardIndex);
		CompactState::CardId topCardId = state.getPileCard(from, fromCardIndex + numCards - 1);
		bool bReverseMove = from.type == PileType::Tableau &&
		                    to.type == PileType::Tableau &&
		                    shouldReverseMove(state, from, to, fromCardIndex, numCards);
		bool bRelayReverseMove = canRelayReverseMove(state, context, from, to, fromCardIndex, numCards);

		if (!bReverseMove && !bRelayReverseMove &&
		    from.type == PileType::Tableau && to.type == PileType::Tableau &&
		    numCards > context.maxMoveToTableau[to.index])
		{
			return false;
		}

		if (!canMoveCardToPile(state, cardId, topCardId, numCards, to))
			return false;

		move.fromCardIndex = fromCardIndex;
		move.numCards = numCards;
		return true;
	}

	bool Solver::applyMove(CompactState const& state, MoveContext const& context, Move const& move, CompactState& outState)
	{
		if (!IsValidPileRef(move.from) || !IsValidPileRef(move.to))
		{
			LogWarning(0, "[FortuneFoundationSolver] Invalid move ref in applyMove fromType=%d fromIndex=%d toType=%d toIndex=%d",
			           int(move.from.type), move.from.index, int(move.to.type), move.to.index);
			return false;
		}

		int fromPileSize = state.getPileSize(move.from);
		int numCards = move.numCards > 0 ? move.numCards : getMoveCardNum(state, context, move.from, move.fromCardIndex);
		if (numCards <= 0 || fromPileSize <= 0)
			return false;

		int fromCardIndex = move.fromCardIndex;
		if (fromCardIndex == INDEX_NONE)
			fromCardIndex = fromPileSize - numCards;
		if (fromCardIndex < 0 || fromCardIndex + numCards > fromPileSize)
			return false;

		bool bPackedAutoMove = move.from.type == PileType::Tableau &&
		                       (move.to.type == PileType::Foundation || move.to.type == PileType::TarotFoundation) &&
		                       numCards > 1;
		if (bPackedAutoMove)
		{
			outState = state;
			if (move.to.type == PileType::Foundation)
			{
				outState.foundationProgress[move.to.index] = uint8_t(outState.foundationProgress[move.to.index] + numCards);
			}
			else
			{
				outState.tarotProgress[move.to.index] = uint8_t(outState.tarotProgress[move.to.index] + numCards);
			}

			int writeIndex = 0;
			for (int pileIndex = 0; pileIndex < State::TableauNum; ++pileIndex)
			{
				outState.offsets[pileIndex] = uint8_t(writeIndex);
				int oldOffset = state.offsets[pileIndex];
				int keepSize = state.sizes[pileIndex];
				if (move.from.index == pileIndex)
					keepSize = fromCardIndex;

				outState.sizes[pileIndex] = uint8_t(keepSize);
				for (int cardIndex = 0; cardIndex < keepSize; ++cardIndex)
					outState.cards[writeIndex++] = state.cards[oldOffset + cardIndex];
			}
			return true;
		}

		bool bReverse = shouldReverseMove(state, move.from, move.to, fromCardIndex, numCards);
		std::array<CompactState::CardId, CompactState::MaxTableauCards> movedCards;
		if (move.from.type == PileType::Tableau)
		{
			int writeIndex = 0;
			if (bReverse)
			{
				for (int index = fromPileSize - 1; index >= fromCardIndex; --index)
					movedCards[writeIndex++] = state.getPileCard(move.from, index);
			}
			else
			{
				for (int index = fromCardIndex; index < fromPileSize; ++index)
					movedCards[writeIndex++] = state.getPileCard(move.from, index);
			}
		}
		else
		{
			movedCards[0] = state.getPileCard(move.from, fromCardIndex);
		}

		CompactState::CardId topMovedCard = movedCards[numCards - 1];
		outState = state;

		switch (move.to.type)
		{
		case PileType::Foundation:
			outState.foundationProgress[move.to.index] = uint8_t(CompactState::getCardRank(topMovedCard) + 1);
			break;
		case PileType::TarotFoundation:
			outState.tarotProgress[move.to.index] = uint8_t(move.to.index == 0 ? CompactState::getTarotIndex(topMovedCard) + 1 :
			                                                        Card::TarotCardNum - CompactState::getTarotIndex(topMovedCard));
			break;
		case PileType::PokerSlot:
			outState.pokerSlot = topMovedCard;
			break;
		default:
			break;
		}

		switch (move.from.type)
		{
		case PileType::Foundation:
			outState.foundationProgress[move.from.index] = uint8_t(Math::Max(0, int(outState.foundationProgress[move.from.index]) - 1));
			break;
		case PileType::TarotFoundation:
			outState.tarotProgress[move.from.index] = uint8_t(Math::Max(0, int(outState.tarotProgress[move.from.index]) - 1));
			break;
		case PileType::PokerSlot:
			outState.pokerSlot = CompactState::NoneCard;
			break;
		default:
			break;
		}

		if (move.from.type == PileType::Tableau || move.to.type == PileType::Tableau)
		{
			int writeIndex = 0;
			for (int pileIndex = 0; pileIndex < State::TableauNum; ++pileIndex)
			{
				outState.offsets[pileIndex] = uint8_t(writeIndex);
				int oldOffset = state.offsets[pileIndex];
				int oldSize = state.sizes[pileIndex];
				int keepSize = oldSize;
				if (move.from.type == PileType::Tableau && move.from.index == pileIndex)
					keepSize = fromCardIndex;

				for (int cardIndex = 0; cardIndex < keepSize; ++cardIndex)
					outState.cards[writeIndex++] = state.cards[oldOffset + cardIndex];

				if (move.to.type == PileType::Tableau && move.to.index == pileIndex)
				{
					for (int cardIndex = 0; cardIndex < numCards; ++cardIndex)
						outState.cards[writeIndex++] = movedCards[cardIndex];
				}

				outState.sizes[pileIndex] = uint8_t(writeIndex - outState.offsets[pileIndex]);
			}
		}

		return true;
	}

	bool Solver::isImmediateUndo(Move const& move, Move const& prevMove)
	{
		if (!prevMove.from.isValid() || prevMove.numCards <= 0)
			return false;

		return move.numCards == prevMove.numCards &&
		       IsSamePile(move.from, prevMove.to) &&
		       IsSamePile(move.to, prevMove.from);
	}

	size_t Solver::StateKeyHasher::operator()(StateKey const& key) const
	{
		uint64_t value = key.hash[0] ^ (key.hash[1] + 0x9e3779b97f4a7c15ull + (key.hash[0] << 6) + (key.hash[0] >> 2));
		return size_t(value);
	}

}//namespace Poker::FortuneFoundation
