#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"

using namespace Render;

class MidlineTestStage : public StageBase
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

	struct Point;
	struct DebugInfo
	{
		Vector2 pos;
		Vector2 dir;
		Vector2 normal;
		Vector2 centerPos;
		float   distNormal;
		float   dist;
		void draw(RHIGraphics2D& g, Vector2 const& testPos)
		{
			Vector2 rPos = centerPos + (distNormal ) * normal;
			g.drawLine(pos, testPos);
			g.drawLine(centerPos, rPos);
			g.drawText(rPos, InlineString<>::Make("%.2f", dist));
		}
	};

	struct Point
	{
		Vector2 pos;
		Vector2 dir;
		float   length;
		float   distance;

		DebugInfo getDebugInfo(float dotDist, Vector2 const& offset) const
		{
			DebugInfo result;
			result.normal = Math::Sign(offset.cross(dir)) * Vector2(dir.y, -dir.x);
			result.distNormal = Math::Sqrt(offset.length2() - Math::Square(dotDist));
			result.centerPos = pos + (0.5 * length) * dir;
			result.pos  = result.centerPos + result.distNormal * result.normal;
			result.dist = distance + 0.5 * length;
			result.dir  = dir;
			return result;
		}
	};

	TArray<Point>   mPoints;
	TArray<Vector2> mPosList =
	{
		Vector2(200, 200),
		Vector2(300, 200),
		Vector2(400, 300),
		Vector2(400, 400),
	};

	void buildMidline(TArray<Vector2> const& posList)
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

	bool    mbInFrist;
	float   mP1;
	float   mP2;
	float   mAlpha;

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
				debugInfos[0].draw(g, testPos);
			}
			if (debugMask & BIT(1))
			{
				debugInfos[1].draw(g, testPos);
			}

			if (debugMask == (BIT(0) | BIT(1)))
			{
				if (mbInFrist)
				{
					Vector2 p1 = testPos + debugInfos[0].dir * mP1;
					Vector2 p2 = p1 + debugInfos[1].dir * mP2;
					RenderUtility::SetPen(g, EColor::Blue);
					g.drawLine(p2, debugInfos[1].centerPos);
					RenderUtility::SetPen(g, EColor::Orange);
					g.drawLine(testPos, p1);
					g.drawLine(p1, p2);
				}
				else
				{
					Vector2 p1 = testPos - debugInfos[1].dir * mP1;
					Vector2 p2 = p1 - debugInfos[0].dir * mP2;
					RenderUtility::SetPen(g, EColor::Blue);
					g.drawLine(p2, debugInfos[0].centerPos);
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
				g.drawText(testPos + Vector2(0, 15), InlineString<>::Make("%.2f , %.2f , %.2f", mAlpha, mP1, mP2));
			}
		}
		g.endRender();
	}


	DebugInfo debugInfos[2];
	uint32    debugMask = 0;


	void rebuildMidline()
	{
		buildMidline(mPosList);
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
			int indexPoint = INDEX_NONE;
			float minDistSq = Math::MaxFloat;
			for (int index = 0; index < mPoints.size(); ++index)
			{
				float distSq = (testPos - mPoints[index].pos).length2();

				if (distSq < minDistSq)
				{
					indexPoint = index;
					minDistSq = distSq;
				}
			}

			mIndexPoint = indexPoint;
			auto const& point = mPoints[indexPoint];
			Vector2 offset = testPos - point.pos;
			float dotOffset = point.dir.dot(offset);

			if (indexPoint == 0)
			{
				mDistance = point.distance + dotOffset;

				debugMask = BIT(1);
				debugInfos[1] = point.getDebugInfo(dotOffset, offset);
			}
			else
			{
				auto const& prevPoint = mPoints[indexPoint - 1];
				float dotOffsetPrev = prevPoint.dir.dot(offset);
				if (indexPoint == mPoints.size() - 1)
				{
					mDistance = point.distance + dotOffsetPrev;

					debugMask = BIT(0);
					debugInfos[0] = prevPoint.getDebugInfo(dotOffsetPrev, offset);
				}
				else
				{

					float cos = point.dir.dot(prevPoint.dir);
					float sin2 = 1 - cos * cos;

					float d1 = 0.5 * prevPoint.length;
					float d2 = 0.5 * point.length;
					float p1 = d1 + dotOffsetPrev;
					float p2 = d2 - dotOffset;
					float det = d2 * p1 - d1 * p2;

					float alpha;
					if (det > 0)
					{
						float x = det / (d1 + d2 * cos);
						float y = p1 - x * cos;
						//alpha = 1 - p2 / (p2 + x + y);
						alpha = 1 - p2 / (p1 + p2 + x * (1 - cos));
						mP1 = x;
						mP2 = y;
						mbInFrist = false;
					}
					else
					{
						float x = -det / (d2 + d1 * cos);
						float y = p2 - x * cos;
						//alpha = p1 / (p1 + x + y);
						alpha = p1 / (p1 + p2 + x * (1 - cos));
						mP1 = x;
						mP2 = y;
						mbInFrist = true;
					}
					mDistance = prevPoint.distance + d1 + alpha * (d1 + d2);
					mAlpha = alpha;

					debugMask = BIT(0) | BIT(1);
					debugInfos[0] = prevPoint.getDebugInfo(dotOffsetPrev, offset);
					debugInfos[1] = point.getDebugInfo(dotOffset, offset);
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