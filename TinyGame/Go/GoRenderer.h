#pragma once
#ifndef GoRenderer_H_63C72169_FDB0_4E4A_B43E_C3F1E1E35D09
#define GoRenderer_H_63C72169_FDB0_4E4A_B43E_C3F1E1E35D09

#include "RHI/TextureAtlas.h"

#include "RandomUtility.h"
#include "Go/GoCore.h"

class RHIGraphics2D;

namespace Render
{
	class SimpleRenderState;
}

namespace GoCore
{
	using namespace Render;

	class BoardBase;
	struct PlayVertex;

	struct RenderContext
	{
		Vector2 renderPos;
		float   scale;
		BoardBase const&  board;
		float   cellLength;
		float   stoneRadius;
		float   starRadius;

		RenderContext(BoardBase const& inBoard, Vector2 inRenderPos = Vector2(0, 0), float inScale = 1.0f)
			:board(inBoard)
			, renderPos(inRenderPos)
			, scale(inScale)
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
			return renderPos + cellLength * Vector2(float(i), board.getSize() - 1.0f - float(j));
		}
		Vec2i getCoord(Vector2 const& pos) const
		{
			return CalcCoordInternal(pos - renderPos, cellLength, board.getSize());
		}

		static Vec2i CalcCoord(Vector2 const& coordPos, Vector2 const& renderPos, float scale, int boradSize)
		{
			float cellLength = DefalutCellLength * scale;
			return CalcCoordInternal(coordPos - renderPos, cellLength, boradSize);
		}

		static Vec2i CalcCoordInternal(Vector2 const& offset, float cellLength, int boradSize)
		{
			Vec2i result = (offset + 0.5 * Vector2(cellLength, cellLength)) / cellLength;
			result.y = boradSize - 1 - result.y;
			return result;
		}
	};

	class BoardRendererBase
	{
	public:

		bool bUseBatchedRender = true;
		bool bDrawStar = true;
		bool bDrawCoord = true;
		bool bUseNoiseOffset = true;

		bool bDrawStepNum = true;


		bool initializeRHI();
		void releaseRHI();

		Vector2 getNoiseOffset(int i, int j, int boradSize)
		{
			return mNoiseOffsets[(i * boradSize + j) % mNoiseOffsets.size()];
		}

		BoardRendererBase()
		{
			mNoiseOffsets.resize(1, Vector2::Zero());
		}

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
			Vector2 uvMin;
			Vector2 uvMax;
			Vector2 uvSize;
		};
		TextureInfo mTexInfos[NumTexture];
		Render::RHITexture2DRef mTextures[NumTexture];

		TArray< Vector2 > mNoiseOffsets;
		struct Vertex
		{
			Vector2 pos;
			Vector4 color;
			Vector2 tex;
		};

		void generateNoiseOffset(int boradSize);

		void drawStoneSequence(RHIGraphics2D& g, SimpleRenderState& renderState, RenderContext const& context, TArray<PlayVertex> const& vertices, int colorStart, float opacity);
		void drawStoneNumber(SimpleRenderState& renderState, RenderContext const& context, int number);


		Vector2 getStonePos(RenderContext const& context, int i, int j);

		void drawBorad(RHIGraphics2D& g, SimpleRenderState& renderState, RenderContext const& context, int const* overrideStoneState = nullptr);

		void drawStone(RHIGraphics2D& g, SimpleRenderState& renderState, Vector2 const& pos, int color, float stoneRadius, float scale, float opaticy = 1.0);
	};
}

namespace Go
{
	using namespace GoCore;

	class Board;
	class Game;

	class BoardRenderer : public BoardRendererBase
	{
	public:

		bool bDrawLinkInfo = false;
		void draw(RHIGraphics2D& g, SimpleRenderState& renderState, RenderContext const& context, int const* overrideStoneState = nullptr);
	};

} //namespace Go

#endif // GoRenderer_H_63C72169_FDB0_4E4A_B43E_C3F1E1E35D09
