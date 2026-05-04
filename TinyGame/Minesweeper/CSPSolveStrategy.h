#ifndef CSPSolveStrategy_h__
#define CSPSolveStrategy_h__

#include "GameInterface.h"

namespace Mine
{
	int const MaxVariableNum = 8;

	struct CellData;
	class ConstraintGroup;

	class Constraint
	{
	public:
		void init();
		bool isCoupleWith(Constraint const& c) const;
		void addVarrible(CellData& var);

		int       constant;
		int       numVar;
		CellData* variables[MaxVariableNum];
	};

	struct CellData
	{
		int x, y;
		int x1, x2, y1, y2;
		int number;
		union
		{
			Constraint ct;
			int varIndex;
		};

		ConstraintGroup* group;
	};

	typedef TArray< Constraint* > ContraintVec;

	class ConstraintGroup
	{
	public:
		ConstraintGroup();

		bool addConstraint(Constraint& ct);
		void addConstraint(ContraintVec const& ctVec);
		void clear();

		bool search();

		TArray< Constraint* > constraints;
		TArray< CellData* > variables;

		struct SimplifiedConstraint
		{
			uint64 mask;
			int constant;
		};
		TArray< SimplifiedConstraint > simplifiedConstraints;
		TArray< int > mineCount;
		TArray< double > solutionCountByMineNum;
		TArray< double > variableMineCountByMineNum;
		int variableMineCountStride;
		int numSolution;

		enum
		{
			eBuildNode        = 1 << 0,
			eCheckSimple      = 1 << 1,
			eSearchResultDone = 1 << 2,
			eCheckSearchSolve = 1 << 3,
		};

		unsigned flag;

	private:
		void buildVariables();
		void buildSimplifiedConstraints();
		bool useSimplifiedConstraints() const { return !simplifiedConstraints.empty(); }
		bool checkPartial(int level, TArray< int > const& assign) const;
		bool checkComplete(TArray< int > const& assign) const;
		bool checkOriginalComplete(TArray< int > const& assign) const;
		bool searchInternal(int level, TArray< int >& assign);
	};

	class CSPSolveStrategy : public ISolveStrategy
	{
	public:
		CSPSolveStrategy();
		~CSPSolveStrategy();

		virtual void restart(IMineControl& control);
		virtual void loadMap(IMineControl& control);
		virtual bool solveStep();

		float getCellProbability(int x, int y) const;
		void getLastGuessInfo(int& x, int& y, float& prob) const;
		int getCellConstraintGroupIndex(int x, int y) const;

		void restoreData(IMineControl& control);
		void buildConstraint(CellData& cell, ConstraintGroup* group);

		void markCell(CellData& cell, Constraint* ct);
		void openCell(CellData& cell, Constraint* ct);
		void updateSolveCell(CellData& cell, int num, Constraint* ct, bool beWait);
		void scanCell(CellData& cell, Constraint* ct, bool beWait);

		struct SolveInfo
		{
			CellData* cell;
			ConstraintGroup* linkGroup;
			int number;
		};

	private:
		int getIndex(int x, int y) const { return x + y * mSizeX; }
		void clearGroups();
		void clearProbabilityDisplay();
		void syncMapFromControl();
		void rebuildConstraints();
		void updateDirtyConstraints();
		void buildGroups(ContraintVec const& constraints);
		void markDirtyCell(CellData& cell);
		void markDirtyAround(CellData& cell);
		CellData* findCellByConstraint(Constraint const& ct);
		ConstraintGroup* findGroupByConstraint(Constraint const& ct) const;
		void addSolvedCell(CellData& cell, int number, ConstraintGroup* group = nullptr);
		bool popSolvedCell();
		bool solveBySimpleConstraint();
		bool solveByGlobalProbability(bool bAllowGuess);
		bool hasPendingCell(CellData const& cell, int number) const;
		int countKnownNeighborCells(CellData const& cell, int dist) const;
		void dumpGuessLocalDistribution(CellData const& cell, double globalProb) const;

		TArray< SolveInfo >        mSolvedCell;
		TArray< CellData* >        mDirtyCells;
		TArray< CellData >         mData;
		TArray< ConstraintGroup* > mCTGroup;
		TArray< float >            mCellProb;
		IMineControl* mControl;
		int       mNumSolvedCell;
		int       mSizeX;
		int       mSizeY;
		bool      mbShowProbability;
		int       mLastGuessX;
		int       mLastGuessY;
		float     mLastGuessProb;
	};

}//namespace Mine

#endif // CSPSolveStrategy_h__
