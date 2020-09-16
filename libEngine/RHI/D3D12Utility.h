
#pragma once

#ifndef D3D12Utility_H_8286EFBB_EBBD_42EF_A628_A96A7810CF00
#define D3D12Utility_H_8286EFBB_EBBD_42EF_A628_A96A7810CF00

#include <d3d12.h>

namespace Render
{
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
	};



}//namespace Render

#endif // D3D12Utility_H_8286EFBB_EBBD_42EF_A628_A96A7810CF00