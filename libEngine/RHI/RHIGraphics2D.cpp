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
	mPaintArgs.brushColor = Color4f(0, 0, 0, 1);
	mPaintArgs.penColor = Color4f(0, 0, 0, 1);

	mFont = nullptr;
	mColorFont = Color3f(0, 0, 0);

	mRenderStateCommitted.texture = nullptr;
	mRenderStateCommitted.sampler = nullptr;
	mRenderStateCommitted.blendMode = ESimpleBlendMode::None;
	mRenderStateCommitted.bEnableMultiSample = false;
	mRenderStateCommitted.bEnableScissor = false;
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
		initPiplineState();
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
		initPiplineState();
		mXFormStack.clear();
	}
}

void RHIGraphics2D::initPiplineState()
{
	RHICommandList& commandList = getCommandList();

	RHISetViewport(commandList, 0, 0, mWidth, mHeight);

	RHISetShaderProgram(commandList, nullptr);
	RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
	RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());

	mBaseTransform = AdjProjectionMatrixForRHI(OrthoMatrix(0, mWidth, mHeight, 0, -1, 1));

	RHISetFixedShaderPipelineState(commandList, mBaseTransform);
	mbPipelineStateChanged = false;
	mRenderStateCommitted.texture = nullptr;
	mRenderStateCommitted.sampler = nullptr;
	mRenderStateCommitted.blendMode = ESimpleBlendMode::None;
	mRenderStateCommitted.bEnableMultiSample = false;
	mRenderStateCommitted.bEnableScissor = false;
	mRenderStatePending = mRenderStateCommitted;
	mCurTextureSize = Vector2(0, 0);
	RHISetInputStream(commandList, &TStaticRenderRTInputLayout<RTVF_XY>::GetRHI(), nullptr, 0);
}

void RHIGraphics2D::endRender()
{
	RHICommandList& commandList = getCommandList();

	if (CVarUseBachedRender2D)
	{
		flushBatchedElements();
	}

	RHIFlushCommand(commandList);
	//glPopAttrib();
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

void RHIGraphics2D::drawPixel(Vector2 const& p, Color3ub const& color)
{
	if (CVarUseBachedRender2D)
	{
		flushBatchedElements();
	}

	setTextureState();
	comitRenderState();
	struct Vertex_XY_CA
	{
		Math::Vector2 pos;
		Color4f c;
	} v = { p , Color4f(color , mPaintArgs.brushColor.a) };
	TRenderRT<RTVF_XY_CA>::Draw(getCommandList(), EPrimitive::Points, &v, 1);
}

void RHIGraphics2D::drawRect(int left, int top, int right, int bottom)
{
	setTextureState();
	comitRenderState();

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
	comitRenderState();

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
	comitRenderState();

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
	comitRenderState();

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
	comitRenderState();

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
	comitRenderState();

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
	comitRenderState();

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

void RHIGraphics2D::drawRoundRect(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleSize)
{
	setTextureState();
	comitRenderState();

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

void RHIGraphics2D::fillGradientRect(Vector2 const& posLT, Color3ub const& colorLT, Vector2 const& posRB, Color3ub const& colorRB, bool bHGrad)
{
	setTextureState();
}

void RHIGraphics2D::setTextColor(Color3ub const& color)
{
	mColorFont = Color4f( Color3f( color ) , mColorFont.a );
}

void RHIGraphics2D::drawText(Vector2 const& pos, char const* str)
{
	if (!mFont || !mFont->isValid())
		return;

	int fontSize = mFont->getSize();
	float ox = pos.x;
	float oy = pos.y;
	drawTextImpl(ox, oy, str);
}

void RHIGraphics2D::drawText(Vector2 const& pos, wchar_t const* str)
{
	if (!mFont || !mFont->isValid())
		return;

	int fontSize = mFont->getSize();
	float ox = pos.x;
	float oy = pos.y;
	drawTextImpl(ox, oy, str);
}


void RHIGraphics2D::drawText(Vector2 const& pos, Vector2 const& size, char const* str, bool beClip /*= false */)
{
	if (!mFont || !mFont->isValid())
		return;

	if (beClip)
	{
		beginClip(pos, size);
	}

	Vector2 extent = mFont->calcTextExtent(str);
	int fontSize = mFont->getSize();
	Vector2 renderPos = pos + (size - extent) / 2;
	drawTextImpl(renderPos.x, renderPos.y, str);

	if (beClip)
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
			comitRenderState();
			auto& element = mBachedElementList.addText(mColorFont, std::move(vertices));
			setupElement(element);
			setBlendState(prevMode);
		}
	}
	else
	{
		ESimpleBlendMode prevMode = mRenderStateCommitted.blendMode;
		setBlendState(ESimpleBlendMode::Translucent);
		comitRenderState();
		Matrix4 transform = mXFormStack.get().toMatrix4() * mBaseTransform;
		mFont->draw(getCommandList(), Vector2(int(ox), int(oy)), transform, mColorFont, str);
		mRenderStateCommitted.blendMode = ESimpleBlendMode::None;
		mbPipelineStateChanged = true;
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
	comitRenderState();

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
		mbPipelineStateChanged = true;
	}
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize, Color4f const& color)
{
	drawTexture(pos, mCurTextureSize, texPos, texSize);
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& size, Vector2 const& texPos, Vector2 const& texSize, Color4f const& color)
{
	comitRenderState();

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
		mbPipelineStateChanged = true;
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
	if ( !mBachedElementList.mElements.empty() )
	{
		mBatchedRender.render(getCommandList(), mBachedElementList);
		mBachedElementList.releaseElements();
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
	mRenderStateCommitted.sampler = &sampler;
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

void SetBlendState(RHICommandList& commandList, ESimpleBlendMode blendMode)
{
	switch (blendMode)
	{
	case ESimpleBlendMode::Translucent:
		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::SrcAlpha, EBlend::OneMinusSrcAlpha >::GetRHI());
		break;
	case ESimpleBlendMode::Add:
		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One >::GetRHI());
		break;
	case ESimpleBlendMode::Multiply:
		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::DestColor, EBlend::Zero >::GetRHI());
		break;
	case ESimpleBlendMode::None:
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		break;
	default:
		NEVER_REACH("RHIGraphics2D::checkComitRenderState");
	}
}

RHIRasterizerState& GetRasterizerState(bool bEnableMultiSample, bool bEnableScissor)
{
	if (bEnableMultiSample)
	{
		if (bEnableScissor)
		{
			TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, true, true >::GetRHI();
		}
		else
		{
			TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, false , true >::GetRHI();
		}

	}
	else if (bEnableScissor)
	{
		return TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, true >::GetRHI();
	}

	return TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, false >::GetRHI();
}

void RHIGraphics2D::comitRenderState()
{
	bool bHadPreModifyRenderStateCalled = false;
	bool bRenderPipleStateNeedCommit = mbPipelineStateChanged || (mRenderStateCommitted.texture != mRenderStatePending.texture);
	auto& commandList = getCommandList();
	if (bRenderPipleStateNeedCommit)
	{
		if (bHadPreModifyRenderStateCalled == false)
		{
			bHadPreModifyRenderStateCalled = true;
			preModifyRenderState();
		}
		mbPipelineStateChanged = false;
		mRenderStateCommitted.texture = mRenderStatePending.texture;
		mRenderStateCommitted.sampler = mRenderStatePending.sampler;
		RHISetFixedShaderPipelineState(commandList, mBaseTransform, LinearColor(1, 1, 1, 1), mRenderStateCommitted.texture, mRenderStateCommitted.sampler);
	}

	if (mRenderStateCommitted.blendMode != mRenderStatePending.blendMode)
	{
		if (bHadPreModifyRenderStateCalled == false)
		{
			bHadPreModifyRenderStateCalled = true;
			preModifyRenderState();
		}
		mRenderStateCommitted.blendMode = mRenderStatePending.blendMode;
		SetBlendState(commandList, mRenderStateCommitted.blendMode);
	}

	if (mRenderStateCommitted.bEnableMultiSample != mRenderStatePending.bEnableMultiSample ||
		mRenderStateCommitted.bEnableScissor != mRenderStatePending.bEnableScissor)
	{
		if (bHadPreModifyRenderStateCalled == false)
		{
			bHadPreModifyRenderStateCalled = true;
			preModifyRenderState();
		}
		mRenderStateCommitted.bEnableMultiSample = mRenderStatePending.bEnableMultiSample;
		mRenderStateCommitted.bEnableScissor = mRenderStatePending.bEnableScissor;
		RHISetRasterizerState(commandList, GetRasterizerState(mRenderStateCommitted.bEnableMultiSample, mRenderStateCommitted.bEnableScissor));
	}

}


void RHIGraphics2D::restoreRenderState()
{
	auto& commandList = getCommandList();
	preModifyRenderState();

	RHISetViewport(commandList, 0, 0, mWidth, mHeight);
	RHISetFixedShaderPipelineState(commandList, mBaseTransform, LinearColor(1, 1, 1, 1), mRenderStateCommitted.texture, mRenderStateCommitted.sampler);
	RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
	RHISetRasterizerState(commandList, GetRasterizerState(mRenderStateCommitted.bEnableMultiSample, mRenderStateCommitted.bEnableScissor));
	SetBlendState(commandList, mRenderStateCommitted.blendMode);
}

void RHIGraphics2D::setPen(Color3ub const& color, int width)
{
	mPaintArgs.bUsePen = true;
	mPaintArgs.penColor = Color4f(Color3f(color), mPaintArgs.penColor.a);
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
	//preModifyRenderState();
	//RHISetRasterizerState(getCommandList(), TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, true >::GetRHI());
	RHISetScissorRect(getCommandList(), pos.x, mHeight - pos.y - size.y, size.x, size.y);
}

void RHIGraphics2D::setClipRect(Vec2i const& pos, Vec2i const& size)
{
	preModifyRenderState();
	RHISetScissorRect(getCommandList(), pos.x, mHeight - pos.y - size.y, size.x, size.y);
}

void RHIGraphics2D::endClip()
{
	mRenderStatePending.bEnableScissor = false;
	//preModifyRenderState();
	//RHISetRasterizerState(getCommandList(), TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, false >::GetRHI());
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
