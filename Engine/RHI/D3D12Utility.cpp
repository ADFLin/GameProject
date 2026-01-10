#include "D3D12Utility.h"

#include "D3DSharedCommon.h"

#include "MacroCommon.h"
#include "BitUtility.h"

#include "CoreShare.h"

namespace Render
{

	bool D3D12HeapPoolChunkBase::initialize(uint inNumElements, ID3D12DeviceRHI* device, D3D12_DESCRIPTOR_HEAP_TYPE inType, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = inType;
		heapDesc.NumDescriptors = inNumElements;
		heapDesc.Flags = flags;

		VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(resource.address())));

		cacheHandle();
		elementSize = device->GetDescriptorHandleIncrementSize(inType);

		numElements = inNumElements;
		numElementsUasge = 0;
		mUsageMask.resize(( numElements + GroupSize - 1 )/ GroupSize, 0);
		return true;
	}

	bool D3D12HeapPoolChunkBase::fetchFreeSlot(uint& outSlotIndex)
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

	void D3D12HeapPoolChunkBase::freeSlot(uint slotIndex)
	{
		uint groupIndex = slotIndex / GroupSize;
		uint subIndex = slotIndex % GroupSize;
		CHECK(mUsageMask[groupIndex] & BIT(subIndex));
		mUsageMask[groupIndex] &= ~BIT(subIndex);

		--numElementsUasge;
	}

	uint D3D12HeapPoolChunk::fetchFreeSlotFirstTime()
	{
		CHECK(numElementsUasge == 0 && numElements > 0);
		mUsageMask[0] |= 0x1;
		++numElementsUasge;
		return 0;
	}

	D3D12DescriptorHeapPool& D3D12DescriptorHeapPool::Get()
	{
		static D3D12DescriptorHeapPool staticInstance;
		return staticInstance;
	}

	void D3D12DescriptorHeapPool::initialize(ID3D12DeviceRHI* device)
	{
		mDevice = device;
		bInitialized = true;

		mCSUData.type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		mCSUData.flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		mCSUData.initialize(128, mDevice);

		mSamplerData.type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		mSamplerData.flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		mSamplerData.initialize(128, mDevice);

		mRTVData.chunkSize = 32;
		mRTVData.type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		mRTVData.flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		mDSVData.chunkSize = 32;
		mDSVData.type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		mDSVData.flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
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

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::alloc(ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		mCSUData.fetchFreeHandle(mDevice, result, [this, resource, desc](D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
		{
			mDevice->CreateShaderResourceView(resource, desc, cpuHandle);
		});
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::alloc(ID3D12Resource* resource, D3D12_CONSTANT_BUFFER_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		mCSUData.fetchFreeHandle(mDevice, result, [this, desc](D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
		{
			mDevice->CreateConstantBufferView(desc, cpuHandle);
		});
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::alloc(ID3D12Resource* resource, D3D12_UNORDERED_ACCESS_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		mCSUData.fetchFreeHandle(mDevice, result, [this, resource, desc](D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
		{
			mDevice->CreateUnorderedAccessView(resource, nullptr, desc, cpuHandle);
		});
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::alloc(ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		if (!mRTVData.fetchFreeHandle(mDevice, result))
		{
			return D3D12PooledHeapHandle();
		}

		mDevice->CreateRenderTargetView(resource, desc, result.getCPUHandle());
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::alloc(ID3D12Resource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC const* desc)
	{
		D3D12PooledHeapHandle result;
		if (!mDSVData.fetchFreeHandle(mDevice, result))
		{
			return D3D12PooledHeapHandle();
		}

		mDevice->CreateDepthStencilView(resource, desc, result.getCPUHandle());
		return result;
	}

	D3D12PooledHeapHandle D3D12DescriptorHeapPool::alloc(D3D12_SAMPLER_DESC const& desc)
	{
		D3D12PooledHeapHandle result;
		mSamplerData.fetchFreeHandle(mDevice, result, [this, &desc](D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
		{
			mDevice->CreateSampler(&desc, cpuHandle);
		});
		return result;
	}

	void D3D12DescriptorHeapPool::freeHandle(D3D12PooledHeapHandle& handle)
	{
		if (!bInitialized)
			return;

		CHECK(handle.isValid());
		if (handle.chunk->owner)
		{
			handle.chunk->owner->freeHandle(handle);
		}
		handle.chunk->freeSlot(handle.chunkSlot);
		handle.chunk = nullptr;
		handle.chunkSlot = INDEX_NONE;
	}

	bool D3D12DescHeapGrowable::initialize(uint size, ID3D12DeviceRHI* device)
	{
		if (!D3D12HeapPoolChunkBase::initialize(size, device, type, flags))
			return false;

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = type;
		heapDesc.NumDescriptors = size;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(mResourceCopy.address())));
		mCachedCPUHandleCopy = mResourceCopy->GetCPUDescriptorHandleForHeapStart();
		return true;
	}

	template< typename TFunc >
	bool D3D12DescHeapGrowable::fetchFreeHandle(ID3D12DeviceRHI* device, D3D12PooledHeapHandle& outHandle, TFunc&& func)
	{
		if (!fetchFreeSlot(outHandle.chunkSlot))
		{
			uint newSize = Math::Max(numElements * 2, 4u);
			if (!grow(newSize, device))
				return false;

			LogMsg("DescHeap Grow to %u", newSize);
			if (!fetchFreeSlot(outHandle.chunkSlot))
				return false;
		}

		outHandle.chunk = this;

		uint offset = outHandle.chunkSlot * elementSize;
		D3D12_CPU_DESCRIPTOR_HANDLE handleCopy = mCachedCPUHandleCopy;
		handleCopy.ptr += offset;
		func(handleCopy);
		D3D12_CPU_DESCRIPTOR_HANDLE handle = mCachedCPUHandle;
		handle.ptr += offset;
		device->CopyDescriptorsSimple(1, handle, handleCopy, type);

		return true;
	}

	bool D3D12DescHeapGrowable::grow(uint numElementsNew, ID3D12DeviceRHI* device)
	{
		CHECK(numElementsNew > numElementsUasge);
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = type;
		heapDesc.NumDescriptors = numElementsNew;
		heapDesc.Flags = flags;
		TComPtr<ID3D12DescriptorHeap> newResource;
		VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(newResource.address())));
		TComPtr<ID3D12DescriptorHeap> newResourceCopy;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(newResourceCopy.address())));

		device->CopyDescriptorsSimple(numElements, newResource->GetCPUDescriptorHandleForHeapStart(), mCachedCPUHandleCopy, type);
		device->CopyDescriptorsSimple(numElements, newResourceCopy->GetCPUDescriptorHandleForHeapStart(), mCachedCPUHandleCopy, type);

		resource.reset();
		resource.initialize(newResource.detach());
		cacheHandle();

		mResourceCopy.reset();
		mResourceCopy.initialize(newResourceCopy.detach());
		mCachedCPUHandleCopy = mResourceCopy->GetCPUDescriptorHandleForHeapStart();

		numElements = numElementsNew;
		mUsageMask.resize((numElementsNew + GroupSize - 1) / GroupSize, 0);
		return true;
	}

	void D3D12DescHeapGrowable::release()
	{
		D3D12HeapPoolChunkBase::release();
		mResourceCopy.reset();
	}

	bool D3D12DescHeapAllocator::fetchFreeHandle(ID3D12DeviceRHI* device, D3D12PooledHeapHandle& outHandle)
	{
		if (availableChunk)
		{
			availableChunk->fetchFreeSlot(outHandle.chunkSlot);
			outHandle.chunk = availableChunk;
			updateAvailableChunk();
		}
		else
		{
			D3D12HeapPoolChunk* chunk = new D3D12HeapPoolChunk;
			if (!chunk->initialize(chunkSize, device, type, flags))
			{
				delete chunk;
				return false;
			}

			chunk->owner = this;
			outHandle.chunkSlot = chunk->fetchFreeSlotFirstTime();
			outHandle.chunk = chunk;
			availableChunk = chunk;
		}
		return true;
	}

	void D3D12DescHeapAllocator::freeHandle(D3D12PooledHeapHandle& handle)
	{
		D3D12HeapPoolChunk* chunk = static_cast<D3D12HeapPoolChunk*>(handle.chunk);
		if (chunk->next == D3D12HeapPoolChunk::NoLinkPtr)
		{
			chunk->next = availableChunk;
			availableChunk = chunk;
		}
	}

	void D3D12DescHeapAllocator::updateAvailableChunk()
	{
		while (availableChunk)
		{
			if (availableChunk->numElements != availableChunk->numElementsUasge)
				break;
			D3D12HeapPoolChunk* nextChunk = availableChunk->next;
			availableChunk->next = D3D12HeapPoolChunk::NoLinkPtr;
			availableChunk = nextChunk;
		}
	}

	void D3D12DescHeapAllocator::release()
	{
		for (auto chunk : chunks)
		{
			delete chunk;
		}
		chunks.clear();
		chunks.shrink_to_fit();

		availableChunk = nullptr;
	}

}//namespace Render

