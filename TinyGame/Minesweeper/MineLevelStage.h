#pragma once
#ifndef MineLevelStage_H_A26CEC18_98E8_4E16_8C5C_F3F446822D68
#define MineLevelStage_H_A26CEC18_98E8_4E16_8C5C_F3F446822D68

#include "Stage/TestStageHeader.h"

#include "MineDefines.h"
#include "GameInterface.h"
#include "DataStructure/Grid2d.h"
#include "SystemPlatform.h"

#include "Tween.h"
#include "EasingFunction.h"
#include "MineSweeperSolver.h"

namespace Mine
{

	class Level
	{
	public:

		int  openCell(int cx, int cy);
		bool openNeighberCell(int cx, int cy, bool& bHaveBomb);

		void expendCell(int cx, int cy);
		bool markCell(int cx, int cy);
		bool unmarkCell(int cx, int cy);
		
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
		void makeCellProbed(CellData& cell);


		int openCellInternal(int cx, int cy);
		bool validateMarkCount();


		int getMarkCount() const { return mMarkCount; }

		TGrid2D< CellData > mCells;
		int  mNumBombs;
		bool mbBuildBomb;
		int  mMarkCount;
	};

	class LevelMineQuery : public IMineQuery
	{
	public:
		LevelMineQuery(Level& level)
			:mLevel(level)
		{
		}

		virtual int  lookCell(int cx, int cy, bool bWaitResult)
		{
			Level::CellData& cell = mLevel.mCells(cx, cy);
			if (bGameOver && !bCracked)
			{
				if (cell.isMarked() && cell.number != CV_BOMB)
				{
					return CV_FLAG_NO_BOMB;
				}
			}

			if (cell.isProbed() || bCracked)
				return cell.number;

			if (cell.isMarked())
				return CV_FLAG;

			return CV_UNPROBLED;
		}

		virtual int  getSizeX() { return mLevel.mCells.getSizeX(); }
		virtual int  getSizeY() { return mLevel.mCells.getSizeY(); }
		virtual int  getBombNum() { return mLevel.mNumBombs; }

		Level& mLevel;
		bool bCracked = false;
		bool bGameOver = false;
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
			::srand(generateRandSeed());
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

			float dt = float(time) / 1000;
			mTweener.update(dt);
		}
		Vec2i mapOrigin = Vec2i(20, 20);
		int const LengthCell = 24;

		int openGameCell(int cx, int cy)
		{
			CHECK(bPlayMode);

			auto const& cell = mLevel.mCells(cx, cy);
			if (cell.isMarked())
				return CV_FLAG;
	
			if (mLevel.mbBuildBomb == false)
			{
				mMsStart = SystemPlatform::GetTickCount();
			}
			int state = mLevel.openCell(cx, cy);
			if (state == CV_BOMB)
			{
				execGameOver();
			}

			return state;

		}


		virtual void onRender(float dFrame);

		Vec2i toCellPos(Vec2i const& screenPos)
		{
			Vec2i pos = screenPos - mapOrigin;
			return Vec2i(Math::ToTileValue(pos.x, LengthCell), Math::ToTileValue(pos.y, LengthCell));
		}

		void draw(Graphics2D& g, IMineQuery& mineMap, Vec2i const& drawOrigin);



		Vec2i mHoveredCellPos;
		bool  bOpenNeighborOp = false;


		bool onMouse(MouseMsg const& msg);

		Tween::GroupTweener< float > mTweener;

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
			case EKeyCode::X:
				{
					buildLevelSolver();
					mSolver->setepSolve();
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

		void buildLevelSolver();

		bool bPlayMode;
		bool bGameOver;

		float mCanvasAlpha;
		int64 mMsStart;
		int64 mElapsedTime;

		ISolveStrategy* mStrategy = nullptr;
		IMineControlClient* mControl = nullptr;
		class MineSweeperSolver* mSolver = nullptr;


		void execGameOver()
		{
			bGameOver = true;
			mCanvasAlpha = 0.0f;
			mTweener.tweenValue< Easing::ICubic >(mCanvasAlpha, 0.0f, 1.0f, 0.8, 0);
		}

		friend class LevelGameControlClient;

	};


	class LevelGameControlClient : public IMineControlClient
	{
	public:
		LevelGameControlClient(TestStage& stage)
			:mStage(stage)
			,mLevel(stage.mLevel)
		{

		}
		virtual bool getCellSize(int& sx, int& sy)
		{
			sx = mLevel.mCells.getSizeX();
			sy = mLevel.mCells.getSizeY();
			return true;
		}
		virtual int  openCell(int cx, int cy)
		{
			return mStage.openGameCell(cx, cy);
		}
		virtual int  lookCell(int cx, int cy, bool bWaitResult)
		{
			Level::CellData& cell = mLevel.mCells(cx, cy);
			if (cell.isProbed())
				return cell.number;

			if (cell.isMarked())
				return CV_FLAG;

			return CV_UNPROBLED;
		}
		virtual bool markCell(int cx, int cy)
		{
			return mLevel.markCell(cx, cy);
		}
		virtual bool unmarkCell(int cx, int cy)
		{
			return mLevel.unmarkCell(cx, cy);
		}
		virtual void openNeighberCell(int cx, int cy)
		{
			bool bHaveBomb;
			mLevel.openNeighberCell(cx, cy, bHaveBomb);
		}

		Level& mLevel;
		TestStage& mStage;

		bool hookGame() override;
		bool setupMode(EGameMode mode) override;
		bool setupCustomMode(int sx, int sy, int numbomb) override;

		void restart() override;

		EGameState checkState() override;

		int getSizeX() override;
		int getSizeY() override;
		int getBombNum() override;

	};
}

#endif // MineLevelStage_H_A26CEC18_98E8_4E16_8C5C_F3F446822D68
