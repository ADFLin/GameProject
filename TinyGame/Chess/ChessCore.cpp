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

	void Game::restart()
	{
		mBoard.resize(BOARD_SIZE, BOARD_SIZE);

		mCurTurn = 0;

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
				chess.lastMoveTurn = -1;
				mChessList.push_back(chess);

				data.chess = &mChessList.back();
			}
			else
			{
				data.chess = nullptr;
			}
		}

		updateAttackTerritory();
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

	bool Game::getPossibleMove(Vec2i const& pos, EChess::Type type, EChessColor color, EMoveState moveState, std::vector<MoveInfo>& outMoveList, bool bCheckAttack /*= false*/) const
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
						outMoveList.push_back(posCheck);
					}
				}
				else
				{
					outMoveList.push_back(MoveInfo(posCheck, EMoveTag::Normal, true));
				}

				return false;
			}

			outMoveList.push_back(posCheck);
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

		size_t oldSize = outMoveList.size();
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
						auto CheckCastling = [&](Vec2i const& startPos, Vec2i const& dir) -> bool
						{
							Vec2i posCheck = startPos;
							int dist = 0;
							for (;;)
							{
								dist += 1;
								posCheck += dir;
								if (!mBoard.checkRange(posCheck.x, posCheck.y))
									return false;

								int const KingMoveDist = 2;
								TileData const& data = mBoard.getData(posCheck.x, posCheck.y);
								if (data.chess != nullptr)
								{
									if (data.chess->color == color &&
										data.chess->type == EChess::Rook &&
										data.chess->moveState == EMoveState::NoMove)
									{
										outMoveList.push_back(MoveInfo(startPos + KingMoveDist * dir, EMoveTag::Castling, posCheck));
										break;
									}

									return false;
								}
								else
								{
									if (dist <= KingMoveDist && data.attackCounts[1 - int(color)] > 0)
										return false;
								}
							}

							return true;
						};

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
				EMoveTag moveTag = EMoveTag::Normal;
				if (color == EChessColor::White)
				{
					if (pos.y == BOARD_SIZE - 2)
						moveTag == EMoveTag::Promotion;

				}
				else
				{
					if (pos.y == 1)
						moveTag == EMoveTag::Promotion;
				}

				Vec2i forwardDir = GetForwardDir(color);
				Vec2i forwardPos = pos + forwardDir;

				auto CheckPawnMove = [&](Vec2i const& posCheck, EMoveTag tag) -> bool
				{
					if (!mBoard.checkRange(posCheck.x, posCheck.y))
						return false;

					TileData const& data = mBoard.getData(posCheck.x, posCheck.y);
					if (data.chess != nullptr)
					{
						return false;
					}
					outMoveList.push_back(MoveInfo(posCheck, tag, false));
					return true;
				};

				if (bCheckAttack == false)
				{
					if (CheckPawnMove(forwardPos, moveTag))
					{
						if (moveState == EMoveState::NoMove)
						{
							CheckPawnMove(forwardPos + forwardDir, EMoveTag::PawnTwoStepMove);
						}
					}
				}

				auto CheckPawnCapture = [&](Vec2i const& posCheck, EMoveTag tag) -> bool
				{
					if (!mBoard.checkRange(posCheck.x, posCheck.y))
						return false;

					TileData const& data = mBoard.getData(posCheck.x, posCheck.y);
					if (data.chess == nullptr || data.chess->color == color)
					{
						if (bCheckAttack)
						{
							outMoveList.push_back(posCheck);
						}
						return false;
					}

					outMoveList.push_back(MoveInfo(posCheck, tag, true));
					return true;
				};

				CheckPawnCapture(forwardPos + Vec2i(1, 0) , moveTag);
				CheckPawnCapture(forwardPos + Vec2i(-1, 0) , moveTag);

				auto CheckEnPassantCapture = [&](Vec2i const& posCheck) -> bool
				{
					if (!mBoard.checkRange(posCheck.x, posCheck.y))
						return false;

					TileData const& data = mBoard.getData(posCheck.x, posCheck.y);
					if (data.chess == nullptr ||
						data.chess->color == color ||
						data.chess->type != EChess::Pawn ||
						data.chess->moveState != EMoveState::PawnTwoStepMove ||
						data.chess->lastMoveTurn != mCurTurn - 1)
						return false;

					outMoveList.push_back(MoveInfo(posCheck + GetForwardDir(color), EMoveTag::EnPassant, true, posCheck));
					return true;
				};

				CheckEnPassantCapture(pos + Vec2i(1, 0));
				CheckEnPassantCapture(pos + Vec2i(-1, 0));
			}
			break;
		}

		return outMoveList.size() != oldSize;
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

		auto UpdateMovedChessState = [](ChessData& chess)
		{
			if (chess.moveState == EMoveState::NoMove)
			{
				chess.moveState = EMoveState::FirstMove;
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
				CHECK(tilePawn.chess && tilePawn.chess->type == EChess::Pawn && move.bCapture);
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
		default:
			if ( move.bCapture )
			{
				RemoveChess(toTile);
			}
		} 

		ChessData* chessMove = fromTile.chess;

		MoveTileChess(fromTile, toTile);
		if (move.tag == EMoveTag::PawnTwoStepMove)
		{
			CHECK(chessMove->moveState == EMoveState::NoMove);
			chessMove->moveState = EMoveState::PawnTwoStepMove;
		}
		else
		{
			UpdateMovedChessState(*chessMove);
		}

		updateAttackTerritory();
		advanceTurn();
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

	union ChessState
	{
		struct
		{
			uint16 bWhite : 1;
			uint16 type : 4;
			uint16 moveState : 3;
			uint16 turnOffset : 8;
		};

		uint8 values[2];
	};

	void Game::saveState(GameStateData& stateData)
	{
		int index = 0;
		stateData.trun = mCurTurn;

		while (index < mBoard.getRawDataSize())
		{
			TileData const& tile = mBoard.getRawData()[index];
			if (tile.chess)
			{
				ChessState chessState;
				chessState.bWhite = tile.chess->color == EChessColor::White;
				chessState.type = uint8(tile.chess->type) + 1;
				chessState.moveState = (uint8)tile.chess->moveState;
				uint32 turnOffset;
				if (tile.chess->lastMoveTurn == -1)
					turnOffset = 0xff;
				else
					turnOffset = Math::Min(0xfe, mCurTurn - tile.chess->lastMoveTurn);
				chessState.turnOffset = (uint8)turnOffset;

				stateData.board.push_back(chessState.values[0]);
				stateData.board.push_back(chessState.values[1]);
			}
			else
			{
				stateData.board.push_back(0);
				uint8 num = 1;
				++index;
				while (index < mBoard.getRawDataSize())
				{
					TileData const& tileCheck = mBoard.getRawData()[index];
					if (tileCheck.chess)
						break;

					++num;
					++index;
				}
				stateData.board.push_back(num);
			}
		}
	}

	bool Game::loadState(GameStateData const& stateData)
	{
		int index = 0;
		mBoard.resize(BOARD_SIZE, BOARD_SIZE);

		mChessList.clear();
		mChessList.reserve(BOARD_SIZE * BOARD_SIZE);

		mCurTurn = stateData.trun;

		uint8 const* pData = stateData.board.data();
		while(pData < stateData.board.data() + stateData.board.size())
		{
			if (*pData)
			{
				ChessState chessState;
				chessState.values[0] = *pData;
				++pData;
				chessState.values[1] = *pData;

				ChessData chess;
				chess.type = EChess::Type(chessState.type - 1);
				chess.color = chessState.bWhite ? EChessColor::White : EChessColor::Black;
				chess.moveState = EMoveState(chessState.moveState);
				if (chessState.turnOffset == 0xff)
					chess.lastMoveTurn = -1;
				else
					chess.lastMoveTurn = stateData.trun - chessState.turnOffset;
				mChessList.push_back(chess);

				TileData& tile = mBoard.getRawData()[index];
				tile.chess = &mChessList.back();
				++index;
				if (index == mBoard.getRawDataSize())
				{
					break;
				}
			}
			else
			{
				++pData;
				int num = *pData;

				for (int i = 0; i < num; ++i)
				{
					TileData& tile = mBoard.getRawData()[index];
					tile.chess = nullptr;
					++index;
					if (index == mBoard.getRawDataSize())
					{
						break;
					}
				}
			}
			++pData;
		}

		if (index != mBoard.getRawDataSize() ||
			pData != stateData.board.data() + stateData.board.size())
		{
			return false;
		}
		
		updateAttackTerritory();
	}

}//namespace Chess
