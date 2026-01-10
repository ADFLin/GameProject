
#pragma once

#ifndef D3D12Utility_H_8286EFBB_EBBD_42EF_A628_A96A7810CF00
#define D3D12Utility_H_8286EFBB_EBBD_42EF_A628_A96A7810CF00

#include "MacroCommon.h"
#include "Core/IntegerType.h"

#include "Platform/Windows/ComUtility.h"
#include "CoreShare.h"
#include "DataStructure/Array.h"

#include "D3D12Definations.h"


namespace Render
{
	using IDXGISwapChainRHI = IDXGISwapChain3;
	using ID3D12DeviceRHI = ID3D12Device8;
	using ID3D12GraphicsCommandListRHI = ID3D12GraphicsCommandList6;

	struct FD3D12Init
	{
		static D3D12_VIEWPORT Viewport(
			FLOAT topLeftX,
			FLOAT topLeftY,
			FLOAT width,
			FLOAT height,
			FLOAT minDepth = D3D12_MIN_DEPTH,
			FLOAT maxDepth = D3D12_MAX_DEPTH) noexcept
		{
			D3D12_VIEWPORT result;
			result.TopLeftX = topLeftX;
			result.TopLeftY = topLeftY;
			result.Width = width;
			result.Height = height;
			result.MinDepth = minDepth;
			result.MaxDepth = maxDepth;

			return result;
		}

		static D3D12_DESCRIPTOR_RANGE1 DescriptorRange(
			D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
			UINT numDescriptors,
			UINT baseShaderRegister,
			UINT registerSpace = 0,
			D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
			UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND) noexcept
		{
			D3D12_DESCRIPTOR_RANGE1 result;
			result.RangeType = rangeType;
			result.NumDescriptors = numDescriptors;
			result.BaseShaderRegister = baseShaderRegister;
			result.RegisterSpace = registerSpace;
			result.Flags = flags;
			result.OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
			return result;
		}

		static D3D12_RESOURCE_BARRIER TransitionBarrier(
			ID3D12Resource* pResource,
			D3D12_RESOURCE_STATES stateBefore,
			D3D12_RESOURCE_STATES stateAfter,
			UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) noexcept
		{
			D3D12_RESOURCE_BARRIER result = {};
			result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			result.Flags = flags;
			result.Transition.pResource = pResource;
			result.Transition.StateBefore = stateBefore;
			result.Transition.StateAfter = stateAfter;
			result.Transition.Subresource = subresource;

			return result;
		}


		static D3D12_RESOURCE_BARRIER UAVBarrier(
			ID3D12Resource* pResource,
			D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) noexcept
		{
			D3D12_RESOURCE_BARRIER result = {};
			result.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			result.Flags = flags;
			result.UAV.pResource = pResource;
			return result;
		}

		static D3D12_HEAP_PROPERTIES HeapProperrties(
			D3D12_HEAP_TYPE type,
			UINT creationNodeMask = 1,
			UINT nodeMask = 1) noexcept
		{
			D3D12_HEAP_PROPERTIES result;
			result.Type = type;
			result.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			result.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			result.CreationNodeMask = creationNodeMask;
			result.VisibleNodeMask = nodeMask;

			return result;
		}

		static D3D12_RESOURCE_DESC BufferDesc(
			UINT64 width,
			D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
			UINT64 alignment = 0)
		{
			D3D12_RESOURCE_DESC result;
			result.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			result.Alignment = alignment;
			result.Width = width;
			result.Height = 1;
			result.DepthOrArraySize = 1;
			result.MipLevels = 1;
			result.Format = DXGI_FORMAT_UNKNOWN;
			result.SampleDesc.Count = 1;
			result.SampleDesc.Quality = 0;
			result.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			result.Flags = flags;

			return result;
		}
		static D3D12_SHADER_RESOURCE_VIEW_DESC BufferViewDesc(
			UINT structureByteStride,
			UINT numElements = 1,
			UINT64 firstElement = 0,
			D3D12_BUFFER_SRV_FLAGS flags = D3D12_BUFFER_SRV_FLAG_NONE)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC result = {};
			result.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			result.Format = DXGI_FORMAT_UNKNOWN;
			result.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			result.Buffer.FirstElement = firstElement;
			result.Buffer.NumElements = numElements;
			result.Buffer.StructureByteStride = structureByteStride;
			result.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

			return result;
		}

	};

	class D3D12DescHeapAllocator;

	class D3D12HeapPoolChunkBase
	{
	public:

		void release()
		{
			resource.reset();
			numElements = 0;
			numElementsUasge = 0;
			elementSize = 0;
			mUsageMask.clear();
		}

		bool initialize(uint inNumElements, ID3D12DeviceRHI* device, D3D12_DESCRIPTOR_HEAP_TYPE inType, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
		bool fetchFreeSlot(uint& outSlotIndex);
		void freeSlot(uint slotIndex);

		D3D12DescHeapAllocator* owner = nullptr;
		TComPtr< ID3D12DescriptorHeap > resource;
		uint elementSize = 0;
		uint numElements = 0;
		uint numElementsUasge = 0;

		static constexpr uint32 GroupSize = sizeof(uint32) * 8;
		TArray< uint32 > mUsageMask;

		D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(uint chunkSlot)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE result = mCachedCPUHandle;
			result.ptr += chunkSlot * elementSize;
			return result;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(uint chunkSlot)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE result = mCachedGPUHandle;
			result.ptr += chunkSlot * elementSize;
			return result;
		}
	protected:
		void cacheHandle()
		{
			mCachedCPUHandle = resource->GetCPUDescriptorHandleForHeapStart();
			mCachedGPUHandle = resource->GetGPUDescriptorHandleForHeapStart();
		}

		D3D12_GPU_DESCRIPTOR_HANDLE mCachedGPUHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE mCachedCPUHandle;
	};


	struct D3D12PooledHeapHandle
	{
		D3D12PooledHeapHandle() { chunk = nullptr; }


		bool isValid() const { return !!chunk; }

		D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle() const
		{
			CHECK(chunk);
			return chunk->getCPUHandle(chunkSlot);
		}
		D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle() const
		{
			CHECK(chunk);
			return chunk->getGPUHandle(chunkSlot);
		}

		void reset()
		{
			chunk = nullptr;
		}

		D3D12HeapPoolChunkBase* chunk;
		uint chunkSlot;

	};

	class D3D12HeapPoolChunk : public D3D12HeapPoolChunkBase
	{
	public:
		uint fetchFreeSlotFirstTime();

		static constexpr D3D12HeapPoolChunk* NoLinkPtr = (D3D12HeapPoolChunk*)INT_PTR(-1);
		D3D12HeapPoolChunk* next = NoLinkPtr;
	};

	class D3D12DescHeapAllocator
	{
	public:
		TArray< D3D12HeapPoolChunk* > chunks;
		D3D12HeapPoolChunk* availableChunk;

		D3D12_DESCRIPTOR_HEAP_TYPE type;
		D3D12_DESCRIPTOR_HEAP_FLAGS flags;
		uint32 chunkSize;

		bool fetchFreeHandle(ID3D12DeviceRHI* device, D3D12PooledHeapHandle& outHandle);

		void freeHandle(D3D12PooledHeapHandle& handle);

		void updateAvailableChunk();

		void release();
	};

	class D3D12DescHeapGrowable : public D3D12HeapPoolChunkBase
	{
	public:
		D3D12_DESCRIPTOR_HEAP_TYPE type;
		D3D12_DESCRIPTOR_HEAP_FLAGS flags;

		bool initialize(uint size, ID3D12DeviceRHI* device);

		template< typename TFunc >
		bool fetchFreeHandle(ID3D12DeviceRHI* device, D3D12PooledHeapHandle& outHandle, TFunc&& func);

		bool grow(uint numElementsNew, ID3D12DeviceRHI* device);

		void release();

		TComPtr< ID3D12DescriptorHeap > mResourceCopy;
		D3D12_CPU_DESCRIPTOR_HANDLE     mCachedCPUHandleCopy;
	};

	class D3D12DescriptorHeapPool
	{
	public:
		CORE_API static D3D12DescriptorHeapPool& Get();

		void initialize(ID3D12DeviceRHI* device);
		template< typename TDesc >
		static D3D12PooledHeapHandle Alloc(ID3D12Resource* resource, TDesc const* desc)
		{
			return D3D12DescriptorHeapPool::Get().alloc(resource, desc);
		}

		static D3D12PooledHeapHandle Alloc(D3D12_SAMPLER_DESC const& desc)
		{
			return D3D12DescriptorHeapPool::Get().alloc(desc);
		}

		static void FreeHandle(D3D12PooledHeapHandle& handle)
		{
			if (handle.isValid())
			{
				D3D12DescriptorHeapPool::Get().freeHandle(handle);
			}
		}
		void releaseRHI();

		D3D12PooledHeapHandle alloc(ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC const* desc);
		D3D12PooledHeapHandle alloc(ID3D12Resource* resource, D3D12_CONSTANT_BUFFER_VIEW_DESC const* desc);
		D3D12PooledHeapHandle alloc(ID3D12Resource* resource, D3D12_UNORDERED_ACCESS_VIEW_DESC const* desc);
		D3D12PooledHeapHandle alloc(ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC const* desc);
		D3D12PooledHeapHandle alloc(ID3D12Resource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC const* desc);
		D3D12PooledHeapHandle alloc(D3D12_SAMPLER_DESC const& desc);

		void freeHandle(D3D12PooledHeapHandle& handle);



		ID3D12DeviceRHI* mDevice;

		bool bInitialized = false;

		D3D12DescHeapGrowable mCSUData;
		D3D12DescHeapGrowable mSamplerData;
		D3D12DescHeapAllocator mRTVData;
		D3D12DescHeapAllocator mDSVData;

	};


}//namespace Render

#endif // D3D12Utility_H_8286EFBB_EBBD_42EF_A628_A96A7810CF00