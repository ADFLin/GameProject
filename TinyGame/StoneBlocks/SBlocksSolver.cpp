#include "SBlocksSolver.h"

#include "PlatformThread.h"
#include "LogSystem.h"
#include "Core/ScopeExit.h"
#include "StdUtility.h"


namespace SBlocks
{

	const Vec2i GConsOffsets[8] = {
		Vec2i(1,0) , Vec2i(0,1) , Vec2i(-1,0) , Vec2i(0,-1),
		Vec2i(1,1) , Vec2i(-1,1) , Vec2i(-1,-1) , Vec2i(1,-1) 
	};

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
		if (!mMap.isInBound(pos))
			return 0;
		if (mMap.getValue(pos))
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

	void SolveData::getConnectedTilePosList(Vec2i const& pos, std::vector< Int16Point2D >& outPosList)
	{
		if (!mMap.isInBound(pos))
			return;
		if (mMap.getValue(pos))
			return;

		auto& frame = mTestFrameMap(pos.x, pos.y);
		if (frame == mTestFrame)
			return;

		frame = mTestFrame;
		outPosList.push_back(pos);
		for (int i = 0; i < 4; ++i)
		{
			Vec2i testPos = pos + GConsOffsets[i];
			getConnectedTilePosList(testPos, outPosList);
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
	ERejectResult::Type SolveData::testRejection(GlobalSolveData& globalData, Vec2i const pos, PieceShapeData& shapeData, SolveOption const& option , TFunc func)
	{

		mCachedPendingTests.clear();
		++mTestFrame;
		for (auto const& block : shapeData.blocks)
		{
			for (int i = 0; i < 4; ++i)
			{
				Vec2i testPos = pos + block + GConsOffsets[i];
				int count = countConnectTiles(testPos);
				if (count != 0)
				{
					if (count < globalData.minShapeBlockCount)
					{
						return ERejectResult::MinConnectTiles;
					}
					if (option.bTestConnectTileShape)
					{
						int pieceMapIndex = count - globalData.minShapeBlockCount;
						if (count < 2 * globalData.minShapeBlockCount && IsValidIndex(globalData.mPieceSizeMap, pieceMapIndex))
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
		}

		if (option.bTestConnectTileShape)
		{
			for (auto const& test : mCachedPendingTests)
			{
				mCachedShapeData.blocks.clear();
				++mTestFrame;
				getConnectedTilePosList(test.pos, mCachedShapeData.blocks);
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
					if (func(testPiece) && testPiece->shape->findSameShape(mCachedShapeData) != INDEX_NONE)
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

	void Solver::setup(Level& level)
	{
		
		SolveOption option;
		option.bTestMinConnectTiles = true;
		option.bTestConnectTileShape = true;

		minShapeBlockCount = MaxInt32;

		if (option.bTestMinConnectTiles)
		{
			for (int index = 0; index < level.mPieces.size(); ++index)
			{
				Piece* piece = level.mPieces[index].get();
				minShapeBlockCount = Math::Min(minShapeBlockCount, piece->shape->getBlockCount());
			}

			option.bTestMinConnectTiles = minShapeBlockCount > 1;

			if (option.bTestConnectTileShape)
			{
				mPieceLinkData.resize(level.mPieces.size());
				mPieceSizeMap.resize(level.mPieces.size());

				maxShapeBlockCount = 0;
				for (int index = 0; index < level.mPieces.size(); ++index)
				{
					Piece* piece = level.mPieces[index].get();
					mPieceLinkData[index].piece = piece;
					mPieceLinkData[index].next = nullptr;
					maxShapeBlockCount = Math::Max(maxShapeBlockCount, piece->shape->getBlockCount());
				}

				mPieceSizeMap.resize(maxShapeBlockCount - minShapeBlockCount + 1);

				for (int index = 0; index < level.mPieces.size(); ++index)
				{
					Piece* piece = level.mPieces[index].get();
					int mapIndex = piece->shape->mDataMap[0].blocks.size() - minShapeBlockCount;
					PieceLink* linkedDataPrev = mPieceSizeMap[mapIndex];

					auto& linkData = mPieceLinkData[index];
					linkData.next = linkedDataPrev;
					mPieceSizeMap[mapIndex] = &linkData;
				}
			}
		}

		mSolveData.setup(level, option);

		mPieceList.resize(level.mPieces.size());
		for (int index = 0; index < level.mPieces.size(); ++index)
		{
			int mapBlockRejectCount = 0;
			int minConnectTilesRejectCount = 0;
			int connectTileShapeRejectCount = 0;

			Piece* piece = level.mPieces[index].get();

			auto PieceFunc = [piece](Piece* testPiece)
			{
				return testPiece != piece;
			};

			PieceSolveData& pieceData = mPieceList[index];
			pieceData.piece = piece;
			pieceData.shape = piece->shape;
			pieceData.states.clear();

#if 1
			int dirs[4];
			int numDir = pieceData.shape->getDifferentShapeDirs(dirs);
			for( int i = 0 ; i < numDir; ++i )
			{
				uint8 dir = dirs[i];
#else
			for (int dir = 0; dir < DirType::RestNumber; ++dir)
			{
#endif
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

							if (option.bTestMinConnectTiles)
							{
								mSolveData.mMap.lockChecked(pos, shapeData);
								ERejectResult::Type result = mSolveData.testRejection(*this, pos, shapeData, option, PieceFunc);
								mSolveData.mMap.unlock(pos, shapeData);
							
								if (result != ERejectResult::None)
								{
									switch (result)
									{
									case ERejectResult::MinConnectTiles:
										++minConnectTilesRejectCount;
										break;
									case ERejectResult::ConnectTileShape:
										++connectTileShapeRejectCount;
										break;
									}
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


			int totalRejectCount = mapBlockRejectCount + minConnectTilesRejectCount + connectTileShapeRejectCount;
			LogMsg("Piece %d - State = %d, Reject: Total = %d, MapBlock = %d, MinConnectTiles = %d, ConnectTileShape = %d", 
				index , (int)pieceData.states.size() ,
				totalRejectCount, mapBlockRejectCount, minConnectTilesRejectCount , connectTileShapeRejectCount);
		}
	}

	int Solver::solveImpl(int index, int startIndex)
	{
		SolveOption option;
		option.bTestMinConnectTiles = true;
		option.bTestConnectTileShape = true;

		int earlyRejectionCount = 0;

		auto CheckPieceFunc = [&index](Piece* testPiece)
		{
			return testPiece->index > index;
		};

		std::vector<int> counts;
		counts.resize(mPieceList.size(), 0);
		while (index >= startIndex)
		{
			PieceSolveData& pieceData = mPieceList[index];
			if (mSolveData.advanceState(pieceData, index))
			{
				if (1 <= index && index < 2 * mPieceList.size() / 3 )
				{
					PieceSolveState const& state = pieceData.states[mSolveData.stateIndices[index]];
					auto result = mSolveData.testRejection(*this, state.pos, pieceData.shape->mDataMap[state.dir], option, CheckPieceFunc);

					if (result != ERejectResult::None)
					{
						++earlyRejectionCount;
						++counts[index];
						continue;
					}
				}

				++index;
				if (index == mPieceList.size())
					break;
			}
			else
			{
				--index;
			}
		}

		LogMsg("earlyRejectionCount = %d", earlyRejectionCount);
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