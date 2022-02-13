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
		type = inType;
		numElementsUasge = 0;
		mUsageMask.resize(( numElements + 31 )/ 32, 0);
		return true;
	}

	bool D3D12HeapPoolChunk::fetchFreeSlot(uint& outSlotIndex)
	{
		if (numElementsUasge == numElements)
			return false;

		for (uint index = 0; index < (uint)mUsageMask.size(); ++index)
		{
			uint32 freeMask = ~mUsageMask[index];

			if (freeMask)
			{
				uint32 slotBit = FBitUtility::ExtractTrailingBit(freeMask);
				mUsageMask[index] |= slotBit;
				outSlotIndex = FBitUtility::ToIndex32(slotBit);
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
		uint groupIndex = slotIndex / 32;
		uint subIndex = slotIndex % 32;
		CHECK(mUsageMask[groupIndex] & BIT(subIndex));
		mUsageMask[groupIndex] &= ~BIT(subIndex);

		--numElementsUasge;
	}

#if CORE_SHARE_CODE
	D3D12DescriptorHeapPool& D3D12DescriptorHeapPool::Get()
	{
		static D3D12DescriptorHeapPool SInstance;
		return SInstance;
	}
#endif

	void D3D12DescriptorHeapPool::initialize(ID3D12DeviceRHI* device)
	{
		mDevice = device;
		bInitialized = true;
	}

	void D3D12DescriptorHeapPool::releaseRHI()
	{
		ReleaseChunks(mCSUChunks);
		ReleaseChunks(mRTVChunks);
		ReleaseChunks(mSamplerChunks);

		mDevice = nullptr;
		bInitialized = false;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::allocSRV(ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		if (!findFreeHandle(mCSUChunks, result))
		{
			if (!fetchHandleFromNewCSUChunk(result))
				return D3D12PooledHeapHandle();
		}

		mDevice->CreateShaderResourceView(resource, desc, result.getCPUHandle());
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::allocCBV(ID3D12Resource* resource, D3D12_CONSTANT_BUFFER_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		if (!findFreeHandle(mCSUChunks, result))
		{
			if (!fetchHandleFromNewCSUChunk(result))
				return D3D12PooledHeapHandle();
		}

		mDevice->CreateConstantBufferView(desc, result.getCPUHandle());
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::allocUAV(ID3D12Resource* resource, D3D12_UNORDERED_ACCESS_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		if (!findFreeHandle(mCSUChunks, result))
		{
			if (!fetchHandleFromNewCSUChunk(result))
				return D3D12PooledHeapHandle();
		}

		mDevice->CreateUnorderedAccessView(resource, nullptr, desc, result.getCPUHandle());
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::allocRTV(ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		if (!findFreeHandle(mRTVChunks, result))
		{
			D3D12HeapPoolChunk* chunk = new D3D12HeapPoolChunk;
			if (!chunk->initialize(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 32, D3D12_DESCRIPTOR_HEAP_FLAG_NONE))
			{
				return D3D12PooledHeapHandle();
			}

			mRTVChunks.push_back(chunk);
			result.chunkSlot = chunk->fetchFreeSlotFirstTime();
			result.chunk = chunk;
		}

		mDevice->CreateRenderTargetView(resource, desc, result.getCPUHandle());
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::allocDSV(ID3D12Resource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		if (!findFreeHandle(mDSVChunks, result))
		{
			D3D12HeapPoolChunk* chunk = new D3D12HeapPoolChunk;
			if (!chunk->initialize(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 32, D3D12_DESCRIPTOR_HEAP_FLAG_NONE))
			{
				return D3D12PooledHeapHandle();
			}

			mDSVChunks.push_back(chunk);
			result.chunkSlot = chunk->fetchFreeSlotFirstTime();
			result.chunk = chunk;
		}

		mDevice->CreateDepthStencilView(resource, desc, result.getCPUHandle());
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::allocSampler(D3D12_SAMPLER_DESC const& desc)
	{
		D3D12PooledHeapHandle result;
		if (!findFreeHandle(mSamplerChunks, result))
		{
			D3D12HeapPoolChunk* chunk = new D3D12HeapPoolChunk;
			if (!chunk->initialize(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 128, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
			{
				return D3D12PooledHeapHandle();
			}

			mSamplerChunks.push_back(chunk);

			result.chunkSlot = chunk->fetchFreeSlotFirstTime();
			result.chunk = chunk;
		}

		mDevice->CreateSampler(&desc, result.getCPUHandle());
		return result;
	}

	bool D3D12DescriptorHeapPool::findFreeHandle(std::vector< D3D12HeapPoolChunk* >& chunks, D3D12PooledHeapHandle& outHandle)
	{
		for (int index = chunks.size() - 1; index >= 0; --index)
		{
			if (chunks[index]->fetchFreeSlot(outHandle.chunkSlot))
			{
				outHandle.chunk = chunks[index];
				return true;
			}
		}

		return false;
	}

	void D3D12DescriptorHeapPool::freeHandle(D3D12PooledHeapHandle& handle)
	{
		if (!bInitialized)
			return;

		if (handle.isValid())
		{
			handle.chunk->freeSlot(handle.chunkSlot);
			handle.chunk = nullptr;
		}
	}

	void D3D12DescriptorHeapPool::ReleaseChunks(std::vector< D3D12HeapPoolChunk* >& chunks)
	{
		for (auto chunk : chunks)
		{
			delete chunk;
		}
		chunks.clear();
		chunks.shrink_to_fit();
	}

	bool D3D12DescriptorHeapPool::fetchHandleFromNewCSUChunk(D3D12PooledHeapHandle& outHandle)
	{
		D3D12HeapPoolChunk* chunk = new D3D12HeapPoolChunk;
		if (!chunk->initialize(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
		{
			return false;
		}

		mCSUChunks.push_back(chunk);

		outHandle.chunkSlot = chunk->fetchFreeSlotFirstTime();
		outHandle.chunk = chunk;
		return true;
	}

}//namespace Render

