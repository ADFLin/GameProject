#include "GoRenderer.h"

#include "GoCore.h"

#include "RenderUtility.h"
#include "RHI/RHIGraphics2D.h"

#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHICommand.h"
#include "RHI/SimpleRenderState.h"

#include "ConsoleSystem.h"

#define USE_OPENGL_NATIVE 0

namespace Go
{
	using namespace Render;

	bool BoardRendererBase::initializeRHI()
	{
		VERIFY_RETURN_FALSE(mTextureAtlas.initialize(ETexture::RGBA8, 128, 128, 2));
		VERIFY_RETURN_FALSE(mTextureAtlas.addImageFile("Go/blackStone.png") == 0);
		VERIFY_RETURN_FALSE(mTextureAtlas.addImageFile("Go/WhiteStone.png") == 1);

#define TEXTURE( ID , PATH , bGetUV )\
			mTextures[ID] = RHIUtility::LoadTexture2DFromFile(PATH);\
			VERIFY_RETURN_FALSE( mTextures[ID].isValid() );\
			if ( bGetUV ) mTextureAtlas.getRectUV(ID, mTexInfos[ID].uvMin , mTexInfos[ID].uvMax);

		TEXTURE(TextureId::eBlockStone, "Go/blackStone.png", true);
		TEXTURE(TextureId::eWhiteStone, "Go/WhiteStone.png", true);
		TEXTURE(TextureId::eBoardA, "Go/badukpan4.png", false);
#undef TEXTURE

#if USE_OPENGL_NATIVE
		//#TODO:Remove me
		auto cmd = ConsoleSystem::Get().findCommand("r.OpenGLFixedPiplineUseShader");
		if (cmd)
		{
			cmd->asVariable()->setFromInt(0);
		}
#endif
		return true;
	}

	void BoardRendererBase::releaseRHI()
	{
		mTextureAtlas.finalize();
		for(auto & mTexture : mTextures)
		{
			mTexture.release();
		}
	}

	void BoardRendererBase::generateNoiseOffset(int boradSize)
	{
		int size = boradSize * boradSize;
		float maxOffset = 1.6;
		mNoiseOffsets.resize(size);
		for( int i = 0; i < size; ++i )
		{
			mNoiseOffsets[i].x = maxOffset * RandFloat(-1, 1);
			mNoiseOffsets[i].y = maxOffset * RandFloat(-1, 1);
		}
	}


	void BoardRendererBase::drawStoneSequence( SimpleRenderState& renderState , RenderContext const& context, std::vector<PlayVertex> const& vertices, int colorStart, float opacity)
	{
		using namespace Render;
		using namespace Go;

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		RHICommandList& commandList = RHICommandList::GetImmediateList();
		{
			GPU_PROFILE("Draw Stone");

#if DRAW_TEXTURE


#if USE_OPENGL_NATIVE
			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::SrcAlpha, EBlend::OneMinusSrcAlpha >::GetRHI());
			glEnable(GL_TEXTURE_2D);
			glActiveTexture(GL_TEXTURE0);
#endif
#endif
			int color = colorStart;
			for( PlayVertex const& v : vertices)
			{
				if ( v.isOnBoard() )
				{
					int x = v.x;
					int y = v.y;
					Vector2 pos = getStonePos(context, x, y);
					drawStone(g, renderState, pos, color, context.stoneRadius, context.scale, opacity);
					color = EStoneColor::Opposite(color);
				}
			}

#if DRAW_TEXTURE

#if USE_OPENGL_NATIVE
			if( bUseBatchedRender )
			{
				if( !mSpriteVertices.empty() )
				{
					GL_SCOPED_BIND_OBJECT(mTextureAtlas.getTexture());
					TRenderRT< RTVF_XY_CA_T2 >::Draw(commandList, EPrimitive::Quad, &mSpriteVertices[0], mSpriteVertices.size());
					mSpriteVertices.clear();
				}
			}

			glDisable(GL_TEXTURE_2D);
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
#endif

#endif
		}
	}

	void BoardRendererBase::drawStoneNumber(SimpleRenderState& renderState, RenderContext const& context, int number)
	{

	}

	Vector2 BoardRendererBase::getStonePos(RenderContext const& context, int i, int j)
	{
		Vector2 pos = context.getIntersectionPos(i, j);
		if( bUseNoiseOffset )
			pos += context.scale * getNoiseOffset(i, j, context.board.getSize());
		return pos;
	}

	void BoardRendererBase::drawBorad(RHIGraphics2D& g, SimpleRenderState& renderState, RenderContext const& context, int const* overrideStoneState)
	{
		using namespace Render;
		using namespace Go;

		static char const* CoordStr = "ABCDEFGHJKLMNOPQRSTQVWXYZ";
		

		int boardSize = context.board.getSize();
		float length = (boardSize - 1) * context.cellLength;

		float const border = 0.5 * context.cellLength + ((bDrawCoord) ? 30 : 0);
		float const boardRenderLength = length + 2 * border;

		RHICommandList& commandList = RHICommandList::GetImmediateList();

#if DRAW_TEXTURE
#if USE_OPENGL_NATIVE
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		{
			GL_SCOPED_BIND_OBJECT(mTextures[TextureId::eBoardA]);
			DrawUtility::Sprite(commandList,
				context.renderPos - Vector2(border, border), Vector2(boardRenderLength, boardRenderLength), Vector2(0, 0),
				LinearColor(1, 1, 1, 1), Vector2(0, 0), 2 * Vector2(1, 1));
		}
		glDisable(GL_TEXTURE_2D);
#else
		RenderUtility::SetBrush(g, EColor::White);
		g.drawTexture(*mTextures[TextureId::eBoardA], context.renderPos - Vector2(border, border), Vector2(boardRenderLength, boardRenderLength), Vector2(0, 0), 2 * Vector2(1, 1));


#endif
#else
		RenderUtility::SetPen(g, EColor::Black);
		RenderUtility::SetBrush(g, EColor::Orange);
		g.drawRect(renderPos - Vector2(border, border), Vector2(boardRenderLength, boardRenderLength));
#endif

		{
			RenderUtility::SetPen(g, EColor::Black);
			Vector2 posV = context.renderPos;
			Vector2 posH = context.renderPos;
			for (int i = 0; i < boardSize; ++i)
			{
				g.drawLine(posV, posV + Vector2(0, length));
				g.drawLine(posH, posH + Vector2(length, 0));
				posV.x += context.cellLength;
				posH.y += context.cellLength;
			}
		}

		RenderUtility::SetPen(g, EColor::Black);
		RenderUtility::SetBrush(g, EColor::Black);

		if( bDrawStar )
		{
			auto DrawStar = [&](TArrayView<int const> starPosList )
			{
				Vector2 pos;
				for (int i = 0; i < starPosList.size(); ++i)
				{
					pos.x = context.renderPos.x + starPosList[i] * context.cellLength;
					for (int j = 0; j < starPosList.size(); ++j)
					{
						pos.y = context.renderPos.y + starPosList[j] * context.cellLength;
						g.drawCircle(pos, context.starRadius);
					}
				}
			};
			switch( boardSize )
			{
			case 19: DrawStar({ 3 , 9 , 15 }); break;
			case 15: DrawStar({ 3 , 7 , 11 });  break;
			case 13: DrawStar({ 3 , 9 });  break;
			}
		}

		if (bDrawCoord)
		{
			g.setBlendState(ESimpleBlendMode::Translucent);
			RenderUtility::SetFont(g, FONT_S12);
			g.setTextColor(Color3ub(0, 0, 0));

			Vector2 posV = context.renderPos;
			Vector2 posH = context.renderPos;
			for (int i = 0; i < boardSize; ++i)
			{
				InlineString< 64 > str;
				str.format("%2d", boardSize - i);
				g.drawText(posH - Vector2(30, 8), str);
				g.drawText(posH + Vector2(12 + length, -8), str);

				str.format("%c", CoordStr[i]);
				g.drawText(posV - Vector2(5, 30), str);
				g.drawText(posV + Vector2(-5, 15 + length), str);
				posV.x += context.cellLength;
				posH.y += context.cellLength;
			}

			g.flush();
		}

		{
			GPU_PROFILE("Draw Stone");

#if DRAW_TEXTURE
#if USE_OPENGL_NATIVE
			g.setBlendState(ESimpleBlendMode::Translucent);
			g.comitRenderState();
			glEnable(GL_TEXTURE_2D);
			glActiveTexture(GL_TEXTURE0);
#else
			RenderUtility::SetBrush(g, EColor::White);
			g.beginBlend(1.0f);
#endif
#endif

			if (overrideStoneState)
			{
				for (int j = 0; j < boardSize; ++j)
				{
					for (int i = 0; i < boardSize; ++i)
					{
						int data = overrideStoneState[j * boardSize + i];
						if (data != EStoneColor::Empty)
						{
							Vector2 pos = getStonePos(context, i, j);
							drawStone(g, renderState, pos, data, context.stoneRadius, context.scale);
						}
					}
				}
			}
			else
			{
				for (int j = 0; j < boardSize; ++j)		
				{
					for (int i = 0; i < boardSize; ++i)
					{
						int data = context.board.getData(i, j);
						if (data != EStoneColor::Empty)
						{
							Vector2 pos = getStonePos(context, i, j);
							drawStone(g, renderState, pos, data, context.stoneRadius, context.scale);
						}
					}
				}
			}

#if DRAW_TEXTURE
#if USE_OPENGL_NATIVE
			if( bUseBatchedRender )
			{
				if( !mSpriteVertices.empty() )
				{
					GL_SCOPED_BIND_OBJECT(mTextureAtlas.getTexture());
					TRenderRT< RTVF_XY_CA_T2 >::Draw(commandList, EPrimitive::Quad, &mSpriteVertices[0], mSpriteVertices.size());
					mSpriteVertices.clear();
				}
			}

			glDisable(GL_TEXTURE_2D);
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			g.setBlendState(ESimpleBlendMode::None);
#else
			g.endBlend();
#endif
#endif
		}
	}

	void BoardRendererBase::addBatchedSprite(int id, Vector2 pos, Vector2 size, Vector2 pivot, Vector4 color)
	{
		Vector2 posLT = pos - size.mul(pivot);
		Vector2 posRB = posLT + size;
		Vector2 const& min = mTexInfos[id].uvMin;
		Vector2 const& max = mTexInfos[id].uvMax;
		mSpriteVertices.push_back({ posLT , color , min });
		mSpriteVertices.push_back({ Vector2(posLT.x , posRB.y) , color , Vector2(min.x , max.y) });
		mSpriteVertices.push_back({ posRB , color , max });
		mSpriteVertices.push_back({ Vector2(posRB.x , posLT.y) , color , Vector2(max.x , min.y) });
	}

	void BoardRendererBase::drawStone(RHIGraphics2D& g, SimpleRenderState& renderState, Vector2 const& pos, int color , float stoneRadius , float scale , float opaticy)
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();

#if DRAW_TEXTURE
#if USE_OPENGL_NATIVE
		if( bUseBatchedRender )
		{
			int id = (color == EStoneColor::Black) ? 0 : 1;
			addBatchedSprite(id, pos + scale * Vector2(2, 2), 2.1 * Vector2(stoneRadius, stoneRadius), Vector2(0.5, 0.5), Vector4(0, 0, 0, 0.2 * opaticy));
			addBatchedSprite(id, pos, 2 * Vector2(stoneRadius, stoneRadius), Vector2(0.5, 0.5), Vector4(1, 1, 1, opaticy));
		}
		else
		{
			int id = (color == EStoneColor::Black) ? TextureId::eBlockStone : TextureId::eWhiteStone;	
			{
				GL_SCOPED_BIND_OBJECT(mTextures[id]);


				glColor4f(0, 0, 0, 0.2 * opaticy);
				DrawUtility::Sprite(commandList, pos + scale * Vector2(2, 2), 2.1 * Vector2(stoneRadius, stoneRadius), Vector2(0.5, 0.5));

				glColor4f(1, 1, 1 , opaticy);
				DrawUtility::Sprite(commandList, pos, 2 * Vector2(stoneRadius, stoneRadius), Vector2(0.5, 0.5));

			}
		}
#else
		int id = (color == EStoneColor::Black) ? TextureId::eBlockStone : TextureId::eWhiteStone;
		auto AddSprite = [&](int id, Vector2 pos, Vector2 size, Vector2 pivot, Vector4 color)
		{
			Vector2 posLT = pos - size.mul(pivot);
			Vector2 posRB = posLT + size;
			Vector2 const& min = mTexInfos[id].uvMin;
			Vector2 const& max = mTexInfos[id].uvMax;
			g.setBrush(LinearColor( color.xyz() ));
			g.setBlendAlapha(color.w);
			g.drawTexture(mTextureAtlas.getTexture(), posLT, size, min, max - min);
		};
		AddSprite(id, pos + scale * Vector2(2, 2), 2.1 * Vector2(stoneRadius, stoneRadius), Vector2(0.5, 0.5), Vector4(0, 0, 0, 0.2 * opaticy));
		AddSprite(id, pos, 2 * Vector2(stoneRadius, stoneRadius), Vector2(0.5, 0.5), Vector4(1, 1, 1, opaticy));

#endif
#else
		RenderUtility::SetPen(g, EColor::Black);
		RenderUtility::SetBrush(g, (color == EStoneColor::Black) ? EColor::Black : EColor::White);
		g.drawCircle(pos, StoneRadius);
		if( color == EStoneColor::Black )
		{
			RenderUtility::SetBrush(g, EColor::White);
			g.drawCircle(pos + Vec2i(5, -5), 3);
		}
#endif
	}

	void BoardRenderer::draw(RHIGraphics2D& g, SimpleRenderState& renderState, RenderContext const& context, int const* overrideStoneState /*= nullptr*/)
	{
		drawBorad(g, renderState, context, overrideStoneState);

		if (overrideStoneState == nullptr)
		{
			Board const& board = static_cast<Board const&>(context.board);
			Vector2 halfCellSize = 0.5 * Vector2(context.cellLength, context.cellLength);
			int boardSize = board.getSize();

			for (int i = 0; i < boardSize; ++i)
			{
				for (int j = 0; j < boardSize; ++j)
				{
					int data = context.board.getData(i, j);
					if (data)
					{
						Vector2 pos = context.getIntersectionPos(i, j);
						//Vector2 pos = getStonePos(renderPos, board, i, j);
						InlineString<128> str;

						Board::Pos posBoard = board.getPos(i, j);

						if (bDrawLinkInfo)
						{
							int dist = board.getLinkToRootDist(posBoard);
							if (dist)
							{
								g.setTextColor(Color3ub(0, 255, 255));
								str.format("%d", dist);
							}
							else
							{
								g.setTextColor(Color3ub(255, 125, 0));
								str.format("%d", board.getCaptureCount(posBoard));
							}
							g.drawText(pos - halfCellSize, Vector2(context.cellLength, context.cellLength), str);
						}

						if (bDrawStepNum)
						{

						}
						}
					}
				}
			}
		}

}//namespace Go