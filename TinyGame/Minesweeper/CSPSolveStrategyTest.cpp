#include "StageRegister.h"

#include "Minesweeper/CSPSolveStrategy.h"

#include <cmath>
#include <cstring>
#include <vector>

namespace
{
	struct LocalCell
	{
		Mine::CellData data;
	};

	void InitVariable(LocalCell& cell, int x, int y)
	{
		cell.data.x = x;
		cell.data.y = y;
		cell.data.number = Mine::CV_UNPROBLED;
		cell.data.group = nullptr;
		cell.data.varIndex = -1;
	}

	void InitConstraint(LocalCell& cell, int x, int y, int constant)
	{
		cell.data.x = x;
		cell.data.y = y;
		cell.data.number = constant;
		cell.data.group = nullptr;
		cell.data.ct.init();
		cell.data.ct.constant = constant;
	}

	struct BoardCellRef
	{
		int x;
		int y;
		int index;
	};

	bool IsMark(char ch)
	{
		return ch == 'M' || ch == 'F';
	}

	bool IsVariable(char ch)
	{
		return ch == '#' || ch == '?';
	}

	bool BuildLocalGroup(char const* const* rows, int sizeY, Mine::ConstraintGroup& group, std::vector< LocalCell >& variables, std::vector< LocalCell >& constraints)
	{
		int const sizeX = int(strlen(rows[0]));
		std::vector< BoardCellRef > variableRefs;
		std::vector< BoardCellRef > numberRefs;

		variables.clear();
		constraints.clear();
		variables.reserve(sizeX * sizeY);

		for( int y = 0; y < sizeY; ++y )
		{
			for( int x = 0; x < sizeX; ++x )
			{
				char ch = rows[y][x];
				if( IsVariable(ch) )
				{
					variableRefs.push_back({ x, y, int(variables.size()) });
					variables.push_back(LocalCell());
					InitVariable(variables.back(), x, y);
				}
				else if( '0' <= ch && ch <= '8' )
				{
					numberRefs.push_back({ x, y, ch - '0' });
				}
			}
		}

		constraints.reserve(numberRefs.size());
		for( BoardCellRef const& numberRef : numberRefs )
		{
			int constant = numberRef.index;
			for( int ny = numberRef.y - 1; ny <= numberRef.y + 1; ++ny )
			{
				for( int nx = numberRef.x - 1; nx <= numberRef.x + 1; ++nx )
				{
					if( nx == numberRef.x && ny == numberRef.y )
						continue;
					if( nx < 0 || nx >= sizeX || ny < 0 || ny >= sizeY )
						continue;

					if( IsMark(rows[ny][nx]) )
						--constant;
				}
			}

			constraints.push_back(LocalCell());
			InitConstraint(constraints.back(), numberRef.x, numberRef.y, constant);

			for( BoardCellRef const& variableRef : variableRefs )
			{
				if( std::abs(variableRef.x - numberRef.x) <= 1 &&
					std::abs(variableRef.y - numberRef.y) <= 1 )
				{
					constraints.back().data.ct.addVarrible(variables[variableRef.index].data);
				}
			}

			LogMsg("CSP local constraint (%d,%d) value = %d -> constant = %d , numVar = %d",
				numberRef.x, numberRef.y, numberRef.index, constraints.back().data.ct.constant, constraints.back().data.ct.numVar);

			if( constraints.back().data.ct.numVar > 0 )
				group.addConstraint(constraints.back().data.ct);
		}

		return !variables.empty();
	}
}

void CSPLocalProbabilityMarked22MarkedTest()
{
	// Local board from the cropped screenshot with one-cell border:
	// ??????
	// ?MMM.?
	// ?35#.?
	// ?1M#.?
	// ?24#.?
	// ?1M#.?
	// ?235.?
	// ?M2#.?
	// ?12#.?
	// ??????
	//
	// M is a marked mine, # is a target local variable to enumerate, and ? is a
	// hidden border variable needed by edge number constraints.
	// The red bomb cell is kept as # because probability is computed before it
	// is opened and revealed on GameOver.
	char const* rows[] =
	{
		"?????",
		"?MMM?",
		"?35#?",
		"?1M#?",
		"?24#?",
		"?1M#?",
		"?235?",
		"?M2#?",
		"?12#?",
	};

	Mine::ConstraintGroup group;
	std::vector< LocalCell > variables;
	std::vector< LocalCell > constraints;
	TEST_CHECK(BuildLocalGroup(rows, int(ARRAY_SIZE(rows)), group, variables, constraints));

	TEST_CHECK(group.search());
	LogMsg("CSP local image group solutions = %d , vars = %u", group.numSolution, unsigned(group.variables.size()));
	if( group.numSolution == 0 )
	{
		LogWarning(0, "CSP local image has no valid solution. The cropped board is missing neighbor cells for at least one visible number.");
		return;
	}

	for( size_t i = 0; i < group.variables.size(); ++i )
	{
		Mine::CellData const* cell = group.variables[i];
		char const cellType = rows[cell->y][cell->x];
		if( cellType != '#' )
			continue;

		double const prob = double(group.mineCount[i]) / double(group.numSolution);
		LogMsg("CSP local image var[%u] %c (%d,%d) = %.2f%% mineCount = %d",
			unsigned(i), cellType, cell->x, cell->y, 100.0 * prob, group.mineCount[i]);
	}
}

REGISTER_MISC_TEST_ENTRY("Minesweeper CSP Test", CSPLocalProbabilityMarked22MarkedTest);
