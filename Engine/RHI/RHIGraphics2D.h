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


using Render::ESimpleBlendMode;

enum class EVerticalAlign
{
	Top,
	Bottom,
	Center,
	Fill,
};


enum class EHorizontalAlign
{
	Left,
	Right,
	Center,
	Fill,
};

class RHIGraphics2D : public Render::GraphicsDefinition
{
public:
	RHIGraphics2D();

	using Vector2 = Math::Vector2;
	using RHITexture2D = Render::RHITexture2D;
	using RHISamplerState = Render::RHISamplerState;

	using Color4Type = Render::ShapePaintArgs::Color4Type;
	using Color3Type = Render::ShapePaintArgs::Color3Type;

	void  setViewportSize(int w, int h);
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
	void  setBlendAlpha(float value);
	void  enableMultisample(bool bEnabled)
	{
		mRenderStatePending.bEnableMultiSample = bEnabled;
	}

	void  drawPixel(Vector2 const& p, Color3Type const& color);
	void  drawLine(Vector2 const& p1, Vector2 const& p2);
	void  drawLineStrip(Vector2 const pos[], int num);
	void  drawArcLine(Vector2 const& pos, float r, float startAngle, float sweepAngle);

	void  drawRect(int left, int top, int right, int bottom);
	void  drawRect(Vector2 const& pos, Vector2 const& size);
	void  drawCircle(Vector2 const& center, float r);
	void  drawEllipse(Vector2 const& center, Vector2 const& size);
	void  drawPolygon(Vector2 const pos[], int num);
	void  drawPolygon(Vec2i const pos[], int num);
	void  drawTriangleList(Vector2 const pos[], int num);
	void  drawTriangleList(Vec2i const pos[], int num);
	void  drawTriangleStrip(Vector2 const pos[], int num);
	void  drawTriangleStrip(Vec2i const pos[], int num);

	void  drawRoundRect(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleSize);

	void  drawGradientRect(Vector2 const& posLT, Color3Type const& colorLT,
		Vector2 const& posRB, Color3Type const& colorRB, bool bHGrad);

	template< typename TCustomRenderer , typename ...TArgs >
	TCustomRenderer*  drawCustom(bool bChangeState, TArgs&& ...args)
	{
		commitRenderState();

		void* ptr = mAllocator.alloc(sizeof(TCustomRenderer));
		new (ptr) TCustomRenderer(std::forward<TArgs>(args)...);
		auto& element = mElementList.addCustomRender((Render::ICustomElementRenderer*)ptr, Render::EObjectManageMode::DestructOnly, bChangeState);

		setupElement(element);
		return (TCustomRenderer*)ptr;
	}

	template< typename TFunc >
	class TCustomFuncRenderer : public Render::ICustomElementRenderer
	{
	public:
		TCustomFuncRenderer(TFunc&& func)
			:mFunc(std::forward<TFunc>(func))
		{
		}
		void render(Render::RHICommandList& commandList, Render::RenderBatchedElement& element, RenderState const& state) override
		{
			if constexpr (TCheckConcept< CFunctionCallable, TFunc, Render::RHICommandList&, Render::RenderBatchedElement&, RenderState const&>::Value)
			{
				mFunc(commandList, element, state);
			}
			else
			{
				mFunc(commandList, element);
			}
		}
		TFunc mFunc;
	};
	template< typename TFunc >
	void  drawCustomFunc(TFunc&& func, bool bChangeState = true)
	{
		commitRenderState();

		using MyCustomRenderer = TCustomFuncRenderer<TFunc>;
		void* ptr = mAllocator.alloc(sizeof(MyCustomRenderer));
		new (ptr) MyCustomRenderer(std::forward<TFunc>(func));
		auto& element = mElementList.addCustomRender(static_cast<Render::ICustomElementRenderer*>(ptr), Render::EObjectManageMode::DestructOnly, bChangeState);

		setupElement(element);
	}

	template< typename TFunc >
	void  setCustomRenderState(TFunc&& func)
	{
		commitRenderState();

		using MyCustomRenderer = TCustomFuncRenderer<TFunc>;
		void* ptr = mAllocator.alloc(sizeof(MyCustomRenderer));
		new (ptr) MyCustomRenderer(std::forward<TFunc>(func));

		auto& element = mElementList.addCustomState(static_cast<Render::ICustomElementRenderer*>(ptr), Render::EObjectManageMode::DestructOnly);
		
		setupElement(element);
	}

	void resetRenderState()
	{
		commitRenderState();

		auto& element = mElementList.addCustomState(nullptr, Render::EObjectManageMode::None);

		setupElement(element);
	}

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
	void  drawText(Vector2 const& pos, Vector2 const& size, char const* str)
	{
		drawText(pos, size, str, false);
	}
	void  drawText(Vector2 const& pos, Vector2 const& size, char const* str, bool bClip);
	void  drawText(Vector2 const& pos, Vector2 const& size, char const* str, EHorizontalAlign alignH, EVerticalAlign alignV, bool bClip = false);
	void  drawText(float x, float y, char const* str) { drawText(Vector2(x, y), str); }

	template< typename CharT, typename ...Args >
	void  drawTextF(Vector2 const& pos, CharT const* format, Args&& ...args)
	{
		TInlineString< 512, CharT > str;
		str.format(format, std::forward<Args>(args)...);
		drawText(pos, str.c_str());
	}

	void commitRenderState();
	void restoreRenderState();


	void flush();

	Math::Matrix4 const& getBaseTransform() const
	{
		return mBatchedRender.mBaseTransform;
	}

	Render::RenderTransform2D const& getCurrentTransform() const
	{
		return mXFormStack.get();
	}

	Render::TransformStack2D& getTransformStack() { return mXFormStack; }

	Render::RHICommandList& getCommandList();

	void initializeRHI();
	void releaseRHI();

	Color4Type getBrushColor() const
	{
		return mPaintArgs.brushColor;
	}
	void setTextureState(RHITexture2D* texture = nullptr);

	void setTextRemoveScale(bool bRemoved) { bTextRemoveScale = bRemoved; }
	void setTextRemoveRotation(bool bRemoved) { bTextRemoveRotation = bRemoved; }
private:

	template< typename CharT >
	void drawTextImpl(float ox, float oy, CharT const* str, int charCount = INDEX_NONE);

	void setupCommittedRenderState();

	void flushBatchedElements();
	void preModifyRenderState();

	bool bTextRemoveScale = false;
	bool bTextRemoveRotation = false;


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

	RenderState   mRenderStateCommitted;
	RenderState   mRenderStatePending;
	StateFlags    mDirtyState;
	Vector2       mCurTextureSize;
	int32         mNextLayer;



	Color4Type   mColorFont;

	Render::FontDrawer*   mFont;


	FrameAllocator mAllocator;
	Render::TransformStack2D mXFormStack;
	Render::RenderBatchedElementList mElementList;
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