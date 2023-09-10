#pragma once
#ifndef GameGraphics2D_H_6F0D63F3_F483_45D5_BDFD_500E0688D9B4
#define GameGraphics2D_H_6F0D63F3_F483_45D5_BDFD_500E0688D9B4

#include "WinGDIRenderSystem.h"

#include "Core/Color.h"
#include "Math/Vector2.h"

using Math::Vector2;

class Graphics2D : public WinGdiGraphics2D
{
public:
	Graphics2D(HDC hDC = NULL) :WinGdiGraphics2D(hDC) {}

	using WinGdiGraphics2D::drawPolygon;
	void  drawPolygon(Vector2 pos[], int num)
	{
		Vec2i* pts = (Vec2i*)alloca(sizeof(Vec2i) * num);
		for (int i = 0; i < num; ++i)
		{
			pts[i].x = Math::FloorToInt(pos[i].x);
			pts[i].y = Math::FloorToInt(pos[i].y);
		}
		drawPolygon(pts, num);
	}
};

class RHIGraphics2D;

class IGraphics2D
{
public:
	virtual void  beginFrame() = 0;
	virtual void  endFrame() = 0;
	virtual void  beginRender() = 0;
	virtual void  endRender() = 0;
	virtual void  beginClip(Vec2i const& pos, Vec2i const& size) = 0;
	virtual void  endClip() = 0;
	virtual void  beginBlend(Vec2i const& pos, Vec2i const& size, float alpha) = 0;
	virtual void  endBlend() = 0;
	virtual void  setPen(Color3ub const& color, int width = 1) = 0;
	virtual void  setBrush(Color3ub const& color) = 0;
	virtual void  drawPixel(Vector2 const& p, Color3ub const& color) = 0;
	virtual void  drawLine(Vector2 const& p1, Vector2 const& p2) = 0;

	virtual void  drawRect(Vector2 const& pos, Vector2 const& size) = 0;
	virtual void  drawCircle(Vector2 const& center, float radius) = 0;
	virtual void  drawEllipse(Vector2 const& pos, Vector2 const& size) = 0;
	virtual void  drawRoundRect(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleSize) = 0;
	virtual void  drawPolygon(Vector2 pos[], int num) = 0;
	virtual void  setTextColor(Color3ub const& color) = 0;
	virtual void  drawText(Vector2 const& pos, char const* str) = 0;
	virtual void  drawText(Vector2 const& pos, Vector2 const& size, char const* str, bool beClip = false) = 0;

	virtual void  beginXForm() = 0;
	virtual void  finishXForm() = 0;
	virtual void  pushXForm() = 0;
	virtual void  popXForm() = 0;
	virtual void  identityXForm() = 0;
	virtual void  translateXForm(float ox, float oy) = 0;
	virtual void  rotateXForm(float angle) = 0;
	virtual void  scaleXForm(float sx, float sy) = 0;

	virtual bool  isUseRHI() const = 0;

	virtual void* getImplPtr() const = 0;

	template< typename TGraphic >
	TGraphic& getImpl() const { return *static_cast<TGraphic*>(getImplPtr()); }

	void  drawRect(int left, int top, int right, int bottom)
	{
		drawRect(Vector2(left, top), Vector2(right - left, bottom - right));
	}
	class Visitor
	{
	public:
		virtual void visit(Graphics2D& g) = 0;
		virtual void visit(RHIGraphics2D& g) = 0;
	};
	virtual void  accept(Visitor& visitor) = 0;
};


#endif // GameGraphics2D_H_6F0D63F3_F483_45D5_BDFD_500E0688D9B4
