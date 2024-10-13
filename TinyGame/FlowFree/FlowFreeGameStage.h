#pragma once
#ifndef FlowFreeGameStage_H_4DE1BF0D_4166_440E_9E63_87D1DF6A457E
#define FlowFreeGameStage_H_4DE1BF0D_4166_440E_9E63_87D1DF6A457E


#include "Stage/TestStageHeader.h"

#include "FlowFreeLevel.h"
#include "FlowFreeSolver.h"
#include "FlowFreeLevelReader.h"

#include "Template/ArrayView.h"
#include "PlatformThread.h"
#include "GameRenderSetup.h"
#include "TemplateMisc.h"


namespace FlowFree
{

#define USE_EDGE_SOLVER 0

	class SolveThreadBase : public RunnableThreadT< SolveThreadBase >
	{
	public:

		unsigned run()
		{
			TGuardValue<bool> solveGuard(bSolving, true);
			solve();
			return 0;
		}

		virtual void solve() = 0;

		bool init() { return true; }
		void exit() {}

		bool    bSolving = false;
	};

	template< typename TSolver> 
	class TSolveThread : public SolveThreadBase
	{
	public:

		virtual void solve()
		{
			mSolver.solve();
		}

		Level*  mLevel;	
		TSolver& mSolver;
	};


	class LevelStage : public StageBase , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		LevelStage() {}

		virtual bool onInit() override;

		virtual void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart();

		Vec2i ToScreenPos(Vec2i const& cellPos);
		Vec2i ToCellPos(Vec2i const& screenPos);

		virtual void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
		}

		void onRender(float dFrame) override;

		ImageReader mReader;

		Vec2i     flowOutCellPos;
		ColorType flowOutColor;
		bool  bStartFlowOut = false;
		DepthFirstSolver mSolver;

#if USE_EDGE_SOLVER
		SATSolverEdge mSolver2;
#else
		SATSolverCell mSolver2;
#endif
		SolveThreadBase* mSolverThread = nullptr;

		int numCellFilled = 0;
		int IndexReaderTextureShow = ImageReader::DebugTextureCount;

		MsgReply onMouse(MouseMsg const& msg) override;

		MsgReply onKey(KeyMsg const& msg) override;

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
		Color3ub mColorMap[32];
		Color3ub mGridColor;
	};
}

#endif // FlowFreeGameStage_H_4DE1BF0D_4166_440E_9E63_87D1DF6A457E