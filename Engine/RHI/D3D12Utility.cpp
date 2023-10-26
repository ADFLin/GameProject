#include "D3D12Utility.h"

#include "D3DSharedCommon.h"

#include "MarcoCommon.h"
#include "BitUtility.h"

#include "CoreShare.h"

namespace Render
{

	bool D3D12HeapPoolChunk::initialize(ID3D12DeviceRHI* device, D3D12_DESCRIPTOR_HEAP_TYPE inType, uint32 inNumElements, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = inType;
		heapDesc.NumDescriptors = inNumElements;
		heapDesc.Flags = flags;

		VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&resource)));

		elementSize = device->GetDescriptorHandleIncrementSize(inType);

		numElements = inNumElements;
		numElementsUasge = 0;
		mUsageMask.resize(( numElements + GroupSize - 1 )/ GroupSize, 0);
		return true;
	}


	bool D3D12HeapPoolChunk::fetchFreeSlot(uint& outSlotIndex)
	{
		if (numElementsUasge == numElements)
			return false;

		for (uint groupIndex = 0; groupIndex < (uint)mUsageMask.size(); ++groupIndex)
		{
			uint32 freeMask = ~mUsageMask[groupIndex];

			if (freeMask)
			{
				uint32 slotBit = FBitUtility::ExtractTrailingBit(freeMask);
				mUsageMask[groupIndex] |= slotBit;
				outSlotIndex = GroupSize * groupIndex + FBitUtility::ToIndex32(slotBit);
				++numElementsUasge;
				break;
			}
		}

		return true;
	}

	uint D3D12HeapPoolChunk::fetchFreeSlotFirstTime()
	{
		CHECK(numElementsUasge == 0 && numElements > 0);
		mUsageMask[0] |= 0x1;
		++numElementsUasge;
		return 0;
	}

	void D3D12HeapPoolChunk::freeSlot(uint slotIndex)
	{
		uint groupIndex = slotIndex / GroupSize;
		uint subIndex = slotIndex % GroupSize;
		CHECK(mUsageMask[groupIndex] & BIT(subIndex));
		mUsageMask[groupIndex] &= ~BIT(subIndex);

		--numElementsUasge;
	}

	D3D12DescriptorHeapPool& D3D12DescriptorHeapPool::Get()
	{
		static D3D12DescriptorHeapPool SInstance;
		return SInstance;
	}

	void D3D12DescriptorHeapPool::initialize(ID3D12DeviceRHI* device)
	{
		mDevice = device;
		bInitialized = true;

		mCSUData.chunkSize = 256;
		mCSUData.type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		mCSUData.flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		mRTVData.chunkSize = 32;
		mRTVData.type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		mRTVData.flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		mDSVData.chunkSize = 32;
		mDSVData.type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		mDSVData.flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		mSamplerData.chunkSize = 128;
		mSamplerData.type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		mSamplerData.flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	}

	void D3D12DescriptorHeapPool::releaseRHI()
	{
		mCSUData.release();
		mRTVData.release();
		mDSVData.release();
		mSamplerData.release();

		mDevice = nullptr;
		bInitialized = false;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::allocSRV(ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		if (!mCSUData.fetchFreeHandle(mDevice, result))
		{
			return D3D12PooledHeapHandle();
		}

		mDevice->CreateShaderResourceView(resource, desc, result.getCPUHandle());
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::allocCBV(ID3D12Resource* resource, D3D12_CONSTANT_BUFFER_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		if (!mCSUData.fetchFreeHandle(mDevice, result))
		{
			return D3D12PooledHeapHandle();
		}

		mDevice->CreateConstantBufferView(desc, result.getCPUHandle());
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::allocUAV(ID3D12Resource* resource, D3D12_UNORDERED_ACCESS_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		if (!mCSUData.fetchFreeHandle(mDevice, result))
		{
			return D3D12PooledHeapHandle();
		}

		mDevice->CreateUnorderedAccessView(resource, nullptr, desc, result.getCPUHandle());
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::allocRTV(ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		if (!mRTVData.fetchFreeHandle(mDevice, result))
		{
			return D3D12PooledHeapHandle();
		}

		mDevice->CreateRenderTargetView(resource, desc, result.getCPUHandle());
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::allocDSV(ID3D12Resource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		if (!mDSVData.fetchFreeHandle(mDevice, result))
		{
			return D3D12PooledHeapHandle();
		}

		mDevice->CreateDepthStencilView(resource, desc, result.getCPUHandle());
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::allocSampler(D3D12_SAMPLER_DESC const& desc)
	{
		D3D12PooledHeapHandle result;
		if (!mSamplerData.fetchFreeHandle(mDevice, result))
		{
			return D3D12PooledHeapHandle();
		}

		mDevice->CreateSampler(&desc, result.getCPUHandle());
		return result;
	}


	void D3D12DescriptorHeapPool::freeHandle(D3D12PooledHeapHandle& handle)
	{
		if (!bInitialized)
			return;

		CHECK(handle.isValid());
		//if (handle.isValid())
		{
			handle.chunk->owner->freeHandle(handle);
		}
	}


}//namespace Render

