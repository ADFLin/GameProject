#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"

using namespace Render;


struct MidlineData
{
	struct Point;

	struct Point
	{
		Vector2 pos;
		Vector2 dir;
		float   length;
		float   distance;
	};


	struct DebugPoint
	{
		Vector2 projectPos;
		Vector2 centerPos;
		Vector2 rPos;
		Vector2 dir;
		float   dist;

		void init(Point const& point, Vector2 const& offset, float dotDist)
		{
			Vector2 normal = Math::Sign(offset.cross(point.dir)) * Vector2(point.dir.y, -point.dir.x);
			float distNormal = Math::Sqrt(offset.length2() - Math::Square(dotDist));
			
			centerPos = point.pos + (0.5 * point.length) * point.dir;
			projectPos  = centerPos + distNormal * normal;
			dist = point.distance + 0.5 * point.length;
			dir  = point.dir;
		}

		void draw(RHIGraphics2D& g, Vector2 const& testPos)
		{
			g.drawLine(projectPos, testPos);
			g.drawLine(centerPos, projectPos);
			g.drawText(projectPos, InlineString<>::Make("%.2f", dist));
		}
	};


	TArray<Point>   mPoints;

	void build(TArray<Vector2> const& posList)
	{
		mPoints.clear();

		float dist = 0;
		for (int index = 0; index < posList.size(); ++index)
		{
			Point* point = mPoints.addUninitialized(1);
			point->pos = posList[index];
			point->distance = dist;
			if (index + 1 < posList.size())
			{
				Vector2 offset = posList[index + 1] - posList[index];
				point->length = offset.normalize();
				dist += point->length;
				point->dir = offset;
			}
			else
			{
				point->dir = Vector2::Zero();
				point->length = 0;
			}
		}
	}

	int32 findNearestPoint(Vector2 const& pos)
	{
		int indexPoint = INDEX_NONE;
		float minDistSq = Math::MaxFloat;
		for (int index = 0; index < mPoints.size(); ++index)
		{
			float distSq = (pos - mPoints[index].pos).length2();

			if (distSq < minDistSq)
			{
				indexPoint = index;
				minDistSq = distSq;
			}
		}
		return indexPoint;
	}
};

class MidlineTestStage : public StageBase
	                   , public MidlineData
					   , public IGameRenderSetup
{
	using BaseClass = StageBase;
public:
	MidlineTestStage() {}

	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();


		auto frame = WidgetUtility::CreateDevFrame();

		restart();
		return true;
	}


	TArray<Vector2> mPosList =
	{
		Vector2(200, 200),
		Vector2(300, 200),
		Vector2(400, 300),
		Vector2(400, 400),
	};



	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart()
	{
		rebuildMidline();
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

	bool    bHavePos = false;
	Vector2 testPos;
	int     mIndexPoint = INDEX_NONE;
	float   mDistance;

	struct Debug
	{




	};
	bool    mbInFrist;
	float   mPx;
	float   mPy;
	float   mAlpha;
	float   mDelta;

	void onRender(float dFrame) override
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();
		auto& commandList = RHICommandList::GetImmediateList();
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);

		g.beginRender();

		RenderUtility::SetFont(g, FONT_S8);

		for (int index = 0; index < mPoints.size(); ++index)
		{
			Point const& point = mPoints[index];
			RenderUtility::SetPen(g, EColor::Red);
			if (index + 1 < mPoints.size())
			{
				Point const& nextPoint = mPoints[index + 1];
				g.drawLine(point.pos, nextPoint.pos);
				RenderUtility::SetFontColor(g, EColor::Red);
				g.drawText(0.5 * (point.pos + nextPoint.pos), InlineString<>::Make("%.2f", point.length));
			}

			RenderUtility::SetPen(g, EColor::Null);
			RenderUtility::SetBrush(g, EColor::Green);
			Vector2 size = Vector2(5, 5);
			g.drawRect(point.pos - 0.5 * size, size);
			RenderUtility::SetFontColor(g, EColor::Green);
			g.drawText(point.pos + Vector2(0, 10), InlineString<>::Make("%.2f", point.distance));
		}

		if (bHavePos)
		{
			RenderUtility::SetPen(g, EColor::Blue);
			RenderUtility::SetFontColor(g, EColor::Cyan);
			if (debugMask & BIT(0))
			{
				debugPoints[0].draw(g, testPos);
			}
			if (debugMask & BIT(1))
			{
				debugPoints[1].draw(g, testPos);
			}

			if (debugMask == (BIT(0) | BIT(1)))
			{
				if (mbInFrist)
				{
					Vector2 p1 = testPos + debugPoints[0].dir * mPx;
					Vector2 p2 = p1 + debugPoints[1].dir * mPy;
					RenderUtility::SetPen(g, EColor::Blue);
					g.drawLine(p2, debugPoints[1].centerPos);
					RenderUtility::SetPen(g, EColor::Orange);
					g.drawLine(testPos, p1);
					g.drawLine(p1, p2);
				}
				else
				{
					Vector2 p1 = testPos - debugPoints[1].dir * mPx;
					Vector2 p2 = p1 - debugPoints[0].dir * mPy;
					RenderUtility::SetPen(g, EColor::Blue);
					g.drawLine(p2, debugPoints[0].centerPos);
					RenderUtility::SetPen(g, EColor::Orange);
					g.drawLine(testPos, p1);
					g.drawLine(p1, p2);
				}
			}

			RenderUtility::SetPen(g, EColor::Null);
			RenderUtility::SetBrush(g, EColor::Yellow);
			Vector2 size = Vector2(5, 5);
			g.drawRect(testPos - 0.5 * size, size);
			RenderUtility::SetFontColor(g, EColor::Yellow);
			g.drawText(testPos, InlineString<>::Make("%.2f", mDistance));

			if (debugMask == (BIT(0) | BIT(1)))
			{
				g.drawText(testPos + Vector2(0, 15), InlineString<>::Make("%.2f , %.2f , %.2f , %.2f", mAlpha, mDelta, mPx, mPy));
			}
		}
		g.endRender();
	}


	DebugPoint debugPoints[2];
	uint32     debugMask = 0;


	void rebuildMidline()
	{
		MidlineData::build(mPosList);
		mIndexPoint = INDEX_NONE;
	}

	MsgReply onMouse(MouseMsg const& msg) override
	{
		if (msg.onLeftDown())
		{
			Vector2 pos = Vector2(msg.getPos());
			mPosList.push_back(pos);
			rebuildMidline();
		}
		else if (msg.onRightDown())
		{
			bHavePos = true;
			testPos = Vector2(msg.getPos());

			int32 indexPoint = findNearestPoint(testPos);
			mIndexPoint = indexPoint;
			auto const& point = mPoints[indexPoint];
			Vector2 offset = testPos - point.pos;
			float dotOffset = point.dir.dot(offset);

			if (indexPoint == 0)
			{
				mDistance = point.distance + dotOffset;

				debugMask = BIT(1);
				debugPoints[1].init(point, offset, dotOffset);
			}
			else
			{
				auto const& prevPoint = mPoints[indexPoint - 1];
				float dotOffsetPrev = prevPoint.dir.dot(offset);
				if (indexPoint == mPoints.size() - 1)
				{
					mDistance = point.distance + dotOffsetPrev;

					debugMask = BIT(0);
					debugPoints[0].init(prevPoint, offset, dotOffsetPrev);
				}
				else
				{

					float cos =  point.dir.dot(prevPoint.dir);
					float d1 = 0.5 * prevPoint.length;
					float d2 = 0.5 * point.length;
					float p1 = d1 + dotOffsetPrev;
					float p2 = d2 - dotOffset;

					float det = d2 * p1 - d1 * p2;
					//float det = d2 * dotOffsetPrev + d1 * dotOffset;

					float alpha;
					if (det < 0)
					{
						float x = -det / (d2 + d1 * cos);
						float y = p2 - x * cos;
						alpha = p1 / (p1 + x + y);
						//alpha = p1 / (p1 + p2 + x * (1 - cos));
						mPx = x;
						mPy = y;
						mbInFrist = true;
					}
					else
					{
						float x = det / (d1 + d2 * cos);
						float y = p1 - x * cos;
						alpha = 1 - p2 / (p2 + x + y);
						//alpha = 1 - p2 / (p1 + p2 + x * (1 - cos));
						mPx = x;
						mPy = y;
						mbInFrist = false;
					}

					mDistance = prevPoint.distance + d1 + alpha * (d1 + d2);
					mAlpha = alpha;
					mDelta = mAlpha - p1 / (p1 + p2);

					debugMask = BIT(0) | BIT(1);
					debugPoints[0].init(prevPoint, offset, dotOffsetPrev);
					debugPoints[1].init(point, offset, dotOffset);
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
			case EKeyCode::Z:
				if (!mPosList.empty())
				{
					mPosList.pop_back();
					rebuildMidline();
				}
				break;
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


REGISTER_STAGE_ENTRY("Midline Test", MidlineTestStage, EExecGroup::MiscTest);