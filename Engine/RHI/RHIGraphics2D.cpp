#include "RHIGraphics2D.h"

#include "Math/Base.h"

#include "RHI/RHICommon.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/DrawUtility.h"

#include <cassert>
#include <algorithm>

#include "ConsoleSystem.h"
#include "ProfileSystem.h"

using namespace Render;

RHIGraphics2D::RHIGraphics2D()
	:mAllocator(2048)
	,mElementList(mAllocator)
{

	mPaintArgs.penWidth = 1;
	mPaintArgs.bUseBrush = true;
	mPaintArgs.bUsePen = true;
	mPaintArgs.brushColor = Color4Type(0, 0, 0);
	mPaintArgs.penColor = Color4Type(0, 0, 0);

	mFont = nullptr;
	mColorFont = Color4Type(0, 0, 0);

	mRenderStateCommitted.setInit();
	mRenderStatePending = mRenderStateCommitted;
	mDirtyState.value = 0;
}

void RHIGraphics2D::initializeRHI()
{
	mBatchedRender.initializeRHI();
}

void RHIGraphics2D::releaseRHI()
{
	mBatchedRender.releaseRHI();
	mRenderStateCommitted.setInit();
	mRenderStatePending = mRenderStateCommitted;
	mDirtyState.value = 0;
}

void RHIGraphics2D::setViewportSize(int w, int h)
{
	mBatchedRender.setViewportSize(w, h);
}

void RHIGraphics2D::beginXForm()
{

}

void RHIGraphics2D::finishXForm()
{
	if (!mXFormStack.mStack.empty())
	{
		LogWarning(0, "push/pop transform is not match");
	}
	mXFormStack.clear();
}

void RHIGraphics2D::pushXForm()
{
	mXFormStack.push();
}

void RHIGraphics2D::popXForm()
{
	mXFormStack.pop();
}

void RHIGraphics2D::identityXForm()
{
	mXFormStack.set(RenderTransform2D::Identity());
}

void RHIGraphics2D::translateXForm(float ox, float oy)
{
	mXFormStack.translate(Vector2(ox, oy));
}

void RHIGraphics2D::rotateXForm(float angle)
{
	mXFormStack.rotate(angle);
}

void RHIGraphics2D::scaleXForm(float sx, float sy)
{
	mXFormStack.scale(Vector2(sx, sy));
}


void RHIGraphics2D::transformXForm(Render::RenderTransform2D const& xform, bool bApplyPrev)
{
	if (bApplyPrev)
	{
		mXFormStack.transform(xform);
	}
	else
	{
		mXFormStack.set(xform);
	}
}


void RHIGraphics2D::beginFrame()
{

}

void RHIGraphics2D::endFrame()
{

}

void RHIGraphics2D::beginRender()
{
	RHICommandList& commandList = getCommandList();

	mRenderStateCommitted.setInit();
	mRenderStateCommitted.bEnableMultiSample = mRenderStatePending.bEnableMultiSample;
	mRenderStatePending = mRenderStateCommitted;
	mDirtyState.value = 0;
	mCurTextureSize = Vector2(0, 0);
	mXFormStack.clear();
	mBatchedRender.beginRender(commandList);

	setupCommittedRenderState();
}

void RHIGraphics2D::endRender()
{
	mBatchedRender.endRender();
	flush();
}

void RHIGraphics2D::setupCommittedRenderState()
{
	RHICommandList& commandList = getCommandList();
	mBatchedRender.commitRenderState(commandList, mRenderStateCommitted);
}

void RHIGraphics2D::commitRenderState()
{
	mDirtyState.blend |= mRenderStateCommitted.blendMode != mRenderStatePending.blendMode;
	mDirtyState.rasterizer |= mRenderStateCommitted.bEnableScissor != mRenderStatePending.bEnableScissor || 
		mRenderStateCommitted.bEnableMultiSample != mRenderStatePending.bEnableMultiSample;
	mDirtyState.pipeline |= mRenderStateCommitted.texture != mRenderStatePending.texture;
	mDirtyState.scissorRect = mRenderStateCommitted.scissorRect != mRenderStatePending.scissorRect;
	if (mDirtyState.value)
	{
		preModifyRenderState();

		if (mDirtyState.pipeline)
		{
			mRenderStateCommitted.texture = mRenderStatePending.texture;
			mRenderStateCommitted.sampler = mRenderStatePending.sampler;
		}
		if (mDirtyState.blend)
		{
			mRenderStateCommitted.blendMode = mRenderStatePending.blendMode;
		}
		if (mDirtyState.rasterizer)
		{
			mRenderStateCommitted.bEnableMultiSample = mRenderStatePending.bEnableMultiSample;
			mRenderStateCommitted.bEnableScissor = mRenderStatePending.bEnableScissor;
		}
		if (mDirtyState.scissorRect)
		{
			mRenderStateCommitted.scissorRect = mRenderStatePending.scissorRect;
		}

		mDirtyState.value = 0;
	}
}

void RHIGraphics2D::drawPixel(Vector2 const& p, Color3Type const& color)
{
	flushBatchedElements();

	setTextureState();
	commitRenderState();
	struct Vertex_XY_CA
	{
		Math::Vector2 pos;
		Color4Type c;
	} v = { p , Color4Type(color , mPaintArgs.brushColor.a) };
	TRenderRT<RTVF_XY_CA8>::Draw(getCommandList(), EPrimitive::Points, &v, 1);
}

void RHIGraphics2D::drawRect(int left, int top, int right, int bottom)
{
	setTextureState();
	commitRenderState();

	Vector2 p1(left, top);
	Vector2 p2(right, bottom);
	auto& element = mElementList.addRect(getPaintArgs(), p1, p2);
	setupElement(element);
}

void RHIGraphics2D::drawRect(Vector2 const& pos, Vector2 const& size)
{
	setTextureState();
	commitRenderState();

	auto& element = mElementList.addRect(getPaintArgs(), pos, pos + size);
	setupElement(element);
}

void RHIGraphics2D::drawCircle(Vector2 const& center, float r)
{
	setTextureState();
	commitRenderState();

	auto& element = mElementList.addCircle(getPaintArgs(), center, r);
	setupElement(element);
}

void RHIGraphics2D::drawEllipse(Vector2 const& center, Vector2 const& size)
{
	setTextureState();
	commitRenderState();

	auto& element = mElementList.addEllipse(getPaintArgs(), center, size );
	setupElement(element);
}

void RHIGraphics2D::drawPolygon(Vector2 const pos[], int num)
{
	setTextureState();
	commitRenderState();

	auto& element = mElementList.addPolygon(mXFormStack.get(), getPaintArgs(), pos, num);
	setupElement(element);
}

void RHIGraphics2D::drawPolygon(Vec2i const pos[], int num)
{
	setTextureState();
	commitRenderState();

	auto& element = mElementList.addPolygon(mXFormStack.get(), getPaintArgs(), pos, num);
	setupElement(element);
}

void RHIGraphics2D::drawLine(Vector2 const& p1, Vector2 const& p2)
{
	setTextureState();
	commitRenderState();

	auto& element = mElementList.addLine(mPaintArgs.penColor, p1, p2, mPaintArgs.penWidth);
	setupElement(element);
}


void RHIGraphics2D::drawLineStrip(Vector2 const pos[], int num)
{
	setTextureState();
	commitRenderState();

	auto& element = mElementList.addLineStrip(mXFormStack.get(), mPaintArgs.penColor, pos, num, mPaintArgs.penWidth);
	setupElement(element);
}

void RHIGraphics2D::drawArcLine(Vector2 const& pos, float r, float startAngle, float sweepAngle)
{
	setTextureState();
	commitRenderState();

	auto& element = mElementList.addArcLine( mPaintArgs.penColor, pos, r, startAngle, sweepAngle, mPaintArgs.penWidth);
	setupElement(element);
}

void RHIGraphics2D::drawRoundRect(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleSize)
{
	setTextureState();
	commitRenderState();

	auto& element = mElementList.addRoundRect( getPaintArgs(),  pos, rectSize, circleSize / 2);
	setupElement(element);
}

void RHIGraphics2D::drawGradientRect(Vector2 const& posLT, Color3Type const& colorLT, Vector2 const& posRB, Color3Type const& colorRB, bool bHGrad)
{
	setTextureState();
	commitRenderState();

	auto& element = mElementList.addGradientRect(posLT, colorLT, posRB, colorRB, bHGrad);
	setupElement(element);
}

void RHIGraphics2D::setTextColor(Color3Type const& color)
{
	mColorFont = Color4Type( Color3Type( color ) , mColorFont.a );
}

void RHIGraphics2D::drawText(Vector2 const& pos, char const* str)
{
	if (!mFont || !mFont->isValid())
		return;

	float ox = pos.x;
	float oy = pos.y;
	drawTextImpl(ox, oy, str);
}

void RHIGraphics2D::drawText(Vector2 const& pos, wchar_t const* str)
{
	if (!mFont || !mFont->isValid())
		return;

	float ox = pos.x;
	float oy = pos.y;
	drawTextImpl(ox, oy, str);
}

void RHIGraphics2D::drawText(Vector2 const& pos, Vector2 const& size, char const* str,
	EHorizontalAlign alignH, EVerticalAlign alignV, bool bClip)
{
	if (!mFont || !mFont->isValid())
		return;

	auto DoDrawText = [&]()
	{
		int charCount = 0;
		Vector2 extent = mFont->calcTextExtent(str, &charCount);
		Vector2 renderPos = pos;
		switch (alignH)
		{
		case EHorizontalAlign::Right: 
			renderPos.x += size.x - extent.x;
			break;
		case EHorizontalAlign::Center:
		case EHorizontalAlign::Fill:
			renderPos.x += 0.5 * ( size.x - extent.x );
			break;
		}
		switch (alignV)
		{
		case EVerticalAlign::Bottom:
			renderPos.y += size.y - extent.y;
			break;
		case EVerticalAlign::Center:
		case EVerticalAlign::Fill:
			renderPos.y += 0.5 * (size.y - extent.y);
			break;
		}
		drawTextImpl(renderPos.x, renderPos.y, str, charCount);
	};

	if (bClip)
	{
		if (mRenderStatePending.bEnableScissor)
		{
			Rect rect = Rect::Intersect(mRenderStatePending.scissorRect, { pos , size });
			if (!rect.isValid())
				return;

			Rect prevRect = mRenderStatePending.scissorRect;
			setClipRect(rect.pos, rect.size);
			DoDrawText();
			setClipRect(prevRect.pos, prevRect.size);
		}
		else
		{
			beginClip(pos, size);
			DoDrawText();
			endClip();
		}
	}
	else
	{
		DoDrawText();
	}


}

void RHIGraphics2D::drawText(Vector2 const& pos, Vector2 const& size, char const* str, bool bClip /*= false */)
{
	if (!mFont || !mFont->isValid())
		return;

	auto DoDrawText = [&]()
	{
		int charCount = 0;
		Vector2 extent = mFont->calcTextExtent(str, &charCount);
		Vector2 renderPos = pos + (size - extent) / 2;
		drawTextImpl(renderPos.x, renderPos.y, str, charCount);
	};

	if (bClip)
	{
		if (mRenderStatePending.bEnableScissor)
		{
			Rect rect = Rect::Intersect(mRenderStatePending.scissorRect, { pos , size });
			if (!rect.isValid())
				return;

			Rect prevRect = mRenderStatePending.scissorRect;
			setClipRect(rect.pos, rect.size);
			DoDrawText();
			setClipRect(prevRect.pos, prevRect.size);
		}
		else
		{
			beginClip(pos, size);
			DoDrawText();
			endClip();
		}
	}
	else
	{
		DoDrawText();
	}
}

template< typename CharT >
void RHIGraphics2D::drawTextImpl(float ox, float oy, CharT const* str, int charCount)
{
	assert(mFont);

	ESimpleBlendMode prevMode = mRenderStateCommitted.blendMode;
	setBlendState(ESimpleBlendMode::Translucent);
	setTextureState(&mFont->getTexture());
	commitRenderState();
	auto& element = mElementList.addText(mColorFont, Vector2(int(ox), int(oy)), *mFont, str, charCount);
	setupElement(element);
	setBlendState(prevMode);
}

void RHIGraphics2D::drawTexture(Vector2 const& pos)
{
	drawTexture(pos, mCurTextureSize, mPaintArgs.brushColor);
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& size)
{
	drawTexture(pos, size, mPaintArgs.brushColor);
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize)
{
	drawTexture(pos, mCurTextureSize, texPos, texSize, mPaintArgs.brushColor);
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& size, Vector2 const& texPos, Vector2 const& texSize)
{
	drawTexture(pos, size, texPos, texSize, mPaintArgs.brushColor);
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Color4f const& color)
{
	drawTexture(pos, mCurTextureSize, color);
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& size, Color4f const& color)
{
	commitRenderState();
	auto& element = mElementList.addTextureRect(color, pos, pos + size, Vector2(0, 0), Vector2(1, 1));
	setupElement(element);
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize, Color4f const& color)
{
	drawTexture(pos, mCurTextureSize, texPos, texSize);
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& size, Vector2 const& texPos, Vector2 const& texSize, Color4f const& color)
{
	commitRenderState();
	auto& element = mElementList.addTextureRect(color, pos, pos + size, texPos, texPos + texSize);
	setupElement(element);
}

void RHIGraphics2D::flushBatchedElements()
{
	if ( !mElementList.isEmpty() )
	{
		mBatchedRender.render(mRenderStateCommitted, mElementList);
		mNextLayer = 0;
	}
}

RHICommandList& RHIGraphics2D::getCommandList()
{
	return RHICommandList::GetImmediateList();
}

void RHIGraphics2D::setTextureState(RHITexture2D* texture /*= nullptr*/)
{
	mRenderStatePending.texture = texture;

}

void RHIGraphics2D::setTexture(RHITexture2D& texture)
{
	setTextureState(&texture);
	mCurTextureSize = Vector2(mRenderStatePending.texture->getSizeX(), mRenderStatePending.texture->getSizeY());
}

void RHIGraphics2D::setSampler(RHISamplerState& sampler)
{
	mRenderStatePending.sampler = &sampler;
	mDirtyState.pipeline |= mRenderStateCommitted.sampler != mRenderStatePending.sampler;
}

void RHIGraphics2D::preModifyRenderState()
{
	flushBatchedElements();
}

void RHIGraphics2D::setBlendState(ESimpleBlendMode mode)
{
	mRenderStatePending.blendMode = mode;
}

void RHIGraphics2D::restoreRenderState()
{
	preModifyRenderState();
	setupCommittedRenderState();
	mBatchedRender.setupInputState(getCommandList());
}

void RHIGraphics2D::setPen(Color3Type const& color, int width)
{
	mPaintArgs.bUsePen = true;
	mPaintArgs.penColor = Color4Type(Color3Type(color), mPaintArgs.penColor.a);
	if (mPaintArgs.penWidth != width)
	{
		if (GRHISystem->getName() == RHISystemName::OpenGL)
		{
			glLineWidth(width);
		}
		mPaintArgs.penWidth = width;
	}
}

void RHIGraphics2D::setPenWidth(int width)
{
	mPaintArgs.penWidth = width;
}

void RHIGraphics2D::beginClip(Vec2i const& pos, Vec2i const& size)
{
	mRenderStatePending.bEnableScissor = true;
	setClipRect(pos, size);
}

void RHIGraphics2D::setClipRect(Vec2i const& pos, Vec2i const& size)
{
	mRenderStatePending.scissorRect.pos  = pos;
	mRenderStatePending.scissorRect.size = size;

}

void RHIGraphics2D::endClip()
{
	mRenderStatePending.bEnableScissor = false;
}

void RHIGraphics2D::beginBlend(Vector2 const& pos, Vector2 const& size, float alpha)
{
	setBlendState(ESimpleBlendMode::Translucent);
	setBlendAlpha(alpha);
}

void RHIGraphics2D::beginBlend(float alpha, ESimpleBlendMode mode)
{
	setBlendState(mode);
	setBlendAlpha(alpha);
}

void RHIGraphics2D::endBlend()
{
	setBlendState(ESimpleBlendMode::None);
	setBlendAlpha(1.0f);
}

void RHIGraphics2D::flush()
{
	commitRenderState();
	flushBatchedElements();
	mBatchedRender.flush();
	mAllocator.clearFrame();
}
