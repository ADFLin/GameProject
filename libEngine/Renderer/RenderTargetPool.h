#pragma once

#ifndef RenderTargetPool_H_1162D12B_B263_437C_AADC_E0C1F845ED40
#define RenderTargetPool_H_1162D12B_B263_437C_AADC_E0C1F845ED40

#include "RHI/RHIType.h"
#include "RHI/RHICommon.h"

#include "HashString.h"
#include "RefCount.h"

namespace Render
{

	struct RenderTargetDesc
	{
		HashString debugName;
		IntVector2 size;
		int numSamples;
		ETexture::Format format;
		uint32 creationFlags;

		RenderTargetDesc()
		{
			numSamples = 1;
		}

		bool isMatch(RenderTargetDesc const& rhs) const
		{
			return size == rhs.size &&
				   numSamples == rhs.numSamples &&
				   format == rhs.format &&
				   (creationFlags | TCF_RenderTarget) == (rhs.creationFlags | TCF_RenderTarget);
		}
	};

	struct PooledRenderTarget : public RefCountedObjectT< PooledRenderTarget >
	{
		RenderTargetDesc desc;
		RHITexture2DRef  texture;
		RHITexture2DRef  resolvedTexture;
	};

	typedef TRefCountPtr< PooledRenderTarget > PooledRenderTargetRef;


	class RenderTargetPool
	{
	public:

		PooledRenderTargetRef fetchElement(RenderTargetDesc const& desc);


		void freeUsedElement(PooledRenderTargetRef const& freeRT);

		void freeAllUsedElements();


		void releaseRHI()
		{
			mFreeRTs.clear();
			mUsedRTs.clear();
		}

		std::vector< PooledRenderTargetRef > mFreeRTs;
		std::vector< PooledRenderTargetRef > mUsedRTs;
	};


	extern CORE_API RenderTargetPool GRenderTargetPool;

}//namespace Render

#endif // RenderTargetPool_H_1162D12B_B263_437C_AADC_E0C1F845ED40
