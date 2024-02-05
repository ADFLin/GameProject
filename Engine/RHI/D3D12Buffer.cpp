#include "D3D12Buffer.h"

#include "D3DSharedCommon.h"
#include "D3D12Utility.h"


namespace Render
{

	D3D12FrameHeapAllocator::~D3D12FrameHeapAllocator()
	{
		for (auto res : mResources)
		{
			res->Release();
		}
	}

	void D3D12FrameHeapAllocator::markFence()
	{
		for (int index = 0; index < mIndexCur; ++index)
		{
			ID3D12Resource* resource = mResources[index];
			D3D12FenceResourceManager::Get().addFuncRelease(
				[this, resource]()
				{
					mResources.push_back(resource);
				}
			);
		}

		if (mSizeUsage != 0)
		{
			if (mCpuPtr)
			{
				mResources[mIndexCur]->Unmap(0, nullptr);
				mCpuPtr = nullptr;
			}

			ID3D12Resource* resource = mResources[mIndexCur];
			D3D12FenceResourceManager::Get().addFuncRelease(
				[this, resource]()
				{
					mResources.push_back(resource);
				}
			);
			++mIndexCur;
		}

		if (mIndexCur != 0)
		{
			mResources.erase(mResources.begin(), mResources.begin() + mIndexCur);
		}

		mIndexCur = 0;
		mSizeUsage = 0;
		mCpuPtr = nullptr;
		if (mIndexCur != mResources.size())
		{
			lockCurrentResource();
		}
	}

	bool D3D12FrameHeapAllocator::alloc(uint32 size, uint32 alignment, D3D12BufferAllocation& outAllocation)
	{
		uint32 allocSize = size;
		if (alignment && (mSizeUsage % alignment) != 0)
		{
			allocSize = size + alignment - mSizeUsage % alignment;
		}

		if (allocSize > mResourceSize)
			return false;

		CHECK(allocSize <= mResourceSize);
		if (mIndexCur == mResources.size())
		{
			if (!createHeap())
			{
				return false;
			}
			mSizeUsage = 0;
		}
		else if (mSizeUsage + allocSize > mResourceSize)
		{
			if (mCpuPtr)
			{
				mResources[mIndexCur]->Unmap(0, nullptr);
				mCpuPtr = nullptr;
			}

			++mIndexCur;
			if (mIndexCur < mResources.size())
			{
				if (!lockCurrentResource())
					return false;
			}
			else if (!createHeap())
			{
				return false;
			}
			mSizeUsage = 0;
		}

		if ( mSizeUsage == 0)
		{
			if (alignment && (mSizeUsage % alignment) != 0)
			{
				allocSize = size + alignment - mSizeUsage % alignment;
			}
			else
			{
				allocSize = size;
			}
		}

		uint32 pos = mSizeUsage;
		if (alignment && (mSizeUsage % alignment) != 0)
		{
			pos = AlignArbitrary(pos, alignment);
		}

		CHECK(mCpuPtr);
		CHECK(pos + size <= mResourceSize);

		outAllocation.ptr = mResources[mIndexCur];
		outAllocation.gpuAddress = outAllocation.ptr->GetGPUVirtualAddress() + pos;
		outAllocation.cpuAddress = mCpuPtr + pos;
		outAllocation.size = size;
		mSizeUsage += allocSize;
		return true;
	}

	bool D3D12FrameHeapAllocator::createHeap()
	{
		ID3D12Resource* resource = nullptr;
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommittedResource(
			&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&FD3D12Init::BufferDesc(mResourceSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&resource)));

		resource->SetName(L"FrameHeap");

		mResources.push_back(resource);
		if (!lockCurrentResource())
			return false;

		return true;
	}

	bool D3D12FrameHeapAllocator::lockCurrentResource()
	{
		D3D12_RANGE range = { 0,0 };
		VERIFY_D3D_RESULT_RETURN_FALSE(mResources[mIndexCur]->Map(0, &range, (void**)&mCpuPtr));
		return true;
	}

	bool D3D12DynamicBufferManager::addFrameAllocator(uint32 size)
	{
		D3D12FrameHeapAllocator allocator;
		allocator.initialize(mDevice, size);
		mFrameAllocators.push_back(std::move(allocator));
		return true;
	}

	void D3D12DynamicBufferManager::markFence()
	{
		for (auto& allocator : mFrameAllocators)
		{
			allocator.markFence();
		}
	}

	bool D3D12DynamicBufferManager::alloc(uint32 size, uint32 alignment, D3D12BufferAllocation& outAllocation)
	{
		for (auto page : mHeapPages)
		{
			if (page->alloc(size, alignment, outAllocation))
				return true;
		}

		uint32 newPageSize = 1024 * 64;

		while (newPageSize < size)
		{
			newPageSize *= 2;
		};

		TComPtr<ID3D12Resource> resource;
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommittedResource(
			&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&FD3D12Init::BufferDesc(newPageSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&resource)));

		resource->SetName(L"PageHeap");
		D3D12UploadHeapPage* newPage = new D3D12UploadHeapPage;
		newPage->initialize(newPageSize, resource.detach());

		mHeapPages.push_back(newPage);
		return newPage->alloc(size, alignment, outAllocation);
	}

	bool D3D12DynamicBufferManager::allocFrame(uint32 size, uint32 alignment, D3D12BufferAllocation& outAllocation)
	{
		for (auto& allocator : mFrameAllocators)
		{
			if (allocator.alloc(size, alignment, outAllocation))
				return true;
		}

		if (!addFrameAllocator(size))
			return false;

		if (!mFrameAllocators.back().alloc(size, alignment, outAllocation))
			return false;

		return true;
	}

	void D3D12DynamicBufferManager::dealloc(BuddyAllocationInfo const& info)
	{
		if (!isInitialized())
			return;

		if (info.page)
		{
			info.page->dealloc(info);
		}
	}

}