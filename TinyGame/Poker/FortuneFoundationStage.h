#pragma once
#ifndef FortuneFoundationStage_H_D3933DE4_2D89_4891_9202_5C8D9F987FB4
#define FortuneFoundationStage_H_D3933DE4_2D89_4891_9202_5C8D9F987FB4

#include "StageBase.h"
#include "FortuneFoundation.h"
#include "FortuneFoundationSolver.h"
#include "CardDraw.h"

#include <atomic>
#include <future>
#include <string>
#include <vector>

class GButton;
class GFrame;
class ExecButton;
class CloseButton;

namespace Poker::FortuneFoundation
{
	class LevelStage : public StageBase
		             , private Level
		             , public ICardResourceSetup
	{
		using BaseClass = StageBase;
		using PileRef = Level::PileRef;
		using PileType = Level::PileType;
	public:
		bool onInit() override;
		void onEnd() override;
		void onUpdate(GameTimeSpan deltaTime) override;
		void onRender(float dFrame) override;
		MsgReply onMouse(MouseMsg const& msg) override;
		MsgReply onKey(KeyMsg const& msg) override;
		bool onWidgetEvent(int event, int id, GWidget* ui) override;

		void setupCardDraw(ICardDraw* cardDraw) override;

		void restart();
		void newGame();

		bool bPlayAnimation = true;

	private:
		enum
		{
			UI_NEW_GAME = BaseClass::NEXT_UI_ID,
			UI_UNDO,
			UI_SOLVE,
			NEXT_UI_ID,
		};

		struct PendingMove
		{
			PileRef from;
			PileRef to;
			int fromCardIndex = INDEX_NONE;
			int numCards = 0;
		};

		PileRef hitTest(Vec2i const& pos, int* outCardIndex = nullptr) const;
		Vec2i getTableauPos(int index) const;
		Vec2i getFoundationPos(int index) const;
		Vec2i getPokerSlotPos() const;
		Vec2i getTarotFoundationPos(int index) const;
		Vec2i getTarotFoundationCardPos(int tarotIndex) const;
		Vec2i getCardPos(PileRef ref, int cardIndex = INDEX_NONE) const;
		Vec2i getMoveDestCardPos(PileRef to, int order, int numCards) const;
		bool isAnimatingCard(PileType type, int pileIndex, int cardIndex) const;


		bool moveCard(PileRef from, PileRef to, bool bForceSingleTableauMove = false, bool bUseExactMove = false, bool bAutoMove = false);

		void startMoveAnimation(PileRef from, PileRef to, int fromCardIndex, int numCards, bool bReverse, bool bAutoMove);

		void handleMoveCompleted(PileRef to, bool bAdvanceSolvePlayback);
		void finishMoveAnimation();
		void drawMoveAnimation(IGraphics2D& g) const;
		void drawPileFrame(IGraphics2D& g, Vec2i const& pos, char const* label) const;
		void drawTableauSlotFrame(IGraphics2D& g, Vec2i const& pos) const;
		void drawTarotFoundation(IGraphics2D& g, int index, bool bTopCardOnly) const;
		void drawTarotGuideArrows(IGraphics2D& g) const;
		void drawSelection(IGraphics2D& g) const;
		bool tryAutoMove(PileRef ref);
		bool startNextAutoMove();
		void clearSolveMoves();
		void stopSolving();
		void solve(bool bExecMove, bool bStepMode = false);
		void pollSolveTask();
		void cancelSolveTask(bool bWait, bool bIgnoreResult);
		void finishSolveTask(Solver::Result&& result);
		void logInvalidSolveMove(PendingMove const& move, int moveIndex) const;
		bool startNextSolveMove(bool bSingleStep = false);
		bool hasPendingSolveMove() const;
		void setupSolveControlFrame();
		void toggleSolvePlayback();
		void stepSolvePlayback();
		void updateSolveControlState();
		void syncNextTarotFoundationDrawOrder();

		ICardDraw* mCardDraw = nullptr;
		Vec2i   mCardSize = Vec2i(73, 98);
		PileRef mSelection;
		int     mSelectionCardIndex = INDEX_NONE;
		int mSeed = 1;
		bool mbWin = false;
		Vec2i mSavedScreenSize = Vec2i(0, 0);
		bool mbScreenSizeChanged = false;

		GFrame* mSolveControlFrame = nullptr;
		ExecButton* mSolvePlayPauseButton = nullptr;
		ExecButton* mSolveStepButton = nullptr;
		CloseButton* mStopSolveButton = nullptr;
		int mNextTarotFoundationDrawOrder = 1;

		bool mbSolving = false;
		bool mbSolvePaused = false;
		TArray<PendingMove> mSolveMoves;
		int mSolveMoveIndex = 0;
		std::string mSolveStatus;
		std::future<Solver::Result> mSolveFuture;
		std::atomic_bool mbCancelSolve = false;
		bool mbSolveTaskRunning = false;
		bool mbIgnoreSolveResult = false;
		bool mbReportCancelledSolve = false;
		bool mbExecuteSolveResult = false;
		bool mbSolveStepRequested = false;

		struct AnimCard
		{
			Card card;
			Vector2 from;
			Vector2 to;
		};

		struct MoveAnimation
		{
			bool bPlaying = false;
			PileRef from;
			PileRef to;
			int fromCardIndex = INDEX_NONE;
			int numCards = 0;
			bool bReverse = false;
			bool bAutoMove = false;
			float elapsed = 0.0f;
			float duration = 180.0f;
			TArray<AnimCard> cards;
		};

		MoveAnimation mMoveAnim;
	};

}//namespace Poker::FortuneFoundation

#endif // FortuneFoundationStage_H_D3933DE4_2D89_4891_9202_5C8D9F987FB4
