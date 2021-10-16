#pragma once
#ifndef SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D
#define SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D

#include "SBlocksCore.h"

namespace SBlocks
{
	class SolveWork;

	class Solver
	{
	public:
		struct PieceState;
		struct LockedState;

		void setup(Level& level);
		bool solve()
		{
			return solveImpl(0, 0) >= 0;
		}
		bool solveNext()
		{
			return solveImpl(mPieceStates.size() - 1, 0) >= 0;
		}
		int  solveImpl(int index, int startIndex);

		void getSolvedStates(std::vector< LockedState >& outStates)
		{
			for (auto& state : mPieceStates)
			{
				CHECK(state.index != INDEX_NONE);
				outStates.push_back(state.possibleStates[state.index]);
			}
		}
		static bool AdvanceState(MarkMap& map, PieceState& state, int& inoutIndex);
		struct ThreadData
		{
			int startIndex;
			int numIndices;
			MarkMap map;
			std::vector<int> stateIndices;
		};
		MarkMap mMap;

		void solveParallel(int numTheads);

		struct LockedState
		{
			Vec2i pos;
			uint8 dir;
		};
		struct PieceState
		{
			Piece* piece;
			PieceShape* shape;
			std::vector< LockedState > possibleStates;
			int index;
		};
		
		std::vector< PieceState > mPieceStates;
	};


}// namespace SBlocks

#endif // SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D