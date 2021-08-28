#include "GomokuCore.h"
#include "Core/ScopeExit.h"

namespace Gomoku
{
#define GOMOKU_DEBUG_LOG 1

#define GOMOKU_DEBUG_VISUAL 1

#if GOMOKU_DEBUG_VISUAL
#define EMIT_POS(index,type) GDebugListener->emitDebugPos(index, EDebugType::type);
#else
#define EMIT_POS(...) 
#endif

	IDebugListener* GDebugListener = nullptr;

	using ConDataType = int16;
	struct FCon
	{
		union InternalType
		{
			struct
			{
				uint16 count : 13;
				uint16 blockMask  : 2;
				uint16 bRoot : 1;
			};

			int16 value;
		};

		static ConDataType Init()
		{
			InternalType result;
			result.value = 0;
			result.bRoot = 1;
			return result.value;
		}

		static void Merge(ConDataType& value, ConDataType other)
		{
			InternalType& iValue = reinterpret_cast<InternalType&>(value);
			InternalType& iOther = reinterpret_cast<InternalType&>(other);
			CHECK((iValue.blockMask & iOther.blockMask) == 0);
			iValue.count += iOther.count;
			iValue.blockMask |= iOther.blockMask;
		}
		static void AddConnect(ConDataType& value)
		{
			InternalType& iValue = reinterpret_cast<InternalType&>(value);
			iValue.count += 1;
		}
		static void RemoveConnect(ConDataType& value)
		{
			InternalType& iValue = reinterpret_cast<InternalType&>(value);
			iValue.count -= 1;
		}
		static void AddBlock(ConDataType& value, bool bPositive)
		{
			InternalType& iValue = reinterpret_cast<InternalType&>(value);
			uint16 mask = bPositive ? 0x1 : 0x2;
			CHECK((iValue.blockMask & mask) == 0);
			iValue.blockMask |= mask;
		}
		static void RemoveBlock(ConDataType& value, bool bPositive)
		{
			InternalType& iValue = reinterpret_cast<InternalType&>(value);
			uint16 mask = bPositive ? 0x1 : 0x2;
			CHECK((iValue.blockMask & mask) != 0);
			iValue.blockMask &= ~mask;
		}
		static int ConnectCount(ConDataType value)
		{
			InternalType& iValue = reinterpret_cast<InternalType&>(value);
			return iValue.count;
		}
		static int BlockMask(ConDataType value)
		{
			InternalType& iValue = reinterpret_cast<InternalType&>(value);
			return iValue.blockMask;
		}
		static int BlockCount(ConDataType value)
		{
			InternalType& iValue = reinterpret_cast<InternalType&>(value);
			if (iValue.blockMask)
			{
				if (iValue.blockMask == 0x3)
					return 2;

				return 1;
			}
			return 0;
		}
	};

	void Board::setup(int size, bool bClear /*= true*/)
	{
		if (mSize == size)
			return;

		if (mSize < size)
		{
			mSize = size;
			int dataSize = getDataSize();
			mData = std::make_unique<char[]>(dataSize);
			for (int i = 0; i < EConDir::Count; ++i)
			{
				mConData[i] = std::make_unique<ConDataType[]>(dataSize);
			}
		}
		else
		{
			mSize = size;
		}

		initIndexOffset();
		if (bClear)
		{
			clear();
		}
		else
		{
			int dataSize = getDataSize();
			setupDataEdge(dataSize);
		}
	}

	void Board::clear()
	{
		int dataSize = getDataSize();
		std::fill_n(mData.get(), dataSize, (char)EStoneColor::Empty);

		for (int i = 0; i < EConDir::Count; ++i)
		{
			std::fill_n(mConData[i].get(), dataSize, 0);
		}

		setupDataEdge(dataSize);
	}

	void Board::putStone(int index, DataType color)
	{
		CHECK(color != EStoneColor::Empty);
		CHECK(mData[index] == EStoneColor::Empty);
		mData[index] = color;

		int oppositeColor = EStoneColor::Opposite(color);

		for (int conDir = 0; conDir < EConDir::Count; ++conDir)
		{
			int const* pOffset = EConDir::GetOffset(EConDir::Type(conDir));
			ConDataType* conData = mConData[conDir].get();

			ConDataType cValue = FCon::Init();
			FCon::AddConnect(cValue);
			int indexLinkRoot = INDEX_NONE;
			int removeRootCount = 0;
			int conIndexA = offsetIndex(index, pOffset[0], pOffset[1]);
			int dataA = mData[conIndexA];
			int countA = -1;
			if (dataA != EDGE_MARK)
			{
				if (dataA == color)
				{
					int indexLinkRootA = GetLinkRoot(conIndexA, conData);
					FCon::Merge(cValue, conData[indexLinkRootA]);

					if (indexLinkRootA != conIndexA)
					{
						indexLinkRoot = indexLinkRootA;
					}
					else
					{
						countA = FCon::ConnectCount(conData[indexLinkRootA]);
						conData[indexLinkRootA] = index;
						++removeRootCount;
					}
				}
				else if (dataA != EStoneColor::Empty)
				{
					int indexLinkRootA = GetLinkRoot(conIndexA, conData);
					FCon::AddBlock(conData[indexLinkRootA], false);
					FCon::AddBlock(cValue, true);
				}
			}
			else
			{
				FCon::AddBlock(cValue, true);
			}

			int conIndexB = offsetIndex(index, -pOffset[0], -pOffset[1]);
			int dataB = mData[conIndexB];
			if (dataB != EDGE_MARK )
			{
				if (dataB == color)
				{
					int indexLinkRootB = GetLinkRoot(conIndexB, conData);
					FCon::Merge(cValue, conData[indexLinkRootB]);

					if (indexLinkRootB != conIndexB)
					{
						if (indexLinkRoot != INDEX_NONE)
						{
							conData[indexLinkRootB] = indexLinkRoot;
						}
						else
						{
							indexLinkRoot = indexLinkRootB;
						}
					}
					else
					{
						conData[indexLinkRootB] = index;
						++removeRootCount;
					}

				}
				else if (dataB != EStoneColor::Empty)
				{
					int indexLinkRootB = GetLinkRoot(conIndexB, conData);
					FCon::AddBlock(conData[indexLinkRootB], true);
					FCon::AddBlock(cValue, false);
				}
			}
			else
			{
				FCon::AddBlock(cValue, false);
			}

			if (removeRootCount == 2)
			{
				CHECK(countA != -1);
				indexLinkRoot = offsetIndex(index, countA * pOffset[0], countA * pOffset[1]);
				conData[index] = indexLinkRoot;
				conData[indexLinkRoot] = cValue;
			}

			if (indexLinkRoot != INDEX_NONE)
			{
				conData[index] = indexLinkRoot;
				conData[indexLinkRoot] = cValue;
			}
			else
			{
				conData[index] = cValue;
			}
		}
	}

	void Board::removeStone(int index)
	{
		DataType color = mData[index];
		if (color != EStoneColor::Black && 
			color != EStoneColor::White)
			return;

		mData[index] = EStoneColor::Empty;
		int oppositeColor = EStoneColor::Opposite(color);
		for (int conDir = 0; conDir < EConDir::Count; ++conDir)
		{
			int const* pOffset = EConDir::GetOffset(EConDir::Type(conDir));
			ConDataType* conData = mConData[conDir].get();

			auto CheckRemoveBlock = [&](int index, bool bPositive)
			{
				CHECK(mData[index] != color);
				if (mData[index] == oppositeColor)
				{
					int indexLinkRoot = GetLinkRoot(index, conData);
					FCon::RemoveBlock(conData[indexLinkRoot], bPositive);
				}
			};

			int conIndexA = offsetIndex(index, pOffset[0], pOffset[1]);
			int conIndexB = offsetIndex(index, -pOffset[0], -pOffset[1]);
			if (mData[conIndexA] == color)
			{
				if (mData[conIndexB] == color)
				{
					reconnect(conIndexA, pOffset, conData, color, true);
					int pOffsetB[2] = { -pOffset[0] , -pOffset[1] };
					reconnect(conIndexB, pOffsetB, conData, color, false);
				}
				else
				{
					CheckRemoveBlock(conIndexB, true);

					int indexLinkRoot = GetLinkRoot(conIndexA, conData);
					if (indexLinkRoot == index)
					{
						reconnect(conIndexA, pOffset, conData, color, true);
						indexLinkRoot = conIndexA;
					}
					else
					{
						FCon::RemoveConnect(conData[indexLinkRoot]);
						if (mData[conIndexB] == oppositeColor)
						{
							FCon::RemoveBlock(conData[indexLinkRoot], false);
						}
					}
				}
			}
			else
			{
				CheckRemoveBlock(conIndexA, false);

				if (mData[conIndexB] == color)
				{
					int indexLinkRoot = GetLinkRoot(conIndexB, conData);
					if (indexLinkRoot == index)
					{
						int pOffsetB[2] = { -pOffset[0] , -pOffset[1] };
						reconnect(conIndexB, pOffsetB, conData, color, false);
						indexLinkRoot = conIndexB;
					}
					else
					{
						FCon::RemoveConnect(conData[indexLinkRoot]);
						if (mData[conIndexA] == oppositeColor)
						{
							FCon::RemoveBlock(conData[indexLinkRoot], true);
						}
					}
				}
				else
				{
					CheckRemoveBlock(conIndexB, true);
				}
			}
		}
	}

	int Board::getConnectEndpoints(int index, EConDir::Type dir, int outIndices[2]) const
	{
		//link root index always in endpoint
		int const* pOffset = EConDir::GetOffset(dir);
		ConDataType* conData = mConData[dir].get();
		int indexLinkRoot = GetLinkRoot(index, conData);
		int count = FCon::ConnectCount(conData[indexLinkRoot]);
		if (count == 1)
		{
			outIndices[0] = indexLinkRoot;
			outIndices[1] = indexLinkRoot;
		}
		else
		{
			int dist = count - 1;
			int indexNext = offsetIndex(indexLinkRoot, pOffset[0], pOffset[1]);
			if (mData[indexNext] == mData[index])
			{
				outIndices[0] = offsetIndex(indexLinkRoot, dist * pOffset[0], dist * pOffset[1]);
				outIndices[1] = indexLinkRoot;
			}
			else
			{
				outIndices[0] = indexLinkRoot;
				outIndices[1] = offsetIndex(indexLinkRoot, -dist * pOffset[0], -dist * pOffset[1]);
			}
		}

		return count;
	}

	int Board::GetLinkRoot(int index, int16* conData)
	{
#if 0
		int result = index;
		int cur;
		while (( cur = conData[result] ) > 0)
		{
			result = cur;
		}
		return result;
#else
		if (conData[index] <= 0)
			return index;

		int prev = index;
		int result = conData[index];
		int next;
		while ((next = conData[result]) > 0)
		{
			conData[prev] = next;
			prev = result;
			result = next;
		}

		conData[index] = result;
		return result;
#endif
	}

	void Board::reconnect(int index, int const* pOffset, int16* conData, int color, bool bPositive)
	{
		CHECK(mData[index] == color);
		ConDataType cValue = FCon::Init();
		FCon::AddConnect(cValue);
		int curIndex = index;
		int oppositeColor = EStoneColor::Opposite(color);
		for (;;)
		{
			curIndex = offsetIndex(curIndex, pOffset[0], pOffset[1]);
			int data = mData[curIndex];
			if (data != color)
			{
				if (data == oppositeColor)
				{
					FCon::AddBlock(cValue, bPositive);
				}
				break;
			}

			FCon::AddConnect(cValue);
			conData[curIndex] = index;
		}

		conData[index] = cValue;
	}

	int Board::getLinkRoot(int index, EConDir::Type dir) const
	{
		ConDataType* conData = mConData[dir].get();
		return GetLinkRoot(index, conData);
	}

	Board::LineInfo Board::getLineInfo(int index, EConDir::Type dir)
	{
		LineInfo result;
		ConDataType* conData = mConData[dir].get();
		int indexLinkRoot = GetLinkRoot(index, conData);

		result.blockMask = FCon::BlockMask(conData[indexLinkRoot]);
		result.count = FCon::ConnectCount(conData[indexLinkRoot]);
		result.color = mData[index];
		result.dir = dir;

		if (result.count == 1)
		{
			result.endpoints[0] = indexLinkRoot;
			result.endpoints[1] = indexLinkRoot;
		}
		else
		{
			int dist = result.count - 1;
			int const* pOffset = EConDir::GetOffset(dir);
			int indexNext = offsetIndex(indexLinkRoot, pOffset[0], pOffset[1]);
			if (mData[indexNext] == mData[index])
			{
				result.endpoints[0] = offsetIndex(indexLinkRoot, dist * pOffset[0], dist * pOffset[1]);
				result.endpoints[1] = indexLinkRoot;
			}
			else
			{
				result.endpoints[0] = indexLinkRoot;
				result.endpoints[1] = offsetIndex(indexLinkRoot, -dist * pOffset[0], -dist * pOffset[1]);
			}
		}

		return result;
	}

	int Board::getConnectCount(int index, EConDir::Type dir) const
	{
		ConDataType* conData = mConData[dir].get();
		int indexRoot = GetLinkRoot(index, conData);
		return FCon::ConnectCount(conData[indexRoot]);
	}

	int Board::getConnectCount(int index, EConDir::Type dir, int& outBlockCount) const
	{
		ConDataType* conData = mConData[dir].get();
		int indexRoot = GetLinkRoot(index, conData);

		outBlockCount = FCon::BlockCount(conData[indexRoot]);
		return FCon::ConnectCount(conData[indexRoot]);
	}

	int Board::getConnectCountWithMask(int index, EConDir::Type dir, uint& outBlockMask) const
	{
		ConDataType* conData = mConData[dir].get();
		int indexRoot = GetLinkRoot(index, conData);

		outBlockMask = FCon::BlockMask(conData[indexRoot]);
		return FCon::ConnectCount(conData[indexRoot]);
	}

	bool Game::canPlayStone(int x, int y) const
	{
		if (!mBoard.checkRange(x, y))
			return false;

		auto pos = mBoard.getPos(x, y);
		if (mBoard.getData(pos) != EStoneColor::Empty)
			return false;

		if (shouldCheckBanMove(mNextPlayColor))
		{
			return isBanMovePutStone(pos.toIndex(), mNextPlayColor);
		}
		return true;
	}

	bool Game::playStone(Pos const& pos)
	{
		if (mBoard.getData(pos) != EStoneColor::Empty)
			return false;

		mBoard.putStone(pos.toIndex(), mNextPlayColor);
		if ( shouldCheckBanMove(mNextPlayColor) )
		{
			if (IsBanMove(checkStatusInternal<true>(pos.toIndex(), mNextPlayColor)))
			{
				mBoard.removeStone(pos.toIndex());
				return false;
			}
		}

		StepInfo step;
		step.bPlay = true;
		step.indexPos = pos.toIndex();
		step.colorAdded = 0;
		mStepHistory.push_back(step);

		++mCurrentStep;
		if (mNextPlayColor == EStoneColor::Black)
		{
			mNextPlayColor = EStoneColor::White;
		}
		else
		{
			mNextPlayColor = EStoneColor::Black;
		}
	}

	EGameStatus Game::checkStatus() const
	{
		if (mCurrentStep == 0)
			return EGameStatus::Nothing;

		StepInfo const& step = mStepHistory[mCurrentStep - 1];

		int color = EStoneColor::Opposite(mNextPlayColor);

		if (shouldCheckBanMove(color))
		{
			return checkStatusInternal<true>(step.indexPos, color);
		}
		else
		{
			return checkStatusInternal<false>(step.indexPos, color);
		}
	}

	bool Game::undo()
	{
		if (mCurrentStep == 0)
			return false;

		StepInfo& step = mStepHistory[mCurrentStep - 1];
		if (step.indexPos != INDEX_NONE)
		{
			mBoard.removeStone(step.indexPos);
		}

		if (step.bPlay)
		{
			mNextPlayColor = EStoneColor::Opposite(mNextPlayColor);
		}

		--mCurrentStep;
		mStepHistory.pop_back();

		return true;
	}

	int Game::isAliveThree(LineInfo const& line, int outNextPos[2]) const
	{
		CHECK(line.blockMask == 0);

		int const* pOffset = EConDir::GetOffset(line.dir);
		auto CheckJumpPointNotColor = [this, pOffset](int const endpoints[2], int dist , int color) -> bool
		{
			int indexJumpA = mBoard.offsetIndex(endpoints[0], dist * pOffset[0], dist * pOffset[1]);
			EMIT_POS(indexJumpA, CHECK_POS);
			if (mBoard.getData(indexJumpA) == color)
				return false;

			int indexJumpB = mBoard.offsetIndex(endpoints[1], -dist * pOffset[0], -dist * pOffset[1]);
			EMIT_POS(indexJumpB, CHECK_POS);
			if (mBoard.getData(indexJumpB) == color)
				return false;

			return true;
		};

		switch (line.count)
		{
		case 1:
		case 2:
			{
				for (int indexEndpoint = 0; indexEndpoint < 2; ++indexEndpoint)
				{
					int const* pOffset = EConDir::GetOffset(line.dir, indexEndpoint);
					int indexJump2 = mBoard.offsetIndex(line.endpoints[indexEndpoint], 2 * pOffset[0], 2 * pOffset[1]);
					EMIT_POS(indexJump2, CHECK_POS);
					if (mBoard.getData(indexJump2) != line.color)
						continue;
					
					LineInfo jumpLine = mBoard.getLineInfo(indexJump2, line.dir);
					if (jumpLine.count + line.count != 3)
						continue;

					if (jumpLine.blockMask)
						return 0;
					//-o-oo-
					int endpoints[2];
					if (indexEndpoint == 0)
					{
						endpoints[0] = jumpLine.endpoints[0];
						endpoints[1] = line.endpoints[1];
					}
					else
					{
						endpoints[0] = line.endpoints[0];
						endpoints[1] = jumpLine.endpoints[1];
					}

					EMIT_POS(endpoints[0], CHECK_POS);
					EMIT_POS(endpoints[1], CHECK_POS);
					//check over 5 case
					if (CheckJumpPointNotColor(endpoints, 2, line.color))
					{
						outNextPos[0] = mBoard.offsetIndex(line.endpoints[indexEndpoint], pOffset[0], pOffset[1]);
						EMIT_POS(outNextPos[0], NEXT_POS);
						return 1;
					}
				}
			}
			break;
		case 3:
			{
				int numPos = 0;
				int indexJump;

				for( int indexEndpoint = 0; indexEndpoint < 2; ++indexEndpoint )
				{
					EMIT_POS(line.endpoints[indexEndpoint], CHECK_POS);
					int const* pOffset = EConDir::GetOffset(line.dir, indexEndpoint);

					int indexJump2 = mBoard.offsetIndex(line.endpoints[indexEndpoint], 2 * pOffset[0], 2 * pOffset[1]);
					EMIT_POS(indexJump2, CHECK_POS);
					if (mBoard.getData(indexJump2) != EStoneColor::Empty)
						continue;
					
					//ignore over5 case
					int indexJump3 = mBoard.offsetIndex(indexJump2, pOffset[0], pOffset[1]);
					EMIT_POS(indexJump3, CHECK_POS);
					if (mBoard.getData(indexJump3) == line.color)
						continue;
		
					outNextPos[numPos] = mBoard.offsetIndex(line.endpoints[indexEndpoint], pOffset[0], pOffset[1]);
					EMIT_POS(outNextPos[numPos], NEXT_POS);
					++numPos;		
				}

				return numPos;
			}
			break;
		}

		return 0;
	}

	int Game::isAnyFour(LineInfo const& line, int outNextPos[2]) const
	{
		CHECK(line.blockMask != 0x3);

		switch (line.count)
		{
		case 1:
		case 2:
		case 3:
			{
				int numPos = 0;
				for (int indexEndpoint = 0; indexEndpoint < 2; ++indexEndpoint)
				{
					if (line.blockMask & (1 << indexEndpoint))
						continue;

					int const* pOffset = EConDir::GetOffset(line.dir, indexEndpoint);
					int indexJump2 = mBoard.offsetIndex(line.endpoints[1], 2 * pOffset[0], 2 * pOffset[1]);
					if (mBoard.getData(indexJump2) != line.color)
						continue;

					int jump2Count = mBoard.getConnectCount(indexJump2, line.dir);
					if (jump2Count + line.count != 4)
						continue;

					outNextPos[numPos] = mBoard.offsetIndex(line.endpoints[indexEndpoint], pOffset[0], pOffset[1]);
					++numPos;
				}

				return numPos;
			}
			break;
		case 4:
			{
				int numPos = 0;
				int indexJump;

				for (int indexEndpoint = 0; indexEndpoint < 2; ++indexEndpoint)
				{
					if (line.blockMask & (1 << indexEndpoint))
						continue;

					int const* pOffset = EConDir::GetOffset(line.dir, indexEndpoint);

					//ignore over5 case
					int indexJump2 = mBoard.offsetIndex(line.endpoints[indexEndpoint], 2 * pOffset[0], 2 * pOffset[1]);
					if (mBoard.getData(indexJump2) == line.color)
						continue;

					outNextPos[numPos] = mBoard.offsetIndex(line.endpoints[indexEndpoint], pOffset[0], pOffset[1]);
					++numPos;

				}

				return numPos;
			}
			break;
		}

		return 0;
	}

	bool Game::isBanMovePutStone(int index, int color) const
	{
		CHECK(mBoard.getData(index) == EStoneColor::Empty);

		mBoard.putStone(index, color);
		EGameStatus status = checkStatusInternal<true>(index, color);
	
		bool result = IsBanMove(status);
#if GOMOKU_DEBUG_LOG
		if (result)
		{
			int coord[2];
			mBoard.getPosCoord(index, coord);
			LogMsg("Ban Move : (%d , %d ) = %d", coord[0] , coord[1], (int)status);
		}
		else if (status == EGameStatus::BlackWin)
		{
			int coord[2];
			mBoard.getPosCoord(index, coord);
			LogMsg("Win Move : (%d , %d ) = %d", coord[0], coord[1], (int)status);
		}
#endif

		mBoard.removeStone(index);
		return result;
	}

	template< bool bCheckBanMove >
	EGameStatus Game::checkStatusInternal(int index, int color) const
	{
		CHECK(mBoard.getData(index) == color);

		int const WinCount = 5;

		if (bCheckBanMove == false)
		{
			for (int dir = 0; dir < EConDir::Count; ++dir)
			{
				int count = mBoard.getConnectCount(index, EConDir::Type(dir));
				if (count >= WinCount)
					return color == EStoneColor::Black ? EGameStatus::BlackWin : EGameStatus::WhiteWin;
			}
		}
		else
		{
			bool bOver5 = false;
			for (int dir = 0; dir < EConDir::Count; ++dir)
			{
				int count = mBoard.getConnectCount(index, EConDir::Type(dir));
				if (count == WinCount)
					return color == EStoneColor::Black ? EGameStatus::BlackWin : EGameStatus::WhiteWin;
				else if (count > WinCount)
					bOver5 = true;
			}

			if (bOver5)
				return EGameStatus::BanMove_Over5;

			int cAliveThree = 0;
			int cAnyFour = 0;
			for (int dir = 0; dir < EConDir::Count; ++dir)
			{
				LineInfo line = mBoard.getLineInfo(index, EConDir::Type(dir));

				if (line.blockMask == 0)
				{
					int outPos[2];
					int numPos = isAliveThree(line, outPos);
					if (numPos)
					{
						int numSaved = numPos;
						for (int i = 0; i < numSaved; ++i)
						{
							if ( isBanMovePutStone(outPos[i], color))
							{
								--numPos;
							}
						}
						
						if (numPos != 0)
						{
							++cAliveThree;
						}
						else
						{
							CHECK(isAnyFour(line, outPos) == false);
						}
					}
					else if (isAnyFour(line, outPos))
					{
						++cAnyFour;
					}
				}
				else if (line.blockMask != 0x3)
				{
					int outPos[2];
					if (isAnyFour(line, outPos))
					{
						++cAnyFour;
					}
				}
			}

			if (cAliveThree >= 2)
				return EGameStatus::BanMove_33;
			if (cAnyFour >= 2)
				return EGameStatus::BanMove_44;
		}


		return EGameStatus::Nothing;
	}


}//namespace Gomoku