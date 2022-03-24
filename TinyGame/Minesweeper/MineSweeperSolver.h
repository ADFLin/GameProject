#ifndef MineSweeperSolver_h__
#define MineSweeperSolver_h__

#include <vector>
#include "GameInterface.h"


namespace Mine
{

	enum SolverSetting
	{
		ST_MARK_FLAG = 1 << 0,
		ST_FAST_OPEN_CELL = 1 << 1,
		ST_DISABLE_PROB_SOLVE = 1 << 2,
		ST_AUTO_RESTART = 1 << 3,
		ST_PAUSE_SOLVE = 1 << 4,
	};

	class GameHookMap
	{
	public:


		void restart()
		{



		}

		IMineControlClient& mController;
	};


	class MineSweeperSolver : public IMineControl
	{
	public:
		MineSweeperSolver(ISolveStrategy& strategy, IMineControlClient& controller);

		void       restart();
		void       scanMap()
		{
			mStrategy->loadMap(*this);
		}
		bool       setepSolve() /*throw ( ControlException )*/;
		void       enableSetting(SolverSetting setting, bool beE = true);
		bool       checkSetting(SolverSetting setting) { return (mSettingFlag & setting) != 0; }
		void       setCustomMode(int sx, int sy, int numBomb);
		EGameState  getGameState() const { return mGameState; }

	public:
		virtual int openCell(int cx, int cy)
		{
			if( mGameState == EGameState::Fail )
				return CV_UNKNOWN;
			return mControl->openCell(cx, cy);
		}

		virtual int lookCell(int cx, int cy, bool bWaitResult)
		{
			return mControl->lookCell(cx, cy, bWaitResult);
		}

		virtual bool markCell(int cx, int cy)
		{
			if( mGameState == EGameState::Fail )
				return false;

			if( mSettingFlag & (ST_MARK_FLAG | ST_FAST_OPEN_CELL) )
			{
				try
				{
					mControl->markCell(cx, cy);
				}
				catch( ControlException& e )
				{
					enableSetting(ST_DISABLE_PROB_SOLVE, false);
					throw e;
				}
			}
			return true;
		}
		virtual bool unmarkCell(int cx, int cy)
		{
			if( mGameState == EGameState::Fail )
				return false;

			if( (mSettingFlag & ST_MARK_FLAG) == 0 )
			{
				mControl->markCell(cx, cy);
			}
			return true;
		}
		virtual int getSizeX() { return getCellSizeX(); }
		virtual int getSizeY() { return getCellSizeY(); }
		virtual int getBombNum() { return mNumTotalBomb; }

	public:

	private:

		int        getCellSizeX() const { return mCellSizeX; }
		int        getCellSizeY() const { return mCellSizeY; }




		int  mCellSizeX;
		int  mCellSizeY;
		int  mNumTotalBomb;
		EGameState      mGameState;
		ISolveStrategy* mStrategy;
		IMineControlClient* mControl;

	private:

		bool      isValidRange(int cx, int cy)
		{
			return 0 <= cx  && cx < mCellSizeX &&
				0 <= cy  && cy < mCellSizeY;
		}


		unsigned mSettingFlag;

	};

}//namespace Mine

#endif // MineSweeperSolver_h__