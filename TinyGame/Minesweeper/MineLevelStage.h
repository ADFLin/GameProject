#pragma once
#ifndef MineLevelStage_H_A26CEC18_98E8_4E16_8C5C_F3F446822D68
#define MineLevelStage_H_A26CEC18_98E8_4E16_8C5C_F3F446822D68

#include "Stage/TestStageHeader.h"

#include "MineDefines.h"
#include "GameInterface.h"
#include "DataStructure/Grid2d.h"
#include "SystemPlatform.h"

namespace Mine
{

	class Level
	{
	public:

		int  openCell(int cx, int cy);


		void expendCell(int cx, int cy);

		bool markCell(int cx, int cy);

		bool unmarkCell(int cx, int cy);

		bool openNeighberCell(int cx, int cy, bool& bHaveBomb);

		int  lookCell(int cx, int cy, bool bCracked = false);

		void restart()
		{
			restoreCell(mCells.getSizeX(), mCells.getSizeY(), mNumBombs);
		}

		void buildBomb(int numBombs, int* ignoreCoords);
		void restoreCell(int sx, int sy, int numBombs);

		struct CellData
		{
			uint32  number;
			uint32  bProbed : 1;
			uint32  bMarked : 1;

			bool isProbed() const { return bProbed; }
			bool isMarked() const { return bMarked; }
		};
		void makeCellProbed(CellData& cell)
		{
			assert(!cell.isProbed());
			cell.bProbed = true;
			if (cell.bMarked)
			{
				cell.bMarked = false;
				--mMarkCount;
			}
		}


		int openCellInternal(int cx, int cy);
		bool validateMarkCount()
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


		int getMarkCount() const { return mMarkCount; }

		TGrid2D< CellData > mCells;
		int  mNumBombs;
		bool mbBuildBomb;
		int  mMarkCount;
	};

	class LevelMineMap : public IMineMap
	{
	public:
		LevelMineMap(Level& level)
			:mLevel(level)
		{
		}

		virtual int  probe(int cx, int cy)
		{
			return mLevel.openCell(cx, cy);
		}
		virtual int  look(int cx, int cy, bool bWaitResult)
		{
			return mLevel.lookCell(cx, cy, bCracked);
		}
		virtual bool mark(int cx, int cy)
		{
			return mLevel.markCell(cx, cy);
		}
		virtual bool unmark(int cx, int cy)
		{
			return mLevel.unmarkCell(cx, cy);
		}

		virtual int  getSizeX() { return mLevel.mCells.getSizeX(); }
		virtual int  getSizeY() { return mLevel.mCells.getSizeY(); }
		virtual int  getBombNum() { return mLevel.mNumBombs; }

		Level& mLevel;
		bool bCracked = false;
	};

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}

		Level mLevel;
		bool  mbCracked = false;
		virtual bool onInit()
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		void restart()
		{
			mLevel.restoreCell(30, 16, 99 + 20);
			bGameOver = false;
			mMsStart = 0;
			mElapsedTime = 0;
			bOpenNeighborOp = false;

			mapOrigin.x = (::Global::GetScreenSize().x - LengthCell * mLevel.mCells.getSizeX()) / 2;
		}
		void tick() {}
		void updateFrame(int frame) {}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for (int i = 0; i < frame; ++i)
				tick();

			updateFrame(frame);
		}
		Vec2i mapOrigin = Vec2i(20, 20);
		int const LengthCell = 24;

		int64 mMsStart;
		int64 mElapsedTime;
		virtual void onRender(float dFrame)
		{
			Graphics2D& g = Global::GetGraphics2D();
			LevelMineMap map{ mLevel };
			if (bPlayMode)
			{
				map.bCracked = mbCracked;
			}
			draw(g, map, mapOrigin);

			if (bPlayMode)
			{
				Vec2i viewportSize = LengthCell * Vec2i(mLevel.mCells.getSizeX(), mLevel.mCells.getSizeY());
				if (bGameOver)
				{
					RenderUtility::SetPen(g, EColor::Black);
					RenderUtility::SetBrush(g, EColor::Black);
					g.beginBlend(mapOrigin, viewportSize, 0.5);
					g.drawRect(mapOrigin, viewportSize);
					g.endBlend();


					RenderUtility::SetFont(g, FONT_S24);
					RenderUtility::SetFontColor(g, EColor::Red);


					g.drawText(mapOrigin, viewportSize,  "Game Over");
				}


				RenderUtility::SetFont(g, FONT_S12);
				RenderUtility::SetFontColor(g, EColor::Yellow);
				g.drawText(Vec2i(200, 0), InlineStringA<>::Make("Mark Count = %d", mLevel.getMarkCount()) );

				if (!bGameOver && mMsStart > 0)
				{
					mElapsedTime = SystemPlatform::GetTickCount() - mMsStart;
				}

				g.drawText(Vec2i(50, 0), InlineStringA<>::Make("Elapsed Time = %ld", mElapsedTime / 1000) );
			}
		}

		Vec2i toCellPos(Vec2i const& screenPos)
		{
			return (screenPos - mapOrigin + Vec2i(LengthCell-1 , LengthCell - 1) ) / LengthCell - Vec2i(1,1);
		}

		void draw(Graphics2D& g, IMineMap& mineMap, Vec2i const& drawOrigin);



		Vec2i mHoveredCellPos;
		bool  bOpenNeighborOp = false;


		bool onMouse(MouseMsg const& msg)
		{
			if (!BaseClass::onMouse(msg))
				return false;

			if (bPlayMode)
			{
				Vec2i cPos = toCellPos(msg.getPos());

				if (msg.onMoving())
				{
					mHoveredCellPos = cPos;
				}

				if (bGameOver == false && mLevel.mCells.checkRange(cPos.x, cPos.y))
				{
					int state = mLevel.lookCell(cPos.x, cPos.y);
					if (state <= 0)
					{
						if (msg.onLeftDown())
						{
							if (state == CV_UNPROBLED)
							{
								if (mLevel.mbBuildBomb == false)
								{
									mMsStart = SystemPlatform::GetTickCount();
								}
								int state = mLevel.openCell(cPos.x, cPos.y);
								if (state == CV_BOMB)
								{
									bGameOver = true;
								}
							}
						}
						else if (msg.onRightDown())
						{
							if (state == CV_UNPROBLED)
							{
								mLevel.markCell(cPos.x, cPos.y);
							}
							else
							{
								mLevel.unmarkCell(cPos.x, cPos.y);
							}
						}
					}
					else if (state > 0)
					{
						if ((msg.onLeftDown() || msg.onRightDown()) && msg.isLeftDown() && msg.isRightDown())
						{
							bOpenNeighborOp = true;
							bool bHaveBomb;
							if (mLevel.openNeighberCell(cPos.x, cPos.y, bHaveBomb))
							{
								if (bHaveBomb)
								{
									bGameOver = true;
								}
							}
						}
						else
						{
							bOpenNeighborOp = false;
						}
					}
				}
			}
			return true;
		}

		bool onKey(KeyMsg const& msg)
		{
			if (!msg.isDown())
				return false;
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::C:
				if (bPlayMode)
				{
					mbCracked = !mbCracked;
				}
				break;
			}
			return false;
		}

		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:

		bool bPlayMode;
		bool bGameOver;
	};



}

#endif // MineLevelStage_H_A26CEC18_98E8_4E16_8C5C_F3F446822D68
