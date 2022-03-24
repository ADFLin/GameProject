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

			COUNT ,
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
		FirstMove,
		PawnTwoStepMove,
		Unspecified,
	};

	enum class EMoveTag : uint8
	{
		Normal,
		EnPassant,
		PawnTwoStepMove,
		Castling,
		Promotion,
	};

	struct MoveInfo
	{
		Vec2i        pos;
		Vec2i        posEffect;
		EMoveTag     tag;
		bool         bCapture;

		MoveInfo() = default;

		static MoveInfo WithCapture(Vec2i const& inPos)
		{
			MoveInfo result;
			result.pos = inPos;
			result.posEffect = inPos;
			result.bCapture = true;
			result.tag = EMoveTag::Normal;
			return result;
		}

		static MoveInfo WithCapture(Vec2i const& inPos, EMoveTag inTag, Vec2i const& inPosEffect)
		{
			MoveInfo result;
			result.pos = inPos;
			result.posEffect = inPosEffect;
			result.bCapture = true;
			result.tag = inTag;
			return result;

		}
		MoveInfo(Vec2i const& inPos)
			:pos(inPos), tag(EMoveTag::Normal),bCapture(false)
		{

		}

		MoveInfo(Vec2i const& inPos, EMoveTag inTag)
			:pos(inPos), tag(inTag), bCapture(false)
		{

		}
		MoveInfo(Vec2i const& inPos, EMoveTag inTag, Vec2i const& inPosEffect)
			:pos(inPos), tag(inTag), posEffect(inPosEffect), bCapture(false)
		{

		}

		bool operator == (MoveInfo const& rhs) const
		{
			if (pos != rhs.pos)
				return false;
			if (tag != rhs.tag)
				return false;
			if (bCapture != rhs.bCapture)
				return false;

			if (tag == EMoveTag::Castling || tag == EMoveTag::EnPassant)
			{
				if (posEffect != rhs.posEffect)
					return false;
			}
			return true;
		}
	};

	struct GameStateData
	{
		uint32 trun;
		std::vector<uint8> board;
		std::vector<uint8> history;
	};


	class Game
	{
	public:
		struct TileData;

		struct ChessData
		{
			EChessColor  color;
			EChess::Type type;
			EMoveState   moveState;
			int          lastMoveTurn;
		};

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

		void restart();

		bool isValidPos(Vec2i const& pos) const
		{
			return mBoard.checkRange(pos.x, pos.y);
		}


		TileData const& getTile(Vec2i const& pos) const
		{
			return mBoard.getData(pos.x, pos.y);
		}

		bool isValidMove(Vec2i const& from, MoveInfo const& move) const;
		bool getPossibleMove(Vec2i const& pos, std::vector<MoveInfo>& outMoveList) const;
		void moveChess(Vec2i const& from, MoveInfo const& move);
		void promotePawn(MoveInfo const& move, EChess::Type promotionType);

		void undo();


		static Vec2i GetForwardDir(EChessColor color)
		{
			return (color == EChessColor::White) ? Vec2i(0, 1) : Vec2i(0, -1);
		}

		bool getPossibleMove(Vec2i const& pos, EChess::Type type, EChessColor color, EMoveState moveState, std::vector<MoveInfo>& outPosList, bool bCheckAttack = false) const;

		void updateAttackTerritory();

		void advanceTurn()
		{
			++mCurTurn;
		}

		EChessColor getCurTurnColor() const
		{
			return EChessColor(mCurTurn & 1);
		}

		void saveState(GameStateData& stateData);
		bool loadState(GameStateData const& stateData);

		struct TileData;

		int mCurTurn;
		TGrid2D<TileData> mBoard;
		struct TurnInfo
		{
			Vec2i     pos;
			MoveInfo  move;
			ChessData chessMoved;
			ChessData chessOther;
		};
		std::vector< TurnInfo >	 mTurnHistory;
		std::vector< ChessData > mChessList;	
		std::vector< ChessData* > mFreeChessList;
	};

}//namespace Chess
#endif // ChessCore_H_8E23099C_2165_4825_BD34_AB1EDFA72499
