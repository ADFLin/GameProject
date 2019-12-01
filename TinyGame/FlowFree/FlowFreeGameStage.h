#pragma once
#ifndef FlowFreeGameStage_H_4DE1BF0D_4166_440E_9E63_87D1DF6A457E
#define FlowFreeGameStage_H_4DE1BF0D_4166_440E_9E63_87D1DF6A457E


#include "Stage/TestStageHeader.h"

#include "FlowFreeLevel.h"
#include "FlowFreeSolver.h"

#include "Template/ArrayView.h"
#include "PlatformThread.h"

namespace FlowFree
{

#define USE_EDGE_SOLVER 0

	class SolveThread : public RunnableThreadT< SolveThread >
	{
	public:

		unsigned run() 
		{
			mSolver.solve();
			return 0; 
		}
		bool init() { return true; }
		void exit() {}

		Level* mLevel;
		DepthFirstSolver& mSolver;
	};

	class TestStage : public StageBase
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		virtual bool onInit() override
		{
			if( !BaseClass::onInit() )
				return false;
			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

		virtual void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart();
		void tick() {}
		void updateFrame(int frame) {}


		Vec2i ToScreenPos(Vec2i const& cellPos);
		Vec2i ToCellPos(Vec2i const& screenPos);

		virtual void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame) override;



		Vec2i     flowOutCellPos;
		ColorType flowOutColor;
		bool  bStartFlowOut = false;
		DepthFirstSolver mSolver;

#if USE_EDGE_SOLVER
		SATSolverEdge mSolver2;
#else
		SATSolverCell mSolver2;
#endif
		
		int numCellFilled = 0;


		bool onMouse(MouseMsg const& msg) override
		{
			if( !BaseClass::onMouse(msg) )
				return false;

			if( msg.onLeftDown() )
			{
				Vec2i cPos = ToCellPos(msg.getPos());

				if( mLevel.isValidCellPos(cPos) )
				{
					bStartFlowOut = true;
					flowOutColor   = mLevel.getCellChecked(cPos).getFunFlowColor();
					flowOutCellPos = cPos;
					if (mLevel.breakFlow(cPos, 0, 0) == Level::BreakResult::NoBreak)
					{



					}
				}
			}
			else if( msg.onLeftUp() )
			{
				bStartFlowOut = false;
			}


			if( bStartFlowOut && msg.onMoving() )
			{
				Vec2i cPos = ToCellPos(msg.getPos());

				if ( mLevel.isValidCellPos(cPos) )
				{
					Vec2i cOffset = cPos - flowOutCellPos;
					Vec2i cOffsetAbs = Abs(cOffset);
					int d = cOffsetAbs.x + cOffsetAbs.y;
					if (d > 0)
					{
						if (d == 1)
						{
							int dir;
							if (cOffset.x == 1)
							{
								dir = 0;
							}
							else if (cOffset.y == 1)
							{
								dir = 1;
							}
							else if (cOffset.x == -1)
							{
								dir = 2;
							}
							else
							{
								dir = 3;
							}

							if (mLevel.breakFlow(cPos, dir, flowOutColor) == Level::BreakResult::HaveBreakSameColor)
							{
								flowOutCellPos = cPos;
							}
							else
							{
								ColorType linkColor = mLevel.linkFlow(flowOutCellPos, dir);
								if (linkColor)
								{
									flowOutCellPos = cPos;
									flowOutColor = linkColor;
								}
							}
						}
						else
						{



						}
					}
				}
			}

			return true;
		}

		bool onKey(unsigned key, bool isDown) override
		{
			if( !isDown )
				return false;
			switch( key )
			{
			case Keyboard::eR: restart(); break;
			case Keyboard::eX: mSolver.solveNext(); break;
			case Keyboard::eC: for (int i = 0; i < 97; ++i) mSolver.solveNext(); break;
			}
			return false;
		}

		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}

		Level mLevel;
	protected:
	};
}

#endif // FlowFreeGameStage_H_4DE1BF0D_4166_440E_9E63_87D1DF6A457E