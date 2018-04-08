#include "GoRenderer.h"

#include "GoCore.h"

#include "RenderUtility.h"
#include "GLGraphics2D.h"

#include "RenderGL/DrawUtility.h"
#include "RenderGL/GpuProfiler.h"

namespace Go
{
	bool GameRenderer::initializeRHI()
	{
		if( !mTextureAtlas.create(Texture::eRGBA8, 128, 128, 2) )
			return false;

		if( mTextureAtlas.addImageFile("Go/blackStone.png") != 0 )
			return false;
		if( mTextureAtlas.addImageFile("Go/WhiteStone.png") != 1 )
			return false;

#define TEXTURE( ID , PATH )\
			if( !mTextures[ID].loadFromFile(PATH) )\
				return false

		TEXTURE(TextureId::eBlockStone, "Go/blackStone.png");
		TEXTURE(TextureId::eWhiteStone, "Go/WhiteStone.png");
		TEXTURE(TextureId::eBoardA, "Go/badukpan4.png");

#undef TEXTURE

		return true;
	}

	void GameRenderer::releaseRHI()
	{
		mTextureAtlas.getTexture().release();
		for( int i = 0; i < NumTexture; ++i )
		{
			mTextures[i].release();
		}
	}

	void GameRenderer::generateNoiseOffset(int boradSize)
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

	void GameRenderer::draw(Vector2 const& renderPos, Game const& game)
	{
		GLGraphics2D& g = ::Global::getGLGraphics2D();

		Board const& board = game.getBoard();

		drawBorad(renderPos, board);

		int lastPlayPos[2] = { -1,-1 };
		game.getCurrentStepPos(lastPlayPos);
		if( lastPlayPos[0] != -1 && lastPlayPos[1] != -1 )
		{
			Vector2 pos = getStonePos(renderPos, board, lastPlayPos[0], lastPlayPos[1]);
			RenderUtility::SetPen(g, Color::eRed);
			RenderUtility::SetBrush(g, Color::eRed);
			g.drawCircle(pos, StoneRadius / 2);
		}
	}

	Vector2 GameRenderer::getStonePos(Vector2 const& renderPos, Board const& board, int i, int j)
	{
		Vector2 pos = renderPos + CellLength * Vector2(i, j);
		if( bUseNoiseOffset )
			pos += getNoiseOffset(i, j, board.getSize());
		return pos;
	}

	Vector2 GameRenderer::getIntersectionPos(Vector2 const& renderPos, Board const& board, int i, int j)
	{
		return renderPos + CellLength * Vector2(i, j);
	}

	void GameRenderer::drawBorad(Vector2 const& renderPos, Board const& board)
	{
		using namespace RenderGL;
		using namespace Go;

		GLGraphics2D& g = ::Global::getGLGraphics2D();

		char const* CoordStr = "ABCDEFGHJKLMNOPQRSTQV";
		int const StarMarkPos[3] = { 3 , 9 , 15 };

		int size = board.getSize();
		int length = (size - 1) * CellLength;

		int const border = 40;
		int const boardSize = length + 2 * border;

#if DRAW_TEXTURE
		glColor3f(1, 1, 1);

		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		{
			GL_BIND_LOCK_OBJECT(mTextures[TextureId::eBoardA]);
			DrawUtility::Sprite(
				renderPos - Vec2i(border, border), Vec2i(boardSize, boardSize), Vector2(0, 0),
				Vector2(0, 0), 2 * Vector2(1, 1));
		}
		glDisable(GL_TEXTURE_2D);
#else
		RenderUtility::SetPen(g, Color::eBlack);
		RenderUtility::SetBrush(g, Color::eOrange);
		g.drawRect(renderPos - Vector2(border, border), Vector2(boardSize, boardSize));
#endif

		Vector2 posV = renderPos;
		Vector2 posH = renderPos;
		RenderUtility::SetPen(g, Color::eBlack);
		RenderUtility::SetFont(g, FONT_S12);
		g.setTextColor(0, 0, 0);
		for( int i = 0; i < size; ++i )
		{
			g.drawLine(posV, posV + Vector2(0, length));
			g.drawLine(posH, posH + Vector2(length, 0));

			FixString< 64 > str;
			str.format("%2d", i + 1);
			g.drawText(posH - Vector2(30, 8), str);
			g.drawText(posH + Vector2(12 + length, -8), str);

			str.format("%c", CoordStr[i]);
			g.drawText(posV - Vector2(5, 30), str);
			g.drawText(posV + Vector2(-5, 15 + length), str);

			posV.x += CellLength;
			posH.y += CellLength;
		}

		RenderUtility::SetPen(g, Color::eBlack);
		RenderUtility::SetBrush(g, Color::eBlack);

		if( bDrawStar )
		{
			switch( size )
			{
			case 19:
				{
					Vector2 pos;
					for( int i = 0; i < 3; ++i )
					{
						pos.x = renderPos.x + StarMarkPos[i] * CellLength;
						for( int j = 0; j < 3; ++j )
						{
							pos.y = renderPos.y + StarMarkPos[j] * CellLength;
							g.drawCircle(pos, StarRadius);
						}
					}
				}
				break;
			case 13:
				{
					Vector2 pos;
					for( int i = 0; i < 2; ++i )
					{
						pos.x = renderPos.x + StarMarkPos[i] * CellLength;
						for( int j = 0; j < 2; ++j )
						{
							pos.y = renderPos.y + StarMarkPos[j] * CellLength;
							g.drawCircle(pos, StarRadius);
						}
					}
					g.drawCircle(renderPos + CellLength * Vec2i(6, 6), StarRadius);
				}
				break;
			}
		}

		{
			GPU_PROFILE("Draw Stone");

#if DRAW_TEXTURE
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_TEXTURE_2D);
			glActiveTexture(GL_TEXTURE0);
#endif
			for( int i = 0; i < size; ++i )
			{
				for( int j = 0; j < size; ++j )
				{
					int data = board.getData(i, j);
					if( data )
					{
						Vector2 pos = getStonePos(renderPos, board, i, j);
						drawStone(g, pos, data);
					}
				}
			}

#if DRAW_TEXTURE
			if( bUseBatchedRender )
			{
				if( !mSpriteVertices.empty() )
				{
					GL_BIND_LOCK_OBJECT(mTextureAtlas.getTexture());
					TRenderRT< RTVF_XY_CA_T2 >::Draw(PrimitiveType::eQuad, &mSpriteVertices[0], mSpriteVertices.size());
					mSpriteVertices.clear();
				}
			}

			glDisable(GL_TEXTURE_2D);
			glDisable(GL_BLEND);
#endif
		}


		{

			Vector2 halfCellSize = 0.5 * Vector2(CellLength, CellLength);

			for( int i = 0; i < size; ++i )
			{
				for( int j = 0; j < size; ++j )
				{
					int data = board.getData(i, j);
					if( data )
					{
						Vector2 pos = renderPos + CellLength * Vector2(i, j);
						//Vector2 pos = getStonePos(renderPos, board, i, j);
						FixString<128> str;

						Board::Pos posBoard = board.getPos(i, j);
						
						if( bDrawLinkInfo )
						{
							int dist = board.getLinkToRootDist(posBoard);
							if( dist )
							{
								g.setTextColor(0, 255, 255);
								str.format("%d", dist);
							}
							else
							{
								g.setTextColor(255, 125, 0);
								str.format("%d", board.getCaptureCount(posBoard));
							}
							g.drawText(pos - halfCellSize, Vector2(CellLength, CellLength), str );
						}

						if( bDrawStepNum )
						{

						}
					}
				}
			}
		}
	}

	void GameRenderer::addBatchedSprite(int id, Vector2 pos, Vector2 size, Vector2 pivot, Vector4 color)
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

	void GameRenderer::drawStone(GLGraphics2D& g, Vector2 const& pos, int color)
	{
#if DRAW_TEXTURE
		if( bUseBatchedRender )
		{
			int id = (color == StoneColor::eBlack) ? 0 : 1;
			addBatchedSprite(id, pos + Vector2(2, 2), 2 * Vector2(StoneRadius, StoneRadius), Vector2(0.5, 0.5), Vector4(0, 0, 0, 0.2));
			addBatchedSprite(id, pos, 2 * Vector2(StoneRadius, StoneRadius), Vector2(0.5, 0.5), Vector4(1, 1, 1, 1));
		}
		else
		{
			int id = (color == StoneColor::eBlack) ? TextureId::eBlockStone : TextureId::eWhiteStone;	
			{
				GL_BIND_LOCK_OBJECT(mTextures[id]);

				glColor4f(0, 0, 0, 0.2);
				DrawUtility::Sprite(pos + Vector2(2, 2), 2 * Vector2(StoneRadius, StoneRadius), Vector2(0.5, 0.5));

				glColor3f(1, 1, 1);
				DrawUtility::Sprite(pos, 2 * Vector2(StoneRadius, StoneRadius), Vector2(0.5, 0.5));

			}
		}
#else
		RenderUtility::SetPen(g, Color::eBlack);
		RenderUtility::SetBrush(g, (color == StoneColor::eBlack) ? Color::eBlack : Color::eWhite);
		g.drawCircle(pos, StoneRadius);
		if( color == StoneColor::eBlack )
		{
			RenderUtility::SetBrush(g, Color::eWhite);
			g.drawCircle(pos + Vec2i(5, -5), 3);
		}
#endif
	}

}//namespace Go