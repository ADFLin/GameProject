#include "FlowFreeGameStage.h"

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
		mSolver2.solve(mLevel);
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

	bool TestStage::onMouse(MouseMsg const& msg)
	{
		if (!BaseClass::onMouse(msg))
			return false;

		if (msg.onLeftDown())
		{
			Vec2i cPos = ToCellPos(msg.getPos());

			if (mLevel.isValidCellPos(cPos))
			{
				bStartFlowOut = true;
				flowOutColor = mLevel.getCellChecked(cPos).getFunFlowColor();
				flowOutCellPos = cPos;
				if (mLevel.breakFlow(cPos, 0, 0) == Level::BreakResult::NoBreak)
				{



				}
			}
		}
		else if (msg.onLeftUp())
		{
			bStartFlowOut = false;
		}


		if (bStartFlowOut && msg.onMoving())
		{
			Vec2i cPos = ToCellPos(msg.getPos());

			if (mLevel.isValidCellPos(cPos))
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

}//namespace Flow
