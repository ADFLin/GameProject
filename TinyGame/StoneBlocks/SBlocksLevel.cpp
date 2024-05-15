#include "SBlocksLevel.h"

#include "GameGlobal.h"

namespace SBlocks
{

	void GameLevel::resetRenderParams()
	{
		constexpr float BlockLen = 32;
		constexpr Vector2 BlockSize = Vector2(BlockLen, BlockLen);

		if (mMaps.empty())
		{
			mWorldToScreen = RenderTransform2D(BlockSize, Vector2::Zero());
		}
		else
		{
			Vector2 screenPos = 0.5 * (Vector2(::Global::GetScreenSize()) - BlockSize.mul(Vector2(mMaps[0].getBoundSize())));
			if (mMaps.size() == 1)
			{
				screenPos = 0.5 * (Vector2(::Global::GetScreenSize()) - BlockSize.mul(Vector2(mMaps[0].getBoundSize())));
			}
			else
			{
				Math::TAABBox< Vector2 > aabb;
				aabb.invalidate();

				for (auto& map : mMaps)
				{
					aabb.addPoint(map.mPos);
					aabb.addPoint(map.mPos + Vector2(map.getBoundSize()));
				}

				screenPos = 0.5 * (Vector2(::Global::GetScreenSize()) - BlockSize.mul(aabb.getSize()));
			}

			mWorldToScreen = RenderTransform2D(BlockSize, screenPos);
		}

		mScreenToWorld = mWorldToScreen.inverse();
	}

	void GameLevel::validatePiecesOrder()
	{
		if (bPiecesOrderDirty)
		{
			bPiecesOrderDirty = false;
			std::sort(mSortedPieces.begin(), mSortedPieces.end(),
				[](Piece* lhs, Piece* rhs) -> bool
				{
					if (lhs->isLocked() == rhs->isLocked())
						return lhs->clickFrame < rhs->clickFrame;
					return lhs->isLocked();
				}
			);
		}
	}

	Piece* GameLevel::getPiece(Vector2 const& pos, Vector2& outHitLocalPos)
	{
		validatePiecesOrder();
		for (int index = mSortedPieces.size() - 1; index >= 0; --index)
		{
			Piece* piece = mSortedPieces[index];
			if (piece->hitTest(pos, outHitLocalPos))
				return piece;
		}
		return nullptr;
	}

}

