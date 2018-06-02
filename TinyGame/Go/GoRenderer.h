#pragma once
#ifndef GoRenderer_H_63C72169_FDB0_4E4A_B43E_C3F1E1E35D09
#define GoRenderer_H_63C72169_FDB0_4E4A_B43E_C3F1E1E35D09

#include "RHI/GLCommon.h"
#include "RHI/TextureAtlas.h"

#include "RandomUtility.h"

#define DRAW_TEXTURE 1

class GLGraphics2D;

namespace Go
{
	using namespace RenderGL;

	class Board;
	class Game;

	struct RenderContext
	{
		Vector2 renderPos;
		float   scale;
		Board const&  board;
		float   cellLength;
		float   stoneRadius;
		float   starRadius;

		RenderContext( Board const& inBoard , Vector2 inRenderPos = Vector2(0,0) , float inScale = 1.0f )
			:board(inBoard)
			,renderPos( inRenderPos )
			,scale(inScale)
		{
			cellLength = DefalutCellLength * scale;
			stoneRadius = DefalutStoneRadius * scale;
			starRadius = DefalutStarRadius * scale;
		}

		float const DefalutCellLength = 28;
		float const DefalutStoneRadius = (DefalutCellLength / 2) * 11 / 12;
		float const DefalutStarRadius = 5;

		Vector2 getIntersectionPos(int i, int j) const
		{
			return renderPos + cellLength * Vector2(i, j);
		}
		Vec2i getCoord( Vector2 const& pos ) const
		{
			return (pos - renderPos + 0.5 * Vector2(cellLength, cellLength) ) / cellLength;
		}
	};

	class BoardRenderer
	{
	public:
		bool bUseBatchedRender = true;
		bool bDrawStar = true;
		bool bUseNoiseOffset = true;
		bool bDrawLinkInfo = false;
		bool bDrawStepNum = true;

		bool initializeRHI();

		void releaseRHI();

		void generateNoiseOffset(int boradSize);


		Vector2 getNoiseOffset(int i, int j, int boradSize)
		{
			return mNoiseOffsets[(i * boradSize + j) % mNoiseOffsets.size()];
		}

		void drawStoneSequence(RenderContext const& context, std::vector<int> const& vertices , int colorStart , float opacity);
		void drawStoneNumber(RenderContext const& context, int number)
		{

		}

		Vector2 getStonePos(RenderContext const& context, int i, int j);
		Vector2 getIntersectionPos(RenderContext const& context, int i, int j);
		void drawBorad(GLGraphics2D& g, RenderContext const& context);

		void addBatchedSprite(int id, Vector2 pos, Vector2 size, Vector2 pivot, Vector4 color);

		void drawStone(GLGraphics2D& g ,Vector2 const& pos, int color , float stoneRadius , float opacity = 1.0f);




		TextureAtlas mTextureAtlas;

		enum TextureId
		{
			eBlockStone,
			eWhiteStone,
			eBoardA,

			NumTexture,
		};
		RenderGL::RHITexture2D mTextures[NumTexture];

		std::vector< Vector2 > mNoiseOffsets;
		struct Vertex
		{
			Vector2 pos;
			Vector4 color;
			Vector2 tex;
		};

		std::vector< Vertex > mSpriteVertices;
	};


} //namespace Go

#endif // GoRenderer_H_63C72169_FDB0_4E4A_B43E_C3F1E1E35D09
