#pragma once
#ifndef SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D
#define SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D

#include "SBlocksCore.h"

namespace SBlocks
{
	class Solver
	{
	public:
		struct PieceState;
		struct LockedState;

		void setup(Level& level);
		bool solve();
		void getSolvedState(std::vector< LockedState >& outStates)
		{
			for (auto& state : mPieceStates)
			{
				CHECK(state.index != INDEX_NONE);
				outStates.push_back(state.possibleStates[state.index]);
			}
		}

		bool advanceState(PieceState& state);


		MarkMap mMap;
		struct LockedState
		{
			Vec2i pos;
			uint8 dir;
		};
		struct PieceState
		{
			Piece* piece;
			std::vector< LockedState > possibleStates;
			int index;
		};
		std::vector< PieceState > mPieceStates;
	};


}// namespace SBlocks

#endif // SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D