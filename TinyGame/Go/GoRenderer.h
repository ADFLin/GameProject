#pragma once
#ifndef GoRenderer_H_63C72169_FDB0_4E4A_B43E_C3F1E1E35D09
#define GoRenderer_H_63C72169_FDB0_4E4A_B43E_C3F1E1E35D09

#include "RenderGL/GLCommon.h"
#include "RenderGL/TextureAtlas.h"

#include "RandomUtility.h"

#define DRAW_TEXTURE 1

class GLGraphics2D;

namespace Go
{
	using namespace RenderGL;

	class Board;
	class Game;

	class GameRenderer
	{
	public:
		bool bUseBatchedRender = true;
		bool bDrawStar = true;
		bool bUseNoiseOffset = true;


		bool initializeRHI();

		void releaseRHI();

		void generateNoiseOffset(int boradSize);


		Vector2 getNoiseOffset(int i, int j, int boradSize)
		{
			return mNoiseOffsets[(i * boradSize + j) % mNoiseOffsets.size()];
		}

		void draw(Vector2 const& renderPos, Game const& game);

		Vector2 getStonePos(Vector2 const& renderPos, Board const& board, int i, int j);
		void drawBorad(Vector2 const& renderPos, Board const& board);


		void addBatchedSprite(int id, Vector2 pos, Vector2 size, Vector2 pivot, Vector4 color);

		void drawStone(GLGraphics2D& g, Vector2 const& pos, int color);


		float const CellSize = 28;
		float const StoneRadius = (CellSize / 2) * 9 / 10;
		float const StarRadius = 5;

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
