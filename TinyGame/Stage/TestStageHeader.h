#pragma once

#include "GameConfig.h"

#include "StageBase.h"
#include "StageRegister.h"

#include "GameGUISystem.h"
#include "DrawEngine.h"
#include "RenderUtility.h"
#include "GameModule.h"
#include "GameGlobal.h"

#include "Widget/WidgetUtility.h"

#include "Core/IntegerType.h"
#include "MiscTestRegister.h"


class SimpleRenderer
{

public:
	SimpleRenderer()
	{
		mScale = 10.0f;
		mOffset = Vector2(0, 0);
	}
	Vector2 convertToWorld(Vec2i const& pos)
	{
		return Vector2(float(pos.x) / mScale, float(pos.y) / mScale) + mOffset;
	}
	Vector2 convertToScreen(Vector2 const& pos)
	{
		return Vec2i(mScale * (pos - mOffset));
	}

	void drawCircle(Graphics2D& g, Vector2 const& pos, float radius)
	{
		Vec2i rPos = convertToScreen(pos);
		g.drawCircle(rPos, int(mScale * radius));
	}

	void drawRect(Graphics2D& g, Vector2 const& pos, Vector2 const& size)
	{
		Vec2i rSize = Vec2i(size * mScale);
		Vec2i rPos = convertToScreen(pos);
		g.drawRect(rPos, rSize);

	}

	void drawLine(Graphics2D& g, Vector2 const& v1, Vector2 const& v2)
	{
		Vec2i buf[2];
		drawLine(g, v1, v2, buf);
	}


	void drawPoly(Graphics2D& g, Vector2 const vertices[], int num)
	{
		Vec2i buf[128];
		assert(num <= ARRAY_SIZE(buf));
		drawPoly(g, vertices, num, buf);
	}

	void drawPoly(Graphics2D& g, Vector2 const vertices[], int num, Vec2i buf[])
	{
		for( int i = 0; i < num; ++i )
		{
			buf[i] = convertToScreen(vertices[i]);
		}
		g.drawPolygon(buf, num);
	}

	void drawLine(Graphics2D& g, Vector2 const& v1, Vector2 const& v2, Vec2i buf[])
	{
		buf[0] = convertToScreen(v1);
		buf[1] = convertToScreen(v2);
		g.drawLine(buf[0], buf[1]);
	}

	float mScale;
	Vector2 mOffset;
};



#if 0
class TemplateTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	TemplateTestStage() {}

	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;
		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	virtual void onEnd()
	{
		BaseClass::onEnd();
	}

	void restart() {}
	void tick() {}
	void updateFrame(int frame) {}

	virtual void onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	void onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();
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
protected:
};
#endif

