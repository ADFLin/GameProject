#include "RHIGraphics2D.h"

#include "Math/Base.h"

#include "RHI/RHICommon.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/DrawUtility.h"

#include <cassert>
#include <algorithm>

#include "ConsoleSystem.h"

CORE_API extern TConsoleVariable<bool> CVarUseBachedRender2D;

#if CORE_SHARE_CODE
TConsoleVariable<bool> CVarUseBachedRender2D(true, "r.UseBachedRender2D");
#endif
using namespace Render;

RHIGraphics2D::RHIGraphics2D()
	: mWidth(1)
	, mHeight(1)
{

	mPaintArgs.penWidth = 1;
	mPaintArgs.bUseBrush = true;
	mPaintArgs.bUsePen = true;
	mPaintArgs.brushColor = Color4Type(0, 0, 0);
	mPaintArgs.penColor = Color4Type(0, 0, 0);

	mFont = nullptr;
	mColorFont = Color4Type(0, 0, 0);

	mRenderStateCommitted.texture = nullptr;
	mRenderStateCommitted.sampler = nullptr;
	mRenderStateCommitted.blendMode = ESimpleBlendMode::None;
	mRenderStateCommitted.bEnableMultiSample = false;
	mRenderStateCommitted.bEnableScissor = false;

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

	mRenderStateCommitted.texture = nullptr;
	mRenderStateCommitted.sampler = nullptr;
	mRenderStateCommitted.blendMode = ESimpleBlendMode::None;
	mRenderStateCommitted.bEnableMultiSample = false;
	mRenderStateCommitted.bEnableScissor = false;

	mRenderStatePending = mRenderStateCommitted;
	mDirtyState.value = 0;
}

void RHIGraphics2D::init(int w, int h)
{
	mWidth = w;
	mHeight = h;
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
	mXFormStack.rotate(Math::Deg2Rad(angle));
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
	if (bUseGraphicOnly)
	{
		initRenderState();
		mXFormStack.clear();
	}
}

void RHIGraphics2D::endFrame()
{

}

void RHIGraphics2D::beginRender()
{
	if (!bUseGraphicOnly)
	{
		initRenderState();
		mXFormStack.clear();
	}
}

void RHIGraphics2D::endRender()
{
	RHICommandList& commandList = getCommandList();

	if (CVarUseBachedRender2D)
	{
		flushBatchedElements();
	}

	//RHIFlushCommand(commandList);
}

RHIBlendState& GetBlendState(ESimpleBlendMode blendMode)
{
	switch (blendMode)
	{
	case ESimpleBlendMode::Translucent: 
		return TStaticBlendState< CWM_RGBA, EBlend::SrcAlpha, EBlend::OneMinusSrcAlpha >::GetRHI();
	case ESimpleBlendMode::Add:
		return TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One >::GetRHI();
	case ESimpleBlendMode::Multiply:
		return TStaticBlendState< CWM_RGBA, EBlend::DestColor, EBlend::Zero >::GetRHI();
	case ESimpleBlendMode::None:
		return TStaticBlendState<>::GetRHI();
	}
	NEVER_REACH("RHIGraphics2D::checkComitRenderState");
	return TStaticBlendState<>::GetRHI();
}

RHIRasterizerState& GetRasterizerState(bool bEnableScissor, bool bEnableMultiSample)
{
	if (bEnableMultiSample)
	{
		if (bEnableScissor)
		{
			return TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, true, true >::GetRHI();
		}
		else
		{
			return TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, false, true >::GetRHI();
		}

	}
	else if (bEnableScissor)
	{
		return TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, true >::GetRHI();
	}

	return TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, false >::GetRHI();
}


using GraphicsDepthState = StaticDepthDisableState;

void RHIGraphics2D::initRenderState()
{

	mRenderStateCommitted.texture = nullptr;
	mRenderStateCommitted.sampler = nullptr;
	mRenderStateCommitted.blendMode = ESimpleBlendMode::None;
	mRenderStateCommitted.bEnableMultiSample = mRenderStatePending.bEnableMultiSample;
	mRenderStateCommitted.bEnableScissor = false;
	mRenderStatePending = mRenderStateCommitted;
	mDirtyState.value = 0;
	mCurTextureSize = Vector2(0, 0);

	mBaseTransform = AdjProjectionMatrixForRHI(OrthoMatrix(0, mWidth, mHeight, 0, -1, 1));
	setupCommittedRenderState();
}

void RHIGraphics2D::setupCommittedRenderState()
{
	RHICommandList& commandList = getCommandList();
	RHISetViewport(commandList, 0, 0, mWidth, mHeight);

	RHISetDepthStencilState(commandList, GraphicsDepthState::GetRHI());
	RHISetBlendState(commandList, GetBlendState(mRenderStateCommitted.blendMode));
	RHISetRasterizerState(commandList, GetRasterizerState(mRenderStateCommitted.bEnableScissor, mRenderStateCommitted.bEnableMultiSample));

	//RHISetShaderProgram(commandList, nullptr);
	RHISetFixedShaderPipelineState(commandList, mBaseTransform, LinearColor(1,1,1,1), mRenderStateCommitted.texture , mRenderStateCommitted.sampler);

	RHISetInputStream(commandList, &TStaticRenderRTInputLayout<RTVF_XY>::GetRHI(), nullptr, 0);
}

void RHIGraphics2D::commitRenderState()
{
	mDirtyState.blend |= mRenderStateCommitted.blendMode != mRenderStatePending.blendMode;
	mDirtyState.rasterizer |= mRenderStateCommitted.bEnableScissor != mRenderStatePending.bEnableScissor || 
		mRenderStateCommitted.bEnableMultiSample != mRenderStatePending.bEnableMultiSample;
	if (mDirtyState.value)
	{
		preModifyRenderState();

		auto& commandList = getCommandList();
		if (mDirtyState.pipeline)
		{
			mRenderStateCommitted.texture = mRenderStatePending.texture;
			mRenderStateCommitted.sampler = mRenderStatePending.sampler;
			RHISetFixedShaderPipelineState(commandList, mBaseTransform, LinearColor(1, 1, 1, 1), mRenderStateCommitted.texture, mRenderStateCommitted.sampler);
		}
		if (mDirtyState.blend)
		{
			mRenderStateCommitted.blendMode = mRenderStatePending.blendMode;
			RHISetBlendState(commandList, GetBlendState(mRenderStatePending.blendMode));
		}
		if (mDirtyState.rasterizer)
		{
			mRenderStateCommitted.bEnableMultiSample = mRenderStatePending.bEnableMultiSample;
			mRenderStateCommitted.bEnableScissor = mRenderStatePending.bEnableScissor;
			RHISetRasterizerState(commandList, GetRasterizerState(mRenderStateCommitted.bEnableScissor, mRenderStateCommitted.bEnableMultiSample));
		}
		if (mDirtyState.scissorRect)
		{
			RHISetScissorRect(commandList, mScissorRect.pos.x, mHeight - mScissorRect.pos.y - mScissorRect.size.y, mScissorRect.size.x, mScissorRect.size.y);
		}

		mDirtyState.value = 0;
	}
}

void RHIGraphics2D::emitLineVertex(Vector2 const &p1, Vector2 const &p2)
{
	emitVertex(p1);
	emitVertex(p2);
}

void RHIGraphics2D::emitVertex(Vector2 const& v)
{
	Vector2 temp = mXFormStack.get().transformPosition(v);
	mBuffer.push_back(temp);
}

void RHIGraphics2D::drawPixel(Vector2 const& p, Color3Type const& color)
{
	if (CVarUseBachedRender2D)
	{
		flushBatchedElements();
	}

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

	if (CVarUseBachedRender2D)
	{
		Vector2 p1(left, top);
		Vector2 p2(right, bottom);
		auto& element = mBachedElementList.addRect(getPaintArgs(), p1, p2);
		setupElement(element);
	}
	else
	{
		Vector2 p1(left, top);
		Vector2 p2(right, bottom);
		ShapeVertexBuilder builder(mBuffer);
		builder.mTransform = mXFormStack.get();
		builder.buildRect(p1, p2);
		drawPolygonBuffer();
	}
}

void RHIGraphics2D::drawRect(Vector2 const& pos, Vector2 const& size)
{
	setTextureState();
	commitRenderState();

	if (CVarUseBachedRender2D)
	{
		auto& element = mBachedElementList.addRect(getPaintArgs(), pos, pos + size);
		setupElement(element);
	}
	else
	{
		Vector2 p2 = pos + size;
		ShapeVertexBuilder builder(mBuffer);
		builder.mTransform = mXFormStack.get();
		builder.buildRect(pos, p2);
		drawPolygonBuffer();
	}
}

void RHIGraphics2D::drawCircle(Vector2 const& center, float r)
{
	setTextureState();
	commitRenderState();

	if (CVarUseBachedRender2D)
	{
		auto& element = mBachedElementList.addCircle(getPaintArgs(), center, r);
		setupElement(element);
	}
	else
	{
		int numSeg = std::max(2 * r * r, 4.0f);
		ShapeVertexBuilder builder(mBuffer);
		builder.mTransform = mXFormStack.get();
		builder.buildCircle(center, r, calcCircleSemgmentNum(r));
		drawPolygonBuffer();
	}
}

void RHIGraphics2D::drawEllipse(Vector2 const& center, Vector2 const& size)
{
	setTextureState();
	commitRenderState();

	if (CVarUseBachedRender2D)
	{
		auto& element = mBachedElementList.addEllipse(getPaintArgs(), center, size );
		setupElement(element);
	}
	else
	{
		ShapeVertexBuilder builder(mBuffer);
		builder.mTransform = mXFormStack.get();
		float yFactor = float(size.y) / size.x;
		builder.buildEllipse(center, size.x, yFactor, calcCircleSemgmentNum((size.x + size.y) / 2));
		drawPolygonBuffer();
	}
}

void RHIGraphics2D::drawPolygon(Vector2 pos[], int num)
{
	setTextureState();
	commitRenderState();

	if (CVarUseBachedRender2D)
	{
		auto& element = mBachedElementList.addPolygon(mXFormStack.get(), getPaintArgs(), pos, num);
		setupElement(element);
	}
	else
	{
		ShapeVertexBuilder builder(mBuffer);
		builder.mTransform = mXFormStack.get();
		builder.buildPolygon(pos, num);
		drawPolygonBuffer();
	}
}

void RHIGraphics2D::drawPolygon(Vec2i pos[], int num)
{
	setTextureState();
	commitRenderState();

	if (CVarUseBachedRender2D)
	{
		auto& element = mBachedElementList.addPolygon(mXFormStack.get(), getPaintArgs(), pos, num);
		setupElement(element);
	}
	else
	{
		ShapeVertexBuilder builder(mBuffer);
		builder.mTransform = mXFormStack.get();
		builder.buildPolygon(pos, num);
		drawPolygonBuffer();
	}
}

void RHIGraphics2D::drawLine(Vector2 const& p1, Vector2 const& p2)
{
	setTextureState();
	commitRenderState();

	if (CVarUseBachedRender2D)
	{
		auto& element = mBachedElementList.addLine(mPaintArgs.penColor, p1, p2, mPaintArgs.penWidth);
		setupElement(element);
	}
	else
	{
		mBuffer.clear();
		emitLineVertex(p1, p2);
		drawLineBuffer();
	}
}


void RHIGraphics2D::drawLineStrip(Vector2 pos[], int num)
{
	setTextureState();
	commitRenderState();

	if (CVarUseBachedRender2D)
	{
		auto& element = mBachedElementList.addLineStrip(mXFormStack.get(), mPaintArgs.penColor, pos, num, mPaintArgs.penWidth);
		setupElement(element);
	}
	else
	{
		mBuffer.clear();
		for (int i = 1; i < num; ++i)
		{
			emitLineVertex(pos[i], pos[i-1]);
		}
		drawLineBuffer();
	}
}


void RHIGraphics2D::drawRoundRect(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleSize)
{
	setTextureState();
	commitRenderState();

	if (CVarUseBachedRender2D)
	{
		auto& element = mBachedElementList.addRoundRect( getPaintArgs(),  pos, rectSize, circleSize / 2);
		setupElement(element);
	}
	else
	{
		ShapeVertexBuilder builder(mBuffer);
		builder.mTransform = mXFormStack.get();
		builder.buildRoundRect(pos, rectSize, circleSize / 2);
		drawPolygonBuffer();
	}
}

void RHIGraphics2D::drawGradientRect(Vector2 const& posLT, Color3Type const& colorLT, Vector2 const& posRB, Color3Type const& colorRB, bool bHGrad)
{
	setTextureState();
	commitRenderState();

	auto& element = mBachedElementList.addGradientRect(posLT, colorLT, posRB, colorRB, bHGrad);
	setupElement(element);

	if (!CVarUseBachedRender2D)
	{
		flushBatchedElements();
	}
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


void RHIGraphics2D::drawText(Vector2 const& pos, Vector2 const& size, char const* str, bool bClip /*= false */)
{
	if (!mFont || !mFont->isValid())
		return;

	if (bClip)
	{
		beginClip(pos, size);
	}

	Vector2 extent = mFont->calcTextExtent(str);
	Vector2 renderPos = pos + (size - extent) / 2;
	drawTextImpl(renderPos.x, renderPos.y, str);

	if (bClip)
	{
		endClip();
	}
}


template< typename CharT >
void RHIGraphics2D::drawTextImpl(float ox, float oy, CharT const* str)
{
	assert(mFont);

	if (CVarUseBachedRender2D)
	{
		std::vector< FontVertex > vertices;
		mFont->generateVertices(Vector2(int(ox), int(oy)), str, vertices);
		if (!vertices.empty())
		{
			ESimpleBlendMode prevMode = mRenderStateCommitted.blendMode;
			setBlendState(ESimpleBlendMode::Translucent);
			setTextureState(&mFont->getTexture());
			commitRenderState();
			auto& element = mBachedElementList.addText(mColorFont, std::move(vertices));
			setupElement(element);
			setBlendState(prevMode);
		}
	}
	else
	{
		ESimpleBlendMode prevMode = mRenderStateCommitted.blendMode;
		setBlendState(ESimpleBlendMode::Translucent);
		commitRenderState();
		Matrix4 transform = mXFormStack.get().toMatrix4() * mBaseTransform;
		mFont->draw(getCommandList(), Vector2(int(ox), int(oy)), transform, mColorFont, str);
		mRenderStateCommitted.blendMode = ESimpleBlendMode::None;
		mDirtyState.pipeline = true;
		setBlendState(prevMode);
	}
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

	if (CVarUseBachedRender2D)
	{
		auto& element = mBachedElementList.addTextureRect(color, pos, pos + size, Vector2(0, 0), Vector2(1, 1));
		setupElement(element);
	}
	else
	{
		Matrix4 transform = mXFormStack.get().toMatrix4() * mBaseTransform;
		RHISetFixedShaderPipelineState(getCommandList(), transform, mPaintArgs.brushColor, mRenderStateCommitted.texture, mRenderStateCommitted.sampler);
		DrawUtility::Sprite(getCommandList(), pos, size, Vector2(0, 0), Vector2(0, 0), Vector2(1, 1));
		mDirtyState.pipeline = true;
	}
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize, Color4f const& color)
{
	drawTexture(pos, mCurTextureSize, texPos, texSize);
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& size, Vector2 const& texPos, Vector2 const& texSize, Color4f const& color)
{
	commitRenderState();

	if (CVarUseBachedRender2D)
	{
		auto& element = mBachedElementList.addTextureRect(color, pos, pos + size, texPos, texPos + texSize);
		setupElement(element);
	}
	else
	{
		Matrix4 transform = mXFormStack.get().toMatrix4() * mBaseTransform;
		RHISetFixedShaderPipelineState(getCommandList(), transform, color, mRenderStateCommitted.texture, mRenderStateCommitted.sampler);
		DrawUtility::Sprite(getCommandList(), pos, size, Vector2(0, 0), texPos, texSize);
		mDirtyState.pipeline = true;
	}
}

void RHIGraphics2D::drawPolygonBuffer()
{
	assert(!mBuffer.empty());
	if (mPaintArgs.bUseBrush)
	{
		TRenderRT<RTVF_XY>::Draw(getCommandList(), EPrimitive::Polygon, mBuffer.data(), mBuffer.size() , mPaintArgs.brushColor);
	}
	if (mPaintArgs.bUsePen)
	{
		TRenderRT<RTVF_XY>::Draw(getCommandList(), EPrimitive::LineLoop, mBuffer.data(), mBuffer.size() , mPaintArgs.penColor);
	}
}

void RHIGraphics2D::drawLineBuffer()
{
	assert(!mBuffer.empty());
	if (mPaintArgs.bUsePen)
	{
		TRenderRT<RTVF_XY>::Draw(getCommandList(), EPrimitive::LineLoop, mBuffer.data(), mBuffer.size() , mPaintArgs.penColor);
	}
}

void RHIGraphics2D::flushBatchedElements()
{
	if ( !mBachedElementList.isEmpty() )
	{
		mBatchedRender.render(getCommandList(), mBachedElementList);
		mBachedElementList.releaseElements();
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
	mDirtyState.pipeline |= mRenderStateCommitted.texture != mRenderStatePending.texture;
}

void RHIGraphics2D::setTexture(RHITexture2D& texture)
{
	setTextureState(&texture);
	mCurTextureSize = Vector2(mRenderStatePending.texture->getSizeX(), mRenderStatePending.texture->getSizeY());
}

void RHIGraphics2D::setSampler(RHISamplerState& sampler)
{
	mRenderStateCommitted.sampler = &sampler;
	mDirtyState.pipeline |= mRenderStateCommitted.sampler != mRenderStatePending.sampler;
}

void RHIGraphics2D::preModifyRenderState()
{
	if (CVarUseBachedRender2D)
	{
		flushBatchedElements();
	}
}

void RHIGraphics2D::setBlendState(ESimpleBlendMode mode)
{
	mRenderStatePending.blendMode = mode;
}

void RHIGraphics2D::restoreRenderState()
{
	preModifyRenderState();
	setupCommittedRenderState();
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
	mScissorRect.pos = pos;
	mScissorRect.size = size;
	mDirtyState.scissorRect = true;
}

void RHIGraphics2D::setClipRect(Vec2i const& pos, Vec2i const& size)
{
	mScissorRect.pos = pos;
	mScissorRect.size = size;
	mDirtyState.scissorRect = true;
}

void RHIGraphics2D::endClip()
{
	mRenderStatePending.bEnableScissor = false;
	mDirtyState.scissorRect = false;
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
