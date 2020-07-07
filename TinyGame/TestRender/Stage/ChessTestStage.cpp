#include "stage/TestStageHeader.h"

#include "Chess/ChessCore.h"
#include "RHI/RHICommand.h"

#include <algorithm>
#include "RHI/RHIGraphics2D.h"

namespace Chess
{
	using namespace Render;


	struct BlendScope
	{
		BlendScope(RHIGraphics2D& g, float alpha)
			:g(g)
		{
			g.beginBlend(alpha);
		}

		~BlendScope()
		{
			g.endBlend();
		}
		RHIGraphics2D& g;
	};

	class TestStage : public StageBase
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		Game mGame;

		RHITexture2DRef mChessTex;

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			if (!::Global::GetDrawEngine().initializeRHI(RHITargetName::OpenGL))
				return false;

			VERIFY_RETURN_FALSE( mChessTex = RHIUtility::LoadTexture2DFromFile("texture/chess.png") );

			::Global::GUI().cleanupWidget();

			auto frame = WidgetUtility::CreateDevFrame();
			restart();
			return true;
		}

		void onEnd() override
		{
			::Global::GetDrawEngine().shutdownRHI(true);
			BaseClass::onEnd();
		}

		void restart() 
		{
			mGame.restart();
		}
		void tick() {}
		void updateFrame(int frame) {}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for (int i = 0; i < frame; ++i)
				tick();

			updateFrame(frame);
		}


		float TileLength = 70;

		struct ChessImageTileSet 
		{
			int width;
			int height;


		};

		void drawChess(RHIGraphics2D& g, Vector2 const& centerPos, EChess::Type type, EChessColor color)
		{
			struct ImageTileInfo
			{
				Vec2i pos;
				Vec2i size;
			};

#if 1
			int const w = 64;
			int const h = 64;
			static const ImageTileInfo ChessTiles[] =
			{
				{Vec2i(w * 1,0),Vec2i(w,h)},
				{Vec2i(w * 0,0),Vec2i(w,h)},
				{Vec2i(w * 4,0),Vec2i(w,h)},
				{Vec2i(w * 3,0),Vec2i(w,h)},
				{Vec2i(w * 2,0),Vec2i(w,h)},
				{Vec2i(w * 5,0),Vec2i(w,h)},
			};
			Vector2 pivot = Vector2(0.5, 0.5);
			float sizeScale = 0.90f;
			
#else
			int const w = 200;
			int const h = 326;
			static const ImageTileInfo ChessTiles[] =
			{
				{Vec2i(0,0),Vec2i(w,h)},
				{Vec2i(w * 1,0),Vec2i(w,h)},
				{Vec2i(w * 4,0),Vec2i(w,h)},
				{Vec2i(w * 3,0),Vec2i(w,h)},
				{Vec2i(w * 2,0),Vec2i(w,h)},
				{Vec2i(w * 5,0),Vec2i(w,h)},
			};
			Vector2 pivot = Vector2(0.5, 0.7);
			float sizeScale = 0.8;
#endif
			ImageTileInfo const& tileInfo = ChessTiles[type];
			Vector2 size;
			size.x = sizeScale * tileInfo.size.x * TileLength / w;
			size.y = size.x * tileInfo.size.y / tileInfo.size.x;
			g.drawTexture(*mChessTex, centerPos - pivot * size , size , tileInfo.pos + (( color == EChessColor::Black ) ? Vec2i(0,0) : Vec2i(0,h)), tileInfo.size );

		}
		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			Vec2i screenSize = Global::GetDrawEngine().getScreenSize();
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.8, 0.8, 0.8, 0), 1);

			g.beginRender();

			RenderUtility::SetPen(g, EColor::Black);
			for (int j = 0; j < BOARD_SIZE; ++j)
			{
				for (int i = 0; i < BOARD_SIZE; ++i)
				{
					Game::TileData const& tile = mGame.mBoard.getData(i, j);
					Vector2 tilePos = toScreenPos(Vec2i(i, j));

					RenderUtility::SetBrush(g, (i + j) % 2 ? EColor::White : EColor::Black);
					g.drawRect(tilePos, Vector2(TileLength, TileLength));
				}
			}


			if (bShowAttackTerritory)
			{
				g.setTextColor(Color3ub(0, 255, 0));

				for (int j = 0; j < BOARD_SIZE; ++j)
				{
					for (int i = 0; i < BOARD_SIZE; ++i)
					{
						Game::TileData const& tile = mGame.mBoard.getData(i, j);
						Vector2 tilePos = toScreenPos(Vec2i(i, j));
						Vector2 tileCenterPos = tilePos + 0.5 * Vector2(TileLength, TileLength);

#if 0
						FixString<256> str;
						str.format("%d %d", tile.whiteAttackCount, tile.blackAttackCount);
						g.drawText(tileCenterPos, str);
#endif


#if 1
#define BLEND_SCOPE( g , alpha ) 
#else
#define BLEND_SCOPE( g , alpha )  BlendScope scope( g , alpha);
#endif
						if (tile.blackAttackCount || tile.whiteAttackCount)
						{
							int border = 0.1 * TileLength;
							float alpha = 0.6;
							Vector2 renderPos = tilePos + Vector2(border, border);
							Vector2 renderSize = Vector2(TileLength, TileLength) - 2 * Vector2(border, border);

							EColor::Name const BlackAttackColor = EColor::Blue;
							EColor::Name const WhiteAttackColor = EColor::Red;

							BLEND_SCOPE(g, alpha);
							if (tile.blackAttackCount == 0)
							{
								RenderUtility::SetBrush(g, WhiteAttackColor);
								g.drawRect(renderPos, renderSize);
							}
							else if (tile.whiteAttackCount == 0)
							{
								RenderUtility::SetBrush(g, BlackAttackColor);
								g.drawRect(renderPos, renderSize);
							}
							else
							{
								if (tile.blackAttackCount != tile.whiteAttackCount)
								{
									float ratio = float(tile.blackAttackCount) / (tile.blackAttackCount + tile.whiteAttackCount);
									RenderUtility::SetBrush(g, BlackAttackColor);
									g.drawRect(renderPos, Vector2(renderSize.x, renderSize.y * ratio));
									RenderUtility::SetBrush(g, WhiteAttackColor);
									g.drawRect(renderPos + Vector2(0, renderSize.y * ratio), Vector2(renderSize.x, renderSize.y * (1 - ratio)));
								}
								else
								{
									RenderUtility::SetBrush(g, EColor::Yellow);
									g.drawRect(renderPos, renderSize);
								}
							}
						}
					}
				}
			}


			{
				BlendScope scope(g, 1.0f);
				RenderUtility::SetBrush(g, EColor::White);
				for (int j = 0; j < BOARD_SIZE; ++j)
				{
					for (int i = 0; i < BOARD_SIZE; ++i)
					{
						Game::TileData const& tile = mGame.mBoard.getData(i, j);
						Vector2 tileCenterPos = toScreenPos(Vec2i(i, j)) + 0.5 * Vector2(TileLength, TileLength);
						if (tile.chess)
						{
							drawChess(g, tileCenterPos, tile.chess->type, tile.chess->color);
						}
					}
				}
			}


			bool bShowAttackList = true;
			if (bShowAttackList)
			{
				BlendScope scope(g, 0.8f);
				if (mGame.isValidPos(mMouseTilePos))
				{
					auto& tileData = mGame.mBoard.getData(mMouseTilePos.x, mMouseTilePos.y);

					RenderUtility::SetBrush(g, EColor::Cyan);
					for (auto const& attack : tileData.attacks)
					{
						Vector2 tileCenterPos = toScreenPos(attack.pos) + 0.5 * Vector2(TileLength, TileLength);
						g.drawCircle(tileCenterPos, 12);
					}
				}
			}


			{
				BlendScope scope(g, 0.8f);
				if (!mMovePosList.empty())
				{
					RenderUtility::SetBrush(g, EColor::Green);
					for (auto const& move : mMovePosList)
					{
						Vector2 tileCenterPos = toScreenPos(move.pos) + 0.5 * Vector2(TileLength, TileLength);
						g.drawCircle(tileCenterPos, 12);
					}
				}
			}
			g.endRender();
		}

		Vector2 toScreenPos(Vec2i const& tPos)
		{
			return Vector2(10, 10) + TileLength * Vector2( tPos.x , BOARD_SIZE - tPos.y - 1 );
		}

		Vec2i toTilePos(Vector2 const& pos)
		{
			Vector2 temp = (pos - Vector2(10, 10)) / TileLength;
			Vec2i result;
			result.x = Math::FloorToInt(temp.x);
			result.y = BOARD_SIZE - Math::FloorToInt(temp.y) - 1;
			return result;
		}

		void moveChess(Vec2i const& from, MoveInfo const& move)
		{
			mGame.moveChess(from, move);
			invalidateSelectedChess();
		}

		void invalidateSelectedChess()
		{
			mSelectedChessPos.x = INDEX_NONE;
			mSelectedChessPos.y = INDEX_NONE;
			mMovePosList.clear();
		}

		bool onMouse(MouseMsg const& msg) override
		{
			if (!BaseClass::onMouse(msg))
				return false;

			Vec2i tPos = toTilePos(msg.getPos());

			mMouseTilePos = tPos;
			if (msg.onLeftDown())
			{
				if (mGame.isValidPos(mSelectedChessPos))
				{
					MoveInfo const* moveUsed = nullptr;
					for (auto const& move : mMovePosList)
					{
						if (move.pos == tPos)
						{
							moveUsed = &move;
							break;
						}
					}
					if (moveUsed)
					{
						moveChess(mSelectedChessPos, *moveUsed);
					}
				}
				else
				{
					mMovePosList.clear();
					if (mGame.getPossibleMove(tPos, mMovePosList))
					{
						mSelectedChessPos = tPos;
					}
				}
			}
			else if (msg.onRightDown())
			{
				invalidateSelectedChess();
			}
			return true;
		}
		bool bShowAttackTerritory = false;
		Vec2i mSelectedChessPos;
		Vec2i mMouseTilePos;
		std::vector<MoveInfo> mMovePosList;

		bool onKey(KeyMsg const& msg) override
		{
			if (!msg.isDown())
				return false;

			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::X: bShowAttackTerritory = !bShowAttackTerritory; break;
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};

	REGISTER_STAGE("Chess", TestStage, EStageGroup::Dev4);


}//namespace Chess
