#pragma once
#ifndef SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D
#define SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D

#include "SBlocksCore.h"
#include "Algo/Combination.h"
#include "Core/LockFreeList.h"
#include "Bitset.h"
#include "BitUtility.h"

namespace SBlocks
{
	class SolveWork;

	struct SolveOption
	{
		bool bEnableSortPiece;
		bool bEnableRejection;
		bool bUseSubsetCheck;
		bool bTestConnectTilesCount;
		bool bTestConnectTileShape;
		bool bEnableIdenticalShapeCombination;
		bool bCheckMapSymmetry;
		bool bReserveLockedPiece;
		bool bUseMapMask;

		SolveOption()
		{
			bEnableRejection = true;
			bUseSubsetCheck = false;
			bTestConnectTilesCount = true;
			bTestConnectTileShape = true;
			bEnableIdenticalShapeCombination = true;
			bCheckMapSymmetry = false;
			bReserveLockedPiece = false;
			bUseMapMask = true;
		}
	};

#if 1
	using MapMask = uint64;
	FORCEINLINE MapMask IndexMask(int index)
	{
		return BIT64(index);
	}
	FORCEINLINE bool Intersection(MapMask const& lhs, MapMask const& rhs)
	{
		return !!(lhs & rhs);
	}
	FORCEINLINE bool IsSet(MapMask const& lhs, int index)
	{
		return !!(lhs & IndexMask(index));
	}
	FORCEINLINE void SetIndex(MapMask& lhs, int index)
	{
		lhs |= IndexMask(index);
	}
	FORCEINLINE bool Extract(MapMask& lhs, int& outIndex)
	{
		return FBitUtility::IterateMask64<64>(lhs, outIndex);
	}
	FORCEINLINE bool TestAndSet(MapMask const& test, MapMask& set, int index)
	{
		uint64 mask = IndexMask(index);
		if (test & mask)
			return false;

		set |= mask;
		return true;
	}
#else
	using MapMask = TBitSet<1 , uint64>;
	FORCEINLINE MapMask IndexMask(int index)
	{
		return MapMask::FromIndex(index);
	}
	FORCEINLINE bool Intersection(MapMask const& lhs, MapMask const& rhs)
	{
		return lhs.testIntersection(rhs);
	}
	FORCEINLINE bool IsSet(MapMask const& lhs, int index)
	{
		return lhs.test(index);
	}
	FORCEINLINE bool TestAndSet(MapMask const& test, MapMask& set, int index)
	{
		if (test.test(index))
			return false;

		set.add(index);
		return true;
	}
#endif

	struct PieceSolveState
	{
		int    mapIndex;
		int    posIndex;
		Vec2i  pos;
		MapMask mapMask;
		PieceShape::OpState op;
		uint8 indexData;
	};

	struct ShapeSolveData;
	struct PieceSolveData
	{
		int             index;
		int             lockedIndexData;
		Piece*          piece;
		PieceShape*     shape;
		ShapeSolveData* shapeSolveData;

		PieceSolveData* sizeLink;
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
			int num = (int)mStates.size();
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
		TArray<int> mStates;
	};

	struct ShapeSolveData
	{
		PieceShape* shape;
		int indexCombination;
		TArray< PieceSolveData* > pieces;
		TArray< PieceSolveState > states;
		TArray< TArray< Int16Point2D > > outerConPosListMap;

		PieceShapeData const& getShapeData( PieceSolveState const& state ) const
		{ 
			return shape->getDataByIndex(state.indexData);
		}

		bool checkFixedState(PieceShape::OpState& outState)
		{
			bool bOutStateSetted = false;
			for (auto pieceData : pieces)
			{
				if (pieceData->piece->bCanOperate)
					return false;
				if (!bOutStateSetted)
				{
					outState.dir = pieceData->piece->dir;
					outState.mirror = pieceData->piece->mirror;
				}
				else if (outState.dir != pieceData->piece->dir || outState.mirror != pieceData->piece->mirror)
					return false;
			}
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
		TArray< PieceSolveData > mPieceList;
		TArray< ShapeSolveData > mShapeList;

		TArray< PieceSolveData* > mPieceSizeMap;
		int mMinShapeBlockCount;
		int mMaxShapeBlockCount;

		SolveOption mUsedOption;

		uint64      mTimeStart;
		bool        bLogProgress = false;
	};

	struct MapSolveData
	{
		MarkMap mMap;
		MapMask mMapMask = 0;
		struct TestFrame
		{
			uint32 master;
			uint32 sub;
		};

		bool    bSymmetry = false;
		TGrid2D<TestFrame> mTestFrameMap;
		uint32  mSubTestFrame = 0;
		uint32* mTestFramePtr;
		int     mMaxCount;
		PieceShape::InitData mCachedShapeData;
		TArray<Vec2i, FixedSizeAllocator> mCachedQueryList;

		void setup(MarkMap const& map, SolveOption const& option);
		void copyFrom(MapSolveData const& rhs);
		int countConnectedTiles(Vec2i const& pos);
		int countConnectedTilesMask(Vec2i const& pos);
		int countConnectedTilesMask(Vec2i const& pos, MapMask const& visitedMask, MapMask& outMask);
		int countConnectedTilesRec(Vec2i const& pos);

		void getConnectedTilePosList(Vec2i const& pos);
		void getConnectedTilePosListMask(Vec2i const& pos);
		MapMask calcMapMask() const;
		MapMask calcMask(PieceShape* shape, PieceSolveState const& state) const;
	};

	struct SolveData
	{
		GlobalSolveData* globalData;
		uint32 mTestFrame = 0;

		TArray<int> stateIndices;
		TArray<MapSolveData> mMaps;
		TArray<StateCombination> mCombinations;

		void getSolvedStates(TArray< PieceSolveState >& outStates) const;

		void copyFrom(SolveData const& rhs);

		struct ShapeTest
		{
			Vec2i pos;
			int   index;
			MapSolveData* mapData;
			MapMask mask;
		};
		TArray<ShapeTest> mCachedPendingTests;
		int mMaxTestShapeSize;
		int mMaxRejectTestIndex;

		void setMaxTestShapeSize(int size)
		{
			mMaxTestShapeSize = size;
			for (auto& map : mMaps)
			{
				map.mMaxCount = mMaxTestShapeSize;
			}
		}

		void setup(Level& level);
		template< bool bUseMapMask >
		bool advanceState(ShapeSolveData& shapeSolveData, int indexPiece, int& outUsedStateCount);
		template< bool bUseMapMask >
		bool advanceCombinedState(ShapeSolveData& shapeSolveData, int indexPiece, int& outUsedStateCount);

		int getPieceStateCount(int indexPiece);

		template< bool bUseMapMask >
		ERejectResult::Type testRejection(MapSolveData& mapData, Vec2i const pos, TArray< Int16Point2D > const& outerConPosList);

		ERejectResult::Type testRejectionMask(MapSolveData& mapData, Vec2i const pos, TArray< Int16Point2D > const& outerConPosList);
		template< bool bUseMapMask >
		ERejectResult::Type testRejection(ShapeSolveData& shapeSolveData, int indexPiece);
		template< bool bUseMapMask >
		ERejectResult::Type testRejectionInternal(MapSolveData &mapData, Vec2i const& testPos);

		ERejectResult::Type testRejectionMaskInternal(MapSolveData &mapData, Vec2i const& testPos, MapMask& visitedMask);

		template< bool bUseSubsetCheck, bool bUseMapMask, typename TFunc >
		ERejectResult::Type runPendingShapeTest(TFunc& CheckPieceFunc);
	};

	class ISolver
	{
	public:
		virtual ~ISolver() = default;
		virtual void setup(Level& level, SolveOption const& option){}
		virtual bool solve() { return false; }
		virtual bool solveNext() { return false; }
		virtual int  solveAll() { return 0; }
		virtual void solveParallel(int numThreads) {}

		enum EType
		{
			eDFS,
			eDLX,
		};
		ISolver* Create(EType type);
	};


	class Solver : public ISolver
		         , public GlobalSolveData
	{
	public:
		~Solver()
		{
			cleanupSolveWork();
		}

		void setup(Level& level, SolveOption const& option = SolveOption());
		bool solve();
		bool solveNext();

		int  solveAll();

		int  solveDLX(bool bRecursive);

		struct PartWorkInfo
		{
			int indexPieceWork;
			int indexStateStart;
			int numStates;
			int numStatesUsed;
		};

		int  solveImpl(SolveData& solveData, int indexPiece);

		int  solvePart(SolveData& solveData, int indexPiece, PartWorkInfo& partWork);


		template< bool bEnableRejection , bool bEnableIdenticalShapeCombination, bool bUseSubsetCheck, bool bUseMapMask, bool bHavePartWork >
		static int  SolveImplT(SolveData& solveData, int indexPiece, PartWorkInfo* partWork);
		template< bool bEnableRejection, bool bEnableIdenticalShapeCombination, bool bUseSubsetCheck, bool bUseMapMask >
		static void SolveAllImplT(Solver& solver, SolveData& solveData);

		using SolveFunc = decltype(&Solver::SolveImplT<false,false,false, false, false>);
		using SolveAllFunc = decltype(&Solver::SolveAllImplT<false, false, false, false>);

		void getSolvedStates(TArray< PieceSolveState >& outStates) const
		{
			return mSolveData.getSolvedStates(outStates);
		}

		static SolveFunc GetSolveFunc(SolveOption const& option, bool bHavePartWork);
		static SolveAllFunc GetSolveAllFunc(SolveOption const& option);
		SolveData   mSolveData;
		SolveFunc   mUsedSolveFunc;
		SolveFunc   mUsedSolvePartFunc;
		SolveAllFunc   mUsedSolveAllFunc;
		TArray< SolveWork* > mParallelWorks;
		TLockFreeFIFOList<TArray< PieceSolveState > > mSolutionList;
		int         mSolutionCount = 0;

		void handleWorkFinish(SolveWork* work);

		void cleanupSolveWork();

		void solveParallel(int numThreads);
		void waitWorkCompletion();

	};



}// namespace SBlocks

#endif // SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D