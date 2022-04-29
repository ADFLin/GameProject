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
		bool bTestConnectTilesCount;
		bool bTestConnectTileShape;
		bool bEnableIdenticalShapeCombination;

		SolveOption()
		{
			bEnableRejection = true;
			bTestConnectTilesCount = true;
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

		int advance(int nStep)
		{
			int step = 0;
			for( ; step < nStep && mbValid; ++step)
				mbValid = NextCombination(mStates.begin(), mStates.begin() + mTakeCount, mStates.end());

			return step;
		}

		int  getStateNum() const
		{
			int result = 1;
			int num = mStates.size();
			for (int i = 1; i <= mTakeCount; ++i)
			{
				result *= num;
				result /= i;
				--num;
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
		std::vector< Int16Point2D > outerConPosListMap[DirType::RestValue];

		bool checkFixedRotation(int* outRotation)
		{
			int rotation = INDEX_NONE;
			for (auto pieceData : pieces)
			{
				if (pieceData->piece->bCanRoate)
					return false;
				if (rotation == INDEX_NONE)
					rotation = pieceData->piece->dir;
				else if (rotation != pieceData->piece->dir)
					return false;
			}
			*outRotation = rotation;
			return true;
		}
	};

	namespace ERejectResult
	{
		enum Type
		{
			None,
			ConnectTilesCount,
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

		SolveOption mUsedOption;
	};


	struct MapSolveData
	{
		MarkMap mMap;
		struct TestFrame
		{
			uint32 master;
			uint32 sub;
		};

		TGrid2D<TestFrame> mTestFrameMap;
		uint32  mSubTestFrame = 0;
		uint32* mTestFramePtr;
		int  mMaxCount;
		PieceShapeData mCachedShapeData;

		void setup(MarkMap const& map, SolveOption const& option);
		void copyFrom(MapSolveData const& rhs);
		int countConnectTiles(Vec2i const& pos);

		int countConnectTilesRec(Vec2i const& pos);

		void getConnectedTilePosList(Vec2i const& pos);


	};
	struct SolveData
	{
		GlobalSolveData* globalData;
		uint32 mTestFrame = 0;

		std::vector<int> stateIndices;
		std::vector< MapSolveData > mMaps;
		std::vector< StateCombination > mCombinations;

		void getSolvedStates(std::vector< PieceSolveState >& outStates) const;

		void copyFrom(SolveData const& rhs);

		struct ShapeTest
		{
			Vec2i pos;
			int   index;
			MapSolveData* mapData;
		};
		std::vector< ShapeTest > mCachedPendingTests;

		void setup(Level& level);
		bool advanceState(ShapeSolveData& shapeSolveData, int indexPiece, int& outUsedStateCount);
		bool advanceCombinedState(ShapeSolveData& shapeSolveData, int indexPiece, int& outUsedStateCount);

		int getPieceStateCount(int indexPiece);

		template< typename TFunc >
		ERejectResult::Type testRejection(
			MapSolveData& mapData, Vec2i const pos, std::vector< Int16Point2D > const& outerConPosList, 
			int maxCompareShapeSize, TFunc& CheckPieceFunc);

		template< typename TFunc >
		ERejectResult::Type testRejection(
			ShapeSolveData& shapeSolveData, int indexPiece,
			int maxCompareShapeSize, TFunc& CheckPieceFunc);

		ERejectResult::Type testRejectionInternal(MapSolveData &mapData, Vec2i const& testPos, int maxCompareShapeSize);

		template< typename TFunc >
		ERejectResult::Type runPendingShapeTest(TFunc& CheckPieceFunc);
	};

	class ISolver
	{
	public:
		virtual ~ISolver() {};
	};


	class Solver : public GlobalSolveData
	{
	public:
		~Solver()
		{
			cleanupSolveWork();
		}

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


		int  solveImpl(SolveData& solveData, int indexPiece);

		int  solvePart(SolveData& solveData, int indexPiece, PartWorkInfo& partWork);


		template< bool bEnableRejection , bool bEnableIdenticalShapeCombination, bool bHavePartWork >
		static int  SolveImplT(SolveData& solveData, int indexPiece, PartWorkInfo* partWork);
		using SolveFunc = decltype(&Solver::SolveImplT<false,false,false>);


		void getSolvedStates(std::vector< PieceSolveState >& outStates) const
		{
			return mSolveData.getSolvedStates(outStates);
		}

		template< bool bHavePartWork >
		static SolveFunc GetSolveFunc(SolveOption const& option);

		SolveData   mSolveData;
		SolveFunc   mUsedSolveFunc;
		SolveFunc   mUsedSolvePartFunc;

		std::vector< SolveWork* > mParallelWorks;
		TLockFreeFIFOList<std::vector< PieceSolveState > > mSolutionList;

		void cleanupSolveWork()
		{
			for (auto work : mParallelWorks)
			{
				delete work;
			}
			mParallelWorks.clear();
		}

		void solveParallel(int numThreads);
		void waitWorkCompletion();

	};



}// namespace SBlocks

#endif // SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D