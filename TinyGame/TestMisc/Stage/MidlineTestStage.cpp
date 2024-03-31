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

		
		restart();
		return true;
	}

	struct Point
	{
		Vector2 pos;
		Vector2 dir;
		float   length;
		float   distance;

		Vector2 getDebugPos(float dotDist, Vector2 const& offset) const
		{
			float distNormal = Math::Sqrt(offset.length2() - Math::Square(dotDist));
			float sign = Math::Sign( offset.cross(dir));
			return pos + ( sign * distNormal ) * Vector2(dir.y, -dir.x) + (0.5 * length) * dir;
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
		buildMidline(mPosList);
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
	float   mDistance;

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
			if (debugMask & BIT(0))
				g.drawLine(debugPos[0], testPos);
			if (debugMask & BIT(1))
				g.drawLine(debugPos[1], testPos);
			RenderUtility::SetPen(g, EColor::Null);
			RenderUtility::SetBrush(g, EColor::Yellow);
			Vector2 size = Vector2(5, 5);
			g.drawRect(testPos - 0.5 * size, size);
			RenderUtility::SetFontColor(g, EColor::Yellow);
			g.drawText(testPos, InlineString<>::Make("%.2f", mDistance));


		}
		g.endRender();
	}


	Vector2 debugPos[2];
	uint32  debugMask = 0;



	MsgReply onMouse(MouseMsg const& msg) override
	{
		if (msg.onLeftDown())
		{
			Vector2 pos = Vector2(msg.getPos());
			mPosList.push_back(pos);
			buildMidline(mPosList);
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
			auto const& point = mPoints[indexPoint];
			Vector2 offset = testPos - point.pos;
			float dotOffset = point.dir.dot(offset);

			if (indexPoint == 0)
			{
				mDistance = point.distance + dotOffset;

				debugMask = BIT(1);
				debugPos[1] = point.getDebugPos(dotOffset, offset);
			}
			else
			{
				auto const& prevPoint = mPoints[indexPoint - 1];
				float dotOffsetPrev = prevPoint.dir.dot(offset);
				if (indexPoint == mPoints.size() - 1)
				{
					mDistance = point.distance + dotOffsetPrev;

					debugMask = BIT(0);
					debugPos[0] = prevPoint.getDebugPos(dotOffsetPrev, offset);
				}
				else
				{
					float totalDist = 0.5 * (prevPoint.length + point.length);
					float prevLengthHalf = 0.5 * prevPoint.length;
					float alpha = (prevLengthHalf + dotOffsetPrev) / (totalDist + dotOffsetPrev - dotOffset);
					mDistance = prevPoint.distance + prevLengthHalf + alpha * totalDist;

					debugMask = BIT(0) | BIT(1);
					debugPos[0] = prevPoint.getDebugPos(dotOffsetPrev, offset);
					debugPos[1] = point.getDebugPos(dotOffset, offset);
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
				{
					mPosList.pop_back();
					buildMidline(mPosList);
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