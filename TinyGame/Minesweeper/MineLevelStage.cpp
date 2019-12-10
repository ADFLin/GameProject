#include "Stage/TestStageHeader.h"

#include "MineDefines.h"
#include "GameInterface.h"
#include "DataStructure/Grid2d.h"

namespace Mine
{
	class Level
	{
	public:
		bool getCellSize(int& sx, int& sy)
		{
			sx = mCells.getSizeX();
			sy = mCells.getSizeY();
		}
		int  openCell(int cx, int cy)
		{
			assert(mCells.checkRange(cx, cy));
			CellData& cell = mCells(cx, cy);
			if( cell.bMarked )
				return CV_FLAG;

			cell.bProbed = true;
			return cell.number;
		}
		bool markCell(int cx, int cy)
		{
			assert(mCells.checkRange(cx, cy));
			CellData& cell = mCells(cx, cy);
			if( cell.bMarked )
				return true;

			cell.bMarked = true;
			return true;
		}

		bool unmarkCell(int cx, int cy)
		{
			assert(mCells.checkRange(cx, cy));
			CellData& cell = mCells(cx, cy);
			if( !cell.bMarked )
				return true;

			cell.bMarked = false;
			return true;
		}

		void openNeighberCell(int cx, int cy)
		{

		}

		int  lookCell(int cx, int cy)
		{
			assert(mCells.checkRange(cx, cy));
			CellData& cell = mCells(cx, cy);
			if( cell.bProbed )
				return cell.number;
			return CV_UNPROBLED;
		}

		void restart()
		{
			restoreCell(mCells.getSizeX(), mCells.getSizeY(), mNumBomb);
		}

		void restoreCell(int sx, int sy, int numBomb)
		{
			mCells.resize(sx, sy);
			assert(numBomb < mCells.getRawDataSize());

			for( int i = 0; i < numBomb; ++i )
			{
				mCells[i].number = CV_BOMB;
			}
			for( int i = numBomb; i < mCells.getRawDataSize(); ++i )
			{
				mCells[i].number = 0;

			}

			for( int i = 0; i < mCells.getRawDataSize(); ++i )
			{
				for( int j = 0; j < mCells.getRawDataSize(); ++j )
				{
					std::swap(mCells[i].number, mCells[rand() % mCells.getRawDataSize()].number);
				}
			}

			for( int i = 0; i < mCells.getRawDataSize(); ++i )
			{
				mCells[i].bProbed = false;

				if( mCells[i].number != 0 )
					continue;
				int cx = i % mCells.getSizeX();
				int cy = i / mCells.getSizeX();
				int x1 = std::max(0, cx - 1);
				int x2 = std::min(mCells.getSizeX() - 1, cx + 1);
				int y1 = std::max(0, cy - 1);
				int y2 = std::min(mCells.getSizeY() - 1, cy + 1);

				int num = 0;
				for( int x = x1; x <= x2; ++x )
				{
					for( int y = y1; y <= y2; ++y )
					{
						int idx = mCells.toIndex(x, y);
						if( mCells[idx].number == CV_BOMB )
							++num;
					}
				}
				mCells[i].number = num;
			}
		}


		struct CellData
		{
			int   number;
			bool  bProbed;
			bool  bMarked;
		};

		TGrid2D< CellData > mCells;
		int mNumBomb;
	};

	class LevelMineMap : public IMineMap
	{
	public:
		LevelMineMap( Level& level )
			:mLevel(level){}

		virtual int  probe(int cx, int cy)
		{
			return mLevel.openCell(cx, cy);
		}
		virtual int  look(int cx, int cy, bool bWaitResult)
		{
			return mLevel.lookCell(cx, cy);
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
		virtual int  getBombNum() { return mLevel.mNumBomb; }

		Level& mLevel;
	};

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}

		Level mLevel;

		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
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
			mLevel.restoreCell(30, 16, 99);
		}
		void tick() {}
		void updateFrame(int frame) {}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		virtual void onRender(float dFrame) 
		{
			Graphics2D& g = Global::GetGraphics2D();
			draw(g ,LevelMineMap(mLevel), Vec2i(20, 20) ); 
		}

		int const LengthCell = 20;

		void draw(Graphics2D& g , IMineMap& mineMap , Vec2i const& drawOrigin )
		{

			RenderUtility::SetPen(g, EColor::Gray);
			RenderUtility::SetBrush(g, EColor::Gray);
			g.drawRect(Vec2i(0, 0), Global::GetDrawEngine().getScreenSize());



			Vec2i textOrg = drawOrigin + Vec2i(5, 3);

			FixString<512> str;

			int sizeX = mineMap.getSizeX();
			int sizeY = mineMap.getSizeY();
			for( int j = 0; j < sizeY; ++j )
			{
				for( int i = 0; i < sizeX; ++i)
				{
					Vec2i offset = Vec2i(i * LengthCell, j *LengthCell);
					Vec2i pt = textOrg + offset;

					int number = mineMap.look(i, j, false);
					switch( number )
					{
					case CV_FLAG:
						g.drawCircle(pt + Vec2i(LengthCell, LengthCell) / 2, 4);
						break;
					case CV_UNPROBLED:
						break;
					case 0:
						break;
					default:
						g.drawText(pt, Vec2i(LengthCell, LengthCell), FStringConv::From(number));
						break;
					}
				}
			}

			RenderUtility::SetPen(g, EColor::Black);
			int TotalSize;
			TotalSize = sizeY * LengthCell;
			for( int i = 0; i <= sizeX; ++i )
			{
				Vec2i p1 = drawOrigin + Vec2i(i * LengthCell, 0);
				g.drawLine(p1, p1 + Vec2i(0, TotalSize));
			}
			TotalSize = sizeX * LengthCell;
			for( int i = 0; i <= sizeY; ++i )
			{
				Vec2i p1 = drawOrigin + Vec2i(0, i * LengthCell);
				g.drawLine(p1, p1 + Vec2i(TotalSize, 0));
			}

#if 0
			ExpSolveStrategy::ProbInfo const& otherProb = mStragtegy.getOtherProbInfo();
			int cx, cy;
			if( mStragtegy.getSolveState() == ExpSolveStrategy::eProbSolve )
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

		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(KeyMsg const& msg)
		{
			if( !msg.isDown())
				return false;
			switch(msg.getCode())
			{
			case EKeyCode::R: restart(); break;
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
	protected:
	};

	REGISTER_STAGE("MineSweeper", TestStage, EStageGroup::Test);
}