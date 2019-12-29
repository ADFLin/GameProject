#pragma once
#ifndef FlowFreeSolver_H_C273289E_1CAF_4237_8CCB_D18992EFE62B
#define FlowFreeSolver_H_C273289E_1CAF_4237_8CCB_D18992EFE62B

#include "FlowFreeLevel.h"

#include "Template/ArrayView.h"

#include <array>

#include "minisat/core/Solver.h"

namespace FlowFree
{

	enum ESolverModel
	{
		DepthFirst ,
		SATCell ,
		SATEdge ,
	};

	class ISolverInterface
	{
	public:
		virtual void setup(Level& level) = 0;
		virtual bool solve(Level& level) = 0;
		virtual EdgeMask getConnectMask(Vec2i const& pos) = 0;

		static ISolverInterface* Create(ESolverModel model);
	};

	class DepthFirstSolver
	{
	public:
		void setup(Level& level);

		void scanLogicalCondition()
		{
			for (int sourceIndex : mSourceIndices)
			{
				Cell& cell = mCellMap[sourceIndex];
				for (int dir = 0; dir < DirCount; ++dir)
				{
					if (cell.blockMask & BIT(dir))
						continue;

					Vec2i linkPos = getLinkPos(cell.pos, dir);
					Cell& linkCell = getCell(linkPos);
					if (linkCell.func == CellFunc::Source)
					{
						if (cell.funcMeta == linkCell.funcMeta)
						{
							cell.blockMask |= ~BIT(dir);
						}
						else
						{
							cell.blockMask |= BIT(dir);
						}
					}
				}
			}
		}

		Vec2i getLinkPos(Vec2i const& pos, int dir)
		{
			assert(mCellMap.checkRange(pos.x, pos.y));

			Vec2i result = pos + gDirOffset[dir];
			WarpPos(result.x, mCellMap.getSizeX());
			WarpPos(result.y, mCellMap.getSizeY());
			return result;
		}

		struct Cell
		{
			Vec2i    pos;
			CellFunc func;
			uint8    funcMeta;

			EdgeMask blockMask;
			int8     dump;
		};

		struct  CellState
		{
			int      indexCell;
			int8     indexUsed;

			EdgeMask connectMask;
			EdgeMask blockMaskRequired;
			EdgeMask connectMaskRequired;

			uint32   checkCount;
		};

		struct SolveState
		{
			std::vector< CellState > cells;
			uint32 checkCount = 0;

			void initialize(int size)
			{
				cells.resize(size);
				checkCount = 0;
				for (int i = 0; i < size; ++i)
				{
					CellState& cellState = cells[i];
					cellState.indexUsed = -1;
					cellState.connectMask = 0;
					cellState.checkCount = 0;
				}
			}
		};

		void initializeState(SolveState& state)
		{
			state.initialize(mCellMap.getRawDataSize());
		}

		Cell& getCell(Vec2i const& pos) { return mCellMap.getData(pos.x, pos.y); }
		Cell const& getCell(Vec2i const& pos) const { return mCellMap.getData(pos.x, pos.y); }

		bool solve()
		{
			return solveInternal(mState, 0);
		}
		bool solveNext()
		{
			return solveInternal(mState, mCellMap.getRawDataSize() - 1);
		}

		bool checkAllSourcesConnectCompleted(SolveState& state);
		bool checkSourceConnectCompleted(SolveState& state, int indexSource);

		bool solveInternal(SolveState& state, int indexCell);
		bool sovleSubStep(SolveState& state, int indexCell, CellState& cellState);

		SolveState mState;
		TGrid2D< Cell > mCellMap;
		std::vector< int > mSourceIndices;

		std::vector< int > mSortedSolveSequeces;
	};


	struct SATUtilities
	{
		using SATVar = Minisat::Var;
		using SATLit = Minisat::Lit;
		using Solver = Minisat::Solver;

		static SATLit Lit(SATVar var) { return Minisat::mkLit(var); }


		static void ExactlyOne(Minisat::Solver& solver, Minisat::vec<SATLit> const& literals)
		{
			solver.addClause(literals);
			for (size_t i = 0; i < literals.size(); ++i)
			{
				for (size_t j = i + 1; j < literals.size(); ++j)
				{
					solver.addClause(~literals[i], ~literals[j]);
				}
			}
		}
		static void ExactlyOne(Minisat::Solver& solver, TArrayView< SATVar const > vars)
		{
			Minisat::vec<Minisat::Lit> literals;
			for (auto var : vars)
			{
				literals.push(Lit(var));
			}
			ExactlyOne(solver, literals);
		}

		static void LessEqualOne(Minisat::Solver& solver, TArrayView< SATVar const > vars)
		{
			Minisat::vec<Minisat::Lit> literals;
			for (auto var : vars)
			{
				literals.push(Lit(var));
			}
			LessEqualOne(solver, literals);
		}

		static void LessEqualOne(Minisat::Solver& solver, Minisat::vec<SATLit> const& literals)
		{
			for (size_t i = 0; i < literals.size(); ++i)
			{
				for (size_t j = i + 1; j < literals.size(); ++j)
				{
					solver.addClause(~literals[i], ~literals[j]);
				}
			}
		}
		static void ExactlyTwo(Minisat::Solver& solver, SATVar a, SATVar b, SATVar c, SATVar d)
		{

#define CLAUSE( A , B , C )\
			solver.addClause( ~Lit(A) , ~Lit(B) , ~Lit(C) );\
			solver.addClause( Lit(A), Lit(B), Lit(C));

			CLAUSE(a, b, c);
			CLAUSE(a, b, d);
			CLAUSE(a, c, d);
			CLAUSE(b, c, d);
		}
	};

#define USE_EDGE_SOLVER 0

	class SATSolverBase : public SATUtilities
	{
	public:
		static int const MaxColorCount = 32;

		EdgeMask getConnectMask(Vec2i const& pos)
		{
			return mSolution(pos.x, pos.y).mask;
		}		
		
		struct SolvedCell
		{
			int      color;
			EdgeMask mask;
			//for bridge
			int      color2;
		};

		SolvedCell const& getSolvedCell(Vec2i const& pos)
		{
			return mSolution(pos.x, pos.y);
		}

		TGrid2D< SolvedCell > mSolution;
		TGrid2D< EdgeMask >   mConnectMaskMap;
		Minisat::Solver       mSolver;
	};

	class SATSolverEdge : public SATSolverBase
	{
	public:
		bool solve(Level& level);

		struct EdgeVar
		{
			std::array< SATVar , MaxColorCount > colors;
		};
		struct VarInfo
		{
			EdgeVar edges[2];
		};
		TGrid2D< VarInfo > mVarTable;
	};

	class SATSolverCell : public SATSolverBase
	{
	public:
		enum ConnectType
		{
			CON_LR,
			CON_TB,
			CON_LT,
			CON_LB,
			CON_RT,
			CON_RB,

			ConnectTypeCount,
		};

		static uint8 GetEmptyConnectTypeMask(ConnectType conType)
		{
			static uint8 const Masks[] =
			{
				BIT(EDir::Left) | BIT(EDir::Right) ,
				BIT(EDir::Top) | BIT(EDir::Bottom) ,
				BIT(EDir::Left) | BIT(EDir::Top) ,
				BIT(EDir::Left) | BIT(EDir::Bottom) ,
				BIT(EDir::Right) | BIT(EDir::Top) ,
				BIT(EDir::Right) | BIT(EDir::Bottom) ,
			};

			return Masks[conType];
		}

		static int const* GetEmptyConnectTypeLinkDir(ConnectType conType)
		{
			static int const LinkDirsMap[][2] =
			{
				{ EDir::Left , EDir::Right } ,
				{ EDir::Top  , EDir::Bottom } ,
				{ EDir::Left , EDir::Top } ,
				{ EDir::Left , EDir::Bottom } ,
				{ EDir::Right ,EDir::Top } ,
				{ EDir::Right ,EDir::Bottom } ,
			};

			return LinkDirsMap[conType];
		}


		static uint8 GetSourceConnectTypeMask(ConnectType conType)
		{
			static uint8 const Masks[] =
			{
				BIT(EDir::Left) , BIT(EDir::Right) ,
				BIT(EDir::Top) , BIT(EDir::Bottom) ,
			};

			return Masks[conType];
		}

		static int const* GetSourceConnectTypeLinkDir(ConnectType conType)
		{
			static int const LinkDirsMap[][1] =
			{
				{ EDir::Left  }, { EDir::Right },
				{ EDir::Top   }, { EDir::Bottom } ,
			};
			return LinkDirsMap[conType];
		}



		int   mColorCount;
		Vec2i mSize;

		bool solve(Level& level);

		struct VarInfo
		{
			std::array< SATVar, MaxColorCount > colors;
			std::array< SATVar, ConnectTypeCount > conTypes;
		};

		TGrid2D< VarInfo > mVarMap;

		SATVar getColorVar(Vec2i const& pos, int c)
		{
			assert(c < mColorCount);
			return mVarMap(pos.x, pos.y).colors[c];
		}

		SATVar getConTypeVar(Vec2i const& pos, int t)
		{
			return mVarMap(pos.x, pos.y).conTypes[t];
		}
	};

}//namespace FlowFree

#endif // FlowFreeSolver_H_C273289E_1CAF_4237_8CCB_D18992EFE62B
