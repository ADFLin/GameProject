#include "RenderTargetPool.h"

#include "RHI/RHICommand.h"

#include "CoreShare.h"

namespace Render
{
#if CORE_SHARE_CODE
	CORE_API GlobalRenderTargetPool GRenderTargetPool;
#endif
	PooledRenderTargetRef RenderTargetPool::fetchElement(RenderTargetDesc const& desc)
	{
		for (auto iter = mFreeRTs.begin(); iter != mFreeRTs.end(); ++iter)
		{
			auto& rt = *iter;
			if (rt->desc.isMatch(desc))
			{
				rt->desc.debugName = desc.debugName;
				rt->lastUsedFrame = mCurrentFrame;
				mUsedRTs.push_back(rt);
#if 0	
				if (&rt != &mFreeRTs.back())
				{
					rt = mFreeRTs.back();
					mFreeRTs.pop_back();
				}

#else
				mFreeRTs.erase(iter);
#endif

				return mUsedRTs.back();
			}
		}
		PooledRenderTarget* result = new PooledRenderTarget;

		result->desc = desc;
		result->desc.creationFlags |= TCF_RenderTarget;

		if (desc.type == ETexture::Type2D)
		{
			if (ETexture::IsDepthStencil(desc.format))
			{
				TextureDesc depthDesc = TextureDesc::Type2D(desc.format, desc.size.x, desc.size.y).Samples(desc.numSamples).Flags(desc.creationFlags).ClearColor(desc.clearColor);
				result->texture = RHICreateTexture2D(depthDesc);
				result->texture->setDebugName("PooledRT.Depath");
			}
			else
			{
				TextureDesc texDesc = TextureDesc::Type2D(desc.format, desc.size.x, desc.size.y).Samples(desc.numSamples).Flags(desc.creationFlags | TCF_RenderTarget).ClearColor(desc.clearColor);
				result->texture = RHICreateTexture2D(texDesc);
				result->texture->setDebugName("PooledRT.Texture2D");
				if (desc.numSamples > 1)
				{
					texDesc.Samples(1);
					result->resolvedTexture = RHICreateTexture2D(texDesc);
				}
				else
				{
					result->resolvedTexture = result->texture;
				}
			}
		}
		else if (desc.type == ETexture::TypeCube)
		{
			TextureDesc texDesc = TextureDesc::TypeCube(desc.format, desc.size.x).Samples(desc.numSamples).Flags(desc.creationFlags | TCF_RenderTarget).ClearColor(desc.clearColor);
			result->texture = RHICreateTextureCube(texDesc);
			result->texture->setDebugName("PooledRT.TextureCube");
			if (desc.numSamples > 1)
			{
				texDesc.Samples(1);
				result->resolvedTexture = RHICreateTextureCube(texDesc);
			}
			else
			{
				result->resolvedTexture = result->texture;
			}

		}

		mUsedRTs.push_back(result);
		result->lastUsedFrame = mCurrentFrame;
		return result;
	}

	void RenderTargetPool::freeUsedElement(PooledRenderTargetRef const& freeRT)
	{
		if (mUsedRTs.removeSwap(freeRT))
		{
			mFreeRTs.push_back(freeRT);
		}
	}

	void RenderTargetPool::freeAllUsedElements()
	{
		++mCurrentFrame;
		for (int index = 0; index < mUsedRTs.size(); ++index)
		{
			auto& rt = mUsedRTs[index];
			if (rt->bResvered)
			{
				rt->lastUsedFrame = mCurrentFrame;
				continue;
			}

			mFreeRTs.push_back(rt);
			mUsedRTs.removeIndexSwap(index);
			--index;
		}

		for (auto& rt : mFreeRTs)
		{
			rt->desc.debugName = EName::None;
		}

		cleanupFreeElements();
	}

	void RenderTargetPool::cleanupFreeElements()
	{
		static constexpr uint32 MaxFrameAge = 120;
		for (int i = 0; i < mFreeRTs.size(); ++i)
		{
			if (mCurrentFrame - mFreeRTs[i]->lastUsedFrame > MaxFrameAge)
			{
				mFreeRTs.removeIndexSwap(i);
				--i;
			}
		}
	}

	void GlobalRenderTargetPool::restoreRHI()
	{
	}

	void GlobalRenderTargetPool::releaseRHI()
	{
		RenderTargetPool::releaseRHI();
	}

}//namespace Render