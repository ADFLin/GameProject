#include "GoRenderer.h"

#include "GoCore.h"

#include "RenderUtility.h"
#include "GLGraphics2D.h"

#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHICommand.h"

namespace Go
{
	using namespace Render;

	bool BoardRenderer::initializeRHI()
	{
		VERIFY_RETURN_FALSE(mTextureAtlas.initialize(Texture::eRGBA8, 128, 128, 2));
		VERIFY_RETURN_FALSE(mTextureAtlas.addImageFile("Go/blackStone.png") == 0);
		VERIFY_RETURN_FALSE(mTextureAtlas.addImageFile("Go/WhiteStone.png") == 1);

#define TEXTURE( ID , PATH )\
			mTextures[ID] = RHIUtility::LoadTexture2DFromFile(PATH);\
			VERIFY_RETURN_FALSE( mTextures[ID].isValid() );

		TEXTURE(TextureId::eBlockStone, "Go/blackStone.png");
		TEXTURE(TextureId::eWhiteStone, "Go/WhiteStone.png");
		TEXTURE(TextureId::eBoardA, "Go/badukpan4.png");
#undef TEXTURE

		return true;
	}

	void BoardRenderer::releaseRHI()
	{
		mTextureAtlas.finalize();
		for( int i = 0; i < NumTexture; ++i )
		{
			mTextures[i].release();
		}
	}

	void BoardRenderer::generateNoiseOffset(int boradSize)
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


	void BoardRenderer::drawStoneSequence(RenderContext const& context, std::vector<PlayVertex> const& vertices, int colorStart, float opacity)
	{
		using namespace Render;
		using namespace Go;

		GLGraphics2D& g = ::Global::GetRHIGraphics2D();
		RHICommandList& commandList = RHICommandList::GetImmediateList();
		{
			GPU_PROFILE("Draw Stone");

#if DRAW_TEXTURE
			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, Blend::eSrcAlpha, Blend::eOneMinusSrcAlpha >::GetRHI());

			glEnable(GL_TEXTURE_2D);
			glActiveTexture(GL_TEXTURE0);
#endif
			int color = colorStart;
			for( PlayVertex const& v : vertices)
			{
				if ( v.isOnBoard() )
				{
					int x = v.x;
					int y = v.y;


					Vector2 pos = getStonePos(context, x, y);
					drawStone(g, pos, color, context.stoneRadius, context.scale, opacity);
					color = StoneColor::Opposite(color);
				}
			}

#if DRAW_TEXTURE
			if( bUseBatchedRender )
			{
				if( !mSpriteVertices.empty() )
				{
					GL_BIND_LOCK_OBJECT(mTextureAtlas.getTexture());
					TRenderRT< RTVF_XY_CA_T2 >::Draw(commandList, PrimitiveType::Quad, &mSpriteVertices[0], mSpriteVertices.size());
					mSpriteVertices.clear();
				}
			}

			glDisable(GL_TEXTURE_2D);

			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI() );
#endif
		}
	}


	Vector2 BoardRenderer::getStonePos(RenderContext const& context, int i, int j)
	{
		Vector2 pos = context.getIntersectionPos(i, j);
		if( bUseNoiseOffset )
			pos += context.scale * getNoiseOffset(i, j, context.board.getSize());
		return pos;
	}

	Vector2 BoardRenderer::getIntersectionPos(RenderContext const& context, int i, int j)
	{
		return context.renderPos + context.cellLength * Vector2(i, j);
	}

	void BoardRenderer::drawBorad(GLGraphics2D& g , RenderContext const& context)
	{
		using namespace Render;
		using namespace Go;

		static char const* CoordStr = "ABCDEFGHJKLMNOPQRSTQV";
		static int const StarMarkPos[3] = { 3 , 9 , 15 };

		int boardSize = context.board.getSize();
		float length = (boardSize - 1) * context.cellLength;

		float const border = 0.5 * context.cellLength + (( bDrawCoord ) ? 30 : 0 );
		float const boardRenderLength = length + 2 * border;

		RHICommandList& commandList = RHICommandList::GetImmediateList();


		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

#if DRAW_TEXTURE
		glColor3f(1, 1, 1);

		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		{
			GL_BIND_LOCK_OBJECT(mTextures[TextureId::eBoardA]);
			DrawUtility::Sprite(commandList,
				context.renderPos - Vector2(border, border), Vector2(boardRenderLength, boardRenderLength), Vector2(0, 0),
				Vector2(0, 0), 2 * Vector2(1, 1));
		}
		glDisable(GL_TEXTURE_2D);
#else
		RenderUtility::SetPen(g, EColor::Black);
		RenderUtility::SetBrush(g, EColor::Orange);
		g.drawRect(renderPos - Vector2(border, border), Vector2(boardRenderLength, boardRenderLength));
#endif

		Vector2 posV = context.renderPos;
		Vector2 posH = context.renderPos;

		RenderUtility::SetPen(g, EColor::Black);
		RenderUtility::SetFont(g, FONT_S12);
		g.setTextColor(Color3ub(0, 0, 0));
		for( int i = 0; i < boardSize; ++i )
		{
			g.drawLine(posV, posV + Vector2(0, length));
			g.drawLine(posH, posH + Vector2(length, 0));
			if( bDrawCoord )
			{
				FixString< 64 > str;
				str.format("%2d", i + 1);
				g.drawText(posH - Vector2(30, 8), str);
				g.drawText(posH + Vector2(12 + length, -8), str);

				str.format("%c", CoordStr[i]);
				g.drawText(posV - Vector2(5, 30), str);
				g.drawText(posV + Vector2(-5, 15 + length), str);
			}
			posV.x += context.cellLength;
			posH.y += context.cellLength;
		}

		RenderUtility::SetPen(g, EColor::Black);
		RenderUtility::SetBrush(g, EColor::Black);

		if( bDrawStar )
		{
			switch( boardSize )
			{
			case 19:
				{
					Vector2 pos;
					for( int i = 0; i < 3; ++i )
					{
						pos.x = context.renderPos.x + StarMarkPos[i] * context.cellLength;
						for( int j = 0; j < 3; ++j )
						{
							pos.y = context.renderPos.y + StarMarkPos[j] * context.cellLength;
							g.drawCircle(pos, context.starRadius);
						}
					}
				}
				break;
			case 13:
				{
					Vector2 pos;
					for( int i = 0; i < 2; ++i )
					{
						pos.x = context.renderPos.x + StarMarkPos[i] * context.cellLength;
						for( int j = 0; j < 2; ++j )
						{
							pos.y = context.renderPos.y + StarMarkPos[j] * context.cellLength;
							g.drawCircle(pos, context.starRadius);
						}
					}
					g.drawCircle(context.renderPos + context.cellLength * Vec2i(6, 6), context.starRadius);
				}
				break;
			}
		}

		{
			GPU_PROFILE("Draw Stone");

#if DRAW_TEXTURE
			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, Blend::eSrcAlpha, Blend::eOneMinusSrcAlpha >::GetRHI());


			glEnable(GL_TEXTURE_2D);
			glActiveTexture(GL_TEXTURE0);
#endif
			for( int i = 0; i < boardSize; ++i )
			{
				for( int j = 0; j < boardSize; ++j )
				{
					int data = context.board.getData(i, j);
					if( data )
					{
						Vector2 pos = getStonePos(context, i, j);
						drawStone( g , pos, data , context.stoneRadius , context.scale);
					}
				}
			}

#if DRAW_TEXTURE
			if( bUseBatchedRender )
			{
				if( !mSpriteVertices.empty() )
				{
					GL_BIND_LOCK_OBJECT(mTextureAtlas.getTexture());
					TRenderRT< RTVF_XY_CA_T2 >::Draw(commandList, PrimitiveType::Quad, &mSpriteVertices[0], mSpriteVertices.size());
					mSpriteVertices.clear();
				}
			}

			glDisable(GL_TEXTURE_2D);
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
#endif
		}



		{

			Vector2 halfCellSize = 0.5 * Vector2(context.cellLength, context.cellLength);

			for( int i = 0; i < boardSize; ++i )
			{
				for( int j = 0; j < boardSize; ++j )
				{
					int data = context.board.getData(i, j);
					if( data )
					{
						Vector2 pos = context.renderPos + context.cellLength * Vector2(i, j);
						//Vector2 pos = getStonePos(renderPos, board, i, j);
						FixString<128> str;

						Board::Pos posBoard = context.board.getPos(i, j);
						
						if( bDrawLinkInfo )
						{
							int dist = context.board.getLinkToRootDist(posBoard);
							if( dist )
							{
								g.setTextColor(Color3ub(0, 255, 255));
								str.format("%d", dist);
							}
							else
							{
								g.setTextColor(Color3ub(255, 125, 0));
								str.format("%d", context.board.getCaptureCount(posBoard));
							}
							g.drawText(pos - halfCellSize, Vector2(context.cellLength, context.cellLength), str );
						}

						if( bDrawStepNum )
						{

						}
					}
				}
			}
		}
	}

	void BoardRenderer::addBatchedSprite(int id, Vector2 pos, Vector2 size, Vector2 pivot, Vector4 color)
	{
		Vector2 posLT = pos - size.mul(pivot);
		Vector2 posRB = posLT + size;
		Vector2 min, max;
		mTextureAtlas.getRectUV(id, min, max);
		mSpriteVertices.push_back({ posLT , color , min });
		mSpriteVertices.push_back({ Vector2(posLT.x , posRB.y) , color , Vector2(min.x , max.y) });
		mSpriteVertices.push_back({ posRB , color , max });
		mSpriteVertices.push_back({ Vector2(posRB.x , posLT.y) , color , Vector2(max.x , min.y) });
	}

	void BoardRenderer::drawStone(GLGraphics2D& g, Vector2 const& pos, int color , float stoneRadius , float scale , float opaticy)
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();

#if DRAW_TEXTURE
		if( bUseBatchedRender )
		{
			int id = (color == StoneColor::eBlack) ? 0 : 1;
			addBatchedSprite(id, pos + scale * Vector2(2, 2), 2.1 * Vector2(stoneRadius, stoneRadius), Vector2(0.5, 0.5), Vector4(0, 0, 0, 0.2 * opaticy));
			addBatchedSprite(id, pos, 2 * Vector2(stoneRadius, stoneRadius), Vector2(0.5, 0.5), Vector4(1, 1, 1, opaticy));
		}
		else
		{
			int id = (color == StoneColor::eBlack) ? TextureId::eBlockStone : TextureId::eWhiteStone;	
			{
				GL_BIND_LOCK_OBJECT(mTextures[id]);

				glColor4f(0, 0, 0, 0.2 * opaticy);
				DrawUtility::Sprite(commandList, pos + scale * Vector2(2, 2), 2.1 * Vector2(stoneRadius, stoneRadius), Vector2(0.5, 0.5));

				glColor4f(1, 1, 1 , opaticy);
				DrawUtility::Sprite(commandList, pos, 2 * Vector2(stoneRadius, stoneRadius), Vector2(0.5, 0.5));

			}
		}
#else
		RenderUtility::SetPen(g, EColor::Black);
		RenderUtility::SetBrush(g, (color == StoneColor::eBlack) ? EColor::Black : EColor::White);
		g.drawCircle(pos, StoneRadius);
		if( color == StoneColor::eBlack )
		{
			RenderUtility::SetBrush(g, EColor::White);
			g.drawCircle(pos + Vec2i(5, -5), 3);
		}
#endif
	}

}//namespace Go