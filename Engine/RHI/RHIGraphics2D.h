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
#include "DataStructure/Array.h"

#include <algorithm>


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

	using Color4Type = Render::ShapePaintArgs::Color4Type;
	using Color3Type = Render::ShapePaintArgs::Color3Type;

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

	void  endRender();

	void  enablePen(bool beE) { mPaintArgs.bUsePen = beE; }
	void  enableBrush(bool beE) { mPaintArgs.bUseBrush = beE; }
	void  setPen(Color3Type const& color)
	{
		mPaintArgs.bUsePen = true;
		mPaintArgs.penColor = Color4Type(color, mPaintArgs.penColor.a);
	}
	void  setPen(Color3Type const& color, int width);
	void  setPenWidth(int width);

	void  setBrush(Color3Type const& color)
	{
		mPaintArgs.bUseBrush = true;
		mPaintArgs.brushColor = Color4Type(color,mPaintArgs.brushColor.a);
	}

	void  beginClip(Vec2i const& pos, Vec2i const& size);
	void  setClipRect(Vec2i const& pos, Vec2i const& size);
	void  endClip();

	void  beginBlend(Vector2 const& pos, Vector2 const& size, float alpha);
	void  beginBlend(float alpha, ESimpleBlendMode mode = ESimpleBlendMode::Translucent);
	void  endBlend();
	void  setBlendState(ESimpleBlendMode mode);
	void  setBlendAlpha(float value)
	{ 
		if constexpr (Meta::IsSameType< Color4Type, Color4f >::Value)
		{
			mPaintArgs.brushColor.a = value;
			mPaintArgs.penColor.a = value;
			mColorFont.a = value;
		}
		else
		{
			uint8 byteValue = uint32(value * 255) & 0xff;
			mPaintArgs.brushColor.a = byteValue;
			mPaintArgs.penColor.a = byteValue;
			mColorFont.a = byteValue;
		}
	}
	void  enableMultisample(bool bEnabled)
	{
		mRenderStatePending.bEnableMultiSample = bEnabled;
	}

	void  drawPixel(Vector2 const& p, Color3Type const& color);
	void  drawLine(Vector2 const& p1, Vector2 const& p2);
	void  drawLineStrip(Vector2 pos[], int num);

	void  drawRect(int left, int top, int right, int bottom);
	void  drawRect(Vector2 const& pos, Vector2 const& size);
	void  drawCircle(Vector2 const& center, float r);
	void  drawEllipse(Vector2 const& center, Vector2 const& size);
	void  drawPolygon(Vector2 pos[], int num);
	void  drawPolygon(Vec2i pos[], int num);
	void  drawRoundRect(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleSize);

	void  drawGradientRect(Vector2 const& posLT, Color3Type const& colorLT,
		Vector2 const& posRB, Color3Type const& colorRB, bool bHGrad);


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
	void  setTextColor(Color3Type const& color);
	void  drawText(Vector2 const& pos, char const* str);
	void  drawText(Vector2 const& pos, wchar_t const* str);
	void  drawText(Vector2 const& pos, Vector2 const& size, char const* str, bool bClip = false);
	void  drawText(float x, float y, char const* str) { drawText(Vector2(x, y), str); }

	void commitRenderState();
	void restoreRenderState();


	void flush()
	{
		commitRenderState();
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

	Render::TransformStack2D& getTransformStack() { return mXFormStack; }

	bool bUseGraphicOnly = false;

	Render::RHICommandList& getCommandList();

	void initializeRHI();
	void releaseRHI();

private:
	void emitLineVertex(Vector2 const &p1, Vector2 const &p2);
	void emitVertex(Vector2 const& v);

	template< typename CharT >
	void drawTextImpl(float ox, float oy, CharT const* str);

	void drawPolygonBuffer();
	void drawLineBuffer();


	void initRenderState();
	void setupCommittedRenderState();


	void flushBatchedElements();
	void preModifyRenderState();
	void setTextureState(RHITexture2D* texture = nullptr);


	void setupElement(Render::RenderBatchedElement& element)
	{
		element.transform = mXFormStack.get();
		element.layer = mNextLayer;
		++mNextLayer;
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
		bool bEnableMultiSample;
		bool bEnableScissor;
	};

	RenderState   mRenderStateCommitted;
	RenderState   mRenderStatePending;
	struct StateFlags
	{
		union 
		{
			struct
			{
				uint8 pipeline : 1;
				uint8 scissorRect : 1;
				uint8 blend : 1;
				uint8 rasterizer : 1;
			};
			uint8 value;
		};
	};
	StateFlags    mDirtyState;
	Vector2       mCurTextureSize;
	int32         mNextLayer;
	struct Rect 
	{
		Vec2i pos;
		Vec2i size;
	};
	Rect      mScissorRect;

	int       mWidth;
	int       mHeight;


	Color4Type   mColorFont;

	Render::FontDrawer*   mFont;
	TArray< Vector2 >  mBuffer;

	Math::Matrix4   mBaseTransform;

	Render::TransformStack2D mXFormStack;
	Render::RenderBachedElementList mBachedElementList;
	Render::BatchedRender mBatchedRender;
};


struct GrapthicStateScope
{
	GrapthicStateScope(RHIGraphics2D& g, bool bNeedFlash = true)
		:mGraphics(g)
	{
		if (bNeedFlash)
		{
			mGraphics.flush();
		}
	}

	~GrapthicStateScope()
	{
		mGraphics.restoreRenderState();
	}

	RHIGraphics2D& mGraphics;
};



#endif // RHIGraphics2D_H_B76821A9_0E45_4D52_8371_17DAF128C490