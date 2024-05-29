#pragma once

#ifndef RenderTargetPool_H_1162D12B_B263_437C_AADC_E0C1F845ED40
#define RenderTargetPool_H_1162D12B_B263_437C_AADC_E0C1F845ED40

#include "RHI/RHIType.h"
#include "RHI/RHICommon.h"
#include "RHI/RHIGlobalResource.h"

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
		TextureCreationFlags creationFlags = TCF_DefalutValue | TCF_RenderTarget;


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

		TArray< PooledRenderTargetRef > mFreeRTs;
		TArray< PooledRenderTargetRef > mUsedRTs;
	};


	class GlobalRenderTargetPool : public RenderTargetPool
		                         , public IGlobalRenderResource
	{
	public:
		void restoreRHI() override;
		void releaseRHI() override;

	};
	extern CORE_API GlobalRenderTargetPool GRenderTargetPool;

}//namespace Render

#endif // RenderTargetPool_H_1162D12B_B263_437C_AADC_E0C1F845ED40
