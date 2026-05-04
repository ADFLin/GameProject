#include "CSPSolveStrategy.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <limits>

namespace Mine
{
	namespace
	{
		int const CSP_UNASSIGNED = -1;

		double Combination(int n, int k)
		{
			if( k < 0 || k > n )
				return 0.0;

			if( k > n - k )
				k = n - k;

			double result = 1.0;
			for( int i = 1; i <= k; ++i )
				result = result * double(n - k + i) / double(i);

			return result;
		}
	}

	void Constraint::init()
	{
		constant = 0;
		numVar = 0;
	}

	bool Constraint::isCoupleWith(Constraint const& c) const
	{
		for( int i = 0; i < numVar; ++i )
		{
			for( int j = 0; j < c.numVar; ++j )
			{
				if( variables[i] == c.variables[j] )
					return true;
			}
		}
		return false;
	}

	void Constraint::addVarrible(CellData& var)
	{
		assert(var.number == CV_UNPROBLED);
		assert(numVar < MaxVariableNum);
		variables[numVar] = &var;
		++numVar;
	}

	ConstraintGroup::ConstraintGroup()
		:variableMineCountStride(0)
		,numSolution(0)
		,flag(eBuildNode)
	{
	}

	bool ConstraintGroup::addConstraint(Constraint& ct)
	{
		if( ct.numVar == 0 )
			return false;

		if( std::find(constraints.begin(), constraints.end(), &ct) == constraints.end() )
			constraints.push_back(&ct);

		flag |= eBuildNode;
		flag &= ~(eCheckSimple | eSearchResultDone | eCheckSearchSolve);
		return true;
	}

	void ConstraintGroup::addConstraint(ContraintVec const& ctVec)
	{
		for( ContraintVec::const_iterator iter = ctVec.begin(); iter != ctVec.end(); ++iter )
		{
			if( std::find(constraints.begin(), constraints.end(), *iter) == constraints.end() )
				constraints.push_back(*iter);
		}
		flag |= eBuildNode;
		flag &= ~(eCheckSimple | eSearchResultDone | eCheckSearchSolve);
	}

	void ConstraintGroup::clear()
	{
		constraints.clear();
		variables.clear();
		simplifiedConstraints.clear();
		mineCount.clear();
		solutionCountByMineNum.clear();
		variableMineCountByMineNum.clear();
		variableMineCountStride = 0;
		numSolution = 0;
		flag = eBuildNode;
	}

	void ConstraintGroup::buildVariables()
	{
		variables.clear();
		for( size_t i = 0; i < constraints.size(); ++i )
		{
			Constraint& ct = *constraints[i];
			for( int n = 0; n < ct.numVar; ++n )
			{
				if( std::find(variables.begin(), variables.end(), ct.variables[n]) == variables.end() )
					variables.push_back(ct.variables[n]);
			}
		}

		for( size_t i = 0; i < variables.size(); ++i )
			variables[i]->varIndex = int(i);

		for( size_t i = 0; i < constraints.size(); ++i )
		{
			Constraint& ct = *constraints[i];
			for( int n = 0; n < ct.numVar; ++n )
			{
				int idx = ct.variables[n]->varIndex;
				assert(0 <= idx && idx < int(variables.size()));
			}
		}

		mineCount.assign(variables.size(), 0);
		solutionCountByMineNum.assign(variables.size() + 1, 0.0);
		variableMineCountStride = int(variables.size()) + 1;
		variableMineCountByMineNum.assign(variables.size() * variableMineCountStride, 0.0);
		buildSimplifiedConstraints();
		flag &= ~(eBuildNode | eSearchResultDone | eCheckSearchSolve);
	}

	void ConstraintGroup::buildSimplifiedConstraints()
	{
		simplifiedConstraints.clear();

		if( variables.empty() || variables.size() > 64 )
			return;

		for( size_t i = 0; i < constraints.size(); ++i )
		{
			Constraint const& ct = *constraints[i];
			uint64 mask = 0;
			for( int n = 0; n < ct.numVar; ++n )
			{
				int idx = ct.variables[n]->varIndex;
				assert(0 <= idx && idx < 64);
				mask |= uint64(1) << idx;
			}

			if( mask == 0 )
				continue;

			SimplifiedConstraint simplified;
			simplified.mask = mask;
			simplified.constant = ct.constant;
			simplifiedConstraints.push_back(simplified);
		}

		bool bChanged = true;
		while( bChanged )
		{
			bChanged = false;

			for( size_t i = 0; i < simplifiedConstraints.size(); ++i )
			{
				for( size_t j = 0; j < simplifiedConstraints.size(); ++j )
				{
					if( i == j )
						continue;

					uint64 maskI = simplifiedConstraints[i].mask;
					uint64 maskJ = simplifiedConstraints[j].mask;
					if( maskI == 0 || maskJ == 0 )
						continue;

					if( (maskI & maskJ) == maskJ )
					{
						simplifiedConstraints[i].mask = maskI & ~maskJ;
						simplifiedConstraints[i].constant -= simplifiedConstraints[j].constant;
						bChanged = true;
					}
				}
			}

			for (int i = int(simplifiedConstraints.size()) - 1; i >= 0; --i)
			{
				if (simplifiedConstraints[i].mask == 0 && simplifiedConstraints[i].constant == 0)
				{
					simplifiedConstraints.removeIndex(i);
					bChanged = true;
				}
			}

		}
	}

	bool ConstraintGroup::checkPartial(int level, TArray< int > const& assign) const
	{
		for( size_t i = 0; i < constraints.size(); ++i )
		{
			Constraint const& ct = *constraints[i];
			int numMine = 0;
			int numUnknown = 0;

			for( int n = 0; n < ct.numVar; ++n )
			{
				int idx = ct.variables[n]->varIndex;
				assert(idx >= 0);
				if( assign[idx] != CSP_UNASSIGNED )
					numMine += assign[idx];
				else
					++numUnknown;
			}

			if( numMine > ct.constant )
				return false;
			if( numMine + numUnknown < ct.constant )
				return false;
		}

		return true;
	}

	bool ConstraintGroup::checkComplete(TArray< int > const& assign) const
	{
		return checkOriginalComplete(assign);
	}

	bool ConstraintGroup::checkOriginalComplete(TArray< int > const& assign) const
	{
		for( size_t i = 0; i < constraints.size(); ++i )
		{
			Constraint const& ct = *constraints[i];
			int numMine = 0;

			for( int n = 0; n < ct.numVar; ++n )
			{
				int idx = ct.variables[n]->varIndex;
				assert(idx >= 0);
				numMine += assign[idx];
			}

			if( numMine != ct.constant )
				return false;
		}

		return true;
	}

	bool ConstraintGroup::searchInternal(int level, TArray< int >& assign)
	{
		if( level == int(variables.size()) )
		{
			if( !checkComplete(assign) )
				return false;

			++numSolution;
			int numMine = 0;
			for( size_t i = 0; i < assign.size(); ++i )
			{
				mineCount[i] += assign[i];
				numMine += assign[i];
			}

			solutionCountByMineNum[numMine] += 1.0;
			for( size_t i = 0; i < assign.size(); ++i )
			{
				if( assign[i] )
					variableMineCountByMineNum[i * variableMineCountStride + numMine] += 1.0;
			}

			return true;
		}

		int idxSelect = -1;
		int bestUnknown = std::numeric_limits< int >::max();
		int bestDegree = -1;
		for( size_t i = 0; i < variables.size(); ++i )
		{
			if( assign[i] != CSP_UNASSIGNED )
				continue;

			int degree = 0;
			int unknown = std::numeric_limits< int >::max();
			if( useSimplifiedConstraints() )
			{
				for( size_t n = 0; n < simplifiedConstraints.size(); ++n )
				{
					uint64 mask = simplifiedConstraints[n].mask;
					if( (mask & (uint64(1) << i)) == 0 )
						continue;

					++degree;
					int numUnknown = 0;
					for( size_t idx = 0; idx < variables.size(); ++idx )
					{
						if( (mask & (uint64(1) << idx)) && assign[idx] == CSP_UNASSIGNED )
							++numUnknown;
					}
					unknown = std::min(unknown, numUnknown);
				}
			}
			else
			{
				for( size_t n = 0; n < constraints.size(); ++n )
				{
					Constraint const& ct = *constraints[n];
					bool bFindVariable = false;
					for( int v = 0; v < ct.numVar; ++v )
					{
						if( ct.variables[v]->varIndex == int(i) )
						{
							bFindVariable = true;
							break;
						}
					}
					if( !bFindVariable )
						continue;

					++degree;
					int numUnknown = 0;
					for( int v = 0; v < ct.numVar; ++v )
					{
						int idx = ct.variables[v]->varIndex;
						assert(idx >= 0);
						if( assign[idx] == CSP_UNASSIGNED )
							++numUnknown;
					}
					unknown = std::min(unknown, numUnknown);
				}
			}

			if( unknown < bestUnknown || (unknown == bestUnknown && degree > bestDegree) )
			{
				idxSelect = int(i);
				bestUnknown = unknown;
				bestDegree = degree;
			}
		}

		assert(idxSelect != -1);

		bool result = false;
		for( int value = 0; value <= 1; ++value )
		{
			assign[idxSelect] = value;
			if( checkPartial(level + 1, assign) )
				result |= searchInternal(level + 1, assign);
			assign[idxSelect] = CSP_UNASSIGNED;
		}

		return result;
	}

	bool ConstraintGroup::search()
	{
		if( flag & eSearchResultDone )
			return numSolution > 0;

		if( flag & eBuildNode )
			buildVariables();

		numSolution = 0;
		std::fill(mineCount.begin(), mineCount.end(), 0);
		std::fill(solutionCountByMineNum.begin(), solutionCountByMineNum.end(), 0.0);
		std::fill(variableMineCountByMineNum.begin(), variableMineCountByMineNum.end(), 0.0);

		if( variables.empty() )
		{
			flag |= eSearchResultDone;
			return false;
		}

		TArray< int > assign(variables.size(), CSP_UNASSIGNED);
		bool result = searchInternal(0, assign);
		flag |= eSearchResultDone;
		return result;
	}

	CSPSolveStrategy::CSPSolveStrategy()
		:mControl(nullptr)
		,mNumSolvedCell(0)
		,mSizeX(0)
		,mSizeY(0)
		,mbShowProbability(false)
		,mLastGuessX(-1)
		,mLastGuessY(-1)
		,mLastGuessProb(-1.0f)
	{
	}

	CSPSolveStrategy::~CSPSolveStrategy()
	{
		clearGroups();
	}

	void CSPSolveStrategy::clearGroups()
	{
		for( size_t i = 0; i < mCTGroup.size(); ++i )
			delete mCTGroup[i];

		mCTGroup.clear();
	}

	void CSPSolveStrategy::restoreData(IMineControl& control)
	{
		mControl = &control;
		mSizeX = mControl->getSizeX();
		mSizeY = mControl->getSizeY();
		mNumSolvedCell = 0;
		mSolvedCell.clear();
		mDirtyCells.clear();
		clearGroups();

		mData.resize(mSizeX * mSizeY);
		mCellProb.assign(mSizeX * mSizeY, -1.0f);
		clearProbabilityDisplay();

		int xMax = mSizeX - 1;
		int yMax = mSizeY - 1;
		for( int y = 0; y < mSizeY; ++y )
		{
			for( int x = 0; x < mSizeX; ++x )
			{
				CellData& data = mData[getIndex(x, y)];
				data.x = x;
				data.y = y;
				data.x1 = std::max(0, x - 1);
				data.y1 = std::max(0, y - 1);
				data.x2 = std::min(xMax, x + 1);
				data.y2 = std::min(yMax, y + 1);
				data.number = CV_UNPROBLED;
				data.group = nullptr;
				data.ct.init();
			}
		}
	}

	float CSPSolveStrategy::getCellProbability(int x, int y) const
	{
		if( !mbShowProbability )
			return -1.0f;

		if( x < 0 || x >= mSizeX || y < 0 || y >= mSizeY )
			return -1.0f;

		int index = x + y * mSizeX;
		if( index < 0 || index >= int(mCellProb.size()) )
			return -1.0f;

		return mCellProb[index];
	}

	void CSPSolveStrategy::clearProbabilityDisplay()
	{
		std::fill(mCellProb.begin(), mCellProb.end(), -1.0f);
		mbShowProbability = false;
		mLastGuessX = -1;
		mLastGuessY = -1;
		mLastGuessProb = -1.0f;
	}

	void CSPSolveStrategy::getLastGuessInfo(int& x, int& y, float& prob) const
	{
		x = mLastGuessX;
		y = mLastGuessY;
		prob = mLastGuessProb;
	}

	int CSPSolveStrategy::getCellConstraintGroupIndex(int x, int y) const
	{
		if( x < 0 || x >= mSizeX || y < 0 || y >= mSizeY )
			return -1;

		ConstraintGroup* group = mData[getIndex(x, y)].group;
		if( group == nullptr )
			return -1;

		for( size_t i = 0; i < mCTGroup.size(); ++i )
		{
			if( mCTGroup[i] == group )
				return int(i);
		}

		return -1;
	}

	void CSPSolveStrategy::restart(IMineControl& control)
	{
		restoreData(control);
		rebuildConstraints();
	}

	void CSPSolveStrategy::loadMap(IMineControl& control)
	{
		restoreData(control);

		for( size_t i = 0; i < mData.size(); ++i )
		{
			CellData& cell = mData[i];
			cell.number = control.lookCell(cell.x, cell.y, false);
			if( cell.number != CV_UNPROBLED )
				++mNumSolvedCell;
		}

		rebuildConstraints();
	}

	void CSPSolveStrategy::buildGroups(ContraintVec const& constraints)
	{
		TArray< bool > used(constraints.size(), false);
		for( size_t i = 0; i < constraints.size(); ++i )
		{
			if( used[i] )
				continue;

			ConstraintGroup* group = new ConstraintGroup();
			mCTGroup.push_back(group);

			used[i] = true;
			group->addConstraint(*constraints[i]);

			bool changed = true;
			while( changed )
			{
				changed = false;
				for( size_t n = 0; n < constraints.size(); ++n )
				{
					if( used[n] )
						continue;

					for( size_t k = 0; k < group->constraints.size(); ++k )
					{
						if( constraints[n]->isCoupleWith(*group->constraints[k]) )
						{
							used[n] = true;
							group->addConstraint(*constraints[n]);
							changed = true;
							break;
						}
					}
				}
			}

			for( size_t n = 0; n < group->constraints.size(); ++n )
			{
				Constraint& ct = *group->constraints[n];
				for( int idx = 0; idx < ct.numVar; ++idx )
					ct.variables[idx]->group = group;
			}
		}
	}

	void CSPSolveStrategy::rebuildConstraints()
	{
		clearGroups();

		for( size_t i = 0; i < mData.size(); ++i )
			mData[i].group = nullptr;

		ContraintVec constraints;
		for( size_t i = 0; i < mData.size(); ++i )
		{
			CellData& cell = mData[i];
			if( cell.number > 0 )
			{
				buildConstraint(cell, nullptr);
				if( cell.ct.numVar > 0 )
					constraints.push_back(&cell.ct);
			}
		}

		buildGroups(constraints);
		mDirtyCells.clear();
	}

	CellData* CSPSolveStrategy::findCellByConstraint(Constraint const& ct)
	{
		for( size_t i = 0; i < mData.size(); ++i )
		{
			if( &mData[i].ct == &ct )
				return &mData[i];
		}
		return nullptr;
	}

	ConstraintGroup* CSPSolveStrategy::findGroupByConstraint(Constraint const& ct) const
	{
		for( size_t i = 0; i < mCTGroup.size(); ++i )
		{
			ConstraintGroup* group = mCTGroup[i];
			if( std::find(group->constraints.begin(), group->constraints.end(), &ct) != group->constraints.end() )
				return group;
		}
		return nullptr;
	}

	void CSPSolveStrategy::markDirtyCell(CellData& cell)
	{
		mDirtyCells.addUnique(&cell);
	}

	void CSPSolveStrategy::markDirtyAround(CellData& cell)
	{
		markDirtyCell(cell);
		for( int y = cell.y1; y <= cell.y2; ++y )
		{
			for( int x = cell.x1; x <= cell.x2; ++x )
				markDirtyCell(mData[getIndex(x, y)]);
		}
	}

	void CSPSolveStrategy::updateDirtyConstraints()
	{
		if( mDirtyCells.empty() )
			return;

		TArray< CellData* > seedCells;
		TArray< ConstraintGroup* > dirtyGroups;

		for( size_t i = 0; i < mDirtyCells.size(); ++i )
		{
			CellData* dirtyCell = mDirtyCells[i];
			if( dirtyCell == nullptr )
				continue;

			if( dirtyCell->group )
				dirtyGroups.addUnique(dirtyCell->group);

			for( int y = dirtyCell->y1; y <= dirtyCell->y2; ++y )
			{
				for( int x = dirtyCell->x1; x <= dirtyCell->x2; ++x )
				{
					CellData& cell = mData[getIndex(x, y)];
					if( cell.group )
						dirtyGroups.addUnique(cell.group);

					if( cell.number > 0 )
					{
						seedCells.addUnique(&cell);
						if( ConstraintGroup* group = findGroupByConstraint(cell.ct) )
							dirtyGroups.addUnique(group);
					}
				}
			}
		}

		size_t numProcessedGroup = 0;
		size_t numProcessedSeed = 0;
		while( numProcessedGroup < dirtyGroups.size() || numProcessedSeed < seedCells.size() )
		{
			for( ; numProcessedGroup < dirtyGroups.size(); ++numProcessedGroup )
			{
				ConstraintGroup* group = dirtyGroups[numProcessedGroup];
				for( size_t n = 0; n < group->constraints.size(); ++n )
				{
					if( CellData* cell = findCellByConstraint(*group->constraints[n]) )
						seedCells.addUnique(cell);
				}
			}

			for( ; numProcessedSeed < seedCells.size(); ++numProcessedSeed )
			{
				CellData* cell = seedCells[numProcessedSeed];
				if( cell == nullptr || cell->number <= 0 )
					continue;

				buildConstraint(*cell, nullptr);
				for( int n = 0; n < cell->ct.numVar; ++n )
				{
					if( cell->ct.variables[n]->group )
						dirtyGroups.addUnique(cell->ct.variables[n]->group);
				}
			}
		}

		for( int i = int(mCTGroup.size()) - 1; i >= 0; --i )
		{
			ConstraintGroup* group = mCTGroup[i];
			if( std::find(dirtyGroups.begin(), dirtyGroups.end(), group) == dirtyGroups.end() )
				continue;

			for( size_t n = 0; n < mData.size(); ++n )
			{
				if( mData[n].group == group )
					mData[n].group = nullptr;
			}

			mCTGroup.removeIndex(i);
			delete group;
		}

		ContraintVec constraints;
		for( size_t i = 0; i < seedCells.size(); ++i )
		{
			CellData* cell = seedCells[i];
			if( cell == nullptr || cell->number <= 0 )
				continue;

			buildConstraint(*cell, nullptr);
			if( cell->ct.numVar > 0 )
				constraints.addUnique(&cell->ct);
		}

		buildGroups(constraints);
		mDirtyCells.clear();
	}

	void CSPSolveStrategy::syncMapFromControl()
	{
		if( mControl == nullptr )
			return;

		mNumSolvedCell = 0;
		for( size_t i = 0; i < mData.size(); ++i )
		{
			CellData& cell = mData[i];
			if( cell.number != CV_UNPROBLED )
			{
				++mNumSolvedCell;
				continue;
			}

			cell.number = mControl->lookCell(cell.x, cell.y, false);
			if( cell.number != CV_UNPROBLED )
			{
				++mNumSolvedCell;
				markDirtyAround(cell);
			}
		}
	}

	void CSPSolveStrategy::buildConstraint(CellData& cell, ConstraintGroup* group)
	{
		cell.ct.init();
		cell.ct.constant = cell.number;

		for( int y = cell.y1; y <= cell.y2; ++y )
		{
			for( int x = cell.x1; x <= cell.x2; ++x )
			{
				if( x == cell.x && y == cell.y )
					continue;

				CellData& neighbor = mData[getIndex(x, y)];
				if( neighbor.number == CV_FLAG )
					--cell.ct.constant;
				else if( neighbor.number == CV_UNPROBLED )
					cell.ct.addVarrible(neighbor);
			}
		}

		if( cell.ct.numVar == 0 )
			return;

		if( group )
		{
			group->addConstraint(cell.ct);
			cell.group = group;

			for( int i = 0; i < cell.ct.numVar; ++i )
				cell.ct.variables[i]->group = group;
		}
	}

	bool CSPSolveStrategy::hasPendingCell(CellData const& cell, int number) const
	{
		for( size_t i = 0; i < mSolvedCell.size(); ++i )
		{
			if( mSolvedCell[i].cell == &cell )
				return true;
		}
		return false;
	}

	int CSPSolveStrategy::countKnownNeighborCells(CellData const& cell, int dist) const
	{
		int result = 0;
		for( int dy = -dist; dy <= dist; ++dy )
		{
			int distX = dist - Math::Abs(dy);
			for( int dx = -distX; dx <= distX; ++dx )
			{
				if( dx == 0 && dy == 0 )
					continue;

				int const x = cell.x + dx;
				int const y = cell.y + dy;
				if( x < 0 || x >= mSizeX || y < 0 || y >= mSizeY )
					continue;

				if( mData[getIndex(x, y)].number != CV_UNPROBLED )
					++result;
			}
		}

		return result;
	}

	void CSPSolveStrategy::dumpGuessLocalDistribution(CellData const& cell, double globalProb) const
	{
		ConstraintGroup* group = cell.group;
		if( group == nullptr )
		{
			LogMsg("CSP Guess (%d,%d) global = %.2f%% , unconstrained cell", cell.x, cell.y, 100.0 * globalProb);
			return;
		}

		int varIndex = -1;
		for( size_t i = 0; i < group->variables.size(); ++i )
		{
			if( group->variables[i] == &cell )
			{
				varIndex = int(i);
				break;
			}
		}

		if( varIndex == -1 || group->numSolution <= 0 )
		{
			LogMsg("CSP Guess (%d,%d) global = %.2f%% , local distribution unavailable", cell.x, cell.y, 100.0 * globalProb);
			return;
		}

		double const localProb = double(group->mineCount[varIndex]) / double(group->numSolution);
		LogMsg("CSP Guess (%d,%d) global = %.2f%% , local = %.2f%% , group vars = %u , solutions = %d",
			cell.x, cell.y, 100.0 * globalProb, 100.0 * localProb, unsigned(group->variables.size()), group->numSolution);

		for( int mineNum = 0; mineNum < int(group->solutionCountByMineNum.size()); ++mineNum )
		{
			double const solutionCount = group->solutionCountByMineNum[mineNum];
			if( solutionCount == 0.0 )
				continue;

			double const varMineCount = group->variableMineCountByMineNum[varIndex * group->variableMineCountStride + mineNum];
			LogMsg("  groupMine = %d : solutions = %.0f , guessMine = %.0f", mineNum, solutionCount, varMineCount);
		}

		for( size_t i = 0; i < group->variables.size(); ++i )
		{
			CellData const* var = group->variables[i];
			double const prob = double(group->mineCount[i]) / double(group->numSolution);
			LogMsg("  var[%u] (%d,%d) local = %.2f%% mineCount = %d",
				unsigned(i), var->x, var->y, 100.0 * prob, group->mineCount[i]);
		}
	}

	void CSPSolveStrategy::addSolvedCell(CellData& cell, int number, ConstraintGroup* group)
	{
		if( cell.number != CV_UNPROBLED )
			return;
		if( hasPendingCell(cell, number) )
			return;

		SolveInfo info;
		info.cell = &cell;
		info.linkGroup = group;
		info.number = number;
		mSolvedCell.push_back(info);
	}

	bool CSPSolveStrategy::popSolvedCell()
	{
		while( !mSolvedCell.empty() )
		{
			SolveInfo info = mSolvedCell.back();
			mSolvedCell.pop_back();

			if( info.cell == nullptr )
				continue;

			if( mControl )
			{
				int const state = mControl->lookCell(info.cell->x, info.cell->y, false);
				if( info.cell->number == CV_UNPROBLED && state != CV_UNPROBLED )
				{
					++mNumSolvedCell;
					markDirtyAround(*info.cell);
				}

				info.cell->number = state;
			}

			if( info.cell->number != CV_UNPROBLED )
				continue;

			if( info.number == CV_FLAG )
				markCell(*info.cell, nullptr);
			else
				openCell(*info.cell, nullptr);

			return true;
		}

		return false;
	}

	void CSPSolveStrategy::markCell(CellData& cell, Constraint* ct)
	{
		if( cell.number != CV_UNPROBLED )
			return;

		if( mControl->markCell(cell.x, cell.y) )
		{
			cell.number = CV_FLAG;
			++mNumSolvedCell;
			markDirtyAround(cell);
		}
	}

	void CSPSolveStrategy::openCell(CellData& cell, Constraint* ct)
	{
		if( cell.number != CV_UNPROBLED )
			return;

		int num = mControl->openCell(cell.x, cell.y);
		if( num >= 0 )
			updateSolveCell(cell, num, ct, true);
	}

	void CSPSolveStrategy::updateSolveCell(CellData& cell, int num, Constraint* ct, bool beWait)
	{
		if( cell.number == CV_UNPROBLED )
		{
			++mNumSolvedCell;
			markDirtyAround(cell);
		}

		cell.number = num;

		if( num == 0 )
		{
			for( int y = cell.y1; y <= cell.y2; ++y )
			{
				for( int x = cell.x1; x <= cell.x2; ++x )
					scanCell(mData[getIndex(x, y)], ct, beWait);
			}
		}
	}

	void CSPSolveStrategy::scanCell(CellData& cell, Constraint* ct, bool beWait)
	{
		if( cell.number != CV_UNPROBLED )
			return;

		int num = mControl->lookCell(cell.x, cell.y, beWait);
		if( num >= 0 )
			updateSolveCell(cell, num, ct, beWait);
	}

	bool CSPSolveStrategy::solveBySimpleConstraint()
	{
		bool result = false;

		for( size_t i = 0; i < mCTGroup.size(); ++i )
		{
			ConstraintGroup* group = mCTGroup[i];
			if( group->flag & ConstraintGroup::eCheckSimple )
				continue;

			for( size_t n = 0; n < group->constraints.size(); ++n )
			{
				Constraint& ct = *group->constraints[n];
				if( ct.constant == 0 )
				{
					for( int idx = 0; idx < ct.numVar; ++idx )
					{
						addSolvedCell(*ct.variables[idx], 0, group);
						result = true;
					}
				}
				else if( ct.constant == ct.numVar )
				{
					for( int idx = 0; idx < ct.numVar; ++idx )
					{
						addSolvedCell(*ct.variables[idx], CV_FLAG, group);
						result = true;
					}
				}
			}

			group->flag |= ConstraintGroup::eCheckSimple;
		}

		return result;
	}

	bool CSPSolveStrategy::solveByGlobalProbability(bool bAllowGuess)
	{
		clearProbabilityDisplay();

		int numMarkedBomb = 0;
		TArray< CellData* > unconstrainedCells;
		for( size_t i = 0; i < mData.size(); ++i )
		{
			CellData& cell = mData[i];
			if( cell.number == CV_FLAG )
				++numMarkedBomb;
			else if( cell.number == CV_UNPROBLED && cell.group == nullptr )
				unconstrainedCells.push_back(&cell);
		}

		int const numRemainingBomb = mControl->getBombNum() - numMarkedBomb;
		if( numRemainingBomb < 0 )
			return false;

		TArray< ConstraintGroup* > groups;
		bool result = false;
		for( size_t i = 0; i < mCTGroup.size(); ++i )
		{
			ConstraintGroup* group = mCTGroup[i];
			if( group->search() && group->numSolution > 0 )
			{
				if( (group->flag & ConstraintGroup::eCheckSearchSolve) == 0 )
				{
					for( size_t varIndex = 0; varIndex < group->variables.size(); ++varIndex )
					{
						if( group->mineCount[varIndex] == 0 )
						{
							addSolvedCell(*group->variables[varIndex], 0, group);
							result = true;
						}
						else if( group->mineCount[varIndex] == group->numSolution )
						{
							addSolvedCell(*group->variables[varIndex], CV_FLAG, group);
							result = true;
						}
					}

					group->flag |= ConstraintGroup::eCheckSearchSolve;
				}

				groups.push_back(group);
			}
		}

		if( result )
			return true;

		int const numGroup = int(groups.size());
		int const dpStride = numRemainingBomb + 1;
		TArray< double > prefix((numGroup + 1) * dpStride, 0.0);
		TArray< double > suffix((numGroup + 1) * dpStride, 0.0);

		prefix[0] = 1.0;
		for( int i = 0; i < numGroup; ++i )
		{
			ConstraintGroup const& group = *groups[i];
			for( int mineUsed = 0; mineUsed <= numRemainingBomb; ++mineUsed )
			{
				double const prefixWeight = prefix[i * dpStride + mineUsed];
				if( prefixWeight == 0.0 )
					continue;

				for( int groupMine = 0; groupMine < int(group.solutionCountByMineNum.size()); ++groupMine )
				{
					if( mineUsed + groupMine > numRemainingBomb )
						break;
					prefix[(i + 1) * dpStride + mineUsed + groupMine] += prefixWeight * group.solutionCountByMineNum[groupMine];
				}
			}
		}

		suffix[numGroup * dpStride] = 1.0;
		for( int i = numGroup - 1; i >= 0; --i )
		{
			ConstraintGroup const& group = *groups[i];
			for( int mineUsed = 0; mineUsed <= numRemainingBomb; ++mineUsed )
			{
				double const suffixWeight = suffix[(i + 1) * dpStride + mineUsed];
				if( suffixWeight == 0.0 )
					continue;

				for( int groupMine = 0; groupMine < int(group.solutionCountByMineNum.size()); ++groupMine )
				{
					if( mineUsed + groupMine > numRemainingBomb )
						break;
					suffix[i * dpStride + mineUsed + groupMine] += suffixWeight * group.solutionCountByMineNum[groupMine];
				}
			}
		}

		int const numUnconstrained = int(unconstrainedCells.size());
		double totalWeight = 0.0;
		for( int frontierMine = 0; frontierMine <= numRemainingBomb; ++frontierMine )
		{
			int unconstrainedMine = numRemainingBomb - frontierMine;
			totalWeight += prefix[numGroup * dpStride + frontierMine] * Combination(numUnconstrained, unconstrainedMine);
		}

		if( totalWeight <= 0.0 )
			return false;

		LogMsg("CSP Global probability: totalBomb = %d , marked = %d , remaining = %d , groups = %d , unconstrained = %d , weight = %.0f",
			mControl->getBombNum(), numMarkedBomb, numRemainingBomb, numGroup, numUnconstrained, totalWeight);

		if( numGroup == 1 )
		{
			ConstraintGroup const& group = *groups[0];
			LogMsg("CSP Single group summary: vars = %u , solutions = %d",
				unsigned(group.variables.size()), group.numSolution);

			for( int groupMine = 0; groupMine < int(group.solutionCountByMineNum.size()); ++groupMine )
			{
				double const solutionCount = group.solutionCountByMineNum[groupMine];
				if( solutionCount == 0.0 )
					continue;

				int const unconstrainedMine = numRemainingBomb - groupMine;
				double const weight = solutionCount * Combination(numUnconstrained, unconstrainedMine);
				LogMsg("  groupMine = %d : localSolutions = %.0f , unconstrainedMine = %d , globalWeight = %.0f%s",
					groupMine, solutionCount, unconstrainedMine, weight, weight > 0.0 ? " *" : "");
			}
		}

		double const eps = 1.0e-9;
		CellData* bestCell = nullptr;
		TArray< CellData* > bestCells;
		double bestProb = std::numeric_limits< double >::max();

		for( int groupIndex = 0; groupIndex < numGroup; ++groupIndex )
		{
			ConstraintGroup& group = *groups[groupIndex];
			for( size_t varIndex = 0; varIndex < group.variables.size(); ++varIndex )
			{
				double mineWeight = 0.0;
				for( int leftMine = 0; leftMine <= numRemainingBomb; ++leftMine )
				{
					double const prefixWeight = prefix[groupIndex * dpStride + leftMine];
					if( prefixWeight == 0.0 )
						continue;

					for( int groupMine = 0; groupMine < group.variableMineCountStride; ++groupMine )
					{
						double const varMineCount = group.variableMineCountByMineNum[varIndex * group.variableMineCountStride + groupMine];
						if( varMineCount == 0.0 || leftMine + groupMine > numRemainingBomb )
							continue;

						for( int rightMine = 0; leftMine + groupMine + rightMine <= numRemainingBomb; ++rightMine )
						{
							double const suffixWeight = suffix[(groupIndex + 1) * dpStride + rightMine];
							if( suffixWeight == 0.0 )
								continue;

							int unconstrainedMine = numRemainingBomb - leftMine - groupMine - rightMine;
							mineWeight += prefixWeight * varMineCount * suffixWeight *
								Combination(numUnconstrained, unconstrainedMine);
						}
					}
				}

				double prob = mineWeight / totalWeight;
				mCellProb[getIndex(group.variables[varIndex]->x, group.variables[varIndex]->y)] = float(prob);
				if( prob <= eps )
				{
					addSolvedCell(*group.variables[varIndex], 0, &group);
					result = true;
				}
				else if( prob >= 1.0 - eps )
				{
					addSolvedCell(*group.variables[varIndex], CV_FLAG, &group);
					result = true;
				}
				else if( bAllowGuess )
				{
					if( prob + eps < bestProb )
					{
						bestProb = prob;
						bestCells.clear();
						bestCells.push_back(group.variables[varIndex]);
					}
					else if( prob <= bestProb + eps )
					{
						bestCells.push_back(group.variables[varIndex]);
					}
				}
			}
		}

		if( numUnconstrained > 0 )
		{
			double mineWeight = 0.0;
			for( int frontierMine = 0; frontierMine <= numRemainingBomb; ++frontierMine )
			{
				int unconstrainedMine = numRemainingBomb - frontierMine;
				mineWeight += prefix[numGroup * dpStride + frontierMine] * Combination(numUnconstrained - 1, unconstrainedMine - 1);
			}

			double prob = mineWeight / totalWeight;
			for( size_t i = 0; i < unconstrainedCells.size(); ++i )
				mCellProb[getIndex(unconstrainedCells[i]->x, unconstrainedCells[i]->y)] = float(prob);

			if( prob <= eps )
			{
				for( size_t i = 0; i < unconstrainedCells.size(); ++i )
					addSolvedCell(*unconstrainedCells[i], 0, nullptr);
				result = true;
			}
			else if( prob >= 1.0 - eps )
			{
				for( size_t i = 0; i < unconstrainedCells.size(); ++i )
					addSolvedCell(*unconstrainedCells[i], CV_FLAG, nullptr);
				result = true;
			}
			else if( bAllowGuess )
			{
				if( prob + eps < bestProb )
				{
					bestProb = prob;
					bestCells.clear();
					bestCells.insert(bestCells.end(), unconstrainedCells.begin(), unconstrainedCells.end());
				}
				else if( prob <= bestProb + eps )
				{
					bestCells.insert(bestCells.end(), unconstrainedCells.begin(), unconstrainedCells.end());
				}
			}
		}

		if( result )
			return true;

		if( !bAllowGuess || bestCells.empty() )
			return false;

		int bestKnownNeighborCount = -1;
		TArray< CellData* > bestKnownCells;
		for( size_t i = 0; i < bestCells.size(); ++i )
		{
			int const knownNeighborCount = countKnownNeighborCells(*bestCells[i], 2);
			if( knownNeighborCount > bestKnownNeighborCount )
			{
				bestKnownNeighborCount = knownNeighborCount;
				bestKnownCells.clear();
				bestKnownCells.push_back(bestCells[i]);
			}
			else if( knownNeighborCount == bestKnownNeighborCount )
			{
				bestKnownCells.push_back(bestCells[i]);
			}
		}

		bestCell = bestKnownCells[::rand() % bestKnownCells.size()];
		mbShowProbability = true;
		mLastGuessX = bestCell->x;
		mLastGuessY = bestCell->y;
		mLastGuessProb = float(bestProb);
		dumpGuessLocalDistribution(*bestCell, bestProb);
		addSolvedCell(*bestCell, 0, nullptr);
		return true;
	}

	bool CSPSolveStrategy::solveStep()
	{
		clearProbabilityDisplay();

		if( popSolvedCell() )
			return true;

		syncMapFromControl();
		updateDirtyConstraints();

		if( solveBySimpleConstraint() && popSolvedCell() )
			return true;

		if( solveByGlobalProbability(true) && popSolvedCell() )
			return true;

		return false;
	}

}//namespace Mine
