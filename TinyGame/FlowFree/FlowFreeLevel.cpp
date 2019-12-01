#include "FlowFreeLevel.h"

namespace FlowFree
{



	Cell::Cell()
	{
		::memset(this, 0, sizeof(*this));
	}

	int Cell::getNeedBreakDirs(int dir, int outDirs[DirCount])
	{
		int result = 0;
		switch (func)
		{
		case CellFunc::Empty:
			for (int i = 0; i < DirCount; ++i)
			{
				if (colors[i])
				{
					outDirs[result] = i;
					++result;
				}
			}
			break;
		case CellFunc::Source:
			{
				for (int i = 0; i < DirCount; ++i)
				{
					if (colors[i])
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

	bool Cell::isFilled() const
	{
		if (bFilledCachedDirty)
		{
			bFilledCachedDirty = false;
			int count = 0;
			for (int i = 0; i < DirCount; ++i)
			{
				if (colors[i])
				{
					++count;
				}
			}

			switch (func)
			{
			case CellFunc::Empty:
				bFilledCached = count == 2;
			case CellFunc::Source:
				bFilledCached = count == 1;
			case CellFunc::Bridge:
				bFilledCached = count == DirCount;
			default:
				bFilledCached = true;
			}
		}

		return bFilledCached;
	}

	int Cell::getLinkedFlowDir(int dir) const
	{
		ColorType color = colors[dir];
		switch (func)
		{
		case CellFunc::Empty:
			{
				int count = 0;
				assert(color);
				for (int i = 0; i < DirCount; ++i)
				{
					if (i == dir || colors[i] == 0)
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
				if (colors[invDir])
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

	FlowFree::ColorType Cell::getFunFlowColor(int dir /*= -1*/, ColorType inColor /*= 0*/) const
	{
		switch (func)
		{
		case CellFunc::Empty:
			{
				int count = 0;
				ColorType outColor = inColor;
				for (ColorType color : colors)
				{
					if (color == 0)
						continue;

					if (inColor && color != inColor)
						return 0;

					++count;
					if (count >= 2)
						return 0;

					outColor = color;
				}

				return outColor;
			}
			break;
		case CellFunc::Source:
			{
				for (ColorType color : colors)
				{
					if (color)
					{
						assert(color == funcMeta);
						return 0;
					}
				}

				if (inColor && inColor != funcMeta)
					return 0;

				return funcMeta;
			}
			break;
		case CellFunc::Bridge:
			{
				if (dir == -1)
				{
					for (ColorType color : colors)
					{
						if (color)
							return color;
					}
				}
				else
				{
					int outColor = colors[InverseDir(dir)];
					if (inColor && outColor != inColor)
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

	void Level::setSize(Vec2i const& size)
	{
		mCellMap.resize(size.x, size.y);
	}

	void Level::addMapBoundBlock()
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
			Cell& leftCell = mCellMap.getData(0, i);
			leftCell.blockMask |= BIT(2);
			Cell& rightCell = mCellMap.getData(mCellMap.getSizeX() - 1, i);
			rightCell.blockMask |= BIT(0);
		}
	}

	FlowFree::Level::BreakResult Level::breakFlow(Vec2i const& pos, int dir, ColorType curColor)
	{
		Cell& cell = getCellChecked(pos);
		BreakResult result = BreakResult::NoBreak;

		int linkDirs[DirCount];
		int numDir = cell.getNeedBreakDirs(dir, linkDirs);

		bool bBreakSameColor = false;
		bool bHaveBreak = false;


		for (int i = 0; i < numDir; ++i)
		{
			int linkDir = linkDirs[i];
			if (linkDir == -1)
				continue;

			int dist = 0;

			ColorType linkColor = cell.colors[linkDir];
			Cell* source = findSourceCell(pos, linkDir, dist);

			auto TryRemoveToSource = [&]() -> bool
			{
				for (int j = i + 1; j < numDir; ++j)
				{
					int otherLinkDir = linkDirs[j];
					if (otherLinkDir == -1)
						continue;
					if (cell.colors[linkDirs[j]] == linkColor)
					{
						int distOther = 0;
						Cell* otherSource = findSourceCell(pos, otherLinkDir, distOther);

						if (otherSource)
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

			if (source)
			{
				if (curColor == 0)
				{
					if (dist == 0)
					{
						removeFlowToEnd(pos, linkDirs[i]);
					}
					else
					{
						TryRemoveToSource();
					}
				}
				else if (linkColor != curColor)
				{
					if (TryRemoveToSource() == false)
					{
						removeFlowLink(pos, linkDir);
					}
				}
			}
			else
			{
				bHaveBreak = true;
				removeFlowToEnd(pos, linkDir);
				if (linkColor == curColor)
					bBreakSameColor = true;
			}
		}

		if (bHaveBreak)
		{
			if (bBreakSameColor)
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

	FlowFree::Cell* Level::findSourceCell(Vec2i const& pos, int dir, int& outDist)
	{
		outDist = 0;

		Vec2i curPos = pos;
		int   curDir = dir;

		Cell& cell = getCellChecked(pos);
		if (cell.func == CellFunc::Source)
			return &cell;

		for (;;)
		{
			Vec2i linkPos = getLinkPos(curPos, curDir);
			Cell& linkCell = getCellChecked(linkPos);
			if (linkCell.func == CellFunc::Source)
				return &linkCell;

			int curDirInv = InverseDir(curDir);
			curDir = linkCell.getLinkedFlowDir(curDirInv);
			if (curDir == -1)
				break;

			curPos = linkPos;
			++outDist;
		}

		return nullptr;
	}

	void Level::removeFlowLink(Vec2i const& pos, int dir)
	{
		Cell& cell = getCellChecked(pos);
		cell.removeFlow(dir);

		Vec2i linkPos = getLinkPos(pos, dir);
		Cell& linkCell = getCellChecked(linkPos);

		int dirInv = InverseDir(dir);
		linkCell.removeFlow(dirInv);
	}

	void Level::removeFlowToEnd(Vec2i const& pos, int dir)
	{
		Vec2i curPos = pos;
		int   curDir = dir;

		Cell* curCell = &getCellChecked(pos);
		for (;;)
		{
			curCell->removeFlow(curDir);
			Vec2i linkPos = getLinkPos(curPos, curDir);
			Cell& linkCell = getCellChecked(linkPos);

			int curDirInv = InverseDir(curDir);
			curDir = linkCell.getLinkedFlowDir(curDirInv);

			linkCell.removeFlow(curDirInv);

			if (curDir == -1)
				break;

			curCell = &linkCell;
			curPos = linkPos;
		}
	}

	FlowFree::ColorType Level::linkFlow(Vec2i const& pos, int dir)
	{
		Cell& cellOut = getCellChecked(pos);

		if (cellOut.blockMask & BIT(dir))
			return 0;

		ColorType color = cellOut.evalFlowOut(dir);
		if (color == 0)
			return 0;

		Vec2i linkPos = getLinkPos(pos, dir);
		Cell& cellIn = getCellChecked(linkPos);

		int dirIn = InverseDir(dir);
		if (!cellIn.flowIn(dirIn, color))
			return 0;

		cellOut.flowOut(dir, color);
		return color;
	}

}//namespace FlowFree
