#include "BasePassRendering.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"

#include "CoreShare.h"
#include "InlineString.h"


namespace Render
{
#if CORE_SHARE_CODE
	IMPLEMENT_SHADER_PROGRAM(DeferredBasePassProgram);
#endif

	GBufferResource::GBufferResource()
	{
		for (int i = 0; i < EGBuffer::Count; ++i)
		{
			mTargetFomats[i] = ETexture::RGBA8;
		}
		mTargetFomats[0] = ETexture::RGB10A2;
	}

	bool GBufferResource::prepare(IntVector2 const& size, int numSamples)
	{
		for (int i = 0; i < EGBuffer::Count; ++i)
		{
			InlineString<512> str;
			RenderTargetDesc desc;
			str.format("GBuffer%c", 'A' + i);
			desc.debugName = str;
			desc.format = mTargetFomats[i];
			desc.size = size;
			desc.numSamples = numSamples;
			desc.creationFlags = TCF_CreateSRV;
			VERIFY_RETURN_FALSE( mTargets[i] = GRenderTargetPool.fetchElement(desc) );
		}

		return true;
	}

	void GBufferResource::releaseRHI()
	{
		for (int i = 0; i < EGBuffer::Count; ++i)
		{
			mTargets[i].release();
		}
	}

	void GBufferResource::attachToBuffer(RHIFrameBuffer& frameBuffer)
	{
		for (int i = 0; i < EGBuffer::Count; ++i)
		{
			frameBuffer.setTexture(i + 1, *mTargets[i]->texture);
		}
	}

	void GBufferResource::drawTexture(RHICommandList& commandList, Matrix4 const& XForm, int x, int y, int width, int height, EGBuffer::Type bufferName)
	{
		DrawUtility::DrawTexture(commandList, XForm, getResolvedTexture(bufferName), Vector2(x, y), Vector2(width, height));
	}

	void GBufferResource::drawTexture(RHICommandList& commandList, Matrix4 const& XForm, int x, int y, int width, int height, EGBuffer::Type bufferName, Vector4 const& colorMask)
	{
		RHISetViewport(commandList, x, y, width, height);
		ShaderHelper::Get().copyTextureMaskToBuffer(commandList, getResolvedTexture(bufferName), colorMask);
	}

	void GBufferResource::drawTextures(RHICommandList& commandList, Matrix4 const& XForm, IntVector2 const& size, IntVector2 const& gapSize)
	{
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
		int width = size.x;
		int height = size.y;
		int gapX = gapSize.x;
		int gapY = gapSize.y;
		int drawWidth = width - 2 * gapX;
		int drawHeight = height - 2 * gapY;

		drawTexture(commandList, XForm, 0 * width + gapX, 0 * height + gapY, drawWidth, drawHeight, EGBuffer::A);
		drawTexture(commandList, XForm, 1 * width + gapX, 0 * height + gapY, drawWidth, drawHeight, EGBuffer::B);
		drawTexture(commandList, XForm, 3 * width + gapX, 0 * height + gapY, drawWidth, drawHeight, EGBuffer::C, Vector4(1, 0, 0, 0));
#if 0
		{
			RHISetViewport(commandList, 1 * width + gapX, 0 * height + gapY, drawWidth, drawHeight);
			float colorBias[2] = { 0.5 , 0.5 };
			ShaderHelper::Get().copyTextureBiasToBuffer(commandList, getResolvedTexture(EGBuffer::B), colorBias);

		}

		
		drawTexture(commandList, XForm, 2 * width + gapX, 0 * height + gapY, drawWidth, drawHeight, EGBuffer::C);
		
		drawTexture(commandList, XForm, 3 * width + gapX, 2 * height + gapY, drawWidth, drawHeight, EGBuffer::D, Vector4(0, 1, 0, 0));
		drawTexture(commandList, XForm, 3 * width + gapX, 1 * height + gapY, drawWidth, drawHeight, EGBuffer::D, Vector4(0, 0, 1, 0));
		{
			RHISetViewport(commandList, 3 * width + gapX, 0 * height + gapY, drawWidth, drawHeight);
			float valueFactor[2] = { 255 , 0 };
			ShaderHelper::Get().mapTextureColorToBuffer(commandList, getResolvedTexture(EGBuffer::D), Vector4(0, 0, 0, 1), valueFactor);
		}
		//renderDepthTexture(width , 3 * height, width, height);
#endif
	}


	void GBufferShaderParameters::clearTextures(RHICommandList& commandList, ShaderProgram& program)
	{
		if (mParamGBufferTextureA.isBound())
		{
			program.clearTexture(commandList, mParamGBufferTextureA);
		}
		if (mParamGBufferTextureB.isBound())
		{
			program.clearTexture(commandList, mParamGBufferTextureB);
		}
		if (mParamGBufferTextureC.isBound())
		{
			program.clearTexture(commandList, mParamGBufferTextureC);
		}
		if (mParamGBufferTextureD.isBound())
		{
			program.clearTexture(commandList, mParamGBufferTextureD);
		}

		if (mParamFrameDepthTexture.isBound())
		{
			program.clearTexture(commandList, mParamFrameDepthTexture);
		}
	}


	bool FrameRenderTargets::prepare(IntVector2 const& size, int numSamples /*= 1*/)
	{
		if (!mGBuffer.prepare(size, numSamples))
			return false;

		if (mSize != size || mNumSamples != numSamples)
		{
			releaseBufferRHIResource();

			if (!createBufferRHIResource(size, numSamples))
			{
				return false;
			}

			mFrameBuffer->setTexture(0,getFrameTexture());
			mSize = size;
			mNumSamples = numSamples;
		}
		return true;
	}

	bool FrameRenderTargets::createBufferRHIResource(IntVector2 const& size, int numSamples /*= 1*/)
	{
		mIdxRenderFrameTexture = 0;
		for (auto& frameTexture : mFrameTextures)
		{
			frameTexture = RHICreateTexture2D(ETexture::FloatRGBA, size.x, size.y, 1, numSamples, TCF_DefalutValue | TCF_RenderTarget);
			if (!frameTexture.isValid())
				return false;
		}

		mDepthTexture = RHICreateTextureDepth(ETexture::D32FS8, size.x, size.y, 1, numSamples, TCF_CreateSRV);
		if (!mDepthTexture.isValid())
			return false;

		if (numSamples > 1)
		{


		}
		else
		{
			mResolvedDepthTexture = mDepthTexture;
		}

		return true;
	}

	void FrameRenderTargets::releaseBufferRHIResource()
	{
		for (auto& frameTexture : mFrameTextures)
		{
			frameTexture.release();
		}
		mSize = IntVector2::Zero();
	}

	bool FrameRenderTargets::initializeRHI()
	{
		VERIFY_RETURN_FALSE(mFrameBuffer = RHICreateFrameBuffer());
		return true;
	}

	void FrameRenderTargets::releaseRHI()
	{
		releaseBufferRHIResource();
		mFrameBuffer.release();
		mDepthTexture.release();
		mResolvedDepthTexture.release();
		mGBuffer.releaseRHI();
	}

}


