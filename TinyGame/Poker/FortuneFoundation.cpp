#include "TinyGamePCH.h"
#include "FortuneFoundation.h"

#include <algorithm>
#include <random>

namespace Poker::FortuneFoundation
{
	namespace
	{
		bool IsSameFamily(Card const& lhs, Card const& rhs)
		{
			if (lhs.isTarot() || rhs.isTarot())
				return lhs.isTarot() && rhs.isTarot();

			return lhs.isStandard() && rhs.isStandard() && lhs.getSuit() == rhs.getSuit();
		}

		int GetFamilyRank(Card const& card)
		{
			return card.isTarot() ? card.getTarotIndex() : card.getFaceRank();
		}

		bool IsAdjacentSameFamily(Card const& lhs, Card const& rhs)
		{
			return IsSameFamily(lhs, rhs) && Math::Abs(GetFamilyRank(lhs) - GetFamilyRank(rhs)) == 1;
		}
	}

	void Level::setupGame(int seed)
	{
		for (auto& pile : mTableaus)
			pile.clear();
		for (auto& card : mFoundations)
			card = Card::None();
		for (auto& card : mTarotFoundations)
			card = Card::None();
		for (auto& order : mTarotFoundationDrawOrder)
			order = 0;
		mPokerSlot = Card::None();

		mUndoStack.clear();
		mMoveCount = 0;

		TArray<Card> deck;
		deck.reserve(Card::TotalCardNum);
		for (int index = 0; index < Card::StandardCardNum; ++index)
		{
			Card card(index);
			if (card.getFace() == Card::eACE)
			{
				mFoundations[int(card.getSuit())] = card;
			}
			else
			{
				deck.push_back(card);
			}
		}
		for (int index = 0; index < Card::TarotCardNum; ++index)
			deck.push_back(Card::Tarot(index));

		std::mt19937 random(seed ? seed : 1);
		std::shuffle(deck.begin(), deck.end(), random);

		int pileIndex = 0;
		for (Card const& card : deck)
		{
			while (pileIndex == EmptyTableauIndex)
				pileIndex = (pileIndex + 1) % TableauNum;

			mTableaus[pileIndex].push_back(card);
			pileIndex = (pileIndex + 1) % TableauNum;
		}
	}

	bool Level::autoMove(PileRef from)
	{
		if (!from.isValid())
			return false;

		int fromPileSize = getPileSize(from);
		if (fromPileSize == 0)
			return false;

		int fromCardIndex = fromPileSize - 1;
		Card const& card = getPileCard(from, fromCardIndex);
		if (card.isTarot())
		{
			MoveInfo moveInfo;
			PileRef lowTo{ PileType::TarotFoundation, 0 };
			if (canMove(from, lowTo, moveInfo))
			{
				moveChecked(from, lowTo, moveInfo.numCards, true, false, true);
				return true;
			}

			PileRef highTo{ PileType::TarotFoundation, 1 };
			if (canMove(from, highTo, moveInfo))
			{
				moveChecked(from, highTo, moveInfo.numCards, true, false, true);
				return true;
			}
			return false;
		}

		if (card.isStandard())
		{
			MoveInfo moveInfo;
			PileRef to{ PileType::Foundation, int(card.getSuit()) };
			if (canMove(from, to, moveInfo))
			{
				moveChecked(from, to, moveInfo.numCards, true, false, true);
				return true;
			}
			return false;
		}

		return false;
	}

	bool Level::undoMove()
	{
		if (mUndoStack.empty())
			return false;

		bool bUndone = false;
		bool bContinueUndo = false;
		do
		{
			MoveRecord record = mUndoStack.back();
			mUndoStack.pop_back();
			bContinueUndo = record.bAutoMove && !mUndoStack.empty();

			PileRef from = record.to;
			PileRef to = record.from;
			State::moveChecked(from, to, record.numCards, record.bReverse);
			mTarotFoundationDrawOrder = record.tarotDrawOrder;

			--mMoveCount;
			bUndone = true;
		} while (bContinueUndo);

		return bUndone;
	}

	bool State::isWin() const
	{
		if (getTarotFoundationProgress(0) + getTarotFoundationProgress(1) != Card::TarotCardNum)
			return false;

		for (auto const& foundation : mFoundations)
		{
			if (foundation.isNone() || foundation.getFace() != Card::eKING)
				return false;
		}

		return true;
	}

	int State::getFoundationProgress(int index) const
	{
		if (index < 0 || index >= FoundationNum || mFoundations[index].isNone())
			return 0;

		return mFoundations[index].getFaceRank() + 1;
	}

	int State::getTarotFoundationProgress(int index) const
	{
		if (index < 0 || index >= TarotFoundationNum || mTarotFoundations[index].isNone())
			return 0;

		Card const& card = mTarotFoundations[index];
		if (!card.isTarot())
			return 0;

		if (index == 0)
			return card.getTarotIndex() + 1;

		return Card::TarotCardNum - card.getTarotIndex();
	}

	bool State::isMovableSequence(int tableauIndex, int cardIndex) const
	{
		if (tableauIndex < 0 || tableauIndex >= TableauNum)
			return false;

		TArray<Card> const& pile = mTableaus[tableauIndex];
		if (cardIndex < 0 || cardIndex >= int(pile.size()))
			return false;

		int direction = 0;
		for (int index = cardIndex; index + 1 < int(pile.size()); ++index)
		{
			Card const& bottom = pile[index];
			Card const& top = pile[index + 1];

			if (!IsAdjacentSameFamily(bottom, top))
				return false;

			int step = GetFamilyRank(top) - GetFamilyRank(bottom);
			if (direction == 0)
			{
				direction = step;
			}
			else if (direction != step)
			{
				return false;
			}
		}

		return true;
	}

	bool State::canMove(PileRef from, PileRef to) const
	{
		MoveInfo moveInfo;
		return canMove(from, to, moveInfo);
	}

	bool State::canMove(PileRef from, PileRef to, int& outCardIndex) const
	{
		outCardIndex = INDEX_NONE;
		MoveInfo moveInfo;
		if (!canMove(from, to, moveInfo))
			return false;

		outCardIndex = moveInfo.fromCardIndex;
		return true;
	}

	bool State::canMove(PileRef from, PileRef to, MoveInfo& outMoveInfo) const
	{
		outMoveInfo = MoveInfo();

		if (!from.isValid() || !to.isValid())
			return false;

		int fromPileSize = getPileSize(from);
		if (fromPileSize == 0)
			return false;

		MoveInfo moveInfo;
		if (from.type == PileType::Tableau && to.type == PileType::Tableau && bFastMove)
		{
			for (int cardIndex = 0; cardIndex < fromPileSize; ++cardIndex)
			{
				if (getMoveInfo(from, cardIndex, moveInfo) && canMoveWithInfo(from, to, moveInfo))
				{
					outMoveInfo = moveInfo;
					return true;
				}
			}

			return false;
		}

		if (!getMoveInfo(from, INDEX_NONE, moveInfo) || !canMoveWithInfo(from, to, moveInfo))
			return false;

		outMoveInfo = moveInfo;
		return true;
	}

	bool State::canMoveWithCardIndex(PileRef from, PileRef to, int fromCardIndex) const
	{
		MoveInfo moveInfo;
		return canMoveWithCardIndex(from, to, fromCardIndex, moveInfo);
	}

	bool State::canMoveWithCardIndex(PileRef from, PileRef to, int fromCardIndex, MoveInfo& outMoveInfo) const
	{
		outMoveInfo = MoveInfo();

		MoveInfo moveInfo;
		if (!getMoveInfo(from, fromCardIndex, moveInfo) || !canMoveWithInfo(from, to, moveInfo))
			return false;

		outMoveInfo = moveInfo;
		return true;
	}

	bool State::canMoveWithInfo(PileRef from, PileRef to, MoveInfo const& moveInfo) const
	{
		if (!from.isValid() || !to.isValid())
			return false;

		if (from == to)
			return false;

		if (from.type == to.type && from.index == to.index)
			return false;

		int fromCardIndex = moveInfo.fromCardIndex;
		int numCards = moveInfo.numCards;
		if (numCards <= 0)
			return false;

		if (from.type == PileType::Tableau && to.type == PileType::Tableau &&
		    numCards > 1 && !bFastMove)
		{
			return false;
		}

		Card const& card = getPileCard(from, fromCardIndex);
		Card const& topCard = getPileCard(from, fromCardIndex + numCards - 1);
		bool bReverseMove = from.type == PileType::Tableau &&
		                    to.type == PileType::Tableau &&
		                    shouldReverseMove(from, to, fromCardIndex, numCards);
		bool bRelayReverseMove = canRelayReverseMove(from, to, fromCardIndex, numCards);

		if (!bReverseMove && !bRelayReverseMove &&
		    from.type == PileType::Tableau && to.type == PileType::Tableau &&
		    numCards > getMaxTableauMoveNum(to))
		{
			return false;
		}

		return canMoveCardToPile(card, topCard, numCards, to);
	}

	TArray<Card>& State::getPile(PileRef ref)
	{
		switch (ref.type)
		{
		case PileType::Tableau: return mTableaus[ref.index];
		default: break;
		}
		assert(false);
		return mTableaus[0];
	}

	TArray<Card> const& State::getPile(PileRef ref) const
	{
		switch (ref.type)
		{
		case PileType::Tableau: return mTableaus[ref.index];
		default: break;
		}
		assert(false);
		return mTableaus[0];
	}

	int State::getPileSize(PileRef ref) const
	{
		switch (ref.type)
		{
		case PileType::Tableau: return int(mTableaus[ref.index].size());
		case PileType::Foundation: return mFoundations[ref.index].isNone() ? 0 : 1;
		case PileType::TarotFoundation: return mTarotFoundations[ref.index].isNone() ? 0 : 1;
		case PileType::PokerSlot: return mPokerSlot.isNone() ? 0 : 1;
		default: return 0;
		}
	}

	Card const& State::getPileCard(PileRef ref, int cardIndex) const
	{
		switch (ref.type)
		{
		case PileType::Tableau: return mTableaus[ref.index][cardIndex];
		case PileType::Foundation: return mFoundations[ref.index];
		case PileType::TarotFoundation: return mTarotFoundations[ref.index];
		case PileType::PokerSlot: return mPokerSlot;
		default: break;
		}

		assert(false);
		return mTableaus[0][0];
	}

	bool State::getMoveInfo(PileRef from, int fromCardIndex, MoveInfo& outInfo) const
	{
		outInfo = MoveInfo();

		if (!from.isValid())
			return false;

		int fromPileSize = getPileSize(from);
		if (fromCardIndex == INDEX_NONE)
			fromCardIndex = fromPileSize - 1;

		if (fromCardIndex < 0 || fromCardIndex >= fromPileSize)
			return false;

		switch (from.type)
		{
		case PileType::Tableau:
			if (!isMovableSequence(from.index, fromCardIndex))
				return false;
			outInfo.fromCardIndex = fromCardIndex;
			outInfo.numCards = fromPileSize - fromCardIndex;
			return true;
		case PileType::Foundation:
		case PileType::TarotFoundation:
		case PileType::PokerSlot:
			if (fromCardIndex != fromPileSize - 1)
				return false;
			outInfo.fromCardIndex = fromCardIndex;
			outInfo.numCards = 1;
			return true;
		default:
			return false;
		}
	}

	int State::getMaxTableauMoveNum(PileRef to) const
	{
		int pokerSlotCapacity = mPokerSlot.isNone() ? 1 : 0;
		int emptyTableaus = 0;
		for (int index = 0; index < TableauNum; ++index)
		{
			if (index == to.index)
				continue;

			if (mTableaus[index].empty())
				++emptyTableaus;
		}

		int maxNum = pokerSlotCapacity + 1;
		for (int index = 0; index < emptyTableaus; ++index)
			maxNum *= 2;

		return maxNum;
	}

	bool State::isTopOnlyPile(PileType type) const
	{
		return type == PileType::Foundation || type == PileType::TarotFoundation;
	}

	bool State::getPreviousTopOnlyCard(PileRef ref, Card const& card, Card& outCard) const
	{
		switch (ref.type)
		{
		case PileType::Foundation:
			if (!card.isStandard() || card.getFaceRank() <= Card::eACE)
				return false;

			outCard = Card(card.getSuit(), card.getFaceRank() - 1);
			return true;
		case PileType::TarotFoundation:
			if (!card.isTarot())
				return false;

			if (ref.index == 0)
			{
				if (card.getTarotIndex() <= 0)
					return false;

				outCard = Card::Tarot(card.getTarotIndex() - 1);
				return true;
			}

			if (ref.index == 1)
			{
				if (card.getTarotIndex() >= Card::TarotCardNum - 1)
					return false;

				outCard = Card::Tarot(card.getTarotIndex() + 1);
				return true;
			}
			return false;
		default:
			return false;
		}
	}

	bool State::canMoveCardToPile(Card const& card, Card const& topCard, int numCards, PileRef to) const
	{
		switch (to.type)
		{
		case PileType::Tableau:
			{
				TArray<Card> const& toPile = getPile(to);
				if (toPile.empty())
					return true;

				Card const& bottom = toPile.back();
				return IsAdjacentSameFamily(bottom, card) || IsAdjacentSameFamily(bottom, topCard);
			}
		case PileType::Foundation:
			{
				if (!mPokerSlot.isNone())
					return false;

				if (numCards != 1 || !card.isStandard())
					return false;
				if (to.index != int(card.getSuit()))
					return false;

				if (mFoundations[to.index].isNone())
					return card.getFace() == Card::eACE;

				return mFoundations[to.index].getSuit() == card.getSuit() &&
				       mFoundations[to.index].getFaceRank() + 1 == card.getFaceRank();
			}
		case PileType::PokerSlot:
			{
				if (numCards != 1 || (!card.isStandard() && !card.isTarot()))
					return false;

				return mPokerSlot.isNone();
			}
		case PileType::TarotFoundation:
			{
				if (numCards != 1 || !card.isTarot())
					return false;
				if (to.index == 0)
					return card.getTarotIndex() == getTarotFoundationProgress(to.index);

				if (to.index == 1)
					return card.getTarotIndex() == Card::TarotCardNum - 1 - getTarotFoundationProgress(to.index);

				return false;
			}
		default:
			return false;
		}
	}

	bool State::shouldReverseMove(PileRef from, PileRef to, int fromCardIndex, int numCards) const
	{
		if (from.type != PileType::Tableau || to.type != PileType::Tableau)
			return false;

		TArray<Card> const& toPile = getPile(to);
		if (numCards <= 1)
			return true;

		if (toPile.empty())
			return bFastMove;

		Card const& targetTop = toPile.back();
		Card const& firstCard = getPileCard(from, fromCardIndex);
		Card const& lastCard = getPileCard(from, fromCardIndex + numCards - 1);
		return !IsAdjacentSameFamily(targetTop, firstCard) && IsAdjacentSameFamily(targetTop, lastCard);
	}

	bool State::canRelayReverseMove(PileRef from, PileRef to, int fromCardIndex, int numCards) const
	{
		if (numCards <= 0)
			return false;

		if (!bFastMove || from.type != PileType::Tableau || to.type != PileType::Tableau)
			return false;

		TArray<Card> const& toPile = getPile(to);
		if (toPile.empty())
			return false;

		bool bHasEmptyTableau = false;
		for (int index = 0; index < TableauNum; ++index)
		{
			if (index != to.index && mTableaus[index].empty())
			{
				bHasEmptyTableau = true;
				break;
			}
		}

		if (!bHasEmptyTableau)
			return false;

		Card const& firstCard = getPileCard(from, fromCardIndex);
		return IsAdjacentSameFamily(toPile.back(), firstCard);
	}

	void State::moveChecked(PileRef from, PileRef to, int numCards, bool bReverse)
	{
		int fromPileSize = getPileSize(from);

		int fromCardIndex = fromPileSize - numCards;

		CHECK(numCards > 0);
		CHECK(fromCardIndex >= 0);
		CHECK(fromCardIndex + numCards <= fromPileSize);

		auto lastCard = bReverse ? getPileCard(from, fromCardIndex) : getPileCard(from, fromPileSize - 1);

		if (isTopOnlyPile(to.type))
		{
			if (to.type == PileType::Foundation)
			{
				mFoundations[to.index] = lastCard;
			}
			else
			{
				mTarotFoundations[to.index] = lastCard;
				mTarotFoundationDrawOrder[to.index] = Math::Max(mTarotFoundationDrawOrder[0], mTarotFoundationDrawOrder[1]) + 1;
			}
		}
		else if (to.type == PileType::PokerSlot)
		{
			mPokerSlot = lastCard;
		}
		else
		{
			TArray<Card>& toPile = getPile(to);
			if (bReverse)
			{
				for (int index = fromPileSize - 1; index >= fromCardIndex; --index)
					toPile.push_back(getPileCard(from, index));
			}
			else
			{
				for (int index = fromCardIndex; index < fromPileSize; ++index)
					toPile.push_back(getPileCard(from, index));
			}
		}

		if (isTopOnlyPile(from.type))
		{
			if (from.type == PileType::Foundation)
				mFoundations[from.index] = Card::None();
			else
				mTarotFoundations[from.index] = Card::None();

			Card previousCard;
			if (getPreviousTopOnlyCard(from, lastCard, previousCard))
			{
				if (from.type == PileType::Foundation)
					mFoundations[from.index] = previousCard;
				else
					mTarotFoundations[from.index] = previousCard;
			}
		}
		else if (from.type == PileType::PokerSlot)
		{
			mPokerSlot = Card::None();
		}
		else
		{
			TArray<Card>& fromPile = getPile(from);
			fromPile.erase(fromPile.begin() + fromCardIndex, fromPile.end());
		}

	}

	void Level::moveChecked(PileRef from, PileRef to, int numCards, bool bRecord, bool bReverse, bool bAutoMove)
	{
		int fromPileSize = getPileSize(from);

		int fromCardIndex = fromPileSize - numCards;

		CHECK(numCards > 0);
		CHECK(fromCardIndex >= 0);
		CHECK(fromCardIndex + numCards <= fromPileSize);

		if (bRecord)
		{
			MoveRecord record;
			record.from = from;
			record.to = to;
			record.numCards = numCards;
			record.bReverse = bReverse;
			record.bAutoMove = bAutoMove;
			record.tarotDrawOrder = mTarotFoundationDrawOrder;
			mUndoStack.push_back(record);
			++mMoveCount;
		}

		State::moveChecked(from, to, numCards, bReverse);
	}

}//namespace Poker::FortuneFoundation
