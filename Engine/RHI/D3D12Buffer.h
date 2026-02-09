#pragma once
#ifndef D3D12Buffer_H_8966BFD8_EAA1_4726_9BED_E775A93A754C
#define D3D12Buffer_H_8966BFD8_EAA1_4726_9BED_E775A93A754C

#include "D3D12Definations.h"

#include "DataStructure/Array.h"
#include "Memory/BuddyAllocator.h"
#include "PlatformThread.h"

namespace Render
{
	class D3D12FenceResourceManager
	{
	public:
		static D3D12FenceResourceManager& Get()
		{
			static D3D12FenceResourceManager StaticInstance;
			return StaticInstance;
		}


		void cleanup(bool bDoRelease)
		{
			for (auto fr : mReleaseList)
			{
				if (bDoRelease)
				{
					fr->release();
				}

				delete fr;

			}
			mReleaseList.clear();
		}

		template < typename TFunc >
		void addFuncRelease(TFunc&& func)
		{
			class FuncRelease : public IFenceRelease
			{
			public:
				FuncRelease(TFunc&& func)
					:mFunc(std::forward<TFunc>(func))
				{

				}
				virtual void release()
				{
					mFunc();
				}
				TFunc mFunc;
			};

			IFenceRelease* fr = new FuncRelease(std::forward<TFunc>(func));
			fr->fenceValue = mFenceValue + 1;
			mReleaseList.push_back(fr);
		}


		class IFenceRelease
		{
		public:
			uint64 fenceValue;
			virtual ~IFenceRelease() = default;
			virtual void release() = 0;
		};

		void releaseFence(uint64 lastCompletedFenceValue)
		{
			int index = 0;
			for (; index < mReleaseList.size(); ++index)
			{
				auto fr = mReleaseList[index];
				if (fr->fenceValue > lastCompletedFenceValue)
					break;

				fr->release();
				delete fr;
			}

			if (index != 0)
			{
				mReleaseList.erase(mReleaseList.begin(), mReleaseList.begin() + index);
			}
		}

		uint64 mFenceValue;
		TArray< IFenceRelease* > mReleaseList;
	};


	class D3D12UploadHeapPage;
	struct BuddyAllocationInfo
	{
		uint32 offset;
		uint32 order;
		D3D12UploadHeapPage* page = nullptr;
	};

	struct D3D12BufferAllocation
	{
		ID3D12Resource* ptr = nullptr;
		uint8*          cpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
		uint32           size;


		BuddyAllocationInfo buddyInfo;
	};

	class D3D12FrameHeapAllocator
	{
	public:
		~D3D12FrameHeapAllocator();

		void initialize(ID3D12DeviceRHI* device, int resourceSize)
		{
			mDevice = device;
			mResourceSize = resourceSize;
			mSizeUsage = 0;
			mCurrentPage.resource = nullptr;
			mCurrentPage.cpuAddr = nullptr;
		}

		bool alloc(uint32 size, uint32 alignment, D3D12BufferAllocation& outAllocation);

		void markFence();

		struct Page
		{
			ID3D12Resource* resource;
			uint8* cpuAddr;
		};
		bool createNewPage(Page& outPage);

		ID3D12DeviceRHI* mDevice = nullptr;
		uint32 mResourceSize = 0;
		uint32 mSizeUsage = 0;

		Page mCurrentPage;
		TArray< Page > mFreePages;
		TArray< Page > mUsedPages;
		RWLock mLock;
	};

	class D3D12UploadHeapPage
	{
	public:
		TComPtr< ID3D12Resource > mResource;
		uint8* mCpuAddress;

		BuddyAllocatorBase mAllocator;

		static constexpr uint32 BlockSize = 256;
		void initialize(uint32 inSize, ID3D12Resource* inResource)
		{
			//CHECK(FBitUtility::IsOneBitSet(inSize));
			mAllocator.initialize(inSize, BlockSize);
			mResource.initialize(inResource);

			D3D12_RANGE range = {};
			mResource->Map(0, &range, (void**)&mCpuAddress);
		}


		void cleanup()
		{
			CHECK(mCpuAddress);
			mResource->Unmap(0, nullptr);
			mResource.reset();
		}

		bool alloc(uint32 size, uint32 alignment, D3D12BufferAllocation& outAllocation)
		{
			BuddyAllocatorBase::Allocation allocation;
			if (!mAllocator.alloc(size, alignment, allocation))
			{
				return false;
			}

			outAllocation.ptr = mResource.get();
			outAllocation.cpuAddress = mCpuAddress + allocation.pos;
			outAllocation.gpuAddress = mResource->GetGPUVirtualAddress() + allocation.pos;
			outAllocation.size = size;

			outAllocation.buddyInfo.page = this;
			outAllocation.buddyInfo.offset = allocation.offset;
			outAllocation.buddyInfo.order = allocation.order;
			return true;
		}

		void dealloc(BuddyAllocationInfo const& info)
		{
			BuddyAllocatorBase::AllocationBlock block;
			block.offset = info.offset;
			block.order  = info.order;

			mAllocator.deallocate(block);
		}
	};


	class D3D12DynamicBufferManager
	{
	public:

		static D3D12DynamicBufferManager& Get()
		{
			static D3D12DynamicBufferManager StaticInstance;
			return StaticInstance;
		}

		bool initialize(ID3D12DeviceRHI* device)
		{
			mDevice = device;
			addFrameAllocator(64 * 1024);
			return true;
		}

		void cleanup()
		{
			for (auto page : mHeapPages)
			{
				page->cleanup();
				delete page;
			}
			mHeapPages.clear();
			for( auto allocator : mFrameAllocators )
			{
				delete allocator;
			}
			mFrameAllocators.clear();
			mDevice = nullptr;
		}

		bool isInitialized() { return !!mDevice; }
		bool addFrameAllocator(uint32 size);

		void markFence();
		bool alloc(uint32 size, uint32 alignment, D3D12BufferAllocation& outAllocation);
		bool allocFrame(uint32 size, uint32 alignment, D3D12BufferAllocation& outAllocation);
		void dealloc(BuddyAllocationInfo const& info);
		ID3D12DeviceRHI* mDevice = nullptr;

		TArray< D3D12UploadHeapPage* > mHeapPages;
		TArray< D3D12FrameHeapAllocator* > mFrameAllocators;

	};

}//namespace Render


#endif // D3D12Buffer_H_8966BFD8_EAA1_4726_9BED_E775A93A754C