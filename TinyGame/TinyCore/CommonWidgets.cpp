#include "TinyGamePCH.h"
#include "CommonWidgets.h"
#include "RenderUtility.h"

ExecButton::ExecButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
	:BaseClass(id, pos, size, parent)
{
}

void ExecButton::onRender()
{
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();
	IGraphics2D& g = Global::GetIGraphics2D();

	RenderUtility::SetPen(g, EColor::Null);
	if (mState == BS_PRESS)
	{
		g.setBrush(Color3ub(136, 155, 53));
	}
	else if (mState == BS_HIGHLIGHT)
	{
		g.setBrush(Color3ub(116, 135, 33));
	}
	else
	{
		g.setBrush(Color3ub(96, 115, 13));
	}
	g.drawRoundRect(pos, size, Vec2i(4, 4));

	g.pushXForm();
	g.translateXForm((float)pos.x, (float)pos.y);

	bool bExecuting = isExecutingFunc && isExecutingFunc();

	if (bExecuting)
	{
	g.setPen(Color3ub(255, 255, 255));
	g.setBrush(Color3ub(255, 255, 255));
		if (bStep)
	{
			g.drawRect(Vec2i(size.x / 4, size.y / 4), Vec2i(size.x / 6, size.y / 2));
			g.drawRect(Vec2i(size.x / 4 + size.x / 3, size.y / 4), Vec2i(size.x / 6, size.y / 2));
		}
		else
		{
			g.drawRect(Vec2i(size.x / 4, size.y / 4), Vec2i(size.x / 2, size.y / 2));
		}
	}
	else
	{
		Math::Vector2 vertices[] =
		{
			Math::Vector2((float)size.x / 4, (float)size.y / 4),
			Math::Vector2(3.0f * (float)size.x / 4, (float)size.y / 2),
			Math::Vector2((float)size.x / 4, 3.0f * (float)size.y / 4),
		};

		g.setPen(Color3ub(255, 255, 255));
		g.setBrush(Color3ub(255, 255, 255));
		
		if (bStep)
		{
			// Draw outline for step if not playing? 
			// Original code: g.enableBrush(!bStep);
			// I'll stick to original logic but adapted
			RenderUtility::SetBrush(g, EColor::Null);
			g.drawPolygon(vertices, ARRAY_SIZE(vertices));
			g.setBrush(Color3ub(255, 255, 255));
		}
		else
		{
			g.drawPolygon(vertices, ARRAY_SIZE(vertices));
		}
	}

	g.popXForm();
}

MinimizeButton::MinimizeButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
	:BaseClass(id, pos, size, parent)
{
}

void MinimizeButton::onRender()
{
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();
	IGraphics2D& g = Global::GetIGraphics2D();

	RenderUtility::SetPen(g, EColor::Null);
	if (mState == BS_PRESS)
	{
		g.setBrush(Color3ub(120, 120, 120));
	}
	else if (mState == BS_HIGHLIGHT)
	{
		g.setBrush(Color3ub(100, 100, 100));
	}
	else
	{
		g.setBrush(Color3ub(86, 86, 86));
	}
	g.drawRoundRect(pos, size, Vec2i(4, 4));

	g.setBrush(Color3ub(255, 255, 255));
	g.drawRect(pos + Vec2i(size.x / 4, size.y / 2 - 1), Vec2i(size.x / 2, 3));
}

CloseButton::CloseButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
	:BaseClass(id, pos, size, parent)
{
}

void CloseButton::onRender()
{
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();
	IGraphics2D& g = Global::GetIGraphics2D();

	RenderUtility::SetPen(g, EColor::Null);
	if (mState == BS_PRESS)
	{
		g.setBrush(Color3ub(190, 20, 20));
	}
	else if (mState == BS_HIGHLIGHT)
	{
		g.setBrush(Color3ub(150, 20, 20));
	}
	else
	{
		g.setBrush(Color3ub(86, 86, 86));
	}
	g.drawRoundRect(pos, size, Vec2i(4, 4));

	g.setPen(Color3ub(255, 255, 255), 2);
	g.drawLine(pos + Vec2i(size.x / 4, size.y / 4), pos + Vec2i(3 * size.x / 4, 3 * size.y / 4));
	g.drawLine(pos + Vec2i(size.x / 4, 3 * size.y / 4), pos + Vec2i(3 * size.x / 4, size.y / 4));
}
