
#pragma once

#ifndef D3D12Utility_H_8286EFBB_EBBD_42EF_A628_A96A7810CF00
#define D3D12Utility_H_8286EFBB_EBBD_42EF_A628_A96A7810CF00

#include "MarcoCommon.h"
#include "Core/IntegerType.h"

#include "Platform/Windows/ComUtility.h"
#include "CoreShare.h"

#include <vector>

#include <D3D12.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <dxgi1_6.h>



#undef max
#undef min

namespace Render
{
	using IDXGISwapChainRHI = IDXGISwapChain3;
	using ID3D12DeviceRHI = ID3D12Device8;
	using ID3D12GraphicsCommandListRHI = ID3D12GraphicsCommandList2;

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

	class D3D12HeapPoolChunk
	{
	public:
		D3D12HeapPoolChunk()
		{
			numElements = 0;
		}

		bool initialize(ID3D12DeviceRHI* device, D3D12_DESCRIPTOR_HEAP_TYPE inType, uint32 inNumElements, D3D12_DESCRIPTOR_HEAP_FLAGS flags);


		bool fetchFreeSlot(uint& outSlotIndex);

		void freeSlot(uint slotIndex);


		uint id;
		TComPtr< ID3D12DescriptorHeap > resource;
		D3D12_DESCRIPTOR_HEAP_TYPE type;
		uint elementSize;
		uint numElements;
		uint numElementsUasge;
		std::vector< uint32 > mUsageMask;


		D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(uint chunkSlot)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE result = resource->GetCPUDescriptorHandleForHeapStart();
			result.ptr += chunkSlot * elementSize;
			return result;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(uint chunkSlot)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE result = resource->GetGPUDescriptorHandleForHeapStart();
			result.ptr += chunkSlot * elementSize;
			return result;
		}
	};


	struct D3D12PooledHeapHandle
	{
		D3D12PooledHeapHandle() { chunk = nullptr; }

		D3D12HeapPoolChunk* chunk;
		uint chunkSlot;
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

	};

	class D3D12DescriptorHeapPool
	{
	public:
		CORE_API static D3D12DescriptorHeapPool& Get();

		void initialize(ID3D12DeviceRHI* device);

		void releaseRHI();
		D3D12PooledHeapHandle allocSRV(ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC const* desc);
		D3D12PooledHeapHandle allocCBV(ID3D12Resource* resource, D3D12_CONSTANT_BUFFER_VIEW_DESC const* desc);
		D3D12PooledHeapHandle allocRTV(ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC const* desc);
		D3D12PooledHeapHandle allocSampler(D3D12_SAMPLER_DESC const& desc);

		bool findFreeHandle(std::vector< D3D12HeapPoolChunk* >& chunks , D3D12PooledHeapHandle& outHandle);

		void freeHandle(D3D12PooledHeapHandle& handle);

		static void ReleaseChunks(std::vector< D3D12HeapPoolChunk* >& chunks);
		ID3D12DeviceRHI* mDevice;

		bool bInitialized = false;

		std::vector< D3D12HeapPoolChunk* > mCSUChunks;
		std::vector< D3D12HeapPoolChunk* > mRTVChunks;
		std::vector< D3D12HeapPoolChunk* > mSamplerChunks;
	};


}//namespace Render

#endif // D3D12Utility_H_8286EFBB_EBBD_42EF_A628_A96A7810CF00