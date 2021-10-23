#include "SBlocksSolver.h"

#include "PlatformThread.h"
#include "LogSystem.h"
#include "Core/ScopeExit.h"
#include "StdUtility.h"
#include "ProfileSystem.h"
#include "InlineString.h"

#define SBLOCK_LOG_SOLVE_INFO 0

namespace SBlocks
{

	void MapSolveData::setup(MarkMap const& map, SolveOption const& option)
	{
		mMap.copyFrom(map, true);
		if (option.bTestConnectTilesCount)
		{
			mTestFrameMap.resize(map.getBoundSize().x, map.getBoundSize().y);
			mTestFrameMap.fillValue({ 0,0 });
			mSubTestFrame = 0;
		}
	}

	void MapSolveData::copyFrom(MapSolveData const& rhs)
	{
		mMap.copyFrom(rhs.mMap);
		mTestFrameMap.resize(rhs.mTestFrameMap.getSizeX(), rhs.mTestFrameMap.getSizeY());
		mTestFrameMap.fillValue({ 0,0 });
		mSubTestFrame = 0;
	}

	int MapSolveData::countConnectTiles(Vec2i const& pos)
	{
		if (!mMap.isInBound(pos) || mMap.getValue(pos) != 0)
			return 0;

		auto& frame = mTestFrameMap(pos.x, pos.y);
		if (frame.master == *mTestFramePtr)
			return 0;

		frame.master = *mTestFramePtr;
		int result = 1;
		for (int i = 0; i < 4; ++i)
		{
			Vec2i testPos = pos + GConsOffsets[i];
			result += countConnectTilesRec(testPos);
			if (result >= mMaxCount)
				break;
		}
		return result;
	}

	int MapSolveData::countConnectTilesRec(Vec2i const& pos)
	{
		if (!mMap.isInBound(pos) || mMap.getValue(pos) != 0)
			return 0;

		auto& frame = mTestFrameMap(pos.x, pos.y);
		if (frame.sub == mSubTestFrame)
			return 0;
		frame.master = *mTestFramePtr;
		frame.sub = mSubTestFrame;

		int result = 1;
		for (int i = 0; i < 4; ++i)
		{
			Vec2i testPos = pos + GConsOffsets[i];
			result += countConnectTilesRec(testPos);
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
		mCachedShapeData.blocks.push_back(PieceShapeData::Block( pos , MarkMap::GetType(data)));
		for (int i = 0; i < 4; ++i)
		{
			Vec2i testPos = pos + GConsOffsets[i];
			getConnectedTilePosList(testPos);
		}
	}

	void SolveData::getSolvedStates(std::vector< PieceSolveState >& outStates) const
	{
		int index = 0;
		for (auto const& pieceData : globalData->mPieceList)
		{
			int indexState = stateIndices[index];
			auto const* shapeSolveData = pieceData.shapeSaveData;
			CHECK(indexState != INDEX_NONE);
			outStates.push_back(shapeSolveData->states[indexState]);
			++index;
		}
	}

	void SolveData::setup(Level& level, SolveOption const& option)
	{
		mMaps.resize(level.mMaps.size());
		for (int i = 0; i < mMaps.size(); ++i)
		{
			mMaps[i].setup(level.mMaps[i], option);
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
	}

	bool SolveData::advanceState(ShapeSolveData& shapeSolveData, int indexPiece, int& outUsedStateCount )
	{
		int& indexState = stateIndices[indexPiece];

		if (indexState >= 0)
		{
			PieceSolveState const& state = shapeSolveData.states[indexState];
			mMaps[state.mapIndex].mMap.unlock(state.pos, shapeSolveData.shape->mDataMap[state.dir]);

			++indexState;
		}
		else
		{
			indexState = -indexState - 1;
		}
	
		while (indexState != shapeSolveData.states.size())
		{
			++outUsedStateCount;
			PieceSolveState const& state = shapeSolveData.states[indexState];
			if (mMaps[state.mapIndex].mMap.tryLockAssumeInBound(state.pos, shapeSolveData.shape->mDataMap[state.dir]))
			{
				return true;
			}

			++indexState;
		}

		indexState = INDEX_NONE;
		return false;
	}

	bool SolveData::advanceCombinedState(ShapeSolveData& shapeSolveData, int indexPiece, int& outUsedStateCount)
	{
		CHECK(indexPiece == shapeSolveData.pieces.front()->piece->indexSolve);

		StateCombination& sc = mCombinations[shapeSolveData.indexCombination];

		auto UnlockUsedStates = [&]()
		{
			for (int i = 0; i < shapeSolveData.pieces.size(); ++i)
			{
				int& indexState = stateIndices[indexPiece + i];
				if (indexState != INDEX_NONE)
				{
					PieceSolveState const& state = shapeSolveData.states[indexState];
					mMaps[state.mapIndex].mMap.unlock(state.pos, shapeSolveData.shape->mDataMap[state.dir]);
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
				if (mMaps[state.mapIndex].mMap.tryLockAssumeInBound(state.pos, shapeSolveData.shape->mDataMap[state.dir]))
				{
					stateIndices[indexPiece + i] = indexState;
				}
				else
				{
					UnlockUsedStates();
					break;
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
		auto const& shapeSolveData = *pieceData.shapeSaveData;
		if (globalData->mUsedOption.bEnableIdenticalShapeCombination && shapeSolveData.indexCombination != INDEX_NONE)
		{
			return mCombinations[shapeSolveData.indexCombination].getStateNum();
		}
		else
		{
			return shapeSolveData.states.size();
		}
	}

	template< typename TFunc >
	ERejectResult::Type SolveData::testRejection(MapSolveData& mapData, Vec2i const pos, std::vector< Int16Point2D > const& outerConPosList, SolveOption const& option, int maxCompareShapeSize, TFunc& CheckPieceFunc)
	{
		mCachedPendingTests.clear();
		++mTestFrame;
		CHECK(mTestFrame != 0);
		mapData.mMaxCount = maxCompareShapeSize;
		for (auto const& outerPos : outerConPosList)
		{
			Vec2i testPos = pos + outerPos;
			++mapData.mSubTestFrame;
			CHECK(mapData.mSubTestFrame != 0);
			auto testResult = testRejectionInternal(mapData, testPos, option, maxCompareShapeSize);
			if (testResult != ERejectResult::None)
				return testResult;
		}

		if (option.bTestConnectTileShape)
		{
			return runPendingShapeTest(CheckPieceFunc);
		}

		return ERejectResult::None;
	}

	template< typename TFunc >
	ERejectResult::Type SolveData::testRejection(ShapeSolveData& shapeSolveData, int indexPiece, SolveOption const& option, int maxCompareShapeSize, TFunc& CheckPieceFunc)
	{
		mCachedPendingTests.clear();
		++mTestFrame;
		CHECK(mTestFrame != 0);
		for (int i = 0; i < shapeSolveData.pieces.size(); ++i)
		{
			PieceSolveState const& state = shapeSolveData.states[stateIndices[indexPiece + i]];
			auto& mapData = mMaps[state.mapIndex];
			mapData.mMaxCount = maxCompareShapeSize;
			for (auto const& outerPos : shapeSolveData.outerConPosListMap[state.dir])
			{
				Vec2i testPos = state.pos + outerPos;
				++mapData.mSubTestFrame;
				CHECK(mapData.mSubTestFrame != 0);
				auto testResult = testRejectionInternal(mapData, testPos, option, maxCompareShapeSize);
				if (testResult != ERejectResult::None)
					return testResult;
			}
		}

		if (option.bTestConnectTileShape)
		{
			return runPendingShapeTest(CheckPieceFunc);
		}

		return ERejectResult::None;
	}

	ERejectResult::Type SolveData::testRejectionInternal(MapSolveData &mapData, Vec2i const& testPos, SolveOption const &option, int maxCompareShapeSize)
	{
		int count = mapData.countConnectTiles(testPos);
		if (count != 0)
		{
			if (count < globalData->mMinShapeBlockCount)
			{
				return ERejectResult::ConnectTilesCount;
			}
	
			int pieceMapIndex = count - globalData->mMinShapeBlockCount;
			if (count < maxCompareShapeSize && IsValidIndex(globalData->mPieceSizeMap, pieceMapIndex))
			{
				if (globalData->mPieceSizeMap[pieceMapIndex] == nullptr)
				{
					return ERejectResult::ConnectTilesCount;
				}
				if (option.bTestConnectTileShape)
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

	template< typename TFunc >
	ERejectResult::Type SolveData::runPendingShapeTest(TFunc& CheckPieceFunc)
	{
		for (auto const& test : mCachedPendingTests)
		{
			auto& mapData = *test.mapData;

			mapData.mCachedShapeData.blocks.clear();
			++mTestFrame;
			CHECK(mTestFrame != 0);
			mapData.getConnectedTilePosList(test.pos);

			mapData.mCachedShapeData.standardizeBlocks();

			PieceSolveData* pieceData = globalData->mPieceSizeMap[test.index];
			for (; pieceData; pieceData = pieceData->link)
			{
				if (CheckPieceFunc(*pieceData) && pieceData->shape->findSameShape(mapData.mCachedShapeData) != INDEX_NONE)
				{
					break;
				}
			}

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

		std::vector<Piece*> sortedPieces;
		for (int index = 0; index < level.mPieces.size(); ++index)
		{
			Piece* piece = level.mPieces[index].get();
			sortedPieces.push_back(piece);
		}

		mShapeList.resize(level.mShapes.size());
		for (int index = 0; index < level.mShapes.size(); ++index)
		{
			PieceShape* shape = level.mShapes[index].get();
			auto& shapeSolveData = mShapeList[index];
			shapeSolveData.shape = shape;
			shapeSolveData.pieces.clear();
			shape->indexSolve = index;
		}

		if (mUsedOption.bEnableSortPiece )
		{
			if (mUsedOption.bEnableIdenticalShapeCombination)
			{
				std::sort(sortedPieces.begin(), sortedPieces.end(), [](Piece* lhs, Piece* rhs)
				{
					if (lhs->shape->getBlockCount() < rhs->shape->getBlockCount())
						return false;
					if (lhs->shape->getBlockCount() == rhs->shape->getBlockCount())
					{
						return lhs->shape->indexSolve < rhs->shape->indexSolve;
					}
					return true;
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

		mSolveData.setup(level, mUsedOption);

		mPieceList.resize(level.mPieces.size());
		int const maxCompareShapeSize = 2 * mMinShapeBlockCount;
		for (int indexPiece = 0; indexPiece < sortedPieces.size(); ++indexPiece)
		{
			Piece* piece = sortedPieces[indexPiece];
			piece->indexSolve = indexPiece;

			PieceSolveData& pieceData = mPieceList[indexPiece];
			pieceData.piece = piece;
			pieceData.shape = piece->shape;
			pieceData.link = nullptr;

			ShapeSolveData& shapeSolveData = mShapeList[piece->shape->indexSolve];
			shapeSolveData.pieces.push_back(&pieceData);
			pieceData.shapeSaveData = &shapeSolveData;
		}

		if (mUsedOption.bEnableRejection && mUsedOption.bTestConnectTilesCount)
		{
			mPieceSizeMap.resize(mMaxShapeBlockCount - mMinShapeBlockCount + 1);

			for (int indexPiece = 0; indexPiece < sortedPieces.size(); ++indexPiece)
			{
				PieceSolveData& pieceData = mPieceList[indexPiece];

				int mapIndex = pieceData.shape->getBlockCount()- mMinShapeBlockCount;
				PieceSolveData* linkedDataPrev = mPieceSizeMap[mapIndex];

				pieceData.link = linkedDataPrev;
				mPieceSizeMap[mapIndex] = &pieceData;
			}

		}

		//generate posible states
		for (auto& shapeSolveData : mShapeList )
		{
			PieceShape* shape = shapeSolveData.shape;

			if (mUsedOption.bEnableRejection && mUsedOption.bTestConnectTileShape)
			{
				for (int dir = 0; dir < 4; ++dir)
				{
					shapeSolveData.outerConPosListMap[dir].clear();
					shape->mDataMap[dir].generateOuterConPosList(shapeSolveData.outerConPosListMap[dir]);
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

			int dirs[4];
			int numDir = shape->getDifferentShapeDirs(dirs);
			for (int i = 0; i < numDir; ++i)
			{
				uint8 dir = dirs[i];
				PieceShapeData& shapeData = shape->mDataMap[dir];

				for (int indexMap = 0; indexMap < mSolveData.mMaps.size(); ++indexMap)
				{
					MapSolveData& mapData = mSolveData.mMaps[indexMap];


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

									ERejectResult::Type result = mSolveData.testRejection(
										mapData, pos, shapeSolveData.outerConPosListMap[dir],
										mUsedOption, maxCompareShapeSize, CheckPieceFunc);

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
								lockState.dir = dir;
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
		}

		if (mUsedOption.bEnableIdenticalShapeCombination)
		{
			mUsedOption.bEnableIdenticalShapeCombination = false;
			for (auto& shapeSovleData : mShapeList)
			{
				if (shapeSovleData.pieces.size() > 1)
				{
					StateCombination sc;
					sc.init(shapeSovleData.states.size(), shapeSovleData.pieces.size());
					shapeSovleData.indexCombination = mSolveData.mCombinations.size();
					mSolveData.mCombinations.push_back(std::move(sc));
					mUsedOption.bEnableIdenticalShapeCombination = true;
				}
				else
				{
					shapeSovleData.indexCombination = INDEX_NONE;
				}
			}
		}

		mUsedSolveFunc = GetSolveFunc< false >(mUsedOption);
		mUsedSolvePartFunc = GetSolveFunc< true >(mUsedOption);
	}

	int Solver::solveImpl(SolveData& solveData, int indexPiece)
	{
		return (this->*mUsedSolveFunc)(solveData, indexPiece, nullptr);
	}

	int Solver::solvePart(SolveData& solveData, int indexPiece, PartWorkInfo& partWork)
	{
		return (this->*mUsedSolvePartFunc)(solveData, indexPiece, &partWork);
	}

	template< bool bEnableRejection, bool bEnableIdenticalShapeCombination, bool bHavePartWork >
	int Solver::solveImplT(SolveData& solveData, int indexPiece, PartWorkInfo* partWork)
	{
		auto CheckPieceFunc = [&indexPiece](PieceSolveData& testPieceData)
		{
			return testPieceData.piece->indexSolve > indexPiece;
		};

		int const maxCompareShapeSize = 2 * mMinShapeBlockCount;
#if SBLOCK_LOG_SOLVE_INFO
		struct IndexCountData
		{
			int noState;
			int rejection;
		};
		std::vector<IndexCountData> indexCounts;
		indexCounts.resize(mPieceList.size(), { 0, 0 });
		int solvingRejectionCount = 0;
#endif

		int numStateUsed = 0;
		int indexPieceMinToSolve = 0;
		if (partWork)
		{
			indexPieceMinToSolve = partWork->indexPieceWork;
		}

		while (indexPiece >= indexPieceMinToSolve)
		{
			PieceSolveData& pieceData = mPieceList[indexPiece];
			ShapeSolveData& shapeSolveData = *pieceData.shapeSaveData;

			int stateUsedCount = 0;
			if (bEnableIdenticalShapeCombination && shapeSolveData.indexCombination != INDEX_NONE)
			{
				indexPiece = shapeSolveData.pieces.front()->piece->indexSolve;

				if (solveData.advanceCombinedState(shapeSolveData, indexPiece, stateUsedCount))
				{
					if (bHavePartWork)
					{
						if (indexPiece == partWork->indexPieceWork)
						{
							partWork->numStatesUsed += stateUsedCount;
							if (partWork->numStatesUsed > partWork->numStates)
							{
								--indexPiece;
								break;
							}
						}
					}

					if (bEnableRejection)
					{
						if (1 <= indexPiece && indexPiece < 2 * mPieceList.size() / 3)
						{
							auto result = solveData.testRejection(
								shapeSolveData, indexPiece,
								mUsedOption, maxCompareShapeSize, CheckPieceFunc);

							if (result != ERejectResult::None)
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
					if (indexPiece == mPieceList.size())
						break;
				}
				else
				{
#if SBLOCK_LOG_SOLVE_INFO
					++indexCounts[indexPiece].noState;
#endif
					--indexPiece;
				}
			}
			else
			{
				if (solveData.advanceState(shapeSolveData, indexPiece, stateUsedCount))
				{
					if (bHavePartWork)
					{
						if (indexPiece == partWork->indexPieceWork)
						{
							partWork->numStatesUsed += stateUsedCount;
							if (partWork->numStatesUsed > partWork->numStates)
							{
								--indexPiece;
								break;
							}
						}
					}

					if (bEnableRejection)
					{
						if (1 <= indexPiece && indexPiece < 2 * mPieceList.size() / 3)
						{
							PieceSolveState const& state = shapeSolveData.states[solveData.stateIndices[indexPiece]];

							auto& mapData = solveData.mMaps[state.mapIndex];

							auto result = solveData.testRejection(
								mapData, state.pos, shapeSolveData.outerConPosListMap[state.dir],
								mUsedOption, maxCompareShapeSize, CheckPieceFunc);

							if (result != ERejectResult::None)
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
					if (indexPiece == mPieceList.size())
						break;
				}
				else
				{
#if SBLOCK_LOG_SOLVE_INFO
					++indexCounts[indexPiece].noState;
#endif
					--indexPiece;
				}
			}
		}
	
#if SBLOCK_LOG_SOLVE_INFO
		LogMsg("SolvingRejectionCount = %d", solvingRejectionCount);
#endif
		return indexPiece;
	}

	template< bool bHavePartWork >
	Solver::SolveFunc Solver::GetSolveFunc(SolveOption const& option)
	{
		if (option.bEnableRejection)
		{
			if (option.bEnableIdenticalShapeCombination)
				return &Solver::solveImplT< true, true , bHavePartWork>;
			else
				return &Solver::solveImplT< true, false, bHavePartWork>;
		}

		if (option.bEnableIdenticalShapeCombination)
			return &Solver::solveImplT< false, true, bHavePartWork>;

		return &Solver::solveImplT< false, false, bHavePartWork>;
	}

	class SolveWork : public RunnableThreadT< SolveWork >
		            , public SolveData
	{
	public:
		SolveWork()
		{
		}

		unsigned run()
		{
			InlineString<256> str;
			str.format("SolveWorkRun %d", indexWorker);
			TIME_SCOPE(str);

			auto const& workPiece = solver->mPieceList[partWork.indexPieceWork];
			ShapeSolveData const& workShapeSolveData = *workPiece.shapeSaveData;

			int numSolutions = 0;

			if (solver->mUsedOption.bEnableIdenticalShapeCombination && workShapeSolveData.indexCombination != INDEX_NONE)
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
					std::vector< PieceSolveState > states;
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

			LogMsg("%d worker Finish %d solutions", indexWorker, numSolutions);
			return 0;
		}
		void exit()
		{
		}

		Solver::PartWorkInfo partWork;

		int indexWorker;	
		Solver* solver;
	};

	void Solver::solveParallel(int numThreads)
	{
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

	void Solver::waitWorkCompilition()
	{
		for (auto work : mParallelWorks)
		{
			work->join();
		}
	}

}//namespace SBlocks