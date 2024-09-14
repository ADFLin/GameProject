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


    FORCEINLINE int ToIndex(Vec2i const& pos)
	{
		return pos.x + BoardSize * pos.y;
	}

	FORCEINLINE int ToIndex(int x, int y)
	{
		return x + BoardSize * y;
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

		Block const& getBlock(int x, int y) const { return mBlockDatas[ToIndex(x,y)]; }

		bool canPlay(int x, int y, EBlockSymbol playerSymbol, uint8 playDir)
		{
			if (!CanPlayPos(x, y))
				return false;
			auto const& block = getBlock(x, y);
			if (block.symbol == EBlockSymbol::Empty)
				return true;

			if (block.symbol == playerSymbol)
			{
				if (mRule == ERule::TwoPlayers)
					return true;

				return block.dir == playDir;
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

		static bool CanPlayPos(int x, int y)
		{
			return (x == 0 || x == BoardSize - 1) || (y == 0 || y == BoardSize - 1);
		}

		static int GetOffset(uint8 moveDir)
		{
			switch (moveDir)
			{
			case 0: return -1;
			case 1: return -BoardSize;
			case 2: return 1;
			case 3: return BoardSize;
			}
			NEVER_REACH("GetOffset");
		}

		void play(int x, int y, EBlockSymbol playerSymbol, uint8 playDir, uint8 moveDir)
		{
			CHECK(canPlay(x, y, playerSymbol, playDir));

			Block block = getBlock(x,y);
			block.symbol = playerSymbol;
			int offset = GetOffset(moveDir);

			int index;
			int indexOffset;
			switch (moveDir)
			{
			case 0: index = x; indexOffset = -1; break;
			case 1: index = y; indexOffset = -BoardSize; break;
			case 2: index = x; indexOffset = 1; break;
			case 3: index = y; indexOffset = BoardSize; break;
			}
			Block* pBlock = &mBlockDatas[ToIndex(x, y)];
			if (offset > 0)
			{
				for (int i = index; i < BoardSize - 1; ++i)
				{
					Block* nextBlock = pBlock + indexOffset;
					*pBlock = *nextBlock;
					pBlock = nextBlock;
				}
			}
			else
			{
				for (int i = index; i > 0; --i)
				{
					Block* nextBlock = pBlock + indexOffset;
					*pBlock = *nextBlock;
					pBlock = nextBlock;
				}
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

			Vec2i screenSize = ::Global::GetScreenSize();
			mWorldToScreen = RenderTransform2D::LookAt(screenSize, 0.5 * Vector2(BoardSize, BoardSize), Vector2(0, 1), screenSize.y / float(BoardSize + 2), true);


			startGame();

			return true;
		}

		void onUpdate(long time) override
		{
			float dt = time / 1000.0f;
			mTweener.update(dt);
		}

		struct Sprite
		{
			Vector2 offset = Vector2::Zero();
			int color = EColor::Null;
		};
		float mOffsetAlpha = 0.0f;
		Sprite mSprite[BoardSize * BoardSize];

		bool bAnimating = false;
		float mAnimDuration;
		float mAnimTime;

		Tween::GroupTweener<float> mTweener;

		void clearAnimOffset()
		{
			mOffsetAlpha = 0;
			for (int i = 0; i < BoardSize * BoardSize; ++i)
				mSprite[i].offset = Vector2::Zero();
		}

		void clearSpriteColor()
		{
			for (int i = 0; i < BoardSize * BoardSize; ++i)
				mSprite[i].color = EColor::Null;
		}

		void playMoveAnim(Vec2i const& pos, uint8 moveDir)
		{
			clearAnimOffset();

			Vector2 offset;
			int index;
			int indexOffset;
			switch (moveDir)
			{
			case 0: offset = Vector2(1, 0); index = pos.x; indexOffset = -1; break;
			case 1: offset = Vector2(0, 1); index = pos.y; indexOffset = -BoardSize; break;
			case 2: offset = Vector2(-1, 0);  index = pos.x; indexOffset = 1; break;
			case 3: offset = Vector2(0, -1); index = pos.y; indexOffset = BoardSize; break;
			}

			Sprite* sprite = &mSprite[pos.x + BoardSize * pos.y];
			sprite->offset = Vector2(-100000, -100000);

			if (indexOffset > 0)
			{
				for (int i = index + 1; i < BoardSize; ++i)
				{
					sprite += indexOffset;
					sprite->offset = offset;
				}
			}
			else
			{
				for (int i = index - 1; i >= 0; --i)
				{
					sprite += indexOffset;
					sprite->offset = offset;
				}
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

			RenderUtility::SetPen(g, EColor::Black);
			RenderUtility::SetBrush(g, EColor::White);


			PlayerInfo const& player = mPlayers[mIndexPlay];
			for (int y = 0; y < BoardSize; ++y)
			{
				for (int x = 0; x < BoardSize; ++x)
				{
					int brushColor = EColor::White;

					auto& sprite = mSprite[BoardSize * y + x];
					if (sprite.color != EColor::Null)
						brushColor = sprite.color;

					RenderUtility::SetBrush(g, brushColor);
					Vector2 offset = mOffsetAlpha * sprite.offset;
					g.pushXForm();
					g.translateXForm(offset.x, offset.y);

					g.drawRect(Vector2(x, y), Vector2(1, 1));

					auto const& block = mGame.getBlock(x, y);
					if (block.symbol == EBlockSymbol::A)
					{

						float len = 0.5;
						Vector2 rPos = Vector2(x, y) + 0.5 * (Vector2(1, 1) - Vector2(len, len));
						RenderUtility::SetBrush(g, EColor::Null);
						g.setPenWidth(5);
						g.drawRect(rPos, Vector2(len,len));
						g.setPenWidth(1);

					}
					else if (block.symbol == EBlockSymbol::B)
					{
	
						float len = 0.25;
						RenderUtility::SetBrush(g, EColor::Null);
						g.setPenWidth(5);
						Vector2 rPos = Vector2(x, y) + 0.5 * Vector2(1, 1);
#if 0
						g.drawCircle(rPos, len);
#else
						g.drawLine(rPos - Vector2(len, len), rPos + Vector2(len, len));
						g.drawLine(rPos - Vector2(len, -len), rPos + Vector2(len, -len));
#endif
						g.setPenWidth(1);
					}

					g.popXForm();
				}
			}

			g.popXForm();
			g.endRender();
		}

		Coroutines::ExecutionHandle mGameHandle;
		EBlockSymbol mWinner = EBlockSymbol::Empty;


		void startGame()
		{
			mGameHandle = Coroutines::Start([this]()
			{
				gameEntry();
			});
		}

		void gameEntry()
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
				for (int y = 0; y < BoardSize; ++y)
				{
					for (int x = 0; x < BoardSize; ++x)
					{
						if (mGame.canPlay(x, y, player.symbol, player.dir))
						{
							mSprite[ToIndex(Vec2i(x, y))].color = EColor::Yellow;
						}

					}
				}

				do
				{
					clickPos = CO_YEILD_RETRUN(Vec2i, nullptr);
				} 
				while (!mGame.canPlay(clickPos.x, clickPos.y, player.symbol, player.dir));

				mSelectedBlockPos = clickPos;


				mStep = EStep::SelectMovePos;
				clearSpriteColor();
				mSprite[ToIndex(mSelectedBlockPos)].color = EColor::Red;

				Vec2i movePosList[4];
				uint8 moveDirList[4];
				int   numMove = 0;

				for (uint8 dir = 0; dir < 4; ++dir)
				{
					if (mGame.CanMove(clickPos.x, clickPos.y, dir))
					{
						Vec2i movePos = clickPos;
						mGame.MoveToPos(movePos.x, movePos.y, dir);

						movePosList[numMove] = movePos;
						moveDirList[numMove] = dir;
						mSprite[ToIndex(movePos)].color = EColor::Yellow;
						++numMove;
					}
				}

				uint moveDir = -1;
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
						}
					}
					if ( moveDir != -1 )
						break;
				}

				if ( moveDir == -1 )
					continue;

				mStep = EStep::ProcPlay;
				clearSpriteColor();
				playMoveAnim(mSelectedBlockPos, moveDir);
				CO_YEILD(nullptr);

				mGame.play(mSelectedBlockPos.x, mSelectedBlockPos.y, player.symbol, player.dir, moveDir);

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
						if ( mGame.mBlockDatas[line.index].symbol != mWinner )
							continue;

						int indexOffset;
						switch (line.type)
						{
						case 0: indexOffset = 1; break;
						case 1: indexOffset = BoardSize; break;
						case 2: indexOffset = BoardSize + 1; break;
						case 3: indexOffset = BoardSize - 1; break;
						}

						Sprite* sprite = &mSprite[line.index];
						for (int n = 0; n < BoardSize; ++n)
						{
							sprite->color = EColor::Red;
							sprite += indexOffset;
						}
					}
					break;
				}

				if (mGame.mRule == ERule::TwoPlayers)
					mIndexPlay = 1 - mIndexPlay;
				else
					mIndexPlay = (mIndexPlay + 1) % 4;
			}

			LogMsg("GameOver player %s win", (mWinner == EBlockSymbol::A) ? "A" : "B");
		}

		struct PlayerInfo
		{
			EBlockSymbol symbol;
			uint8 dir;
		};

		PlayerInfo mPlayers[4];
		int mIndexPlay = 0;

		enum class EStep
		{
			SelectBlock,
			SelectMovePos,
			ProcPlay,
		};

		Vec2i mSelectedBlockPos;

		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (msg.onLeftDown())
			{
				Vector2 worldPos = mWorldToScreen.transformInvPosition(msg.getPos());
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
		Game mGame;
	};

	REGISTER_STAGE_ENTRY("Quixo", GameStage, EExecGroup::Dev);
}