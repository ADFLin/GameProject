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
		updateGameInfoFromControl();
	}


	void MineSweeperSolver::setCustomMode(int sx, int sy, int numBomb)
	{
		mCellSizeX = sx;
		mCellSizeY = sy;
		mNumTotalBomb = numBomb;
	}

	void MineSweeperSolver::updateGameInfoFromControl()
	{
		setCustomMode(mControl->getSizeX(), mControl->getSizeY(), mControl->getBombNum());
	}

	bool MineSweeperSolver::stepSolve() /*throw ( ControlException )*/
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
		updateGameInfoFromControl();
		enableSetting(ST_PAUSE_SOLVE, false);
		mStrategy->restart(*this);
	}

	void MineSweeperSolver::resetStrategy()
	{
		mGameState = EGameState::Run;
		updateGameInfoFromControl();
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
