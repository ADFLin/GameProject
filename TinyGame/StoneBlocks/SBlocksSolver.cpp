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

	void MapSolveData::setup(Level& level, SolveOption const& option)
	{
		mMap.copyFrom(level.mMap, true);
		if (option.bTestMinConnectTiles)
		{
			mTestFrameMap.resize(level.mMap.getBoundSize().x, level.mMap.getBoundSize().y);
			mTestFrameMap.fillValue(0);
			mTestFrame = 0;
		}
	}

	void MapSolveData::copyFrom(MapSolveData const& rhs)
	{
		mMap.copyFrom(rhs.mMap);
		mTestFrameMap = rhs.mTestFrameMap;
		mTestFrame = rhs.mTestFrame;
	}

	int MapSolveData::countConnectTiles(Vec2i const& pos)
	{
		if (!mMap.isInBound(pos) || mMap.getValue(pos) != 0)
			return 0;

		auto& frame = mTestFrameMap(pos.x, pos.y);
		if (frame == mTestFrame)
			return 0;

		frame = mTestFrame;

		int result = 1;
		for (int i = 0; i < 4; ++i)
		{
			Vec2i testPos = pos + GConsOffsets[i];
			result += countConnectTiles(testPos);
		}
		return result;
	}

	void MapSolveData::getConnectedTilePosList(Vec2i const& pos)
	{
		if (!mMap.isInBound(pos) || mMap.getValue(pos) != 0)
			return;

		auto& frame = mTestFrameMap(pos.x, pos.y);
		if (frame == mTestFrame)
			return;

		frame = mTestFrame;
		mCachedShapeData.blocks.push_back(pos);
		for (int i = 0; i < 4; ++i)
		{
			Vec2i testPos = pos + GConsOffsets[i];
			getConnectedTilePosList(testPos);
		}
	}

	void SolveData::setup(Level& level, SolveOption const& option)
	{
		mMaps.resize(level.mMaps.size());
		for (int i = 0; i < mMaps.size(); ++i)
		{
			mMaps[i].setup(level, option);
		}
		stateIndices.resize(level.mPieces.size(), INDEX_NONE);
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

	template< typename TFunc >
	ERejectResult::Type SolveData::testRejection(GlobalSolveData& globalData, MapSolveData& mapData, Vec2i const pos, std::vector< Int16Point2D > const& outerConPosList, SolveOption const& option, int maxCompareShapeSize, TFunc CheckPieceFunc)
	{

		mCachedPendingTests.clear();
		++mapData.mTestFrame;
		for (auto const& outerPos : outerConPosList)
		{
			Vec2i testPos = pos + outerPos;
			int count = mapData.countConnectTiles(testPos);
			if (count != 0)
			{
				if (count < globalData.mMinShapeBlockCount)
				{
					return ERejectResult::MinConnectTiles;
				}
				if (option.bTestConnectTileShape)
				{
					int pieceMapIndex = count - globalData.mMinShapeBlockCount;
					if (count < maxCompareShapeSize && IsValidIndex(globalData.mPieceSizeMap, pieceMapIndex))
					{
						if (globalData.mPieceSizeMap[pieceMapIndex] == nullptr)
						{
							return ERejectResult::ConnectTileShape;
						}
						ShapeTest test;
						test.pos = testPos;
						test.mapIndex = pieceMapIndex;
						test.mapData = &mapData;
						mCachedPendingTests.push_back(test);
					}
				}
			}
		}

		if (option.bTestConnectTileShape)
		{
			return runPendingTest(globalData, CheckPieceFunc);
		}

		return ERejectResult::None;
	}

	template< typename TFunc >
	ERejectResult::Type SolveData::testRejection(GlobalSolveData& globalData, ShapeSolveData& shapeSolveData, int indexPiece, SolveOption const& option, int maxCompareShapeSize, TFunc CheckPieceFunc)
	{
		mCachedPendingTests.clear();

		for (int i = 0; i < shapeSolveData.pieces.size(); ++i)
		{
			PieceSolveState const& state = shapeSolveData.states[stateIndices[indexPiece + i]];
			auto& mapData = mMaps[state.mapIndex];

			++mapData.mTestFrame;
			for (auto const& outerPos : shapeSolveData.outerConPosListMap[state.dir])
			{
				Vec2i testPos = state.pos + outerPos;
				int count = mapData.countConnectTiles(testPos);
				if (count != 0)
				{
					if (count < globalData.mMinShapeBlockCount)
					{
						return ERejectResult::MinConnectTiles;
					}
					if (option.bTestConnectTileShape)
					{
						int pieceMapIndex = count - globalData.mMinShapeBlockCount;
						if (count < maxCompareShapeSize && IsValidIndex(globalData.mPieceSizeMap, pieceMapIndex))
						{
							if (globalData.mPieceSizeMap[pieceMapIndex] == nullptr)
							{
								return ERejectResult::ConnectTileShape;
							}
							ShapeTest test;
							test.pos = testPos;
							test.mapIndex = pieceMapIndex;
							test.mapData = &mapData;
							mCachedPendingTests.push_back(test);
						}
					}
				}
			}
		}

		if (option.bTestConnectTileShape)
		{
			return runPendingTest(globalData, CheckPieceFunc);
		}

		return ERejectResult::None;
	}

	template< typename TFunc >
	ERejectResult::Type SolveData::runPendingTest(GlobalSolveData& globalData, TFunc CheckPieceFunc)
	{
		for (auto const& test : mCachedPendingTests)
		{
			auto& mapData = *test.mapData;

			mapData.mCachedShapeData.blocks.clear();
			++mapData.mTestFrame;
			mapData.getConnectedTilePosList(test.pos);

			mapData.mCachedShapeData.standardizeBlocks();

			PieceSolveData* pieceData = globalData.mPieceSizeMap[test.mapIndex];
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

		if (mUsedOption.bEnableRejection && mUsedOption.bTestConnectTileShape)
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
			int minConnectTilesRejectCount = 0;
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
										*this, mapData, pos,
										shapeSolveData.outerConPosListMap[dir],
										mUsedOption, maxCompareShapeSize, CheckPieceFunc);

									mapData.mMap.unlock(pos, shapeData);

									if (result != ERejectResult::None)
									{
#if SBLOCK_LOG_SOLVE_INFO
										switch (result)
										{
										case ERejectResult::MinConnectTiles:
											++minConnectTilesRejectCount;
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
			int totalRejectCount = mapBlockRejectCount + minConnectTilesRejectCount + connectTileShapeRejectCount;
			LogMsg("Shape %d - State = %d, Reject: Total = %d, MapBlock = %d, MinConnectTiles = %d, ConnectTileShape = %d",
				shape->indexSolve, (int)shapeSolveData.states.size(),
				totalRejectCount, mapBlockRejectCount, minConnectTilesRejectCount, connectTileShapeRejectCount);
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
	}

	int Solver::solveImpl(SolveData& solveData, int indexPiece, PartWorkInfo* partWork)
	{
		if (mUsedOption.bEnableRejection)
		{
			if (mUsedOption.bEnableIdenticalShapeCombination)
				return solveImplT< true, true >(solveData, indexPiece, partWork);
			else
				return solveImplT< true, false >(solveData, indexPiece, partWork);
		}

		if (mUsedOption.bEnableIdenticalShapeCombination)
			return solveImplT< false, true >(solveData, indexPiece, partWork);
			
		return solveImplT< false, false >(solveData, indexPiece, partWork);
	}


	template< bool bEnableRejection, bool bEnableIdenticalShapeCombination >
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
					if (partWork)
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
							auto result = solveData.testRejection(*this,
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
					if (partWork)
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

							auto& mapData = mSolveData.mMaps[state.mapIndex];

							auto result = solveData.testRejection(
								*this, mapData, state.pos,
								shapeSolveData.outerConPosListMap[state.dir],
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
				indexPieceCur = solver->solveImpl(*this, indexPieceCur, &partWork);

				if (indexPieceCur >= partWork.indexPieceWork)
				{
					++numSolutions;
					std::vector< PieceSolveState > states;
					getSolvedStates(*solver, states);
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
		int numWorkState = getPieceStateCount(0);
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