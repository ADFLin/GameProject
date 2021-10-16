#pragma once
#ifndef RHIGraphics2D_H_B76821A9_0E45_4D52_8371_17DAF128C490
#define RHIGraphics2D_H_B76821A9_0E45_4D52_8371_17DAF128C490

#include "Graphics2DBase.h"
#include "Core/IntegerType.h"

#include "RHI/Font.h"
#include "RHI/SimpleRenderState.h"
#include "Renderer/RenderTransform2D.h"
#include "Renderer/BatchedRender2D.h"

#include "Math/Vector2.h"
#include "Math/Matrix2.h"
#include "Math/Matrix4.h"

#include <algorithm>
#include <vector>


enum class ESimpleBlendMode
{
	None,
	Translucent,
	Add,
	Multiply,
};


class RHIGraphics2D
{
public:
	RHIGraphics2D();

	using Vector2 = Math::Vector2;
	using RHITexture2D = Render::RHITexture2D;
	using RHISamplerState = Render::RHISamplerState;

	void  init(int w, int h);
	void  beginXForm();
	void  finishXForm();

	void  pushXForm();
	void  popXForm();

	void  identityXForm();
	void  translateXForm(float ox, float oy);
	void  rotateXForm(float angle);
	void  scaleXForm(float sx, float sy);

	void  transformXForm(Render::RenderTransform2D const& xform, bool bApplyPrev);

	void  beginFrame();
	void  endFrame();
	void  beginRender();

	void  initPiplineState();

	void  endRender();

	void  enablePen(bool beE) { mPaintArgs.bUsePen = beE; }
	void  enableBrush(bool beE) { mPaintArgs.bUseBrush = beE; }

	void  setPen(Color3f const& color)
	{
		mPaintArgs.bUsePen = true;
		mPaintArgs.penColor = Color4f(color, mPaintArgs.penColor.a);
	}
	void  setPen(Color3ub const& color, int width);
	void  setPenWidth(int width);

	void  setBrush(Color3f const& color)
	{
		mPaintArgs.bUseBrush = true;
		mPaintArgs.brushColor = Color4f(color,mPaintArgs.brushColor.a);
	}

	void  beginClip(Vec2i const& pos, Vec2i const& size);
	void  setClipRect(Vec2i const& pos, Vec2i const& size);
	void  endClip();

	void  beginBlend(Vector2 const& pos, Vector2 const& size, float alpha);
	void  beginBlend(float alpha, ESimpleBlendMode mode = ESimpleBlendMode::Translucent);
	void  endBlend();
	void  setBlendState(ESimpleBlendMode mode);
	void  setBlendAlapha(float value)
	{ 
		mPaintArgs.brushColor.a = value;
		mPaintArgs.penColor.a = value;
		mColorFont.a = value; 
	}

	void  drawPixel(Vector2 const& p, Color3ub const& color);
	void  drawLine(Vector2 const& p1, Vector2 const& p2);

	void  drawRect(int left, int top, int right, int bottom);
	void  drawRect(Vector2 const& pos, Vector2 const& size);
	void  drawCircle(Vector2 const& center, float r);
	void  drawEllipse(Vector2 const& center, Vector2 const& size);
	void  drawPolygon(Vector2 pos[], int num);
	void  drawPolygon(Vec2i pos[], int num);
	void  drawRoundRect(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleSize);

	void  fillGradientRect(Vector2 const& posLT, Color3ub const& colorLT,
		Vector2 const& posRB, Color3ub const& colorRB, bool bHGrad);


	void  setTexture(RHITexture2D& texture);
	void  setSampler(RHISamplerState& sampler);
	void  drawTexture(Vector2 const& pos);
	void  drawTexture(Vector2 const& pos, Vector2 const& size);
	void  drawTexture(Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize);
	void  drawTexture(Vector2 const& pos, Vector2 const& size, Vector2 const& texPos, Vector2 const& texSize);
	void  drawTexture(Vector2 const& pos, Color4f const& color);
	void  drawTexture(Vector2 const& pos, Vector2 const& size, Color4f const& color);
	void  drawTexture(Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize, Color4f const& color);
	void  drawTexture(Vector2 const& pos, Vector2 const& size, Vector2 const& texPos, Vector2 const& texSize, Color4f const& color);

	void  drawTexture(RHITexture2D& texture, Vector2 const& pos) { setTexture(texture); drawTexture(pos); }
	void  drawTexture(RHITexture2D& texture, Vector2 const& pos, Vector2 const& size) { setTexture(texture); drawTexture(pos, size); }
	void  drawTexture(RHITexture2D& texture, Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize) { setTexture(texture); drawTexture(pos , texPos , texSize); }
	void  drawTexture(RHITexture2D& texture, Vector2 const& pos, Vector2 const& size, Vector2 const& texPos, Vector2 const& texSize) { setTexture(texture); drawTexture(pos, size , texPos , texSize); }

	void  setFont(Render::FontDrawer& font)
	{
		mFont = &font;
	}
	void  setTextColor(Color3ub const& color);
	void  drawText(Vector2 const& pos, char const* str);
	void  drawText(Vector2 const& pos, wchar_t const* str);
	void  drawText(Vector2 const& pos, Vector2 const& size, char const* str, bool beClip = false);
	void  drawText(float x, float y, char const* str) { drawText(Vector2(x, y), str); }

	void comitRenderState();
	void restoreRenderState();
	void flush()
	{
		comitRenderState();
		flushBatchedElements();
	}

	Math::Matrix4 const& getBaseTransform() const
	{
		return mBaseTransform;
	}

	Render::RenderTransform2D const& getCurrentTransform() const
	{
		return mXFormStack.get();
	}

	bool bUseGraphicOnly = false;

private:
	void emitLineVertex(Vector2 const &p1, Vector2 const &p2);
	void emitVertex(Vector2 const& v);

	template< typename CharT >
	void drawTextImpl(float ox, float oy, CharT const* str);

	void drawPolygonBuffer();
	void drawLineBuffer();

	Render::RHICommandList& GetCommandList();

	void flushBatchedElements();
	void preModifyRenderState();
	void setTextureState(RHITexture2D* texture = nullptr);


	void setupElement(Render::RenderBachedElement& element)
	{
		element.transform = mXFormStack.get();
	}

	Render::ShapePaintArgs const& getPaintArgs()
	{
		return mPaintArgs;
	}
	Render::ShapePaintArgs mPaintArgs;

	struct RenderState
	{
		RHITexture2D*    texture;
		RHISamplerState* sampler;
		ESimpleBlendMode blendMode;
	};

	RenderState   mRenderStateCommitted;
	RenderState   mRenderStatePending;
	bool          mbPipelineStateChanged;
	bool          mbBlendStateChanged;
	Vector2       mCurTextureSize;

	int       mWidth;
	int       mHeight;

	Color4f   mColorFont;

	Render::FontDrawer*   mFont;
	std::vector< Vector2 >  mBuffer;

	Math::Matrix4   mBaseTransform;

	Render::TransformStack2D mXFormStack;
	Render::RenderBachedElementList mBachedElementList;
	Render::BatchedRender mBatchedRender;
};


#endif // RHIGraphics2D_H_B76821A9_0E45_4D52_8371_17DAF128C490