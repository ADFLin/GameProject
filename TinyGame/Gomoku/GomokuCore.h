#pragma once
#ifndef GobangCore_H_0DF5DFB2_7D93_474F_BCEB_B17696D7CF09
#define GobangCore_H_0DF5DFB2_7D93_474F_BCEB_B17696D7CF09

#include "Go/GoCore.h"

namespace Gomoku
{
	using namespace GoCore;

	namespace EDebugType
	{
		enum 
		{
			CHECK_POS,
			NEXT_POS,
		};
	};
	class IDebugListener
	{
	public:
		virtual ~IDebugListener() {};
		virtual void emitDebugPos(int index, int type) = 0;
	};
	extern IDebugListener* GDebugListener;

	struct EConDir
	{
		enum Type
		{
			NS,
			NE,
			EW,
			SE,

			Count,
		};

		static int const* GetOffset(Type type)
		{
			static int const OffsetMap[][2] =
			{
				{0,1} , { 1 , 1} , { 1,0 } , { 1, -1}
			};

			return &OffsetMap[type][0];
		}

		static int const* GetOffset(Type type, int indexEndpoint)
		{
			CHECK(indexEndpoint == 0 || indexEndpoint == 1);
			static int const OffsetMap[][2] =
			{
				{0,1} , { 1 , 1} , { 1,0 } , { 1, -1},
				{0,-1} , { -1 , -1} , { -1,0 } , { -1, 1}
			};

			return &OffsetMap[type + 4 * indexEndpoint][0];
		}
	};
	class Board : public BoardBase
	{
	public:
		void setup(int size, bool bClear = true);

		void clear();

		void putStone(int index, DataType color);
		void removeStone(int index);

		int getConnectEndpoints(int index, EConDir::Type dir, int outIndices[2]) const;
		int getConnectCount(int index, EConDir::Type dir) const;
		int getConnectCount(int index, EConDir::Type dir, int& outBlockCount) const;
		int getConnectCountWithMask(int index, EConDir::Type dir, uint& outBlockMask) const;

		void reconnect(int index, int const* pOffset, int16* conData, int color, bool bPositive);

		int getLinkRoot(int index, EConDir::Type dir) const;

		using BoardBase::offsetIndex;
		int      offsetIndex(int idx, int const* pOffset) const { return offsetIndex(idx, pOffset[0], pOffset[1]); }
		int      offsetIndex(int idx, int const* pOffset, int dist) const { return offsetIndex(idx, dist * pOffset[0], dist * pOffset[1]); }

		struct LineInfo
		{
			int color;
			int count;
			int endpoints[2];
			uint blockMask;
			EConDir::Type dir;
		};
		LineInfo getLineInfo(int index, EConDir::Type dir);
		static int GetLinkRoot(int index, int16* conData);
		mutable std::unique_ptr< int16[] > mConData[EConDir::Count];
	};

	enum class EGameStatus
	{
		BanMove_44 = -3,
		BanMove_Over5 = -2,
		BanMove_33 = -1,
		Nothing    = 0,
		BlackWin   = 1,
		WhiteWin   = 2,
		Draw       = 1,
	};

	FORCEINLINE bool IsBanMove(EGameStatus status) { return int(status) < 0; }

	class Game
	{
	public:
		using DataType = Board::DataType;
		using Pos = Board::Pos;
		using LineInfo = Board::LineInfo;

		Board const& getBoard() { return mBoard; }
		static int const StandardBoardSize = 15;
		void setup()
		{
			mBoard.setup(StandardBoardSize);
		}

		void restart()
		{
			doRestart(true);
		}

		void doRestart(bool bClearBoard)
		{
			if (bClearBoard)
			{
				mBoard.clear();
			}
			mCurrentStep = 0;
			mNextPlayColor = EStoneColor::Black;
		}

		bool    canPlayStone(int x, int y) const;

		bool    shouldCheckBanMove(int color) const
		{
			if ( mRule.bBanMove )
				return color == EStoneColor::Black;

			return false;
		}
		bool    playStone(int x, int y)
		{
			if (!mBoard.checkRange(x, y))
				return false;

			return playStone(mBoard.getPos(x, y));
		}

		bool    playStone(Pos const& pos);

		EGameStatus checkStatus() const;

		bool undo();

		int       mCurrentStep;
		mutable Board mBoard;

		struct StepInfo
		{
			int16 indexPos;
			uint8 colorAdded;
			uint8 bPlay;
		};


		int isAliveThree(LineInfo const& line, int outNextPos[2]) const;
		int isAnyFour(LineInfo const& line, int outNextPos[2]) const;

		bool isBanMove(int x, int y) const
		{
			if (!mBoard.checkRange(x, y))
				return false;

			auto pos = mBoard.getPos(x, y);
			int color = mBoard.getData(pos);
			if (color == EStoneColor::Empty)
			{
				return isBanMovePutStone(pos.toIndex(), mNextPlayColor);
			}
			return IsBanMove( checkStatusInternal<true>(pos.toIndex(),color) );
		}

		bool isBanMovePutStone(int index, int color) const;

		template< bool bCheckBanMove >
		EGameStatus checkStatusInternal(int index, int color) const;


		struct Rule
		{
			bool bBanMove;

			Rule()
			{
				bBanMove = true;
			}
		};

		Rule mRule;
		std::vector< StepInfo > mStepHistory;
		DataType  mNextPlayColor;
	};


}//namespace Gomoku

#endif // GobangCore_H_0DF5DFB2_7D93_474F_BCEB_B17696D7CF09