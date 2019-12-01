#include "DataStructure/Grid2D.h"

#include "TestStageHeader.h"
#include "Template/ArrayView.h"
#include "PlatformThread.h"

#include "minisat/core/Solver.h"

#include <array>

namespace FlowFree
{
	int const DirCount = 4;
	Vec2i const gDirOffset[] = { Vec2i(1,0) , Vec2i(0,1) , Vec2i(-1,0) , Vec2i(0,-1) };
	using ColorType = uint8;
	using EdgeMask = uint8;

	namespace EDir
	{
		enum 
		{
			Right ,
			Bottom ,
			Left ,
			Top ,
		};
	}
	int InverseDir(int dir)
	{
		return (dir + (DirCount/2) ) % DirCount;
	}
	FORCEINLINE void WarpPos(int& p, int size)
	{
		if (p < 0)
			p += size;
		else if (p >= size)
			p -= size;
	}

	enum class CellFunc : uint8
	{
		Empty = 0,
		Source,
		Bridge,
		Block ,
	};


	struct Cell
	{
		Cell()
		{
			::memset(this, 0, sizeof(*this));
		}

		int getColor(int dir) const { return colors[dir]; }

		ColorType     colors[DirCount];
		CellFunc      func;
		uint8         funcMeta;
		EdgeMask      blockMask;
		mutable uint8 bFilledCached : 1;
		mutable uint8 bFilledCachedDirty : 1;

		void removeFlow(int dir)
		{
			colors[dir] = 0;
		}

		void setSource(ColorType color)
		{
			assert(color);
			func = CellFunc::Source;
			funcMeta = color;
		}

		ColorType evalFlowOut(int dir) const 
		{ 
			if( colors[dir] )
				return 0;

			return getFunFlowColor(dir);
		}

		int  getNeedBreakDirs( int dir , int outDirs[DirCount])
		{
			int result = 0;
			switch( func )
			{
			case CellFunc::Empty:
				for( int i = 0; i < DirCount; ++i )
				{
					if( colors[i] )
					{
						outDirs[result] = i;
						++result;
					}
				}
				break;
			case CellFunc::Source:
				{
					for( int i = 0; i < DirCount; ++i )
					{
						if( colors[i] )
						{
							assert(colors[i] == funcMeta);
							outDirs[result] = i;
							++result;
							break;
						}
					}
				}
				break;
			case CellFunc::Bridge:
				{

				}
				break;
			default:
				break;
			}
			

			return result;
		}

		bool isFilled() const
		{
			if ( bFilledCachedDirty )
			{
				bFilledCachedDirty = false;
				int count = 0;
				for( int i = 0; i < DirCount; ++i )
				{
					if( colors[i] )
					{
						++count;
					}
				}

				switch( func )
				{
				case CellFunc::Empty:
					bFilledCached = count == 2;
				case CellFunc::Source:
					bFilledCached =  count == 1;
				case CellFunc::Bridge:
					bFilledCached =  count == DirCount;
				default:
					bFilledCached = true;
				}
			}

			return bFilledCached;
		}

		int  getLinkedFlowDir(int dir) const
		{
			ColorType color = colors[dir];
			switch( func )
			{
			case CellFunc::Empty:
				{
					int count = 0;
					assert(color);
					for( int i = 0; i < DirCount; ++i )
					{
						if( i == dir || colors[i] == 0 )
							continue;

						assert(colors[i] == color);
						return i;
					}
				}
				break;
			case CellFunc::Source:
				assert(color == funcMeta);
				return -1;
			case CellFunc::Bridge:
				{
					int invDir = InverseDir(dir);
					if( colors[invDir] )
					{
						assert(colors[invDir] == color);
						return invDir;
					}
				}
				break;

			default:
				assert(0);
			}

			return -1;

		}

		ColorType getFunFlowColor( int dir = -1, ColorType inColor = 0 ) const
		{
			switch( func )
			{
			case CellFunc::Empty:
				{
					int count = 0;
					ColorType outColor = inColor;
					for(ColorType color : colors)
					{
						if( color == 0 )
							continue;

						if( inColor && color != inColor )
							return 0;

						++count;
						if( count >= 2 )
							return 0;

						outColor = color;
					}

					return outColor;
				}
				break;
			case CellFunc::Source:
				{
					for(ColorType color : colors)
					{
						if( color )
						{
							assert(color == funcMeta);
							return 0;
						}
					}

					if( inColor && inColor != funcMeta )
						return 0;

					return funcMeta;
				}
				break;
			case CellFunc::Bridge:
				{
					if( dir == -1 )
					{
						for(ColorType color : colors)
						{
							if( color )
								return color;
						}
					}
					else
					{
						int outColor = colors[InverseDir(dir)];
						if( inColor && outColor != inColor )
							return 0;

						return outColor;
					}
				}
				break;
			default:
				assert(0);
			}

			return 0;
		}

		void flowOut(int dir, ColorType color)
		{
			colors[dir] = color;
			bFilledCachedDirty = true;
		}

		bool flowIn( int dir , ColorType color )
		{
			if( colors[dir] )
				return false;

			if( getFunFlowColor(dir, color) == 0 )
				return false;

			colors[dir] = color;
			bFilledCachedDirty = true;
			return true;
		}
	};

	class Level
	{
	public:

		void setSize(Vec2i const& size)
		{
			mCellMap.resize(size.x, size.y);
		}

		void addMapBoundBlock()
		{
			for (int i = 0; i < mCellMap.getSizeX(); ++i)
			{
				Cell& topCell = mCellMap.getData(i, 0);
				topCell.blockMask |= BIT(3);
				Cell& bottomCell = mCellMap.getData(i, mCellMap.getSizeY() - 1);
				bottomCell.blockMask |= BIT(1);
			}

			for (int i = 0; i < mCellMap.getSizeY(); ++i)
			{
				Cell& leftCell = mCellMap.getData(0,i);
				leftCell.blockMask |= BIT(2);
				Cell& rightCell = mCellMap.getData(mCellMap.getSizeX() - 1,i);
				rightCell.blockMask |= BIT(0);
			}
		}
		Vec2i getSize() const
		{
			return Vec2i( mCellMap.getSizeX(), mCellMap.getSizeY() );
		}

		bool isValidCellPos(Vec2i const& cellPos) const
		{
			return Vec2i(0,0) <= cellPos && cellPos < Vec2i(mCellMap.getSizeX(), mCellMap.getSizeY());
		}

		void addSource(Vec2i const& posA, ColorType color)
		{
			getCellChecked(posA).setSource(color);
			mSourceLocations.push_back(posA);
		}

		void addSource(Vec2i const& posA, Vec2i const& posB, ColorType color)
		{
			getCellChecked(posA).setSource(color);
			mSourceLocations.push_back(posA);
			getCellChecked(posB).setSource(color);
			mSourceLocations.push_back(posB);
		}

		void setCellBlocked(Vec2i const& pos, int dir )
		{
			Vec2i linkPos = getLinkPos(pos, dir);
			getCellChecked(pos).blockMask |= BIT(dir);
			getCellChecked(linkPos).blockMask |= BIT(InverseDir(dir));
		}

		bool getLinkSource( Vec2i const& inPos , Vec2i& outLinkPos ) const
		{
			Cell const& cell = getCellChecked(inPos);

			assert(cell.func == CellFunc::Source);
		}

		Cell const& getCellChecked(Vec2i const& pos) const
		{
			return mCellMap.getData(pos.x, pos.y);
		}

		Cell& getCellChecked(Vec2i const& pos)
		{
			return mCellMap.getData(pos.x, pos.y);
		}

		enum class BreakResult
		{
			NoBreak,
			HaveBreak ,
			HaveBreakSameColor ,
		};

		int checkFilledCellCount() const
		{
			int count = 0;
			visitAllCells([&count](Cell const& cell)
			{
				if (cell.isFilled())
					++count;
			});
			return count;
		}

		BreakResult breakFlow(Vec2i const& pos , int dir , ColorType curColor)
		{
			Cell& cell = getCellChecked( pos );
			BreakResult result = BreakResult::NoBreak;

			int linkDirs[DirCount];
			int numDir = cell.getNeedBreakDirs( dir , linkDirs);

			bool bBreakSameColor = false;
			bool bHaveBreak = false;


			for( int i = 0 ; i < numDir ; ++i )
			{
				int linkDir = linkDirs[i];
				if ( linkDir == -1 )
					continue;
				
				int dist = 0;
				
				ColorType linkColor = cell.colors[linkDir];
				Cell* source = findSourceCell(pos, linkDir, dist);

				auto TryRemoveToSource = [&]() -> bool
				{
					for( int j = i + 1; j < numDir; ++j )
					{
						int otherLinkDir = linkDirs[j];
						if ( otherLinkDir == -1 )
							continue;
						if( cell.colors[linkDirs[j]] == linkColor )
						{
							int distOther = 0;
							Cell* otherSource = findSourceCell(pos, otherLinkDir, distOther);

							if( otherSource )
							{
								int removeDir = (distOther < dist) ? otherLinkDir : linkDir;
								removeFlowToEnd(pos, removeDir);
								linkDirs[j] = -1;
								return true;
							}
						}
					}
					return false;
				};

				if( source )
				{
					if( curColor == 0 )
					{
						if( dist == 0 )
						{
							removeFlowToEnd(pos, linkDirs[i]);
						}
						else
						{
							TryRemoveToSource();
						}				
					}
					else if( linkColor != curColor )
					{
						if( TryRemoveToSource() == false )
						{
							removeFlowLink(pos, linkDir);
						}
					}
				}
				else
				{
					bHaveBreak = true;
					removeFlowToEnd(pos, linkDir);
					if( linkColor == curColor )
						bBreakSameColor = true;
				}
			}

			if( bHaveBreak )
			{
				if( bBreakSameColor )
				{
					result = BreakResult::HaveBreakSameColor;
				}
				else
				{
					result = BreakResult::HaveBreak;
				}
			}

			return result;
		}

		Cell* findSourceCell(Vec2i const& pos, int dir, int& outDist)
		{
			outDist = 0;

			Vec2i curPos = pos;
			int   curDir = dir;

			Cell& cell = getCellChecked(pos);
			if( cell.func == CellFunc::Source )
				return &cell;

			for(;;)
			{
				Vec2i linkPos = getLinkPos(curPos, curDir);
				Cell& linkCell = getCellChecked(linkPos);
				if( linkCell.func == CellFunc::Source )
					return &linkCell;

				int curDirInv = InverseDir(curDir);
				curDir = linkCell.getLinkedFlowDir(curDirInv);
				if( curDir == -1 )
					break;

				curPos = linkPos;
				++outDist;
			}

			return nullptr;
		}

		void removeFlowLink(Vec2i const& pos, int dir)
		{
			Cell& cell = getCellChecked(pos);
			cell.removeFlow(dir);

			Vec2i linkPos = getLinkPos(pos, dir);
			Cell& linkCell = getCellChecked(linkPos);

			int dirInv = InverseDir(dir);
			linkCell.removeFlow(dirInv);
		}

		void removeFlowToEnd(Vec2i const& pos, int dir)
		{
			Vec2i curPos = pos;
			int   curDir = dir;

			Cell* curCell = &getCellChecked(pos);
			for( ;;)
			{
				curCell->removeFlow(curDir);
				Vec2i linkPos = getLinkPos(curPos, curDir);
				Cell& linkCell = getCellChecked(linkPos);

				int curDirInv = InverseDir(curDir);
				curDir = linkCell.getLinkedFlowDir(curDirInv);

				linkCell.removeFlow(curDirInv);

				if ( curDir == -1 )
					break;

				curCell = &linkCell;
				curPos = linkPos;
			}
		}


		ColorType linkFlow( Vec2i const& pos , int dir )
		{
			Cell& cellOut = getCellChecked(pos);

			if( cellOut.blockMask & BIT(dir) )
				return 0;

			ColorType color = cellOut.evalFlowOut(dir);
			if( color == 0 )
				return 0;

			Vec2i linkPos = getLinkPos(pos, dir);
			Cell& cellIn = getCellChecked(linkPos);
			
			int dirIn = InverseDir(dir);
			if ( !cellIn.flowIn( dirIn , color ) )
				return 0;

			cellOut.flowOut(dir, color);
			return color;
		}

		Vec2i getLinkPos(Vec2i const& pos, int dir)
		{
			assert(mCellMap.checkRange(pos.x, pos.y));
			
			Vec2i result = pos + gDirOffset[dir];
			WarpPos(result.x, mCellMap.getSizeX());
			WarpPos(result.y, mCellMap.getSizeY());
			return result;
		}

		Cell* getCellInternal(Vec2i const& pos)
		{
			if( !mCellMap.checkRange(pos.x, pos.y) )
				return nullptr;
			return &mCellMap.getData(pos.x, pos.y);
		}


		template< class TFunc >
		void visitAllCellsBreak(TFunc&& func) const
		{
			for (int i = 0; i < mCellMap.getRawDataSize(); ++i)
			{
				if (func(mCellMap.getRawData()[i]))
					return;
			}
		}
		template< class TFunc >
		void visitAllCells(TFunc&& func) const
		{
			for (int i = 0; i < mCellMap.getRawDataSize(); ++i)
			{
				func(mCellMap.getRawData()[i]);
			}
		}

		template< class TFunc >
		void visitAllSourcesBreak(TFunc&& func) const
		{
			for ( auto const& pos : mSourceLocations )
			{
				if (func( pos , getCellChecked(pos) ))
					return;
			}
		}
		template< class TFunc >
		void visitAllSources(TFunc&& func) const
		{
			for (auto const& pos : mSourceLocations)
			{
				func(pos, getCellChecked(pos));
			}
		}

		TGrid2D< Cell > mCellMap;
		std::vector< Vec2i > mSourceLocations;
	};


	class Solver
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
					if ( cell.blockMask & BIT(dir) )
						continue;

					Vec2i linkPos = getLinkPos(cell.pos, dir);
					Cell& linkCell = getCell(linkPos);
					if (linkCell.func == CellFunc::Source)
					{
						if (cell.funcMeta == linkCell.funcMeta )
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
			return solveInternal(mState , 0);
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

#define USE_EDGE_SOLVER 0

	class SATSolverBase
	{
	public:
		using SATVar = Minisat::Var;
		using SATLit = Minisat::Lit;

		static SATLit Lit(SATVar var) { return Minisat::mkLit(var); }

		static int const MaxColorCount = 32;

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

		uint8 getConnectMask(Vec2i const& pos)
		{
			return mConnectMaskMap(pos.x, pos.y);
		}


		TGrid2D< uint8   > mConnectMaskMap;
		Minisat::Solver    mSolver;
	};

	class SATSolverEdge : public SATSolverBase
	{
	public:
		void setup(Level& level)
		{

			Vec2i size = level.getSize();

			int colorCount = level.mSourceLocations.size() / 2;
			colorCount = 1;
			assert(colorCount <= MaxColorCount);

			mVarTable.resize(size.x, size.y);
			for (int i = 0; i < size.x; ++i)		
			{
				for (int j = 0; j < size.y; ++j)
				{
					VarInfo& info = mVarTable(i, j);
					for (int c = 0; c < colorCount; ++c)
					{
						info.edges[0].colors[c] = mSolver.newVar();
						info.edges[1].colors[c] = mSolver.newVar();
					}
				}
			}

			for (int i = 0; i < size.x; ++i)
			{
				for (int j = 0; j < size.y; ++j)
				{
					VarInfo& info = mVarTable(i, j);
					Cell const& cell = level.getCellChecked(Vec2i(i, j));
					for (int dir = 0; dir < 2; ++dir)
					{
						if (cell.blockMask & BIT(dir+2))
						{
							for (int c = 0; c < colorCount; ++c)
							{
								mSolver.addClause(~Lit(info.edges[dir].colors[c]));
							}
						}
					}
				}
			}


			for (int i = 0; i < size.x; ++i)
			{
				for (int j = 0; j < size.y; ++j)
				{
					Cell const& cell = level.getCellChecked(Vec2i(i, j));
					EdgeVar* edges[DirCount];
					edges[0] = &mVarTable((i + 1) % size.x , j).edges[0];
					edges[1] = &mVarTable(i, (j + 1) % size.y).edges[1];
					edges[2] = &mVarTable(i, j).edges[0];
					edges[3] = &mVarTable(i, j).edges[1];

					switch (cell.func)
					{
					case CellFunc::Source:
						{

							for (int c = 0; c < colorCount; ++c)
							{
								auto c0 = edges[0]->colors[c];
								auto c1 = edges[1]->colors[c];
								auto c2 = edges[2]->colors[c];
								auto c3 = edges[3]->colors[c];
								if (c == cell.funcMeta - 1)
								{
									ExactlyOne(mSolver, { c0, c1 , c2 , c3 });
								}
								else
								{
									mSolver.addClause(~Lit(c0));
									mSolver.addClause(~Lit(c1));
									mSolver.addClause(~Lit(c2));
									mSolver.addClause(~Lit(c3));
								}
							}
						}
						break;
					case CellFunc::Empty:
						{

#if 0
							for (int c = 0; c < colorCount; ++c)
							{
								for (int dir = 0; dir < DirCount; ++dir)
								{
									auto c0 = Lit(edges[dir]->colors[c]);
									auto c1 = Lit(edges[(dir + 1) % DirCount]->colors[c]);
									auto c2 = Lit(edges[(dir + 2) % DirCount]->colors[c]);
									auto c3 = Lit(edges[(dir + 3) % DirCount]->colors[c]);


									mSolver.addClause(~c0, c1, c2);
									mSolver.addClause(~c0, c1, c3);
									mSolver.addClause(~c0, c2, c3);
									Minisat::vec<Minisat::Lit> literals;
									literals.push(~c0);
									literals.push(c1);
									literals.push(c2);
									literals.push(c3);
									mSolver.addClause(literals);
								}
							}
#else
							int dir = 0;
							int c = 0;
							auto c0 = edges[dir]->colors[c];
							auto c1 = edges[(dir + 1) % DirCount]->colors[c];
							auto c2 = edges[(dir + 2) % DirCount]->colors[c];
							auto c3 = edges[(dir + 3) % DirCount]->colors[c];
							ExactlyTwo(mSolver, c0, c1, c2, c3);
#endif
						}
						break;
					}
				}
			}

			LogMsg("Var = %d : Clause = %d", mSolver.nVars(), mSolver.nClauses());

			bool bSolved = mSolver.solve();
			mConnectMaskMap.resize(size.x, size.y);
			if ( bSolved )
			{
				for (int i = 0; i < size.x; ++i)
				{
					for (int j = 0; j < size.y; ++j)
					{
						EdgeVar* edges[DirCount];
						edges[0] = &mVarTable((i + 1) % size.x, j).edges[0];
						edges[1] = &mVarTable(i, (j + 1) % size.y).edges[1];
						edges[2] = &mVarTable(i, j).edges[0];
						edges[3] = &mVarTable(i, j).edges[1];

						uint8 mask = 0;
						for (int dir = 0; dir < 4; ++dir)
						{
							for (int c = 0; c < colorCount; ++c)
							{
								if (mSolver.modelValue(edges[dir]->colors[c]).isTrue())
								{
									mask |= BIT(dir);
									break;
								}
							}
						}

						mConnectMaskMap(i, j) = mask;
					}
				}
			}
			else
			{
				mConnectMaskMap.fillValue(0);

				for (int i = 0; i < size.x; ++i)
				{
					for (int j = 0; j < size.y; ++j)
					{
						EdgeVar* edges[4];
						edges[0] = &mVarTable((i + 1) % size.x, j).edges[0];
						edges[1] = &mVarTable(i, (j + 1) % size.y).edges[1];
						edges[2] = &mVarTable(i, j).edges[0];
						edges[3] = &mVarTable(i, j).edges[1];

						uint8 mask = 0;
						for (int dir = 0; dir < DirCount; ++dir)
						{
							for (int c = 0; c < colorCount; ++c)
							{
								if (mSolver.value(edges[dir]->colors[c]).isTrue())
								{
									mask |= BIT(dir);
									break;
								}
							}
						}

						mConnectMaskMap(i, j) = mask;
					}
				}
			}
		}

		struct EdgeVar
		{
			SATVar colors[MaxColorCount];
		};
		struct VarInfo
		{
			EdgeVar edges[2];
		};
		TGrid2D< VarInfo > mVarTable;

	};

	class SATSolverSide : public SATSolverBase
	{
	public:
		enum ConnectType
		{
			CON_LR ,
			CON_TB ,
			CON_LT ,
			CON_LB ,
			CON_RT ,
			CON_RB ,

			ConnectTypeCount ,
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

		void setup(Level& level)
		{
			Vec2i size = level.getSize();
			mSize = size;
			mColorCount = level.mSourceLocations.size() / 2;
			assert(mColorCount <= MaxColorCount);

			mVarMap.resize(size.x, size.y);
			for (int j = 0; j < size.y; ++j)
			{
				for (int i = 0; i < size.x; ++i)
				{
					for (int c = 0; c < mColorCount; ++c)				
					{
						auto var = mSolver.newVar();
						mVarMap(i, j).colors[c] = var;
					}

					for (int t = 0; t < ConnectTypeCount; ++t)
					{
						auto var = mSolver.newVar();
						mVarMap(i, j).conTypes[t] = var;
					}
				}
			}
			
			for (int j = 0; j < size.y; ++j)
			{
				for (int i = 0; i < size.x; ++i)
				{
					Vec2i pos = Vec2i(i, j);

					Cell const& cell = level.getCellChecked(pos);

					//Every cell is assigned a single color
					//#TODO: check bridge cell
					{
						Minisat::vec< SATLit > literals;
						for (int c = 0; c < mColorCount; ++c)
						{
							literals.push(Lit(getColorVar(i, j, c)));
						}
						ExactlyOne(mSolver, literals);
					}

					switch (cell.func)
					{
					case CellFunc::Source:
						{
							//The color of every endpoint cell is known and specified
							int sourceColor = cell.funcMeta - 1;
							for (int c = 0; c < mColorCount; ++c)
							{
								if (c == sourceColor)
								{
									mSolver.addClause(Lit(getColorVar(i, j, c)));
								}
								else
								{
									mSolver.addClause(~Lit(getColorVar(i, j, c)));
								}
							}

							//Every endpoint cell has exactly one neighbor which matches its color
							Minisat::vec< SATLit > literals;
							for (int dir = 0; dir < DirCount; ++dir)
							{
								Vec2i linkPos = level.getLinkPos(pos, dir);
								if (cell.blockMask & BIT(dir))
									continue;

								literals.push(Lit(getColorVar(linkPos.x, linkPos.y, sourceColor)));
							}

							if (!literals.empty())
							{
								ExactlyOne(mSolver, literals);
							}

							for (int t = 0; t < ConnectTypeCount; ++t)
							{
								mSolver.addClause(~Lit(getConTypeVar(i, j, t)));
							}
						}
						break;
					case CellFunc::Empty:
						{
							//The flow through every non-endpoint cell matches exactly one of the six direction types
							{
								Minisat::vec< SATLit > literals;
								for (int t = 0; t < ConnectTypeCount; ++t)
								{
									uint8 conTypMask = GetEmptyConnectTypeMask(ConnectType(t));
									if (conTypMask & cell.blockMask)
									{
										mSolver.addClause(~Lit(getConTypeVar(i, j, t)));
									}
									else
									{
										literals.push(Lit(getConTypeVar(i, j, t)));
									}							
								}

								if (!literals.empty())
								{
									ExactlyOne(mSolver, literals);
								}
							}

							
							for (int t = 0; t < ConnectTypeCount; ++t)
							{
								int const* linkDirs = GetEmptyConnectTypeLinkDir(ConnectType(t));

								auto Tij = Lit(getConTypeVar(i, j, t));
			
								Vec2i ConPos0 = level.getLinkPos(pos, linkDirs[0]);
								Vec2i ConPos1 = level.getLinkPos(pos, linkDirs[1]);
								for (int c = 0; c < mColorCount; ++c)
								{
									auto Cij = Lit(getColorVar(i, j, c));
									//The neighbors of a cell specified by its direction type must match its color
									mSolver.addClause(~Tij, ~Cij, Lit(getColorVar(ConPos0.x, ConPos0.y, c)));
									mSolver.addClause(~Tij, Cij, ~Lit(getColorVar(ConPos0.x, ConPos0.y, c)));
									mSolver.addClause(~Tij, ~Cij, Lit(getColorVar(ConPos1.x, ConPos1.y, c)));
									mSolver.addClause(~Tij, Cij, ~Lit(getColorVar(ConPos1.x, ConPos1.y, c)));

									//The neighbors of a cell not specified by its direction type must not match its color
									for (int dir = 0; dir < DirCount; ++dir)
									{
										if (dir == linkDirs[0] || dir == linkDirs[1])
											continue;

										Vec2i linkPos = level.getLinkPos(pos, dir);
										mSolver.addClause(~Tij, ~Cij, ~Lit(getColorVar(linkPos.x, linkPos.y, c)));
									}
								}

							}

						}
						break;
					}
				}				
			}

			LogMsg("Var = %d : Clause = %d", mSolver.nVars(), mSolver.nClauses());
			bool bSolved = mSolver.solve();
			mConnectMaskMap.resize(size.x, size.y);
			mConnectMaskMap.fillValue(0);
			if (bSolved)
			{			
				for (int j = 0; j < size.y; ++j)
				{
					for (int i = 0; i < size.x; ++i)
					{
						for (int t = 0; t < ConnectTypeCount; ++t)
						{
							if (mSolver.modelValue(getConTypeVar(i, j, t)).isTrue())
							{
								mConnectMaskMap(i, j) = GetEmptyConnectTypeMask(ConnectType(t));
								break;
							}
						}
					}
				}
			}
		}

		struct VarInfo
		{
			std::array< SATVar , MaxColorCount > colors;
			std::array< SATVar , ConnectTypeCount > conTypes;
		};

		TGrid2D< VarInfo > mVarMap;

		SATVar getColorVar(int x, int y, int c)
		{
			return mVarMap(x, y).colors[c];
		}

		SATVar getConTypeVar(int x, int y, int t)
		{
			return mVarMap(x, y).conTypes[t];
		}
	};

	class SolveThread : public RunnableThreadT< SolveThread >
	{
	public:

		unsigned run() 
		{
			mSolver.solve();
			return 0; 
		}
		bool init() { return true; }
		void exit() {}

		Solver& mSolver;
	};

	class TestStage : public StageBase
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		virtual bool onInit() override
		{
			if( !BaseClass::onInit() )
				return false;
			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

		virtual void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart();
		void tick() {}
		void updateFrame(int frame) {}


		Vec2i ToScreenPos(Vec2i const& cellPos);
		Vec2i ToCellPos(Vec2i const& screenPos);

		virtual void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame) override;



		Vec2i     flowOutCellPos;
		ColorType flowOutColor;
		bool  bStartFlowOut = false;
		Solver mSolver;

#if USE_EDGE_SOLVER
		SATSolverEdge mSolver2;
#else
		SATSolverSide mSolver2;
#endif
		
		int numCellFilled = 0;


		bool onMouse(MouseMsg const& msg) override
		{
			if( !BaseClass::onMouse(msg) )
				return false;

			if( msg.onLeftDown() )
			{
				Vec2i cPos = ToCellPos(msg.getPos());

				if( mLevel.isValidCellPos(cPos) )
				{
					bStartFlowOut = true;
					flowOutColor   = mLevel.getCellChecked(cPos).getFunFlowColor();
					flowOutCellPos = cPos;
					if (mLevel.breakFlow(cPos, 0, 0) == Level::BreakResult::NoBreak)
					{



					}
				}
			}
			else if( msg.onLeftUp() )
			{
				bStartFlowOut = false;
			}


			if( bStartFlowOut && msg.onMoving() )
			{
				Vec2i cPos = ToCellPos(msg.getPos());

				if ( mLevel.isValidCellPos(cPos) )
				{
					Vec2i cOffset = cPos - flowOutCellPos;
					Vec2i cOffsetAbs = Abs(cOffset);
					int d = cOffsetAbs.x + cOffsetAbs.y;
					if (d > 0)
					{
						if (d == 1)
						{
							int dir;
							if (cOffset.x == 1)
							{
								dir = 0;
							}
							else if (cOffset.y == 1)
							{
								dir = 1;
							}
							else if (cOffset.x == -1)
							{
								dir = 2;
							}
							else
							{
								dir = 3;
							}

							if (mLevel.breakFlow(cPos, dir, flowOutColor) == Level::BreakResult::HaveBreakSameColor)
							{
								flowOutCellPos = cPos;
							}
							else
							{
								ColorType linkColor = mLevel.linkFlow(flowOutCellPos, dir);
								if (linkColor)
								{
									flowOutCellPos = cPos;
									flowOutColor = linkColor;
								}
							}
						}
						else
						{



						}
					}
				}
			}

			return true;
		}

		bool onKey(unsigned key, bool isDown) override
		{
			if( !isDown )
				return false;
			switch( key )
			{
			case Keyboard::eR: restart(); break;
			case Keyboard::eX: mSolver.solveNext(); break;
			case Keyboard::eC: for (int i = 0; i < 97; ++i) mSolver.solveNext(); break;
			}
			return false;
		}

		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}

		Level mLevel;
	protected:
	};
}