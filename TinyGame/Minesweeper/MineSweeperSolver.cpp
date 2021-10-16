#include "MineSweeperSolver.h"
#include <cassert>

namespace Mine
{
	MineSweeperSolver::MineSweeperSolver(ISolveStrategy& strategy, IMineControlClient& control)
	{
		mSettingFlag = 0;
		mGameState = EGameState::Run;
		mControl = &control;
		mStrategy = &strategy;
		setCustomMode(30, 16, 99);
	}


	void MineSweeperSolver::setCustomMode(int sx, int sy, int numBomb)
	{
		mCellSizeX = sx;
		mCellSizeY = sy;
		mNumTotalBomb = numBomb;
	}

	bool MineSweeperSolver::setepSolve() /*throw ( ControlException )*/
	{
		mGameState = mControl->checkState();

		switch( mGameState )
		{
		case EGameState::Fail:
		case EGameState::Complete:
			if( mSettingFlag & ST_AUTO_RESTART )
			{
				restart();
				return true;
			}
			else
			{
				return false;
			}
			break;
		}

		if( mSettingFlag & ST_PAUSE_SOLVE )
			return true;

		return mStrategy->solveStep();
	}

	void MineSweeperSolver::restart()
	{
		mControl->restart();
		enableSetting(ST_PAUSE_SOLVE, false);
		mStrategy->restart(*this);
	}

	void MineSweeperSolver::enableSetting(SolverSetting setting, bool beE /*= true */)
	{
		if( beE )
			mSettingFlag |= setting;
		else
			mSettingFlag &= ~setting;
	}


}//namespace Mine