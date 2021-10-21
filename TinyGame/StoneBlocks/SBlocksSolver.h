#pragma once
#ifndef SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D
#define SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D

#include "SBlocksCore.h"
#include "Algo/Combination.h"
#include "Core/LockFreeList.h"

namespace SBlocks
{
	class SolveWork;

	struct SolveOption
	{
		bool bEnableSortPiece;
		bool bEnableRejection;
		bool bTestMinConnectTiles;
		bool bTestConnectTileShape;
		bool bEnableIdenticalShapeCombination;

		SolveOption()
		{
			bEnableRejection = true;
			bTestMinConnectTiles = true;
			bTestConnectTileShape = true;
			bEnableIdenticalShapeCombination = true;
		}
	};


	struct PieceSolveState
	{
		int   mapIndex;
		Vec2i pos;
		uint8 dir;
	};

	struct ShapeSolveData;
	struct PieceSolveData
	{
		Piece* piece;
		PieceShape* shape;
		ShapeSolveData* shapeSaveData;

		PieceSolveData* link;
	};

	struct StateCombination
	{
		void init(int m, int n)
		{
			mTakeCount = n;
			mStates.resize(m);
			reset();
		}

		bool isValid() const { return mbValid; }

		void reset()
		{
			mbValid = true;
			for (int i = 0; i < mStates.size(); ++i)
				mStates[i] = i;
		}

		void advance()
		{
			mbValid = NextCombination(mStates.begin(), mStates.begin() + mTakeCount, mStates.end());
		}

		void advance(int n)
		{
			for( int i = 0 ; i < n; ++i)
				mbValid = NextCombination(mStates.begin(), mStates.begin() + mTakeCount, mStates.end());
		}


		int  getStateNum() const
		{
			int result = 1;

			int num = mStates.size();
			for (int i = 0; i < mTakeCount; ++i)
			{
				result *= num;
				--num;
			}

			for (int i = 2; i <= mTakeCount; ++i)
			{
				result /= i;
			}

			return result;
		}

		int const* getStates() const { return mStates.data(); }

		bool mbValid;
		int  mTakeCount;
		std::vector<int> mStates;
	};

	struct ShapeSolveData
	{
		PieceShape* shape;

		int indexCombination;
		std::vector< PieceSolveData* > pieces;
		std::vector< PieceSolveState > states;
		std::vector< Int16Point2D > outerConPosListMap[4];
	};

	namespace ERejectResult
	{
		enum Type
		{
			None,
			MinConnectTiles,
			ConnectTileShape,
		};
	};

	struct GlobalSolveData
	{
		std::vector< PieceSolveData > mPieceList;
		std::vector< ShapeSolveData > mShapeList;

		std::vector< PieceSolveData* > mPieceSizeMap;
		int mMinShapeBlockCount;
		int mMaxShapeBlockCount;
	};


	struct MapSolveData
	{
		MarkMap mMap;
		TGrid2D<int> mTestFrameMap;
		int mTestFrame;
		PieceShapeData mCachedShapeData;

		void setup(Level& level, SolveOption const& option);

		void copyFrom(MapSolveData const& rhs);
		int countConnectTiles(Vec2i const& pos);

		void getConnectedTilePosList(Vec2i const& pos);


	};
	struct SolveData
	{
		std::vector<int> stateIndices;
		std::vector< MapSolveData > mMaps;
		std::vector< StateCombination > mCombinations;

		void getSolvedStates(GlobalSolveData const& globalData, std::vector< PieceSolveState >& outStates) const;

		void copyFrom(SolveData const& rhs);

		struct ShapeTest
		{
			Vec2i pos;
			int   mapIndex;
			MapSolveData* mapData;
		};
		std::vector< ShapeTest > mCachedPendingTests;

		void setup(Level& level, SolveOption const& option);
		bool advanceState(ShapeSolveData& shapeSolveData, int indexPiece, int& outUsedStateCount);
		bool advanceCombinedState(ShapeSolveData& shapeSolveData, int indexPiece, int& outUsedStateCount);


		template< typename TFunc >
		ERejectResult::Type testRejection(
			GlobalSolveData& globalData, MapSolveData& mapData, 
			Vec2i const pos, std::vector< Int16Point2D > const& outerConPosList, 
			SolveOption const& option, int maxCompareShapeSize, 
			TFunc CheckPieceFunc);

		template< typename TFunc >
		ERejectResult::Type testRejection(
			GlobalSolveData& globalData, 
			ShapeSolveData& shapeSolveData, int indexPiece,
			SolveOption const& option, int maxCompareShapeSize, 
			TFunc CheckPieceFunc);

		template< typename TFunc >
		ERejectResult::Type runPendingTest(GlobalSolveData& globalData, TFunc CheckPieceFunc);
	};


	class Solver : public GlobalSolveData
	{
	public:
		void setup(Level& level, SolveOption const& option = SolveOption());
		bool solve()
		{
			return solveImpl(mSolveData, 0) >= 0;
		}
		bool solveNext()
		{
			return solveImpl(mSolveData, mPieceList.size() - 1) >= 0;
		}

		int  solveAll()
		{
			int numSolution = 0;
			if (!solve())
				return 0;

			++numSolution;
			while (solveNext())
			{
				++numSolution;
			}
			return numSolution;
		}

		struct PartWorkInfo
		{
			int indexPieceWork;
			int indexStateStart;
			int numStates;
			int numStatesUsed;
		};

		int  solveImpl(SolveData& solveData, int indexPiece, PartWorkInfo* partWork = nullptr);

		template< bool bEnableRejection , bool bEnableIdenticalShapeCombination >
		int  solveImplT(SolveData& solveData, int indexPiece, PartWorkInfo* partWork);


		void getSolvedStates(std::vector< PieceSolveState >& outStates) const
		{
			return mSolveData.getSolvedStates(*this, outStates);
		}

		int getPieceStateCount(int indexPiece)
		{
			auto const& pieceData = mPieceList[indexPiece];
			auto const& shapeSolveData = *pieceData.shapeSaveData;
			if (mUsedOption.bEnableIdenticalShapeCombination && shapeSolveData.indexCombination != INDEX_NONE)
			{
				return mSolveData.mCombinations[shapeSolveData.indexCombination].getStateNum();
			}
			else
			{
				return shapeSolveData.states.size();
			}
		}

		TLockFreeFIFOList<std::vector< PieceSolveState > > mSolutionList;
		
		SolveData   mSolveData;
		SolveOption mUsedOption;

		std::vector< SolveWork* > mParallelWorks;

		void solveParallel(int numThreads);
		void waitWorkCompilition();

	};



}// namespace SBlocks

#endif // SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D