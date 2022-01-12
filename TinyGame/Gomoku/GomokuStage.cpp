#include "GomokuStage.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/ShaderManager.h"
#include "RHI/SimpleRenderState.h"
#include "RHI/RHIGraphics2D.h"

REGISTER_STAGE_ENTRY("Gomoku", Gomoku::TestStage, EExecGroup::Test);

namespace Gomoku
{
	void TestStage::onRender(float dFrame)
	{
		using namespace Render;

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		RHICommandList& commandList = RHICommandList::GetImmediateList();


		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1);

		g.beginRender();
		SimpleRenderState renderState;

		//auto& viewingGame = getViewingGame();


		RenderContext context(mGame.getBoard(), BoardPos, RenderBoardScale);

		mBoardRenderer.draw(g, renderState, context, nullptr);

		for (auto pos : mPosList)
		{
			Vector2 renderPos = context.getIntersectionPos(pos.x, pos.y);
			g.drawCircle(renderPos, 5);
		}

		g.endRender();
	}

	MsgReply TestStage::onMouse(MouseMsg const& msg)
	{
		if (msg.onLeftDown())
		{
			Vec2i pos = RenderContext::CalcCoord(msg.getPos(), BoardPos, RenderBoardScale, mGame.getBoard().getSize());

			if (!mGame.getBoard().checkRange(pos.x, pos.y))
				return MsgReply::Handled();

			mGame.playStone(pos.x, pos.y);
		}
		else if (msg.onRightDown())
		{
			if (msg.isControlDown())
			{
				Vec2i pos = RenderContext::CalcCoord(msg.getPos(), BoardPos, RenderBoardScale, mGame.getBoard().getSize());
				mPosList.clear();
				bool isBanMove = mGame.isBanMove(pos.x, pos.y);
				LogMsg("Is Ban Move %s", isBanMove ? "true" : "false");
			}
			else
			{
				mGame.undo();
			}
		}
		return BaseClass::onMouse(msg);
	}

	bool TestStage::setupRenderSystem(ERenderSystem systemName)
	{
		VERIFY_RETURN_FALSE(mBoardRenderer.initializeRHI());
		return true;
	}

	void TestStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{
		mBoardRenderer.releaseRHI();
	}

	void TestStage::emitDebugPos(int index, int type)
	{
		if ( type == EDebugType::NEXT_POS )
		{
			int coord[2];
			mGame.getBoard().getPosCoord(index, coord);
			mPosList.push_back(Vec2i(coord[0], coord[1]));
		}
	}

	void BoardRenderer::draw(RHIGraphics2D& g, SimpleRenderState& renderState, RenderContext const& context, int const* overrideStoneState /*= nullptr*/)
	{
		drawBorad(g, renderState, context, overrideStoneState);

		{
			Board const& board = static_cast<Board const&>(context.board);
			int boardSize = context.board.getSize();

			RenderUtility::SetFont(g, FONT_S8);
			for (int j = 0; j < boardSize; ++j)
			{
				for (int i = 0; i < boardSize; ++i)
				{
					Board::Pos posBoard = board.getPos(i, j);
					int data = context.board.getData(posBoard);
					if (data != EStoneColor::Empty)
					{
						Vector2 const OffsetMap[4] =
						{
							Vector2(0,1),
							Vector2(1,-1),
							Vector2(1, 0),
							Vector2(-1,-1),
						};
						Vector2 offset = Vector2(3, 3);
						Vector2 pos = getStonePos(context, i, j) - offset;
						int conCounts[4];
						for (int i = 0; i < EConDir::Count; ++i)
						{
							//int blockValue = 0;
							uint blockMask = 0;
							int count = board.getConnectCountWithMask(posBoard.toIndex(), EConDir::Type(i) , blockMask);
							int indexLinkRoot = board.getLinkRoot(posBoard.toIndex(), EConDir::Type(i));

							switch (blockMask)
							{
							case 0x0: g.setTextColor(Color3f(0, 1, 0)); break;
							case 0x1: g.setTextColor(Color3f(1, 0, 0)); break;
							case 0x2: g.setTextColor(Color3f(0, 0, 1)); break;
							case 0x3: g.setTextColor(Color3f(1, 0, 1)); break;
							}
							InlineString<128> str;
							str.format("%d", count);
							g.drawText(pos + 0.5 * context.stoneRadius * context.scale * OffsetMap[i], str);

							if (indexLinkRoot == posBoard.toIndex())
							{
								RenderUtility::SetBrush(g, EColor::Null);
								RenderUtility::SetPen(g, EColor::Red);
								g.drawRect(pos + 0.5 * context.stoneRadius * context.scale * OffsetMap[i], Vector2(5, 1));
							}
						}
					}
				}
			}
		}
	}

}//namespace Gomoku