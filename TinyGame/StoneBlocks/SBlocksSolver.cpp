#include "SBlocksSolver.h"

#include "PlatformThread.h"


namespace SBlocks
{


	void Solver::setup(Level& level)
	{
		mMap.copy(level.mMap, true);

		mPieceStates.resize(level.mPieces.size());

		for (int index = 0; index < level.mPieces.size(); ++index)
		{
			Piece* piece = level.mPieces[index].get();

			PieceState& state = mPieceStates[index];
			state.piece = piece;
			state.shape = piece->shape;
			state.possibleStates.clear();
			state.index = INDEX_NONE;

#if 1
			int dirs[4];
			int numDir = state.shape->getDifferentShapeDirs(dirs);
			for( int i = 0 ; i < numDir; ++i )
			{
				uint8 dir = dirs[i];
#else
			for (int dir = 0; dir < DirType::RestNumber; ++dir)
			{
#endif
				PieceShape::Data& shapeData = state.shape->mDataMap[dir];
				Vec2i posMax = mMap.getBoundSize() - shapeData.boundSize;
				for (int y = 0; y <= posMax.y; ++y)
				{
					for (int x = 0; x <= posMax.x; ++x)
					{
						Vec2i pos = { x, y };

						if (mMap.canLockAssumeInBound(pos, shapeData))
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

	int Solver::solveImpl(int index, int startIndex)
	{
		while (index >= startIndex)
		{
			PieceState& state = mPieceStates[index];
			if (AdvanceState(mMap, state, state.index))
			{
				++index;
				if (index == mPieceStates.size())
					break;
			}
			else
			{
				state.index = INDEX_NONE;
				--index;
			}
		}
		return index;
	}

	bool Solver::AdvanceState(MarkMap& map, PieceState& state, int& inoutIndex)
	{
		if (inoutIndex != INDEX_NONE)
		{
			LockedState const& lockState = state.possibleStates[inoutIndex];
			map.unlock(lockState.pos, state.shape->mDataMap[lockState.dir]);

		}

		++inoutIndex;
		while (inoutIndex != state.possibleStates.size())
		{
			LockedState const& lockState = state.possibleStates[inoutIndex];
			if (map.tryLockAssumeInBound(lockState.pos, state.shape->mDataMap[lockState.dir]))
			{
				return true;
			}

			++inoutIndex;
		}
		return false;
	}

	struct SolveWork : public RunnableThreadT< SolveWork >
	{
	public:
		SolveWork()
		{
		}

		unsigned run()
		{

		}
		void exit()
		{
			delete this;
		}
	};

	void Solver::solveParallel(int numTheads)
	{

	}



}//namespace SBlocks