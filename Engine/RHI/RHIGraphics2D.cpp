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
#include "Renderer/RenderThread.h"
#include "RHI/Font.h"
#include "PlatformThread.h"

using namespace Render;

#include "Singleton.h"
#include "CoreShare.h"

struct RHI2DContext
{
	FrameAllocator allocator;
	Render::RenderBatchedElementList elementList;
	Math::Matrix4 baseTransform;
	int viewportWidth;
	int viewportHeight;

	RHI2DContext(uint32 pageSize)
		:allocator(pageSize)
		,elementList(allocator)
	{
		baseTransform = Math::Matrix4::Identity();
		viewportWidth = 0;
		viewportHeight = 0;
	}

	void reset()
	{
		elementList.reset();
		allocator.clearFrame();
	}
};

class RHIGraphicsBatchManager
{
public:
	CORE_API static RHIGraphicsBatchManager& Get();

	BatchedRender mBatchedRender;
	TArray<RHI2DContext*> mFreeContexts;
	TArray<RHI2DContext*> mUsedContexts;
	Mutex mContextMutex;

	RHI2DContext* acquire()
	{
		Mutex::Locker lock(mContextMutex);
		if (!mFreeContexts.empty())
		{
			auto* context = mFreeContexts.back();
			mFreeContexts.pop_back();
			mUsedContexts.push_back(context);
			return context;
		}
		auto* context = new RHI2DContext(2048);
		mUsedContexts.push_back(context);
		return context;
	}

	void release(RHI2DContext* context)
	{
		context->reset();
		Mutex::Locker lock(mContextMutex);
		auto iter = std::find(mUsedContexts.begin(), mUsedContexts.end(), context);
		if (iter != mUsedContexts.end())
		{
			mUsedContexts.erase(iter);
			mFreeContexts.push_back(context);
		}
	}

	int initializeCount = 0;

	void initializeRHI()
	{
		if (initializeCount == 0)
		{
			mBatchedRender.initializeRHI();
		}
		++initializeCount;
	}

	void releaseRHI()
	{
		--initializeCount;
		if (initializeCount == 0)
		{
			mBatchedRender.releaseRHI();
		}
	}
};

#if CORE_SHARE_CODE
RHIGraphicsBatchManager& RHIGraphicsBatchManager::Get()
{
	static RHIGraphicsBatchManager StaticInstance;
	return StaticInstance;
}
#endif


RHIGraphics2D::RHIGraphics2D()
{ 
	mImmediateContext = new RHI2DContext(2048);
	mWriteContext = mImmediateContext;
	mFont = nullptr;
	mColorFont = Color4Type(0, 0, 0);

	mRenderStateCommitted.setInit();
	mRenderStatePending = mRenderStateCommitted;
	mDirtyState.value = 0;

	mBaseTransform = Math::Matrix4::Identity();
}

RHIGraphics2D::~RHIGraphics2D()
{
	delete mImmediateContext;
}


void RHIGraphics2D::initializeRHI()
{
	RHIGraphicsBatchManager::Get().initializeRHI();
}

void RHIGraphics2D::releaseRHI()
{
	RHIGraphicsBatchManager::Get().releaseRHI();
	mRenderStateCommitted.setInit();
	mRenderStatePending = mRenderStateCommitted;
	mDirtyState.value = 0;
}


void RHIGraphics2D::setViewportSize(int w, int h)
{
	//LogMsg("RHIGraphics2D::setViewportSize [w: %d, h: %d]", w, h);
	mViewportWidth = w;
	mViewportHeight = h;
	mBaseTransform = AdjustProjectionMatrixForRHI(OrthoMatrix(0, (float)w, (float)h, 0, -1, 1));

	if (mRenderMode == ERenderMode::Immediate)
	{
		RHIGraphicsBatchManager::Get().mBatchedRender.setViewportSize(w, h);
	}


	if (mWriteContext)
	{
		mWriteContext->viewportWidth = w;
		mWriteContext->viewportHeight = h;
		mWriteContext->baseTransform = mBaseTransform;
	}
}

void RHIGraphics2D::syncTransform()
{
	if (bTransformDirty)
	{
		mCurrentTransformIndex = getElementList().setTransform(mXFormStack.get());
		bTransformDirty = false;
	}
}

Render::RenderBatchedElementList& RHIGraphics2D::getElementList()
{
	return mWriteContext->elementList;
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
	mTransformIndexStack.push_back(mCurrentTransformIndex);
}

void RHIGraphics2D::popXForm()
{
	mXFormStack.pop();
	mCurrentTransformIndex = mTransformIndexStack.back();
	mTransformIndexStack.pop_back();
	bTransformDirty = false;
}

void RHIGraphics2D::identityXForm()
{
	mXFormStack.set(RenderTransform2D::Identity());
	bTransformDirty = true;
}

void RHIGraphics2D::translateXForm(float ox, float oy)
{
	mXFormStack.translate(Vector2(ox, oy));
	bTransformDirty = true;
}

void RHIGraphics2D::rotateXForm(float angle)
{
	mXFormStack.rotate(angle);
	bTransformDirty = true;
}

void RHIGraphics2D::scaleXForm(float sx, float sy)
{
	mXFormStack.scale(Vector2(sx, sy));
	bTransformDirty = true;
}

void RHIGraphics2D::transformXForm(Render::RenderTransform2D const& xform, bool bReset)
{
	if (bReset)
	{
		mXFormStack.set(xform);
	}
	else
	{
		mXFormStack.transform(xform);
	}
	bTransformDirty = true;
}

void RHIGraphics2D::beginFrame()
{
	//LogMsg("RHIGraphics2D::beginFrame");
	mFlushCount = 0;
}

void RHIGraphics2D::endFrame()
{
	//LogMsg("RHIGraphics2D::endFrame");
}

void RHIGraphics2D::beginRender()
{
	//LogMsg("RHIGraphics2D::beginRender");
	mRenderStateCommitted.setInit();
	mRenderStatePending = mRenderStateCommitted;
	mDirtyState.value = 0;
	mCurTextureSize = Vector2(0, 0);
	mXFormStack.clear();
	mTransformIndexStack.clear();
	mCurrentTransformIndex = 0;
	bTransformDirty = false;
	mNextLayer = 0;

	mWriteContext->baseTransform = mBaseTransform;
	mWriteContext->viewportWidth = mViewportWidth;
	mWriteContext->viewportHeight = mViewportHeight;

	if (mRenderMode == ERenderMode::Immediate)
	{
		RHICommandList& commandList = getCommandList();
		RHIGraphicsBatchManager::Get().mBatchedRender.beginRender(commandList);
	}


	mPaintArgs.penWidth = 1;
	mPaintArgs.bUseBrush = true;
	mPaintArgs.bUsePen = true;
	mPaintArgs.brushColor = Color4Type(0, 0, 0);
	mPaintArgs.penColor = Color4Type(0, 0, 0);

	setupCommittedRenderState();
}

void RHIGraphics2D::endRender()
{
	//LogMsg("RHIGraphics2D::endRender");
	PROFILE_ENTRY("RHIGraphics2D.endRender");
	flush();
}

void RHIGraphics2D::setupCommittedRenderState()
{
	mCurrentStateIndex = getElementList().setState(mRenderStateCommitted);
	bStateDirty = false;
}


void RHIGraphics2D::commitRenderState()
{
	mDirtyState.blend |= mRenderStateCommitted.blendMode != mRenderStatePending.blendMode;
	mDirtyState.rasterizer |= mRenderStateCommitted.bEnableScissor != mRenderStatePending.bEnableScissor;
	mDirtyState.pipeline |= mRenderStateCommitted.texture != mRenderStatePending.texture;
	mDirtyState.pipeline |= mRenderStateCommitted.sampler != mRenderStatePending.sampler;
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
			mRenderStateCommitted.bEnableScissor = mRenderStatePending.bEnableScissor;
		}
		if (mDirtyState.scissorRect)
		{
			mRenderStateCommitted.scissorRect = mRenderStatePending.scissorRect;
		}

		mDirtyState.value = 0;
		bStateDirty = true;
	}
}

void RHIGraphics2D::syncState()
{
	if (bStateDirty)
	{
		mCurrentStateIndex = getElementList().setState(mRenderStateCommitted);
		bStateDirty = false;
	}
}

void RHIGraphics2D::drawPixel(Vector2 const& p, Color4ub const& color)
{
	setTextureState();
	auto& element = getElementList().addPoint(p, color, 0.0);
	setupElement(element);
}

void RHIGraphics2D::drawRect(int left, int top, int right, int bottom)
{
	setTextureState();
	Vector2 p1(left, top);
	Vector2 p2(right, bottom);
	auto& element = getElementList().addRect(getPaintArgs(), p1, p2);
	setupElement(element);
}

void RHIGraphics2D::drawRect(Vector2 const& pos, Vector2 const& size)
{
	setTextureState();
	auto& element = getElementList().addRect(getPaintArgs(), pos, pos + size);
	setupElement(element);
}

void RHIGraphics2D::drawCircle(Vector2 const& center, float r)
{
	setTextureState();
	auto& element = getElementList().addCircle(getPaintArgs(), center, r);
	setupElement(element);
}

void RHIGraphics2D::drawEllipse(Vector2 const& center, Vector2 const& size)
{
	setTextureState();
	auto& element = getElementList().addEllipse(getPaintArgs(), center, size );
	setupElement(element);
}

void RHIGraphics2D::drawPolygon(Vector2 const pos[], int num)
{
	if (num <= 2)
		return;

	setTextureState();
	auto& element = getElementList().addPolygon(getPaintArgs(), pos, num);
	setupElement(element);
}

void RHIGraphics2D::drawPolygon(Vec2i const pos[], int num)
{
	if (num <= 2)
		return;

	setTextureState();
	auto& element = getElementList().addPolygon(getPaintArgs(), pos, num);
	setupElement(element);
}

void RHIGraphics2D::drawTriangleList(Vector2 const pos[], int num)
{
	if (num <= 2)
		return;

	setTextureState();
	auto& element = getElementList().addTriangleList(getPaintArgs(), pos, num);
	setupElement(element);
}

void RHIGraphics2D::drawTriangleList(Vec2i const pos[], int num)
{
	if (num <= 2)
		return;

	setTextureState();
	auto& element = getElementList().addTriangleList(getPaintArgs(), pos, num);
	setupElement(element);
}

void RHIGraphics2D::drawTriangleStrip(Vector2 const pos[], int num)
{
	if (num <= 2)
		return;

	setTextureState();
	auto& element = getElementList().addTriangleStrip(getPaintArgs(), pos, num);
	setupElement(element);
}

void RHIGraphics2D::drawTriangleStrip(Vec2i const pos[], int num)
{
	if (num <= 2)
		return;

	setTextureState();
	auto& element = getElementList().addTriangleStrip(getPaintArgs(), pos, num);
	setupElement(element);
}

void RHIGraphics2D::drawLine(Vector2 const& p1, Vector2 const& p2)
{
	setTextureState();
	auto& element = getElementList().addLine(mPaintArgs.penColor, p1, p2, mPaintArgs.penWidth);
	setupElement(element);
}


void RHIGraphics2D::drawLineStrip(Vector2 const pos[], int num)
{
	setTextureState();
	auto& element = getElementList().addLineStrip(mPaintArgs.penColor, pos, num, mPaintArgs.penWidth);
	setupElement(element);
}

void RHIGraphics2D::drawArcLine(Vector2 const& pos, float r, float startAngle, float sweepAngle)
{
	setTextureState();
	auto& element = getElementList().addArcLine(mPaintArgs.penColor, pos, r, startAngle, sweepAngle, mPaintArgs.penWidth);
	setupElement(element);
}

void RHIGraphics2D::drawRoundRect(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleSize)
{
	setTextureState();
	auto& element = getElementList().addRoundRect(getPaintArgs(),  pos, rectSize, circleSize / 2);
	setupElement(element);
}

void RHIGraphics2D::drawGradientRect(Vector2 const& posLT, Color3Type const& colorLT, Vector2 const& posRB, Color3Type const& colorRB, bool bHGrad)
{
	setTextureState();
	auto& element = getElementList().addGradientRect(posLT, colorLT, posRB, colorRB, bHGrad);
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

	int charCount = 0;
	Vector2 extent = mFont->calcTextExtent(str, &charCount);
	if (charCount == 0)
	{
		return;
	}

	Vector2 renderPos = pos;
	switch (alignH)
	{
	case EHorizontalAlign::Right: 
		renderPos.x += size.x - extent.x;
		break;
	case EHorizontalAlign::Center:
	case EHorizontalAlign::Fill:
		renderPos.x += 0.5f * ( size.x - extent.x );
		break;
	}
	switch (alignV)
	{
	case EVerticalAlign::Bottom:
		renderPos.y += size.y - extent.y;
		break;
	case EVerticalAlign::Center:
	case EVerticalAlign::Fill:
		renderPos.y += 0.5f * (size.y - extent.y);
		break;
	}

	auto DoDrawText = [&]()
	{
		drawTextImpl(renderPos.x, renderPos.y, str, charCount);
	};

	if (bClip)
	{
		Rect textRect = { renderPos, extent };
		Rect clipRect = { pos, size };
		if (clipRect.contains(textRect))
		{
			if (mRenderStatePending.bEnableScissor)
			{
				if (mRenderStatePending.scissorRect.contains(textRect))
					bClip = false;
			}
			else
			{
				bClip = false;
			}
		}
	}
	
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

void RHIGraphics2D::drawText(Vector2 const& pos, Vector2 const& size, char const* str, EHorizontalAlign alignment, bool bClip)
{
	drawText(pos, size, str, alignment, EVerticalAlign::Center, bClip);
}

void RHIGraphics2D::drawText(Vector2 const& pos, Vector2 const& size, char const* str, bool bClip /*= false */)
{
	if (!mFont || !mFont->isValid())
		return;

	int charCount = 0;
	Vector2 extent = mFont->calcTextExtent(str, &charCount);
	extent = getCurrentTransform().transformInvVector(extent);
	if (charCount == 0)
	{
		return;
	}

	Vector2 renderPos = pos + (size - extent) / 2;
	auto DoDrawText = [&]()
	{
		drawTextImpl(renderPos.x, renderPos.y, str, charCount);
	};

	if (bClip)
	{
		Rect textRect = { renderPos, extent };
		Rect clipRect = { pos, size };
		if (clipRect.contains(textRect))
		{
			if (mRenderStatePending.bEnableScissor)
			{
				if (mRenderStatePending.scissorRect.contains(textRect))
					bClip = false;
			}
			else
			{
				bClip = false;
			}
		}
	}

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

void RHIGraphics2D::drawText(Vector2 const& pos, Vector2 const& size, float scale, char const* str, bool bClip)
{
	if (!mFont || !mFont->isValid())
		return;

	int charCount = 0;
	Vector2 extent = mFont->calcTextExtent(str, scale, &charCount);
	extent = getCurrentTransform().transformInvVector(extent);
	if (charCount == 0)
	{
		return;
	}

	Vector2 renderPos = pos + (size - extent) / 2;
	auto DoDrawText = [&]()
	{
		drawTextImpl(renderPos.x, renderPos.y, scale, str, charCount);
	};

	if (bClip)
	{
		Rect textRect = { renderPos, extent };
		Rect clipRect = { pos, size };
		if (clipRect.contains(textRect))
		{
			if (mRenderStatePending.bEnableScissor)
			{
				if (mRenderStatePending.scissorRect.contains(textRect))
					bClip = false;
			}
			else
			{
				bClip = false;
			}
		}
	}

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

void SnapValue(float& inoutValue)
{
	inoutValue = int(inoutValue);
}
void SnapValue(Vector2& inoutPos)
{
	SnapValue(inoutPos.x);
	SnapValue(inoutPos.y);
}

template< typename CharT >
void RHIGraphics2D::drawTextImpl(float ox, float oy, CharT const* str, int charCount)
{
	CHECK(mFont);
	CHECK(charCount > 0 || charCount == INDEX_NONE);

	ESimpleBlendMode prevMode = mRenderStateCommitted.blendMode;
	setBlendState(ESimpleBlendMode::Translucent);
	setTextureState(&mFont->getTexture());
	Vector2 pos = Vector2(ox, oy);

	bool bRemoveScale = false;
	if (!mXFormStack.get().hadScaledOrRotation())
	{
		SnapValue(pos);
	}
	else if (mXFormStack.get().hadSacled())
	{
		bRemoveScale = bTextRemoveScale;
	}

	if (charCount == INDEX_NONE)
	{
		charCount = mFont->getCharCount(str);
		if (charCount == 0)
		{
			setBlendState(prevMode);
			return;
		}
	}

	auto& element = getElementList().addText(mColorFont, pos, *mFont, str, charCount, bRemoveScale, bTextRemoveRotation);
	setupElement(element);
	setBlendState(prevMode);
}


void RHIGraphics2D::drawTextQuad(TArray<Render::GlyphVertex> const& vertices)
{
	if (vertices.empty())
		return;

	bool bRemoveScale = false;
	if (mXFormStack.get().hadSacled())
	{
		bRemoveScale = bTextRemoveScale;
	}
	auto& element = getElementList().addText(mColorFont, vertices, bRemoveScale, bTextRemoveRotation);
	setupElement(element);
}

void RHIGraphics2D::drawTextQuad(TArray<Render::GlyphVertex> const& vertices, TArray<Color4Type> const& colors)
{
	if (vertices.empty())
		return;

	bool bRemoveScale = false;
	if (mXFormStack.get().hadSacled())
	{
		bRemoveScale = bTextRemoveScale;
	}
	auto& element = getElementList().addText(colors, vertices, bRemoveScale, bTextRemoveRotation);
	setupElement(element);
}

template< typename CharT >
void RHIGraphics2D::drawTextImpl(float ox, float oy, float scale, CharT const* str, int charCount)
{
	CHECK(mFont);
	CHECK(charCount > 0 || charCount == INDEX_NONE);

	ESimpleBlendMode prevMode = mRenderStateCommitted.blendMode;
	setBlendState(ESimpleBlendMode::Translucent);
	setTextureState(&mFont->getTexture());
	Vector2 pos = Vector2(ox, oy);

	bool bRemoveScale = false;
	if (!mXFormStack.get().hadScaledOrRotation())
	{
		SnapValue(pos);
	}
	else if (mXFormStack.get().hadSacled())
	{
		bRemoveScale = bTextRemoveScale;
	}

	if (charCount == INDEX_NONE)
	{
		charCount = mFont->getCharCount(str);
		if (charCount == 0)
		{
			setBlendState(prevMode);
			return;
		}
	}

	auto& element = getElementList().addText(mColorFont, pos, scale, *mFont, str, charCount, bRemoveScale, bTextRemoveRotation);
	setupElement(element);
	setBlendState(prevMode);
}

RHIGraphics2D::Vec2i RHIGraphics2D::calcTextExtentSize(char const* str, int len)
{
	if (mFont)
	{
		Vector2 extent = mFont->calcTextExtent(str, nullptr);
		return Vec2i((int)extent.x, (int)extent.y);
	}
	return Vec2i(0, 0);
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
	auto& element = getElementList().addTextureRect(color, pos, pos + size, Vector2(0, 0), Vector2(1, 1));
	setupElement(element);
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize, Color4f const& color)
{
	drawTexture(pos, mCurTextureSize, texPos, texSize);
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& size, Vector2 const& texPos, Vector2 const& texSize, Color4f const& color)
{
	auto& element = getElementList().addTextureRect(color, pos, pos + size, texPos, texPos + texSize);
	setupElement(element);
}

void RHIGraphics2D::flushBatchedElements()
{
	flush();
}

RHICommandList& RHIGraphics2D::getCommandList()
{
	if (mRenderMode == ERenderMode::Buffered && !IsInRenderThread())
	{
		// This should not be used on Game Thread in buffered mode!
		// Return a dummy or handle error. 
		// For now, return immediate list but flushBatchedElements should prevent its use.
	}
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
}

void RHIGraphics2D::preModifyRenderState()
{

}

void RHIGraphics2D::setBlendState(ESimpleBlendMode mode)
{
	mRenderStatePending.blendMode = mode;
}

void RHIGraphics2D::setBlendAlpha(float value)
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

void RHIGraphics2D::restoreRenderState()
{
	preModifyRenderState();
	setupCommittedRenderState();
}

class RenderCommand_RHIGraphicsBatch : public RenderCommand
{
public:
	RenderCommand_RHIGraphicsBatch(RHIGraphics2D& graphics, RHI2DContext* context)
		:mGraphics(graphics), mContext(context) {}

	void execute(RenderExecuteContext& context) override
	{
		auto& batchRender = RHIGraphicsBatchManager::Get().mBatchedRender;
		batchRender.mWidth = mContext->viewportWidth;
		batchRender.mHeight = mContext->viewportHeight;
		batchRender.mBaseTransform = mContext->baseTransform;

		batchRender.beginRender(mGraphics.getCommandList());
		batchRender.render(mGraphics.getCommandList(), mContext->elementList);
		batchRender.flush();
		mGraphics.releaseContext(mContext);
	}


	RHIGraphics2D& mGraphics;
	RHI2DContext* mContext;
	int mBatchId;
};

void RHIGraphics2D::flush()
{
	if (getElementList().mElements.empty())
		return;

	commitRenderState();
	if (mRenderMode == ERenderMode::Immediate)
	{
		if (!getElementList().mElements.empty())
		{
			auto& batchRender = RHIGraphicsBatchManager::Get().mBatchedRender;
			batchRender.render(getCommandList(), getElementList());
			batchRender.flush();
			getElementList().reset();
			mWriteContext->allocator.clearFrame();
		}
	}


	else
	{
		RHI2DContext* pendingContext = mWriteContext;
		mWriteContext = acquireContext();
		mRenderStateCommitted.setInit();
		mWriteContext->baseTransform = pendingContext->baseTransform;
		mWriteContext->viewportWidth = pendingContext->viewportWidth;
		mWriteContext->viewportHeight = pendingContext->viewportHeight;
		
		if (mRecordingList)
		{
			auto* command = mRecordingList->allocCommand<RenderCommand_RHIGraphicsBatch>(*this, pendingContext);
			command->mBatchId = mFlushCount++;
			command->debugName = "RHIGraphicsBatch";
		}
		else
		{
			auto* command = RenderThread::AllocCommand<RenderCommand_RHIGraphicsBatch>(*this, pendingContext);
			command->mBatchId = mFlushCount++;
			command->debugName = "RHIGraphicsBatch";
		}
	}
}

RHI2DContext* RHIGraphics2D::acquireContext()
{
	return RHIGraphicsBatchManager::Get().acquire();
}


void RHIGraphics2D::setRecordingList(::RenderCommandList* list)
{
	mRecordingList = list;
	if (mRecordingList)
	{
		mRenderMode = ERenderMode::Buffered;
		if (mWriteContext == mImmediateContext)
		{
			mWriteContext = acquireContext();
		}
	}
	else
	{
		mRenderMode = ERenderMode::Immediate;
	}
}


void RHIGraphics2D::releaseContext(RHI2DContext* context)
{
	if (context == mImmediateContext)
	{
		context->reset();
		return;
	}

	RHIGraphicsBatchManager::Get().release(context);
}



void RHIGraphics2D::setPen(Color3Type const& color, int width)
{
	mPaintArgs.bUsePen = true;
	mPaintArgs.penColor = Color4Type(Color3Type(color), mPaintArgs.penColor.a);
	mPaintArgs.penWidth = width;
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
	mRenderStatePending.scissorRect.pos = pos;
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

void* RHIGraphics2D::allocRaw(size_t size)
{
	return mWriteContext->allocator.alloc(size);
}
