#include "D3D12Command.h"
#include "D3D12ShaderCommon.h"
#include "D3D12Utility.h"

#include "RHIGlobalResource.h"

#include "LogSystem.h"
#include "GpuProfiler.h"
#include "MarcoCommon.h"

#pragma comment(lib , "DXGI.lib")



#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

namespace Render
{

	D3D12System::~D3D12System()
	{

	}

	bool D3D12System::initialize(RHISystemInitParams const& initParam)
	{
		HRESULT hr;
		UINT dxgiFactoryFlags = 0;

		// Enable the debug layer (requires the Graphics Tools "optional feature").
		// NOTE: Enabling the debug layer after device creation will invalidate the active device.
		if ( initParam.bDebugMode && !GRHIPrefEnabled )
		{
			TComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();

				// Enable additional debug layers.
				dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			}
		}

		TComPtr<IDXGIFactory4> factory;
		CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));

		auto minimalFeatureLevel = D3D_FEATURE_LEVEL_11_0;
		bool const bUseWarpDevice = false;
		if (bUseWarpDevice)
		{
			TComPtr<IDXGIAdapter> warpAdapter;
			factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));

			VERIFY_D3D_RESULT_RETURN_FALSE(D3D12CreateDevice(
				warpAdapter,
				minimalFeatureLevel,
				IID_PPV_ARGS(&mDevice)));
		}
		else
		{
			TComPtr<IDXGIAdapter1> hardwareAdapter;
		
			VERIFY_RETURN_FALSE( getHardwareAdapter(factory , minimalFeatureLevel, &hardwareAdapter) );

			VERIFY_D3D_RESULT_RETURN_FALSE(D3D12CreateDevice(
				hardwareAdapter,
				minimalFeatureLevel,
				IID_PPV_ARGS(&mDevice)
			));

			DXGI_ADAPTER_DESC1 adapterDesc;
			if (SUCCEEDED(hardwareAdapter->GetDesc1(&adapterDesc)))
			{
				switch (adapterDesc.VendorId)
				{
				case 0x10DE: GRHIDeviceVendorName = DeviceVendorName::NVIDIA; break;
				case 0x8086: GRHIDeviceVendorName = DeviceVendorName::Intel; break;
				case 0x1002: GRHIDeviceVendorName = DeviceVendorName::ATI; break;
				}
			}
		}

		GRHIClipZMin = 0;
		GRHIProjectYSign = 1;
		GRHIVericalFlip = -1;

		D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels{};
		if (SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(featureLevels))))
		{

		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS3 featureOption3 = {};
		if (SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &featureOption3, sizeof(featureOption3))))
		{
			if (featureOption3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED)
			{
			}
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureOption5 = {};
		if (SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureOption5, sizeof(featureOption5))) )
		{
			GRHISupportRayTracing = featureOption5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureOption7 = {};
		if ( SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureOption7, sizeof(featureOption7))) )
		{
			GRHISupportMeshShader = featureOption7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
		}

		D3D12DescriptorHeapPool::Get().initialize(mDevice);


		if (!mRenderContext.initialize(this))
		{
			return false;
		}

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCopyCmdQueue)));
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&mCopyCmdAllocator)));
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, mCopyCmdAllocator, nullptr, IID_PPV_ARGS(&mCopyCmdList)));
		mCopyCmdList->Close();
		mImmediateCommandList = new RHICommandListImpl(mRenderContext);

		return true;
	}

	void D3D12System::shutdown()
	{
		mSwapChain->release();

		releaseShaderBoundState();

		D3D12DescriptorHeapPool::Get().releaseRHI();
	}

	ShaderFormat* D3D12System::createShaderFormat()
	{
		return new ShaderFormatHLSL_D3D12( mDevice );
	}

	RHISwapChain* D3D12System::RHICreateSwapChain(SwapChainCreationInfo const& info)
	{
		UINT dxgiFactoryFlags = 0;
		TComPtr<IDXGIFactory4> factory;
		CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));

		// Describe and create the swap chain.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = info.bufferCount;
		swapChainDesc.Width = info.extent.x;
		swapChainDesc.Height = info.extent.y;
		swapChainDesc.Format = D3D12Translate::To(info.colorForamt);
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = info.numSamples;

		TComPtr<IDXGISwapChain1> swapChain;
		VERIFY_D3D_RESULT(factory->CreateSwapChainForHwnd(
			mRenderContext.mCommandQueue,        // Swap chain needs the queue so that it can force a flush on it.
			info.windowHandle,
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain),
			return nullptr; );

		// This sample does not support fullscreen transitions.
		VERIFY_D3D_RESULT(factory->MakeWindowAssociation(info.windowHandle, DXGI_MWA_NO_ALT_ENTER), return nullptr; );

		TComPtr<IDXGISwapChainRHI> swapChainRHI;
		if (!swapChain.castTo(swapChainRHI))
			return nullptr;

		D3D12SwapChain* result = new D3D12SwapChain;
		if (!result->initialize(swapChainRHI, mDevice, info.bufferCount))
		{
			delete result;
			return nullptr;
		}
		mSwapChain = result;

		mRenderContext.configFromSwapChain(mSwapChain);
		return result;
	}

	Render::RHITexture2D* D3D12System::RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 createFlags, void* data, int dataAlign)
	{

		// Describe and create a Texture2D.
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = (numMipLevel) ? numMipLevel : 1;
		textureDesc.Format = D3D12Translate::To(format);
		textureDesc.Width = w;
		textureDesc.Height = h;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = numSamples ? numSamples : 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		TComPtr<ID3D12Resource> textureResource;
		VERIFY_D3D_RESULT(mDevice->CreateCommittedResource(
			&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&textureResource)), return nullptr;);


		if (data)
		{
			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(textureResource, 0, 1) + D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;


			TComPtr<ID3D12Resource> textureCopy;

			// Create the GPU upload buffer.
			VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommittedResource(
				&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&FD3D12Init::BufferDesc(uploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&textureCopy)), return nullptr);

			// Copy data to the intermediate upload heap and then schedule a copy 
			// from the upload heap to the Texture2D.
			D3D12_SUBRESOURCE_DATA textureData = {};
			textureData.pData = data;
			textureData.RowPitch = w * Texture::GetFormatSize(format);
			textureData.SlicePitch = textureData.RowPitch * h;

			mCopyCmdAllocator->Reset();
			mCopyCmdList->Reset(mCopyCmdAllocator, nullptr);

			UpdateSubresources(mCopyCmdList, textureResource, textureCopy, 0, 0, 1, &textureData);

			mCopyResources.push_back(std::move(textureCopy));

			mCopyCmdList->Close();
			ID3D12CommandList* ppCommandLists[] = { mCopyCmdList };
			mCopyCmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

			waitCopyCommand();
		}


		D3D12Texture2D* texture = new D3D12Texture2D;
		if (!texture->initialize(textureResource, w, h ))
		{
			delete texture;
			return nullptr;
		}

		if (createFlags & TCF_CreateSRV)
		{
			// Describe and create a SRV for the texture.
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = textureDesc.Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;

			texture->mSRV = D3D12DescriptorHeapPool::Get().allocSRV(texture->mResource, &srvDesc);
		}

		return texture;
	}

	RHIVertexBuffer* D3D12System::RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		int bufferSize = vertexSize * numVertices;

		if (creationFlags & BCF_UsageConst)
		{
			
			bufferSize = ConstBufferMultipleSize * ( (bufferSize + ConstBufferMultipleSize - 1) / ConstBufferMultipleSize);
		}

		TComPtr< ID3D12Resource > resource;
		VERIFY_D3D_RESULT(mDevice->CreateCommittedResource(
			&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&FD3D12Init::BufferDesc(bufferSize) ,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&resource)), return nullptr; );

		// Copy the triangle data to the vertex buffer.
		if (data)
		{
			UINT8* pVertexDataBegin;
			D3D12_RANGE readRange = {};        // We do not intend to read from this resource on the CPU.
			VERIFY_D3D_RESULT(resource->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)), return nullptr; );
			memcpy(pVertexDataBegin, data, bufferSize);
			resource->Unmap(0, nullptr);
		}

		D3D12VertexBuffer* result = new D3D12VertexBuffer;
		if (!result->initialize(resource, mDevice, vertexSize, numVertices))
		{
			delete result;
			return nullptr;
		}

		if (creationFlags & BCF_CreateSRV)
		{
			if (creationFlags & BCF_UsageConst)
			{
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				cbvDesc.BufferLocation = result->mResource->GetGPUVirtualAddress();
				cbvDesc.SizeInBytes = bufferSize;
				result->mViewHandle = D3D12DescriptorHeapPool::Get().allocCBV(resource, &cbvDesc);
			}
		}

		return result;
	}

	void* D3D12System::RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		D3D12_RANGE range = {};
		void* result = nullptr;
		static_cast<D3D12VertexBuffer*>(buffer)->mResource->Map(0, &range, &result);
		return result;
	}

	void D3D12System::RHIUnlockBuffer(RHIVertexBuffer* buffer)
	{
		D3D12_RANGE range = {};
		static_cast<D3D12VertexBuffer*>(buffer)->mResource->Unmap(0, nullptr);
	}

	RHIInputLayout* D3D12System::RHICreateInputLayout(InputLayoutDesc const& desc)
	{
		return new D3D12InputLayout(desc);
	}

	RHISamplerState* D3D12System::RHICreateSamplerState(SamplerStateInitializer const& initializer)
	{
		return new D3D12SamplerState(initializer);
	}

	RHIRasterizerState* D3D12System::RHICreateRasterizerState(RasterizerStateInitializer const& initializer)
	{
		return new D3D12RasterizerState(initializer);
	}

	RHIBlendState* D3D12System::RHICreateBlendState(BlendStateInitializer const& initializer)
	{
		return new D3D12BlendState(initializer);
	}

	RHIDepthStencilState* D3D12System::RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer)
	{
		return new D3D12DepthStencilState(initializer);
	}

	RHIShader* D3D12System::RHICreateShader(EShader::Type type)
	{
		return new D3D12Shader(type);
	}

	_Use_decl_annotations_
	bool D3D12System::getHardwareAdapter(IDXGIFactory1* pFactory, D3D_FEATURE_LEVEL featureLevel, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter)
	{
		*ppAdapter = nullptr;

		TComPtr<IDXGIAdapter1> adapter;

		TComPtr<IDXGIFactory6> factory6;
		if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
		{
			for (
				UINT adapterIndex = 0;
				DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(
					adapterIndex,
					requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
					IID_PPV_ARGS(&adapter));
				++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					// Don't select the Basic Render Driver adapter.
					// If you want a software adapter, pass in "/warp" on the command line.
					continue;
				}

				// Check to see whether the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(adapter , featureLevel, _uuidof(ID3D12DeviceRHI), nullptr)))
				{
					break;
				}
			}
		}
		else
		{
			for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					// Don't select the Basic Render Driver adapter.
					// If you want a software adapter, pass in "/warp" on the command line.
					continue;
				}

				// Check to see whether the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(adapter, featureLevel, _uuidof(ID3D12DeviceRHI), nullptr)))
				{
					break;
				}
			}
		}

		*ppAdapter = adapter.detach();

		return *ppAdapter != nullptr;
	}

	UINT64 D3D12System::UpdateSubresources(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pDestinationResource, ID3D12Resource* pIntermediate, UINT FirstSubresource, UINT NumSubresources, UINT64 RequiredSize, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts, const UINT* pNumRows, const UINT64* pRowSizesInBytes, const D3D12_SUBRESOURCE_DATA* pSrcData) noexcept
	{
		// Minor validation
		auto IntermediateDesc = pIntermediate->GetDesc();
		auto DestinationDesc = pDestinationResource->GetDesc();
		if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
			IntermediateDesc.Width < RequiredSize + pLayouts[0].Offset ||
			RequiredSize > SIZE_T(-1) ||
			(DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
			(FirstSubresource != 0 || NumSubresources != 1)))
		{
			return 0;
		}

		BYTE* pData;
		HRESULT hr = pIntermediate->Map(0, nullptr, reinterpret_cast<void**>(&pData));
		if (FAILED(hr))
		{
			return 0;
		}

		for (UINT i = 0; i < NumSubresources; ++i)
		{
			if (pRowSizesInBytes[i] > SIZE_T(-1)) return 0;
			D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, SIZE_T(pLayouts[i].Footprint.RowPitch) * SIZE_T(pNumRows[i]) };
			MemcpySubresource(&DestData, &pSrcData[i], static_cast<SIZE_T>(pRowSizesInBytes[i]), pNumRows[i], pLayouts[i].Footprint.Depth);
		}
		pIntermediate->Unmap(0, nullptr);

		if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			pCmdList->CopyBufferRegion(
				pDestinationResource, 0, pIntermediate, pLayouts[0].Offset, pLayouts[0].Footprint.Width);
		}
		else
		{
			for (UINT i = 0; i < NumSubresources; ++i)
			{
				D3D12_TEXTURE_COPY_LOCATION Dst;
				Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				Dst.pResource = pDestinationResource;
				Dst.PlacedFootprint = {};
				Dst.SubresourceIndex = i + FirstSubresource;
				D3D12_TEXTURE_COPY_LOCATION Src;
				Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				Src.pResource = pIntermediate;
				Src.PlacedFootprint = pLayouts[i];
				pCmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
			}
		}
		return RequiredSize;
	}

	UINT64 D3D12System::UpdateSubresources(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pDestinationResource, ID3D12Resource* pIntermediate, UINT64 IntermediateOffset, UINT FirstSubresource, UINT NumSubresources, const D3D12_SUBRESOURCE_DATA* pSrcData)
	{
		UINT64 RequiredSize = 0;
		UINT64 MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * NumSubresources;
		if (MemToAlloc > SIZE_MAX)
		{
			return 0;
		}
		void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
		if (pMem == nullptr)
		{
			return 0;
		}
		auto pLayouts = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
		UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + NumSubresources);
		UINT* pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + NumSubresources);

		auto Desc = pDestinationResource->GetDesc();
		ID3D12Device* pDevice = nullptr;
		pDestinationResource->GetDevice(IID_ID3D12Device, reinterpret_cast<void**>(&pDevice));
		pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
		pDevice->Release();

		UINT64 Result = UpdateSubresources(pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, pLayouts, pNumRows, pRowSizesInBytes, pSrcData);
		HeapFree(GetProcessHeap(), 0, pMem);
		return Result;
	}

	D3D12ShaderBoundState* D3D12System::getShaderBoundState(GraphicsShaderStateDesc const& state)
	{
		D3D12ShaderBoundStateKey key;
		key.initialize(state);
		auto iter = mShaderBoundStateMap.find(key);
		if (iter != mShaderBoundStateMap.end())
		{
			return iter->second;
		}

		std::unique_ptr< D3D12ShaderBoundState > boundState = std::make_unique< D3D12ShaderBoundState>();

		std::vector< D3D12_STATIC_SAMPLER_DESC > rootSamplers;
		std::vector< D3D12_ROOT_PARAMETER1 > rootParameters;

		D3D12_ROOT_SIGNATURE_FLAGS denyShaderRootAccessFlags =
			D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

		auto AddShader = [&](RHIShader* shader) -> bool
		{
			if (shader == nullptr)
				return false;
			D3D12ShaderBoundState::ShaderInfo info;
			info.resource = shader;
			info.rootSlotStart = rootParameters.size();
			boundState->mShaders.push_back(info);

			auto shaderImpl = static_cast<D3D12Shader*>(shader);
			bool result = false;
			for (int i = 0; i < shaderImpl->mRootSignature.descRanges.size(); ++i)
			{
				D3D12_ROOT_PARAMETER1 parameter = shaderImpl->getRootParameter(i);
				rootParameters.push_back(parameter);
				result = true;
			}
			rootSamplers.insert(rootSamplers.end(), shaderImpl->mRootSignature.samplers.begin(), shaderImpl->mRootSignature.samplers.end());
			return result;
		};

		if (AddShader(state.vertex))
			denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;

		if (AddShader(state.pixel))
			denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		if (AddShader(state.geometry))
			denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		if (AddShader(state.hull))
			denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

		if (AddShader(state.domain))
			denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSignatureDesc.Desc_1_1.NumParameters = rootParameters.size();
		rootSignatureDesc.Desc_1_1.pParameters = rootParameters.data();
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = rootSamplers.size();
		rootSignatureDesc.Desc_1_1.pStaticSamplers = rootSamplers.data();
		rootSignatureDesc.Desc_1_1.Flags = denyShaderRootAccessFlags | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		TComPtr<ID3DBlob> signature;
		TComPtr<ID3DBlob> error;
		if (FAILED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error)))
		{
			LogWarning(0, (char const*)error->GetBufferPointer());
			return nullptr;
		}
		VERIFY_D3D_RESULT(mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&boundState->mRootSignature)), return nullptr;);


		D3D12ShaderBoundState* result = boundState.release();
		result->cachedKey = key;
		mShaderBoundStateMap.emplace(key, result);
		return result;
	}

	D3D12PipelineState* D3D12System::getPipelineState(GraphicsPipelineStateDesc const& stateDesc, D3D12ShaderBoundState* boundState, D3D12RenderTargetsState* renderTargetsState)
	{

		D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12Translate::ToTopologyType(stateDesc.primitiveType);
		D3D12PipelineStateKey key;
		key.initialize(stateDesc, boundState, renderTargetsState, topologyType);
		auto iter = mPipelineStateMap.find(key);
		if (iter != mPipelineStateMap.end())
		{
			return iter->second;
		}

		D3D12PipelineStateStream stateStream;

		for (auto& shaderInfo : boundState->mShaders)
		{
			switch (shaderInfo.resource->getType())
			{
			case EShader::Vertex:
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS>();
					data = static_cast<D3D12Shader*>(shaderInfo.resource.get())->getByteCode();
				}
				break;
			case EShader::Pixel:
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS>();
					data = static_cast<D3D12Shader*>(shaderInfo.resource.get())->getByteCode();
				}
				break;
			case EShader::Geometry:
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS>();
					data = static_cast<D3D12Shader*>(shaderInfo.resource.get())->getByteCode();
				}
				break;
			case EShader::Hull:
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS>();
					data = static_cast<D3D12Shader*>(shaderInfo.resource.get())->getByteCode();
				}
				break;
			case EShader::Domain:
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS>();
					data = static_cast<D3D12Shader*>(shaderInfo.resource.get())->getByteCode();
				}
				break;
			case EShader::Mesh:
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS>();
					data = static_cast<D3D12Shader*>(shaderInfo.resource.get())->getByteCode();
				}
				break;
			case EShader::Task:
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS>();
					data = static_cast<D3D12Shader*>(shaderInfo.resource.get())->getByteCode();
				}
				break;
			}
		}

		if (stateDesc.inputLayout)
		{
			auto& inputLayout = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT>();
			inputLayout = static_cast<D3D12InputLayout*>(stateDesc.inputLayout)->getDesc();
		}

#if 0
		D3D12_VIEW_INSTANCE_LOCATION viewInstanceLocations[D3D12_MAX_VIEW_INSTANCE_COUNT];
		if (true)
		{
			auto& viewInstancing = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING>();
			for (int i = 0; i < 4; ++i)
			{
				viewInstanceLocations[i].RenderTargetArrayIndex = 0;
				viewInstanceLocations[i].ViewportArrayIndex = i;
			}
			viewInstancing.pViewInstanceLocations = viewInstanceLocations;
			viewInstancing.ViewInstanceCount = ARRAY_SIZE(viewInstanceLocations);
			viewInstancing.Flags = D3D12_VIEW_INSTANCING_FLAG_NONE;
		}
#endif

		struct FixedPipelineStateStream
		{
			TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE > pRootSignature;
			TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS > RTFormatArray;
			TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY > PrimitiveTopologyType;
			TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER > RasterizerState;
			TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_RHI > DepthStencilState;
			TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND > BlendState;
		};

		auto& streamDesc = stateStream.addDataT< FixedPipelineStateStream >();

		streamDesc.pRootSignature = boundState->mRootSignature;

		RHIRasterizerState* rasterizerStateUsage = stateDesc.rasterizerState ? stateDesc.rasterizerState : &TStaticRasterizerState<>::GetRHI();
		streamDesc.RasterizerState = static_cast<D3D12RasterizerState*>(rasterizerStateUsage)->mDesc;
		RHIDepthStencilState* depthStencielStateUsage = stateDesc.depthStencilState ? stateDesc.depthStencilState : &TStaticDepthStencilState<>::GetRHI();
		streamDesc.DepthStencilState = static_cast<D3D12DepthStencilState*>(depthStencielStateUsage)->mDesc;
		RHIBlendState* blendStateUsage = stateDesc.blendState ? stateDesc.blendState : &TStaticBlendState<>::GetRHI();
		streamDesc.BlendState = static_cast<D3D12BlendState*>(blendStateUsage)->mDesc;

		streamDesc.PrimitiveTopologyType = topologyType;

		streamDesc.RTFormatArray.RTFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
		streamDesc.RTFormatArray.NumRenderTargets = 1;
		TComPtr<ID3D12PipelineState> resource;
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreatePipelineState(&stateStream.getDesc(), IID_PPV_ARGS(&resource)));

		auto result = new D3D12PipelineState;
		result->mResource = resource.detach();
		mPipelineStateMap.emplace(key, result);
		return result;

	}

	bool D3D12Context::initialize(D3D12System* system)
	{
		mDevice = system->mDevice;
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

		for (int i = 0; i < EShader::Count; ++i)
		{
			VERIFY_RETURN_FALSE( mResourceBoundStates[i].initialize(mDevice) );
		}

		return true;
	}

	void D3D12Context::release()
	{
		waitForGpu();

		mCommandAllocator.reset();
		mCommandQueue.reset();

		mFence.reset();
		CloseHandle(mFenceEvent);

		mDevice.reset();
	}

	void D3D12Context::RHISetViewport(float x, float y, float w, float h, float zNear, float zFar)
	{
		mGraphicsCmdList->RSSetViewports(1, &FD3D12Init::Viewport(x, y, w, h, zNear, zFar));
		D3D12_RECT scissorRect;
		scissorRect.left = x;
		scissorRect.top = y;
		scissorRect.right = x + w;
		scissorRect.bottom = y + h;
		mGraphicsCmdList->RSSetScissorRects(1, &scissorRect);
	}

	void D3D12Context::RHISetViewports(ViewportInfo const viewports[], int numViewports)
	{
		D3D12_VIEWPORT viewportsD3D[8];
		D3D12_RECT scissorRects[8];
		assert(numViewports < ARRAY_SIZE(viewportsD3D));
		for (int i = 0; i < numViewports; ++i)
		{
			ViewportInfo const& vp = viewports[i];
			D3D12_VIEWPORT& viewport = viewportsD3D[i];
			viewport.TopLeftX = vp.x;
			viewport.TopLeftY = vp.y;
			viewport.Width = vp.w;
			viewport.Height = vp.h;
			viewport.MinDepth = vp.zNear;
			viewport.MaxDepth = vp.zFar;

			D3D12_RECT& scissorRect = scissorRects[i];
			scissorRect.left = vp.x;
			scissorRect.top = vp.y;
			scissorRect.right = vp.x + vp.w;
			scissorRect.bottom = vp.y + vp.h;
		}
		mGraphicsCmdList->RSSetViewports(numViewports, viewportsD3D);
		mGraphicsCmdList->RSSetScissorRects(numViewports, scissorRects);
	}

	void D3D12Context::RHISetScissorRect(int x, int y, int w, int h)
	{
		D3D12_RECT scissorRect;
		scissorRect.left = x;
		scissorRect.top = y;
		scissorRect.right = x + w;
		scissorRect.bottom = y + h;
		mGraphicsCmdList->RSSetScissorRects(1, &scissorRect);
	}

	void D3D12Context::RHISetFrameBuffer(RHIFrameBuffer* frameBuffer)
	{
		D3D12RenderTargetsState* newState;
		bool bForceReset = false;
		if (frameBuffer == nullptr)
		{
			D3D12SwapChain* swapChain = static_cast<D3D12System*>(GRHISystem)->mSwapChain.get();
			if (swapChain == nullptr)
			{
				mGraphicsCmdList->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
				mRenderTargetsState = nullptr;
				return;
			}

			newState = &swapChain->mRenderTargetsStates[mFrameIndex];
		}
		else
		{
			D3D12FrameBuffer* frameBufferImpl = static_cast<D3D12FrameBuffer*>(frameBuffer);
			newState = &frameBufferImpl->mRenderTargetsState;
			bForceReset = frameBufferImpl->bStateDirty;
			frameBufferImpl->bStateDirty = false;
		}

		CHECK(newState);
		if (bForceReset || mRenderTargetsState != newState)
		{
			mRenderTargetsState = newState;
			//for (int i = 0; i < mRenderTargetsState->numColorBuffers; ++i)
			//{
			//	clearSRVResource(*mRenderTargetsState->colorResources[i]);
			//}

			//if (mRenderTargetsState->depthBuffer)
			//{
			//	clearSRVResource(*mRenderTargetsState->depthResource);
			//}

			D3D12_CPU_DESCRIPTOR_HANDLE handles[D3D12RenderTargetsState::MaxSimulationBufferCount];
			for (int i = 0; i < mRenderTargetsState->numColorBuffers; ++i)
			{
				handles[i] = mRenderTargetsState->colorBuffers[i].RTVHandle.getCPUHandle();
			}

			mGraphicsCmdList->OMSetRenderTargets(mRenderTargetsState->numColorBuffers, handles, FALSE, nullptr);
		}
	}

	void D3D12Context::RHIClearRenderTargets(EClearBits clearBits, LinearColor colors[], int numColor, float depth, uint8 stenceil)
	{
		if (mRenderTargetsState == nullptr)
			return;
		if (HaveBits(clearBits, EClearBits::Color))
		{
			for (int i = 0; i < numColor; ++i)
			{
				auto const& RTVHandle = mRenderTargetsState->colorBuffers[i].RTVHandle;
				if (RTVHandle.isValid())
				{
					mGraphicsCmdList->ClearRenderTargetView(RTVHandle.getCPUHandle(), colors[i], 0, nullptr);
				}

			}
		}
		if (HaveBits(clearBits, EClearBits::Depth | EClearBits::Stencil))
		{




		}
	}

	void D3D12Context::RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream)
	{
		mInputLayoutPending = inputLayout;
		D3D12_VERTEX_BUFFER_VIEW bufferViews[16];
		assert(numInputStream < ARRAY_SIZE(bufferViews));

		for (int i = 0; i < numInputStream; ++i)
		{
			auto& bufferView = bufferViews[i];
			if (inputStreams[i].buffer)
			{
				D3D12_GPU_VIRTUAL_ADDRESS address = static_cast<D3D12VertexBuffer&>(*inputStreams[i].buffer).getResource()->GetGPUVirtualAddress();
				address += inputStreams[i].offset;
				bufferView.BufferLocation = address;
				bufferView.SizeInBytes = inputStreams[i].buffer->getSize() - inputStreams[i].offset;
				bufferView.StrideInBytes = (inputStreams[i].stride >= 0) ? inputStreams[i].stride : inputStreams[i].buffer->getElementSize();
			}
			else
			{
				bufferView.BufferLocation = 0;
				bufferView.SizeInBytes = 0;
				bufferView.StrideInBytes = 0;
			}
		}

		mGraphicsCmdList->IASetVertexBuffers(0, numInputStream, bufferViews);
	}

	void D3D12Context::RHIFlushCommand()
	{
		mGraphicsCmdList->Close();
		ID3D12CommandList* ppCommandLists[] = { mGraphicsCmdList };
		mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	void D3D12Context::RHISetGraphicsShaderBoundState(GraphicsShaderStateDesc const& state)
	{
		mBoundState = static_cast<D3D12System*>(GRHISystem)->getShaderBoundState(state);

		if (mBoundState)
		{
			mGraphicsCmdList->SetGraphicsRootSignature(mBoundState->mRootSignature);
			mCSUHeapUsageMask = 0;
			mSamplerHeapUsageMAsk = 0;
			mUsedHeaps.clear();

			for (auto& shaderInfo : mBoundState->mShaders)
			{
				mRootSlotStart[shaderInfo.resource->getType()] = shaderInfo.rootSlotStart;
			}

		}
	}

	void D3D12Context::setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector4 const val[], int dim)
	{
		auto& shaderImpl = static_cast<D3D12Shader&>(shader);

		auto const& slotInfo = shaderImpl.mRootSignature.Slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shader.mType] + slotInfo.slotOffset;

		auto& resourceBoundState = mResourceBoundStates[shader.mType];

		resourceBoundState.UpdateConstantData(val, slotInfo.dataOffset, slotInfo.dataSize);

		auto handle = resourceBoundState.mConstBufferHandle;
		if (!(mCSUHeapUsageMask & BIT(handle.chunk->id)))
		{
			mCSUHeapUsageMask |= BIT(handle.chunk->id);
			mUsedHeaps.push_back(handle.chunk->resource);
			mGraphicsCmdList->SetDescriptorHeaps(mUsedHeaps.size(), mUsedHeaps.data());
		}
		mGraphicsCmdList->SetGraphicsRootDescriptorTable(rootSlot, handle.getGPUHandle());
	}

	void D3D12Context::setShaderValue(RHIShader& shader, ShaderParameter const& param, float const val[], int dim)
	{
		auto& shaderImpl = static_cast<D3D12Shader&>(shader);

		auto const& slotInfo = shaderImpl.mRootSignature.Slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shader.mType] + slotInfo.slotOffset;

		auto& resourceBoundState = mResourceBoundStates[shader.mType];

		resourceBoundState.UpdateConstantData(val, slotInfo.dataOffset, slotInfo.dataSize);

		auto handle = resourceBoundState.mConstBufferHandle;
		if (!(mCSUHeapUsageMask & BIT(handle.chunk->id)))
		{
			mCSUHeapUsageMask |= BIT(handle.chunk->id);
			mUsedHeaps.push_back(handle.chunk->resource);
			mGraphicsCmdList->SetDescriptorHeaps(mUsedHeaps.size(), mUsedHeaps.data());
		}
		mGraphicsCmdList->SetGraphicsRootDescriptorTable(rootSlot, handle.getGPUHandle());
	}

	void D3D12Context::setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture)
	{
		auto& shaderImpl = static_cast<D3D12Shader&>(shader);

		auto const& slotInfo = shaderImpl.mRootSignature.Slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shader.mType] + slotInfo.slotOffset;
		auto handle = static_cast<D3D12Texture2D&>(texture).mSRV;

		if (!(mCSUHeapUsageMask & BIT(handle.chunk->id)))
		{
			mCSUHeapUsageMask |= BIT(handle.chunk->id);
			mUsedHeaps.push_back(handle.chunk->resource);
			mGraphicsCmdList->SetDescriptorHeaps(mUsedHeaps.size(), mUsedHeaps.data());
		}

		mGraphicsCmdList->SetGraphicsRootDescriptorTable(rootSlot, handle.getGPUHandle());
	}

	void D3D12Context::setShaderSampler(RHIShader& shader, ShaderParameter const& param, RHISamplerState& sampler)
	{
		auto& shaderImpl = static_cast<D3D12Shader&>(shader);

		auto const& slotInfo = shaderImpl.mRootSignature.Slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shader.mType] + slotInfo.slotOffset;
		auto handle = static_cast<D3D12SamplerState&>(sampler).mHandle;

		if (!(mSamplerHeapUsageMAsk & BIT(handle.chunk->id)))
		{
			mSamplerHeapUsageMAsk |= BIT(handle.chunk->id);
			mUsedHeaps.push_back(handle.chunk->resource);
			mGraphicsCmdList->SetDescriptorHeaps(mUsedHeaps.size(), mUsedHeaps.data());
		}
		mGraphicsCmdList->SetGraphicsRootDescriptorTable(rootSlot, handle.getGPUHandle());
	}

	void D3D12Context::commitGraphicsState(EPrimitive type)
	{
		mGraphicsCmdList->IASetPrimitiveTopology(D3D12Translate::To(type));

		GraphicsPipelineStateDesc stateDesc;

		stateDesc.primitiveType = type;
		stateDesc.inputLayout = mInputLayoutPending;

		stateDesc.rasterizerState = mRasterizerStatePending;
		stateDesc.depthStencilState = mDepthStencilStatePending;
		stateDesc.blendState = mBlendStateStatePending;

		auto pipelineState = static_cast<D3D12System*>(GRHISystem)->getPipelineState(stateDesc, mBoundState, mRenderTargetsState);
		if (pipelineState)
		{
			mGraphicsCmdList->SetPipelineState(pipelineState->mResource);
		}
	}

	bool D3D12Context::configFromSwapChain(D3D12SwapChain* swapChain)
	{
		int numFrames = swapChain->mRenderTargetsStates.size();

		mFrameDataList.resize(numFrames);
		mFrameIndex = swapChain->mResource->GetCurrentBackBufferIndex();
		for (int i = 0; i < numFrames; ++i)
		{
			VERIFY_RETURN_FALSE(mFrameDataList[i].init(mDevice));
			//if (i != mFrameIndex)
			{
				mFrameDataList[i].graphicsCmdList->Close();
			}
		}

		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateFence(mFrameDataList[mFrameIndex].fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
		mFrameDataList[mFrameIndex].fenceValue += 1;

		mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (mFenceEvent == nullptr)
		{
			VERIFY_D3D_RESULT_RETURN_FALSE(HRESULT_FROM_WIN32(GetLastError()));
		}

		mGraphicsCmdList = mFrameDataList[mFrameIndex].graphicsCmdList;
		return true;
	}

	bool D3D12Context::beginFrame()
	{
		FrameData& frameData = mFrameDataList[mFrameIndex];
		VERIFY_RETURN_FALSE(frameData.reset());
		mGraphicsCmdList = frameData.graphicsCmdList;

		D3D12_RESOURCE_BARRIER barrier = FD3D12Init::TransitionBarrier(
			static_cast<D3D12System*>(GRHISystem)->mSwapChain->mRenderTargetsStates[mFrameIndex].colorBuffers[0].resource,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mGraphicsCmdList->ResourceBarrier(1, &barrier);
		return true;
	}

	void D3D12Context::endFrame()
	{
		D3D12_RESOURCE_BARRIER barrier = FD3D12Init::TransitionBarrier(
			static_cast<D3D12System*>(GRHISystem)->mSwapChain->mRenderTargetsStates[mFrameIndex].colorBuffers[0].resource,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		mGraphicsCmdList->ResourceBarrier(1, &barrier);
	}

	void D3D12Context::waitForGpu()
	{
		waitForGpu(mCommandQueue);
	}


	void D3D12Context::waitForGpu(ID3D12CommandQueue* cmdQueue)
	{
		HRESULT hr;
		// Schedule a Signal command in the queue.
		hr = cmdQueue->Signal(mFence, mFrameDataList[mFrameIndex].fenceValue);

		// Wait until the fence has been processed.
		hr = mFence->SetEventOnCompletion(mFrameDataList[mFrameIndex].fenceValue, mFenceEvent);
		hr = WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);

		// Increment the fence value for the current frame.
		mFrameDataList[mFrameIndex].fenceValue++;
	}

	void D3D12Context::moveToNextFrame(IDXGISwapChainRHI* swapChain)
	{
		// Schedule a Signal command in the queue.
		const UINT64 currentFenceValue = mFrameDataList[mFrameIndex].fenceValue;
		mCommandQueue->Signal(mFence, currentFenceValue);

		// Update the frame index.
		mFrameIndex = swapChain->GetCurrentBackBufferIndex();

		// If the next frame is not ready to be rendered yet, wait until it is ready.
		if (mFence->GetCompletedValue() < mFrameDataList[mFrameIndex].fenceValue)
		{
			mFence->SetEventOnCompletion(mFrameDataList[mFrameIndex].fenceValue, mFenceEvent);
			WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);
		}

		// Set the fence value for the next frame.
		mFrameDataList[mFrameIndex].fenceValue = currentFenceValue + 1;
	}

	bool D3D12ResourceBoundState::initialize(ID3D12DeviceRHI* device)
	{
		uint bufferSize = ConstBufferMultipleSize * (  (2048 + ConstBufferMultipleSize - 1) / ConstBufferMultipleSize );
		VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateCommittedResource(
			&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&FD3D12Init::BufferDesc(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mConstBuffer)));

		D3D12_RANGE readRange = {};        // We do not intend to read from this resource on the CPU.
		VERIFY_D3D_RESULT(mConstBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mConstDataPtr)), return nullptr; );

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = mConstBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = bufferSize;
		mConstBufferHandle = D3D12DescriptorHeapPool::Get().allocCBV(mConstBuffer, &cbvDesc);
		return true;
	}

	void D3D12PipelineStateKey::initialize(GraphicsPipelineStateDesc const& stateDesc, D3D12ShaderBoundState* boundState, D3D12RenderTargetsState* renderTargetsState, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
	{
		boundStateKey = boundState->cachedKey;
		rasterizerID = stateDesc.rasterizerState ? stateDesc.rasterizerState->mGUID : TStaticRasterizerState<>::GetRHI().mGUID;
		depthStenceilID = stateDesc.depthStencilState ? stateDesc.depthStencilState->mGUID : TStaticDepthStencilState<>::GetRHI().mGUID;
		blendID = stateDesc.blendState ? stateDesc.blendState->mGUID : TStaticBlendState<>::GetRHI().mGUID;
		inputLayoutID = stateDesc.inputLayout ? stateDesc.inputLayout->mGUID : 0;
		renderTargetFormatID = renderTargetsState->formatGUID;
		topologyType = topologyType;
	}

}//namespace Render

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif
