#include "MineLevelStage.h"
#include "ExpSolveStrategy.h"
#include "MineSweeperSolver.h"
#include "CSPSolveStrategy.h"
#include "RHI/RHIGraphics2D.h"
#include "ProfileSystem.h"


#include <fstream>
#include <string>
#include <vector>


namespace Mine
{
	namespace
	{
		class MineLayerPolicy : public RHIGraphics2D::LayerPolicy
		{
		public:
			enum
			{
				eRectLayer = 0,
				eShapeLayer = 1,
				eTextLayer = 2,
			};

			int32 getLayer(Render::RenderBatchedElement const& element, RHIGraphics2D::RenderState const& state) override
			{
				switch (element.type)
				{
				case Render::RenderBatchedElement::Text:
				case Render::RenderBatchedElement::ColoredText:
					return eTextLayer;
				case Render::RenderBatchedElement::Rect:
					return eRectLayer;
				default:
					return eShapeLayer;
				}
			}
		};
	}

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

	bool Level::validateMarkCount()
	{
		int count = 0;
		for (int i = 0; i < mCells.getRawDataSize(); ++i)
		{
			if (mCells[i].isMarked())
				++count;
		}

		if (mMarkCount != count)
		{
			mMarkCount = count;
			return false;
		}
		return true;
	}

	bool Level::openNeighberCell(int cx, int cy, bool& bHaveBomb)
	{
		assert(mCells.checkRange(cx, cy));
		CellData& cell = mCells(cx, cy);
		if (!cell.isProbed())
			return false;

		if (cell.number <= 0)
			return false;

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

	void Level::makeCellProbed(CellData& cell)
	{
		assert(!cell.isProbed());
		cell.bProbed = true;
		if (cell.bMarked)
		{
			cell.bMarked = false;
			--mMarkCount;
		}
	}

	bool TestStage::saveDebugState(char const* path)
	{
		std::ofstream file(path);
		if (!file)
		{
			LogWarning(0, "Minesweeper save debug state failed: %s", path);
			return false;
		}

		int const sizeX = mLevel.mCells.getSizeX();
		int const sizeY = mLevel.mCells.getSizeY();

		file << "MinesweeperDebugState 1\n";
		file << "Size " << sizeX << " " << sizeY << "\n";
		file << "Bombs " << mLevel.mNumBombs << "\n";
		file << "BuildBomb " << (mLevel.mbBuildBomb ? 1 : 0) << "\n";
		file << "GameOver " << (bGameOver ? 1 : 0) << "\n";
		file << "Elapsed " << mElapsedTime << "\n";
		file << "FailedCell " << (mbHaveFailedCell ? 1 : 0) << " " << mLastFailedCellPos.x << " " << mLastFailedCellPos.y << "\n";

		file << "Numbers\n";
		for (int y = 0; y < sizeY; ++y)
		{
			for (int x = 0; x < sizeX; ++x)
			{
				Level::CellData const& cell = mLevel.mCells(x, y);
				int const number = int(cell.number);
				file << (number == CV_BOMB ? '*' : char('0' + number));
			}
			file << "\n";
		}

		file << "View\n";
		for (int y = 0; y < sizeY; ++y)
		{
			for (int x = 0; x < sizeX; ++x)
			{
				Level::CellData const& cell = mLevel.mCells(x, y);
				int const number = int(cell.number);
				bool const bSkipFailedStep = bGameOver && mbHaveFailedCell && x == mLastFailedCellPos.x && y == mLastFailedCellPos.y;
				if (bSkipFailedStep)
					file << '#';
				else if (cell.isProbed())
					file << (number == CV_BOMB ? '*' : char('0' + number));
				else if (cell.isMarked())
					file << 'F';
				else
					file << '#';
			}
			file << "\n";
		}

		LogMsg("Minesweeper save debug state: %s", path);
		return true;
	}

	bool TestStage::loadDebugState(char const* path)
	{
		std::ifstream file(path);
		if (!file)
		{
			LogWarning(0, "Minesweeper load debug state failed: %s", path);
			return false;
		}

		std::string tag;
		int version = 0;
		file >> tag >> version;
		if (tag != "MinesweeperDebugState" || version != 1)
		{
			LogWarning(0, "Minesweeper load debug state invalid header: %s", path);
			return false;
		}

		int sizeX = 0;
		int sizeY = 0;
		int numBombs = 0;
		int buildBomb = 0;
		int gameOver = 0;
		int haveFailedCell = 0;
		int64 elapsed = 0;
		Vec2i failedCellPos = Vec2i(-1, -1);

		file >> tag >> sizeX >> sizeY;
		if (tag != "Size" || sizeX <= 0 || sizeY <= 0)
			return false;

		file >> tag >> numBombs;
		if (tag != "Bombs")
			return false;

		file >> tag >> buildBomb;
		if (tag != "BuildBomb")
			return false;

		file >> tag >> gameOver;
		if (tag != "GameOver")
			return false;

		file >> tag >> elapsed;
		if (tag != "Elapsed")
			return false;

		file >> tag;
		if (tag == "FailedCell")
		{
			file >> haveFailedCell >> failedCellPos.x >> failedCellPos.y;
			file >> tag;
		}

		if (tag != "Numbers")
			return false;

		TArray< std::string > numbers(sizeY);
		for (int y = 0; y < sizeY; ++y)
		{
			file >> numbers[y];
			if (int(numbers[y].size()) != sizeX)
				return false;
		}

		file >> tag;
		if (tag != "View")
			return false;

		TArray< std::string > view(sizeY);
		for (int y = 0; y < sizeY; ++y)
		{
			file >> view[y];
			if (int(view[y].size()) != sizeX)
				return false;
		}

		mLevel.restoreCell(sizeX, sizeY, numBombs);
		mLevel.mbBuildBomb = buildBomb != 0;
		mLevel.mMarkCount = 0;

		for (int y = 0; y < sizeY; ++y)
		{
			for (int x = 0; x < sizeX; ++x)
			{
				Level::CellData& cell = mLevel.mCells(x, y);
				char const numChar = numbers[y][x];
				if (numChar == '*')
					cell.number = CV_BOMB;
				else if ('0' <= numChar && numChar <= '8')
					cell.number = numChar - '0';
				else
					return false;

				char const viewChar = view[y][x];
				cell.bProbed = (viewChar == '*') || ('0' <= viewChar && viewChar <= '8');
				cell.bMarked = viewChar == 'F';
				if (cell.bMarked)
					++mLevel.mMarkCount;
			}
		}

		bGameOver = false;
		mbHaveFailedCell = false;
		mLastFailedCellPos = Vec2i(-1, -1);
		mElapsedTime = elapsed;
		mMsStart = 0;
		bOpenNeighborOp = false;
		mapOrigin.x = (::Global::GetScreenSize().x - LengthCell * mLevel.mCells.getSizeX()) / 2;

		if (mSolver)
			mSolver->scanMap();

		LogMsg("Minesweeper load debug state: %s", path);
		return true;
	}

	void TestStage::onRender(float dFrame)
	{
		IGraphics2D& g = Global::GetIGraphics2D();
		MineLayerPolicy layerPolicy;
		RHIGraphics2D* rhiGraphics = nullptr;
		RHIGraphics2D::LayerPolicy* prevLayerPolicy = nullptr;
		if (g.isUseRHI())
		{
			rhiGraphics = &g.getImpl<RHIGraphics2D>();
			prevLayerPolicy = rhiGraphics->getLayerPolicy();
			rhiGraphics->setLayerPolicy(&layerPolicy);
		}

		LevelMineQuery map{ mLevel };
		if (bPlayMode)
		{
			map.bCracked = mbCracked;
			map.bGameOver = bGameOver;
		}
		draw(g, map, mapOrigin);

		if (rhiGraphics)
		{
			rhiGraphics->flush();
			rhiGraphics->setLayerPolicy(prevLayerPolicy);
		}

		if (bPlayMode)
		{
			Vec2i viewportSize = LengthCell * Vec2i(mLevel.mCells.getSizeX(), mLevel.mCells.getSizeY());
			if (bGameOver)
			{
				RenderUtility::SetPen(g, EColor::Black);
				RenderUtility::SetBrush(g, EColor::Black);
				g.beginBlend(mapOrigin, viewportSize, 0.5 * mCanvasAlpha);
				g.drawRect(mapOrigin, viewportSize);
				g.endBlend();

				if (mCanvasAlpha < 1.0f)
				{
					g.beginBlend(mapOrigin, viewportSize, mCanvasAlpha);
				}

				RenderUtility::SetFont(g, FONT_S24);
				RenderUtility::SetFontColor(g, EColor::Red);
				g.drawText(mapOrigin, viewportSize, "Game Over");
				if (mCanvasAlpha < 1.0f)
				{
					g.endBlend();
				}
			}


			RenderUtility::SetFont(g, FONT_S12);
			RenderUtility::SetFontColor(g, EColor::Yellow);
			g.drawText(Vec2i(200, 0), InlineStringA<>::Make("Mark Count = %d", mLevel.getMarkCount()));

			if (CSPSolveStrategy* cspStrategy = dynamic_cast<CSPSolveStrategy*>(mStrategy))
			{
				int guessX;
				int guessY;
				float guessProb;
				cspStrategy->getLastGuessInfo(guessX, guessY, guessProb);
				if (guessProb >= 0.0f)
				{
					g.drawText(Vec2i(360, 0), InlineStringA<>::Make("Guess = (%d,%d) %d%%", guessX, guessY, int(100.0f * guessProb + 0.5f)));
				}
			}

			if (!bGameOver && mMsStart > 0)
			{
				mElapsedTime = SystemPlatform::GetTickCount() - mMsStart;
			}

			g.drawText(Vec2i(50, 0), InlineStringA<>::Make("Elapsed Time = %ld", mElapsedTime / 1000));
		}
	}

	void TestStage::draw(IGraphics2D& g, IMineQuery& mineMap, Vec2i const& drawOrigin)
	{
		RenderUtility::SetPen(g, EColor::Gray);
		RenderUtility::SetBrush(g, EColor::Gray);
		g.drawRect(Vec2i(0, 0), ::Global::GetScreenSize());

		Vec2i textOrg = drawOrigin + Vec2i(5, 3);

		InlineString<512> str;

		static const Color3ub NumberColorMap[] =
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

		static const Color3ub GroupFrameColorMap[] =
		{
			Color3ub(255, 235, 59),
			Color3ub(244, 67, 54),
			Color3ub(33, 150, 243),
			Color3ub(76, 175, 80),
			Color3ub(156, 39, 176),
			Color3ub(255, 152, 0),
			Color3ub(0, 188, 212),
			Color3ub(233, 30, 99),
			Color3ub(205, 220, 57),
			Color3ub(121, 85, 72),
		};

		int sizeX = mineMap.getSizeX();
		int sizeY = mineMap.getSizeY();
		auto IsValidPos = [=](int cx, int cy) -> bool
		{
			return 0 <= cx && cx < sizeX &&
				   0 <= cy && cy < sizeY;
		};

		RenderUtility::SetFont(g, FONT_S12);

		bool bHoveredPosValid = IsValidPos(mHoveredCellPos.x, mHoveredCellPos.y);
		for (int j = 0; j < sizeY; ++j)
		{
			for (int i = 0; i < sizeX; ++i)
			{
				Vec2i offset = Vec2i(i * LengthCell, j *LengthCell);
				Vec2i pt = drawOrigin + offset;

				int state = mineMap.lookCell(i, j, false);

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
				case CV_FLAG_NO_BOMB:
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
							int L = 56 * LengthCell / 100;
							int Tx = 20 * LengthCell / 100;
							int Ty = 10 * LengthCell / 100;
							int Bx = Tx + L;
							int By = Ty + L;


							Vector2 TriPos[] =
							{
								pt + Vector2(Tx,By),
								pt + Vector2(Bx,(By+Ty)/2),
								pt + Vector2(Tx,Ty),
							};
							g.drawPolygon(TriPos, 3);
							g.drawRect(pt + Vec2i(Tx, Ty), Vec2i(2 , 80 * LengthCell / 100));


						}
						else if (state == CV_FLAG_NO_BOMB)
						{
							int offset = 2;
							g.setPen(Color3ub(255, 0, 0), 2);
							g.drawLine(pt + Vec2i(offset, offset), pt + Vec2i(LengthCell - offset, LengthCell - offset));
							g.drawLine(pt + Vec2i(LengthCell - offset, offset), pt + Vec2i(offset, LengthCell - offset));
						}
						else if (state == CV_UNPROBLED)
						{
							if (CSPSolveStrategy* cspStrategy = dynamic_cast<CSPSolveStrategy*>(mStrategy))
							{
								int groupIndex = cspStrategy->getCellConstraintGroupIndex(i, j);
								if (groupIndex >= 0)
								{
									int const border = 2;
									Color3ub frameColor = GroupFrameColorMap[groupIndex % ARRAY_SIZE(GroupFrameColorMap)];
									g.setPen(frameColor);
									g.setBrush(frameColor);
									g.drawRect(pt, Vec2i(LengthCell, border));
									g.drawRect(pt, Vec2i(border, LengthCell));
									g.drawRect(pt + Vec2i(0, LengthCell - border), Vec2i(LengthCell, border));
									g.drawRect(pt + Vec2i(LengthCell - border, 0), Vec2i(border, LengthCell));
								}

							}
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

						static const Vec2i dirOffsets[] =
						{
							Vec2i(1,0),Vec2i(-1,0),Vec2i(0,1),Vec2i(0,-1),
						};

						auto IsNeedBorder = [](int stateN)
						{
							return stateN == CV_FLAG || stateN == CV_UNPROBLED || stateN == CV_BOMB || stateN == CV_FLAG_NO_BOMB;
						};
						for (int dir = 0; dir < 4; ++dir)
						{
							int nx = i + dirOffsets[dir].x;
							int ny = j + dirOffsets[dir].y;
							if (!IsValidPos(nx, ny))
								continue;

							int stateN = mineMap.lookCell(nx, ny, false);
							if (IsNeedBorder(stateN))
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

						static const Vec2i dirOffsets2[] =
						{
							Vec2i(1,1),Vec2i(-1,1),Vec2i(-1,-1),Vec2i(1,-1),
						};
						for (int dir = 0; dir < 4; ++dir)
						{
							int nx = i + dirOffsets2[dir].x;
							int ny = j + dirOffsets2[dir].y;
							if ( !IsValidPos(nx ,ny) )
								continue;

							int stateN = mineMap.lookCell(nx, ny, false);
							if (IsNeedBorder(stateN))
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

				if (state == CV_UNPROBLED)
				{
					if (CSPSolveStrategy* cspStrategy = dynamic_cast<CSPSolveStrategy*>(mStrategy))
					{
						float prob = cspStrategy->getCellProbability(i, j);
						if (prob >= 0.0f)
						{
							RenderUtility::SetFont(g, FONT_S8);
							g.setTextColor(Color3ub(255, 255, 0));
							g.drawText(pt, Vec2i(LengthCell, LengthCell), InlineStringA<>::Make("%d", int(100.0f * prob + 0.5f)));
							RenderUtility::SetFont(g, FONT_S12);
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

	MsgReply TestStage::onMouse(MouseMsg const& msg)
	{
		if (bPlayMode)
		{
			Vec2i cPos = toCellPos(msg.getPos());

			if (msg.onMoving())
			{
				mHoveredCellPos = cPos;
			}

			if (bGameOver == false && mLevel.mCells.checkRange(cPos.x, cPos.y))
			{
				auto const& cell = mLevel.mCells(cPos.x, cPos.y);
				if (cell.isProbed())
				{
					if ((msg.onLeftDown() && msg.isRightDown()) || (msg.onRightDown() && msg.isLeftDown()))
					{
						bOpenNeighborOp = true;
						bool bHaveBomb;
						if (mLevel.openNeighberCell(cPos.x, cPos.y, bHaveBomb))
						{
							if (bHaveBomb)
							{
								execGameOver();
							}
						}
					}
					else
					{
						bOpenNeighborOp = false;
					}
				}
				else
				{
					if (msg.onLeftDown())
					{
						openGameCell(cPos.x, cPos.y);
					}
					else if (msg.onRightDown())
					{
						if (cell.isMarked())
						{
							mLevel.unmarkCell(cPos.x, cPos.y);
						}
						else
						{
							mLevel.markCell(cPos.x, cPos.y);
						}
					}
				}
			}
		}
		return BaseClass::onMouse(msg);
	}

	MsgReply TestStage::onKey(KeyMsg const& msg)
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::C:
				if (bPlayMode)
				{
					mbCracked = !mbCracked;
				}
				break;
			case EKeyCode::X:
				{
					buildLevelSolver();
					{
						PROFILE_ENTRY("StepSolve");
						mSolver->stepSolve();
					}

				}
				break;
			case EKeyCode::F5:
				saveDebugState("MinesweeperDebugState.txt");
				break;
			case EKeyCode::F6:
				loadDebugState("MinesweeperDebugState.txt");
				break;
			}
		}
		return BaseClass::onKey(msg);
	}

	void TestStage::buildLevelSolver()
	{
		if (mSolver == nullptr)
		{
			mControl = new LevelGameControlClient(*this);
			//mStrategy = new ExpSolveStrategy();
			mStrategy = new CSPSolveStrategy();
			mSolver = new MineSweeperSolver(*mStrategy, *mControl);
			mSolver->enableSetting(ST_MARK_FLAG, true);
			mSolver->scanMap();
		}
	}

	bool LevelGameControlClient::hookGame()
	{
		return true;
	}

	bool LevelGameControlClient::setupMode(EGameMode mode)
	{
		return false;
	}

	bool LevelGameControlClient::setupCustomMode(int sx, int sy, int numbomb)
	{
		mLevel.restoreCell(sx, sy, numbomb);
		return true;
	}

	void LevelGameControlClient::restart()
	{
		mStage.restart(false);
	}

	EGameState LevelGameControlClient::checkState()
	{
		if (mStage.bGameOver)
		{
			return EGameState::Fail;
		}
		return EGameState::Run;
	}

	int LevelGameControlClient::getSizeX()
	{
		return mLevel.mCells.getSizeX();
	}

	int LevelGameControlClient::getSizeY()
	{
		return mLevel.mCells.getSizeY();
	}

	int LevelGameControlClient::getBombNum()
	{
		return mLevel.mNumBombs;
	}

}
