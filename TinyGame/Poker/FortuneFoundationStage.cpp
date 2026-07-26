#include "TinyGamePCH.h"
#include "FortuneFoundationStage.h"

#include "DrawEngine.h"
#include "CommonWidgets.h"
#include "FortuneFoundationSolver.h"
#include "GameGUISystem.h"
#include "RenderUtility.h"
#include "StageRegister.h"
#include "Widget/WidgetUtility.h"

#include <utility>
#include <chrono>
#include <cstdio>
#include "ProfileSystem.h"

namespace Poker::FortuneFoundation
{
	namespace
	{
		Vec2i const FortuneFoundationScreenSize(1024, 720);
		int const TableauGapX = 14;
		int const TableauOffsetY = 24;
		Vec2i const TableauStartPos(38, 184);
		int const FoundationGapX = TableauGapX;
		Vec2i const FoundationStartPos(647, 48);
		Vec2i const PokerSlotPos(560, 48);
		int const TarotFoundationY = 48;
		float const MoveAnimBaseTime = 160.0f;
		float const MoveAnimDistFactor = 0.32f;

		bool IsInRect(Vec2i const& pos, Vec2i const& rectPos, Vec2i const& rectSize)
		{
			return rectPos.x <= pos.x && pos.x < rectPos.x + rectSize.x &&
			       rectPos.y <= pos.y && pos.y < rectPos.y + rectSize.y;
		}

		char const* GetPileTypeName(State::PileType type)
		{
			switch (type)
			{
			case State::PileType::Tableau: return "Tableau";
			case State::PileType::Foundation: return "Foundation";
			case State::PileType::TarotFoundation: return "TarotFoundation";
			case State::PileType::PokerSlot: return "PokerSlot";
			case State::PileType::None: return "None";
			default: return "Unknown";
			}
		}

		std::string FormatPileRef(State::PileRef ref)
		{
			char buffer[64];
			std::snprintf(buffer, sizeof(buffer), "%s[%d]", GetPileTypeName(ref.type), ref.index);
			return buffer;
		}

		std::string FormatCard(Card const& card)
		{
			char buffer[32];
			if (card.isNone())
			{
				return "None";
			}
			if (card.isTarot())
			{
				std::snprintf(buffer, sizeof(buffer), "T%d", card.getTarotIndex());
				return buffer;
			}
			if (card.isStandard())
			{
				static char const SuitChars[] = { 'C', 'D', 'H', 'S' };
				std::snprintf(buffer, sizeof(buffer), "%c%s", SuitChars[card.getSuit()], Card::ToString(card.getFace()));
				return buffer;
			}
			return "?";
		}
	}

	bool LevelStage::onInit()
	{
		DrawEngine& drawEngine = ::Global::GetDrawEngine();
		mSavedScreenSize = drawEngine.getScreenSize();
		if (mSavedScreenSize.x < FortuneFoundationScreenSize.x ||
		    mSavedScreenSize.y < FortuneFoundationScreenSize.y)
		{
			drawEngine.changeScreenSize(
				Math::Max(mSavedScreenSize.x, FortuneFoundationScreenSize.x),
				Math::Max(mSavedScreenSize.y, FortuneFoundationScreenSize.y));
			mbScreenSizeChanged = true;
		}

		::Global::GUI().cleanupWidget();
		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addButton("Restart", [this]()
		{
			restart();
		});
		frame->addButton("New Game", [this]()
		{
			newGame();
		});
		frame->addButton("Undo", [this]()
		{
			if (undoMove())
				syncNextTarotFoundationDrawOrder();
		});
		frame->addButton("Solve", [this]()
		{
			solve(true);
		});
		frame->addButton("Check Solution", [this]()
		{
			solve(false);
		});
		frame->addCheckBox("Fast Move", bFastMove);
		frame->addCheckBox("Play Animation", bPlayAnimation);

		setupSolveControlFrame();

		mSeed = -2036482128;

		restart();
		return true;
	}

	void LevelStage::onEnd()
	{
		cancelSolveTask(true, true);

		if (mbScreenSizeChanged)
		{
			::Global::GetDrawEngine().changeScreenSize(mSavedScreenSize.x, mSavedScreenSize.y);
			mbScreenSizeChanged = false;
		}

		BaseClass::onEnd();
	}

	void LevelStage::setupSolveControlFrame()
	{
		Vec2i frameSize(128, 46);
		Vec2i buttonSize(34, 30);
		int gap = 8;
		mSolveControlFrame = new GFrame(UI_ANY, GUISystem::calcScreenCenterPos(frameSize) + Vec2i(0, 300), frameSize, nullptr);
		::Global::GUI().addWidget(mSolveControlFrame);

		int buttonsWidth = 3 * buttonSize.x + 2 * gap;
		Vec2i buttonPos((frameSize.x - buttonsWidth) / 2, (frameSize.y - buttonSize.y) / 2);
		mSolvePlayPauseButton = new ExecButton(UI_ANY, buttonPos, buttonSize, mSolveControlFrame);
		mSolvePlayPauseButton->bStep = true;
		mSolvePlayPauseButton->isExecutingFunc = [this]()
		{
			return mbSolving && !mbSolvePaused;
		};
		mSolvePlayPauseButton->onEvent = [this](int, GWidget*) -> bool
		{
			toggleSolvePlayback();
			return false;
		};

		buttonPos.x += buttonSize.x + gap;
		mSolveStepButton = new ExecButton(UI_ANY, buttonPos, buttonSize, mSolveControlFrame);
		mSolveStepButton->isExecutingFunc = []()
		{
			return false;
		};
		mSolveStepButton->onEvent = [this](int, GWidget*) -> bool
		{
			stepSolvePlayback();
			return false;
		};

		buttonPos.x += buttonSize.x + gap;
		mStopSolveButton = new CloseButton(UI_ANY, buttonPos, buttonSize, mSolveControlFrame);
		mStopSolveButton->onEvent = [this](int, GWidget*) -> bool
		{
			stopSolving();
			return false;
		};

		mSolveControlFrame->show(false, true);
	}

	void LevelStage::onUpdate(GameTimeSpan deltaTime)
	{
		pollSolveTask();

		if (!mMoveAnim.bPlaying)
			return;

		mMoveAnim.elapsed += float(long(deltaTime));
		if (mMoveAnim.elapsed >= mMoveAnim.duration)
			finishMoveAnimation();
	}

	void LevelStage::onRender(float dFrame)
	{
		IGraphics2D& g = ::Global::GetIGraphics2D();
		g.beginRender();

		g.setPen(Color3ub(54, 32, 27));
		g.setBrush(Color3ub(64, 37, 31));
		g.drawRect(Vec2i(0, 0), ::Global::GetScreenSize());

		Vec2i pokerSlotPos = getPokerSlotPos();
		drawPileFrame(g, pokerSlotPos, "P");
		Card const& pokerSlot = getPokerSlot();
		if (!pokerSlot.isNone())
		{
			if (!isAnimatingCard(PileType::PokerSlot, 0, 0))
			{
				g.setBrush(Color3ub(255, 255, 255));
				mCardDraw->draw(g, pokerSlotPos, pokerSlot);
			}
		}

		bool const bFoundationLocked = !pokerSlot.isNone();
		for (int index = 0; index < FoundationNum; ++index)
		{
			char const* suitLabels[] = { "C", "D", "H", "S" };
			Vec2i pos = getFoundationPos(index);
			drawPileFrame(g, pos, suitLabels[index]);
			Card const& card = getFoundation(index);
			if (!card.isNone())
			{
				if (!isAnimatingCard(PileType::Foundation, index, 0))
				{
					g.setBrush(Color3ub(255, 255, 255));
					mCardDraw->draw(g, pos, card);
				}
			}
			if (bFoundationLocked)
			{
				g.beginBlend(pos, mCardSize, 0.45f);
				g.setBrush(Color3ub(30, 20, 18));
				RenderUtility::SetPen(g, EColor::Null);
				g.drawRect(pos, mCardSize);
				g.endBlend();
			}
		}

		drawTarotGuideArrows(g);

		int firstTarotFoundation = 0;
		int secondTarotFoundation = 1;
		if (getTarotFoundationDrawOrder(0) > getTarotFoundationDrawOrder(1))
			std::swap(firstTarotFoundation, secondTarotFoundation);
		drawTarotFoundation(g, firstTarotFoundation, false);
		drawTarotFoundation(g, secondTarotFoundation, false);
		drawTarotFoundation(g, firstTarotFoundation, true);
		drawTarotFoundation(g, secondTarotFoundation, true);

		for (int pileIndex = 0; pileIndex < TableauNum; ++pileIndex)
		{
			Vec2i basePos = getTableauPos(pileIndex);
			drawTableauSlotFrame(g, basePos);


			auto const& pile = getTableau(pileIndex);
			for (int cardIndex = 0; cardIndex < int(pile.size()); ++cardIndex)
			{
				if (isAnimatingCard(PileType::Tableau, pileIndex, cardIndex))
					continue;

				Vec2i pos = basePos + Vec2i(0, cardIndex * TableauOffsetY);

				g.setBrush(Color3ub(255, 255, 255));
				mCardDraw->draw(g, pos, pile[cardIndex]);
			}
		}

		drawSelection(g);
		drawMoveAnimation(g);

		if (mbWin)
		{
			Vec2i screenSize = ::Global::GetScreenSize();
			g.setBrush(Color3ub(30, 20, 20));
			g.setPen(Color3ub(238, 204, 126), 2);
			Vec2i boxSize(260, 70);
			Vec2i pos = (screenSize - boxSize) / 2;
			g.drawRoundRect(pos, boxSize, Vec2i(8, 8));
			g.setTextColor(Color3ub(255, 240, 190));
			g.drawText(pos, boxSize, "You Win!", EHorizontalAlign::Center);
		}

		g.setTextColor(Color3ub(238, 204, 126));
		
		g.drawText(Vec2i(16, 8), InlineString<>::Make("Fortune Foundation : %d", mSeed));
		g.drawText(Vec2i(16, 24), "LClick: select/move  DClick: auto  R/U: restart/undo");
		if (!mSolveStatus.empty())
			g.drawText(Vec2i(16, 40), mSolveStatus.c_str());

		g.endRender();
	}

	MsgReply LevelStage::onMouse(MouseMsg const& msg)
	{
		if (mMoveAnim.bPlaying)
			return BaseClass::onMouse(msg);

		if (msg.onLeftDClick())
		{
			clearSolveMoves();
			tryAutoMove(hitTest(msg.getPos()));
			mSelection = PileRef();
			mSelectionCardIndex = INDEX_NONE;
		}
		else if (msg.onLeftDown())
		{
			clearSolveMoves();
			int cardIndex = INDEX_NONE;
			PileRef ref = hitTest(msg.getPos(), &cardIndex);
			if (ref.type == PileType::Tableau && !getTableau(ref.index).empty() && cardIndex == INDEX_NONE)
				cardIndex = int(getTableau(ref.index).size()) - 1;
			bool bForceSingleTableauMove = msg.isControlDown();
			if (!ref.isValid())
			{
				mSelection = PileRef();
				mSelectionCardIndex = INDEX_NONE;
			}
			else if (mSelection.isValid())
			{
				if (mSelection.type == ref.type && mSelection.index == ref.index)
				{
					mSelection = ref.type == PileType::Tableau && !getTableau(ref.index).empty() ? ref : PileRef();
					mSelectionCardIndex = mSelection.isValid() ? cardIndex : INDEX_NONE;
				}
				else if (moveCard(mSelection, ref, bForceSingleTableauMove))
				{
					mSelection = PileRef();
					mSelectionCardIndex = INDEX_NONE;
				}
				else if (ref.type == PileType::Tableau && !getTableau(ref.index).empty())
				{
					mSelection = ref;
					mSelectionCardIndex = cardIndex;
				}
				else
				{
					mSelection = PileRef();
					mSelectionCardIndex = INDEX_NONE;
				}
			}
			else
			{
				if (getPileSize(ref) > 0)
				{
					mSelection = ref;
					mSelectionCardIndex = cardIndex;
				}
			}
		}

		return BaseClass::onMouse(msg);
	}

	MsgReply LevelStage::onKey(KeyMsg const& msg)
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R:
				restart();
				break;
			case EKeyCode::U:
			case EKeyCode::Z:
				clearSolveMoves();
				if (!mMoveAnim.bPlaying && undoMove())
				{
					syncNextTarotFoundationDrawOrder();
					mbWin = false;
				}
				break;
			default:
				break;
			}
		}
		return BaseClass::onKey(msg);
	}

	bool LevelStage::onWidgetEvent(int event, int id, GWidget* ui)
	{
		return true;
	}

	void LevelStage::setupCardDraw(ICardDraw* cardDraw)
	{
		mCardDraw = cardDraw;
		if (mCardDraw)
			mCardSize = mCardDraw->getSize();
		restart();
	}

	void LevelStage::restart()
	{
		clearSolveMoves();
		setupGame(mSeed);
		syncNextTarotFoundationDrawOrder();
		mSelection = PileRef();
		mMoveAnim = MoveAnimation();
		mbWin = false;
	}

	void LevelStage::newGame()
	{
		clearSolveMoves();
		mSeed = (int)GenerateRandSeed();
		restart();
	}

	LevelStage::PileRef LevelStage::hitTest(Vec2i const& pos, int* outCardIndex) const
	{
		if (outCardIndex)
			*outCardIndex = INDEX_NONE;

		if (IsInRect(pos, getPokerSlotPos(), mCardSize))
			return PileRef{ PileType::PokerSlot, 0 };

		for (int index = 0; index < FoundationNum; ++index)
		{
			if (IsInRect(pos, getFoundationPos(index), mCardSize))
				return PileRef{ PileType::Foundation, index };
		}

		for (int index = 0; index < TarotFoundationNum; ++index)
		{
			Vec2i leftPos = getTarotFoundationPos(0);
			Vec2i rightPos = getTarotFoundationPos(1);
			int splitX = (leftPos.x + rightPos.x) / 2;
			Vec2i hitPos = index == 0 ? leftPos : Vec2i(splitX, leftPos.y);
			Vec2i hitSize = index == 0 ? Vec2i(splitX - leftPos.x, mCardSize.y) :
			                              Vec2i(rightPos.x + mCardSize.x - splitX, mCardSize.y);
			if (IsInRect(pos, hitPos, hitSize))
				return PileRef{ PileType::TarotFoundation, index };
		}

		for (int pileIndex = 0; pileIndex < TableauNum; ++pileIndex)
		{
			auto const& pile = getTableau(pileIndex);
			Vec2i pilePos = getTableauPos(pileIndex);
			int height = Math::Max(mCardSize.y, (int(pile.size()) - 1) * TableauOffsetY + mCardSize.y);
			if (!IsInRect(pos, pilePos, Vec2i(mCardSize.x, height)))
				continue;

			if (pile.empty())
				return PileRef{ PileType::Tableau, pileIndex };

			for (int cardIndex = int(pile.size()) - 1; cardIndex >= 0; --cardIndex)
			{
				Vec2i cardPos = pilePos + Vec2i(0, cardIndex * TableauOffsetY);
				if (IsInRect(pos, cardPos, mCardSize))
				{
					if (outCardIndex)
						*outCardIndex = cardIndex;
					return PileRef{ PileType::Tableau, pileIndex };
				}
			}
		}

		return PileRef();
	}

	Vec2i LevelStage::getTableauPos(int index) const
	{
		return TableauStartPos + Vec2i(index * (mCardSize.x + TableauGapX), 0);
	}

	Vec2i LevelStage::getFoundationPos(int index) const
	{
		return FoundationStartPos + Vec2i(index * (mCardSize.x + FoundationGapX), 0);
	}

	Vec2i LevelStage::getPokerSlotPos() const
	{
		return PokerSlotPos;
	}

	Vec2i LevelStage::getTarotFoundationPos(int index) const
	{
		int tableauIndex = index == 0 ? 0 : 4;
		return Vec2i(getTableauPos(tableauIndex).x, TarotFoundationY);
	}

	Vec2i LevelStage::getTarotFoundationCardPos(int tarotIndex) const
	{
		int distance = getTarotFoundationPos(1).x - getTarotFoundationPos(0).x;
		return getTarotFoundationPos(0) + Vec2i(distance * tarotIndex / (Card::TarotCardNum - 1), 0);
	}

	Vec2i LevelStage::getCardPos(PileRef ref, int cardIndex) const
	{
		switch (ref.type)
		{
		case PileType::Tableau:
			if (cardIndex == INDEX_NONE)
				cardIndex = Math::Max(0, getPileSize(ref) - 1);
			return getTableauPos(ref.index) + Vec2i(0, cardIndex * TableauOffsetY);
		case PileType::Foundation:
			return getFoundationPos(ref.index);
		case PileType::PokerSlot:
			return getPokerSlotPos();
		case PileType::TarotFoundation:
			{
				int pileSize = getPileSize(ref);
				if (cardIndex == INDEX_NONE)
					cardIndex = pileSize - 1;
				if (0 <= cardIndex && cardIndex < pileSize)
				{
					Card const& card = getPileCard(ref, cardIndex);
					if (card.isTarot())
						return getTarotFoundationCardPos(card.getTarotIndex());
				}
				return getTarotFoundationPos(ref.index);
			}
		default:
			return Vec2i(0, 0);
		}
	}

	Vec2i LevelStage::getMoveDestCardPos(PileRef to, int order, int numCards) const
	{
		switch (to.type)
		{
		case PileType::Tableau:
			return getTableauPos(to.index) + Vec2i(0, (int(getTableau(to.index).size()) + order) * TableauOffsetY);
		case PileType::Foundation:
			return getFoundationPos(to.index);
		case PileType::PokerSlot:
			return getPokerSlotPos();
		case PileType::TarotFoundation:
			{
				int progress = getTarotFoundationProgress(to.index);
				int tarotIndex = to.index == 0 ? progress + order :
				                                 Card::TarotCardNum - 1 - progress - order;
				return getTarotFoundationCardPos(Math::Clamp(tarotIndex, 0, Card::TarotCardNum - 1));
			}
		default:
			return Vec2i(0, 0);
		}
	}

	bool LevelStage::isAnimatingCard(PileType type, int pileIndex, int cardIndex) const
	{
		if (!mMoveAnim.bPlaying)
			return false;

		if (mMoveAnim.from.type != type || mMoveAnim.from.index != pileIndex)
			return false;

		return mMoveAnim.fromCardIndex <= cardIndex &&
		       cardIndex < mMoveAnim.fromCardIndex + mMoveAnim.numCards;
	}

	bool LevelStage::moveCard(PileRef from, PileRef to, bool bForceSingleTableauMove, bool bUseExactMove, bool bAutoMove)
	{
		int fromPileSize = getPileSize(from);
		if (fromPileSize <= 0)
			return false;

		MoveInfo moveInfo;
		int fromCardIndex = fromPileSize - 1;
		int numCards = 0;
		if (bForceSingleTableauMove)
		{
			if (from.type != PileType::Tableau || to.type != PileType::Tableau)
				return false;
			if (from.index == to.index)
				return false;

			if (!canMoveWithCardIndex(from, to, fromCardIndex, moveInfo))
				return false;

			fromCardIndex = moveInfo.fromCardIndex;
			numCards = 1;
		}
		else
		{
			if (bUseExactMove)
			{
				if (!canMoveWithCardIndex(from, to, fromCardIndex, moveInfo))
					return false;
			}
			else if (!canMove(from, to, moveInfo))
			{
				return false;
			}

			fromCardIndex = moveInfo.fromCardIndex;
			numCards = moveInfo.numCards;
		}

		if (numCards <= 0)
			return false;


		bool bReverse = !bForceSingleTableauMove && shouldReverseMove(from, to, fromCardIndex, numCards);

		if (bPlayAnimation)
		{
			startMoveAnimation(from, to, fromCardIndex, numCards, bReverse, bAutoMove);
		}
		else
		{
			moveChecked(from, to, numCards, true, bReverse, bAutoMove);
			handleMoveCompleted(to, false);
		}

		return true;
	}

	void LevelStage::startMoveAnimation(PileRef from, PileRef to, int fromCardIndex, int numCards, bool bReverse, bool bAutoMove)
	{
		mMoveAnim = MoveAnimation();
		mMoveAnim.bPlaying = true;
		mMoveAnim.from = from;
		mMoveAnim.to = to;
		mMoveAnim.fromCardIndex = fromCardIndex;
		mMoveAnim.numCards = numCards;
		mMoveAnim.bReverse = bReverse;
		mMoveAnim.bAutoMove = bAutoMove;
		mMoveAnim.cards.reserve(numCards);

		float maxDist2 = 0.0f;
		for (int i = 0; i < numCards; ++i)
		{
			int srcIndex = fromCardIndex + i;
			int dstOrder = mMoveAnim.bReverse ? numCards - 1 - i : i;
			Vector2 startPos(getCardPos(from, srcIndex));
			Vector2 endPos(getMoveDestCardPos(to, dstOrder, numCards));
			maxDist2 = Math::Max(maxDist2, (endPos - startPos).length2());

			AnimCard animCard;
			animCard.card = getPileCard(from, srcIndex);
			animCard.from = startPos;
			animCard.to = endPos;
			mMoveAnim.cards.push_back(animCard);
		}

		mMoveAnim.duration = MoveAnimBaseTime + MoveAnimDistFactor * Math::Sqrt(maxDist2);
	}

	void LevelStage::handleMoveCompleted(PileRef to, bool bAdvanceSolvePlayback)
	{
		if (to.type == PileType::TarotFoundation)
			mTarotFoundationDrawOrder[to.index] = mNextTarotFoundationDrawOrder++;
		else
			syncNextTarotFoundationDrawOrder();

		mbWin = isWin();
		if (mbSolving)
		{
			if (mbWin)
			{
				mbSolving = false;
				mbSolvePaused = false;
				updateSolveControlState();
			}
			else if (mbSolvePaused)
			{
				mSolveStatus = "Solve paused";
				updateSolveControlState();
			}
			else if (bAdvanceSolvePlayback && !startNextSolveMove())
			{
				mbSolving = false;
				mbSolvePaused = false;
				updateSolveControlState();
			}
		}
		else if (!mbWin)
		{
			startNextAutoMove();
		}
	}

	void LevelStage::finishMoveAnimation()
	{
		if (!mMoveAnim.bPlaying)
			return;

		MoveAnimation anim = mMoveAnim;
		mMoveAnim = MoveAnimation();
		moveChecked(anim.from, anim.to, anim.numCards, true, anim.bReverse, anim.bAutoMove);
		handleMoveCompleted(anim.to, true);
	}

	void LevelStage::drawMoveAnimation(IGraphics2D& g) const
	{
		if (!mMoveAnim.bPlaying)
			return;

		float alpha = Math::Clamp(mMoveAnim.elapsed / mMoveAnim.duration, 0.0f, 1.0f);
		float ease = 1.0f - (1.0f - alpha) * (1.0f - alpha);

		g.setBrush(Color3ub(255, 255, 255));
		auto drawAnimCard = [&](AnimCard const& animCard)
		{
			Vector2 pos;
			if (mMoveAnim.numCards > 1)
			{
				float dx = Math::Abs(animCard.to.x - animCard.from.x);
				float dy = Math::Abs(animCard.to.y - animCard.from.y);
				float drop = Math::Max(float(mCardSize.y), dx * 0.35f + dy * 0.5f);
				Vector2 p1 = animCard.from + Vector2(0, drop);
				Vector2 p2 = animCard.to + Vector2(0, drop);
				float invEase = 1.0f - ease;
				pos = animCard.from * (invEase * invEase * invEase) +
				      p1 * (3.0f * invEase * invEase * ease) +
				      p2 * (3.0f * invEase * ease * ease) +
				      animCard.to * (ease * ease * ease);
			}
			else
			{
				pos = animCard.from + (animCard.to - animCard.from) * ease;
			}
			mCardDraw->draw(g, Vec2i(pos), animCard.card);
		};

		if (mMoveAnim.bReverse && alpha > 0.5f)
		{
			for (int index = int(mMoveAnim.cards.size()) - 1; index >= 0; --index)
				drawAnimCard(mMoveAnim.cards[index]);
		}
		else
		{
			for (AnimCard const& animCard : mMoveAnim.cards)
				drawAnimCard(animCard);
		}
	}

	void LevelStage::drawPileFrame(IGraphics2D& g, Vec2i const& pos, char const* label) const
	{
		g.setPen(Color3ub(170, 110, 48), 2);
		g.setBrush(Color3ub(48, 27, 25));
		g.drawRoundRect(pos, mCardSize, Vec2i(7, 7));
		if (label && label[0])
		{
			g.setTextColor(Color3ub(220, 170, 88));
			g.drawText(pos, mCardSize, label, EHorizontalAlign::Center);
		}
	}

	void LevelStage::drawTableauSlotFrame(IGraphics2D& g, Vec2i const& pos) const
	{
		Color3ub frameColor(138, 91, 45);
		Vec2i size = mCardSize - 2 * Vec2i(4, 4);
		Vec2i rPos = pos + Vec2i(4, 4);
		Vec2i markSize(4, 4);

		g.setPen(frameColor, 1);
		g.setBrush(Color3ub(34, 27, 21));
		g.drawRect(rPos, size);

		g.setPen(frameColor, 1);
		g.drawLine(rPos + Vec2i(2, 0), rPos + Vec2i(size.x - 2, 0));
		g.drawLine(rPos + Vec2i(2, size.y - 1), rPos + Vec2i(size.x - 2, size.y - 1));
		g.drawLine(rPos + Vec2i(0, 2), rPos + Vec2i(0, size.y - 2));
		g.drawLine(rPos + Vec2i(size.x - 1, 2), rPos + Vec2i(size.x - 1, size.y - 2));

		Vec2i marks[] =
		{
			rPos,
			rPos + Vec2i(size.x - 1, 0),
			rPos + Vec2i(0, size.y - 1),
			rPos + Vec2i(size.x - 1, size.y - 1),
			rPos + Vec2i(0, size.y / 2),
			rPos + Vec2i(size.x - 1, size.y / 2),
		};

		g.setBrush(frameColor);
		RenderUtility::SetPen(g, EColor::Null);
		for (Vec2i const& mark : marks)
		{
			Vector2 center(mark);
			Vector2 points[4] =
			{
				center + Vector2(0, -markSize.y),
				center + Vector2(markSize.x, 0),
				center + Vector2(0, markSize.y),
				center + Vector2(-markSize.x, 0),
			};
			g.drawPolygon(points, 4);
		}
	}

	void LevelStage::drawTarotFoundation(IGraphics2D& g, int index, bool bTopCardOnly) const
	{
		Vec2i basePos = getTarotFoundationPos(index);
		char const* label = index == 0 ? "0" : "21";
		int displaySize = getTarotFoundationProgress(index);
		if (displaySize > 0 && isAnimatingCard(PileType::TarotFoundation, index, 0))
			--displaySize;

		if (displaySize == 0)
		{
			if (!bTopCardOnly)
				drawPileFrame(g, basePos, label);
			return;
		}

		if (!bTopCardOnly)
			drawPileFrame(g, basePos, label);

		int countShow = displaySize;
		int startIndex = bTopCardOnly ? countShow - 1 : 0;
		int endIndex = bTopCardOnly ? countShow : countShow - 1;
		for (int i = startIndex; i < endIndex; ++i)
		{
			int sequenceIndex = displaySize - countShow + i;
			int tarotIndex = index == 0 ? sequenceIndex : Card::TarotCardNum - 1 - sequenceIndex;
			Card card = Card::Tarot(tarotIndex);
			Vec2i pos = getTarotFoundationCardPos(tarotIndex);
			g.setBrush(Color3ub(255, 255, 255));
			mCardDraw->draw(g, pos, card);
		}
	}

	void LevelStage::drawTarotGuideArrows(IGraphics2D& g) const
	{
		Color3ub arrowColor(186, 120, 54);
		Vec2i leftPos = getTarotFoundationPos(0);
		Vec2i rightPos = getTarotFoundationPos(1);
		int startX = leftPos.x + mCardSize.x + 28;
		int endX = rightPos.x - 28;
		int topY = leftPos.y + 32;
		int bottomY = leftPos.y + 68;
		int shaftHeight = 5;

		g.setPen(arrowColor, 1);
		g.setBrush(arrowColor);

		g.drawRect(Vec2i(startX, topY), Vec2i(endX - startX, shaftHeight));
		Vector2 topHead[3] =
		{
			Vector2(float(startX - 14), float(topY + shaftHeight / 2)),
			Vector2(float(startX + 2), float(topY - 9)),
			Vector2(float(startX + 2), float(topY + 14)),
		};
		g.drawPolygon(topHead, 3);
		for (int i = 0; i < 4; ++i)
		{
			int x = endX - 8 - i * 8;
			Vector2 feather[3] =
			{
				Vector2(float(x), float(topY + shaftHeight / 2)),
				Vector2(float(x + 7), float(topY - 7)),
				Vector2(float(x + 5), float(topY + shaftHeight / 2)),
			};
			g.drawPolygon(feather, 3);
		}

		g.drawRect(Vec2i(startX, bottomY), Vec2i(endX - startX, shaftHeight));
		Vector2 bottomHead[3] =
		{
			Vector2(float(endX + 14), float(bottomY + shaftHeight / 2)),
			Vector2(float(endX - 2), float(bottomY - 9)),
			Vector2(float(endX - 2), float(bottomY + 14)),
		};
		g.drawPolygon(bottomHead, 3);
		for (int i = 0; i < 4; ++i)
		{
			int x = startX + 8 + i * 8;
			Vector2 feather[3] =
			{
				Vector2(float(x), float(bottomY + shaftHeight / 2)),
				Vector2(float(x - 7), float(bottomY + 14)),
				Vector2(float(x - 5), float(bottomY + shaftHeight / 2)),
			};
			g.drawPolygon(feather, 3);
		}
	}

	void LevelStage::drawSelection(IGraphics2D& g) const
	{
		if (!mSelection.isValid())
			return;

		Vec2i pos = getCardPos(mSelection, mSelectionCardIndex);
		Vec2i border(4, 4);
		g.setPen(Color3ub(255, 224, 96), 3);
		RenderUtility::SetBrush(g, EColor::Null);
		g.drawRoundRect(pos - border, mCardSize + 2 * border, Vec2i(9, 9));
	}

	bool LevelStage::tryAutoMove(PileRef ref)
	{
		if (!ref.isValid())
			return false;

		int fromPileSize = getPileSize(ref);
		if (fromPileSize == 0)
			return false;

		int fromCardIndex = fromPileSize - 1;
		Card const& card = getPileCard(ref, fromCardIndex);
		if (card.isTarot())
		{
			if (moveCard(ref, PileRef{ PileType::TarotFoundation, 0 }, false, false, true))
				return true;

			return moveCard(ref, PileRef{ PileType::TarotFoundation, 1 }, false, false, true);
		}

		if (card.isStandard())
			return moveCard(ref, PileRef{ PileType::Foundation, int(card.getSuit()) }, false, false, true);

		return false;
	}

	bool LevelStage::startNextAutoMove()
	{
		if (mMoveAnim.bPlaying)
			return false;

		for (int pileIndex = 0; pileIndex < TableauNum; ++pileIndex)
		{
			TArray<Card> const& pile = getTableau(pileIndex);
			if (pile.empty())
				continue;

			PileRef ref{ PileType::Tableau, pileIndex };
			if (tryAutoMove(ref))
				return true;
		}

		return false;
	}

	void LevelStage::clearSolveMoves()
	{
		cancelSolveTask(false, true);
		mbSolving = false;
		mbSolvePaused = false;
		mbSolveStepRequested = false;
		mSolveMoves.clear();
		mSolveMoveIndex = 0;
		mSolveStatus.clear();
		updateSolveControlState();
	}

	void LevelStage::stopSolving()
	{
		if (mbSolveTaskRunning)
		{
			mbIgnoreSolveResult = true;
			mbReportCancelledSolve = true;
			mbCancelSolve.store(true, std::memory_order_relaxed);
			mSolveStatus = "Stopping solve...";
			updateSolveControlState();
			return;
		}

		mbSolving = false;
		mbSolvePaused = false;
		mbSolveStepRequested = false;
		mSolveMoves.clear();
		mSolveMoveIndex = 0;
		mMoveAnim = MoveAnimation();
		mSolveStatus = "Solve playback stopped";
		updateSolveControlState();
	}

	void LevelStage::solve(bool bExecMove, bool bStepMode)
	{
		if (mMoveAnim.bPlaying)
			return;

		if (mbSolveTaskRunning)
		{
			mSolveStatus = "Solving...";
			return;
		}

		TIME_SCOPE("Solve Request");

		clearSolveMoves();
		mSelection = PileRef();
		mSelectionCardIndex = INDEX_NONE;
		mSolveStatus = "Solving...";
		mbSolveStepRequested = bStepMode;

		State solveState = static_cast<State const&>(*this);
		Solver::Config config;
		config.cancelFlag = &mbCancelSolve;
		mbCancelSolve.store(false, std::memory_order_relaxed);
		mbSolveTaskRunning = true;
		mbIgnoreSolveResult = false;
		mbReportCancelledSolve = false;
		mbExecuteSolveResult = bExecMove;
		updateSolveControlState();

		mSolveFuture = std::async(std::launch::async, [solveState, config]() mutable
		{
			TIME_SCOPE("Solve");
			Solver solver;
			return solver.solve(solveState, config);
		});
	}

	void LevelStage::pollSolveTask()
	{
		if (!mbSolveTaskRunning || !mSolveFuture.valid())
			return;

		if (mSolveFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
			return;

		Solver::Result result = mSolveFuture.get();
		mbSolveTaskRunning = false;
		mbCancelSolve.store(false, std::memory_order_relaxed);

		if (mbIgnoreSolveResult)
		{
			mbIgnoreSolveResult = false;
			if (mbReportCancelledSolve)
			{
				mSolveStatus = "Solve cancelled";
				mbReportCancelledSolve = false;
			}
			updateSolveControlState();
			return;
		}

		finishSolveTask(std::move(result));
		updateSolveControlState();
	}

	void LevelStage::cancelSolveTask(bool bWait, bool bIgnoreResult)
	{
		if (!mbSolveTaskRunning)
			return;

		if (bIgnoreResult)
		{
			mbIgnoreSolveResult = true;
			mbReportCancelledSolve = false;
		}

		mbCancelSolve.store(true, std::memory_order_relaxed);
		if (!bWait || !mSolveFuture.valid())
			return;

		Solver::Result result = mSolveFuture.get();
		mbSolveTaskRunning = false;
		mbCancelSolve.store(false, std::memory_order_relaxed);

		if (mbIgnoreSolveResult)
		{
			mbIgnoreSolveResult = false;
			mbReportCancelledSolve = false;
			return;
		}

		finishSolveTask(std::move(result));
	}

	void LevelStage::finishSolveTask(Solver::Result&& result)
	{
		double nodesPerSec = result.elapsedMS > 0.0 ? double(result.visitedCount) * 1000.0 / result.elapsedMS : 0.0;
		LogMsg("[FortuneFoundation] Solve seed=%d solved=%d aborted=%d cancelled=%d visited=%d generated=%d moves=%d elapsed=%.3fms nodesPerSec=%.1f maxOpen=%d maxDepth=%d invalidSolutions=%d exec=%d",
		       mSeed, result.bSolved ? 1 : 0, result.bAborted ? 1 : 0, result.bCancelled ? 1 : 0,
		       result.visitedCount, result.generatedCount, int(result.moves.size()), result.elapsedMS, nodesPerSec,
		       result.maxOpenNodes, result.maxVisitedDepth, result.invalidSolutionCount,
		       mbExecuteSolveResult ? 1 : 0);

		if (!result.bSolved)
		{
			mbSolveStepRequested = false;
			if (result.bCancelled)
				mSolveStatus = "Solve cancelled";
			else
				mSolveStatus = result.bAborted ? "Solve aborted" : "No solution";
			updateSolveControlState();
			return;
		}

		mSolveMoves.clear();
		mSolveMoveIndex = 0;
		mSolveMoves.reserve(result.moves.size());
		for (Solver::Move const& move : result.moves)
			mSolveMoves.push_back(PendingMove{ move.from, move.to, move.fromCardIndex, move.numCards });

		if (mSolveMoves.empty())
		{
			mbSolveStepRequested = false;
			mSolveStatus = "Already solved";
			mbWin = true;
			updateSolveControlState();
			return;
		}

		if (mbExecuteSolveResult)
		{
			mbSolving = true;
			mbSolvePaused = mbSolveStepRequested;
			mbSolveStepRequested = false;
			mSolveStatus = mbSolvePaused ? "Solve paused" : "Solving";
			updateSolveControlState();
			if (!startNextSolveMove(mbSolvePaused))
			{
				mbSolving = false;
				if (mSolveStatus.find("Invalid solve move") != 0)
				{
					mbSolvePaused = false;
					mSolveStatus = "Solve playback stopped";
				}
				updateSolveControlState();
			}
		}
		else
		{
			mbSolveStepRequested = false;
			mSolveStatus = "Have Solution";
			updateSolveControlState();
		}
	}

	void LevelStage::logInvalidSolveMove(PendingMove const& move, int moveIndex) const
	{
		int moveCount = int(mSolveMoves.size());
		int fromSize = move.from.isValid() ? getPileSize(move.from) : 0;
		int toSize = move.to.isValid() ? getPileSize(move.to) : 0;

		MoveInfo moveInfo;
		bool bHasMoveInfo = getMoveInfo(move.from, INDEX_NONE, moveInfo);
		bool bCanMove = false;
		MoveInfo checkedMoveInfo;
		if (bHasMoveInfo)
			bCanMove = canMove(move.from, move.to, checkedMoveInfo);

		LogWarning(0, "[FortuneFoundation] Invalid solver playback move step=%d/%d seed=%d fastMove=%d from=%s size=%d to=%s size=%d",
		           moveIndex + 1, moveCount, mSeed, bFastMove ? 1 : 0,
		           FormatPileRef(move.from).c_str(), fromSize,
		           FormatPileRef(move.to).c_str(), toSize);

		LogWarning(0, "[FortuneFoundation] Solver move: fromCardIndex=%d numCards=%d",
		           move.fromCardIndex, move.numCards);

		LogWarning(0, "[FortuneFoundation] Level check: getMoveInfo=%d canMove=%d fromCardIndex=%d numCards=%d",
		           bHasMoveInfo ? 1 : 0, bCanMove ? 1 : 0,
		           bHasMoveInfo ? moveInfo.fromCardIndex : INDEX_NONE,
		           bHasMoveInfo ? moveInfo.numCards : 0);

		if (move.fromCardIndex != INDEX_NONE && move.numCards > 0)
		{
			MoveInfo solverMoveInfo;
			solverMoveInfo.fromCardIndex = move.fromCardIndex;
			solverMoveInfo.numCards = move.numCards;
			bool bCanExactMove = canMoveWithInfo(move.from, move.to, solverMoveInfo);
			LogWarning(0, "[FortuneFoundation] Level exact solver move check: canMoveWithInfo=%d",
			           bCanExactMove ? 1 : 0);
		}

		if (fromSize > 0)
		{
			int topIndex = fromSize - 1;
			LogWarning(0, "[FortuneFoundation] From top card: index=%d card=%s",
			           topIndex, FormatCard(getPileCard(move.from, topIndex)).c_str());
		}

		if (toSize > 0)
		{
			int topIndex = toSize - 1;
			LogWarning(0, "[FortuneFoundation] To top card: index=%d card=%s",
			           topIndex, FormatCard(getPileCard(move.to, topIndex)).c_str());
		}
		else
		{
			LogWarning(0, "[FortuneFoundation] To pile is empty");
		}

		if (bHasMoveInfo && move.from.type == PileType::Tableau && move.to.type == PileType::Tableau)
		{
			bool bReverse = shouldReverseMove(move.from, move.to, moveInfo.fromCardIndex, moveInfo.numCards);
			bool bRelayReverse = canRelayReverseMove(move.from, move.to, moveInfo.fromCardIndex, moveInfo.numCards);
			LogWarning(0, "[FortuneFoundation] Tableau rule: reverse=%d relayReverse=%d maxMove=%d",
			           bReverse ? 1 : 0, bRelayReverse ? 1 : 0, getMaxTableauMoveNum(move.to));

			for (int index = moveInfo.fromCardIndex; index < fromSize; ++index)
			{
				LogWarning(0, "[FortuneFoundation] Move card[%d]=%s", index, FormatCard(getPileCard(move.from, index)).c_str());
			}
		}

		if (move.fromCardIndex != INDEX_NONE && move.numCards > 0)
		{
			for (int offset = 0; offset < move.numCards && move.fromCardIndex + offset < fromSize; ++offset)
			{
				int index = move.fromCardIndex + offset;
				LogWarning(0, "[FortuneFoundation] Solver card[%d]=%s", index, FormatCard(getPileCard(move.from, index)).c_str());
			}
		}
	}

	bool LevelStage::startNextSolveMove(bool bSingleStep)
	{
		if (mMoveAnim.bPlaying)
			return false;

		bool bMovedAny = false;
		while (mSolveMoveIndex < int(mSolveMoves.size()))
		{
			int moveIndex = mSolveMoveIndex;
			PendingMove const& move = mSolveMoves[moveIndex];
			if (!moveCard(move.from, move.to))
			{
				logInvalidSolveMove(move, moveIndex);
				mSolveStatus = "Invalid solve move " + std::to_string(moveIndex + 1) + "/" + std::to_string(mSolveMoves.size());
				mbSolving = false;
				mbSolvePaused = true;
				updateSolveControlState();
				return false;
			}

			++mSolveMoveIndex;

			bMovedAny = true;
			if (mMoveAnim.bPlaying)
				return true;

			if (mbWin)
			{
				mSolveStatus = "Solved";
				mbSolving = false;
				mbSolvePaused = false;
				updateSolveControlState();
				return true;
			}

			if (bSingleStep)
			{
				mSolveStatus = "Solve paused";
				updateSolveControlState();
				return true;
			}
		}

		mSolveStatus = isWin() ? "Solved" : "Solve playback finished";
		mbSolving = false;
		mbSolvePaused = false;
		updateSolveControlState();
		return bMovedAny;
	}

	bool LevelStage::hasPendingSolveMove() const
	{
		return mSolveMoveIndex < int(mSolveMoves.size());
	}

	void LevelStage::toggleSolvePlayback()
	{
		if (mbSolveTaskRunning)
		{
			mSolveStatus = "Solving...";
			return;
		}

		if (mbSolving && !mbSolvePaused)
		{
			mbSolvePaused = true;
			mSolveStatus = "Solve paused";
			updateSolveControlState();
			return;
		}

		if (hasPendingSolveMove())
		{
			mbSolving = true;
			mbSolvePaused = false;
			mSolveStatus = "Solving";
			updateSolveControlState();

			if (!mMoveAnim.bPlaying && !startNextSolveMove())
			{
				mbSolving = false;
				if (mSolveStatus.find("Invalid solve move") != 0)
				{
					mbSolvePaused = false;
					mSolveStatus = "Solve playback stopped";
				}
				updateSolveControlState();
			}
			return;
		}

		solve(true);
	}

	void LevelStage::stepSolvePlayback()
	{
		if (mbSolveTaskRunning || mMoveAnim.bPlaying)
			return;

		if (!hasPendingSolveMove())
		{
			solve(true, true);
			return;
		}

		mbSolving = true;
		mbSolvePaused = true;
		mSolveStatus = "Solve paused";
		updateSolveControlState();

		if (!startNextSolveMove(true))
		{
			mbSolving = false;
			if (mSolveStatus.find("Invalid solve move") != 0)
			{
				mbSolvePaused = false;
				mSolveStatus = "Solve playback stopped";
			}
			updateSolveControlState();
		}
	}

	void LevelStage::updateSolveControlState()
	{
		bool bHasPlayback = hasPendingSolveMove() || mbSolving || mbSolveTaskRunning || mMoveAnim.bPlaying;
		if (mSolveControlFrame)
			mSolveControlFrame->show(bHasPlayback && mbExecuteSolveResult, true);
		if (mSolvePlayPauseButton)
			mSolvePlayPauseButton->enable(!mbSolveTaskRunning);
		if (mSolveStepButton)
			mSolveStepButton->enable(!mbSolveTaskRunning && !mMoveAnim.bPlaying);
		if (mStopSolveButton)
			mStopSolveButton->enable(bHasPlayback);
	}

	void LevelStage::syncNextTarotFoundationDrawOrder()
	{
		mNextTarotFoundationDrawOrder = Math::Max(getTarotFoundationDrawOrder(0), getTarotFoundationDrawOrder(1)) + 1;
	}

}//namespace Poker::FortuneFoundation
