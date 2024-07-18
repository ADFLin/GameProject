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
				TextureDesc depthDesc = TextureDesc::Type2D(desc.format, desc.size.x, desc.size.y).Samples(desc.numSamples).Flags(desc.creationFlags);
				result->texture = RHICreateTextureDepth(depthDesc);
			}
			else
			{
				TextureDesc texDesc = TextureDesc::Type2D(desc.format, desc.size.x, desc.size.y).Samples(desc.numSamples).Flags(desc.creationFlags | TCF_RenderTarget);
				result->texture = RHICreateTexture2D(texDesc);
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
			TextureDesc texDesc = TextureDesc::TypeCube(desc.format, desc.size.x).Samples(desc.numSamples).Flags(desc.creationFlags | TCF_RenderTarget);
			result->texture = RHICreateTextureCube(texDesc);
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
		return result;
	}

	void RenderTargetPool::freeUsedElement(PooledRenderTargetRef const& freeRT)
	{
		mUsedRTs.removeSwap(freeRT);
	}

	void RenderTargetPool::freeAllUsedElements()
	{
		for (int index = 0; index < mUsedRTs.size(); ++index)
		{
			auto& rt = mUsedRTs[index];
			if (rt->bResvered)
				continue;

			mFreeRTs.push_back(rt);
			mUsedRTs.removeIndexSwap(index);
			--index;
		}

		for (auto& rt : mFreeRTs)
		{
			rt->desc.debugName == EName::None;
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