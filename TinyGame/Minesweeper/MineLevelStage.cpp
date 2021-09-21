#include "MineLevelStage.h"

namespace Mine
{

	REGISTER_STAGE_ENTRY("Mine Sweeper", TestStage, EExecGroup::Test, "Game|AI");

	int Level::openCell(int cx, int cy)
	{
		if (mbBuildBomb == false)
		{
			mbBuildBomb = true;
			int coords[2] = { cx , cy };
			buildBomb(mNumBombs, coords);
		}
		return openCellInternal(cx, cy);
	}

	int Level::openCellInternal(int cx, int cy)
	{
		assert(mCells.checkRange(cx, cy));
		CellData& cell = mCells(cx, cy);
		if (cell.isMarked())
			return CV_FLAG;

		if (!cell.isProbed())
		{
			if (cell.number == 0)
			{
				expendCell(cx, cy);
			}
			else
			{
				makeCellProbed(cell);
			}
		}

		return cell.number;
	}
	void Level::expendCell(int cx, int cy)
	{
		CellData& cell = mCells(cx, cy);
		
		if (cell.isProbed())
			return;

		makeCellProbed(cell);

		if (cell.number == 0)
		{
			for (int j = -1; j <= 1; ++j)
			{
				for (int i = -1; i <= 1; ++i)
				{
					if (i == 0 && j == 0)
						continue;

					if (mCells.checkRange(cx + i, cy + j))
					{
						expendCell(cx + i, cy + j);
					}
				}
			}
		}
	}

	bool Level::markCell(int cx, int cy)
	{
		assert(mCells.checkRange(cx, cy));
		CellData& cell = mCells(cx, cy);

		if (cell.isMarked())
			return true;

		cell.bMarked = true;
		++mMarkCount;
		return true;
	}

	bool Level::unmarkCell(int cx, int cy)
	{
		assert(mCells.checkRange(cx, cy));
		CellData& cell = mCells(cx, cy);
		
		if (!cell.isMarked())
			return true;

		cell.bMarked = false;
		--mMarkCount;
		return true;
	}

	bool Level::openNeighberCell(int cx, int cy, bool& bHaveBomb)
	{
		assert(mCells.checkRange(cx, cy));
		CellData& cell = mCells(cx, cy);
		if (!cell.isProbed())
			return false;

		assert(cell.number > 0);

		int numMarks = 0;
		for (int j = -1; j <= 1; ++j)
		{
			for (int i = -1; i <= 1; ++i)
			{
				if (i == 0 && j == 0)
					continue;

				if (mCells.checkRange(cx + i, cy + j))
				{
					CellData& nCell = mCells(cx + i, cy + j);
					if (nCell.isProbed() == false && nCell.isMarked() == true)
					{
						++numMarks;
					}
				}
			}
		}
		if (numMarks != cell.number)
			return false;

		bHaveBomb = false;
		for (int j = -1; j <= 1; ++j)
		{
			for (int i = -1; i <= 1; ++i)
			{
				if (i == 0 && j == 0)
					continue;

				if (mCells.checkRange(cx + i, cy + j))
				{
					int state = openCell(cx + i, cy + j);
					if (state == CV_BOMB)
					{
						bHaveBomb = true;
					}
				}
			}
		}

		return true;
	}

	int Level::lookCell(int cx, int cy, bool bCracked /*= false*/)
	{
		assert(mCells.checkRange(cx, cy));
		CellData& cell = mCells(cx, cy);
		if (cell.isProbed() || bCracked)
			return cell.number;

		if (cell.isMarked())
			return CV_FLAG;

		return CV_UNPROBLED;
	}

	void Level::buildBomb(int numBombs, int* ignoreCoords)
	{
		assert(numBombs < mCells.getRawDataSize());

		for (int i = 0; i < numBombs; ++i)
		{
			mCells[i].number = CV_BOMB;
		}
		for (int i = numBombs; i < mCells.getRawDataSize(); ++i)
		{
			mCells[i].number = 0;
		}

		for (int i = 0; i < mCells.getRawDataSize(); ++i)
		{
			for (int j = 0; j < mCells.getRawDataSize(); ++j)
			{
				std::swap(mCells[i].number, mCells[rand() % mCells.getRawDataSize()].number);
			}
		}

		if (ignoreCoords)
		{
			for (int j = -1; j <= 1; ++j)
			{
				for (int i = -1; i <= 1; ++i)
				{
					int x = ignoreCoords[0] + i;
					int y = ignoreCoords[1] + j;

					if (!mCells.checkRange(x, y))
						continue;

					if (mCells(x, y).number != 0)
					{
						for (;;)
						{
							int nx = rand() % mCells.getSizeX();
							int ny = rand() % mCells.getSizeY();

							if ( Math::Abs(nx - ignoreCoords[0]) <= 1 && Math::Abs(ny - ignoreCoords[1]) <= 1)
								continue;

							CellData& cellSwap = mCells(nx, ny);

							if (cellSwap.number != 0)
								continue;

							std::swap(mCells(x, y).number, cellSwap.number);
							break;
						}
					}
				}
			}
		}

		for (int i = 0; i < mCells.getRawDataSize(); ++i)
		{
			if (mCells[i].number != 0)
				continue;

			int cx = i % mCells.getSizeX();
			int cy = i / mCells.getSizeX();
			int x1 = std::max(0, cx - 1);
			int x2 = std::min(mCells.getSizeX() - 1, cx + 1);
			int y1 = std::max(0, cy - 1);
			int y2 = std::min(mCells.getSizeY() - 1, cy + 1);

			int num = 0;
			for (int x = x1; x <= x2; ++x)
			{
				for (int y = y1; y <= y2; ++y)
				{
					int idx = mCells.toIndex(x, y);
					if (mCells[idx].number == CV_BOMB)
						++num;
				}
			}
			mCells[i].number = num;
		}
	}

	void Level::restoreCell(int sx, int sy, int numBombs)
	{
		mNumBombs = numBombs;
		if (mCells.getSizeX() != sx || mCells.getSizeY() != sy)
		{
			mCells.resize(sx, sy);
		}

		for (int i = 0; i < mCells.getRawDataSize(); ++i)
		{
			CellData& cell = mCells[i];
			cell.bProbed = false;
			cell.bMarked = false;
			cell.number = 0;
		}

		mMarkCount = 0;
		mbBuildBomb = false;
	}

	void TestStage::draw(Graphics2D& g, IMineMap& mineMap, Vec2i const& drawOrigin)
	{
		RenderUtility::SetPen(g, EColor::Gray);
		RenderUtility::SetBrush(g, EColor::Gray);
		g.drawRect(Vec2i(0, 0), ::Global::GetScreenSize());

		Vec2i textOrg = drawOrigin + Vec2i(5, 3);

		InlineString<512> str;

		Color3ub NumberColorMap[] =
		{
			Color3ub(25, 118, 210),
			Color3ub(56, 142, 60),
			Color3ub(211, 47, 47),
			Color3ub(123, 31, 162),
			Color3ub(255, 143, 0),
			Color3ub(3, 150, 170),
			Color3ub(66, 66, 66),
			Color3ub(117, 128, 139),
		};

		int sizeX = mineMap.getSizeX();
		int sizeY = mineMap.getSizeY();

		RenderUtility::SetFont(g, FONT_S12);

		bool bHoveredPosValid = mineMap.isValidPos(mHoveredCellPos.x, mHoveredCellPos.y);
		for (int j = 0; j < sizeY; ++j)
		{
			for (int i = 0; i < sizeX; ++i)
			{
				Vec2i offset = Vec2i(i * LengthCell, j *LengthCell);
				Vec2i pt = drawOrigin + offset;

				int state = mineMap.look(i, j, false);

				bool bHightlight = false;
				if (bHoveredPosValid && bGameOver == false)
				{
					if (mHoveredCellPos.x == i && mHoveredCellPos.y == j)
					{
						bHightlight = true;
					}
					else if (bOpenNeighborOp && state == CV_UNPROBLED )
					{
						bHightlight = Math::Abs(i - mHoveredCellPos.x) <= 1 && Math::Abs(j - mHoveredCellPos.y) <= 1;
					}
				}

				switch (state)
				{
				case CV_FLAG:
				case CV_UNPROBLED:
					{
						Color3ub c;
						if (bHightlight)
						{
							c = (i + j) % 2 ? Color3ub(191, 225, 125) : Color3ub(185, 221, 119);
						}
						else
						{
							c = (i + j) % 2 ? Color3ub(162, 209, 73) : Color3ub(170, 215, 81);
						}
						g.setPen(c);
						g.setBrush(c);
						g.drawRect(pt, Vec2i(LengthCell, LengthCell));

						if (state == CV_FLAG)
						{
							RenderUtility::SetBrush(g, EColor::Red);
							RenderUtility::SetPen(g, EColor::Red);
							//g.drawRect(pt + Vec2i(4, 4), Vec2i(LengthCell - 8, LengthCell - 8));

							// Tx Ty
							//   |\
							//	 |_\ Bx By
							//
							int L = 60 * LengthCell / 100;
							int Tx = 20 * LengthCell / 100;
							int Ty = 10 * LengthCell / 100;
							int Bx = Tx + L;
							int By = Ty + L;


							Vec2i TriPos[] =
							{
								pt + Vec2i(Tx,By),
								pt + Vec2i(Bx,By),
								pt + Vec2i(Tx,Ty),
							};
							g.drawPolygon(TriPos, 3);
							g.drawRect(pt + Vec2i(Tx, Ty), Vec2i(2 , 80 * LengthCell / 100));
						}
					}
					break;
				case CV_BOMB:
					{
						RenderUtility::SetBrush(g, EColor::Red);
						RenderUtility::SetPen(g, EColor::Red);
						g.drawCircle(pt + Vec2i(LengthCell, LengthCell) / 2, 3 * LengthCell / 8);
					}
					break;
				default:
					{
						Color3ub c;
						if (bHightlight && state)
						{
							c = (i + j) % 2 ? Color3ub(236, 209, 183) : Color3ub(236, 209, 183);
						}
						else
						{
							c = (i + j) % 2 ? Color3ub(229, 194, 159) : Color3ub(215, 184, 153);
						}
						g.setPen(c);
						g.setBrush(c);
						g.drawRect(pt, Vec2i(LengthCell, LengthCell));
						if (state)
						{
							g.setTextColor(NumberColorMap[state - 1]);
							g.drawText(pt, Vec2i(LengthCell, LengthCell), FStringConv::From(state));
						}

						int border = 2;
						Color3ub borderColor{ 135, 175, 58 };

						Vec2i dirOffsets[] =
						{
							Vec2i(1,0),Vec2i(-1,0),Vec2i(0,1),Vec2i(0,-1),
						};
						for (int dir = 0; dir < 4; ++dir)
						{
							int nx = i + dirOffsets[dir].x;
							int ny = j + dirOffsets[dir].y;
							if (!mineMap.isValidPos(nx, ny))
								continue;

							int stateN = mineMap.look(nx, ny, false);
							if (stateN == CV_FLAG || stateN == CV_UNPROBLED || stateN == CV_BOMB)
							{
								g.setPen(borderColor);
								g.setBrush(borderColor);


								switch (dir)
								{
								case 0: g.drawRect(pt + Vec2i(LengthCell - border, 0), Vec2i(border, LengthCell)); break;
								case 1: g.drawRect(pt, Vec2i(border, LengthCell)); break;
								case 2: g.drawRect(pt + Vec2i(0, LengthCell - border), Vec2i(LengthCell, border)); break;
								case 3: g.drawRect(pt, Vec2i(LengthCell, border)); break;
								}
							}
						}

						Vec2i dirOffsets2[] =
						{
							Vec2i(1,1),Vec2i(-1,1),Vec2i(-1,-1),Vec2i(1,-1),
						};
						for (int dir = 0; dir < 4; ++dir)
						{
							int nx = i + dirOffsets2[dir].x;
							int ny = j + dirOffsets2[dir].y;
							if ( !mineMap.isValidPos(nx ,ny) )
								continue;

							int stateN = mineMap.look(nx, ny, false);
							if (stateN == CV_FLAG || stateN == CV_UNPROBLED || stateN == CV_BOMB)
							{
								g.setPen(borderColor);
								g.setBrush(borderColor);

								switch (dir)
								{
								case 0: g.drawRect(pt + Vec2i(LengthCell - border, LengthCell - border), Vec2i(border, border)); break;
								case 1: g.drawRect(pt + Vec2i(0, LengthCell - border), Vec2i(border, border)); break;
								case 2: g.drawRect(pt + Vec2i(0, 0), Vec2i(border, border)); break;
								case 3: g.drawRect(pt + Vec2i(LengthCell - border, 0), Vec2i(border, border)); break;
								}
							}
						}
					}
				}
			}
		}


		RenderUtility::SetPen(g, EColor::Black);
		int TotalSize;
		TotalSize = sizeY * LengthCell;
		for (int i = 0; i <= sizeX; i += sizeX)
		{
			Vec2i p1 = drawOrigin + Vec2i(i * LengthCell, 0);
			g.drawLine(p1, p1 + Vec2i(0, TotalSize));
		}
		TotalSize = sizeX * LengthCell;
		for (int i = 0; i <= sizeY; i += sizeY)
		{
			Vec2i p1 = drawOrigin + Vec2i(0, i * LengthCell);
			g.drawLine(p1, p1 + Vec2i(TotalSize, 0));
		}

#if 0
		ExpSolveStrategy::ProbInfo const& otherProb = mStragtegy.getOtherProbInfo();
		int cx, cy;
		if (mStragtegy.getSolveState() == ExpSolveStrategy::eProbSolve)
		{
			mStragtegy.getPorbSelect(cx, cy);
			sprintf_s(str, "Other Prob = %d , Select = ( %d , %d )", int(100 * otherProb.prob), cx, cy);
			dc.TextOut(0, 20, str);
		}


		mStragtegy.getLastCheckPos(cx, cy);

		sprintf_s(str, "( %d , %d ) index = %d",
			(int)mCellPos.x, (int)mCellPos.y,
			(int)(mCellPos.x + mStragtegy.getCellSizeX() * mCellPos.y));
		dc.TextOut(0, 0, str);
		sprintf_s(str, "Bomb = %d , Open Cell = %d , LastCheck Cell = ( %d , %d )",
			mStragtegy.getSolvedBombNum(), mStragtegy.getOpenCellNum(), cx, cy);
		dc.TextOut(150, 0, str);
#endif
	}

}