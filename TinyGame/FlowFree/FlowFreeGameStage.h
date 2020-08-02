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


	class TestStage : public StageBase , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		virtual bool onInit() override;

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
		
		int numCellFilled = 0;
		int IndexReaderTextureShow = ImageReader::DebugTextureCount;

		bool onMouse(MouseMsg const& msg) override;

		bool onKey(KeyMsg const& msg) override
		{
			if( !msg.isDown() )
				return false;
			switch(msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::X: mSolver.solveNext(); break;
			case EKeyCode::C: for (int i = 0; i < 97; ++i) mSolver.solveNext(); break;
			case EKeyCode::P: ++IndexReaderTextureShow; if (IndexReaderTextureShow > ImageReader::DebugTextureCount) IndexReaderTextureShow = 0; break;
			case EKeyCode::O: --IndexReaderTextureShow; if (IndexReaderTextureShow < 0) IndexReaderTextureShow = ImageReader::DebugTextureCount; break;
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

		Color3ub mColorMap[32];
	};
}

#endif // FlowFreeGameStage_H_4DE1BF0D_4166_440E_9E63_87D1DF6A457E