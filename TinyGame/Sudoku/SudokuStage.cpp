#include "Stage/TestStageHeader.h"

#include "SudokuSolver.h"
#include "Coroutine.h"
#include "SamplerTest.h"


struct ProbInfo
{
	char const* probDec;
	int const*  prob;
};

class SudokuSolver : public SudokuSolverT< SudokuSolver >
{
public:
	SudokuSolver()
	{
		lastIndex = -1;
		methodMask = -1;
	}
	bool onPrevEvalMethod(Method method, Group group, int idx, unsigned numBit) 
	{
		if( !(methodMask & BIT(method)) )
			return false;

		std::fill_n(bRelatedCell, NumberNum * NumberNum, false);
		lastIndex = idx;
		lastGroup = group;
		lastMethod = method;
		lastNumBit = numBit;
		Jumper.jump();
		return true; 
	}
	void onPostEvalMethod(Method method, Group group, int idx, unsigned numBit) 
	{
		if( !checkState() )
		{
			::Msg("error method");
		}
		//Jumper.jump();
	}
	void doEnumRelatedCellInfo(RelatedCellInfo const& info)
	{
		bRelatedCell[info.index] = true;
	}
	static void foo()
	{

	}
	void solveProblem(int* prob)
	{
		if( !setupProblem(prob) )
		{
			::Msg("Can't init problem");
			return;
		}

		this->evalSimpleColourMethod(ToCellIndex(4,5));
		

		std::function< void() > fun = std::bind(&SudokuSolver::solveLogic, (SudokuSolver*)this);
		Jumper.start( fun );
	}

	bool        bRelatedCell[NumberNum * NumberNum];
	Method      lastMethod;
	Group       lastGroup;
	int         lastIndex;
	unsigned    lastNumBit;
	int MaxStep;
	int numStep;
	unsigned    methodMask;
	FunctionJumper Jumper;
};



class SudokuStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	SudokuStage() {}

	virtual bool onInit()
	{
		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	virtual void onEnd()
	{

	}

	virtual void onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	void restart()
	{
		mSolver.solveProblem(probSimpleColour);
		//mSolver.methodMask = BIT(SudokuSolver::eNakedPair);
		//mSolver.solveProblem(probNakedPair);
	}


	void tick()
	{

	}

	void updateFrame(int frame)
	{

	}


	void onRender(float dFrame)
	{
		Graphics2D& g = Global::getGraphics2D();

		//RenderUtility::setBrush(g, Color::eWhite);
		//RenderUtility::setPen(g, Color::eWhite);
		//g.drawRect(Vec2i(0, 0), ::Global::getDrawEngine()->getScreenSize());

		int const CellSize = 50;
		int const TotalSize = CellSize * SudokuSolver::NumberNum;

		Vec2i org(20, 20);

		RenderUtility::SetPen(g, Color::eRed , COLOR_LIGHT );
		for( int i = 0; i < SudokuSolver::NumberNum; ++i )
		{
			Vec2i p1 = org + Vec2i(i * CellSize, 0);
			g.drawLine( p1 , p1 + Vec2i(0, TotalSize));

			Vec2i p2 = org + Vec2i(0, i * CellSize);
			g.drawLine(p2, p2 + Vec2i(TotalSize, 0));
		}

		RenderUtility::SetPen(g, Color::eRed);
		for( int i = 0; i <= SudokuSolver::BoxSize; ++i )
		{
			Vec2i p1 = org + Vec2i(3 * i * CellSize, 0);
			g.drawLine(p1, p1 + Vec2i(0, TotalSize));

			Vec2i p2 = org + Vec2i(0, 3 * i * CellSize);
			g.drawLine(p2, p2 + Vec2i(TotalSize, 0));
		}

		TCHAR temp[128];
		FixString< 128 > str;
		int const* prob = mSolver.getProblem();

		for( int i = 0; i < SudokuSolver::MaxIndex; ++i )
		{
			if( mSolver.bRelatedCell[i] )
			{
				Vec2i pos = org + Vec2i((i % 9) * CellSize, (i / 9) * CellSize);
				RenderUtility::SetBrush(g, Color::eOrange);
				g.drawRect(pos, Vec2i(CellSize, CellSize));
			}
			int sol = mSolver.getSolution(i);

			if( sol )
			{
				Vec2i ptBox(CellSize * (i % SudokuSolver::NumberNum) + 17,
							 CellSize * (i / SudokuSolver::NumberNum) + 9);
				ptBox += org;

				if( prob[i] )
					g.setTextColor(255, 255, 255);
				else
					g.setTextColor(255, 255, 55);

				RenderUtility::SetFont(g, FONT_S24);
				g.drawText(ptBox, str.format("%d", SudokuSolver::Bit2Num(sol)));
			}
			else
			{
				RenderUtility::SetFont(g, FONT_S12);
				unsigned posible = mSolver.getIdxPosible(i);

				Vec2i ptBox(CellSize * (i % SudokuSolver::NumberNum) + 7,
							 CellSize * (i / SudokuSolver::NumberNum) + 5);
				ptBox += org;
				for( int n = 0; n < SudokuSolver::NumberNum; ++n )
				{
					int numBit = 1 << n;
					if( (posible & numBit) == 0 )
						continue;
					Vec2i of(14 * (n % SudokuSolver::BoxSize),
							  14 * (n / SudokuSolver::BoxSize));

					Vec2i pt = ptBox + of;

					if( mSolver.lastIndex == i )
						g.setTextColor(0, 255, 255);
					else
						g.setTextColor(255, 255, 255);

					g.drawText( pt , str.format("%d", n + 1));
				}
			}
		}


		RenderUtility::SetFont(g, FONT_S12);
		char const* groupStr[] = { "Col" , "Row" , "Cell" , "None" };
		//dc.SelectFont(fontPsb);
		g.setTextColor(0, 0, 255);
		g.drawText(50, 0, groupStr[mSolver.lastGroup]);


		Vec2i methodPos = Vec2i(100, 0);
		char const* methodStr = "Unknown Method!";
		switch( mSolver.lastMethod )
		{
		case SudokuSolver::eSolvedValue:   methodStr = "SolvedValue"; break;
		case SudokuSolver::eSingleValue:   methodStr = "SingleValue"; break;
		case SudokuSolver::eNakedPair:     methodStr = "NakedPair"; break;
		case SudokuSolver::eNakedTriple:   methodStr = "NakedTriple"; break;
		case SudokuSolver::eHiddenPair:    methodStr = "HiddenPair"; break;
		case SudokuSolver::eHiddenTriple:  methodStr = "HiddenTriple"; break;
		case SudokuSolver::ePointing:      methodStr = "Pointing"; break;
		case SudokuSolver::eBoxLine:       methodStr = "BoxLine"; break;
		case SudokuSolver::eXWing:         methodStr = "X-Wing"; break;
		case SudokuSolver::eYWing:         methodStr = "Y-Wing"; break;
		case SudokuSolver::eSimpleColour:  methodStr = "SimpleColour"; break;
		case SudokuSolver::eXCycle:        methodStr = "X-Cycle"; break;
		}

		g.drawText(methodPos, methodStr );

		{
			unsigned numBit = mSolver.lastNumBit;
			int n = 0;
			int num[SudokuSolver::NumberNum];
			while( numBit )
			{
				unsigned bit = numBit & -numBit;
				num[n++] = SudokuSolver::Bit2Num(bit);
				numBit -= bit;
			}

			for( int i = 0; i < n; ++i )
			{
				g.drawText(200 + 20 * i, 0, str.format("%d", num[i]));
			}
		}
	}


	bool onMouse(MouseMsg const& msg)
	{
		if( !BaseClass::onMouse(msg) )
			return false;
		return true;
	}

	bool onKey(unsigned key, bool isDown)
	{
		if( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
		case Keyboard::eJ: mSolver.Jumper.jump(); break;
		}
		return false;
	}

protected:


	SudokuSolver mSolver;
};


REGISTER_STAGE("Sudoku Test Stage", SudokuStage, EStageGroup::Test);
