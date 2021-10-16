#include "SBlocksSolver.h"

namespace SBlocks
{


	void Solver::setup(Level& level)
	{
		mMap.copy(level.mMap);
		mPieceStates.resize(level.mPieces.size());

		for (int index = 0; index < level.mPieces.size(); ++index)
		{
			Piece* piece = level.mPieces[index].get();

			PieceState& state = mPieceStates[index];
			state.piece = piece;
			state.possibleStates.clear();
			state.index = INDEX_NONE;

			for (uint8 dir = 0; dir < DirType::RestNumber; ++dir)
			{
				PieceShape::Data& shapeData = piece->shape->mDataMap[dir];
				Vec2i posMax = mMap.getBoundSize() - shapeData.boundSize;
				for (int y = 0; y <= posMax.y; ++y)
				{
					for (int x = 0; x <= posMax.x; ++x)
					{
						Vec2i pos = { x, y };

						if (mMap.canLock(pos, shapeData))
						{
							LockedState lockState;
							lockState.pos = pos;
							lockState.dir = dir;
							state.possibleStates.push_back(lockState);
						}
					}
				}
			}
		}
	}

	bool Solver::solve()
	{
		int index = 0;
		while (index >= 0)
		{
			PieceState& state = mPieceStates[index];
			if (advanceState(state))
			{
				++index;
				if (index == mPieceStates.size())
					return true;
			}
			else
			{
				state.index = INDEX_NONE;
				--index;
			}
		}
		return false;
	}

	bool Solver::advanceState(PieceState& state)
	{
		if (state.index != INDEX_NONE)
		{
			LockedState const& lockState = state.possibleStates[state.index];
			mMap.unlock(lockState.pos, state.piece->shape->mDataMap[lockState.dir]);
		}

		++state.index;
		while (state.index != state.possibleStates.size())
		{
			LockedState const& lockState = state.possibleStates[state.index];
			if (mMap.tryLock(lockState.pos, state.piece->shape->mDataMap[lockState.dir]))
			{
				return true;
			}
			++state.index;
		}
		return false;
	}

}//namespace SBlocks