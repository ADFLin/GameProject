#ifndef ExpSolveStrategy_h__
#define ExpSolveStrategy_h__

#include "GameInterface.h"

#include <vector>

namespace Mine
{
	class ExpSolveStrategy : public ISolveStrategy
	{
	public:
		ExpSolveStrategy();
	public:
		virtual void loadMap(IMineMap& map) { refreshInformation(map, false); }
		virtual void restart(IMineMap& map) { refreshInformation(map, true); }
		virtual bool solveStep() /*throw ( HookException )*/;

	private:
		IMineMap*  mMineMap;
	public:

		int  getCellSizeX() const { return mCellSizeX; }
		int  getCellSizeY() const { return mCellSizeY; }
		int  getSolvedBombNum() const { return mNumSolvedBomb; }
		int  getOpenCellNum() const { return mNumOpenCell; }
		void getLastCheckPos(int& cx, int& cy)
		{
			cx = mLastCheckPos.x;
			cy = mLastCheckPos.y;
		}

		void setRule(IMineMap& map);
		void refreshInformation(IMineMap& map, bool beRestart);
		void print();
		bool setepSolve() /*throw ( HookException )*/;
		void enableMarkFlag(bool beE) { mMarkFlagEnable = beE; }

		enum CellFlag
		{
			eCheck = 1 << 0,
		};
		struct CellData
		{
			short  number;
			short  numBomb;
			short  numSolved;
			short  numNeighbor;
			short  idxProb; //ProbInfo
			short  idxCT;   // Constraint
			unsigned flag;
		};

		enum SolveState
		{
			eNoSolve,
			eLogicSolve,
			eProbSolve,
		};

		CellData const& getCellData(int idx) const { return mCellData[idx]; }
		bool checkPos(int cx, int cy);
		void openCell(int cx, int cy);

		SolveState getSolveState() { return mSolveState; }

		struct ProbInfo
		{
			float   prob;
			short   idx;
			short   num;
			short   cellCT[8];
		};

		struct ConstraintInfo
		{
			short  num;
			short  cell[8];
		};

		ProbInfo const* getProbInfo(CellData const& data)
		{
			if( data.idxProb == -1 )
				return NULL;
			return &mCacheProbs[data.idxProb];
		}
		ProbInfo const& getOtherProbInfo() { return mCacheProbs.front(); }

		void  getPorbSelect(int& cx, int& cy)
		{
			cx = mPorbSelect.x; cy = mPorbSelect.y;
		}

	private:

		struct CellPos
		{
			short idx;
			short x, y;
		};

		void addCheckPos(CellPos const& cp);
		void addCellInformation(CellPos const& cp, int num);

		void      restoreCell();
		CellData& getCellData(CellPos const& cp) { return mCellData[cp.idx]; }
		int       getNeighbourNum(int cx, int cy);
		int       getIndex(int cx, int cy) { return cx + mCellSizeX * cy; }
		int       calcIndex(CellPos& cp) { return cp.idx = cp.x + mCellSizeX * cp.y; }
		bool      isVaildRange(int cx, int cy)
		{
			return 0 <= cx  && cx < mCellSizeX &&
				0 <= cy  && cy < mCellSizeY;
		}

		struct HiddenParam
		{
			CellPos const* lockPos;
			int numLock;
			int numOverlap;
			int numBomb;
		};

		void markCell(CellPos const& cp);
		void openCell(CellPos const& cp);
		void scanCell(CellPos const& cp);
		void scanCellInternal(CellPos const& cp, int num);

		bool solveLogic(CellPos const& cp);
		bool solveProb();

		bool execUniqueMethod(CellPos const& cp);
		bool execBombClearMethod(CellPos const& cp);
		bool execHiddenBombMethod(CellPos const& cp, int skipDir, HiddenParam& param);
		bool execShareCellMethod(CellPos const& cp, CellPos const& ncp, int dir);

		bool execHiddenBombInternal(CellPos const& cp, CellPos const& ncp, int ndir, HiddenParam &param);
		bool execShareCellInternal(CellPos const& cp, CellPos const& ncp, CellPos const sharePos[], int numShare, int dir, HiddenParam* param);

		bool makeNeighborPos(CellPos const& cp, CellPos& ncp, int dir);
		bool makeGapPos(CellPos const& cp, CellPos& ncp, int dir);
		bool makeCellPos(CellPos& cp, int cx, int cy);

		void generateCellProb();
		void geerateCellProbConstraint();
		void geerateCellProbSimple();

		void updateOtherPropInfo(float accProb);
		int  calcCellProb(CellPos const& cp, ProbInfo &info);

		int  getShareFreeNeighbor(CellPos const& p1, CellPos const& p2, CellPos share[]);

		static bool isPosIncluded(CellPos const cpList[], int num, CellPos const& cp);
		static bool testNeighbor(CellPos const& cp1, CellPos const& cp2);

		unsigned mSettingFlag;
		bool     mMarkFlagEnable;

		int  mNumSolvedBomb;
		int  mNumOpenCell;

		int  mNumTotalBomb;
		int  mCellSizeX;
		int  mCellSizeY;
		std::vector< CellData > mCellData;
		std::vector< CellPos >  mCheckList;

		ProbInfo& fetchProbInfo(CellPos const& cp);
		void      releaseProbInfo(CellPos const& cp);

		std::vector< ProbInfo >   mCacheProbs;
		std::vector< CellData* >  mSortedProbCells;
		std::vector< CellData* >  mUnknownProbCells;

		int        mIdxFreeProb;

		CellPos    mLastCheckPos;
		SolveState mSolveState;
		EGameState  mGameState;
		CellPos    mPorbSelect;
	};

}//namespace Mine

#endif // ExpSolveStrategy_h__