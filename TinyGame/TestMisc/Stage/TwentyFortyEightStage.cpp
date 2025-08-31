#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGraphics2D.h"

#include "Core/IntegerType.h"
#include "MarcoCommon.h"

#include "Async/Coroutines.h"
#include "Tween.h"
#include "RandomUtility.h"

namespace TwentyFortyEight
{
	class GameTimeControl
	{
	public:
		float time = 0;

		void update(float deltaTime)
		{
			time += deltaTime;
		}

		void reset()
		{
			time = 0;
		}
	};

	GameTimeControl GGameTime;

	class WaitForSeconds : public IYieldInstruction
	{
	public:
		WaitForSeconds(float duration)
		{
			time = GGameTime.time + duration;
		}

		float time;


		bool isKeepWaiting() const override
		{
			return time > GGameTime.time;
		}

	};


	constexpr int BoardSize = 4;


	FORCEINLINE int ToIndex(int x, int y)
	{
		return x + BoardSize * y;
	}

    FORCEINLINE int ToIndex(Vec2i const& pos)
	{
		return ToIndex(pos.x, pos.y);
	}

	Vector2 GetDirOffset(uint8 dir)
	{
		static const Vector2 StaticOffsets[] =
		{
			Vector2(1, 0), Vector2(0, 1), Vector2(-1, 0), Vector2(0, -1),
		};
		return StaticOffsets[dir];
	}


	struct GameState
	{
		int step;
		int score;
		int blockState[BoardSize * BoardSize];
	};

	class Game
	{
	public:

		int score;
		int step;

		void importState(GameState const& state)
		{
			step = state.step;
			score = state.score;
			for (int i = 0; i < BoardSize*BoardSize; ++i)
			{
				mBlocks[i].valueLevel = state.blockState[i];
			}
		}

		void exportState(GameState& state)
		{
			state.step = step;
			state.score = score;
			for (int i = 0; i < BoardSize*BoardSize; ++i)
			{
				state.blockState[i] = mBlocks[i].valueLevel;
			}
		}

		int getRandValueLevel()
		{
			float constexpr Weights[] = { 90, 10, 0 };
			float constexpr TotalWeight = [&]() constexpr
			{
				float totalWeight = 0.0;
				for (int i = 0; i < ARRAY_SIZE(Weights); ++i)
				{
					totalWeight += Weights[i];
				}
				return totalWeight;
			}();

			float w = TotalWeight * RandFloat();
			for (int level = 0; level < ARRAY_SIZE(Weights); ++level)
			{
				if (w <= Weights[level])
					return (level + 1);
				w -= Weights[level];

			}
			NEVER_REACH("");
			return 0;
		}

		int randomEmptyIndex()
		{
			int indices[BoardSize * BoardSize];
			int num = 0;
			for (int i = 0; i < BoardSize * BoardSize; ++i)
			{
				if (mBlocks[i].valueLevel == 0)
				{
					indices[num] = i;
					++num;
				}
			}

			if (num == 0)
				return INDEX_NONE;

			return indices[RandRange(0, num)];
		}

		void restart()
		{
			Block* pBlock = mBlocks;
			for (int i = 0; i < BoardSize * BoardSize; ++i)
			{
				pBlock->valueLevel = 0;
				++pBlock;
			}

			for (int i = 0; i < 2; ++i)
			{
				int index = randomEmptyIndex();
				CHECK(index != INDEX_NONE);
				mBlocks[index].valueLevel = getRandValueLevel();
			}
		}


		int spawnRandBlock()
		{
			int index = randomEmptyIndex();
			if (index != INDEX_NONE)
			{
				mBlocks[index].valueLevel = getRandValueLevel();
			}

			return index;
		}


		struct Block
		{
			uint8 valueLevel;
			int getValue() const
			{
				return valueLevel ? 1 << uint32(valueLevel) : 0;
			}
		};

		Block const& getBlock(Vec2i const& pos) const { return mBlocks[ToIndex(pos)]; }

		bool canPlay(uint8 dir) const
		{
			LineVisit const& visit = GetLineVisit(dir);

			int index = visit.index;
			for (int i = 0; i < BoardSize; ++i)
			{
				if (canPlayLine(index, visit.blockStride))
					return true;

				index += visit.lineStrde;
			}
			return false;
		}




		struct LineVisit
		{
			int index;
			int blockStride;
			int lineStrde;
		};

		static LineVisit const& GetLineVisit(uint8 dir)
		{
			static LineVisit constexpr StaticMap[] =
			{
				{ BoardSize - 1, -1, BoardSize},
				{ (BoardSize - 1) * BoardSize, -BoardSize, 1 },
				{ 0, 1 , BoardSize},
				{ 0, BoardSize,  1},
			};

			return StaticMap[dir];
		};

		bool canPlayLine(int index, int blockStride) const
		{
			int checkSize = BoardSize;
			for (; checkSize; --checkSize)
			{
				if (mBlocks[index].valueLevel != 0)
					break;

				index += blockStride;
			}

			if (checkSize)
			{
				if (checkSize != BoardSize)
					return true;

				int indexEmpty = INDEX_NONE;
				int indexNext = index + blockStride;
				for (--checkSize; checkSize; --checkSize)
				{
					if (mBlocks[indexNext].valueLevel == 0)
					{
						if (indexEmpty == INDEX_NONE)
							indexEmpty = indexNext;
					}
					else
					{
						if (indexEmpty != INDEX_NONE)
							return true;

						if (mBlocks[indexNext].valueLevel == mBlocks[index].valueLevel)
							return true;
					}

					index = indexNext;
					indexNext += blockStride;
				}
			}

			return false;
		}


		static bool IsVaildPos(int x, int y)
		{
			return 0 <= x && x < BoardSize && 0 <= y && y < BoardSize;
		}

		struct MoveInfo
		{
			int  fromIndex;
			int  toIndex;
			bool bMerged;
		};

		int getMoves(uint8 dir, MoveInfo outMoves[]) const
		{
			CHECK(canPlay(dir));
			LineVisit const& visit = GetLineVisit(dir);
			int result = 0;
			int index = visit.index;
			for (int i = 0; i < BoardSize; ++i)
			{
				result += getMovesLine(index, visit.blockStride, outMoves + result);
				index += visit.lineStrde;
			}
			return result;
		}
		int getMovesLine(int indexStart, int blockStride, MoveInfo outMoves[]) const
		{
			int index = indexStart;
			int result = 0;

			int checkSize = BoardSize;

			for (; checkSize; --checkSize)
			{
				if (mBlocks[index].valueLevel != 0)
					break;
				index += blockStride;
			}

			if (checkSize)
			{
				int indexEmpty = INDEX_NONE;
				int indexNext = index + blockStride;

				if (checkSize != BoardSize)
				{
					auto& move = outMoves[result++];
					move.fromIndex = index;
					move.toIndex = indexStart;
					move.bMerged = false;

					indexEmpty = indexStart + blockStride;
				}

				for (--checkSize; checkSize; --checkSize)
				{
					if (mBlocks[indexNext].valueLevel == 0)
					{
						if (indexEmpty == INDEX_NONE)
							indexEmpty = indexNext;
					}
					else
					{

						if (index != INDEX_NONE && mBlocks[indexNext].valueLevel == mBlocks[index].valueLevel)
						{
							if (result > 0)
							{
								auto& moveLast = outMoves[result - 1];
								if (moveLast.fromIndex == index)
								{
									index = moveLast.toIndex;
								}
							}

							auto& move = outMoves[result++];
							move.toIndex = index;
							move.fromIndex = indexNext;
							move.bMerged = true;

							index = INDEX_NONE;

							if (indexEmpty == INDEX_NONE)
								indexEmpty = indexNext;
						}
						else
						{
							if (indexEmpty != INDEX_NONE)
							{
								auto& move = outMoves[result++];
								move.toIndex = indexEmpty;
								move.fromIndex = indexNext;
								move.bMerged = false;

								indexEmpty += blockStride;
							}

							index = indexNext;
						}
					}

					indexNext += blockStride;
				}
			}

			return result;
		}

		void play(uint8 dir)
		{
			++step;

			CHECK(canPlay(dir));
			LineVisit const& visit = GetLineVisit(dir);
			int result = 0;
			int index = visit.index;
			for (int i = 0; i < BoardSize; ++i)
			{
				playLine(index, visit.blockStride);
				index += visit.lineStrde;
			}
		}

		void playLine(int indexStart, int blockStride)
		{
			int index = indexStart;
			int result = 0;

			int checkSize = BoardSize;

			for (; checkSize; --checkSize)
			{
				if (mBlocks[index].valueLevel != 0)
					break;
				index += blockStride;
			}

			if (checkSize)
			{
				int indexEmpty = INDEX_NONE;
				int indexNext = index + blockStride;

				if (checkSize != BoardSize)
				{
					mBlocks[indexStart].valueLevel = mBlocks[index].valueLevel;
					mBlocks[index].valueLevel = 0;
					index = indexStart;
					indexEmpty = indexStart + blockStride;
				}

				for (--checkSize; checkSize; --checkSize)
				{
					if (mBlocks[indexNext].valueLevel == 0)
					{
						if (indexEmpty == INDEX_NONE)
							indexEmpty = indexNext;
					}
					else
					{
						if (index != INDEX_NONE && mBlocks[indexNext].valueLevel == mBlocks[index].valueLevel)
						{
							mBlocks[index].valueLevel += 1;
							mBlocks[indexNext].valueLevel = 0;

							score += (1 << mBlocks[index].valueLevel);
							index = INDEX_NONE;

							if (indexEmpty == INDEX_NONE)
								indexEmpty = indexNext;
						}
						else
						{
							if (indexEmpty != INDEX_NONE)
							{
								mBlocks[indexEmpty].valueLevel = mBlocks[indexNext].valueLevel;
								mBlocks[indexNext].valueLevel = 0;
								index = indexEmpty;
								indexEmpty += blockStride;
							}
							else
							{
								index = indexNext;
							}
						}
					}

					indexNext += blockStride;
				}
			}
		}


		Block mBlocks[BoardSize*BoardSize];

	};

	using namespace Render;

	class GameStage : public StageBase
		            , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			::Global::GUI().cleanupWidget();

			auto frame = WidgetUtility::CreateDevFrame();

			frame->addButton("Restart", [this](int event, GWidget*) -> bool
			{
				restart();
				return false;
			});

			frame->addButton("Undo", [this](int event, GWidget*) -> bool
			{
				mGame.importState(mPrevState);
				return false;
			});

			frame->addButton("Test", [this](int event, GWidget*) -> bool
			{
				for (int i = 0; i < BoardSize * BoardSize; ++i)
				{
					int x = i % BoardSize;
					int y = i / BoardSize;
					if (y % 2)
					{
						mGame.mBlocks[ToIndex(BoardSize - x - 1, y)].valueLevel = i + 1;
					}
					else
					{
						mGame.mBlocks[i].valueLevel = i + 1;
					}
				}
				mGame.mBlocks[0].valueLevel = 2;
				return false;
			});

			frame->addCheckBox("Show Debug", bShowDeubg);
			frame->addCheckBox("Auto Play", bAutoPlay);
			Vec2i screenSize = ::Global::GetScreenSize();
			mWorldToScreen = RenderTransform2D::LookAt(screenSize, 0.5 * Vector2(BoardSize, BoardSize), Vector2(0, -1), screenSize.y / float(BoardSize + 2.2), false);
			mScreenToWorld = mWorldToScreen.inverse();

			startGame();
			return true;
		}


		void restart()
		{
			if (mGameHandle.isValid())
			{
				Coroutines::Stop(mGameHandle);
			}

			mTweener.clear();
			startGame();
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
			GGameTime.update(deltaTime);
			mTweener.update(deltaTime);
			Coroutines::ThreadContext::Get().checkAllExecutions();
		}

		struct Sprite
		{
			Vector2 offset = Vector2::Zero();
			int color = EColor::Null;
		};
		float mOffsetAlpha = 0.0f;
		Sprite mSprites[BoardSize * BoardSize];

		bool bAutoPlay = false;

		bool bAnimating = false;
		float mAnimDuration;
		float mAnimTime;
		bool bShowDeubg = true;

		uint mLastPlayDir = 0;
		GameState mPrevState;

		Tween::GroupTweener<float> mTweener;

		void clearAnimOffset()
		{
			mOffsetAlpha = 0;
			for(auto& sprite : mSprites)
				sprite.offset = Vector2::Zero();
		}

		void clearSpriteColor()
		{
			for (auto& sprite : mSprites)
				sprite.color = EColor::Null;
		}

		template< typename TFunc >
		void playMoveAnim(TArrayView<Game::MoveInfo const> moves, TFunc func)
		{
			clearAnimOffset();

			auto ToPos = [](int index) -> Vector2
			{
				int x = index % BoardSize;
				int y = index / BoardSize;
				return Vector2(x, y);
			};
			
			for (auto const& move : moves)
			{
				Sprite* sprite = &mSprites[move.fromIndex];
				sprite->offset = ToPos(move.toIndex) - ToPos(move.fromIndex);
			}

			mTweener.tweenValue< Easing::IOQuad >(mOffsetAlpha, 0.0f, 1.0f, 0.2).finishCallback(
				[this, func]()
				{
					mOffsetAlpha = 0;
					func();
				}
			);
		}

		void onRender(float dFrame) override
		{
			Vec2i screenSize = ::Global::GetScreenSize();

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.2, 0.2, 0.2, 1), 1);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();
			g.pushXForm();
			g.transformXForm(mWorldToScreen, true);

			RenderUtility::SetBrush(g, EColor::White);

			RenderUtility::SetFont(g, FONT_S24);
			
			g.setTextRemoveScale(true);
			g.setTextRemoveRotation(true);
			PlayerInfo const& player = mPlayers[mIndexPlay];


			auto ToRenderPos = [](int index) -> Vector2
			{
				int x = index % BoardSize;
				int y = index / BoardSize;
				return Vector2(x, y) + 0.5 * Vector2(1, 1);
			};

			float constexpr gap = 0.04;
			Color3ub const BlockColorMap[] =
			{
				Color3ub(214, 205, 196),
				Color3ub(238, 228, 218),
				Color3ub(236, 224, 202),
				Color3ub(242, 177, 121),
				Color3ub(245, 149, 101),
				Color3ub(247, 126, 105),
				Color3ub(246, 93, 59),
				Color3ub(237, 206, 113),
				Color3ub(238, 214, 144),
				Color3ub(236, 209, 120),
				Color3ub(238, 206, 107),
				Color3ub(237, 205, 96),
				Color3ub(64, 62, 53),
			};

			g.setBrush(Color3ub(155, 139, 124));
			g.drawRoundRect(-Vector2(gap, gap), Vector2(BoardSize, BoardSize) + 2 * Vector2(gap, gap), Vector2(0.4, 0.4));

			for (int y = 0; y < BoardSize; ++y)
			{
				for (int x = 0; x < BoardSize; ++x)
				{
					auto const& block = mGame.getBlock(Vec2i(x, y));

					Vector2 offset = Vector2(x, y);
					g.pushXForm();
					g.translateXForm(offset.x, offset.y);

					RenderUtility::SetPen(g, EColor::Null);
					g.setBrush(BlockColorMap[0]);
					g.drawRoundRect(Vector2(gap, gap), Vector2(1, 1) - 2 * Vector2(gap, gap), Vector2(0.2, 0.2));
					g.popXForm();
				}
			}

			Game::LineVisit const& visit = Game::GetLineVisit(mLastPlayDir);
			int indexLine = visit.index;
			for (int i = 0; i < BoardSize; ++i)
			{
				int index = indexLine;
				for (int n = 0; n < BoardSize; ++n)
				{
					int x = index % BoardSize;
					int y = index / BoardSize;

					auto const& block = mGame.getBlock(Vec2i(x, y));

					RenderUtility::SetPen(g, EColor::Null);

					if (block.valueLevel)
					{
						auto& sprite = mSprites[ToIndex(x, y)];

						Vector2 offset = Vector2(x, y) + mOffsetAlpha * sprite.offset;
						g.pushXForm();
						g.translateXForm(offset.x, offset.y);

						g.setBrush(BlockColorMap[Math::Min<int>(block.valueLevel, ARRAY_SIZE(BlockColorMap) - 1)]);
						g.drawRoundRect(Vector2(gap, gap), Vector2(1, 1) - 2 * Vector2(gap, gap), Vector2(0.2, 0.2));
						g.setTextColor(block.valueLevel > 2 ? Color3ub(249, 246, 241) : Color3ub(119, 110, 101));
						g.drawTextF(Vector2::Zero(), Vector2(1, 1), "%d", block.getValue());

						g.popXForm();
					}

					index += visit.blockStride;
				}
				indexLine += visit.lineStrde;
			}

			if (bShowDeubg)
			{
				RenderUtility::SetPen(g, EColor::Red);
				g.drawLine(Vector2(0, 0), Vector2(10, 0));
				RenderUtility::SetPen(g, EColor::Green);
				g.drawLine(Vector2(0, 0), Vector2(0, 10));

				int ColorMap[] = { EColor::Red , EColor::Green, EColor::Blue, EColor::Magenta, EColor::Brown, EColor::Orange };
				for (int i = 0; i < mNumMoves; ++i)
				{
					auto const& move = mDebugMoves[i];
					RenderUtility::SetPen(g, ColorMap[i % ARRAY_SIZE(ColorMap)]);
					g.drawLine(ToRenderPos(move.fromIndex), ToRenderPos(move.toIndex));
				}
			}

			g.setTextRemoveScale(false);
			g.setTextRemoveRotation(false);

			g.setTextColor(Color3ub(255, 255, 255));
			g.popXForm();
			g.drawTextF(Vector2(20, 20), "Score %d", mGame.score);
			g.drawTextF(Vector2(20, 60), "Step %d", mGame.step);
			g.endRender();
		}

		Coroutines::ExecutionHandle mGameHandle;

		void startGame()
		{
			mGameHandle = Coroutines::Start([this]()
			{
				for(;;)
				{
					gameRound();
#if 0
					if (bAutoPlay)
					{
						CO_YEILD(WaitForSeconds(1.0));
					}
					else
					{
						InlineString<> str;
						str.format("Player %s win ! Do you play again?", (mWinner == EBlockSymbol::A) ? "A" : "B");
						GWidget* widget = ::Global::GUI().showMessageBox(UI_ANY, str.c_str(), EMessageButton::YesNo);
						widget->onEvent = [this](int event, GWidget*)->bool
						{
							Coroutines::Resume(mGameHandle, bool(event == EVT_BOX_YES));
							return false;
						};
						bool bPlayAgain = CO_YEILD<bool>(nullptr);

						if (!bPlayAgain)
							break;
					}
#endif
				}
			});
		}

		void gameRound()
		{
			mGame.restart();
			mGame.exportState(mPrevState);
			for (;;)
			{
				mStep = EStep::SelectDir;

				uint8 inputDir;
				do 
				{
					inputDir = CO_YEILD<int>(nullptr);
				}
				while (!mGame.canPlay(inputDir));

				mStep = EStep::ProcPlay;

				mNumMoves = mGame.getMoves(inputDir, mDebugMoves);
				static bool bCall = false;
				bCall = false;
				playMoveAnim(TArrayView<Game::MoveInfo const>(mDebugMoves, mNumMoves), [&]()
				{
					CHECK(bCall == false);
					bCall = true;
					//CHECK(mStep == EStep::ProcPlay);
					Coroutines::Resume(mGameHandle);
				});

				CO_YEILD(nullptr);


				mGame.exportState(mPrevState);
				mGame.play(inputDir);
				int index = mGame.spawnRandBlock();

				if (index == INDEX_NONE)
					break;

				bool bOver = true;
				for (int i = 0; i < 4; ++i)
				{
					if (mGame.canPlay(i))
					{
						bOver = false;
						break;
					}

				}
				if ( bOver )
					break;
			}

		}


		Game::MoveInfo mDebugMoves[BoardSize * BoardSize];
		int mNumMoves = 0;

		PlayerInfo mPlayers[4];
		int mIndexPlay = 0;
		enum class EStep
		{
			SelectDir,
			ProcPlay,
		};


		struct PlayAction
		{
			Vec2i pos;
			uint8 moveDir;
		};

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (mStep == EStep::SelectDir && msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::D: Coroutines::Resume(mGameHandle, 0); break;
				case EKeyCode::S: Coroutines::Resume(mGameHandle, 1); break;
				case EKeyCode::A: Coroutines::Resume(mGameHandle, 2); break;
				case EKeyCode::W: Coroutines::Resume(mGameHandle, 3); break;
				}
			}

			return BaseClass::onKey(msg);
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}


		EStep mStep = EStep::SelectDir;

		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;
		Game mGame;
	};

	REGISTER_STAGE_ENTRY("Twenty Forty-Eight", GameStage, EExecGroup::Dev);
}