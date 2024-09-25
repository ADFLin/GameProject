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

namespace Quixo
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


	constexpr int BoardSize = 5;

	enum class ERule
	{
		TwoPlayers,
		FourPlayers,
	};

	enum class EBlockSymbol : uint8
	{
		Empty,
		A,
		B,
	};

	struct PlayerInfo
	{
		EBlockSymbol symbol;
		uint8 dir;
	};

    FORCEINLINE int ToIndex(Vec2i const& pos)
	{
		return pos.x + BoardSize * pos.y;
	}

	FORCEINLINE int ToIndex(int x, int y)
	{
		return x + BoardSize * y;
	}

	Vector2 GetDirOffset(uint8 dir)
	{
		static const Vector2 StaticOffsets[] =
		{
			Vector2(1, 0), Vector2(0, 1), Vector2(-1, 0), Vector2(0, -1),
		};
		return StaticOffsets[dir];
	}

	class Game
	{
	public:

		void restart()
		{
			Block* pBlock = mBlockDatas;
			for (int i = 0; i < BoardSize * BoardSize; ++i)
			{
				pBlock->symbol = EBlockSymbol::Empty;
				pBlock->dir = 0;
				++pBlock;
			}
		}

		struct Block
		{
			EBlockSymbol symbol;
			uint8 dir;
		};

		Block const& getBlock(Vec2i const& pos) const { return mBlockDatas[ToIndex(pos)]; }


		bool canPlay(PlayerInfo const& player, Vec2i const& pos) const
		{
			if (!CanPlayPos(pos.x, pos.y))
				return false;
			auto const& block = getBlock(pos);
			if (block.symbol == EBlockSymbol::Empty)
				return true;

			if (block.symbol == player.symbol)
			{
				if (mRule == ERule::TwoPlayers)
					return true;

				return block.dir == player.dir;
			}

			return false;
		}

		static bool CanMove(int x, int y, uint8 moveDir)
		{
			switch (moveDir)
			{
			case 0: return x != 0;
			case 1: return y != 0;
			case 2: return x != BoardSize - 1;
			case 3: return y != BoardSize - 1;
			}
			NEVER_REACH("moveDir error");
			return false;
		}

		static void MoveToPos(int& x, int& y, uint8 moveDir)
		{
			switch (moveDir)
			{
			case 0: x = 0; return;
			case 1: y = 0; return;
			case 2: x = BoardSize - 1; return;
			case 3: y = BoardSize - 1; return;
			}
			NEVER_REACH("MoveToPos");
		}

		static bool IsVaildPos(int x, int y)
		{
			return 0 <= x && x < BoardSize && 0 <= y && y < BoardSize;
		}
		static bool CanPlayPos(int x, int y)
		{
			if (!IsVaildPos(x, y))
				return false;
			return (x == 0 || x == BoardSize - 1) || (y == 0 || y == BoardSize - 1);
		}

		static int GetMoveInfo(Vec2i const& pos, uint8 moveDir, int& outIndexOffset)
		{
			switch (moveDir)
			{
			case 0: outIndexOffset = -1; return pos.x;
			case 1: outIndexOffset = -BoardSize; return pos.y;
			case 2: outIndexOffset = 1; return BoardSize - pos.x - 1;
			case 3: outIndexOffset = BoardSize; return BoardSize - pos.y - 1;
			}

			NEVER_REACH("GetMoveInfo");
			return 0;
		}

		void play(PlayerInfo const& player, Vec2i const& pos, uint8 moveDir)
		{
			CHECK(canPlay(player, pos));

			Block block = getBlock(pos);
			block.symbol = player.symbol;
			block.dir = player.dir;

			int indexOffset;
			int numMove = GetMoveInfo(pos, moveDir, indexOffset);
			Block* pBlock = &mBlockDatas[ToIndex(pos)];
			for (int i = 0; i < numMove; ++i)
			{
				Block* nextBlock = pBlock + indexOffset;
				*pBlock = *nextBlock;
				pBlock = nextBlock;
			}

			*pBlock = block;
		}

		struct Line
		{
			int index;
			int type;
		};

		uint8 checkResult(TArray<Line>& lines)
		{
			constexpr uint8 AllBits = BIT(uint8(EBlockSymbol::A)) | BIT(uint8(EBlockSymbol::B));
			uint8 result = 0;
			uint8 bit;
			for (int i = 0; i < BoardSize; ++i)
			{
				if (bit = checkResult(i*BoardSize, 1))
				{
					lines.push_back({ i*BoardSize, 0 });
					result |= bit;
				}
				if (bit = checkResult(i, BoardSize))
				{
					lines.push_back({ i, 1 });
					result |= bit;
				}
			}
			if (bit = checkResult(0, BoardSize + 1))
			{
				lines.push_back({ 0, 2 });
				result |= bit;
			}
			if (bit = checkResult(BoardSize - 1, BoardSize - 1))
			{
				lines.push_back({ BoardSize - 1, 3 });
				result |= bit;
			}
			return result;
		}

		uint8 checkResult(int index, int offset)
		{
			EBlockSymbol symbol = mBlockDatas[index].symbol;
			if (symbol == EBlockSymbol::Empty)
				return 0;

			for (int i = 1; i < BoardSize; ++i)
			{
				if (mBlockDatas[index + i * offset].symbol != symbol)
					return 0;
			}
			return BIT(uint8(symbol));
		}


		ERule mRule;
		union 
		{
			Block mBlocks[BoardSize][BoardSize];
			Block mBlockDatas[BoardSize*BoardSize];
		};

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
			Vec2i screenSize = ::Global::GetScreenSize();
			mWorldToScreen = RenderTransform2D::LookAt(screenSize, 0.5 * Vector2(BoardSize, BoardSize), Vector2(0, 1), screenSize.y / float(BoardSize + 2.5), true);
			mScreenToWorld = mWorldToScreen.inverse();

			mGame.mRule = ERule::FourPlayers;

			mWinCounts[0] = 0;
			mWinCounts[1] = 0;
			startGame();

			return true;
		}

		int mWinCounts[2];

		void restart()
		{
			if (mGameHandle.isValid())
			{
				Coroutines::Stop(mGameHandle);
			}

			mTweener.clear();
			startGame();
		}

		void onUpdate(long time) override
		{
			float dt = time / 1000.0f;
			GGameTime.update(dt);
			mTweener.update(dt);
			Coroutines::ThreadContext::Get().checkAllExecutions();
		}

		struct Sprite
		{
			Vector2 offset = Vector2::Zero();
			int color = EColor::Null;
		};
		float mOffsetAlpha = 0.0f;
		Sprite mSprites[BoardSize * BoardSize];

		bool bAutoPlay = true;

		bool bAnimating = false;
		float mAnimDuration;
		float mAnimTime;

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

		void playMoveAnim(Vec2i const& pos, uint8 moveDir)
		{
			clearAnimOffset();

			Vector2 offset = GetDirOffset(moveDir);

			int indexOffset;
			int numMove = Game::GetMoveInfo(pos, moveDir, indexOffset);
			Sprite* sprite = &mSprites[ToIndex(pos)];
			sprite->offset = Vector2(-100000, -100000);

			for (int i = 0; i < numMove; ++i)
			{
				sprite += indexOffset;
				sprite->offset = offset;
			}

			mTweener.tweenValue< Easing::IOQuad >(mOffsetAlpha, 0.0f, 1.0f, 0.2).finishCallback(
				[this]()
				{
					mOffsetAlpha = 0;
					Coroutines::Resume(mGameHandle);
				});
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
			g.transformXForm(mWorldToScreen, false);

			RenderUtility::SetBrush(g, EColor::White);


			PlayerInfo const& player = mPlayers[mIndexPlay];
			for (int y = 0; y < BoardSize; ++y)
			{
				for (int x = 0; x < BoardSize; ++x)
				{
					int brushColor = EColor::White;

					auto& sprite = mSprites[BoardSize * y + x];
					if (sprite.color != EColor::Null)
						brushColor = sprite.color;

					RenderUtility::SetBrush(g, brushColor);
					Vector2 offset = Vector2(x,y) + 0.5 * Vector2(1,1) + mOffsetAlpha * sprite.offset;
					g.pushXForm();
					g.translateXForm(offset.x, offset.y);


					RenderUtility::SetPen(g, EColor::Black);
					g.drawRoundRect(-Vector2(0.5,0.5), Vector2(1, 1), Vector2(0.4,0.4));

					auto const& block = mGame.getBlock(Vec2i(x, y));
					if (block.symbol == EBlockSymbol::A)
					{
#if 1
						float len = 0.25;
						RenderUtility::SetBrush(g, EColor::Null);
						g.setPenWidth(5);
						g.drawCircle(Vector2::Zero(), len);
						g.setPenWidth(1);
#else
						float len = 0.5;
						Vector2 rPos = -0.5 * Vector2(len, len);
						RenderUtility::SetBrush(g, EColor::Null);
						g.setPenWidth(5);
						g.drawRect(rPos, Vector2(len,len));
						g.setPenWidth(1);
#endif

					}
					else if (block.symbol == EBlockSymbol::B)
					 {	
						float len = 0.25;
						RenderUtility::SetBrush(g, EColor::Null);
						g.setPenWidth(5);
						g.drawLine(-Vector2(len, len), Vector2(len, len));
						g.drawLine(-Vector2(len, -len), Vector2(len, -len));
						g.setPenWidth(1);
					}

					if (mGame.mRule == ERule::FourPlayers && block.symbol != EBlockSymbol::Empty)
					{
						RenderUtility::SetBrush(g, EColor::Black);
						RenderUtility::SetPen(g, EColor::Null);
						g.drawCircle(0.38 * GetDirOffset(block.dir), 0.06);

					}
					g.popXForm();
				}
			}


			Vector2 offset = 0.5 * Vector2(BoardSize, BoardSize) + (0.5 * BoardSize + 0.5) * GetDirOffset(player.dir);

			int playerColors[] = { EColor::Cyan , EColor::Pink, EColor::Orange, EColor::Brown };
			RenderUtility::SetBrush(g, playerColors[mIndexPlay]);
			RenderUtility::SetPen(g, EColor::Black);
			g.drawCircle(offset, 0.25);

			g.popXForm();

			g.drawTextF(Vector2(20, 20), "A = %d , B = %d", mWinCounts[0], mWinCounts[1]);

			g.endRender();
		}

		Coroutines::ExecutionHandle mGameHandle;
		EBlockSymbol mWinner = EBlockSymbol::Empty;


		void startGame()
		{
			mGameHandle = Coroutines::Start([this]()
			{
				for(;;)
				{
					gameRound();

					if (mWinner == EBlockSymbol::A)
					{
						mWinCounts[0] += 1;
					}
					else
					{
						mWinCounts[1] += 1;
					}


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
						bool bPlayAgain = CO_YEILD_RETRUN(bool, nullptr);

						if (!bPlayAgain)
							break;
					}
				}
			});
		}

		void gameRound()
		{

			mGame.restart();

			if (mGame.mRule == ERule::TwoPlayers)
			{
				mPlayers[0] = { EBlockSymbol::A , 0 };
				mPlayers[1] = { EBlockSymbol::B , 2 };
			}
			else
			{
				mPlayers[0] = { EBlockSymbol::A , 0 };
				mPlayers[1] = { EBlockSymbol::B , 1 };
				mPlayers[2] = { EBlockSymbol::A , 2 };
				mPlayers[3] = { EBlockSymbol::B , 3 };
			}
			mWinner = EBlockSymbol::Empty;
			mIndexPlay = 0;

			for(;;)
			{
				PlayerInfo const& player = mPlayers[mIndexPlay];
				Vec2i clickPos;
				mStep = EStep::SelectBlock;

				clearSpriteColor();
				int playableCount = 0;
				for (int y = 0; y < BoardSize; ++y)
				{
					for (int x = 0; x < BoardSize; ++x)
					{
						if (mGame.canPlay(player, Vec2i(x, y)))
						{
							++playableCount;
							mSprites[ToIndex(x, y)].color = EColor::Yellow;
						}
					}
				}

				if (playableCount)
				{
					do
					{
						if (bAutoPlay)
						{
							int indexPlay = ::Global::Random() % playableCount;
							for (int y = 0; y < BoardSize; ++y)
							{
								for (int x = 0; x < BoardSize; ++x)
								{
									if (mSprites[ToIndex(x, y)].color == EColor::Yellow)
									{
										if (indexPlay == 0)
										{
											clickPos = Vec2i(x, y);
											goto End;
										}
										else
										{
											--indexPlay;
										}
									}
								}
							}
						End:;
						}
						else
						{
							clickPos = CO_YEILD_RETRUN(Vec2i, nullptr);
						}
					} while (!mGame.canPlay(player, clickPos));

					mSelectedBlockPos = clickPos;

					mStep = EStep::SelectMovePos;
					clearSpriteColor();
					mSprites[ToIndex(mSelectedBlockPos)].color = EColor::Red;

					Vec2i movePosList[4];
					uint8 moveDirList[4];
					int   numMove = 0;

					for (uint8 dir = 0; dir < 4; ++dir)
					{
						if (Game::CanMove(clickPos.x, clickPos.y, dir))
						{
							Vec2i movePos = clickPos;
							Game::MoveToPos(movePos.x, movePos.y, dir);

							movePosList[numMove] = movePos;
							moveDirList[numMove] = dir;
							mSprites[ToIndex(movePos)].color = EColor::Yellow;
							++numMove;
						}
					}

					uint8 moveDir = 0xff;
					if (bAutoPlay)
					{
						moveDir = moveDirList[::Global::Random() % numMove];
						CO_YEILD(WaitForSeconds(0.2));
					}
					else
					{
						for (;;)
						{

							clickPos = CO_YEILD_RETRUN(Vec2i, nullptr);
							if (clickPos.x == -1)
								break;

							for (int i = 0; i < numMove; ++i)
							{
								if (movePosList[i] == clickPos)
								{
									moveDir = moveDirList[i];
									break;
								}
							}
							if (moveDir != 0xff)
								break;
						}
					}

					if (moveDir == 0xff)
						continue;

					mStep = EStep::ProcPlay;
					clearSpriteColor();
					playMoveAnim(mSelectedBlockPos, moveDir);
					CO_YEILD(nullptr);

					mGame.play(player, mSelectedBlockPos, moveDir);

					TArray<Game::Line> lines;
					uint8 gameResult = mGame.checkResult(lines);

					if (gameResult)
					{
						if (gameResult & BIT(uint8(player.symbol)))
						{
							mWinner = player.symbol;
						}
						else
						{
							mWinner = (player.symbol == EBlockSymbol::A) ? EBlockSymbol::B : EBlockSymbol::A;
						}

						for (int i = 0; i < lines.size(); ++i)
						{
							auto const& line = lines[i];
							if (mGame.mBlockDatas[line.index].symbol != mWinner)
								continue;

							int indexOffset;
							switch (line.type)
							{
							case 0: indexOffset = 1; break;
							case 1: indexOffset = BoardSize; break;
							case 2: indexOffset = BoardSize + 1; break;
							case 3: indexOffset = BoardSize - 1; break;
							}

							Sprite* sprite = &mSprites[line.index];
							for (int n = 0; n < BoardSize; ++n)
							{
								sprite->color = EColor::Red;
								sprite += indexOffset;
							}
						}
						break;
					}
				}
				else
				{


				}

				if (mGame.mRule == ERule::TwoPlayers)
					mIndexPlay = 1 - mIndexPlay;
				else
					mIndexPlay = (mIndexPlay + 1) % 4;
			}


		}


		PlayerInfo mPlayers[4];
		int mIndexPlay = 0;

		enum class EStep
		{
			SelectBlock,
			SelectMovePos,
			ProcPlay,
		};


		struct PlayAction
		{
			Vec2i pos;
			uint8 moveDir;
		};

		Vec2i mSelectedBlockPos;


		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (msg.onLeftDown())
			{
				Vector2 worldPos = mScreenToWorld.transformPosition(msg.getPos());
				Vec2i blockPos;
				blockPos.x = Math::FloorToInt(Math::ToTileValue(worldPos.x, 1.0f));
				blockPos.y = Math::FloorToInt(Math::ToTileValue(worldPos.y, 1.0f));

				if (mStep == EStep::SelectBlock || mStep == EStep::SelectMovePos)
				{
					Coroutines::Resume(mGameHandle, blockPos);
				}
			}
			else if (msg.onRightDown())
			{
				if (mStep == EStep::SelectMovePos)
				{
					Coroutines::Resume(mGameHandle, Vec2i(-1,-1));
				}
			}
			return BaseClass::onMouse(msg);
		}


		EStep mStep = EStep::SelectBlock;

		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;
		Game mGame;
	};

	REGISTER_STAGE_ENTRY("Quixo", GameStage, EExecGroup::Dev);
}