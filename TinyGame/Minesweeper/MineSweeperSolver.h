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




	class IGame
	{

		void restart()
		{



		}
	};

	class GameHookMap
	{


		void restart()
		{



		}








		IGameController& mController;
	};


	class MineSweeperSolver : public IMineMap
	{
	public:
		MineSweeperSolver(ISolveStrategy& strategy, IGameController& controller);

		void       restart();
		void       scanMap()
		{
			mStrategy->loadMap(*this);
		}
		bool       setepSolve() /*throw ( HookException )*/;
		void       enableSetting(SolverSetting setting, bool beE = true);
		bool       checkSetting(SolverSetting setting) { return (mSettingFlag & setting) != 0; }
		void       setCustomMode(int sx, int sy, int numBomb);
		EGameState  getGameState() const { return mGameState; }

	public:
		virtual int probe(int cx, int cy)
		{
			if( mGameState == EGameState::Fail )
				return CV_UNKNOWN;
			return mControler->openCell(cx, cy);
		}

		virtual int look(int cx, int cy, bool bWaitResult)
		{
			return mControler->lookCell(cx, cy, bWaitResult);
		}

		virtual bool mark(int cx, int cy)
		{
			if( mGameState == EGameState::Fail )
				return false;

			if( mSettingFlag & (ST_MARK_FLAG | ST_FAST_OPEN_CELL) )
			{
				try
				{
					mControler->markCell(cx, cy);
				}
				catch( ControlException& e )
				{
					enableSetting(ST_DISABLE_PROB_SOLVE, false);
					throw e;
				}
			}
			return true;
		}
		virtual bool unmark(int cx, int cy)
		{
			if( mGameState == EGameState::Fail )
				return false;

			if( (mSettingFlag & ST_MARK_FLAG) == 0 )
			{
				mControler->markCell(cx, cy);
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
		IGameController* mControler;

	private:

		bool      isVaildRange(int cx, int cy)
		{
			return 0 <= cx  && cx < mCellSizeX &&
				0 <= cy  && cy < mCellSizeY;
		}


		unsigned mSettingFlag;

	};

}//namespace Mine

#endif // MineSweeperSolver_h__