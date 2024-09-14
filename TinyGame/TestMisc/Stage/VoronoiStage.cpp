#include "Stage/TestStageHeader.h"

#include "GameRenderSetup.h"
#include "GameGlobal.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "ProfileSystem.h"
#include "RandomUtility.h"
#include "DebugDraw.h"
#include "Algo/Voronoi.h"
#include "Async/Coroutines.h"

namespace Voronoi 
{
	using namespace Render;

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

		virtual bool onInit()
		{
			::Global::GUI().cleanupWidget();
			auto frame = WidgetUtility::CreateDevFrame();
			frame->addButton("Benchmark", [this](int event, GWidget*)
			{
#if 1
				Vector2 size = 2 * Vector2(::Global::GetScreenSize());
				Vector2 offset = -0.5 * Vector2(::Global::GetScreenSize());
#else
				Vector2 size = 0.5 * Vector2(::Global::GetScreenSize());
				Vector2 offset = 0.25 * Vector2(::Global::GetScreenSize());
#endif
				mPosList.clear();
				for (int i = 0; i < 5000; ++i)
				{
					mPosList.push_back( Vector2(RandFloat(0, size.x) , RandFloat(0, size.y)) + offset);
				}
				doGenerate();
				return true;
			});
			frame->addCheckBox("Draw Circumcircle", bDrawCircumcircle);
			restart();

			TArray<int> aa = { 1 , 2 , 3 };
			for (auto a : MakeReverseView(aa))
			{
				LogMsg("%d", a);

			}

			return true;
		}

		TArray< Vector2 >  mPosList;
		TArray< Vector2 >  mBoundaryLines;
		virtual void onEnd()
		{

		}

		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}

		bool bDrawCircumcircle = false;

		void onRender( float dFrame )
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.2, 0.2, 0.2, 1), 1);

			g.beginRender();
			RenderUtility::SetPen(g, EColor::Yellow);
			if ( hExec.isNone() || hExec.isCompleted())
			{
				for (auto const& tri : solver.triangulation)
				{
					auto const& v0 = mPosList[tri.indices[0]];
					auto const& v1 = mPosList[tri.indices[1]];
					auto const& v2 = mPosList[tri.indices[2]];

					g.drawLine(v0, v1);
					g.drawLine(v1, v2);
					g.drawLine(v2, v0);
				}

#if USE_CIRCUCIRCLE_CACHE
				if (bDrawCircumcircle)
				{
					RenderUtility::SetPen(g, EColor::Cyan);
					RenderUtility::SetBrush(g, EColor::Null);
					for (auto const& tri : solver.triangulation)
					{
						g.drawCircle(tri.pos, Math::Sqrt(tri.radiusSquare));
					}
				}
#endif
				RenderUtility::SetPen(g, EColor::Pink);
				for (int i = 0; i < mBoundaryLines.size(); i += 2)
				{
					g.drawLine(mBoundaryLines[i], mBoundaryLines[i + 1]);

				}
			}

			DrawDebugCommit(::Global::GetIGraphics2D());

			RenderUtility::SetPen(g, EColor::Black);
			RenderUtility::SetBrush(g, EColor::Red);
			for (auto const& pos : mPosList)
			{
				g.drawCircle(pos, 5);
			}

			g.endRender();
		}

		void restart()
		{
#if 0
			mPosList = { Vector2(100,100), Vector2(100,200), Vector2(200,200) };
			generate();
#endif
		}


		void tick()
		{

		}

		void updateFrame( int frame )
		{

		}


		DelaunaySolver solver;
		void doGenerate()
		{
			{
				TIME_SCOPE("Delaunay");
				DrawDebugClear();
				solver.solve(mPosList);
			}

			generateBoundaryLines();
		}

		void generateBoundaryLines()
		{

#if USE_CIRCUCIRCLE_CACHE
			mBoundaryLines.clear();
			for (int i = 0; i < solver.triangulation.size(); ++i)
			{
				auto const& tri = solver.triangulation[i];

				for (int indexEdge = 0; indexEdge < 3; ++indexEdge)
				{
#if USE_TRIANGLE_CONNECTIVITY && USE_EDGE_INFO

					Edge const& edge = solver.edges[tri.edgeIDs[indexEdge]];
					int indexShared = edge.getOtherTriID(i);
					if (indexShared != INDEX_NONE)
					{
						auto& triOther = solver.triangulation[indexShared];
						mBoundaryLines.push_back(tri.pos);
						mBoundaryLines.push_back(triOther.pos);
					}
					else
					{
						int i0 = edge.indices[0];
						int i1 = edge.indices[1];
						int i2 = edge.getOtherIndex(tri);
						Vector2 d1 = mPosList[i1] - mPosList[i0];
						Vector2 d2 = mPosList[i2] - mPosList[i0];
						Vector2 dir = Math::GetNormal(Math::TripleProduct(d2, d1, d1));
						mBoundaryLines.push_back(tri.pos);
						mBoundaryLines.push_back(tri.pos + 10000 * dir);
					}
#else		
					int indexShared = INDEX_NONE;
					int i0 = tri.indices[indexEdge];
					int i1 = tri.indices[(indexEdge + 1) % 3];

					{
						for (int j = 0; j < solver.triangulation.size(); ++j)
						{
							if (i == j)
								continue;
							auto& triOther = solver.triangulation[j];

							if ((triOther.indices[0] == i0 || triOther.indices[1] == i0 || triOther.indices[2] == i0) &&
								(triOther.indices[0] == i1 || triOther.indices[1] == i1 || triOther.indices[2] == i1))
							{
								indexShared = j;
								break;
							}
						}
					}

					if (indexShared != INDEX_NONE)
					{
						auto& triOther = solver.triangulation[indexShared];
						mBoundaryLines.push_back(tri.pos);
						mBoundaryLines.push_back(triOther.pos);
					}
					else
					{
						int i2 = tri.indices[(indexEdge + 2) % 3];
						Vector2 d1 = mPosList[i1] - mPosList[i0];
						Vector2 d2 = mPosList[i2] - mPosList[i0];
						Vector2 dir = Math::GetNormal(Math::TripleProduct(d2, d1, d1));
						mBoundaryLines.push_back(tri.pos);
						mBoundaryLines.push_back(tri.pos + 10000 * dir);
					}
					
#endif
				}
			}
#endif

		}

		Coroutines::ExecutionHandle hExec;
		void generate()
		{
#if 1
			Coroutines::Stop(hExec);
			hExec = Coroutines::Start([this]()
			{
				doGenerate();
			});
#else
			doGenerate();
#endif

		}

		MsgReply onMouse( MouseMsg const& msg )
		{
			if (msg.onLeftDown())
			{
				Vector2 pos = msg.getPos();
				mPosList.push_back(pos);
				generate();
				return MsgReply::Handled();
			}
			else if (msg.onRightDown())
			{
				if (!mPosList.empty())
				{
					mPosList.pop_back();
					generate();
				}
				return MsgReply::Handled();
			}
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg)
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				case EKeyCode::X:
					{
						DrawDebugClear();
						Coroutines::Resume(hExec);
					}
					break;
				}
			}
			return BaseClass::onKey(msg);
		}
	};

	REGISTER_STAGE_ENTRY("Voronoi Test", TestStage, EExecGroup::Test);

}//namespace Voronoi
