
#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "Math/GeometryPrimitive.h"


namespace VGR
{
	using namespace Render;

	struct CurveSegment
	{
		Vector2 start;
		Vector2 end;
		Vector2 control;

		bool findIntersection(float y, float outT[2])
		{
			float a = start.y + end.y - 2 * control.y;
			float b = control.y - start.y;
			float c = start.y - y;

			float det = b * b - a * c;
			if (det < 0)
			{
				return false;
			}

			det = Math::Sqrt(det);

			outT[0] = (-b + det) / a;
			outT[1] = (-b - det) / a;
			return true;
		}

		Vector2 get(float t)
		{
			return Math::BezierLerp(start, control, end, t);
		}
		void drawDebug(RHIGraphics2D& g)
		{
			RenderUtility::SetBrush(g, EColor::Null);

			Vector2 size = Vector2(4,4);
			RenderUtility::SetPen(g, EColor::Red);
			g.drawRect(start - 0.5 * size, size);
			g.drawRect(end - 0.5 * size, size);
			RenderUtility::SetPen(g, EColor::Blue);
			g.drawRect(control - 0.5 * size, size);
		}

		void draw(RHIGraphics2D& g, int num)
		{
			float delta = 1.0f / num;
			Vector2 p0 = get(0);

			float t = 0;
			for (int i = 0; i < num; ++i)
			{
				t += delta;
				Vector2 p1 = get(t);
				g.drawLine(p0, p1);
				p0 = p1;
			}
		}
	};




	class TestStage : public StageBase
	                , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		CurveSegment segments[8];

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();

			auto frame = WidgetUtility::CreateDevFrame();
			FWidgetProperty::Bind(frame->addSlider("Size") , mSettings.size , 1 , 20, [&](float value)
			{
				rasterize(mSettings);
			});

			restart();
			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() 
		{
			{
				CurveSegment& segment = segments[0];
				segment.start = Vector2(100, 200);
				segment.control = Vector2(100, 100);
				segment.end = Vector2(200, 100);
			}
			{
				CurveSegment& segment = segments[1];
				segment.start = Vector2(200, 100);
				segment.control = Vector2(300, 100);
				segment.end = Vector2(300, 200);
			}
			{
				CurveSegment& segment = segments[2];
				segment.start = Vector2(300, 200);
				segment.control = Vector2(300, 300);
				segment.end = Vector2(200, 300);
			}
			{
				CurveSegment& segment = segments[3];
				segment.start = Vector2(200, 300);
				segment.control = Vector2(100, 300);
				segment.end = Vector2(100, 200);
			}

			Vector2 offset = Vector2(50, 50);
			{
				CurveSegment& segment = segments[4];
				segment.start = Vector2(100, 150) + offset;
				segment.control = Vector2(100, 100) + offset;
				segment.end = Vector2(150, 100) + offset;
			}
			{
				CurveSegment& segment = segments[5];
				segment.start = Vector2(150, 100) + offset;
				segment.control = Vector2(150, 250);
				segment.end = Vector2(200, 150) + offset;
			}
			{
				CurveSegment& segment = segments[6];
				segment.start = Vector2(200, 150) + offset;
				segment.control = Vector2(300, 300);
				segment.end = Vector2(150, 200) + offset;
			}
			{
				CurveSegment& segment = segments[7];
				segment.start = Vector2(150, 200) + offset;
				segment.control = Vector2(100, 300);
				segment.end = Vector2(100, 150) + offset;
			}

			mSettings.bound.min = Vector2(50,50);
			mSettings.bound.max = Vector2(350,350);
			rasterize(mSettings);
		}

		void tick() {}
		void updateFrame(int frame) {}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for (int i = 0; i < frame; ++i)
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			auto& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);

			g.beginRender();

			RenderUtility::SetBrush(g, EColor::Purple);
			for (auto const& pos : mRasterDebugPosList)
			{
				Vector2 size = 0.5 * Vector2(mSettings.size, mSettings.size);
				RenderUtility::SetPen(g, EColor::Null);
				g.drawRect(pos - 0.5 * size, size);
			}

			for (auto& segment : segments)
			{
				RenderUtility::SetPen(g, EColor::Yellow);
				segment.draw(g, 32);
				segment.drawDebug(g);
			}

			RenderUtility::SetBrush(g, EColor::Null);
			for (auto const& pos : mDebugPosList)
			{
				Vector2 size = Vector2(4, 4);
				RenderUtility::SetPen(g, EColor::Green);
				g.drawRect(pos - 0.5 * size, size);
			}

			g.endRender();
		}


		TArray< Vector2 > mDebugPosList;
		TArray< Vector2 > mRasterDebugPosList;
		struct RasterizeSettings
		{
			float size = 20;
			Math::TAABBox< Vector2 > bound;
		};

		RasterizeSettings mSettings;

		struct Intersection
		{
			CurveSegment* segment;
			float posX;
			float t;
		};

		//TArray< TArray<Intersection> > mDebug;

		void rasterize(RasterizeSettings const& settings)
		{
			//mDebug.clear();
			mRasterDebugPosList.clear();

			float curY = settings.bound.min.y + 0.5 * settings.size;
			TArray<Intersection> intersectionList;

			auto TestCount = [](Intersection& intersection, float yValue)
			{
				float outT[2];
				if (!intersection.segment->findIntersection(yValue, outT))
					return 0;

				int index = (Math::Abs(outT[0] - intersection.t) <= Math::Abs(outT[1] - intersection.t)) ? 0 : 1;
				int count = 0;
				if (0 <= outT[index] && outT[index] <= 1.0f)
				{
					++count;
				}
				return count;
			};

			while (curY < settings.bound.max.y)
			{
				intersectionList.clear();

				for (auto& segment : segments)
				{
					auto AddPosConditional = [&](float t)
					{
						if (0 <= t && t <= 1.0f)
						{
							intersectionList.push_back({&segment, segment.get(t).x, t});
						}
					};

					float outT[2];
					if (segment.findIntersection(curY, outT))
					{
						AddPosConditional(outT[0]);
						if (Math::Abs(outT[0] - outT[1]) > 1e-5)
						{
							AddPosConditional(outT[1]);
						}
					}
				}

				std::sort(intersectionList.begin(), intersectionList.end(), 
					[](auto const& lhs, auto const& rhs)
					{
						return lhs.posX < rhs.posX;
					}
				);

				{
					int index = 0;
					while (index < intersectionList.size())
					{
						int nextIndex = index + 1;
						if (nextIndex >= intersectionList.size())
							break;

						if (Math::Abs(intersectionList[index].posX - intersectionList[nextIndex].posX) < 1e-4 )
						{
							auto RunTest = [&](float yValue)
							{
								int countA = TestCount(intersectionList[index], yValue);
								int countB = TestCount(intersectionList[nextIndex], yValue);
								return countA == 0 && countB == 0;
							};
							float offset = 0.1 * settings.size;
							if (RunTest(curY + offset) || RunTest(curY - offset))
							{
								++index;
							}
							else
							{
								intersectionList.removeIndex(nextIndex);
							}
						}
						else
						{
							++index;
						}
					}
				}

				if ( !intersectionList.empty() )
				{
					int curIndex = 0;
					float curX = settings.bound.min.x + 0.5 * settings.size;
					bool bInside = false;
					while (curX < settings.bound.max.x)
					{
						while (intersectionList.isValidIndex(curIndex))
						{
							if (bInside)
							{
								if (curX <= intersectionList[curIndex].posX)
									break;
							}
							else
							{
								if (curX < intersectionList[curIndex].posX)
									break;
							}

							++curIndex;
							bInside = !bInside;
						}

						if (bInside)
						{
							mRasterDebugPosList.push_back(Vector2(curX, curY));
						}
						curX += settings.size;
					}

					//mDebug.push_back(std::move(intersectionList));
				}

				curY += settings.size;
			}
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (msg.onLeftDown())
			{
				Vector2 pos = msg.getPos();
				mDebugPosList.clear();

				for (auto& segment : segments)
				{
					auto AddPosConditional = [&](float t)
					{
						if (0 <= t && t <= 1.0f)
						{
							mDebugPosList.push_back(segment.get(t));
						}
					};

					float outT[2];
					if (segment.findIntersection(pos.y, outT))
					{		
						AddPosConditional(outT[0]);
						if (Math::Abs(outT[0] - outT[1]) > 1e-5)
						{
							AddPosConditional(outT[1]);
						}
					}
				}
			}
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};

	REGISTER_STAGE_ENTRY("VGR Test", TestStage, EExecGroup::MiscTest);
}