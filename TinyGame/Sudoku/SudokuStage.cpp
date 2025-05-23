#include "Stage/TestStageHeader.h"

#include "SudokuSolver.h"
#include "Async/Coroutines.h"
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
	bool onPrevEvalMethod(EMethod method, Group group, int idx, unsigned numBit) 
	{
		if( !(methodMask & BIT(method)) )
			return false;

		std::fill_n(bRelatedCell, NumberNum * NumberNum, false);
		lastIndex = idx;
		lastGroup = group;
		lastMethod = method;
		lastNumBit = numBit;
		CO_YEILD(nullptr);
		return true; 
	}
	void onPostEvalMethod(EMethod method, Group group, int idx, unsigned numBit) 
	{
		if( !checkState() )
		{
			LogMsg("error method");
		}
		//CO_YEILD(nullptr);
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
			LogMsg("Can't init problem");
			return;
		}

		this->evalSimpleColourMethod(ToCellIndex(4,5));

		hExec = Coroutines::Start([&]()
		{
			solveLogic();
		});
	}

	Coroutines::ExecutionHandle hExec;

	bool        bRelatedCell[NumberNum * NumberNum];
	EMethod      lastMethod;
	Group       lastGroup;
	int         lastIndex;
	unsigned    lastNumBit;
	int MaxStep;
	int numStep;
	unsigned    methodMask;
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

	virtual void onUpdate(GameTimeSpan deltaTime)
	{
		BaseClass::onUpdate(deltaTime);
	}

	void restart()
	{
		mSolver.solveProblem(probSimpleColour);
		//mSolver.methodMask = BIT(SudokuSolver::eNakedPair);
		//mSolver.solveProblem(probNakedPair);
	}

	void onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();

		//RenderUtility::setBrush(g, EColor::eWhite);
		//RenderUtility::setPen(g, EColor::eWhite);
		//g.drawRect(Vec2i(0, 0), ::Global::GetScreenSize());

		int const CellSize = 50;
		int const TotalSize = CellSize * SudokuSolver::NumberNum;

		Vec2i org(20, 20);

		RenderUtility::SetPen(g, EColor::Red , COLOR_LIGHT );
		for( int i = 0; i < SudokuSolver::NumberNum; ++i )
		{
			Vec2i p1 = org + Vec2i(i * CellSize, 0);
			g.drawLine( p1 , p1 + Vec2i(0, TotalSize));

			Vec2i p2 = org + Vec2i(0, i * CellSize);
			g.drawLine(p2, p2 + Vec2i(TotalSize, 0));
		}

		RenderUtility::SetPen(g, EColor::Red);
		for( int i = 0; i <= SudokuSolver::BoxSize; ++i )
		{
			Vec2i p1 = org + Vec2i(3 * i * CellSize, 0);
			g.drawLine(p1, p1 + Vec2i(0, TotalSize));

			Vec2i p2 = org + Vec2i(0, 3 * i * CellSize);
			g.drawLine(p2, p2 + Vec2i(TotalSize, 0));
		}

		InlineString< 128 > str;
		int const* prob = mSolver.getProblem();

		for( int i = 0; i < SudokuSolver::MaxIndex; ++i )
		{
			if( mSolver.bRelatedCell[i] )
			{
				Vec2i pos = org + Vec2i((i % 9) * CellSize, (i / 9) * CellSize);
				RenderUtility::SetBrush(g, EColor::Orange);
				g.drawRect(pos, Vec2i(CellSize, CellSize));
			}
			int sol = mSolver.getSolution(i);

			if( sol )
			{
				Vec2i ptBox(CellSize * (i % SudokuSolver::NumberNum) + 17,
							 CellSize * (i / SudokuSolver::NumberNum) + 9);
				ptBox += org;

				if( prob[i] )
					g.setTextColor(Color3ub(255, 255, 255));
				else
					g.setTextColor(Color3ub(255, 255, 55));

				RenderUtility::SetFont(g, FONT_S24);
				str.format("%d", SudokuSolver::Bit2Num(sol));
				g.drawText(ptBox, str );
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
						g.setTextColor(Color3ub(0, 255, 255));
					else
						g.setTextColor(Color3ub(255, 255, 255));

					str.format("%d", n + 1);
					g.drawText( pt , str );
				}
			}
		}


		RenderUtility::SetFont(g, FONT_S12);
		char const* groupStr[] = { "Col" , "Row" , "Cell" , "None" };
		//dc.SelectFont(fontPsb);
		g.setTextColor(Color3ub(0, 0, 255));
		g.drawText(50, 0, groupStr[mSolver.lastGroup]);


		Vec2i methodPos = Vec2i(100, 0);
		char const* methodStr = "Unknown Method!";
		switch( mSolver.lastMethod )
		{
		case SudokuSolver::SolvedValue:   methodStr = "SolvedValue"; break;
		case SudokuSolver::SingleValue:   methodStr = "SingleValue"; break;
		case SudokuSolver::NakedPair:     methodStr = "NakedPair"; break;
		case SudokuSolver::NakedTriple:   methodStr = "NakedTriple"; break;
		case SudokuSolver::HiddenPair:    methodStr = "HiddenPair"; break;
		case SudokuSolver::HiddenTriple:  methodStr = "HiddenTriple"; break;
		case SudokuSolver::Pointing:      methodStr = "Pointing"; break;
		case SudokuSolver::BoxLine:       methodStr = "BoxLine"; break;
		case SudokuSolver::XWing:         methodStr = "X-Wing"; break;
		case SudokuSolver::YWing:         methodStr = "Y-Wing"; break;
		case SudokuSolver::SimpleColour:  methodStr = "SimpleColour"; break;
		case SudokuSolver::XCycle:        methodStr = "X-Cycle"; break;
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
				g.drawText(200 + 20 * i, 0, FStringConv::From(num[i]));
			}
		}
	}

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if(msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::J: Coroutines::Resume(mSolver.hExec); break;
			}
		}
		return BaseClass::onKey(msg);
	}

protected:


	SudokuSolver mSolver;
};


REGISTER_STAGE_ENTRY("Sudoku Test Stage", SudokuStage, EExecGroup::Test, "Test|Game");
