#include "FlowFreeSolver.h"

#include "BitUtility.h"
#include "ProfileSystem.h"


namespace FlowFree
{

#define BIT4( a , b , c , d) ((a)|((b) << 1)|((c) << 2)|((d) << 3))

	static EdgeMask const EmptyCellConnectMask[] =
	{
		BIT4(1,1,0,0),
		BIT4(1,0,1,0),
		BIT4(1,0,0,1),
		BIT4(0,1,1,0),
		BIT4(0,0,1,1),
		BIT4(0,1,0,1),
	};

	static EdgeMask const SourceCellConnectMask[] =
	{
		BIT4(1,0,0,0),
		BIT4(0,1,0,0),
		BIT4(0,0,1,0),
		BIT4(0,0,0,1),
	};

#define USE_SORTED_CELL 1

	void DepthFirstSolver::setup(Level& level)
	{
		Vec2i mapSize = level.getSize();
		mCellMap.resize(mapSize.x, mapSize.y);
		for (Vec2i const& pos : level.mSourceLocations)
		{
			mSourceIndices.push_back(mCellMap.toIndex(pos.x, pos.y));
		}

		struct SortCellInfo
		{
			int   indexCell;
			Cell* cell;
		};

		std::vector<SortCellInfo> sortedCells;
		for (int i = 0; i < mCellMap.getRawDataSize(); ++i)
		{
			Cell& cell = mCellMap[i];
			auto const& levelCell = level.mCellMap[i];
			cell.pos.x = i % mCellMap.getSizeX();
			cell.pos.y = i / mCellMap.getSizeY();
			cell.func = levelCell.func;
			cell.funcMeta = levelCell.funcMeta;
			cell.blockMask = levelCell.blockMask;

			SortCellInfo info;
			info.cell = &cell;
			info.indexCell = i;
			sortedCells.push_back(info);
		}

		scanLogicalCondition();

		initializeState(mState);


#if USE_SORTED_CELL
		std::sort(sortedCells.begin(), sortedCells.end(),
			[](SortCellInfo const& lhs, SortCellInfo const& rhs)
		{
			if (lhs.cell->func == CellFunc::Source && rhs.cell->func != CellFunc::Source)
				return true;
			if (rhs.cell->func == CellFunc::Source && lhs.cell->func != CellFunc::Source)
				return false;

			return FBitUtility::CountSet(lhs.cell->blockMask) > FBitUtility::CountSet(rhs.cell->blockMask);
		}
		);

		for (int i = 0; i < sortedCells.size(); ++i)
		{
			mSortedSolveSequeces.push_back(sortedCells[i].indexCell);
		}
#endif
	}

	bool DepthFirstSolver::checkAllSourcesConnectCompleted(SolveState& state)
	{
		++state.checkCount;
		if (state.checkCount == 0)
		{
			for (CellState& cellState : state.cells)
			{
				cellState.checkCount = 0;
			}
		}

		for (int sourceIndex : mSourceIndices)
		{
			CellState& cellState = state.cells[sourceIndex];

			if (cellState.checkCount == state.checkCount)
				continue;;

			cellState.checkCount = state.checkCount;

			if (!checkSourceConnectCompleted(state, sourceIndex))
			{
				return false;
			}
		}
		return true;
	}

	static int const gBitIndexMap[9] = { 0 , 0 , 1 , 1 , 2 , 2 , 2 , 2 , 3 };
	int ToIndex8(uint8 bit)
	{
		assert((bit & 0xff) == bit);
		assert((bit & (bit - 1)) == 0);
		int result = 0;
		if (bit & 0xf0) { result += 4; bit >>= 4; }
		return result + gBitIndexMap[bit];
	}

	bool DepthFirstSolver::checkSourceConnectCompleted(SolveState& state, int indexSource)
	{
		Cell& cell = mCellMap[indexSource];
		assert(cell.func == CellFunc::Source);
		uint8 const sourceColor = cell.funcMeta;

		int dir = ToIndex8(state.cells[indexSource].connectMask);
		Vec2i linkPos = cell.pos;

		for (;;)
		{
			linkPos = getLinkPos(linkPos, dir);
			int linkIndex = mCellMap.toIndex(linkPos.x, linkPos.y);
			Cell& linkCell = getCell(linkPos);
			CellState& linkCellState = state.cells[linkIndex];
			if (linkCell.func == CellFunc::Source)
			{
				linkCellState.checkCount = state.checkCount;
				return linkCell.funcMeta == sourceColor;
			}
			else
			{
				if (linkCell.func == CellFunc::Empty)
				{
					uint8 connectMask = linkCellState.connectMask & ~BIT(InverseDir(dir));
					if (connectMask == 0)
					{
						break;
					}

					dir = ToIndex8(connectMask);
				}
			}
		}

		return false;
	}

	bool DepthFirstSolver::solveInternal(SolveState& state, int indexSeq)
	{
		int indexSeqEnd = mCellMap.getRawDataSize();

		while (indexSeq >= 0)
		{
#if USE_SORTED_CELL
			int indexCell = mSortedSolveSequeces[indexSeq];
#else

			int indexCell = indexSeq;
#endif

			CellState& cellState = state.cells[indexCell];

			if (sovleSubStep(state, indexCell, cellState))
			{
				++indexSeq;
				if (indexSeq == indexSeqEnd)
				{
					if (checkAllSourcesConnectCompleted(state))
					{
						break;
					}
					else
					{
						--indexSeq;
					}
				}
			}
			else
			{
				cellState.indexUsed = -1;
				cellState.connectMask = 0;
				--indexSeq;
			}
		}

		if (indexSeq < 0)
			return false;

		return true;
	}

	bool DepthFirstSolver::sovleSubStep(SolveState& state, int indexCell, CellState& cellState)
	{

		Cell& cell = mCellMap[indexCell];
		Vec2i const& pos = cell.pos;

		EdgeMask blockMaskRequired;
		EdgeMask connectMaskRequired;
		if (cellState.indexUsed == -1)
		{
			blockMaskRequired = cell.blockMask;
			connectMaskRequired = 0;

			auto TryAddBlockMask = [this, &state, &blockMaskRequired, &connectMaskRequired, &pos](uint8 dir)
			{
				EdgeMask dirMask = BIT(dir);
				if (!(blockMaskRequired & dirMask))
				{
					Vec2i linkPos = getLinkPos(pos, dir);
					int linkIndex = mCellMap.toIndex(linkPos.x, linkPos.y);
					CellState const& linkState = state.cells[linkIndex];
#if USE_SORTED_CELL
					if (linkState.indexUsed != -1)
#endif
						if (linkState.connectMask & BIT(InverseDir(dir)))
						{
							connectMaskRequired |= dirMask;
						}
						else
						{
							blockMaskRequired |= dirMask;
						}
				}
			};

#if USE_SORTED_CELL
			for (int dir = 0; dir < DirCount; ++dir)
				TryAddBlockMask(dir);
#else
			if (pos.x != 0)
			{
				TryAddBlockMask(2);
			}
			if (pos.y != 0)
			{
				TryAddBlockMask(3);
			}
#endif

			cellState.blockMaskRequired = blockMaskRequired;
			cellState.connectMaskRequired = connectMaskRequired;
		}
		else
		{
			blockMaskRequired = cellState.blockMaskRequired;
			connectMaskRequired = cellState.connectMaskRequired;
		}

		auto FindNextUsedIndex = [&cellState, blockMaskRequired, connectMaskRequired](TArrayView< EdgeMask const > masks) -> bool
		{
			for (int index = cellState.indexUsed + 1; index < masks.size(); ++index)
			{
				EdgeMask connectMask = masks[index];
				if (connectMask & blockMaskRequired)
					continue;
				if (connectMaskRequired && (connectMaskRequired & connectMask) != connectMaskRequired)
					continue;

				cellState.indexUsed = index;
				cellState.connectMask = masks[index];
				return true;
			}

			return false;
		};

		switch (cell.func)
		{
		case CellFunc::Empty: return FindNextUsedIndex(TArrayView<EdgeMask const>(EmptyCellConnectMask, ARRAY_SIZE(EmptyCellConnectMask)));
		case CellFunc::Source: return FindNextUsedIndex(TArrayView<EdgeMask const>(SourceCellConnectMask, ARRAY_SIZE(SourceCellConnectMask)));
		}

		return false;
	}


	bool SATSolverEdge::solve(Level& level)
	{
		mSAT.reset();

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
					info.edges[0].colors[c] = mSAT->newVar();
					info.edges[1].colors[c] = mSAT->newVar();
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
					if (cell.blockMask & BIT(dir + 2))
					{
						for (int c = 0; c < colorCount; ++c)
						{
							mSAT->addClause(~Lit(info.edges[dir].colors[c]));
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
				edges[0] = &mVarTable((i + 1) % size.x, j).edges[0];
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
								ExactlyOne(mSAT, { c0, c1 , c2 , c3 });
							}
							else
							{
								mSAT->addClause(~Lit(c0));
								mSAT->addClause(~Lit(c1));
								mSAT->addClause(~Lit(c2));
								mSAT->addClause(~Lit(c3));
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
						ExactlyTwo(mSAT, c0, c1, c2, c3);
#endif
					}
					break;
				}
			}
		}

		LogMsg("Var = %d : Clause = %d", mSAT->nVars(), mSAT->nClauses());

		bool bSolved = mSAT->solve();
		mSolution.resize(size.x, size.y);
		if (bSolved)
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
							if (mSAT->modelValue(edges[dir]->colors[c]).isTrue())
							{
								mask |= BIT(dir);
								break;
							}
						}
					}

					mSolution(i, j).mask = mask;
				}
			}
		}
		else
		{
			SolvedCell c;
			c.color = 0;
			c.color2 = 0;
			c.mask = 0;
			mSolution.fillValue(c);

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
							if (mSAT->value(edges[dir]->colors[c]).isTrue())
							{
								mask |= BIT(dir);
								break;
							}
						}
					}

					mSolution(i, j).mask = mask;
				}
			}
		}

		return bSolved;
	}

	static void execbreak()
	{
		int i = 1;
	}

	bool SATSolverCell::solve(Level& level)
	{
		mSAT.reset();

		Vec2i size = level.getSize();
		mSize = size;
		mColorCount = level.mSourceLocations.size() / 2;
		assert(mColorCount <= MaxColorCount);


		auto GetLinkPosSkipBirdge = [&level](Vec2i const& pos, int dir) -> Vec2i
		{
			Vec2i linkPos = pos;
			for(;;)
			{
				linkPos = level.getLinkPos(linkPos, dir);
				Cell& cell = level.getCellChecked(linkPos);
				if (cell.func != CellFunc::Bridge)
				{
					break;
				}
			}
			return linkPos;
		};


		{
			TIME_SCOPE("SAT Setup");
#define IGNORE_CELLS 1

			auto DoseHaveVar = [](Cell const& cell)
			{
#if IGNORE_CELLS
				if (cell.func == CellFunc::Block ||
					cell.func == CellFunc::Bridge)
					return false;
#endif
				return true;
			};

			mVarMap.resize(size.x, size.y);
			for (int j = 0; j < size.y; ++j)
			{
				for (int i = 0; i < size.x; ++i)
				{
					Vec2i pos = Vec2i(i, j);

					Cell const& cell = level.getCellChecked(pos);

					if ( DoseHaveVar(cell) )
					{
						for (int c = 0; c < mColorCount; ++c)
						{
							auto var = mSAT->newVar();
							mVarMap(i, j).colors[c] = var;
						}

						for (int t = 0; t < ConnectTypeCount; ++t)
						{
							auto var = mSAT->newVar();
							mVarMap(i, j).conTypes[t] = var;
						}
					}
				}
			}

#define CHECK_OK_INTERNAL( FILE , LINE , FUNC )  if (! mSAT->okay() ) { execbreak(); LogMsg( "%s(%d) : %s Sat is not OK" , FILE , LINE , FUNC ); }
#define CHECK_OK CHECK_OK_INTERNAL(__FILE__ , __LINE__ , __FUNCTION__)

			for (int j = 0; j < size.y; ++j)
			{
				for (int i = 0; i < size.x; ++i)
				{
					Vec2i pos = Vec2i(i, j);

					Cell const& cell = level.getCellChecked(pos);

					//Every cell is assigned a single color
					if ( DoseHaveVar(cell) )
					{
						Minisat::vec< SATLit > literals;
						for (int c = 0; c < mColorCount; ++c)
						{
							literals.push(Lit(getColorVar(pos, c)));
						}
						ExactlyOne(mSAT, literals);
						CHECK_OK;
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
									mSAT->addClause(Lit(getColorVar(pos, c)));
								}
								else
								{
									mSAT->addClause(~Lit(getColorVar(pos, c)));
								}
							}

							//Every endpoint cell has exactly one neighbor which matches its color
							Minisat::vec< SATLit > literals;
							for (int dir = 0; dir < DirCount; ++dir)
							{
								if (cell.blockMask & BIT(dir))
								{
									continue;
								}

								Vec2i linkPos = GetLinkPosSkipBirdge(pos, dir);
								literals.push(Lit(getColorVar(linkPos, sourceColor)));
							}

							if (!literals.empty())
							{
								ExactlyOne(mSAT, literals);
							}

							for (int t = 0; t < ConnectTypeCount; ++t)
							{
								mSAT->addClause(~Lit(getConTypeVar(pos, t)));
							}
							CHECK_OK;
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
										mSAT->addClause(~Lit(getConTypeVar(pos, t)));
									}
									else
									{
										literals.push(Lit(getConTypeVar(pos, t)));
									}
								}

								if (!literals.empty())
								{
									ExactlyOne(mSAT, literals);
								}
								CHECK_OK;
							}


							for (int t = 0; t < ConnectTypeCount; ++t)
							{
								int const* linkDirs = GetEmptyConnectTypeLinkDir(ConnectType(t));
								uint8 conTypMask = GetEmptyConnectTypeMask(ConnectType(t));

								auto Tij = Lit(getConTypeVar(pos, t));

								Vec2i ConPos0 = GetLinkPosSkipBirdge(pos, linkDirs[0]);
								Vec2i ConPos1 = GetLinkPosSkipBirdge(pos, linkDirs[1]);
#if IGNORE_CELLS
								if (DoseHaveVar(level.getCellChecked(ConPos0)) && DoseHaveVar(level.getCellChecked(ConPos1)))
#endif
								{
									for (int c = 0; c < mColorCount; ++c)
									{
										auto Cij = Lit(getColorVar(pos, c));
										//The neighbors of a cell specified by its direction type must match its color
										mSAT->addClause(~Tij, ~Cij, Lit(getColorVar(ConPos0, c)));
										mSAT->addClause(~Tij, Cij, ~Lit(getColorVar(ConPos0, c)));
										mSAT->addClause(~Tij, ~Cij, Lit(getColorVar(ConPos1, c)));
										mSAT->addClause(~Tij, Cij, ~Lit(getColorVar(ConPos1, c)));
										CHECK_OK;
										//The neighbors of a cell not specified by its direction type must not match its color
										for (int dir = 0; dir < DirCount; ++dir)
										{
											if (dir == linkDirs[0] || dir == linkDirs[1])
												continue;

											if (BIT(dir) & cell.blockMask)
												continue;

											Vec2i linkPos = GetLinkPosSkipBirdge(pos, dir);
											mSAT->addClause(~Tij, ~Cij, ~Lit(getColorVar(linkPos, c)));
										}
										CHECK_OK;
									}
								}
#if IGNORE_CELLS
								else
								{
									mSAT->addClause(~Tij);
								}
#endif
							}
						}
						break;
					}
				}
			}
		}

		LogMsg("Var = %d : Clause = %d", mSAT->nVars(), mSAT->nClauses());
		mSAT->simplify();
		//LogMsg("Var = %d : Clause = %d", mSolver->nVars(), mSolver->nClauses());

		bool bSolved;
		{
			TIME_SCOPE("SAT Solve");
			bSolved = mSAT->solve();
		}

		mSolution.resize(size.x, size.y);

		{
			SolvedCell c;
			c.color = 0;
			c.mask = 0;
			mSolution.fillValue(c);
		}
		
		if (bSolved)
		{
			TIME_SCOPE("Read Solution");
			std::vector< Vec2i > sourceCellPosList;
			std::vector< Vec2i > bridgeCellPosList;
			for (int j = 0; j < size.y; ++j)
			{
				for (int i = 0; i < size.x; ++i)
				{
					Vec2i pos = Vec2i(i, j);
					Cell const& cell = level.getCellChecked(pos);

					auto& cellSolution = mSolution(i, j);
					switch (cell.func)
					{
					case CellFunc::Empty:
						{
							for (int t = 0; t < ConnectTypeCount; ++t)
							{
								if (mSAT->modelValue(getConTypeVar(pos, t)).isTrue())
								{
									cellSolution.mask = GetEmptyConnectTypeMask(ConnectType(t));
									break;
								}
							}

							for (int c = 0; c < mColorCount; ++c)
							{
								if (mSAT->modelValue(getColorVar(pos, c)).isTrue())
								{
									cellSolution.color = c + 1;
									break;
								}
							}
						}
						break;
					case CellFunc::Source:
						sourceCellPosList.push_back(pos);
						break;
					case CellFunc::Bridge:
						bridgeCellPosList.push_back(pos);
						break;
					case CellFunc::Block:
						{
							cellSolution.color = 0;
							cellSolution.color2 = 0;
							cellSolution.mask = 0;
						}
					default:
						break;
					}
				}
			}


			for( auto pos : sourceCellPosList )
			{
				Cell const& cell = level.getCellChecked(pos);
				
				assert(cell.func == CellFunc::Source);

				auto& cellSolution = mSolution(pos.x, pos.y);
				cellSolution.color = cell.funcMeta;

				for (int dir = 0; dir < DirCount; ++dir)
				{
					Vec2i linkPos = GetLinkPosSkipBirdge(pos, dir);
					auto const& linkCell = mSolution(linkPos.x, linkPos.y);

					if (linkCell.mask & BIT(InverseDir(dir)))
					{
						cellSolution.mask = BIT(dir);
						break;
					}
				}
			}
	
			for (auto pos : bridgeCellPosList)
			{
				Cell const& cell = level.getCellChecked(pos);
				assert(cell.func == CellFunc::Bridge);

				auto& cellSolution = mSolution(pos.x, pos.y);
				Vec2i linkPosX = GetLinkPosSkipBirdge(pos, EDir::Left);
				Vec2i linkPosY = GetLinkPosSkipBirdge(pos, EDir::Bottom);
				cellSolution.color = mSolution(linkPosX.x, linkPosX.y).color;
				cellSolution.color2 = mSolution(linkPosY.x, linkPosY.y).color;
			}
		}
		else
		{

		}

		return bSolved;
	}

}//namespace FlowFree