#pragma once
#ifndef SBlocksLevel_H_F2AD7DBB_23F0_4FA5_B180_E1C35A7AED48
#define SBlocksLevel_H_F2AD7DBB_23F0_4FA5_B180_E1C35A7AED48

#include "SBlocksCore.h"

#include "Core/Color.h"
#include "Renderer/RenderTransform2D.h"

namespace SBlocks
{
	struct SceneTheme
	{
		Color3ub pieceBlockColor;
		Color3ub pieceBlockLockedColor;
		Color3ub pieceOutlineColor;

		Color3ub mapEmptyColor;
		Color3ub mapBlockColor;
		Color3ub mapOuterColor;
		Color3ub mapFrameColor;

		Color3ub backgroundColor;

		Color3ub shadowColor;
		float    shadowOpacity;
		Vector2  shadowOffset;

		SceneTheme()
		{
			pieceBlockColor = Color3ub(203, 105, 5);
			pieceBlockLockedColor = Color3ub(255, 155, 18);

			mapEmptyColor = Color3ub(165, 82, 37);
			mapBlockColor = Color3ub(126, 51, 15);
			mapOuterColor = Color3ub(111, 39, 9);

			mapFrameColor = Color3ub(222, 186, 130);

			backgroundColor = Color3ub(203, 163, 106);

			shadowColor = Color3ub(20, 20, 20);
			shadowOpacity = 0.5;
			shadowOffset = Vector2(0.2, 0.2);
		}

		template< class OP >
		void serialize(OP& op)
		{
			op & pieceBlockColor & pieceBlockLockedColor;
			op & mapEmptyColor & mapEmptyColor & mapBlockColor & mapOuterColor;
			op & mapFrameColor;
			op & backgroundColor;
			op & shadowColor & shadowOpacity & shadowOffset;
		}
	};

	class GameLevel : public Level
	{
	public:
		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;

		SceneTheme mTheme;


		void resetRenderParams();
		bool bPiecesOrderDirty;
		TArray< Piece* > mSortedPieces;

		void initialize()
		{
			resetRenderParams();
			refreshPieceList();
		}

		void refreshPieceList()
		{
			mSortedPieces.clear();
			for (int index = 0; index < mPieces.size(); ++index)
			{
				Piece* piece = mPieces[index].get();
				piece->index = index;
				mSortedPieces.push_back(piece);
			}
			bPiecesOrderDirty = true;
		}

		void validatePiecesOrder();


		Piece* getPiece(Vector2 const& pos, Vector2& outHitLocalPos);

		virtual void notifyPieceLocked()
		{


		}
	};

}
#endif // SBlocksLevel_H_F2AD7DBB_23F0_4FA5_B180_E1C35A7AED48