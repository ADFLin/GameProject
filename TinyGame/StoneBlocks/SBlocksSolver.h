#pragma once
#ifndef SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D
#define SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D

#include "SBlocksCore.h"

namespace SBlocks
{
	class SolveWork;

	struct SolveOption
	{
		bool bTestMinConnectTiles;
		bool bTestConnectTileShape;
	};


	struct PieceSolveState
	{
		Vec2i pos;
		uint8 dir;
	};


	struct PieceSolveData
	{
		Piece* piece;
		PieceShape* shape;
		std::vector< PieceSolveState > states;
		int index;
	};

	namespace ERejectResult
	{
		enum Type
		{
			None,
			MinConnectTiles,
			ConnectTileShape,
		};
	};

	struct GlobalSolveData
	{
		struct PieceLink
		{
			Piece* piece;
			PieceLink* next;
		};
		std::vector< PieceLink >  mPieceLinkData;
		std::vector< PieceLink* > mPieceSizeMap;
		std::vector< PieceSolveData > mPieceList;
		int minShapeBlockCount;
		int maxShapeBlockCount;


		void setup(Level& level, SolveOption const& option)
		{

		}

	};

	struct SolveData
	{
		MarkMap mMap;
		TGrid2D<int> mTestFrameMap;
		int mTestFrame;
		std::vector<int> stateIndices;

		PieceShapeData mCachedShapeData;
		struct ShapeTest
		{
			Vec2i pos;
			int   count;
			int   mapIndex;
		};
		std::vector< ShapeTest > mCachedPendingTests;

		void setup(Level& level, SolveOption const& option);
		int  countConnectTiles(Vec2i const& pos);
		void getConnectedTilePosList(Vec2i const& pos, std::vector< Int16Point2D >& outPosList);
		bool advanceState(PieceSolveData& pieceData, int indexPiece);

		template< typename TFunc >
		ERejectResult::Type testRejection(GlobalSolveData& globalData, Vec2i const pos,  PieceShapeData& shapeData, SolveOption const& option, TFunc func);
	};


	class Solver : public GlobalSolveData
	{
	public:
		void setup(Level& level);
		bool solve()
		{
			return solveImpl(0, 0) >= 0;
		}
		bool solveNext()
		{
			return solveImpl(mPieceList.size() - 1, 0) >= 0;
		}
		int  solveImpl(int index, int startIndex);

		void getSolvedStates(std::vector< PieceSolveState >& outStates)
		{
			int index = 0;
			for (auto& pieceData : mPieceList)
			{
				int indexState = mSolveData.stateIndices[index];
				CHECK(indexState != INDEX_NONE);
				outStates.push_back(pieceData.states[indexState]);
				++index;
			}
		}
		struct ThreadData
		{
			int startIndex;
			int numIndices;
			MarkMap map;
			std::vector<int> stateIndices;
		};
		SolveData mSolveData;

		void solveParallel(int numTheads);



	};


}// namespace SBlocks

#endif // SBlocksSolver_H_1D8CC0FC_2F43_420E_A6F7_D28F07AE850D