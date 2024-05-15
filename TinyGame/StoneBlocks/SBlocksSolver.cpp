#include "SBlocksSolver.h"

#include "PlatformThread.h"
#include "LogSystem.h"
#include "Core/ScopeGuard.h"
#include "StdUtility.h"
#include "ProfileSystem.h"
#include "InlineString.h"
#include "TemplateMisc.h"

#include "Algo/DLX.h"

#define SBLOCK_LOG_SOLVE_INFO 0

#define USE_STAT 0

namespace SBlocks
{
	struct Stat
	{
		char const*   name;
		uint64  duration;
		uint32  count;
		Stat(char const* name)
			:name(name)
		{
		}
		void reset()
		{
			count = 0;
			duration = 0;
		}
	};
	struct StatScope
	{
		StatScope(Stat& stat)
			:stat(stat)
		{
			Profile_GetTicks(&startTime);
		}

		~StatScope()
		{
			uint64 endTime;
			Profile_GetTicks(&endTime);
			stat.duration += endTime - startTime;
			stat.count += 1;
		}

		Stat&   stat;
		uint64  startTime;
	};


#if USE_STAT
#define STAT_SCOPE(STAT) StatScope ANONYMOUS_VARIABLE(StatScope)(STAT);
#else
#define STAT_SCOPE(STAT)
#endif

	Stat StatRejectCount("RejectCount");
	Stat StatRejectShape("RejectShape");
	Stat StatFindShape("FindShape");
	Stat StatAdvance("Advance");
	Stat* ReportedStatList[] = { &StatAdvance, &StatRejectCount, &StatRejectShape , &StatFindShape };

	void MapSolveData::setup(MarkMap const& map, SolveOption const& option)
	{
		mMap.copyFrom(map, !option.bReserveLockedPiece);
		bSymmetry = map.isSymmetry();
		if (option.bTestConnectTilesCount)
		{
			mTestFrameMap.resize(map.getBoundSize().x, map.getBoundSize().y);
			mTestFrameMap.fillValue({ 0,0 });
			mSubTestFrame = 0;
		}

		mCachedQueryList.reserve(mTestFrameMap.getRawDataSize());
		mCachedShapeData.blocks.reserve(mTestFrameMap.getRawDataSize());
	}

	void MapSolveData::copyFrom(MapSolveData const& rhs)
	{
		mMap.copyFrom(rhs.mMap);
		mTestFrameMap.resize(rhs.mTestFrameMap.getSizeX(), rhs.mTestFrameMap.getSizeY());
		mTestFrameMap.fillValue({ 0,0 });
		mSubTestFrame = 0;
		bSymmetry = rhs.bSymmetry;
		mMapMask = rhs.mMapMask;

		mCachedQueryList.reserve(mTestFrameMap.getRawDataSize());
		mCachedShapeData.blocks.reserve(mTestFrameMap.getRawDataSize());
	}

	int MapSolveData::countConnectedTiles(Vec2i const& pos)
	{
		if (!mMap.isInBound(pos))
			return 0;

		uint8 data = mMap.getValue(pos);
		if (!MarkMap::CanLock(data))
			return 0;

		auto& frame = mTestFrameMap(pos.x, pos.y);
		if (frame.master == *mTestFramePtr)
			return 0;
		frame.master = *mTestFramePtr;
		frame.sub = mSubTestFrame;

		int result = 1;
#if 1
		mCachedQueryList.clear();
		TArray< Vec2i , FixedSizeAllocator >& queryList = mCachedQueryList;
		queryList.push_back(pos);

		while (!queryList.empty())
		{
			Vec2i tPos = queryList.back();
			queryList.pop_back();

			for (int i = 0; i < DirType::RestValue; ++i)
			{
				Vec2i testPos = tPos + GConsOffsets[i];

				if (!mMap.isInBound(testPos))
					continue;

				uint8 data = mMap.getValue(testPos);
				if (!MarkMap::CanLock(data))
					continue;

				auto& frame = mTestFrameMap(testPos.x, testPos.y);
				if (frame.sub == mSubTestFrame)
					continue;

				frame.master = *mTestFramePtr;
				frame.sub = mSubTestFrame;

				++result;
				queryList.push_back(testPos);
				if (result >= mMaxCount)
					return result;
			}
		}
#else
		for (int i = 0; i < DirType::RestValue; ++i)
		{
			Vec2i testPos = pos + GConsOffsets[i];
			result += countConnectedTilesRec(testPos);
			if (result >= mMaxCount)
				break;
		}
#endif
		return result;
	}

	int MapSolveData::countConnectedTilesMask(Vec2i const& pos)
	{
		{
			if (!mMap.isInBound(pos))
				return 0;

			int index = mMap.toIndex(pos);
			if (IsSet(mMapMask, index))
				return 0;

			auto& frame = mTestFrameMap[index];
			if (frame.master == *mTestFramePtr)
				return 0;

			frame.master = *mTestFramePtr;
			frame.sub = mSubTestFrame;
		}

		int result = 1;
		mCachedQueryList.clear();
		TArray< Vec2i, FixedSizeAllocator >& queryList = mCachedQueryList;

		Vec2i curPos = pos;
		for(;;)
		{
			for (int i = 0; i < DirType::RestValue; ++i)
			{
				Vec2i testPos = curPos + GConsOffsets[i];

				if (!mMap.isInBound(testPos))
					continue;

				int index = mMap.toIndex(testPos);
				if (IsSet(mMapMask, index))
					continue;

				auto& frame = mTestFrameMap[index];
				if (frame.sub == mSubTestFrame)
					continue;

				frame.master = *mTestFramePtr;
				frame.sub = mSubTestFrame;

				++result;
				queryList.push_back(testPos);
				if (result >= mMaxCount)
					return result;
			}

			if (queryList.empty())
				break;

			curPos = queryList.back();
			queryList.pop_back();
		}

		return result;
	}

	int MapSolveData::countConnectedTilesMask(Vec2i const& pos, MapMask const& visitedMask , MapMask& outMask)
	{
		{
			if (!mMap.isInBound(pos))
				return 0;

			int index = mMap.toIndex(pos);
			if (!TestAndSet(visitedMask, outMask, index))
				return 0;
		}

		int result = 1;
		mCachedQueryList.clear();
		TArray< Vec2i, FixedSizeAllocator >& queryList = mCachedQueryList;

		Vec2i curPos = pos;

		auto CheckPos = [&](Vec2i const& testPos)
		{
			if (!mMap.isInBound(testPos))
				return true;

			int index = mMap.toIndex(testPos);
			if (IsSet(mMapMask, index))
				return true;

			if (!TestAndSet(outMask, outMask, index))
				return true;

			++result;
			queryList.push_back(testPos);
			return result < mMaxCount;

		};
		for (;;)
		{
			if (!CheckPos(curPos + GConsOffsets[0]))
				return result;
			if (!CheckPos(curPos + GConsOffsets[1]))
				return result;
			if (!CheckPos(curPos + GConsOffsets[2]))
				return result;
			if (!CheckPos(curPos + GConsOffsets[3]))
				return result;

			if (queryList.empty())
				break;

			curPos = queryList.back();
			queryList.pop_back();
		}

		return result;
	}

	int MapSolveData::countConnectedTilesRec(Vec2i const& pos)
	{
		if (!mMap.isInBound(pos))
			return 0;

		uint8 data = mMap.getValue(pos);
		if (!MarkMap::CanLock(data))
			return 0;

		auto& frame = mTestFrameMap(pos.x, pos.y);
		if (frame.sub == mSubTestFrame)
			return 0;
		frame.master = *mTestFramePtr;
		frame.sub = mSubTestFrame;

		int result = 1;
		for (int i = 0; i < DirType::RestValue; ++i)
		{
			Vec2i testPos = pos + GConsOffsets[i];
			result += countConnectedTilesRec(testPos);
			if (result >= mMaxCount)
				break;
		}
		return result;
	}

	void MapSolveData::getConnectedTilePosList(Vec2i const& pos)
	{
		if (!mMap.isInBound(pos))
			return;

		uint8 data = mMap.getValue(pos);
		if (!MarkMap::CanLock(data))
			return;

		auto& frame = mTestFrameMap(pos.x, pos.y);
		if (frame.master == *mTestFramePtr)
			return;

		frame.master = *mTestFramePtr;
		mCachedShapeData.blocks.push_back(PieceShapeData::Block(pos, MarkMap::GetType(data)));
#if 1
		mCachedQueryList.clear();
		TArray< Vec2i, FixedSizeAllocator >& queryList = mCachedQueryList;
		queryList.push_back(pos);
		while (!queryList.empty())
		{
			Vec2i tPos = queryList.back();
			queryList.pop_back();

			for (int i = 0; i < DirType::RestValue; ++i)
			{
				Vec2i testPos = tPos + GConsOffsets[i];

				if (!mMap.isInBound(testPos))
					continue;

				uint8 data = mMap.getValue(testPos);
				if (!MarkMap::CanLock(data))
					continue;

				auto& frame = mTestFrameMap(testPos.x, testPos.y);
				if (frame.master == *mTestFramePtr)
					continue;

				frame.master = *mTestFramePtr;
				mCachedShapeData.blocks.push_back(PieceShapeData::Block(testPos, MarkMap::GetType(data)));
				queryList.push_back(testPos);
			}
		}
#else
		for (int i = 0; i < DirType::RestValue; ++i)
		{
			Vec2i testPos = pos + GConsOffsets[i];
			getConnectedTilePosList(testPos);
		}
#endif
	}

	void MapSolveData::getConnectedTilePosListMask(Vec2i const& pos)
	{

		MapMask mask = mMapMask;
		{
			CHECK(mMap.isInBound(pos));

			int index = mMap.toIndex(pos);
			CHECK(!IsSet(mMapMask, index));
			SetIndex(mask, index);
			uint8 data = mMap.mData[index];
			mCachedShapeData.blocks.push_back(PieceShapeData::Block(pos, MarkMap::GetType(data)));
		}

		mCachedQueryList.clear();
		TArray< Vec2i , FixedSizeAllocator >& queryList = mCachedQueryList;

		auto TestPos = [&](Vec2i const& testPos)
		{
			if (!mMap.isInBound(testPos))
				return;

			int index = mMap.toIndex(testPos);
			if (!TestAndSet(mask, mask, index))
				return;

			uint8 data = mMap.mData[index];
			mCachedShapeData.blocks.push_back(PieceShapeData::Block(testPos, MarkMap::GetType(data)));
			queryList.push_back(testPos);

		};

		Vec2i curPos = pos;
		for(;;)
		{
			TestPos(curPos + GConsOffsets[0]);
			TestPos(curPos + GConsOffsets[1]);
			TestPos(curPos + GConsOffsets[2]);
			TestPos(curPos + GConsOffsets[3]);

			if (queryList.empty())
				break;

			curPos = queryList.back();
			queryList.pop_back();
		}
	}

	MapMask MapSolveData::calcMapMask() const
	{
		MapMask result = 0;
		for (int index = 0; index < mMap.mData.getRawDataSize(); ++index)
		{
			auto data = mMap.mData[index];
			if (!MarkMap::CanLock(data))
			{
				SetIndex(result, index);
			}
		}
		return result;
	}

	MapMask MapSolveData::calcMask(PieceShape* shape, PieceSolveState const& state) const
	{
		auto const& shapeData = shape->getDataByIndex(state.indexData);

		MapMask result = 0;
		for (auto const& block : shapeData.blocks)
		{
			int index = state.posIndex + mMap.toIndex(block.pos);
			SetIndex(result, index);
		}
		return result;
	}

	void SolveData::getSolvedStates(TArray< PieceSolveState >& outStates) const
	{
		int index = 0;
		for (auto const& pieceData : globalData->mPieceList)
		{
			int indexState = stateIndices[index];
			auto const* shapeSolveData = pieceData.shapeSolveData;
			CHECK(indexState != INDEX_NONE);
			outStates.push_back(shapeSolveData->states[indexState]);
			++index;
		}
	}

	void SolveData::setup(Level& level)
	{
		mMaps.resize(level.mMaps.size());
		for (int i = 0; i < mMaps.size(); ++i)
		{
			mMaps[i].setup(level.mMaps[i], globalData->mUsedOption);
			mMaps[i].mTestFramePtr = &mTestFrame;
		}
		stateIndices.resize(level.mPieces.size(), INDEX_NONE);
	}

	void SolveData::copyFrom(SolveData const& rhs)
	{
		globalData = rhs.globalData;
		stateIndices = rhs.stateIndices;
		mTestFrame = 0;
		mMaps.resize(rhs.mMaps.size());
		for (int i = 0; i < mMaps.size(); ++i)
		{
			mMaps[i].copyFrom(rhs.mMaps[i]);
			mMaps[i].mTestFramePtr = &mTestFrame;
		}
		mCombinations = rhs.mCombinations;
		mMaxRejectTestIndex = rhs.mMaxRejectTestIndex;
		setMaxTestShapeSize(rhs.mMaxTestShapeSize);
	}

	template< bool bUseMapMask >
	bool SolveData::advanceState(ShapeSolveData& shapeSolveData, int indexPiece, int& outUsedStateCount)
	{
		STAT_SCOPE(StatAdvance);
		TGuardLoadStore indexState = { stateIndices[indexPiece] };
		if (indexState >= 0)
		{
			PieceSolveState const& state = shapeSolveData.states[indexState];
			auto& mapData = mMaps[state.mapIndex];
			if constexpr (bUseMapMask)
			{
				mapData.mMapMask &= ~state.mapMask;
			}
			else
			{
				mapData.mMap.unlock(state.posIndex, shapeSolveData.getShapeData(state));
			}

			++indexState;
		}
		else
		{
			indexState = -indexState - 1;
		}

		CHECK(indexState >= 0);
		PieceSolveData& pieceData = globalData->mPieceList[indexPiece];
		while (indexState != shapeSolveData.states.size())
		{
			++outUsedStateCount;
			PieceSolveState const& state = shapeSolveData.states[indexState];
			if (LIKELY(pieceData.lockedIndexData == INDEX_NONE || pieceData.lockedIndexData == state.indexData))
			{
				auto& mapData = mMaps[state.mapIndex];
				if constexpr (bUseMapMask)
				{
					bool canLock = !Intersection(mapData.mMapMask, state.mapMask);
					if (canLock)
					{
						mapData.mMapMask |= state.mapMask;
						return true;
					}
				}
				else
				{
					if (mapData.mMap.tryLockAssumeInBound(state.posIndex, shapeSolveData.getShapeData(state)))
					{
						return true;
					}
				}
			}

			++indexState;
		}

		indexState = INDEX_NONE;
		return false;
	}

	template< bool bUseMapMask >
	bool SolveData::advanceCombinedState(ShapeSolveData& shapeSolveData, int indexPiece, int& outUsedStateCount)
	{
		STAT_SCOPE(StatAdvance);
		CHECK(indexPiece == shapeSolveData.pieces.front()->index);

		StateCombination& sc = mCombinations[shapeSolveData.indexCombination];

		auto UnlockUsedStates = [&]()
		{
			for (int i = 0; i < shapeSolveData.pieces.size(); ++i)
			{
				int& indexState = stateIndices[indexPiece + i];
				if (indexState >= 0)
				{
					PieceSolveState const& state = shapeSolveData.states[indexState];

					auto& mapData = mMaps[state.mapIndex];
					if constexpr (bUseMapMask)
					{
						mapData.mMapMask &= ~state.mapMask;
					}
					else
					{
						mapData.mMap.unlock(state.posIndex, shapeSolveData.getShapeData(state));
					}

					indexState = INDEX_NONE;
				}
			}
		};

		UnlockUsedStates();

		while (sc.isValid())
		{
			++outUsedStateCount;
			int const* pStateIndices = sc.getStates();

			int i = 0;
			for (; i < shapeSolveData.pieces.size(); ++i)
			{
				int indexState = pStateIndices[i];
				PieceSolveState const& state = shapeSolveData.states[indexState];
				PieceSolveData& pieceData = globalData->mPieceList[indexPiece + i];
				if (pieceData.lockedIndexData == INDEX_NONE || pieceData.lockedIndexData == state.indexData)
				{
					auto& mapData = mMaps[state.mapIndex];
					if constexpr (bUseMapMask)
					{
						bool canLock = !Intersection(mapData.mMapMask, state.mapMask);
						if (canLock)
						{
							mapData.mMapMask |= state.mapMask;
							stateIndices[indexPiece + i] = indexState;
						}
						else
						{
							UnlockUsedStates();
							break;
						}
					}
					else
					{
						if (mapData.mMap.tryLockAssumeInBound(state.posIndex, shapeSolveData.getShapeData(state)))
						{
							stateIndices[indexPiece + i] = indexState;
						}
						else
						{
							UnlockUsedStates();
							break;
						}
					}
				}
			}

			sc.advance();
			if (i == shapeSolveData.pieces.size())
			{
				return true;
			}
		}

		sc.reset();
		return false;
	}

	int SolveData::getPieceStateCount(int indexPiece)
	{
		auto const& pieceData = globalData->mPieceList[indexPiece];
		auto const& shapeSolveData = *pieceData.shapeSolveData;
		if (shapeSolveData.indexCombination != INDEX_NONE)
		{
			return mCombinations[shapeSolveData.indexCombination].getStateNum();
		}
		else
		{
			return shapeSolveData.states.size();
		}
	}

	template< bool bUseMapMask >
	ERejectResult::Type SolveData::testRejection(MapSolveData& mapData, Vec2i const pos, TArray< Int16Point2D > const& outerConPosList)
	{
		STAT_SCOPE(StatRejectCount);
		mCachedPendingTests.clear();
		++mTestFrame;
		CHECK(mTestFrame != 0);
		for (auto const& outerPos : outerConPosList)
		{
			Vec2i testPos = pos + outerPos;
			++mapData.mSubTestFrame;
			CHECK(mapData.mSubTestFrame != 0);
			auto testResult = testRejectionInternal<bUseMapMask>(mapData, testPos);
			if (testResult != ERejectResult::None)
				return testResult;
		}
		return ERejectResult::None;
	}


	ERejectResult::Type SolveData::testRejectionMask(MapSolveData& mapData, Vec2i const pos, TArray< Int16Point2D > const& outerConPosList)
	{
		STAT_SCOPE(StatRejectCount);
		mCachedPendingTests.clear();
		MapMask visitedMask = mapData.mMapMask;
		for (auto const& outerPos : outerConPosList)
		{
			Vec2i testPos = pos + outerPos;
			auto testResult = testRejectionMaskInternal(mapData, testPos, visitedMask);
			if (testResult != ERejectResult::None)
				return testResult;
		}
		return ERejectResult::None;
	}

	template< bool bUseMapMask >
	ERejectResult::Type SolveData::testRejection(ShapeSolveData& shapeSolveData, int indexPiece)
	{
		STAT_SCOPE(StatRejectCount);
		mCachedPendingTests.clear();
		++mTestFrame;
		CHECK(mTestFrame != 0);
		for (int i = 0; i < shapeSolveData.pieces.size(); ++i)
		{
			PieceSolveState const& state = shapeSolveData.states[stateIndices[indexPiece + i]];
			auto& mapData = mMaps[state.mapIndex];
			mapData.mMaxCount = mMaxTestShapeSize;
			for (auto const& outerPos : shapeSolveData.outerConPosListMap[state.indexData])
			{
				Vec2i testPos = state.pos + outerPos;
				++mapData.mSubTestFrame;
				CHECK(mapData.mSubTestFrame != 0);
				auto testResult = testRejectionInternal<bUseMapMask>(mapData, testPos);
				if (testResult != ERejectResult::None)
					return testResult;
			}
		}
		return ERejectResult::None;
	}

	ERejectResult::Type SolveData::testRejectionMaskInternal(MapSolveData &mapData, Vec2i const& testPos, MapMask& visitedMask)
	{
		MapMask mask = 0;
		int count = mapData.countConnectedTilesMask(testPos, visitedMask, mask);
		if (count != 0)
		{
			visitedMask |= mask;

			if (count < globalData->mMinShapeBlockCount)
			{
				return ERejectResult::ConnectTilesCount;
			}

			if (count < mMaxTestShapeSize)
			{
				int pieceMapIndex = count - globalData->mMinShapeBlockCount;
				if (pieceMapIndex >= globalData->mPieceSizeMap.size() ||
					globalData->mPieceSizeMap[pieceMapIndex] == nullptr)
				{
					return ERejectResult::ConnectTilesCount;
				}
				if (globalData->mUsedOption.bTestConnectTileShape)
				{
					ShapeTest test;
					test.pos = testPos;
					test.index = pieceMapIndex;
					test.mapData = &mapData;
					test.mask = mask;
					mCachedPendingTests.push_back(test);
				}
			}
		}

		return ERejectResult::None;
	}

	template< bool bUseMapMask >
	ERejectResult::Type SolveData::testRejectionInternal(MapSolveData &mapData, Vec2i const& testPos)
	{
		int count;
		if constexpr (bUseMapMask)
		{
			count = mapData.countConnectedTilesMask(testPos);
		}
		else
		{
			count = mapData.countConnectedTiles(testPos);
		}

		if (count != 0)
		{
			if (count < globalData->mMinShapeBlockCount)
			{
				return ERejectResult::ConnectTilesCount;
			}

			if (count < mMaxTestShapeSize)
			{
				int pieceMapIndex = count - globalData->mMinShapeBlockCount;
				if (pieceMapIndex >= globalData->mPieceSizeMap.size() ||
					globalData->mPieceSizeMap[pieceMapIndex] == nullptr)
				{
					return ERejectResult::ConnectTilesCount;
				}
				if (globalData->mUsedOption.bTestConnectTileShape)
				{
					ShapeTest test;
					test.pos = testPos;
					test.index = pieceMapIndex;
					test.mapData = &mapData;
					mCachedPendingTests.push_back(test);
				}
			}
		}

		return ERejectResult::None;
	}

	template< bool bUseSubsetCheck, bool bUseMapMask, typename TFunc >
	ERejectResult::Type SolveData::runPendingShapeTest(TFunc& CheckPieceFunc)
	{
		STAT_SCOPE(StatRejectShape);
		for (auto const& test : mCachedPendingTests)
		{
			auto& mapData = *test.mapData;

			mapData.mCachedShapeData.blocks.clear();

			if constexpr (bUseMapMask)
			{
#if 1
				MapMask mask = test.mask;
				int index;
				while (Extract(mask, index))
				{
					Int16Point2D pos;
					pos.x = index % mapData.mMap.mData.getSizeX();
					pos.y = index / mapData.mMap.mData.getSizeX();
					mapData.mCachedShapeData.blocks.push_back(PieceShapeData::Block(pos, MarkMap::Normal));
				}
#else
				mapData.getConnectedTilePosListMask(test.pos);
#endif
			}
			else
			{
				++mTestFrame;
				CHECK(mTestFrame != 0);
				mapData.getConnectedTilePosList(test.pos);
			}


			mapData.mCachedShapeData.standardizeBlocks();

			PieceSolveData* pieceData = nullptr;
			if constexpr (bUseSubsetCheck)
			{
				for (int index = 0; index <= test.index; ++index)
				{
					pieceData = globalData->mPieceSizeMap[index];
					for (; pieceData; pieceData = pieceData->sizeLink)
					{
						if (CheckPieceFunc(*pieceData) && pieceData->shape->findSubset(mapData.mCachedShapeData) != INDEX_NONE)
						{
							goto Found;
						}
					}
				}
			}
			else
			{
				STAT_SCOPE(StatFindShape);

#if USE_SHAPE_REGISTER
				PieceShapeData* shapeData = PieceShape::FindData(mapData.mCachedShapeData);
				if (shapeData == nullptr)
					return ERejectResult::ConnectTileShape;

				pieceData = globalData->mPieceSizeMap[test.index];
				for (; pieceData; pieceData = pieceData->sizeLink)
				{
					if (CheckPieceFunc(*pieceData) && pieceData->shape->findShapeData(shapeData) != INDEX_NONE)
					{
						break;
					}
				}
#else
				pieceData = globalData->mPieceSizeMap[test.index];
				for (; pieceData; pieceData = pieceData->sizeLink)
				{
					if (CheckPieceFunc(*pieceData) && pieceData->shape->findSameShape(mapData.mCachedShapeData) != INDEX_NONE)
					{
						break;
					}
				}
#endif
			}

		Found:
			if (pieceData == nullptr)
			{
				return ERejectResult::ConnectTileShape;
			}
		}

		return ERejectResult::None;
	}

	void Solver::setup(Level& level, SolveOption const& option)
	{
		mUsedOption = option;
		mSolveData.globalData = this;

		TArray<Piece*> sortedPieces;
		for (int index = 0; index < level.mPieces.size(); ++index)
		{
			Piece* piece = level.mPieces[index].get();
			if (mUsedOption.bReserveLockedPiece && piece->isLocked())
			{
				continue;
			}

			sortedPieces.push_back(piece);
		}

		mShapeList.resize(level.mShapes.size());
		for (int index = 0; index < level.mShapes.size(); ++index)
		{
			PieceShape* shape = level.mShapes[index].get();
			auto& shapeSolveData = mShapeList[index];
			shapeSolveData.shape = shape;
			shapeSolveData.pieces.clear();
			shapeSolveData.indexCombination = INDEX_NONE;

			shape->indexSolve = index;
		}

		if (mUsedOption.bEnableSortPiece)
		{
			if (mUsedOption.bEnableIdenticalShapeCombination)
			{
				std::sort(sortedPieces.begin(), sortedPieces.end(), [](Piece* lhs, Piece* rhs)
				{
					if (lhs->shape->getBlockCount() != rhs->shape->getBlockCount())
						return lhs->shape->getBlockCount() > rhs->shape->getBlockCount();

					//if (lhs->shape->indexSolve != rhs->shape->indexSolve)
					return lhs->shape->indexSolve < rhs->shape->indexSolve;
				});
			}
			else
			{
				std::sort(sortedPieces.begin(), sortedPieces.end(), [](Piece* lhs, Piece* rhs)
				{
					return lhs->shape->getBlockCount() > rhs->shape->getBlockCount();
				});
			}
		}
		else if (mUsedOption.bEnableIdenticalShapeCombination)
		{
			std::sort(sortedPieces.begin(), sortedPieces.end(), [](Piece* lhs, Piece* rhs)
			{
				return lhs->shape->indexSolve < rhs->shape->indexSolve;
			});
		}

		mSolveData.setup(level);
		mPieceList.resize(sortedPieces.size());

		std::string str;
		for (int indexPiece = 0; indexPiece < sortedPieces.size(); ++indexPiece)
		{
			str += FStringConv::From(sortedPieces[indexPiece]->index);
			str += " ";
		}
		LogMsg("Sorted Piece : %s", str.c_str());


		if (mUsedOption.bEnableRejection)
		{
			mMinShapeBlockCount = MaxInt32;
			mMaxShapeBlockCount = 0;
			for (Piece* piece : sortedPieces)
			{
				int count = piece->shape->getBlockCount();
				mMinShapeBlockCount = Math::Min(mMinShapeBlockCount, count);
				mMaxShapeBlockCount = Math::Max(mMaxShapeBlockCount, count);
			}
			mUsedOption.bEnableRejection = mMinShapeBlockCount > 1;
		}
		else
		{
			mMinShapeBlockCount = 0;
		}

		auto GetMaxTestShapeSize = [this](bool bUseSubsetCheck)
		{
			if (bUseSubsetCheck)
			{
				return mMaxShapeBlockCount + 1;
			}
			else
			{
				return 2 * mMinShapeBlockCount;
			}
		};

		constexpr bool bSetupUseSubsetCheck = true;
		mSolveData.setMaxTestShapeSize(GetMaxTestShapeSize(bSetupUseSubsetCheck));
		for (int indexPiece = 0; indexPiece < sortedPieces.size(); ++indexPiece)
		{
			Piece* piece = sortedPieces[indexPiece];

			PieceSolveData& pieceData = mPieceList[indexPiece];
			pieceData.index = indexPiece;
			pieceData.piece = piece;
			pieceData.shape = piece->shape;
			if (piece->bCanOperate)
				pieceData.lockedIndexData = INDEX_NONE;
			else
				pieceData.lockedIndexData = pieceData.shape->getDataIndex(piece->dir, piece->mirror);

			pieceData.sizeLink = nullptr;
			ShapeSolveData& shapeSolveData = mShapeList[piece->shape->indexSolve];
			shapeSolveData.pieces.push_back(&pieceData);
			pieceData.shapeSolveData = &shapeSolveData;
		}

		if (mUsedOption.bEnableRejection)
		{
			if (mUsedOption.bTestConnectTilesCount)
			{
				mPieceSizeMap.resize(mMaxShapeBlockCount - mMinShapeBlockCount + 1, nullptr);

				for (int indexPiece = 0; indexPiece < sortedPieces.size(); ++indexPiece)
				{
					PieceSolveData& pieceData = mPieceList[indexPiece];

					int mapIndex = pieceData.shape->getBlockCount() - mMinShapeBlockCount;
					PieceSolveData* linkedDataPrev = mPieceSizeMap[mapIndex];

					pieceData.sizeLink = linkedDataPrev;
					mPieceSizeMap[mapIndex] = &pieceData;
				}
			}
			if (mUsedOption.bTestConnectTileShape)
			{
				mSolveData.mMaxRejectTestIndex = mPieceList.size() - 3;
				if (mSolveData.mMaxRejectTestIndex <= 1)
				{
					mUsedOption.bTestConnectTileShape = false;
				}
			}
		}

		if (mUsedOption.bUseMapMask)
		{
			auto TestCanUseMapMask = [&]()
			{
				for (auto const& map : level.mMaps)
				{
					if (map.getBoundSize().x * map.getBoundSize().y > sizeof(MapMask) * 8)
					{
						return false;
					}

					if (!map.isAllNormalType())
					{
						return false;
					}
				}

				for (auto piece : sortedPieces)
				{
					if (!piece->shape->getDataByIndex(0).isNormal())
						return false;
				}

				return true;
			};

			if (TestCanUseMapMask())
			{
				for (int indexMap = 0; indexMap < mSolveData.mMaps.size(); ++indexMap)
				{
					MapSolveData& mapData = mSolveData.mMaps[indexMap];
					mapData.mMapMask = mapData.calcMapMask();
				}
			}
			else
			{
				mUsedOption.bUseMapMask = false;
			}
		}

		int indexSymmetryFixedShape = INDEX_NONE;
		if (mUsedOption.bCheckMapSymmetry)
		{
			int indexShape = 0;
			for (ShapeSolveData& shapeSolveData : mShapeList)
			{
				PieceShape* shape = shapeSolveData.shape;
				PieceShape::OpState stateFixed;
				bool bFixedState = shapeSolveData.checkFixedState(stateFixed);
				if (bFixedState)
				{
					mUsedOption.bCheckMapSymmetry = false;
					break;
				}

				if (shapeSolveData.pieces.size() == 1)
				{
					if (indexSymmetryFixedShape == INDEX_NONE ||
						shape->getDifferentShapeNum() > mShapeList[indexSymmetryFixedShape].shape->getDifferentShapeNum())
					{
						indexSymmetryFixedShape = indexShape;
					}
				}

				++indexShape;
			}
		}

		//generate possible states
		int indexShape = 0;
		for (ShapeSolveData& shapeSolveData : mShapeList )
		{
			PieceShape* shape = shapeSolveData.shape;

			if (mUsedOption.bEnableRejection && mUsedOption.bTestConnectTileShape)
			{
				shapeSolveData.outerConPosListMap.resize(shape->getDifferentShapeNum());
				for (int index = 0; index < shape->getDifferentShapeNum(); ++index)
				{
					shapeSolveData.outerConPosListMap[index].clear();
					shape->getDataByIndex(index).generateOuterConPosList(shapeSolveData.outerConPosListMap[index]);
				}
			}

			shapeSolveData.states.clear();

			int mapBlockRejectCount = 0;
			int connectTilesCountRejectCount = 0;
			int connectTileShapeRejectCount = 0;

			auto CheckPieceFunc = [&shapeSolveData](PieceSolveData& testPieceData)
			{
				if (shapeSolveData.shape != testPieceData.shape)
					return true;

				return shapeSolveData.pieces.size() > 1;
			};

			PieceShape::OpState stateFixed;
			bool bFixedState = shapeSolveData.checkFixedState(stateFixed);
			int numDifferences = bFixedState ? 1 : shape->getDifferentShapeNum();
			for (int i = 0; i < numDifferences; ++i)
			{
				int indexData;
				PieceShape::OpState opState;
				if (bFixedState)
				{
					opState = stateFixed;
					indexData = shape->getDataIndex(DirType::ValueChecked(stateFixed.dir), stateFixed.mirror);
				}
				else
				{
					opState = shape->getOpState(i);
					indexData = i;
				}

				PieceShapeData const& shapeData = shape->getDataByIndex(indexData);
				for (int indexMap = 0; indexMap < mSolveData.mMaps.size(); ++indexMap)
				{
					MapSolveData& mapData = mSolveData.mMaps[indexMap];

					if (mUsedOption.bCheckMapSymmetry && mapData.bSymmetry)
					{
						if ( indexSymmetryFixedShape == indexShape && indexData != 0)
							continue;
					}

					Vec2i posMax = mapData.mMap.getBoundSize() - shapeData.boundSize;
					Vec2i pos;
					for (pos.y = 0; pos.y <= posMax.y; ++pos.y)
					{
						for (pos.x = 0; pos.x <= posMax.x; ++pos.x)
						{
							do
							{
								if (!mapData.mMap.canLockAssumeInBound(pos, shapeData))
								{
									++mapBlockRejectCount;
									break;
								}

								if (mUsedOption.bEnableRejection)
								{
									mapData.mMap.lockChecked(pos, shapeData);

									ERejectResult::Type result = mSolveData.testRejection<false>(mapData, pos, shapeSolveData.outerConPosListMap[indexData]);

									if (result == ERejectResult::None)
									{
										if (mUsedOption.bTestConnectTileShape)
										{
											result = mSolveData.runPendingShapeTest<true, false>(CheckPieceFunc);
										}
									}

									mapData.mMap.unlock(pos, shapeData);

									if (result != ERejectResult::None)
									{
#if SBLOCK_LOG_SOLVE_INFO
										switch (result)
										{
										case ERejectResult::ConnectTilesCount:
											++connectTilesCountRejectCount;
											break;
										case ERejectResult::ConnectTileShape:
											++connectTileShapeRejectCount;
											break;
										}
#endif
										break;
									}
								}

								PieceSolveState lockState;
								lockState.mapIndex = indexMap;
								lockState.pos = pos;
								lockState.posIndex = mapData.mMap.toIndex(pos);
								lockState.op  = opState;
								lockState.indexData = indexData;
								if (mUsedOption.bUseMapMask)
								{
									lockState.mapMask = mapData.calcMask(shape, lockState);
								}
								shapeSolveData.states.push_back(lockState);

							} while (0);
						}
					}
				}
			}

#if SBLOCK_LOG_SOLVE_INFO
			int totalRejectCount = mapBlockRejectCount + connectTilesCountRejectCount + connectTileShapeRejectCount;
			LogMsg("Shape %d - State = %d, Reject: Total = %d, MapBlock = %d, ConnectTilesCount = %d, ConnectTileShape = %d",
				shape->indexSolve, (int)shapeSolveData.states.size(),
				totalRejectCount, mapBlockRejectCount, connectTilesCountRejectCount, connectTileShapeRejectCount);
#endif
			++indexShape;
		}

		if (bSetupUseSubsetCheck != mUsedOption.bUseSubsetCheck)
		{
			mSolveData.setMaxTestShapeSize(GetMaxTestShapeSize(mUsedOption.bUseSubsetCheck));
		}

		if (mUsedOption.bEnableIdenticalShapeCombination)
		{
			mUsedOption.bEnableIdenticalShapeCombination = false;
			for (auto& shapeSolveData : mShapeList)
			{
				if (shapeSolveData.pieces.size() > 1)
				{
					StateCombination sc;
					sc.init(shapeSolveData.states.size(), shapeSolveData.pieces.size());
					shapeSolveData.indexCombination = mSolveData.mCombinations.size();
					mSolveData.mCombinations.push_back(std::move(sc));
					mUsedOption.bEnableIdenticalShapeCombination = true;
				}
			}
		}

		mUsedSolveFunc = GetSolveFunc(mUsedOption, false);
		mUsedSolvePartFunc = GetSolveFunc(mUsedOption, true);
		mUsedSolveAllFunc = GetSolveAllFunc(mUsedOption);
	}

	int Solver::solveAll()
	{
		Profile_GetTicks(&mTimeStart);
#if USE_STAT
		for (auto stat : ReportedStatList)
		{
			stat->reset();
		}
#endif
#if 0
		if (solve())
		{
			while (solveNext())
			{

			}
		}
#else
		mUsedSolveAllFunc(*this, mSolveData);
#endif

#if USE_STAT
		for (auto stat : ReportedStatList)
		{
			double duration = double(stat->duration) / Profile_GetTickRate();
			LogMsg("%s %u %lf", stat->name, stat->count, duration);
		}
#endif
		return mSolutionCount;
	}

	bool Solver::solve()
	{
		mSolutionCount = 0;
		if (solveImpl(mSolveData, 0) >= 0)
		{
			++mSolutionCount;
			return true;
		}
		return false;
	}

	bool Solver::solveNext()
	{
		if (solveImpl(mSolveData, mPieceList.size() - 1) >= 0)
		{
			++mSolutionCount;
			return true;
		}
		return false;
	}

	int Solver::solveImpl(SolveData& solveData, int indexPiece)
	{
		return (mUsedSolveFunc)(solveData, indexPiece, nullptr);
	}

	int Solver::solvePart(SolveData& solveData, int indexPiece, PartWorkInfo& partWork)
	{
		return (mUsedSolvePartFunc)(solveData, indexPiece, &partWork);
	}

	template< bool bEnableRejection, bool bEnableIdenticalShapeCombination, bool bUseSubsetCheck, bool bUseMapMask, bool bHavePartWork >
	int Solver::SolveImplT(SolveData& solveData, int indexPiece, PartWorkInfo* partWork)
	{
		auto CheckPieceFunc = [&indexPiece](PieceSolveData& testPieceData)
		{
			return testPieceData.index > indexPiece;
		};

		GlobalSolveData& globalData = *solveData.globalData;

		CHECK(
			globalData.mUsedOption.bEnableRejection == bEnableRejection &&
			globalData.mUsedOption.bEnableIdenticalShapeCombination == bEnableIdenticalShapeCombination &&
			globalData.mUsedOption.bUseSubsetCheck == bUseSubsetCheck &&
			(partWork != nullptr) == bHavePartWork
		);

#if SBLOCK_LOG_SOLVE_INFO
		struct IndexCountData
		{
			int noState;
			int rejection;
		};
		TArray<IndexCountData> indexCounts;
		indexCounts.resize(globalData.mPieceList.size(), { 0, 0 });
		int solvingRejectionCount = 0;
#endif

		int numStateUsed = 0;
		int indexPieceMinToSolve = 0;
		if constexpr (bHavePartWork)
		{
			indexPieceMinToSolve = partWork->indexPieceWork;
		}

		while (indexPiece >= indexPieceMinToSolve)
		{
			PieceSolveData& pieceData = globalData.mPieceList[indexPiece];
			ShapeSolveData& shapeSolveData = *pieceData.shapeSolveData;

			auto LogProgress = [&]()
			{
				if (globalData.bLogProgress && indexPiece == 0)
				{
					uint64 timeEnd;
					Profile_GetTicks(&timeEnd);
					double time = double(timeEnd - globalData.mTimeStart) / Profile_GetTickRate();

					float pct = (100.0f * float(solveData.stateIndices[0] + 1)) / globalData.mPieceList[0].shapeSolveData->states.size();
					LogMsg("Solve progress = %3.2f%% , Duration = %.2lf", pct, time);
				}
			};

			int stateUsedCount = 0;

			if constexpr ( bEnableIdenticalShapeCombination )
			{
				if (shapeSolveData.indexCombination == INDEX_NONE)
					goto NoCombineSolve;

				indexPiece = shapeSolveData.pieces.front()->index;

				if (solveData.advanceCombinedState<bUseMapMask>(shapeSolveData, indexPiece, stateUsedCount))
				{
					if constexpr (bHavePartWork)
					{
						if (indexPiece == partWork->indexPieceWork)
						{
							partWork->numStatesUsed += stateUsedCount;
							if (partWork->numStatesUsed > partWork->numStates)
							{
								--indexPiece;
								return indexPiece;
							}
						}
					}

					if constexpr (bEnableRejection)
					{
						if (1 <= indexPiece && indexPiece <= solveData.mMaxRejectTestIndex)
						{
							ERejectResult::Type rejectResult;
							if constexpr (bUseMapMask)
							{
								rejectResult = solveData.testRejection<true>(shapeSolveData, indexPiece);
							}
							else
							{
								rejectResult = solveData.testRejection<false>(shapeSolveData, indexPiece);
							}
							
							if (rejectResult == ERejectResult::None)
							{
								if (globalData.mUsedOption.bTestConnectTileShape)
								{
									rejectResult = solveData.runPendingShapeTest<bUseSubsetCheck, bUseMapMask>(CheckPieceFunc);
								}
							}

							if (rejectResult != ERejectResult::None)
							{
#if SBLOCK_LOG_SOLVE_INFO
								++solvingRejectionCount;
								++indexCounts[indexPiece].rejection;
#endif
								continue;
							}
						}
					}

					indexPiece += shapeSolveData.pieces.size();
					if (indexPiece == globalData.mPieceList.size())
						return indexPiece;
				}
				else
				{
#if SBLOCK_LOG_SOLVE_INFO
					++indexCounts[indexPiece].noState;
#endif
					--indexPiece;

					if constexpr (!bHavePartWork)
					{
						LogProgress();
					}
				}

				continue;
			}

NoCombineSolve:
			{
				if (solveData.advanceState<bUseMapMask>(shapeSolveData, indexPiece, stateUsedCount))
				{
					if constexpr (bHavePartWork)
					{
						if (indexPiece == partWork->indexPieceWork)
						{
							partWork->numStatesUsed += stateUsedCount;
							if (partWork->numStatesUsed > partWork->numStates)
							{
								--indexPiece;
								return indexPiece;
							}
						}
					}

					if constexpr (bEnableRejection)
					{
						if (1 <= indexPiece && indexPiece <= solveData.mMaxRejectTestIndex)
						{
							PieceSolveState const& state = shapeSolveData.states[solveData.stateIndices[indexPiece]];

							auto& mapData = solveData.mMaps[state.mapIndex];

							ERejectResult::Type rejectResult;
							if constexpr (bUseMapMask)
							{
								rejectResult = solveData.testRejectionMask(mapData, state.pos, shapeSolveData.outerConPosListMap[state.indexData]);
							}
							else
							{
								rejectResult = solveData.testRejection<false>(mapData, state.pos, shapeSolveData.outerConPosListMap[state.indexData]);
							}

							if (rejectResult == ERejectResult::None)
							{
								if (globalData.mUsedOption.bTestConnectTileShape)
								{
									rejectResult = solveData.runPendingShapeTest<bUseSubsetCheck, bUseMapMask>(CheckPieceFunc);
								}
							}

							if (rejectResult != ERejectResult::None)
							{
#if SBLOCK_LOG_SOLVE_INFO
								++solvingRejectionCount;
								++indexCounts[indexPiece].rejection;
#endif
								continue;
							}
						}
					}

					++indexPiece;
					if (indexPiece == globalData.mPieceList.size())
						return indexPiece;
				}
				else
				{
#if SBLOCK_LOG_SOLVE_INFO
					++indexCounts[indexPiece].noState;
#endif
					--indexPiece;

					if constexpr (!bHavePartWork)
					{
						LogProgress();
					}
				}
			}
		}
	
#if SBLOCK_LOG_SOLVE_INFO
		LogMsg("SolvingRejectionCount = %d", solvingRejectionCount);
#endif
		return indexPiece;
	}


	template< bool bEnableRejection, bool bEnableIdenticalShapeCombination, bool bUseSubsetCheck, bool bUseMapMask >
	void Solver::SolveAllImplT(Solver& solver, SolveData& solveData)
	{
		//using SolveImpl = Solver::SolveImplT<bEnableRejection, bEnableIdenticalShapeCombination, bUseSubsetCheck, bUseMapMask, bHavePartWork>;

		solver.mSolutionCount = 0;
		if (Solver::SolveImplT<bEnableRejection, bEnableIdenticalShapeCombination, bUseSubsetCheck, bUseMapMask, false>(solveData, 0, nullptr) < 0)
			return;

		++solver.mSolutionCount;
		while (Solver::SolveImplT<bEnableRejection, bEnableIdenticalShapeCombination, bUseSubsetCheck, bUseMapMask, false>(solveData, solver.mPieceList.size() - 1, nullptr) >= 0)
		{
			++solver.mSolutionCount;
		}
	}

	template< bool... Is>
	struct TBoolList {};

	template< typename TFunc, bool ...Is , typename ...Args >
	FORCEINLINE auto BoolBranch(TBoolList<Is...>, bool p0, Args... args)
	{
		if (p0)
		{
			return BoolBranch<TFunc>(TBoolList<Is..., true>(), args...);
		}
		else
		{
			return BoolBranch<TFunc>(TBoolList<Is..., false>(), args...);
		}
	}

	template< typename TFunc, bool ...Is >
	FORCEINLINE auto BoolBranch(TBoolList<Is...>)
	{
		return TFunc::template Exec<Is...>();
	}	

	template< typename TFunc, typename ...Args >
	FORCEINLINE auto BoolBranch(Args... args)
	{
		return BoolBranch<TFunc>(TBoolList<>(), args...);
	}

	struct FSolveImpl
	{
		template< bool ...Is >
		static auto Exec()
		{
			return &Solver::SolveImplT<Is...>;
		}
	};
	Solver::SolveFunc Solver::GetSolveFunc(SolveOption const& option, bool bHavePartWork)
	{
		return BoolBranch< FSolveImpl >(
			option.bEnableRejection,
			option.bEnableIdenticalShapeCombination,
			option.bUseSubsetCheck,
			option.bUseMapMask,
			bHavePartWork
		);
	}

	struct FSolveAllImpl
	{
		template< bool ...Is >
		static auto Exec()
		{
			return &Solver::SolveAllImplT<Is...>;
		}
	};
	Solver::SolveAllFunc Solver::GetSolveAllFunc(SolveOption const& option)
	{
		return BoolBranch< FSolveAllImpl >(
			option.bEnableRejection,
			option.bEnableIdenticalShapeCombination,
			option.bUseSubsetCheck,
			option.bUseMapMask
		);
	}

	class SolveWork : public RunnableThreadT< SolveWork >
		            , public SolveData
	{
	public:
		SolveWork()
		{
		}

		~SolveWork()
		{
			kill();
		}

		unsigned run()
		{
			InlineString<256> str;
			str.format("SolveWorkRun %d", indexWorker);
			TIME_SCOPE(str);

			auto const& workPiece = solver->mPieceList[partWork.indexPieceWork];
			ShapeSolveData const& workShapeSolveData = *workPiece.shapeSolveData;

			if (workShapeSolveData.indexCombination != INDEX_NONE)
			{
				mCombinations[workShapeSolveData.indexCombination].advance(partWork.indexStateStart);
			}
			else
			{
				stateIndices[partWork.indexPieceWork] = -partWork.indexStateStart - 1;
			}

			int indexPieceCur = partWork.indexPieceWork;
			while ( partWork.numStatesUsed <= partWork.numStates )
			{
				indexPieceCur = solver->solvePart(*this, indexPieceCur, partWork);

				if (indexPieceCur >= partWork.indexPieceWork)
				{
					++numSolutions;
					TArray< PieceSolveState > states;
					getSolvedStates(states);
					solver->mSolutionList.push(std::move(states));
					--indexPieceCur;
				}
				else
				{
					indexPieceCur = partWork.indexPieceWork;
					if ( partWork.numStatesUsed >= partWork.numStates )
						break;
				}
			}

			bFinished = true;
			LogMsg("%d worker Finish %d solutions", indexWorker, numSolutions);
			solver->handleWorkFinish(this);
			return 0;
		}
		void exit()
		{
		}

		int numSolutions = 0;
		Solver::PartWorkInfo partWork;
		bool bFinished = false;
		int indexWorker;	
		Solver* solver;

	};

	void Solver::handleWorkFinish(SolveWork* work)
	{
		mSolutionCount += work->numSolutions;

		bool bAllWorkFinished = true;
		for (auto work : mParallelWorks)
		{
			if (!work->bFinished)
			{
				bAllWorkFinished = false;
				break;
			}
		}

		if (bAllWorkFinished)
		{
			uint64 timeEnd;
			Profile_GetTicks(&timeEnd);
			double time = double(timeEnd - mTimeStart) / Profile_GetTickRate();
			LogMsg("SolveParallel finish !! Total Solutions = %u , Duration = %lf", mSolutionCount, time);
		}
	}

	void Solver::solveParallel(int numThreads)
	{
		Profile_GetTicks(&mTimeStart);

		int numWorkState = mSolveData.getPieceStateCount(0);
		int numTheadWork = numWorkState / numThreads;
		int numResWork = numWorkState - numTheadWork * numThreads;

		int indexState = 0;
		for (int i = 0; i < numThreads; ++i)
		{
			SolveWork* work = new SolveWork;
			work->solver = this;
			work->indexWorker = i;
			work->partWork.indexPieceWork = 0;
			work->partWork.numStates = numTheadWork + (i < numResWork);
			work->partWork.numStatesUsed = 0;
			work->partWork.indexStateStart = indexState;
			indexState += work->partWork.numStates;

			work->copyFrom(mSolveData);
			mParallelWorks.push_back(work);

			work->start();
		}
	}

	void Solver::waitWorkCompletion()
	{
		for (auto work : mParallelWorks)
		{
			work->join();
		}
	}


	class SolverDLX : public  ISolver
		            , private DLX::Solver
	{
	public:



	};

	int SolveDLX(GlobalSolveData& globalData, SolveData& solveData, bool bRecursive)
	{
		TArray<uint8> data;
		int numCol, numRow;
		{
			TIME_SCOPE("DLX Matrix Data");

			int mapBlockCount = 0;
			TArray< int > mapDataOffsets;
			mapDataOffsets.resize(solveData.mMaps.size());
			for (int indexMap = 0; indexMap < solveData.mMaps.size(); ++indexMap)
			{
				mapDataOffsets[indexMap] = mapBlockCount;
				mapBlockCount += solveData.mMaps[indexMap].mMap.mData.getRawDataSize();
			}

			TArray< PieceSolveData* > sortedPieceDatas;
			for (int index = 0; index < globalData.mPieceList.size(); ++index)
			{
				sortedPieceDatas.push_back(&globalData.mPieceList[index]);
			}

			std::sort(sortedPieceDatas.begin(), sortedPieceDatas.end(),
				[](PieceSolveData* a, PieceSolveData* b)
				{
					return a->shapeSolveData->states.size() < b->shapeSolveData->states.size();
				}
			);

			numRow = 0;
			for (int index = 0; index < globalData.mPieceList.size(); ++index)
			{
				auto const& pieceData = globalData.mPieceList[index];
				//auto const& pieceData = *sortedPieceDatas[index];

				if (pieceData.lockedIndexData != INDEX_NONE)
				{
					for (int indexState = 0; indexState < pieceData.shapeSolveData->states.size(); ++indexState)
					{
						auto const& state = pieceData.shapeSolveData->states[indexState];
						if (pieceData.lockedIndexData != state.indexData)
							continue;

						numRow += 1;
					}
				}
				else
				{
					numRow += pieceData.shapeSolveData->states.size();
				}
			}
			numCol = globalData.mPieceList.size() + mapBlockCount;
			data.resize(numRow * numCol, 0);
			auto FillRowData = [&](uint8 data[], PieceSolveData const& pieceData, PieceSolveState const& state)
			{
				auto const& shapeData = pieceData.shapeSolveData->getShapeData(state);

				data[pieceData.index] = 1;

				uint8* dataMap = data + mapDataOffsets[state.mapIndex] + state.posIndex + globalData.mPieceList.size();
				for (auto const& block : shapeData.blocks)
				{
					int offset = solveData.mMaps[state.mapIndex].mMap.toIndex(block.pos);
					dataMap[offset] = 1;
				}
			};

			struct RowState
			{
				int indexPiece;
				int indexState;
			};
			TArray< RowState > rowStateMap;
			rowStateMap.resize(numRow);
			auto rowStatePtr = rowStateMap.data();
			uint8* dataPiece = data.data();
			for (int index = 0; index < globalData.mPieceList.size(); ++index)
			{
				auto const& pieceData = globalData.mPieceList[index];
				//auto const& pieceData = *sortedPieceDatas[index];
				if (pieceData.lockedIndexData != INDEX_NONE)
				{
					for (int indexState = 0; indexState < pieceData.shapeSolveData->states.size(); ++indexState)
					{
						auto const& state = pieceData.shapeSolveData->states[indexState];
						if (pieceData.lockedIndexData != state.indexData)
							continue;

						FillRowData(dataPiece, pieceData, state);
						dataPiece += numCol;
						rowStatePtr->indexPiece = index;
						rowStatePtr->indexState = indexState;
						++rowStatePtr;
					}
				}
				else
				{
					for (int indexState = 0; indexState < pieceData.shapeSolveData->states.size(); ++indexState)
					{
						auto const& state = pieceData.shapeSolveData->states[indexState];
						FillRowData(dataPiece, pieceData, state);
						dataPiece += numCol;
						rowStatePtr->indexPiece = index;
						rowStatePtr->indexState = indexState;
						++rowStatePtr;
					}
				}
			}
		}

		DLX::Matrix matrix;
		{
			TIME_SCOPE("DLX Matrix Build");
			matrix.build(numRow, numCol, data.data());
			matrix.removeUnsedColumns();
		}
		DLX::Solver solver(matrix);
		{
			TIME_SCOPE("DLX Solve");
			solver.bRecursive = bRecursive;
			solver.solveAll();
		}
		return solver.mSolutionCount;
	}

	int Solver::solveDLX(bool bRecursive)
	{
		return SolveDLX(*this, mSolveData, bRecursive);
	}

	ISolver* ISolver::Create(EType type)
	{
		switch (type)
		{
		case SBlocks::ISolver::eDFS:
			return new Solver;
		case SBlocks::ISolver::eDLX:
			return new SolverDLX;
		}
		return nullptr;
	}

}//namespace SBlocks