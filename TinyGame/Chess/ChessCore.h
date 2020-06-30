#pragma once
#ifndef ChessCore_H_8E23099C_2165_4825_BD34_AB1EDFA72499
#define ChessCore_H_8E23099C_2165_4825_BD34_AB1EDFA72499

#include "Math/TVector2.h"
#include "DataStructure/Grid2D.h"
#include "RHI/RHICommand.h"

namespace Chess
{
	typedef TVector2<int> Vec2i;

	namespace EChess
	{
		enum Type
		{
			King,
			Queen,
			Bishop,
			Knight,
			Rook,
			Pawn,
		};
	}

	enum class EChessColor
	{
		White = 0,
		Black = 1,
	};
	constexpr int BOARD_SIZE = 8;



	enum class EMoveState
	{
		NoMove,
		FristMove,
		PawnTwoStepMove,
		Unspecified,
	};

	enum class EMoveTag
	{
		Normal,
		EnPassant,
		PawnTwoStepMove,
		Castling,
	};

	struct MoveInfo
	{
		Vec2i      pos;
		EMoveTag   tag;
		Vec2i      posEffect;

		MoveInfo() = default;

		MoveInfo(Vec2i const& inPos)
			:pos(inPos), tag(EMoveTag::Normal)
		{

		}

		MoveInfo(Vec2i const& inPos, EMoveTag inTag)
			:pos(inPos), tag(inTag)
		{

		}

		MoveInfo(Vec2i const& inPos, EMoveTag inTag, Vec2i inPosEffect)
			:pos(inPos), tag(inTag), posEffect(inPosEffect)
		{

		}

		bool operator == (MoveInfo const& rhs) const
		{
			if (pos != rhs.pos)
				return false;
			if (tag != rhs.tag)
				return false;

			if (tag == EMoveTag::Castling || tag == EMoveTag::EnPassant)
			{
				if (posEffect != rhs.posEffect)
					return false;
			}
			return true;
		}
	};

	class Game
	{
	public:

		void restart();

		bool isValidPos(Vec2i const& pos) const
		{
			return mBoard.checkRange(pos.x, pos.y);
		}

		bool isValidMove(Vec2i const& from, MoveInfo const& move) const
		{
			std::vector< MoveInfo > moveList;
			if (getPossibleMove(from, moveList))
			{
				for (auto& moveCheck : moveList)
				{
					if (moveCheck == move)
						return true;
				}
			}
			return false;
		}


		bool getPossibleMove(Vec2i const& pos, std::vector<MoveInfo>& outPosList, bool bCheckAttack = false) const;

		static Vec2i GetForwardDir(EChessColor color)
		{
			return (color == EChessColor::White) ? Vec2i(0, 1) : Vec2i(0, -1);
		}

		bool getPossibleMove(Vec2i const& pos, EChess::Type type, EChessColor color, EMoveState moveState, std::vector<MoveInfo>& outPosList, bool bCheckAttack = false) const;

		void moveChess(Vec2i const& from, MoveInfo const& move);


		void updateAttackTerritory();

		void advanceTurn()
		{
			++mCurTurn;
		}

		EChessColor getCurTurnColor() const
		{
			return EChessColor(mCurTurn & 1);
		}

		struct ChessData
		{
			EChessColor  color;
			EChess::Type type;
			EMoveState   moveState;
			int          lastMoveTurn;
		};

		struct TileData;
		struct AttackInfo
		{
			Vec2i     pos;
			TileData* tile;
			MoveInfo  move;
		};

		struct TileData
		{
			ChessData* chess;
			union
			{
				struct
				{
					int whiteAttackCount;
					int blackAttackCount;
				};

				int attackCounts[2];
			};

			std::vector<AttackInfo> attacks;
		};

		int mCurTurn;
		TGrid2D<TileData> mBoard;
		std::vector< ChessData > mChessList;
	};

}//namespace Chess
#endif // ChessCore_H_8E23099C_2165_4825_BD34_AB1EDFA72499
