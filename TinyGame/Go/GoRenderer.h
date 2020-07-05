#pragma once
#ifndef GoRenderer_H_63C72169_FDB0_4E4A_B43E_C3F1E1E35D09
#define GoRenderer_H_63C72169_FDB0_4E4A_B43E_C3F1E1E35D09

#include "RHI/OpenGLCommon.h"
#include "RHI/TextureAtlas.h"

#include "RandomUtility.h"
#include "Go/GoCore.h"

#define DRAW_TEXTURE 1

class RHIGraphics2D;

namespace Render
{
	class SimpleRenderState;
}

namespace Go
{
	using namespace Render;

	class Board;
	class Game;
	struct PlayVertex;

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

		static float constexpr DefalutCellLength = 28;
		static float constexpr DefalutStoneRadius = (DefalutCellLength / 2) * 11 / 12;
		static float constexpr DefalutStarRadius = 5;

		static float CalcDefalutSize(int boradSize)
		{
			return (boradSize - 1) * DefalutCellLength;
		}

		Vector2 getIntersectionPos(int i, int j) const
		{
			return renderPos + cellLength * Vector2(i, board.getSize() - 1 - j);
		}
		Vec2i getCoord( Vector2 const& pos ) const
		{
			Vec2i result = (pos - renderPos + 0.5 * Vector2(cellLength, cellLength) ) / cellLength;
			result.y = board.getSize() - 1 - result.y;
			return result;
		}
		static Vec2i CalcCoord(Vector2 const& coordPos , Vector2 const& renderPos , float scale , int boradSize )
		{
			float cellLength = DefalutCellLength * scale;
			Vec2i result = (coordPos - renderPos + 0.5 * Vector2(cellLength, cellLength)) / cellLength;
			result.y = boradSize - 1 - result.y;
			return result;
		}
	};

	class BoardRenderer
	{
	public:
		bool bUseBatchedRender = true;
		bool bDrawStar = true;
		bool bDrawCoord = true;
		bool bUseNoiseOffset = true;
		bool bDrawLinkInfo = false;
		bool bDrawStepNum = true;
		BoardRenderer()
		{
			mNoiseOffsets.resize(1, Vector2::Zero());
		}

		bool initializeRHI();

		void releaseRHI();

		void generateNoiseOffset(int boradSize);


		Vector2 getNoiseOffset(int i, int j, int boradSize)
		{
			return mNoiseOffsets[(i * boradSize + j) % mNoiseOffsets.size()];
		}

		void drawStoneSequence(SimpleRenderState& renderState, RenderContext const& context, std::vector<PlayVertex> const& vertices , int colorStart , float opacity);
		void drawStoneNumber(SimpleRenderState& renderState, RenderContext const& context, int number);


		Vector2 getStonePos(RenderContext const& context, int i, int j);

		void drawBorad(RHIGraphics2D& g, SimpleRenderState& renderState, RenderContext const& context, int const* overrideStoneState = nullptr );

		void addBatchedSprite(int id, Vector2 pos, Vector2 size, Vector2 pivot, Vector4 color);

		void drawStone(RHIGraphics2D& g, SimpleRenderState& renderState, Vector2 const& pos, int color, float stoneRadius, float scale, float opaticy = 1.0 );



		TextureAtlas mTextureAtlas;

		enum TextureId
		{
			eBlockStone,
			eWhiteStone,
			eBoardA,

			NumTexture,
		};
		struct TextureInfo
		{
			Vector2 uvMin, uvMax;
		};
		TextureInfo mTexInfos[NumTexture];
		Render::RHITexture2DRef mTextures[NumTexture];

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
