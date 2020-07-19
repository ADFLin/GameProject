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
TConsoleVariable<bool> CVarUseBachedRender2D(false, "r.UseBachedRender2D");
#endif
using namespace Render;

RHIGraphics2D::RHIGraphics2D()
	: mWidth(1)
	, mHeight(1)
{
	mFont = nullptr;
	mPenWidth = 1;
	mDrawBrush = true;
	mDrawPen = true;
	mAlpha = 1.0;
	mColorBrush = Color3f(0, 0, 0);
	mColorPen = Color3f(0, 0, 0);
	mColorFont = Color3f(0, 0, 0);
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

void RHIGraphics2D::beginRender()
{
	//glPushAttrib(GL_ENABLE_BIT | GL_VIEWPORT_BIT);

	using namespace Render;
	RHICommandList& commandList = GetCommandList();

	RHISetViewport(commandList, 0, 0, mWidth, mHeight);

	RHISetShaderProgram(commandList, nullptr);
	RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
	RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());

	mBaseTransform = AdjProjectionMatrixForRHI(OrthoMatrix(0, mWidth, mHeight, 0, -1, 1));

	RHISetFixedShaderPipelineState(commandList, mBaseTransform);
	mbPipelineStateChanged = false;
	mRenderStateCommitted.texture = nullptr;
	mRenderStateCommitted.blendMode = ESimpleBlendMode::None;
	mRenderStatePending = mRenderStateCommitted;
	mCurTextureSize = Vector2(0, 0);
	RHISetInputStream(commandList, &TStaticRenderRTInputLayout<RTVF_XY>::GetRHI(), nullptr, 0);

	mXFormStack.clear();
	//glDisable(GL_TEXTURE_2D);
}

void RHIGraphics2D::endRender()
{
	RHICommandList& commandList = GetCommandList();

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

	comitRenderState();
	struct Vertex_XY_CA
	{
		Math::Vector2 pos;
		Color4f c;
	} v = { p , Color4f(color , mAlpha) };
	TRenderRT<RTVF_XY_CA>::Draw(GetCommandList(), EPrimitive::Points, &v, 1);
}

void RHIGraphics2D::drawRect(int left, int top, int right, int bottom)
{
	if (CVarUseBachedRender2D)
	{
		setTextureState();
		Vector2 p1(left, top);
		Vector2 p2(right, bottom);
		comitRenderState();
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
	if (CVarUseBachedRender2D)
	{	
		setTextureState();
		comitRenderState();
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
	if (CVarUseBachedRender2D)
	{
		setTextureState();
		comitRenderState();
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

	if (CVarUseBachedRender2D)
	{
		setTextureState();
		comitRenderState();
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
	if (CVarUseBachedRender2D)
	{
		setTextureState();
		comitRenderState();
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
	if (CVarUseBachedRender2D)
	{
		setTextureState();
		comitRenderState();
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
	if (CVarUseBachedRender2D)
	{
		setTextureState();
		comitRenderState();
		auto& element = mBachedElementList.addLine(LinearColor(mColorPen, mAlpha), p1, p2, mPenWidth);
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
	if (CVarUseBachedRender2D)
	{
		setTextureState();
		comitRenderState();
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

}

void RHIGraphics2D::setTextColor(Color3ub const& color)
{
	mColorFont = color;
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

void RHIGraphics2D::drawText(Vector2 const& pos, Vector2 const& size, char const* str, bool beClip /*= false */)
{
#if	IGNORE_NSIGHT_UNSUPPORT_CODE
	return;
#endif

	if (!mFont || !mFont->isValid())
		return;
	if (beClip)
	{
		beginClip(pos, size);
	}

	Vector2 extent = mFont->calcTextExtent(str);
	int fontSize = mFont->getSize();
	int strLen = strlen(str);
	Vector2 renderPos = pos + (size - extent) / 2;
	drawTextImpl(renderPos.x, renderPos.y, str);

	if (beClip)
	{
		endClip();
	}
}

void RHIGraphics2D::drawTextImpl(float  ox, float  oy, char const* str)
{
#if	IGNORE_NSIGHT_UNSUPPORT_CODE
	return;
#endif
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
			auto& element = mBachedElementList.addText(LinearColor(mColorFont, mAlpha), std::move(vertices));
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
		mFont->draw(GetCommandList(), Vector2(int(ox), int(oy)), transform, LinearColor(mColorFont, mAlpha), str);
		mRenderStateCommitted.blendMode = ESimpleBlendMode::None;
		mbPipelineStateChanged = true;
		mRenderStatePending.texture = nullptr;
		setBlendState(prevMode);
	}
	
}


void RHIGraphics2D::drawTexture(Vector2 const& pos)
{
	drawTexture(pos, mCurTextureSize, LinearColor(mColorBrush, mAlpha));
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& size)
{
	drawTexture(pos, size, LinearColor(mColorBrush, mAlpha));
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize)
{
	drawTexture(pos, mCurTextureSize, texPos, texSize, LinearColor(mColorBrush, mAlpha));
}

void RHIGraphics2D::drawTexture(Vector2 const& pos, Vector2 const& size, Vector2 const& texPos, Vector2 const& texSize)
{
	drawTexture(pos, size, texPos, texSize, LinearColor(mColorBrush, mAlpha));
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
		RHISetFixedShaderPipelineState(GetCommandList(), transform, LinearColor(mColorBrush, mAlpha), mRenderStateCommitted.texture);
		DrawUtility::Sprite(GetCommandList(), pos, size, Vector2(0, 0), Vector2(0, 0), Vector2(1, 1));
		mbPipelineStateChanged = true;
		mRenderStatePending.texture = nullptr;
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
		Vector2 uvPos = Vector2(texPos.div(mCurTextureSize));
		Vector2 uvSize = Vector2(texSize.div(mCurTextureSize));
		auto& element = mBachedElementList.addTextureRect(color, pos, pos + size, uvPos, uvPos + uvSize);
		setupElement(element);
	}
	else
	{
		Matrix4 transform = mXFormStack.get().toMatrix4() * mBaseTransform;
		RHISetFixedShaderPipelineState(GetCommandList(), transform, color, mRenderStateCommitted.texture);
		DrawUtility::Sprite(GetCommandList(), pos, size, Vector2(0, 0), Vector2(texPos.div(mCurTextureSize)), Vector2(texSize.div(mCurTextureSize)));
		mbPipelineStateChanged = true;
		mRenderStatePending.texture = nullptr;
	}
}

void RHIGraphics2D::drawPolygonBuffer()
{
#if	IGNORE_NSIGHT_UNSUPPORT_CODE
	return;
#endif

	assert(!mBuffer.empty());
	comitRenderState();

	if (mDrawBrush)
	{
		TRenderRT<RTVF_XY>::Draw(GetCommandList(), EPrimitive::Polygon, mBuffer.data(), mBuffer.size() , LinearColor(mColorBrush, mAlpha));
	}
	if (mDrawPen)
	{
		TRenderRT<RTVF_XY>::Draw(GetCommandList(), EPrimitive::LineLoop, mBuffer.data(), mBuffer.size() , LinearColor(mColorPen, mAlpha));
	}

}

void RHIGraphics2D::drawLineBuffer()
{
#if	IGNORE_NSIGHT_UNSUPPORT_CODE
	return;
#endif
	assert(!mBuffer.empty());
	comitRenderState();

	if (mDrawPen)
	{
		TRenderRT<RTVF_XY>::Draw(GetCommandList(), EPrimitive::LineLoop, mBuffer.data(), mBuffer.size() , LinearColor(mColorPen, mAlpha));
	}
}


void RHIGraphics2D::flushBatchedElements()
{
	if ( !mBachedElementList.mElements.empty() )
	{
		mBatchedRender.render(GetCommandList(), mBachedElementList);
		mBachedElementList.releaseElements();
	}
}

RHICommandList& RHIGraphics2D::GetCommandList()
{
	return RHICommandList::GetImmediateList();
}

void RHIGraphics2D::setTextureState(RHITexture2D* texture /*= nullptr*/)
{
	mRenderStatePending.texture = texture;
}

void RHIGraphics2D::setTexture(RHITexture2D& texture)
{
	if (CVarUseBachedRender2D)
	{
		setTextureState(&texture);
	}
	else
	{
		mRenderStatePending.texture = &texture;
	}
	mCurTextureSize = Vector2(mRenderStateCommitted.texture->getSizeX(), mRenderStateCommitted.texture->getSizeY());
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

void RHIGraphics2D::comitRenderState()
{
	bool bRenderPipleStateNeedCommit = mbPipelineStateChanged || (mRenderStateCommitted.texture != mRenderStatePending.texture);
	if (bRenderPipleStateNeedCommit || mRenderStateCommitted.blendMode != mRenderStatePending.blendMode)
	{
		preModifyRenderState();

		if (bRenderPipleStateNeedCommit)
		{
			mbPipelineStateChanged = false;
			mRenderStateCommitted.texture = mRenderStatePending.texture;
			RHISetFixedShaderPipelineState(GetCommandList(), mBaseTransform, LinearColor(1, 1, 1, 1), mRenderStateCommitted.texture);
		}

		if (mRenderStateCommitted.blendMode != mRenderStatePending.blendMode)
		{
			mRenderStateCommitted.blendMode = mRenderStatePending.blendMode;

			switch (mRenderStateCommitted.blendMode)
			{
			case ESimpleBlendMode::Translucent:
				RHISetBlendState(GetCommandList(), TStaticBlendState< CWM_RGBA, Blend::eSrcAlpha, Blend::eOneMinusSrcAlpha >::GetRHI());
				break;
			case ESimpleBlendMode::Add:
				RHISetBlendState(GetCommandList(), TStaticBlendState< CWM_RGBA, Blend::eOne, Blend::eOne >::GetRHI());
				break;
			case ESimpleBlendMode::Multiply:
				RHISetBlendState(GetCommandList(), TStaticBlendState< CWM_RGBA, Blend::eDestColor, Blend::eZero >::GetRHI());
				break;
			case ESimpleBlendMode::None:
				RHISetBlendState(GetCommandList(), TStaticBlendState<>::GetRHI());
				break;
			default:
				NEVER_REACH("RHIGraphics2D::checkComitRenderState");
			}	
		}
	}
}

void RHIGraphics2D::restoreRenderState()
{
	auto& commandList = GetCommandList();
	preModifyRenderState();

	RHISetViewport(commandList, 0, 0, mWidth, mHeight);
	RHISetFixedShaderPipelineState(commandList, mBaseTransform, LinearColor(1, 1, 1, 1), mRenderStateCommitted.texture);
	switch (mRenderStateCommitted.blendMode)
	{
	case ESimpleBlendMode::Translucent:
		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, Blend::eSrcAlpha, Blend::eOneMinusSrcAlpha >::GetRHI());
		break;
	case ESimpleBlendMode::Add:
		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, Blend::eOne, Blend::eOne >::GetRHI());
		break;
	case ESimpleBlendMode::Multiply:
		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, Blend::eDestColor, Blend::eZero >::GetRHI());
		break;
	case ESimpleBlendMode::None:
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		break;
	default:
		NEVER_REACH("RHIGraphics2D::checkComitRenderState");
	}
}

void RHIGraphics2D::setPen(Color3ub const& color, int width)
{
	mColorPen = color;
	if (mPenWidth != width)
	{
		if (GRHISystem->getName() == RHISytemName::OpenGL)
		{
			glLineWidth(width);
		}
		mPenWidth = width;
	}
}

void RHIGraphics2D::beginClip(Vec2i const& pos, Vec2i const& size)
{
	preModifyRenderState();
	RHISetRasterizerState(GetCommandList(), TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, true >::GetRHI());
	RHISetScissorRect(GetCommandList(), pos.x, mHeight - pos.y - size.y, size.x, size.y);
}

void RHIGraphics2D::endClip()
{
	preModifyRenderState();
	RHISetRasterizerState(GetCommandList(), TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, false >::GetRHI());
}

void RHIGraphics2D::beginBlend(Vector2 const& pos, Vector2 const& size, float alpha)
{
	setBlendState(ESimpleBlendMode::Translucent);
	mAlpha = alpha;
}

void RHIGraphics2D::endBlend()
{
	setBlendState(ESimpleBlendMode::None);
	mAlpha = 1.0f;
}
