#include "SBlocksSolver.h"

#include "PlatformThread.h"
#include "LogSystem.h"
#include "Core/ScopeExit.h"
#include "StdUtility.h"

#define SBLOCK_LOG_SOLVE_INFO 1

namespace SBlocks
{

	void SolveData::setup(Level& level, SolveOption const& option)
	{
		mMap.copy(level.mMap, true);
		stateIndices.resize(level.mPieces.size(), INDEX_NONE);
		if (option.bTestMinConnectTiles)
		{
			mTestFrameMap.resize(level.mMap.getBoundSize().x, level.mMap.getBoundSize().y);
			mTestFrameMap.fillValue(0);
			mTestFrame = 0;
		}
	}

	int SolveData::countConnectTiles(Vec2i const& pos)
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

	void SolveData::getConnectedTilePosList(Vec2i const& pos)
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

	bool SolveData::advanceState(PieceSolveData& pieceData, int indexPiece)
	{
		int& indexState = stateIndices[indexPiece];

		if (indexState != INDEX_NONE)
		{
			PieceSolveState const& state = pieceData.states[indexState];
			mMap.unlock(state.pos, pieceData.shape->mDataMap[state.dir]);
		}

		++indexState;
		while (indexState != pieceData.states.size())
		{
			PieceSolveState const& state = pieceData.states[indexState];
			if (mMap.tryLockAssumeInBound(state.pos, pieceData.shape->mDataMap[state.dir]))
			{
				return true;
			}

			++indexState;
		}

		indexState = INDEX_NONE;
		return false;
	}

	template< typename TFunc >
	ERejectResult::Type SolveData::testRejection(GlobalSolveData& globalData, Vec2i const pos, std::vector< Int16Point2D > const& outerConPosList, SolveOption const& option, int maxCompareShapeSize, TFunc CheckPieceFunc)
	{

		mCachedPendingTests.clear();
		++mTestFrame;
		for (auto const& outerPos : outerConPosList)
		{
			Vec2i testPos = pos + outerPos;
			int count = countConnectTiles(testPos);
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
						mCachedPendingTests.push_back(test);
					}
				}
			}
		}

		if (option.bTestConnectTileShape)
		{
			for (auto const& test : mCachedPendingTests)
			{
				mCachedShapeData.blocks.clear();
				++mTestFrame;
				getConnectedTilePosList(test.pos);
				AABB bound;
				bound.invalidate();
				for (auto const& block : mCachedShapeData.blocks)
				{
					bound.addPoint(block);
				}
				mCachedShapeData.boundSize = bound.max - bound.min + Vec2i(1, 1);
				for (auto& block : mCachedShapeData.blocks)
				{
					block -= bound.min;
				}
				mCachedShapeData.sortBlocks();

				GlobalSolveData::PieceLink* link = globalData.mPieceSizeMap[test.mapIndex];
				bool bFound = false;
				for (; link; link = link->next)
				{
					Piece* testPiece = link->piece;
					if (CheckPieceFunc(testPiece) && testPiece->shape->findSameShape(mCachedShapeData) != INDEX_NONE)
					{
						bFound = true;
						break;
					}
				}

				if (!bFound)
				{
					return ERejectResult::ConnectTileShape;
				}
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

		std::sort(sortedPieces.begin(), sortedPieces.end(), [](Piece* lhs, Piece* rhs)
		{
			return lhs->shape->getBlockCount() > rhs->shape->getBlockCount();
		});


		if (mUsedOption.bEnableRejection)
		{
			mMinShapeBlockCount = MaxInt32;
			for (Piece* piece : sortedPieces)
			{
				mMinShapeBlockCount = Math::Min(mMinShapeBlockCount, piece->shape->getBlockCount());
			}
			mUsedOption.bEnableRejection = mMinShapeBlockCount > 1;
		}
		else
		{
			mMinShapeBlockCount = 0;
		}

		if (mUsedOption.bEnableRejection && mUsedOption.bTestConnectTileShape)
		{
			mPieceLinkData.resize(level.mPieces.size());
			mPieceSizeMap.resize(level.mPieces.size());

			mMaxShapeBlockCount = 0;
			for (int indexPiece = 0; indexPiece < sortedPieces.size(); ++indexPiece)
			{
				Piece* piece = sortedPieces[indexPiece];
				mPieceLinkData[indexPiece].piece = piece;
				mPieceLinkData[indexPiece].next = nullptr;
				mMaxShapeBlockCount = Math::Max(mMaxShapeBlockCount, piece->shape->getBlockCount());
			}

			mPieceSizeMap.resize(mMaxShapeBlockCount - mMinShapeBlockCount + 1);

			for (int indexPiece = 0; indexPiece < sortedPieces.size(); ++indexPiece)
			{
				Piece* piece = sortedPieces[indexPiece];
				int mapIndex = piece->shape->mDataMap[0].blocks.size() - mMinShapeBlockCount;
				PieceLink* linkedDataPrev = mPieceSizeMap[mapIndex];

				auto& linkData = mPieceLinkData[indexPiece];
				linkData.next = linkedDataPrev;
				mPieceSizeMap[mapIndex] = &linkData;
			}

			mShapeList.resize(level.mShapes.size());
			for (int index = 0; index < level.mShapes.size(); ++index)
			{
				PieceShape* shape = level.mShapes[index].get();
				shape->indexSolve = index;
				auto& shapeData = mShapeList[index];
				for (int dir = 0; dir < 4; ++dir)
				{
					shapeData.outerConPosListMap[dir].clear();
					shape->mDataMap[dir].generateOuterConPosList(shapeData.outerConPosListMap[dir]);
				}
			}
		}

		mSolveData.setup(level, mUsedOption);

		mPieceList.resize(level.mPieces.size());
		int const maxCompareShapeSize = 2 * mMinShapeBlockCount;
		for (int indexPiece = 0; indexPiece < sortedPieces.size(); ++indexPiece)
		{
			int mapBlockRejectCount = 0;
			int minConnectTilesRejectCount = 0;
			int connectTileShapeRejectCount = 0;

			Piece* piece = sortedPieces[indexPiece];
			piece->indexSolve = indexPiece;

			auto CheckPieceFunc = [piece](Piece* testPiece)
			{
				return testPiece != piece;
			};

			PieceSolveData& pieceData = mPieceList[indexPiece];
			pieceData.piece = piece;
			pieceData.shape = piece->shape;
			pieceData.states.clear();

			int dirs[4];
			int numDir = pieceData.shape->getDifferentShapeDirs(dirs);
			for( int i = 0 ; i < numDir; ++i )
			{
				uint8 dir = dirs[i];

				PieceShapeData& shapeData = pieceData.shape->mDataMap[dir];
				Vec2i posMax = mSolveData.mMap.getBoundSize() - shapeData.boundSize;
				Vec2i pos;
				for (pos.y = 0; pos.y <= posMax.y; ++pos.y)
				{
					for (pos.x = 0; pos.x <= posMax.x; ++pos.x)
					{
						do
						{
							if (!mSolveData.mMap.canLockAssumeInBound(pos, shapeData))
							{
								++mapBlockRejectCount;
								break;
							}

							if (mUsedOption.bEnableRejection)
							{
								mSolveData.mMap.lockChecked(pos, shapeData);

								ERejectResult::Type result = mSolveData.testRejection(*this, pos,
									mShapeList[pieceData.shape->indexSolve].outerConPosListMap[dir], 
									mUsedOption, maxCompareShapeSize, CheckPieceFunc);

								mSolveData.mMap.unlock(pos, shapeData);
							
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
							lockState.pos = pos;
							lockState.dir = dir;
							pieceData.states.push_back(lockState);

						} while (0);
					}
				}
			}

#if SBLOCK_LOG_SOLVE_INFO
			int totalRejectCount = mapBlockRejectCount + minConnectTilesRejectCount + connectTileShapeRejectCount;
			LogMsg("%d)Piece %d - State = %d, Reject: Total = %d, MapBlock = %d, MinConnectTiles = %d, ConnectTileShape = %d", 
				indexPiece, piece->index , (int)pieceData.states.size() ,
				totalRejectCount, mapBlockRejectCount, minConnectTilesRejectCount , connectTileShapeRejectCount);
#endif
		}
	}

	int Solver::solveImpl(int index, int startIndex)
	{
		auto CheckPieceFunc = [&index](Piece* testPiece)
		{
			return testPiece->indexSolve > index;
		};

		int const maxCompareShapeSize = 2 * mMinShapeBlockCount;
#if SBLOCK_LOG_SOLVE_INFO
		struct IndexCountData 
		{
			int noState;
			int rejection;
		};
		std::vector<IndexCountData> indexCounts;
		indexCounts.resize(mPieceList.size(), { 0, 0});
		int earlyRejectionCount = 0;
#endif
		while (index >= startIndex)
		{
			PieceSolveData& pieceData = mPieceList[index];
			if (mSolveData.advanceState(pieceData, index))
			{
				if ( mUsedOption.bEnableRejection )
				{
					if (1 <= index && index < 2 * mPieceList.size() / 3)
					{
						PieceSolveState const& state = pieceData.states[mSolveData.stateIndices[index]];

						auto result = mSolveData.testRejection(*this, state.pos,
							mShapeList[pieceData.shape->indexSolve].outerConPosListMap[state.dir],
							mUsedOption, maxCompareShapeSize, CheckPieceFunc);

						if (result != ERejectResult::None)
						{
#if SBLOCK_LOG_SOLVE_INFO
							++earlyRejectionCount;
							++indexCounts[index].rejection;
#endif
							continue;
						}
					}
				}

				++index;
				if (index == mPieceList.size())
					break;
			}
			else
			{
				++indexCounts[index].noState;
				--index;
			}
		}
#if SBLOCK_LOG_SOLVE_INFO
		LogMsg("earlyRejectionCount = %d", earlyRejectionCount);
#endif
		return index;
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