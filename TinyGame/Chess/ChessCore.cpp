#include "ChessCore.h"

namespace Chess
{

#define B -1
#define W  1
#define C( COLOR , TYPE ) (COLOR * ( EChess::TYPE + 1 ))
#define CEmpty 0
	int StandardInitState[]
	{
		C(W , Rook),C(W, Knight),C(W, Bishop),C(W, King),C(W, Queen),C(W, Bishop),C(W, Knight),C(W, Rook),
		C(W , Pawn),C(W, Pawn)  ,C(W, Pawn)  ,C(W, Pawn),C(W, Pawn),C(W, Pawn)  ,C(W, Pawn)  ,C(W, Pawn),
		CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,
		CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,
		CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,
		CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,CEmpty,
		C(B, Pawn),C(B, Pawn)  ,C(B, Pawn)  ,C(B, Pawn),C(B, Pawn),C(B, Pawn)  ,C(B, Pawn)  ,C(B, Pawn),
		C(B, Rook),C(B, Knight),C(B, Bishop),C(B ,King),C(B, Queen),C(B, Bishop),C(B, Knight),C(B, Rook),

	};
#undef C
#undef B
#undef W
#undef CEmpty

	static Vec2i const Dirs[4] = { Vec2i(1,0) ,Vec2i(0,1),Vec2i(-1,0),Vec2i(0,-1) };
	static Vec2i const DiagonalDirs[4] = { Vec2i(1,1) ,Vec2i(-1,1),Vec2i(1,-1),Vec2i(-1,-1) };
	static Vec2i const KnightMoves[8] = { Vec2i(2,1) ,Vec2i(1,2),Vec2i(-1,2),Vec2i(-2,1),Vec2i(2,-1) ,Vec2i(1,-2),Vec2i(-1,-2),Vec2i(-2,-1) };
	static Vec2i const PawnBlackMoves[3] = { Vec2i(0,-1) , Vec2i(-1,-1) , Vec2i(1,-1) };
	static Vec2i const PawnWhiteMoves[3] = { Vec2i(0,1) , Vec2i(-1,1) , Vec2i(1,1) };

	void Game::restart()
	{
		mBoard.resize(BOARD_SIZE, BOARD_SIZE);
		mChessList.clear();
		mChessList.reserve(BOARD_SIZE * BOARD_SIZE);

		for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; ++i)
		{
			TileData& data = mBoard[i];
			if (StandardInitState[i])
			{
				ChessData chess;
				chess.type = EChess::Type(std::abs(StandardInitState[i]) - 1);
				chess.color = StandardInitState[i] > 0 ? EChessColor::White : EChessColor::Black;
				chess.moveState = EMoveState::NoMove;

				mChessList.push_back(chess);

				data.chess = &mChessList.back();
			}
			else
			{
				data.chess = nullptr;
			}
		}

		updateAttackTerritory();

		mCurTurn = 0;
	}

	bool Game::getPossibleMove(Vec2i const& pos, std::vector<MoveInfo>& outPosList, bool bCheckAttack /*= false*/) const
	{
		if (!mBoard.checkRange(pos.x, pos.y))
			return false;

		TileData const& tileData = mBoard.getData(pos.x, pos.y);
		ChessData const* chess = tileData.chess;
		if (chess == nullptr)
			return false;
		return getPossibleMove(pos, chess->type, chess->color, chess->moveState, outPosList, bCheckAttack);
	}

	bool Game::getPossibleMove(Vec2i const& pos, EChess::Type type, EChessColor color, EMoveState moveState, std::vector<MoveInfo>& outPosList, bool bCheckAttack /*= false*/) const
	{
		auto CheckMove = [&](Vec2i const& posCheck) -> bool
		{
			if (!mBoard.checkRange(posCheck.x, posCheck.y))
				return false;
			TileData const& data = mBoard.getData(posCheck.x, posCheck.y);

			if (data.chess != nullptr)
			{
				if (data.chess->color == color)
				{
					if (bCheckAttack)
					{
						outPosList.push_back(posCheck);
					}
				}
				else
				{
					outPosList.push_back(MoveInfo(posCheck, EMoveTag::RemoveChess));
				}

				return false;
			}

			outPosList.push_back(posCheck);
			return true;
		};


		auto CheckLineMove = [&](Vec2i const& startPos, Vec2i const& dir)
		{
			Vec2i posCheck = startPos;
			for (;;)
			{
				posCheck += dir;
				if (!CheckMove(posCheck))
					break;
			}
		};

		auto CheckPawnMove = [&](Vec2i const& posCheck, EMoveTag tag) -> bool
		{
			if (!mBoard.checkRange(posCheck.x, posCheck.y))
				return false;

			TileData const& data = mBoard.getData(posCheck.x, posCheck.y);
			if (data.chess != nullptr)
			{
				return false;
			}
			outPosList.push_back(MoveInfo(posCheck, tag));
			return true;
		};

		auto CheckPawnCapture = [&](Vec2i const& posCheck) -> bool
		{
			if (!mBoard.checkRange(posCheck.x, posCheck.y))
				return false;

			TileData const& data = mBoard.getData(posCheck.x, posCheck.y);
			if (data.chess == nullptr || data.chess->color == color)
			{
				if (bCheckAttack)
				{
					outPosList.push_back(posCheck);
				}
				return false;
			}

			outPosList.push_back(posCheck);
			return true;
		};


		auto CheckEnPassantCapture = [&](Vec2i const& posCheck) -> bool
		{
			if (!mBoard.checkRange(posCheck.x, posCheck.y))
				return false;

			TileData const& data = mBoard.getData(posCheck.x, posCheck.y);
			if (data.chess == nullptr || data.chess->color == color)
				return false;

			if (data.chess->type != EChess::Pawn || data.chess->moveState != EMoveState::PawnTwoStepMove || data.chess->lastMoveTurn != mCurTurn - 1)
				return false;

			outPosList.push_back(MoveInfo(posCheck + GetForwardDir(color), EMoveTag::EnPassant, posCheck));
			return true;
		};

		auto CheckCastling = [&](Vec2i const& startPos, Vec2i const& dir) -> bool
		{
			Vec2i posCheck = startPos;
			for (;;)
			{
				posCheck += dir;
				if (!mBoard.checkRange(posCheck.x, posCheck.y))
					return false;
				TileData const& data = mBoard.getData(posCheck.x, posCheck.y);
				if (data.chess != nullptr)
				{
					if (data.chess->color == color &&
						data.chess->type == EChess::Rook &&
						data.chess->moveState == EMoveState::NoMove)
					{
						outPosList.push_back(MoveInfo(startPos + 2 * dir, EMoveTag::Castling, posCheck));
						break;
					}

					return false;
				}
				else
				{
					if (data.attackCounts[1 - int(color)] > 0)
						return false;
				}
			}

			return true;
		};

		size_t oldSize = outPosList.size();
		switch (type)
		{
		case EChess::King:
			for (auto dir : Dirs)
			{
				CheckMove(pos + dir);
			}
			for (auto dir : DiagonalDirs)
			{
				CheckMove(pos + dir);
			}

			if (bCheckAttack == false)
			{
				if (moveState == EMoveState::NoMove)
				{
					TileData const& curTile = mBoard.getData(pos.x, pos.y);

					if (curTile.attackCounts[1 - int(color)] == 0)
					{
						CheckCastling(pos, Vec2i(-1, 0));
						CheckCastling(pos, Vec2i(1, 0));
					}
				}
			}
			break;
		case EChess::Queen:
			for (auto dir : Dirs)
			{
				CheckLineMove(pos, dir);
			}
			for (auto dir : DiagonalDirs)
			{
				CheckLineMove(pos, dir);
			}
			break;
		case EChess::Bishop:
			for (auto dir : DiagonalDirs)
			{
				CheckLineMove(pos, dir);
			}
			break;
		case EChess::Knight:
			for (auto dir : KnightMoves)
			{
				CheckMove(pos + dir);
			}
			break;
		case EChess::Rook:
			for (auto dir : Dirs)
			{
				CheckLineMove(pos, dir);
			}
			break;
		case EChess::Pawn:
			{
				auto const& PawnMoves = (color == EChessColor::White) ? PawnWhiteMoves : PawnBlackMoves;

				if (bCheckAttack == false)
				{
					if (CheckPawnMove(pos + PawnMoves[0], EMoveTag::Normal))
					{
						if (moveState == EMoveState::NoMove)
						{
							CheckPawnMove(pos + 2 * PawnMoves[0], EMoveTag::PawnTwoStepMove);
						}
					}
				}

				CheckPawnCapture(pos + PawnMoves[1]);
				CheckPawnCapture(pos + PawnMoves[2]);

				CheckEnPassantCapture(pos + Vec2i(1, 0));
				CheckEnPassantCapture(pos + Vec2i(-1, 0));
			}
			break;
		}

		return outPosList.size() != oldSize;
	}

	void Game::moveChess(Vec2i const& from, MoveInfo const& move)
	{
		CHECK(isValidPos(from) && isValidPos(move.pos));

		TileData& fromTile = mBoard.getData(from.x, from.y);
		TileData& toTile = mBoard.getData(move.pos.x, move.pos.y);

		auto RemoveChess = [](TileData& tile)
		{
			tile.chess = nullptr;
		};

		auto UpdateMovedChessState = [](ChessData& chess )
		{
			if (chess.moveState == EMoveState::NoMove)
			{
				chess.moveState = EMoveState::FristMove;
			}
			else
			{
				chess.moveState = EMoveState::Unspecified;
			}
		};

		auto MoveTileChess = [this](TileData& fromTile, TileData& toTile)
		{
			toTile.chess = fromTile.chess;
			toTile.chess->lastMoveTurn = mCurTurn;
			fromTile.chess = nullptr;
		};

		switch (move.tag)
		{
		case EMoveTag::EnPassant:
			{
				Game::TileData& tilePawn = mBoard.getData(move.posEffect.x, move.posEffect.y);
				CHECK(tilePawn.chess && tilePawn.chess->type == EChess::Pawn);
				RemoveChess(tilePawn);
			}
			break;
		case EMoveTag::Castling:
			{
				Game::TileData& tileRook = mBoard.getData(move.posEffect.x, move.posEffect.y);
				CHECK(tileRook.chess && tileRook.chess->type == EChess::Rook);
				Vec2i posRookMoveTo = (from + move.pos) / 2;
				Game::TileData& tileRookMoveTo = mBoard.getData(posRookMoveTo.x, posRookMoveTo.y);
				CHECK(tileRookMoveTo.chess == nullptr);

				MoveTileChess(tileRook, tileRookMoveTo);
				UpdateMovedChessState(*tileRookMoveTo.chess);
			}
			break;
		case EMoveTag::RemoveChess:
			{
				RemoveChess(toTile);
			}
			break;
		}

		ChessData* chessMove = fromTile.chess;

		MoveTileChess(fromTile, toTile);
		if (move.tag == EMoveTag::PawnTwoStepMove)
		{
			chessMove->moveState = EMoveState::PawnTwoStepMove;
		}
		else
		{
			UpdateMovedChessState(*chessMove);
		}
		updateAttackTerritory();

		++mCurTurn;
	}

	void Game::updateAttackTerritory()
	{
		for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; ++i)
		{
			Game::TileData& tileData = mBoard[i];
			tileData.blackAttackCount = 0;
			tileData.whiteAttackCount = 0;
			tileData.attacks.clear();
		}

		std::vector< MoveInfo > moveList;
		for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; ++i)
		{
			TileData& tileData = mBoard[i];
			if (tileData.chess == nullptr)
				continue;

			Vec2i pos;
			mBoard.toCoord(i, pos.x, pos.y);
			moveList.clear();
			getPossibleMove(pos, moveList, true);

			int indexColor = (int)tileData.chess->color;
			for (auto const& move : moveList)
			{
				TileData& moveTileData = mBoard.getData(move.pos.x, move.pos.y);
				moveTileData.attackCounts[indexColor] += 1;

				AttackInfo info;
				info.pos  = pos;
				info.tile = &tileData;
				info.move = move;
				moveTileData.attacks.push_back(info);
			}
		}
	}

}//namespace Chess
