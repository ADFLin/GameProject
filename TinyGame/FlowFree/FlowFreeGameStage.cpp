#include "FlowFreeGameStage.h"
#include "BitUtility.h"

REGISTER_STAGE("Flow Free Game", FlowFree::TestStage , EStageGroup::Test);


namespace FlowFree
{
	struct LevelData
	{
		int sizeX;
		int sizeY;
		int type;
		char const* mapData;
	};

	namespace EBoundType
	{
		enum 
		{
			Close,
			Open,
		};
	}
	LevelData Level0 = { 15 , 15 , EBoundType::Close ,
		"a - - - - - b - - - - - - - -"
		"- - - - - - - - - - - - d c -"
		"- - - - - - - - - - - - c - d"
		"- - j - - - k - - - - - - - e"
		"- - - - - - - - - - - - a - -"
		"- - - - - - - - - - - l - - -"
		"- f - - - - m - - - - - - - -"
		"- - - - g - - n p - - - - - -"
		"- - - - - h - - - - - - - - -"
		"- - g - - - i - - - k - - - -"
		"- - - - - - j o - - p - - b -"
		"- - - i - h - - - - - - - - -"
		"- - - - - - - - - o - - - - -"
		"- - - - - - - n - - - f - - -"
		"- - - - - - - - m - - - - l e"
	};

	LevelData Level1 = { 8 , 8 , EBoundType::Close ,
		"a - - a b - - -"
		"- - f g c - c -"
		"- - d - - - - -"
		"- - e - i h - -"
		"- - - - g - - -"
		"- - - f - - - -"
		"- d - e - - i b"
		"- - - - - - - h"
	};

	LevelData Level2 = { 8 , 8 , EBoundType::Close ,
		"- - - - - - d c"
		"- - - a f - - -"
		"- a b - - - e d"
		"- - - - - - - -"
		"- - - - - - - -"
		"- - - c - e f -"
		"- b - - - - - -"
		"- - - - - - - -"
	};

	LevelData Level3 = { 10 , 10 , EBoundType::Close ,
		"- - - - - - - - - -"
		"- - - - - - - - - -"
		"- - - - - - - e - -"
		"- d e - - - - - d -"
		"- i - - - h f - c -"
		"- - - - - g - - - -"
		"- - a f g - - - - -"
		"- - - - - - - - - b"
		"- h - i - - - - - c"
		"- b - - - - - - - a"
	};

	LevelData Level4 = { 9 , 9 , EBoundType::Close ,
		"- - - - - - - - -"
		"- a - - - - - - -"
		"- - - - - d e - -"
		"c b f - - e - - -"
		"h g - - - - - - -"
		"- - - - - - - d -"
		"- - - - - - - c -"
		"- - - g - - - - -"
		"- - - h f - - b a"
	};


	LevelData Level5 = { 4 , 3 , EBoundType::Close ,
		"a - a d"
		"b - b -"
		"c - c d"
	};


	void ReadLevel(Level& level, char const* mapData )
	{
		int index = 0;
		while (*mapData != 0)
		{
			char c = *mapData;
			if ( c == ' ' )
			{

			}
			else if (c == '-')
			{
				++index;
			}
			else
			{
				int color = c - 'a' + 1;
				level.addSource(Vec2i(index % level.getSize().x, index / level.getSize().x), color);
				++index;
			}

			++mapData;
		}
	}

	void LoadLevel(Level& level, LevelData& data)
	{
		level.setSize(Vec2i(data.sizeX, data.sizeY));
		if (data.type == EBoundType::Close)
		{
			level.addMapBoundBlock();
		}
		ReadLevel(level, data.mapData);
	}


	int const CellLength = 30;
	Vec2i ScreenOffset = Vec2i(20, 20);
	int const FlowWidth = 16;
	int const FlowGap = (CellLength - FlowWidth) / 2;
	int const FlowSourceRadius = 12;

	void TestStage::restart()
	{
		bStartFlowOut = false;

#if 0
		mLevel.setSize(Vec2i(5, 5));
		mLevel.addMapBoundBlock();

		mLevel.addSource(Vec2i(0, 0), Vec2i(1, 4), EColor::Red);
		mLevel.addSource(Vec2i(2, 0), Vec2i(1, 3), EColor::Green);
		mLevel.addSource(Vec2i(2, 1), Vec2i(2, 4), EColor::Blue);
		mLevel.addSource(Vec2i(3, 3), Vec2i(4, 0), EColor::Yellow);
		mLevel.addSource(Vec2i(3, 4), Vec2i(4, 1), EColor::Orange);
#else
		LoadLevel(mLevel, Level2);
#endif

#if 0
		mSolver.setup(mLevel);
		mSolver.solve();
#else
		mSolver2.setup(mLevel);
#endif
	}

	Vec2i TestStage::ToScreenPos(Vec2i const& cellPos)
	{
		return ScreenOffset + CellLength * cellPos;
	}

	Vec2i TestStage::ToCellPos(Vec2i const& screenPos)
	{
		return (screenPos - ScreenOffset) / CellLength;
	}

	void TestStage::onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();

		Vec2i size = mLevel.getSize();

		RenderUtility::SetPen(g, EColor::White);
		
		for( int i = 0; i <= size.x; ++i )
		{
			Vec2i start = ScreenOffset + Vec2i(i * CellLength, 0);
			g.drawLine(start, start + Vec2i(0, size.y * CellLength));
		}

		for( int j = 0; j <= size.y; ++j )
		{
			Vec2i start = ScreenOffset + Vec2i(0, j * CellLength);
			g.drawLine(start, start + Vec2i(size.x * CellLength, 0));
		}

		for( int i = 0; i < size.x; ++i )
		{
			for( int j = 0; j < size.y; ++j )
			{
				Vec2i cellPos = Vec2i(i, j);
				int cellIndex = mLevel.mCellMap.toIndex(i, j);
				Cell const& cell = mLevel.getCellChecked(Vec2i(i, j));

#if 0
				Solver::Cell const& cellData = mSolver.getCell(cellPos);
#endif


				Vec2i posCellLT = ScreenOffset + CellLength * Vec2i(i, j);
				switch( cell.func )
				{
				case CellFunc::Source:
					RenderUtility::SetPen(g, cell.funcMeta);
					RenderUtility::SetBrush(g, cell.funcMeta);
					g.drawCircle( posCellLT + Vec2i(CellLength, CellLength) / 2, FlowSourceRadius );
					break;
				case CellFunc::Bridge:
				default:
					break;
				}

				auto DrawConnection = [&]( int dir )
				{
					switch (dir)
					{
					case 0: g.drawRect(posCellLT + Vec2i(FlowGap, FlowGap), Vec2i(CellLength - FlowGap, FlowWidth)); break;
					case 1: g.drawRect(posCellLT + Vec2i(FlowGap, FlowGap), Vec2i(FlowWidth, CellLength - FlowGap)); break;
					case 2: g.drawRect(posCellLT + Vec2i(0, FlowGap), Vec2i(CellLength - FlowGap, FlowWidth)); break;
					case 3: g.drawRect(posCellLT + Vec2i(FlowGap, 0), Vec2i(FlowWidth, CellLength - FlowGap)); break;
					}
				};

				for( int dir = 0; dir < DirCount; ++dir )
				{
					if ( !cell.colors[dir] )
						continue;

					RenderUtility::SetPen(g, cell.colors[dir]);
					RenderUtility::SetBrush(g, cell.colors[dir]);

					DrawConnection(dir);
				}

				if (cell.blockMask)
				{
					for (int dir = 0; dir < DirCount; ++dir)
					{
						RenderUtility::SetPen(g, EColor::Red);

						if ( cell.blockMask & BIT(dir) )
						{
							switch (dir)
							{
							case 0: g.drawLine(posCellLT + Vec2i(CellLength, 0), posCellLT + Vec2i(CellLength, CellLength)); break;
							case 1: g.drawLine(posCellLT + Vec2i(0, CellLength), posCellLT + Vec2i(CellLength, CellLength)); break;
							case 2: g.drawLine(posCellLT, posCellLT + Vec2i(0, CellLength)); break;
							case 3: g.drawLine(posCellLT, posCellLT + Vec2i(CellLength, 0)); break;
							}
						}
					}
				}

				bool bDrawSolveDebug = true;
				if (bDrawSolveDebug)
				{
#if 0
					Solver::CellState& cellState = mSolver.mState.cells[cellIndex];
					uint8 connectMask = cellState.connectMask;
#else
					uint8 connectMask = mSolver2.getConnectMask(cellPos);
#endif
					RenderUtility::SetPen(g, EColor::Gray);
					RenderUtility::SetBrush(g, EColor::Null);
					for (int dir = 0; dir < DirCount; ++dir)
					{
						if (connectMask & BIT(dir))
						{
							DrawConnection(dir);
						}
					}
				}
			}
		}
	}

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

	void Solver::setup(Level& level)
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

				return BitUtility::CountSet(lhs.cell->blockMask) > BitUtility::CountSet(rhs.cell->blockMask);
			}
		);

		for( int i = 0 ; i < sortedCells.size() ; ++i )
		{
			mSortedSolveSequeces.push_back(sortedCells[i].indexCell);
		}
#endif
	}

	bool Solver::checkAllSourcesConnectCompleted(SolveState& state)
	{
		++state.checkCount;
		if (state.checkCount == 0)
		{
			for (CellState& cellState: state.cells)
			{
				cellState.checkCount = 0;
			}
		}

		for (int sourceIndex : mSourceIndices)
		{
			CellState& cellState = state.cells[sourceIndex];

			if (cellState.checkCount == state.checkCount )
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

	bool Solver::checkSourceConnectCompleted(SolveState& state, int indexSource)
	{
		Cell& cell = mCellMap[indexSource];
		assert(cell.func == CellFunc::Source);
		uint8 const sourceColor = cell.funcMeta;

		int dir = ToIndex8(state.cells[indexSource].connectMask);
		Vec2i linkPos = cell.pos;

		for(;;)
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

	bool Solver::solveInternal(SolveState& state, int indexSeq)
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

	bool Solver::sovleSubStep(SolveState& state , int indexCell , CellState& cellState)
	{

		Cell& cell = mCellMap[indexCell];
		Vec2i const& pos = cell.pos;

		EdgeMask blockMaskRequired;
		EdgeMask connectMaskRequired;
		if (cellState.indexUsed == -1)
		{
			blockMaskRequired = cell.blockMask;
			connectMaskRequired = 0;

			auto TryAddBlockMask = [this, &state , &blockMaskRequired, &connectMaskRequired, &pos](uint8 dir)
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
			for( int dir = 0 ; dir < DirCount ; ++dir )
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

}//namespace Flow
