#include "D3D12Buffer.h"

#include "D3DSharedCommon.h"
#include "D3D12Utility.h"


namespace Render
{

	D3D12FrameHeapAllocator::~D3D12FrameHeapAllocator()
	{
		if (mCurrentPage.resource) mCurrentPage.resource->Release();
		for (auto& page : mFreePages)
		{
			if (page.resource) page.resource->Release();
		}
		for (auto& page : mUsedPages)
		{
			if (page.resource) page.resource->Release();
		}
	}

	void D3D12FrameHeapAllocator::markFence()
	{
		if (mCurrentPage.resource && mSizeUsage > 0)
		{
			mUsedPages.push_back(mCurrentPage);
			mCurrentPage.resource = nullptr;
			mCurrentPage.cpuAddr = nullptr;
			mSizeUsage = 0;
		}

		for (auto& page : mUsedPages)
		{
			D3D12FenceResourceManager::Get().addFuncRelease(
				[this, page]()
				{
					RWLock::WriteLocker locker(mLock);
					mFreePages.push_back(page);
				}
			);
		}
		mUsedPages.clear();
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

		bool bNeedNewPage = false;
		if (mCurrentPage.resource == nullptr)
		{
			bNeedNewPage = true;
		}
		else if (mSizeUsage + allocSize > mResourceSize)
		{
			mUsedPages.push_back(mCurrentPage);
			mCurrentPage.resource = nullptr;
			mCurrentPage.cpuAddr = nullptr;
			mSizeUsage = 0;
			bNeedNewPage = true;
		}

		if (bNeedNewPage)
		{
			{
				RWLock::WriteLocker locker(mLock);
				if (mFreePages.size())
				{
					mCurrentPage = mFreePages.back();
					mFreePages.pop_back();
				}
			}

			if (mCurrentPage.resource == nullptr)
			{
				if (!createNewPage(mCurrentPage))
					return false;
			}
			mSizeUsage = 0;

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

		outAllocation.ptr = mCurrentPage.resource;
		outAllocation.gpuAddress = outAllocation.ptr->GetGPUVirtualAddress() + pos;
		outAllocation.cpuAddress = mCurrentPage.cpuAddr + pos;
		outAllocation.size = size;
		mSizeUsage += allocSize;
		return true;
	}

	bool D3D12FrameHeapAllocator::createNewPage(Page& outPage)
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
		outPage.resource = resource;
		
		D3D12_RANGE range = { 0,0 };
		VERIFY_D3D_RESULT_RETURN_FALSE(outPage.resource->Map(0, &range, (void**)&outPage.cpuAddr));

		return true;
	}

	bool D3D12DynamicBufferManager::addFrameAllocator(uint32 size)
	{
		if (size < 64 * 1024)
			size = 64 * 1024;
		D3D12FrameHeapAllocator* allocator = new D3D12FrameHeapAllocator();
		allocator->initialize(mDevice, size);
		mFrameAllocators.push_back(allocator);
		return true;
	}

	void D3D12DynamicBufferManager::markFence()
	{
		for (auto allocator : mFrameAllocators)
		{
			allocator->markFence();
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
		for (auto allocator : mFrameAllocators)
		{
			if (allocator->alloc(size, alignment, outAllocation))
				return true;
		}

		if (!addFrameAllocator(size))
			return false;

		if (!mFrameAllocators.back()->alloc(size, alignment, outAllocation))
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