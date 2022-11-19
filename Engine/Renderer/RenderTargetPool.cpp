#include "RenderTargetPool.h"

#include "RHI/RHICommand.h"

#include "CoreShare.h"

namespace Render
{
#if CORE_SHARE_CODE
	CORE_API GlobalRenderTargetTool GRenderTargetPool;
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
		result->texture = RHICreateTexture2D(desc.format, desc.size.x, desc.size.y, 0, desc.numSamples, desc.creationFlags | TCF_RenderTarget);
		if (desc.numSamples > 1 )
		{
			result->resolvedTexture = RHICreateTexture2D(desc.format, desc.size.x, desc.size.y, 0, 1, desc.creationFlags | TCF_RenderTarget);
		}
		else
		{
			result->resolvedTexture = result->texture;
		}
		mUsedRTs.push_back(result);
		return result;
	}

	void RenderTargetPool::freeUsedElement(PooledRenderTargetRef const& freeRT)
	{
		for (auto iter = mUsedRTs.begin(); iter != mUsedRTs.end(); ++iter)
		{
			auto& rt = *iter;
			if (rt == freeRT)
			{
				if (&rt != &mUsedRTs.back())
				{
					rt = mUsedRTs.back();
					mUsedRTs.pop_back();
				}
			}
		}
	}

	void RenderTargetPool::freeAllUsedElements()
	{
		mFreeRTs.insert(mFreeRTs.end(), mUsedRTs.begin(), mUsedRTs.end());
		mUsedRTs.clear();
		for (auto iter = mFreeRTs.begin(); iter != mFreeRTs.end(); ++iter)
		{
			iter->get()->desc.debugName == EName::None;
		}
	}

	void GlobalRenderTargetTool::restoreRHI()
	{
	}

	void GlobalRenderTargetTool::releaseRHI()
	{
		RenderTargetPool::releaseRHI();
	}

}//namespace Render