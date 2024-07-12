#include "D3D12Command.h"
#include "D3D12ShaderCommon.h"
#include "D3D12Utility.h"

#include "RHIGlobalResource.h"

#include "LogSystem.h"
#include "GpuProfiler.h"
#include "MarcoCommon.h"

#pragma comment(lib , "DXGI.lib")

#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

namespace Render
{
	EXPORT_RHI_SYSTEM_MODULE(RHISystemName::D3D12, D3D12System);

#if 1
	class D3D12ProfileCore : public RHIProfileCore
	{
	public:

		D3D12ProfileCore()
		{
			mCycleToMillisecond = 0;
		}

		bool init(TComPtr<ID3D12DeviceRHI> const& device, double cycleToMillisecond)
		{
			mDevice = device;
			mCycleToMillisecond = cycleToMillisecond;
			VERIFY_RETURN_FALSE(addChunkResource());
			mNextHandle = 1;
			return true;
		}

		bool addChunkResource()
		{
			TimeStampChunk timeStamp;
			D3D12_QUERY_HEAP_DESC desc;
			desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
			desc.NodeMask = 0x1;
			desc.Count = TimeStampChunk::Size * 2;
			VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateQueryHeap(&desc, IID_PPV_ARGS(&timeStamp.heap)));

			D3D12_HEAP_PROPERTIES heapProps;
			heapProps.Type = D3D12_HEAP_TYPE_READBACK;
			heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProps.CreationNodeMask = 1;
			heapProps.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC bufferDesc;
			bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			bufferDesc.Alignment = 0;
			bufferDesc.Width = sizeof(uint64) * TimeStampChunk::Size * 2;
			bufferDesc.Height = 1;
			bufferDesc.DepthOrArraySize = 1;
			bufferDesc.MipLevels = 1;
			bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
			bufferDesc.SampleDesc.Count = 1;
			bufferDesc.SampleDesc.Quality = 0;
			bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
				D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&timeStamp.resultResource)));

			mChunks.push_back(std::move(timeStamp));
			return true;
		}

		virtual void beginFrame() override
		{
			mCmdList->EndQuery(mChunks[0].heap, D3D12_QUERY_TYPE_TIMESTAMP, 0);
		}

		virtual bool endFrame() override
		{
			mCmdList->EndQuery(mChunks[0].heap, D3D12_QUERY_TYPE_TIMESTAMP, 1);
			for (auto& chunk : mChunks)
			{
				mCmdList->ResolveQueryData(chunk.heap, D3D12_QUERY_TYPE_TIMESTAMP, 0, TimeStampChunk::Size * 2, chunk.resultResource, 0);
			}
			return true;
		}

		virtual void beginReadback() override
		{
			for (auto& chunk : mChunks)
			{
				chunk.resultResource->Map(0, NULL, (void**)&chunk.timeStampData);
			}
		}

		virtual void endReadback() override
		{
			for (auto& chunk : mChunks)
			{
				chunk.resultResource->Unmap(0, NULL);
			}
		}

		virtual uint32 fetchTiming() override
		{
			uint32 chunkIndex = mNextHandle / TimeStampChunk::Size;
			uint32 index = 2 * (mNextHandle % TimeStampChunk::Size);

			if (index == 0)
			{
				if (!addChunkResource())
					return RHI_ERROR_PROFILE_HANDLE;
			}

			uint32 result = mNextHandle;
			++mNextHandle;
			return result;
		}

		virtual void startTiming(uint32 timingHandle) override
		{
			uint32 chunkIndex = timingHandle / TimeStampChunk::Size;
			uint32 index = 2 * ( timingHandle % TimeStampChunk::Size );
			mCmdList->EndQuery(mChunks[chunkIndex].heap, D3D12_QUERY_TYPE_TIMESTAMP, index + 0);
		}

		virtual void endTiming(uint32 timingHandle) override
		{
			uint32 chunkIndex = timingHandle / TimeStampChunk::Size;
			uint32 index = 2 * (timingHandle % TimeStampChunk::Size);
			mCmdList->EndQuery(mChunks[chunkIndex].heap, D3D12_QUERY_TYPE_TIMESTAMP, index + 1 );
		}

		virtual bool getTimingDuration(uint32 timingHandle, uint64& outDuration) override
		{
			uint32 chunkIndex = timingHandle / TimeStampChunk::Size;
			uint32 index = 2 * (timingHandle % TimeStampChunk::Size);

			auto timeStampData = mChunks[chunkIndex].timeStampData;
			if (timeStampData)
			{
				uint64 startData = mChunks[chunkIndex].timeStampData[index];
				uint64 endData = mChunks[chunkIndex].timeStampData[index + 1];
				outDuration = endData - startData;
			}
			else
			{
				outDuration = 0.0;
			}
			return true;
		}

		virtual double getCycleToMillisecond() override
		{
			return mCycleToMillisecond;
		}

		virtual void releaseRHI() override
		{
			mChunks.clear();
			mDevice.reset();
		}
		double mCycleToMillisecond;


		struct TimeStampChunk
		{
			TComPtr< ID3D12QueryHeap > heap;
			TComPtr< ID3D12Resource >  resultResource;
			uint64* timeStampData;
			
			static constexpr uint32 Size = 512;
			void releaseRHI()
			{
				heap.reset();
				resultResource.reset();
			}
		};
		uint32 mNextHandle;
		TArray< TimeStampChunk > mChunks;
		ID3D12GraphicsCommandListRHI* mReadBackCmdList;
		ID3D12GraphicsCommandListRHI* mCmdList;
		TComPtr<ID3D12DeviceRHI> mDevice;
	};
#endif

	D3D12System::~D3D12System()
	{

	}

	bool D3D12System::initialize(RHISystemInitParams const& initParam)
	{
		HRESULT hr;
		UINT dxgiFactoryFlags = 0;

		bool bDebugModeEnabled = initParam.bDebugMode && !GRHIPrefEnabled;
		//bDebugModeEnabled = false;
		bool bWarningBreakEnabled = false;

		// Enable the debug layer (requires the Graphics Tools "optional feature").
		// NOTE: Enabling the debug layer after device creation will invalidate the active device.
		if (bDebugModeEnabled)
		{
			TComPtr<ID3D12Debug3> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
				//debugController->SetEnableGPUBasedValidation(TRUE);
				//debugController->SetEnableSynchronizedCommandQueueValidation(TRUE);
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
				GRHIDeviceVendorName = FD3DUtils::GetDevicVenderName(adapterDesc.VendorId);
			}
		}

		GRHIClipZMin = 0;
		GRHIProjectionYSign = 1;
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

		if (bDebugModeEnabled && bWarningBreakEnabled)
		{
			TComPtr< ID3D12InfoQueue > infoQueue;
			mDevice->QueryInterface(IID_PPV_ARGS(&infoQueue));

			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE); 				
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		}


		D3D12DescriptorHeapPool::Get().initialize(mDevice);

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCopyCmdQueue)));
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&mCopyCmdAllocator)));
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, mCopyCmdAllocator, nullptr, IID_PPV_ARGS(&mCopyCmdList)));
		mCopyCmdList->Close();

		if (!mRenderContext.initialize(this))
		{
			return false;
		}

		mImmediateCommandList = new RHICommandListImpl(mRenderContext);

		D3D12DynamicBufferManager::Get().initialize(mDevice);

		UINT64 frequency;
		if (mRenderContext.mCommandQueue->GetTimestampFrequency(&frequency) == S_OK)
		{
			double cycleToMillisecond = 1000.0 / frequency;

#if 1
			mProfileCore = new D3D12ProfileCore;
			mProfileCore->init(mDevice, cycleToMillisecond);
			GpuProfiler::Get().setCore(mProfileCore);
#endif

		}

		return true;
	}

	void D3D12System::clearResourceReference()
	{
		mRenderContext.release();
	}

	void D3D12System::shutdown()
	{
		cleanupPipelineState();
		cleanupShaderBoundState();

		mSwapChain->release();

		D3D12DescriptorHeapPool::Get().releaseRHI();
		D3D12DynamicBufferManager::Get().cleanup();
	}

	ShaderFormat* D3D12System::createShaderFormat()
	{
		return new ShaderFormatHLSL_D3D12( mDevice );
	}

	bool D3D12System::RHIBeginRender()
	{
		if (!mRenderContext.beginFrame())
		{
			return false;
		}

		if (mProfileCore)
		{
			mProfileCore->mCmdList = mRenderContext.mGraphicsCmdList;
		}

		mbInRendering = true;
		return true;
	}

	void D3D12System::RHIEndRender(bool bPresent)
	{
		mbInRendering = false;

		mRenderContext.endFrame();

		mRenderContext.flushCommand();
		if (bPresent)
		{
			mSwapChain->present(bPresent);
		}
		mRenderContext.moveToNextFrame(mSwapChain->mResource);
	}

	RHISwapChain* D3D12System::RHICreateSwapChain(SwapChainCreationInfo const& info)
	{
		UINT dxgiFactoryFlags = 0;
		TComPtr<IDXGIFactory4> factory;
		CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));

		int sampleCount = 1;

		// Describe and create the swap chain.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = info.bufferCount;
		swapChainDesc.Width = info.extent.x;
		swapChainDesc.Height = info.extent.y;
		swapChainDesc.Format = D3D12Translate::To(info.colorForamt);
		swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		//#FIXME info.SampleCount
		swapChainDesc.SampleDesc.Count = sampleCount;

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

		if (info.bCreateDepth)
		{
			auto depthFormat = D3D12Translate::To(info.depthFormat);

			TComPtr<ID3D12Resource> textureResource;
			textureResource.initialize(createDepthTexture2D(depthFormat, info.extent.x, info.extent.y, sampleCount, info.depthCreateionFlags | TCF_RenderTarget));
			if (!textureResource.isValid())
			{
				return nullptr;
			}
			textureResource->SetName(L"SwapChainDepth");

			for (int i = 0; i < info.bufferCount; ++i)
			{
				auto& RTState = result->mRenderTargetsStates[i];

				RTState.depthBuffer.format = depthFormat;
				RTState.depthBuffer.resource = textureResource;
				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = depthFormat;
				dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
				if (sampleCount > 1)
				{
					dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
				}
				else
				{
					dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
					dsvDesc.Texture2D.MipSlice = 0;
				}
				RTState.depthBuffer.DSVHandle = D3D12DescriptorHeapPool::Alloc(RTState.depthBuffer.resource, &dsvDesc);
			}

		}


		mSwapChain = result;

		mRenderContext.configFromSwapChain(mSwapChain);
		return result;
	}

	RHITexture1D* D3D12System::RHICreateTexture1D(TextureDesc const& desc, void* data)
	{
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = desc.numMipLevel;
		textureDesc.Format = D3D12Translate::To(desc.format);
		textureDesc.Width = desc.dimension.x;
		textureDesc.Height = 1;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (desc.creationFlags & TCF_CreateUAV)
			textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		if (desc.creationFlags & TCF_RenderTarget)
			textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = desc.numSamples;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;


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
			if (!updateTexture1DSubresources(textureResource, desc.format, data, 0, desc.dimension.x, 0))
				return nullptr;
		}


		D3D12Texture1D* texture = new D3D12Texture1D(desc, textureResource);

		if (desc.creationFlags & TCF_CreateSRV)
		{
			// Describe and create a SRV for the texture.
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = textureDesc.Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;

			//#TODO
			srvDesc.Texture1D.MipLevels = 1;

			texture->mSRV = D3D12DescriptorHeapPool::Alloc(texture->mResource, &srvDesc);
		}
		if (desc.creationFlags & TCF_CreateUAV)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = textureDesc.Format;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			uavDesc.Texture1D.MipSlice = 0;
			texture->mUAV = D3D12DescriptorHeapPool::Alloc(texture->mResource, &uavDesc);
		}

		return texture;

	}

	RHITexture2D* D3D12System::RHICreateTexture2D(TextureDesc const& desc, void* data, int dataAlign)
	{
		TComPtr<ID3D12Resource> textureResource;
		bool bDepthStencilFormat = ETexture::IsDepthStencil(desc.format);
		
		DXGI_FORMAT formatD3D = D3D12Translate::To(desc.format);
		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;
		if (bDepthStencilFormat)
		{
			textureResource.initialize(createDepthTexture2D(formatD3D, desc.dimension.x, desc.dimension.y, desc.numSamples, desc.creationFlags));

			resourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		}
		else
		{
			// Describe and create a Texture2D.
			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = desc.numMipLevel;
			textureDesc.Format = formatD3D;
			textureDesc.Width = desc.dimension.x;
			textureDesc.Height = desc.dimension.y;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			D3D12_CLEAR_VALUE clearValue;

			if (desc.creationFlags & TCF_CreateUAV)
				textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			if (desc.creationFlags & TCF_RenderTarget)
			{
				clearValue.Format = formatD3D;
				clearValue.Color[0] = desc.clearColor.r;
				clearValue.Color[1] = desc.clearColor.g;
				clearValue.Color[2] = desc.clearColor.b;
				clearValue.Color[3] = desc.clearColor.a;
				textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
				resourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			}

			textureDesc.DepthOrArraySize = 1;
			textureDesc.SampleDesc.Count = desc.numSamples;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;


			VERIFY_D3D_RESULT(mDevice->CreateCommittedResource(
				&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				resourceState,
				(desc.creationFlags & TCF_RenderTarget) ? &clearValue : nullptr,
				IID_PPV_ARGS(&textureResource)), return nullptr;);

			if (data)
			{
				if (!updateTexture2DSubresources(textureResource, desc.format, data, 0, 0, desc.dimension.x, desc.dimension.y,
					desc.dimension.x * ETexture::GetFormatSize(desc.format), 0))
					return nullptr;
			}

		}

		D3D12Texture2D* texture = new D3D12Texture2D(desc, textureResource);
		texture->mCurrentStates = resourceState;

		if (desc.creationFlags & TCF_CreateSRV)
		{
			// Describe and create a SRV for the texture.
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = D3D12Translate::ToSRV(formatD3D);
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

			//#TODO
			srvDesc.Texture2D.MipLevels = 1;

			texture->mSRV = D3D12DescriptorHeapPool::Alloc(texture->mResource, &srvDesc);
		}

		if (desc.creationFlags & TCF_CreateUAV)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = formatD3D;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = 0;
			uavDesc.Texture2D.PlaneSlice = 0;
			texture->mUAV = D3D12DescriptorHeapPool::Alloc(texture->mResource, &uavDesc);
		}

		if (bDepthStencilFormat)
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
			dsvDesc.Format = formatD3D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			if (desc.numSamples > 1)
			{
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
			}
			else
			{
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Texture2D.MipSlice = 0;
			}
			texture->mRTVorDSV = D3D12DescriptorHeapPool::Alloc(texture->mResource, &dsvDesc);
		}
		if (desc.creationFlags & TCF_RenderTarget)
		{

			{
				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
				if (desc.numSamples > 1)
				{
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
					rtvDesc.Format = formatD3D;
				}
				else
				{
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
					rtvDesc.Format = formatD3D;
					rtvDesc.Texture2D.MipSlice = 0;
					rtvDesc.Texture2D.PlaneSlice = 0;
				}

				texture->mRTVorDSV = D3D12DescriptorHeapPool::Alloc(texture->mResource, &rtvDesc);
			}
		}

		return texture;
	}

	RHITexture3D* RHISystem::RHICreateTexture3D(TextureDesc const& desc, void* data)
	{
		return nullptr;
	}

	RHITextureCube* RHISystem::RHICreateTextureCube(TextureDesc const& desc, void* data[])
	{
#if 0
		TComPtr<ID3D12Resource> textureResource;

		DXGI_FORMAT formatD3D = D3D12Translate::To(desc.format);
		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

		{
			// Describe and create a Texture2D.
			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = desc.numMipLevel;
			textureDesc.Format = formatD3D;
			textureDesc.Width = desc.dimension.x;
			textureDesc.Height = desc.dimension.y;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			D3D12_CLEAR_VALUE clearValue;

			if (desc.creationFlags & TCF_CreateUAV)
				textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			if (desc.creationFlags & TCF_RenderTarget)
			{
				clearValue.Format = formatD3D;
				clearValue.Color[0] = desc.clearColor.r;
				clearValue.Color[1] = desc.clearColor.g;
				clearValue.Color[2] = desc.clearColor.b;
				clearValue.Color[3] = desc.clearColor.a;
				textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
				resourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			}

			textureDesc.DepthOrArraySize = 1;
			textureDesc.SampleDesc.Count = desc.numSamples;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;


			VERIFY_D3D_RESULT(mDevice->CreateCommittedResource(
				&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				resourceState,
				(desc.creationFlags & TCF_RenderTarget) ? &clearValue : nullptr,
				IID_PPV_ARGS(&textureResource)), return nullptr;);

			if (data)
			{
				if (!updateTexture2DSubresources(textureResource, desc.format, data, 0, 0, desc.dimension.x, desc.dimension.y,
					desc.dimension.x * ETexture::GetFormatSize(desc.format), 0))
					return nullptr;
			}

		}

		D3D12Texture2D* texture = new D3D12Texture2D(desc, textureResource);
		texture->mCurrentStates = resourceState;

		if (desc.creationFlags & TCF_CreateSRV)
		{
			// Describe and create a SRV for the texture.
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = formatD3D;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

			//#TODO
			srvDesc.Texture2D.MipLevels = 1;

			texture->mSRV = D3D12DescriptorHeapPool::Get().allocSRV(texture->mResource, &srvDesc);
		}

		if (desc.creationFlags & TCF_CreateUAV)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = formatD3D;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			texture->mUAV = D3D12DescriptorHeapPool::Get().allocUAV(texture->mResource, &uavDesc);
		}

		if (desc.creationFlags & TCF_RenderTarget)
		{
			{
				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
				if (desc.numSamples > 1)
				{
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
					rtvDesc.Format = formatD3D;
				}
				else
				{
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
					rtvDesc.Format = formatD3D;
					rtvDesc.Texture2D.MipSlice = 0;
					rtvDesc.Texture2D.PlaneSlice = 0;
				}

				texture->mRTVorDSV = D3D12DescriptorHeapPool::Get().allocRTV(texture->mResource, &rtvDesc);
			}
		}

		return texture;
#else
		return nullptr;
#endif
	}

	RHIBuffer* D3D12System::RHICreateBuffer(BufferDesc const& desc, void* data)
	{
		int bufferSize = desc.getSize();


		if (desc.creationFlags & BCF_CpuAccessWrite)
		{
			D3D12Buffer* result = new D3D12Buffer;
			if (!result->initialize(desc))
			{
				delete result;
				return nullptr;
			}

			if (data)
			{
				void* dest = RHILockBuffer(result, ELockAccess::WriteDiscard, 0, desc.getSize());
				FMemory::Copy(dest, data, desc.getSize());
				RHIUnlockBuffer(result);
			}

			return result;
		}
		else
		{

			if (desc.creationFlags & BCF_UsageConst)
			{
				bufferSize = ConstBufferMultipleSize * ((bufferSize + ConstBufferMultipleSize - 1) / ConstBufferMultipleSize);
			}

			TComPtr< ID3D12Resource > resource;
			VERIFY_D3D_RESULT(mDevice->CreateCommittedResource(
				&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&FD3D12Init::BufferDesc(bufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&resource)), return nullptr; );

			// Copy the triangle data to the vertex buffer.
			if (data)
			{
				UINT8* pDataBegin;
				D3D12_RANGE readRange = {};        // We do not intend to read from this resource on the CPU.
				VERIFY_D3D_RESULT(resource->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)), return nullptr; );
				memcpy(pDataBegin, data, bufferSize);
				resource->Unmap(0, nullptr);
			}

			D3D12Buffer* result = new D3D12Buffer;
			if (!result->initialize(desc, resource))
			{
				delete result;
				return nullptr;
			}

			if (desc.creationFlags & BCF_UsageConst)
			{
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				cbvDesc.BufferLocation = result->mResource->GetGPUVirtualAddress();
				cbvDesc.SizeInBytes = bufferSize;
				result->mCBV = D3D12DescriptorHeapPool::Alloc(resource, &cbvDesc);
			}
			else if (desc.creationFlags & BCF_CreateSRV)
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = FD3D12Init::BufferViewDesc(desc.elementSize);
				result->mSRV = D3D12DescriptorHeapPool::Alloc(resource, &srvDesc);
			}
			return result;
		}
	
		return nullptr;
	}

	void* D3D12System::RHILockBuffer(RHIBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		D3D12Buffer* bufferImpl = static_cast<D3D12Buffer*>(buffer);

		if (bufferImpl->isDyanmic())
		{
			if (access == ELockAccess::WriteDiscard || access == ELockAccess::WriteOnly)
			{
				if (bufferImpl->mDynamicAllocation.ptr)
				{
					auto buddyInfo = bufferImpl->mDynamicAllocation.buddyInfo;
					D3D12FenceResourceManager::Get().addFuncRelease(
						[buddyInfo]()
						{
							D3D12DynamicBufferManager::Get().dealloc(buddyInfo);
						}
					);
				}

				uint32 bufferSize = bufferImpl->getDesc().getSize();
				if (bufferImpl->getDesc().creationFlags & BCF_UsageConst)
				{
					bufferSize = ConstBufferMultipleSize * ((bufferSize + ConstBufferMultipleSize - 1) / ConstBufferMultipleSize);
				}

				if (D3D12DynamicBufferManager::Get().alloc(bufferSize, 0, bufferImpl->mDynamicAllocation))
				{
					return bufferImpl->mDynamicAllocation.cpuAddress + offset;
				}
				else
				{
					LogWarning(0, "D3D12DynamicBufferManager alloc fail");
				}
			}
			else if (access == ELockAccess::ReadOnly)
			{
				if (bufferImpl->mDynamicAllocation.ptr)
				{
					return bufferImpl->mDynamicAllocation.cpuAddress + offset;
				}
			}
		}
		else
		{
			if (access == ELockAccess::ReadOnly)
			{
				if (buffer->getDesc().creationFlags & BCF_CpuAccessRead)
				{
					D3D12_RANGE range = { offset , offset + size };
					void* result = nullptr;
					bufferImpl->mResource->Map(0, &range, &result);
					return result;
				}
				else
				{

				}
			}
		}

		return nullptr;
	}

	void D3D12System::RHIUnlockBuffer(RHIBuffer* buffer)
	{
		D3D12Buffer* bufferImpl = static_cast<D3D12Buffer*>(buffer);
		if (bufferImpl->isDyanmic())
		{

		}
		else
		{
			if (buffer->getDesc().creationFlags & BCF_CpuAccessRead)
			{
				D3D12_RANGE range = {};
				static_cast<D3D12Buffer*>(buffer)->mResource->Unmap(0, nullptr);
			}
		}
	}

	bool D3D12System::RHIUpdateTexture(RHITexture2D& texture, int ox, int oy, int w, int h, void* data, int level, int dataWidth)
	{
		D3D12Texture2D& textureImpl = static_cast<D3D12Texture2D&>(texture);
		if (dataWidth == 0)
		{
			dataWidth = w;
		}
		return updateTexture2DSubresources(
				textureImpl.mResource, textureImpl.mCurrentStates, texture.getFormat(), data, ox, oy, w, h, dataWidth * ETexture::GetFormatSize(texture.getFormat()), level
		);
	}

	RHIFrameBuffer* D3D12System::RHICreateFrameBuffer()
	{
		return new D3D12FrameBuffer;
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

	RHIShaderProgram* D3D12System::RHICreateShaderProgram()
	{
		return new D3D12ShaderProgram;
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


	bool D3D12System::updateTexture2DSubresources(ID3D12Resource* textureResource, ETexture::Format format, void* data, uint32 ox, uint32 oy, uint32 width, uint32 height, uint32 rowPatch, uint32 level )
	{
		auto Desc = textureResource->GetDesc();
		Desc.Width = width;
		Desc.Height = height;
		uint64 RequiredSize = 0;
		UINT64 rowSizesInBytes = 0;
		UINT numRows = 0;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
		mDevice->GetCopyableFootprints(&Desc, level, 1, 0, &layout, &numRows, &rowSizesInBytes, &RequiredSize);
		const UINT64 uploadBufferSize = (D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * RequiredSize + D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - 1) / D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;


		TComPtr<ID3D12Resource> textureCopy;

		// Create the GPU upload buffer.
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommittedResource(
			&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&FD3D12Init::BufferDesc(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&textureCopy)));

		textureCopy->SetName(L"TextureUpload");


		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = data;
		textureData.RowPitch = rowPatch;
		textureData.SlicePitch = textureData.RowPitch * height;

		mCopyCmdAllocator->Reset();
		mCopyCmdList->Reset(mCopyCmdAllocator, nullptr);

		BYTE* pData;
		textureCopy->Map(0, nullptr, reinterpret_cast<void**>(&pData));

		D3D12_MEMCPY_DEST DestData = { pData + layout.Offset, layout.Footprint.RowPitch, SIZE_T(layout.Footprint.RowPitch) * SIZE_T(numRows) };
		MemcpySubresource(&DestData, &textureData, static_cast<SIZE_T>(rowSizesInBytes), numRows);

		textureCopy->Unmap(0, nullptr);

		{
			D3D12_TEXTURE_COPY_LOCATION Dst;
			Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			Dst.pResource = textureResource;
			Dst.SubresourceIndex = level;
			D3D12_TEXTURE_COPY_LOCATION Src;
			Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			Src.pResource = textureCopy;
			Src.PlacedFootprint = layout;
			mCopyCmdList->CopyTextureRegion(&Dst, ox, oy, 0, &Src, nullptr);
		}

		mCopyResources.push_back(std::move(textureCopy));
		mCopyCmdList->Close();
		ID3D12CommandList* ppCommandLists[] = { mCopyCmdList };
		mCopyCmdQueue->ExecuteCommandLists(ARRAY_SIZE(ppCommandLists), ppCommandLists);

		waitCopyCommand();
		return true;
	}

	bool D3D12System::updateTexture2DSubresources(ID3D12Resource* textureResource, D3D12_RESOURCE_STATES states, ETexture::Format format, void* data, uint32 ox, uint32 oy, uint32 width, uint32 height, uint32 rowPatch, uint32 level /*= 0*/)
	{
		mRenderContext.waitForGpu();

		auto Desc = textureResource->GetDesc();
		Desc.Width = width;
		Desc.Height = height;
		uint64 RequiredSize = 0;
		UINT64 rowSizesInBytes = 0;
		UINT numRows = 0;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
		mDevice->GetCopyableFootprints(&Desc, level, 1, 0, &layout, &numRows, &rowSizesInBytes, &RequiredSize);
		const UINT64 uploadBufferSize = (D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * RequiredSize + D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - 1) / D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;


		TComPtr<ID3D12Resource> textureCopy;

		// Create the GPU upload buffer.
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommittedResource(
			&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&FD3D12Init::BufferDesc(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&textureCopy)));

		textureCopy->SetName(L"TextureUpload");


		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = data;
		textureData.RowPitch = rowPatch;
		textureData.SlicePitch = textureData.RowPitch * height;



		BYTE* pData;
		textureCopy->Map(0, nullptr, reinterpret_cast<void**>(&pData));

		D3D12_MEMCPY_DEST DestData = { pData + layout.Offset, layout.Footprint.RowPitch, SIZE_T(layout.Footprint.RowPitch) * SIZE_T(numRows) };
		MemcpySubresource(&DestData, &textureData, static_cast<SIZE_T>(rowSizesInBytes), numRows);

		textureCopy->Unmap(0, nullptr);

		mCopyCmdAllocator->Reset();
		mCopyCmdList->Reset(mCopyCmdAllocator, nullptr);

		mCopyCmdList->ResourceBarrier(1, &FD3D12Init::TransitionBarrier(textureResource, states, D3D12_RESOURCE_STATE_COPY_DEST));

		{
			D3D12_TEXTURE_COPY_LOCATION Dst;
			Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			Dst.pResource = textureResource;
			Dst.SubresourceIndex = level;
			D3D12_TEXTURE_COPY_LOCATION Src;
			Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			Src.pResource = textureCopy;
			Src.PlacedFootprint = layout;
			mCopyCmdList->CopyTextureRegion(&Dst, ox, oy, 0, &Src, nullptr);
		}


		mCopyCmdList->ResourceBarrier(1, &FD3D12Init::TransitionBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, states));

		mCopyResources.push_back(std::move(textureCopy));
		mCopyCmdList->Close();
		ID3D12CommandList* ppCommandLists[] = { mCopyCmdList };
		mCopyCmdQueue->ExecuteCommandLists(ARRAY_SIZE(ppCommandLists), ppCommandLists);

		waitCopyCommand();
		return true;
	}

	bool D3D12System::updateTexture1DSubresources(ID3D12Resource* textureResource, ETexture::Format format, void* data, uint32 offset, uint32 length, uint32 level /*= 0*/)
	{
		mRenderContext.waitForGpu();

		auto Desc = textureResource->GetDesc();
		Desc.Width = length;
		uint64 RequiredSize = 0;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
		mDevice->GetCopyableFootprints(&Desc, level, 1, 0, &layout, nullptr, nullptr, &RequiredSize);
		const UINT64 uploadBufferSize = (D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * RequiredSize + D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - 1) / D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

		TComPtr<ID3D12Resource> textureCopy;

		// Create the GPU upload buffer.
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommittedResource(
			&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&FD3D12Init::BufferDesc(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&textureCopy)));


		mCopyCmdAllocator->Reset();
		mCopyCmdList->Reset(mCopyCmdAllocator, nullptr);

		BYTE* pData;
		textureCopy->Map(0, nullptr, reinterpret_cast<void**>(&pData));

		::memcpy(pData, data, length * ETexture::GetFormatSize(format));

		textureCopy->Unmap(0, nullptr);

		{
			D3D12_TEXTURE_COPY_LOCATION Dst;
			Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			Dst.pResource = textureResource;
			Dst.SubresourceIndex = level;
			D3D12_TEXTURE_COPY_LOCATION Src;
			Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			Src.pResource = textureCopy;
			Src.PlacedFootprint = layout;
			mCopyCmdList->CopyTextureRegion(&Dst, offset, 0, 0, &Src, nullptr);
		}

		mCopyResources.push_back(std::move(textureCopy));
		mCopyCmdList->Close();
		ID3D12CommandList* ppCommandLists[] = { mCopyCmdList };
		mCopyCmdQueue->ExecuteCommandLists(ARRAY_SIZE(ppCommandLists), ppCommandLists);

		waitCopyCommand();
		return true;
	}

	struct ShaderStateSetup 
	{
		TArray< D3D12_STATIC_SAMPLER_DESC > rootSamplers;
		TArray< D3D12_ROOT_PARAMETER1 > rootParameters;

		std::unique_ptr< D3D12ShaderBoundState > boundState = std::make_unique< D3D12ShaderBoundState>();

		bool addShader(RHIShader* shader)
		{
			if (shader == nullptr)
				return false;

			auto shaderImpl = static_cast<D3D12Shader*>(shader);
			return addShaderData(shaderImpl->getType(), *shaderImpl);
		}
		bool addShader(RHIShader& shader)
		{
			auto& shaderImpl = static_cast<D3D12Shader&>(shader);
			return addShaderData(shaderImpl.getType(), shaderImpl);
		}

		bool addShaderData(D3D12ShaderProgram::ShaderData& shaderData)
		{
			return addShaderData(shaderData.type, shaderData);
		}

		bool addShaderData(EShader::Type shaderType, D3D12ShaderData& shaderData)
		{
			D3D12ShaderBoundState::ShaderInfo info;
			info.type = shaderType;
			info.data = &shaderData;
			info.rootSlotStart = rootParameters.size();
			boundState->mShaders.push_back(info);

			bool result = false;
			if (!shaderData.rootSignature.parameters.empty())
			{
				rootParameters.append(shaderData.rootSignature.parameters);
				result = true;
			}

			rootSamplers.append(shaderData.rootSignature.samplers);
			return result;
		}

		bool createRootSignature(TComPtr<ID3D12DeviceRHI>& device, D3D12_ROOT_SIGNATURE_FLAGS flags)
		{
			D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
			desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
			desc.Desc_1_1.NumParameters = rootParameters.size();
			desc.Desc_1_1.pParameters = rootParameters.data();
			desc.Desc_1_1.NumStaticSamplers = rootSamplers.size();
			desc.Desc_1_1.pStaticSamplers = rootSamplers.data();
			desc.Desc_1_1.Flags = flags;

			TComPtr<ID3DBlob> signature;
			TComPtr<ID3DBlob> error;
			if (FAILED(D3D12SerializeVersionedRootSignature(&desc, &signature, &error)))
			{
				LogWarning(0, (char const*)error->GetBufferPointer());
				return false;
			}

			VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&boundState->mRootSignature)));
			return true;
		}
	};

	D3D12ShaderBoundState* D3D12System::getShaderBoundState(GraphicsShaderStateDesc const& stateDesc)
	{
		ShaderBoundStateKey key;
		key.initialize(stateDesc);
		auto iter = mShaderBoundStateMap.find(key);
		if (iter != mShaderBoundStateMap.end())
		{
			return iter->second;
		}

		ShaderStateSetup stateSetup;

		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		if (!stateSetup.addShader(stateDesc.vertex))
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;

		if (!stateSetup.addShader(stateDesc.pixel))
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		if (!stateSetup.addShader(stateDesc.geometry))
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		if (!stateSetup.addShader(stateDesc.hull))
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

		if (!stateSetup.addShader(stateDesc.domain))
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;


		if (!stateSetup.createRootSignature(mDevice, flags))
			return nullptr;

		D3D12ShaderBoundState* result = stateSetup.boundState.release();
		result->cachedKey = key;
		mShaderBoundStateMap.emplace(key, result);
		return result;
	}

	D3D12ShaderBoundState* D3D12System::getShaderBoundState(MeshShaderStateDesc const& stateDesc)
	{
		ShaderBoundStateKey key;
		key.initialize(stateDesc);
		auto iter = mShaderBoundStateMap.find(key);
		if (iter != mShaderBoundStateMap.end())
		{
			return iter->second;
		}

		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		ShaderStateSetup stateSetup;

		if (!stateSetup.addShader(stateDesc.task))
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;

		if (!stateSetup.addShader(stateDesc.mesh))
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

		if (!stateSetup.addShader(stateDesc.pixel))
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		if (!stateSetup.createRootSignature(mDevice, flags))
			return nullptr;

		D3D12ShaderBoundState* result = stateSetup.boundState.release();
		result->cachedKey = key;
		mShaderBoundStateMap.emplace(key, result);
		return result;
	}

	D3D12ShaderBoundState* D3D12System::getShaderBoundState(D3D12ShaderProgram& shaderProgram)
	{
		ShaderBoundStateKey key;
		key.initialize(shaderProgram);
		auto iter = mShaderBoundStateMap.find(key);
		if (iter != mShaderBoundStateMap.end())
		{
			return iter->second;
		}

		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
		auto FindShaderTypeMask = [&shaderProgram](uint32 shaderMasks) -> uint32
		{
			for (int i = 0; i < shaderProgram.mNumShaders; ++i)
			{
				uint32 mask = BIT(shaderProgram.mShaderDatas[i].type);
				if (shaderMasks & mask)
					return mask;
			}

			return 0;
		};

		auto specialShaderTypeMask = FindShaderTypeMask(BIT(EShader::Mesh) | BIT(EShader::Compute));
		if (specialShaderTypeMask & BIT(EShader::Compute))
		{
			CHECK(shaderProgram.mNumShaders == 1);

		}
		else if (specialShaderTypeMask & BIT(EShader::Mesh))
		{
			CHECK(shaderProgram.mNumShaders <= 3);
		}
		else
		{
			flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		}

		ShaderStateSetup stateSetup;
		for (int i = 0; i < shaderProgram.mNumShaders; ++i)
		{
			auto& shaderData = shaderProgram.mShaderDatas[i];
			if (!stateSetup.addShaderData(shaderData))
			{
				switch (shaderData.type)
				{
				case EShader::Vertex:   flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS; break;
				case EShader::Pixel:    flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS; break;
				case EShader::Geometry: flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS; break;
				case EShader::Hull:     flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS; break;
				case EShader::Domain:   flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS; break;
				case EShader::Mesh:     flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS; break;
				case EShader::Task:     flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS; break;
				}
			}
		}

		if (!stateSetup.createRootSignature(mDevice, flags))
			return nullptr;

		D3D12ShaderBoundState* result = stateSetup.boundState.release();
		result->cachedKey = key;
		mShaderBoundStateMap.emplace(key, result);
		return result;
	}

	D3D12ShaderBoundState* D3D12System::getShaderBoundState(RHIShader& computeShader)
	{
		ShaderBoundStateKey key;
		key.initialize(computeShader);
		auto iter = mShaderBoundStateMap.find(key);
		if (iter != mShaderBoundStateMap.end())
		{
			return iter->second;
		}

		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
		ShaderStateSetup stateSetup;
		if (!stateSetup.addShader(computeShader))
		{

		}

		if (!stateSetup.createRootSignature(mDevice, flags))
			return nullptr;

		D3D12ShaderBoundState* result = stateSetup.boundState.release();
		result->cachedKey = key;
		mShaderBoundStateMap.emplace(key, result);
		return result;
	}


	struct FPipelineStream
	{
		template< typename TRenderStateDesc >
		static void Add(D3D12PipelineStateStream& stateStream, TRenderStateDesc const& renderState)
		{
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


			if (renderState.rasterizerState)
			{
				stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER>(
					static_cast<D3D12RasterizerState*>(renderState.rasterizerState)->mDesc);
			}
			if (renderState.depthStencilState)
			{
				stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_RHI>(
					static_cast<D3D12DepthStencilState*>(renderState.depthStencilState)->mDesc);
			}
			if (renderState.blendState)
			{
				stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND>(
					static_cast<D3D12BlendState*>(renderState.blendState)->mDesc);
			}
		}

		static void Add(D3D12PipelineStateStream& stateStream, D3D12RenderTargetsState* renderTargetsState)
		{
			if (renderTargetsState)
			{
				auto& RTFormatArray = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS>();

				RTFormatArray.NumRenderTargets = renderTargetsState->numColorBuffers;
				for (int i = 0; i < renderTargetsState->numColorBuffers; ++i)
				{
					RTFormatArray.RTFormats[i] = renderTargetsState->colorBuffers[i].format;
				}

				if (renderTargetsState->depthBuffer.DSVHandle.isValid())
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT>();
					data = renderTargetsState->depthBuffer.format;
				}
			}
		}
	};

	D3D12PipelineState* D3D12System::getPipelineState(GraphicsRenderStateDesc const& renderState, D3D12ShaderBoundState* boundState, D3D12RenderTargetsState* renderTargetsState)
	{
		D3D12PipelineStateKey key;
		key.initialize(renderState, boundState, renderTargetsState);
		auto iter = mPipelineStateMap.find(key);
		if (iter != mPipelineStateMap.end())
		{
			return iter->second;
		}

		D3D12PipelineStateStream stateStream;

		if (renderState.inputLayout)
		{
			stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT>(
				static_cast<D3D12InputLayout*>(renderState.inputLayout)->getDesc());
		}


		for (auto& shaderInfo : boundState->mShaders)
		{
			switch (shaderInfo.type)
			{
			case EShader::Vertex:
			{
				stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS>(shaderInfo.data->getByteCode());
			}
			break;
			case EShader::Pixel:
			{
				stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS>(shaderInfo.data->getByteCode());
			}
			break;
			case EShader::Geometry:
			{
				stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS>(shaderInfo.data->getByteCode());
			}
			break;
			case EShader::Hull:
			{
				stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS>(shaderInfo.data->getByteCode());
			}
			break;
			case EShader::Domain:
			{
				stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS>(shaderInfo.data->getByteCode());
			}
			break;
			}
		}

		FPipelineStream::Add(stateStream, renderState);
		FPipelineStream::Add(stateStream, renderTargetsState);

		struct FixedPipelineStateStream
		{
			TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE > pRootSignature;
			TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY > PrimitiveTopologyType;
		};

		auto& streamDesc = stateStream.addDataT< FixedPipelineStateStream >();
		streamDesc.pRootSignature = boundState->mRootSignature;
		streamDesc.PrimitiveTopologyType = D3D12Translate::ToTopologyType(renderState.primitiveType);


		TComPtr<ID3D12PipelineState> resource;
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreatePipelineState(&stateStream.getDesc(), IID_PPV_ARGS(&resource)));

		auto result = new D3D12PipelineState;
		result->mResource = resource.detach();
		mPipelineStateMap.emplace(key, result);
		return result;

	}

	D3D12PipelineState* D3D12System::getPipelineState(MeshRenderStateDesc const& renderState, D3D12ShaderBoundState* boundState, D3D12RenderTargetsState* renderTargetsState)
	{
		D3D12PipelineStateKey key;
		key.initialize(renderState, boundState, renderTargetsState);
		auto iter = mPipelineStateMap.find(key);
		if (iter != mPipelineStateMap.end())
		{
			return iter->second;
		}

		D3D12PipelineStateStream stateStream;

		for (auto& shaderInfo : boundState->mShaders)
		{
			switch (shaderInfo.type)
			{
			case EShader::Mesh:
				{
					stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS>(shaderInfo.data->getByteCode());
				}
				break;
			case EShader::Task:
				{
					stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS>(shaderInfo.data->getByteCode());
				}
				break;
			default:
				{

				}
			}
		}

		FPipelineStream::Add(stateStream, renderState);
		FPipelineStream::Add(stateStream, renderTargetsState);

		struct FixedPipelineStateStream
		{
			TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE > pRootSignature;
		};

		auto& streamDesc = stateStream.addDataT< FixedPipelineStateStream >();
		streamDesc.pRootSignature = boundState->mRootSignature;

		TComPtr<ID3D12PipelineState> resource;
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreatePipelineState(&stateStream.getDesc(), IID_PPV_ARGS(&resource)));

		auto result = new D3D12PipelineState;
		result->mResource = resource.detach();
		mPipelineStateMap.emplace(key, result);
		return result;
	}

	D3D12PipelineState* D3D12System::getPipelineState(D3D12ShaderBoundState* boundState)
	{
		D3D12PipelineStateKey key;
		key.initialize(boundState);
		auto iter = mPipelineStateMap.find(key);
		if (iter != mPipelineStateMap.end())
		{
			return iter->second;
		}

		D3D12PipelineStateStream stateStream;

		for (auto& shaderInfo : boundState->mShaders)
		{
			switch (shaderInfo.type)
			{
			case EShader::Compute:
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS>();
					data = shaderInfo.data->getByteCode();
				}
				break;
			default:
				{

				}
			}
		}

		struct FixedPipelineStateStream
		{
			TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE > pRootSignature;
		};

		auto& streamDesc = stateStream.addDataT< FixedPipelineStateStream >();

		streamDesc.pRootSignature = boundState->mRootSignature;

		TComPtr<ID3D12PipelineState> resource;
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreatePipelineState(&stateStream.getDesc(), IID_PPV_ARGS(&resource)));

		auto result = new D3D12PipelineState;
		result->mResource = resource.detach();
		mPipelineStateMap.emplace(key, result);
		return result;
	}

	void D3D12System::notifyFlushCommand()
	{
		if (mbInRendering)
		{
			mRenderContext.resetCommandList();
		}
	}

	void D3D12System::handleErrorResult(HRESULT errorResult)
	{

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

		VERIFY_RETURN_FALSE( mVertexBufferUP.initialize() );
		VERIFY_RETURN_FALSE( mIndexBufferUP.initialize() );


		{
			D3D12_INDIRECT_ARGUMENT_DESC args[1];
			args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
			D3D12_COMMAND_SIGNATURE_DESC desc;
			desc.ByteStride = 36;
			desc.NumArgumentDescs = ARRAY_SIZE(args);
			desc.pArgumentDescs = args;
			mDevice->CreateCommandSignature(&desc, NULL, IID_PPV_ARGS(&mDrawCmdSignature));
		}
		{
			D3D12_INDIRECT_ARGUMENT_DESC args[1];
			args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
			D3D12_COMMAND_SIGNATURE_DESC desc;
			desc.ByteStride = 36;
			desc.NumArgumentDescs = ARRAY_SIZE(args);
			desc.pArgumentDescs   = args;
			mDevice->CreateCommandSignature(&desc, NULL, IID_PPV_ARGS(&mDrawIndexedCmdSignature));
		}

		mFrameIndex = 0;
		mFrameDataList.resize(1);
		auto& frameData = mFrameDataList[mFrameIndex];
		frameData.fenceValue = 0;

		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateFence(frameData.fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
		VERIFY_RETURN_FALSE(frameData.init(mDevice));
		frameData.graphicsCmdList->Close();

		mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (mFenceEvent == nullptr)
		{
			VERIFY_D3D_RESULT_RETURN_FALSE(HRESULT_FROM_WIN32(GetLastError()));
		}

		mGraphicsCmdList = frameData.graphicsCmdList;
		frameData.fenceValue += 1;
		return true;
	}


	bool D3D12Context::configFromSwapChain(D3D12SwapChain* swapChain)
	{
		int numFrames = swapChain->mRenderTargetsStates.size();

		mFrameDataList.resize(numFrames);
		mFrameIndex = swapChain->mResource->GetCurrentBackBufferIndex();
		for (int i = 1; i < numFrames; ++i)
		{
			VERIFY_RETURN_FALSE(mFrameDataList[i].init(mDevice));
			//if (i != mFrameIndex)
			{
				mFrameDataList[i].graphicsCmdList->Close();
			}
		}
		mFrameDataList[mFrameIndex].fenceValue = mFrameDataList[0].fenceValue + 1;
		return true;
	}


	void D3D12Context::release()
	{
		if (!mDevice.isValid())
			return;

		waitForGpu();

		D3D12FenceResourceManager::Get().cleanup(true);
		mCommandAllocator.reset();
		mCommandQueue.reset();
		mFence.reset();
		CloseHandle(mFenceEvent);

		mDevice.reset();
	}

	void D3D12Context::RHISetRasterizerState(RHIRasterizerState& rasterizerState)
	{
		if (mRasterizerStatePending != &rasterizerState)
		{
			mRasterizerStatePending = &rasterizerState;
			if (static_cast<D3D12RasterizerState&>(rasterizerState).bEnableScissor)
			{
				mGraphicsCmdList->RSSetScissorRects(mNumViewports, mScissorRects);
			}
			else
			{
				mGraphicsCmdList->RSSetScissorRects(mNumViewports, mViewportRects);
			}
		}
	}

	void D3D12Context::RHISetBlendState(RHIBlendState& blendState)
	{
		if (mBlendStateStatePending != &blendState)
		{
			mBlendStateStatePending = &blendState;
		}
	}

	void D3D12Context::RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef)
	{
		if (mDepthStencilStatePending != &depthStencilState)
		{
			mDepthStencilStatePending = &depthStencilState;
		}
		mGraphicsCmdList->OMSetStencilRef(stencilRef);
	}

	void D3D12Context::RHISetViewport(float x, float y, float w, float h, float zNear, float zFar)
	{
		mGraphicsCmdList->RSSetViewports(1, &FD3D12Init::Viewport(x, y, w, h, zNear, zFar));
		D3D12_RECT& scissorRect = mViewportRects[0];
		scissorRect.left = x;
		scissorRect.top = y;
		scissorRect.right = x + w;
		scissorRect.bottom = y + h;
		mGraphicsCmdList->RSSetScissorRects(1, mViewportRects);

		mScissorRects[0] = mViewportRects[0];
		mNumViewports = 1;
	}

	void D3D12Context::RHISetViewports(ViewportInfo const viewports[], int numViewports)
	{
		D3D12_VIEWPORT viewportsD3D[MaxViewportCount];
		D3D12_RECT scissorRects[MaxViewportCount];
		CHECK(numViewports < ARRAY_SIZE(viewportsD3D));
		for (int i = 0; i < numViewports; ++i)
		{
			ViewportInfo const& vp = viewports[i];
			viewportsD3D[i] = FD3D12Init::Viewport(vp.x, vp.y, vp.w, vp.h, vp.zNear, vp.zFar);;

			D3D12_RECT& scissorRect = mViewportRects[i];
			scissorRect.left = vp.x;
			scissorRect.top = vp.y;
			scissorRect.right = vp.x + vp.w;
			scissorRect.bottom = vp.y + vp.h;

			mScissorRects[i] = mViewportRects[i];
		}
		mGraphicsCmdList->RSSetViewports(numViewports, viewportsD3D);
		mGraphicsCmdList->RSSetScissorRects(numViewports, mViewportRects);
		mNumViewports = numViewports;
	}

	void D3D12Context::RHISetScissorRect(int x, int y, int w, int h)
	{
		for( int i = 0 ; i < mNumViewports; ++i)
		{
			D3D12_RECT& scissorRect = mScissorRects[i];
			scissorRect.left = x;
			scissorRect.top = mViewportRects[i].bottom - ( y + h );
			scissorRect.right = x + w;
			scissorRect.bottom = mViewportRects[i].bottom - y;
		}
		mGraphicsCmdList->RSSetScissorRects(mNumViewports, mScissorRects);
	}

	template< typename TBuffer >
	bool DetermitPrimitiveTopologyUP(EPrimitive primitive, int num, uint32 const* pIndices, TBuffer& buffer, EPrimitive& outPrimitiveDetermited, int& outIndexNum)
	{
		switch (primitive)
		{
		case EPrimitive::Quad:
			{
				int numQuad = num / 4;
				int indexBufferSize = sizeof(uint32) * numQuad * 6;
				void* pIndexBufferData = buffer.lock(indexBufferSize);
				if (pIndexBufferData == nullptr)
					return false;

				uint32* pData = (uint32*)pIndexBufferData;
				if (pIndices)
				{
					for (int i = 0; i < numQuad; ++i)
					{
						pData[0] = pIndices[0];
						pData[1] = pIndices[1];
						pData[2] = pIndices[2];
						pData[3] = pIndices[0];
						pData[4] = pIndices[2];
						pData[5] = pIndices[3];
						pData += 6;
						pIndices += 4;
					}
				}
				else
				{
					for (int i = 0; i < numQuad; ++i)
					{
						int index = 4 * i;
						pData[0] = index + 0;
						pData[1] = index + 1;
						pData[2] = index + 2;
						pData[3] = index + 0;
						pData[4] = index + 2;
						pData[5] = index + 3;
						pData += 6;
					}
				}
				outPrimitiveDetermited = EPrimitive::TriangleList;
				buffer.unlock();
				outIndexNum = numQuad * 6;
				return true;
			}
			break;
		case EPrimitive::LineLoop:
			{
				int indexBufferSize = sizeof(uint32) * (num + 1);
				void* pIndexBufferData = buffer.lock(indexBufferSize);
				if (pIndexBufferData == nullptr)
					return false;
				uint32* pData = (uint32*)pIndexBufferData;

				if (pIndices)
				{
					for (int i = 0; i < num; ++i)
					{
						pData[i] = pIndices[i];
					}
					pData[num] = pIndices[0];
				}
				else
				{
					for (int i = 0; i < num; ++i)
					{
						pData[i] = i;
					}
					pData[num] = 0;
				}

				buffer.unlock();
				outIndexNum = num + 1;
				outPrimitiveDetermited = EPrimitive::LineStrip;
				return true;
			}
			break;
		case EPrimitive::Polygon:
			{
				if (num <= 2)
					return false;

				int numTriangle = (num - 2);

				int indexBufferSize = sizeof(uint32) * numTriangle * 3;
				void* pIndexBufferData = buffer.lock(indexBufferSize);
				if (pIndexBufferData == nullptr)
					return false;

				uint32* pData = (uint32*)pIndexBufferData;
				if (pIndices)
				{
					for (int i = 0; i < numTriangle; ++i)
					{
						pData[0] = pIndices[0];
						pData[1] = pIndices[i + 1];
						pData[2] = pIndices[i + 2];
						pData += 3;
					}
				}
				else
				{
					for (int i = 0; i < numTriangle; ++i)
					{
						pData[0] = 0;
						pData[1] = i + 1;
						pData[2] = i + 2;
						pData += 3;
					}
				}
				outPrimitiveDetermited = EPrimitive::TriangleList;
				buffer.unlock();
				outIndexNum = numTriangle * 3;
				return true;
			}
			break;
		}

		if (D3D12Translate::To(primitive) == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
			return false;

		if (pIndices)
		{
			uint32 indexBufferSize = num * sizeof(uint32);
			void* pIndexBufferData = buffer.lock(indexBufferSize);
			if (pIndexBufferData == nullptr)
				return false;

			FMemory::Copy(pIndexBufferData, pIndices, indexBufferSize);
			buffer.unlock();
			outIndexNum = num;
		}

		outPrimitiveDetermited = primitive;
		return true;
	}

	bool D3D12Context::determitPrimitiveTopologyUP(EPrimitive primitive, int num, uint32 const* pIndices, EPrimitive& outPrimitiveDetermited, D3D12_INDEX_BUFFER_VIEW& outIndexBufferView, int& outIndexNum)
	{
		struct MyBuffer
		{
			MyBuffer(D3D12DynamicBuffer& buffer, D3D12_INDEX_BUFFER_VIEW& bufferView)
				:mBuffer(buffer),mBufferView(bufferView)
			{
			}
			void* lock(uint32 size)
			{
				return mBuffer.lock(size);
			}
			void unlock()
			{
				mBuffer.unlock(mBufferView);
			}
			D3D12DynamicBuffer& mBuffer;
			D3D12_INDEX_BUFFER_VIEW& mBufferView;
		};

		return DetermitPrimitiveTopologyUP(primitive, num, pIndices,  MyBuffer(mIndexBufferUP, outIndexBufferView), outPrimitiveDetermited, outIndexNum);
	}

	void D3D12Context::RHIDrawPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride)
	{
		auto& bufferImpl = *static_cast<D3D12Buffer*>(commandBuffer);
		commitGraphicsPipelineState(type);
		mGraphicsCmdList->ExecuteIndirect(mDrawCmdSignature, numCommand, bufferImpl.getResource(), offset, nullptr , 0);
		postDrawPrimitive();
	}

	void D3D12Context::RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride)
	{
		auto& bufferImpl = *static_cast<D3D12Buffer*>(commandBuffer);
		commitGraphicsPipelineState(type);
		mGraphicsCmdList->ExecuteIndirect(mDrawIndexedCmdSignature, numCommand, bufferImpl.getResource(), offset, nullptr, 0);
		postDrawPrimitive();
	}

	void D3D12Context::RHIDrawPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numVertexData)
	{

		assert(numVertexData <= MAX_INPUT_STREAM_NUM);
		EPrimitive determitedPrimitive;
		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		int numDrawIndex;
		if (!determitPrimitiveTopologyUP(type, numVertices, nullptr, determitedPrimitive, indexBufferView, numDrawIndex))
			return;

		uint32 vertexBufferSize = 0;
		if (type == EPrimitive::LineLoop)
		{
			++numVertices;
			for (int i = 0; i < numVertexData; ++i)
			{
				auto const& info = dataInfos[i];
				vertexBufferSize += (D3D12_BUFFER_SIZE_ALIGN * (info.size + info.stride ) + D3D12_BUFFER_SIZE_ALIGN - 1) / D3D12_BUFFER_SIZE_ALIGN;
			}
		}
		else
		{
			for (int i = 0; i < numVertexData; ++i)
			{
				auto const& info = dataInfos[i];
				vertexBufferSize += (D3D12_BUFFER_SIZE_ALIGN * info.size + D3D12_BUFFER_SIZE_ALIGN - 1) / D3D12_BUFFER_SIZE_ALIGN;
			}
		}

		uint8* pVBufferData = (uint8*)mVertexBufferUP.lock(vertexBufferSize);
		if (pVBufferData)
		{
			commitGraphicsPipelineState(determitedPrimitive);

			uint32 dataOffset = 0;
			UINT offsets[MAX_INPUT_STREAM_NUM];

			for (int i = 0; i < numVertexData; ++i)
			{
				auto const& info = dataInfos[i];
				offsets[i] = dataOffset;
				if (type == EPrimitive::LineLoop)
				{
					FMemory::Copy(pVBufferData + dataOffset, info.ptr, info.size);
					FMemory::Copy(pVBufferData + dataOffset + info.size, info.ptr, info.stride);
					dataOffset += (D3D12_BUFFER_SIZE_ALIGN * (info.size + info.stride) + D3D12_BUFFER_SIZE_ALIGN - 1) / D3D12_BUFFER_SIZE_ALIGN;
				}
				else
				{
					FMemory::Copy(pVBufferData + dataOffset, info.ptr, info.size);
					dataOffset += (D3D12_BUFFER_SIZE_ALIGN * info.size + D3D12_BUFFER_SIZE_ALIGN - 1) / D3D12_BUFFER_SIZE_ALIGN;
				}
			}

			D3D12_VERTEX_BUFFER_VIEW bufferView;
			mVertexBufferUP.unlock(bufferView);
			D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[MAX_INPUT_STREAM_NUM];
			for (int i = 0; i < numVertexData; ++i)
			{
				auto const& info = dataInfos[i];
				vertexBufferViews[i].BufferLocation = bufferView.BufferLocation + offsets[i];
				vertexBufferViews[i].StrideInBytes = info.stride;
				vertexBufferViews[i].SizeInBytes   = info.size;
				if (type == EPrimitive::LineLoop)
				{
					vertexBufferViews[i].SizeInBytes += info.stride;
				}
			}

			mGraphicsCmdList->IASetVertexBuffers(0, numVertexData, vertexBufferViews);
			if (indexBufferView.SizeInBytes)
			{
				mGraphicsCmdList->IASetIndexBuffer(&indexBufferView);
				mGraphicsCmdList->DrawIndexedInstanced(numDrawIndex, 1, 0, 0, 0);
			}
			else
			{
				mGraphicsCmdList->DrawInstanced(numVertices, 1, 0, 0);
			}

			postDrawPrimitive();
		}
	}

	void D3D12Context::RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numVertexData, uint32 const* pIndices, int numIndex)
	{
		assert(numVertexData <= MAX_INPUT_STREAM_NUM);
		EPrimitive determitedPrimitive;
		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		int numDrawIndex;
		if (!determitPrimitiveTopologyUP(type, numIndex, pIndices, determitedPrimitive, indexBufferView, numDrawIndex))
			return;

		uint32 vertexBufferSize = 0;
		for (int i = 0; i < numVertexData; ++i)
		{
			vertexBufferSize += (D3D12_BUFFER_SIZE_ALIGN * dataInfos[i].size + D3D12_BUFFER_SIZE_ALIGN - 1) / D3D12_BUFFER_SIZE_ALIGN;
		}

		uint8* pVBufferData = (uint8*)mVertexBufferUP.lock(vertexBufferSize);
		if (pVBufferData)
		{
			commitGraphicsPipelineState(determitedPrimitive);

			uint32 dataOffset = 0;
			UINT offsets[MAX_INPUT_STREAM_NUM];
			for (int i = 0; i < numVertexData; ++i)
			{
				auto const& info = dataInfos[i];
				offsets[i] = dataOffset;
				FMemory::Copy(pVBufferData + dataOffset, info.ptr, info.size);
				dataOffset += (D3D12_BUFFER_SIZE_ALIGN * info.size + D3D12_BUFFER_SIZE_ALIGN - 1) / D3D12_BUFFER_SIZE_ALIGN;
			}

			D3D12_VERTEX_BUFFER_VIEW bufferView;
			mVertexBufferUP.unlock(bufferView);
			D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[MAX_INPUT_STREAM_NUM];
			for (int i = 0; i < numVertexData; ++i)
			{
				vertexBufferViews[i].BufferLocation = bufferView.BufferLocation + offsets[i];
				vertexBufferViews[i].StrideInBytes = dataInfos[i].stride;
				vertexBufferViews[i].SizeInBytes = dataInfos[i].size;
			}

			mGraphicsCmdList->IASetVertexBuffers(0, numVertexData, vertexBufferViews);
			mGraphicsCmdList->IASetIndexBuffer(&indexBufferView);
			mGraphicsCmdList->DrawIndexedInstanced(numDrawIndex, 1, 0, 0, 0);


			postDrawPrimitive();
		}
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
			if (frameBufferImpl->bStateDirty)
			{
				frameBufferImpl->mRenderTargetsState.updateState();
				frameBufferImpl->bStateDirty = false;
			}
		}


		CHECK(newState);
		if (bForceReset || mRenderTargetsState != newState)
		{
			mRenderTargetsState = newState;
			commitRenderTargetState();
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
			auto const& DSVHandle = mRenderTargetsState->depthBuffer.DSVHandle;

			if (DSVHandle.isValid())
			{
				D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL;
				if (!HaveBits(clearBits, EClearBits::Depth))
					flags &= ~D3D12_CLEAR_FLAG_DEPTH;
				else if (!HaveBits(clearBits, EClearBits::Stencil))
					flags &= ~D3D12_CLEAR_FLAG_STENCIL;

				mGraphicsCmdList->ClearDepthStencilView(DSVHandle.getCPUHandle(), flags, depth, stenceil, 0, nullptr);
			}
		}
	}

	void D3D12Context::RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream)
	{
		mInputLayoutPending = inputLayout;
		std::copy(inputStreams, inputStreams + numInputStream, mInputStreams);
		mNumInputStream = numInputStream;
		assert(mNumInputStream < MaxInputStream);
		mbInpuStreamDirty = true;
	}

	void D3D12Context::RHISetIndexBuffer(RHIBuffer* indexBuffer)
	{
		mIndexBufferState = indexBuffer;
		bIndexBufferStateDirty = true;
	}


	D3D12_RESOURCE_STATES GetResourceStates(EResourceTransition transition)
	{
		switch (transition)
		{
		case EResourceTransition::UAV:
			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
#if 0
		case EResourceTransition::SRV:
			return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
#endif
		case EResourceTransition::RenderTarget:
			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		}
		NEVER_REACH("GetResourceStates");
		return D3D12_RESOURCE_STATE_COMMON;
	}
	void D3D12Context::RHIResourceTransition(TArrayView<RHIResource*> resources, EResourceTransition transition)
	{
		D3D12_RESOURCE_BARRIER barriers[16];

		CHECK(resources.size() < ARRAY_SIZE(barriers));

		int numBarrier = 0;
		for (auto resource : resources)
		{
			if ( resource == nullptr )
				continue;

			switch (resource->getResourceType())
			{
			case EResourceType::Buffer:
				break;
			case EResourceType::Texture:
				{
					switch (static_cast<RHITextureBase*>(resource)->getType())
					{
					case ETexture::Type1D:
						{
							D3D12Texture1D* textureImpl = static_cast<D3D12Texture1D*>(resource);
							auto toState = GetResourceStates(transition);
							if (textureImpl->mCurrentStates != toState)
							{
								barriers[numBarrier] = FD3D12Init::TransitionBarrier(textureImpl->getResource(), textureImpl->mCurrentStates, toState,
									D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE);
								textureImpl->mCurrentStates = toState;
								++numBarrier;
							}
						}
						break;
					case ETexture::Type2D:
						{
							D3D12Texture2D* textureImpl = static_cast<D3D12Texture2D*>(resource);
							auto toState = GetResourceStates(transition);
							if (textureImpl->mCurrentStates != toState)
							{
								barriers[numBarrier] = FD3D12Init::TransitionBarrier(textureImpl->getResource(), textureImpl->mCurrentStates, toState,
									D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE);
								textureImpl->mCurrentStates = toState;
								++numBarrier;
							}
						}
						break;
					case ETexture::Type3D:
						break;
					case ETexture::TypeCube:
						break;
					}
				}
				break;
			}

		}

		if (numBarrier)
		{
			mGraphicsCmdList->ResourceBarrier(numBarrier, barriers);
		}
	}

	ID3D12Resource* GetTextureResource(RHITextureBase& texture)
	{
		switch (texture.getType())
		{
		case ETexture::Type1D:
			return static_cast<D3D12Texture1D&>(texture).getResource();
		case ETexture::Type2D:
			return static_cast<D3D12Texture2D&>(texture).getResource();
		case ETexture::Type3D:
			return static_cast<D3D12Texture3D&>(texture).getResource();
		case ETexture::TypeCube:
			return static_cast<D3D12TextureCube&>(texture).getResource();
#if 0
		case ETexture::Type2DArray:
			return static_cast<D3D12Texture2DArray&>(texture).getResource();
#endif
		}

		NEVER_REACH("GetTextureResource");
		return nullptr;
	}

	void D3D12Context::RHIResolveTexture(RHITextureBase& destTexture, int destSubIndex, RHITextureBase& srcTexture, int srcSubIndex)
	{
		ID3D12Resource* destResource = GetTextureResource(destTexture);
		ID3D12Resource* srcResource = GetTextureResource(srcTexture);
		mGraphicsCmdList->ResolveSubresource(destResource, destSubIndex, srcResource, srcSubIndex, D3D12Translate::To(destTexture.getFormat()));
	}

	void D3D12Context::RHIFlushCommand()
	{
#if 1
		flushCommand();
		static_cast<D3D12System*>(GRHISystem)->notifyFlushCommand();
#endif
	}

	void D3D12Context::RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ)
	{
		commitComputePipelineState();
		mGraphicsCmdList->Dispatch(numGroupX, numGroupY, numGroupZ);
		postDispatchCompute();
	}

	void D3D12Context::RHISetGraphicsShaderBoundState(GraphicsShaderStateDesc const& stateDesc)
	{
		auto newState = static_cast<D3D12System*>(GRHISystem)->getShaderBoundState(stateDesc);
		setGfxShaderBoundState(newState);
	}

	void D3D12Context::RHISetMeshShaderBoundState(MeshShaderStateDesc const& stateDesc)
	{
		auto newState = static_cast<D3D12System*>(GRHISystem)->getShaderBoundState(stateDesc);
		setGfxShaderBoundState(newState);
	}

	void D3D12Context::RHISetComputeShader(RHIShader* shader)
	{
		if (shader)
		{
			auto newState = static_cast<D3D12System*>(GRHISystem)->getShaderBoundState(*shader);
			setComputeShaderBoundState(newState);
		}
		else
		{
			setComputeShaderBoundState(nullptr);
		}
	}

	void D3D12Context::RHISetShaderProgram(RHIShaderProgram* shaderProgram)
	{
		if (shaderProgram == nullptr)
		{
			mGfxBoundState = nullptr;
			return;
		}

		auto shaderProgramImpl = static_cast<D3D12ShaderProgram*>(shaderProgram);

		if (shaderProgramImpl->cachedState == nullptr)
		{
			shaderProgramImpl->cachedState = static_cast<D3D12System*>(GRHISystem)->getShaderBoundState(*shaderProgramImpl);
		}

		if (shaderProgramImpl->cachedState->mShaders[0].type == EShader::Compute)
		{
			setComputeShaderBoundState(shaderProgramImpl->cachedState);
		}
		else
		{
			setGfxShaderBoundState(shaderProgramImpl->cachedState);
		}
	}

	void D3D12Context::setGfxShaderBoundState(D3D12ShaderBoundState* newState)
	{
		mGfxBoundState = newState;
		if (newState)
		{
			mGraphicsCmdList->SetGraphicsRootSignature(mGfxBoundState->mRootSignature);
			mNumUsedHeaps = 0;
			mUsedDescHeaps[0] = mUsedDescHeaps[1] = nullptr;

			for (auto& shaderInfo : mGfxBoundState->mShaders)
			{
				mRootSlotStart[shaderInfo.type] = shaderInfo.rootSlotStart;
			}
		}
	}

	void D3D12Context::setComputeShaderBoundState(D3D12ShaderBoundState* newState)
	{
		mComputeBoundState = newState;
		if (newState)
		{
			mGraphicsCmdList->SetComputeRootSignature(mComputeBoundState->mRootSignature);
			mNumUsedHeaps = 0;
			mUsedDescHeaps[0] = mUsedDescHeaps[1] = nullptr;

			auto& shaderInfo = mComputeBoundState->mShaders[0];
			mRootSlotStart[shaderInfo.type] = shaderInfo.rootSlotStart;
		}
	}

	void D3D12Context::setShaderValueInternal(EShader::Type shaderType , D3D12ShaderData& shaderData, ShaderParameter const& param, uint8 const* pData, uint32 size)
	{
		if (shaderData.rootSignature.globalCBSize <= 0)
		{
			return;
		}
		auto const& slotInfo = shaderData.rootSignature.slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shaderType] + slotInfo.slotOffset;

		auto& resourceBoundState = mResourceBoundStates[shaderType];
		if ( resourceBoundState.updateConstBuffer(shaderData) )
		{
			if (shaderType == EShader::Compute)
			{
				mGraphicsCmdList->SetComputeRootConstantBufferView(rootSlot, resourceBoundState.getConstantGPUAddress());
			}
			else
			{
				mGraphicsCmdList->SetGraphicsRootConstantBufferView(rootSlot, resourceBoundState.getConstantGPUAddress());
			}
		}
		resourceBoundState.updateConstantData(pData, slotInfo.dataOffset, slotInfo.dataSize);
	}

	void D3D12Context::setShaderValueInternal(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, uint8 const* pData, uint32 size, uint32 elementSize, uint32 stride)
	{
		if (shaderData.rootSignature.globalCBSize <= 0)
		{
			return;
		}
		auto const& slotInfo = shaderData.rootSignature.slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shaderType] + slotInfo.slotOffset;

		auto& resourceBoundState = mResourceBoundStates[shaderType];	
		if (resourceBoundState.updateConstBuffer(shaderData))
		{
			if (shaderType == EShader::Compute)
			{
				mGraphicsCmdList->SetComputeRootConstantBufferView(rootSlot, resourceBoundState.getConstantGPUAddress());
			}
			else
			{

				mGraphicsCmdList->SetGraphicsRootConstantBufferView(rootSlot, resourceBoundState.getConstantGPUAddress());
			}
		}
		

		uint32 offset = slotInfo.dataOffset;
		while (size > elementSize)
		{
			resourceBoundState.updateConstantData(pData, offset, elementSize);
			offset += stride;
			size -= elementSize;
		}
		if (size)
		{
			resourceBoundState.updateConstantData(pData, offset, size);
		}
	}

	void D3D12Context::setShaderTexture(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHITextureBase& texture)
	{
		auto const& handle = static_cast<D3D12Texture2D&>(texture).mSRV;
		updateCSUHeapUsage(handle);

		auto const& slotInfo = shaderData.rootSignature.slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shaderType] + slotInfo.slotOffset;
		if (shaderType == EShader::Compute)
		{
			mGraphicsCmdList->SetComputeRootDescriptorTable(rootSlot, handle.getGPUHandle());
		}
		else
		{
			mGraphicsCmdList->SetGraphicsRootDescriptorTable(rootSlot, handle.getGPUHandle());
		}
	}

	void D3D12Context::setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture)
	{
		auto& shaderImpl = static_cast<D3D12Shader&>(shader);
		setShaderTexture(shaderImpl.getType(), shaderImpl, param, texture);
	}

	void D3D12Context::setShaderSampler(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHISamplerState& sampler)
	{
		auto const& handle = static_cast<D3D12SamplerState&>(sampler).mHandle;
		updateSamplerHeapUsage(handle);

		auto const& slotInfo = shaderData.rootSignature.slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shaderType] + slotInfo.slotOffset;
		if (shaderType == EShader::Compute)
		{
			mGraphicsCmdList->SetComputeRootDescriptorTable(rootSlot, handle.getGPUHandle());
		}
		else
		{
			mGraphicsCmdList->SetGraphicsRootDescriptorTable(rootSlot, handle.getGPUHandle());
		}
	}


	void D3D12Context::setShaderSampler(RHIShader& shader, ShaderParameter const& param, RHISamplerState& sampler)
	{
		auto& shaderImpl = static_cast<D3D12Shader&>(shader);
		setShaderSampler(shaderImpl.getType(), shaderImpl, param, sampler);
	}

	void D3D12Context::setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler)
	{
		auto& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &sampler](EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& shaderParam)
		{
			setShaderSampler(shaderType, shaderData, shaderParam, sampler);
		});
	}

	void D3D12Context::setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op)
	{
		auto& shaderImpl = static_cast<D3D12Shader&>(shader);
		setShaderRWTexture(shaderImpl.getType(), shaderImpl, param, texture, op);
	}

	void D3D12Context::setShaderRWTexture(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op)
	{

		D3D12PooledHeapHandle const* handle;
		ID3D12Resource* resource;
		switch (texture.getType())
		{
		case ETexture::Type1D: 
			handle = &static_cast<D3D12Texture1D&>(texture).mUAV; 
			resource = static_cast<D3D12Texture1D&>(texture).getResource(); 
			break;
		case ETexture::Type2D:
			handle = &static_cast<D3D12Texture2D&>(texture).mUAV; 
			resource = static_cast<D3D12Texture2D&>(texture).getResource(); 
			break;
#if 0
		case ETexture::Type3D:
			handle = &static_cast<D3D12Texture3D&>(texture).mUAV;
			resource = static_cast<D3D12Texture3D&>(texture).getResource();
			break;
#endif
		}

		mResourceBoundStates[shaderType].mUAVStates[param.bindIndex].resource = resource;

		mGraphicsCmdList->ResourceBarrier(1, &FD3D12Init::TransitionBarrier(resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0));
		//mGraphicsCmdList->ResourceBarrier(1, &FD3D12Init::UAVBarrier(resource));

		updateCSUHeapUsage(*handle);

		auto const& slotInfo = shaderData.rootSignature.slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shaderType] + slotInfo.slotOffset;
		if (shaderType == EShader::Compute)
		{
			mGraphicsCmdList->SetComputeRootDescriptorTable(rootSlot, handle->getGPUHandle());
		}
		else
		{
			mGraphicsCmdList->SetGraphicsRootDescriptorTable(rootSlot, handle->getGPUHandle());
		}

	}

	void D3D12Context::clearShaderRWTexture(RHIShader& shader, ShaderParameter const& param)
	{
		auto& shaderImpl = static_cast<D3D12Shader&>(shader);
		clearShaderRWTexture(shaderImpl.getType(), shaderImpl, param);
	}

	void D3D12Context::clearShaderRWTexture(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param)
	{
		ID3D12Resource* resource = mResourceBoundStates[shaderType].mUAVStates[param.bindIndex].resource;
		if (resource)
		{
			//mGraphicsCmdList->ResourceBarrier(1, &FD3D12Init::UAVBarrier(resource));
			mGraphicsCmdList->ResourceBarrier(1, &FD3D12Init::TransitionBarrier(resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0));
		}
	}

	void D3D12Context::setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer)
	{
		auto& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &buffer](EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& shaderParam)
		{
			setShaderUniformBuffer(shaderType, shaderData, shaderParam, buffer);
		});
	}

	void D3D12Context::setShaderUniformBuffer(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHIBuffer& buffer)
	{
		auto& bufferImpl = static_cast<D3D12Buffer&>(buffer);
		auto const& slotInfo = shaderData.rootSignature.slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shaderType] + slotInfo.slotOffset;
		if (slotInfo.type == ShaderParameterSlotInfo::eDescriptorTable_CBV)
		{
			auto const& handle = bufferImpl.mCBV;
			updateCSUHeapUsage(handle);
			uint32 rootSlot = mRootSlotStart[shaderType] + slotInfo.slotOffset;
			if (shaderType == EShader::Compute)
			{
				mGraphicsCmdList->SetComputeRootDescriptorTable(rootSlot, handle.getGPUHandle());
			}
			else
			{
				mGraphicsCmdList->SetGraphicsRootDescriptorTable(rootSlot, handle.getGPUHandle());
			}
		}
		else
		{
			D3D12_GPU_VIRTUAL_ADDRESS address;
			if (bufferImpl.isDyanmic())
			{
				address = bufferImpl.mDynamicAllocation.gpuAddress;
			}
			else
			{
				address = bufferImpl.getResource()->GetGPUVirtualAddress();
			}
			if (shaderType == EShader::Compute)
			{

				mGraphicsCmdList->SetComputeRootConstantBufferView(rootSlot, address);
			}
			else
			{
				mGraphicsCmdList->SetGraphicsRootConstantBufferView(rootSlot, address);
			}
		}
	}

	void D3D12Context::setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer)
	{
		auto& shaderImpl = static_cast<D3D12Shader&>(shader);
		setShaderUniformBuffer(shaderImpl.getType(), shaderImpl, param, buffer);
	}

	void D3D12Context::setShaderStorageBuffer(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHIBuffer& buffer, EAccessOperator op)
	{
		auto& bufferImpl = static_cast<D3D12Buffer&>(buffer);

		ID3D12Resource* resource = bufferImpl.getResource();

		auto const& slotInfo = shaderData.rootSignature.slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shaderType] + slotInfo.slotOffset;


		switch (slotInfo.type)
		{
		case ShaderParameterSlotInfo::eUAV:
			{
				mResourceBoundStates[shaderType].mUAVStates[param.bindIndex].resource = resource;
				//mGraphicsCmdList->ResourceBarrier(1, &FD3D12Init::TransitionBarrier(resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0));
				//mGraphicsCmdList->ResourceBarrier(1, &FD3D12Init::UAVBarrier(resource));

				auto const& handle = bufferImpl.mUAV;
				updateCSUHeapUsage(handle);

				if (shaderType == EShader::Compute)
				{
					mGraphicsCmdList->SetComputeRootDescriptorTable(rootSlot, handle.getGPUHandle());
				}
				else
				{
					mGraphicsCmdList->SetGraphicsRootDescriptorTable(rootSlot, handle.getGPUHandle());
				}
			}
			break;
		case ShaderParameterSlotInfo::eDescriptorTable_UAV:
			{
				D3D12_GPU_VIRTUAL_ADDRESS address;
				if (bufferImpl.isDyanmic())
				{
					address = bufferImpl.mDynamicAllocation.gpuAddress;
				}
				else
				{
					address = bufferImpl.getResource()->GetGPUVirtualAddress();
					mResourceBoundStates[shaderType].mUAVStates[param.bindIndex].resource = resource;
				}

				//mGraphicsCmdList->ResourceBarrier(1, &FD3D12Init::TransitionBarrier(resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0));
				//mGraphicsCmdList->ResourceBarrier(1, &FD3D12Init::UAVBarrier(resource));
				if (shaderType == EShader::Compute)
				{
					mGraphicsCmdList->SetComputeRootUnorderedAccessView(rootSlot, address);
				}
				else
				{
					mGraphicsCmdList->SetGraphicsRootUnorderedAccessView(rootSlot, address);
				}
			}
			break;
		case ShaderParameterSlotInfo::eSRV:
			{
				D3D12_GPU_VIRTUAL_ADDRESS address;
				if (bufferImpl.isDyanmic())
				{
					address = bufferImpl.mDynamicAllocation.gpuAddress;
				}
				else
				{
					address = bufferImpl.getResource()->GetGPUVirtualAddress();
				}

				if (shaderType == EShader::Compute)
				{
					mGraphicsCmdList->SetComputeRootShaderResourceView(rootSlot, address);
				}
				else
				{
					mGraphicsCmdList->SetGraphicsRootShaderResourceView(rootSlot, address);
				}
			}
			break;
		case ShaderParameterSlotInfo::eDescriptorTable_SRV:
			{
				auto const& handle = bufferImpl.mSRV;
				updateCSUHeapUsage(handle);

				if (shaderType == EShader::Compute)
				{
					mGraphicsCmdList->SetComputeRootDescriptorTable(rootSlot, handle.getGPUHandle());
				}
				else
				{
					mGraphicsCmdList->SetGraphicsRootDescriptorTable(rootSlot, handle.getGPUHandle());
				}
			}
			break;
		}
	}


	void D3D12Context::setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer, EAccessOperator op)
	{
		auto& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &buffer, op](EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& shaderParam)
		{
			setShaderStorageBuffer(shaderType, shaderData, shaderParam, buffer, op);
		});
	}


	void D3D12Context::setShaderStorageBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer, EAccessOperator op)
	{
		auto& shaderImpl = static_cast<D3D12Shader&>(shader);
		setShaderStorageBuffer(shaderImpl.getType(), shaderImpl, param, buffer, op);
	}

	void D3D12Context::setShaderMatrix22(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim)
	{
		auto& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &val, dim](EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& shaderParam)
		{
			setShaderValueT(shaderType, shaderData, shaderParam, val, dim, 2 * sizeof(float));
		});
	}
	void D3D12Context::setShaderMatrix43(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim)
	{
		auto& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &val, dim](EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& shaderParam)
		{
			setShaderValueT(shaderType, shaderData, shaderParam, val, dim, 3 * sizeof(float));
		});
	}
	void D3D12Context::setShaderMatrix34(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim)
	{
		auto& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &val, dim](EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& shaderParam)
		{
			setShaderValueT(shaderType, shaderData, shaderParam, val, dim);
		});
	}

	void D3D12Context::setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler)
	{
		auto& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &texture](EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& shaderParam)
		{
			setShaderTexture(shaderType, shaderData, shaderParam, texture);
		});

		shaderProgramImpl.setupShader(paramSampler, [this, &sampler](EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& shaderParam)
		{
			setShaderSampler(shaderType, shaderData, shaderParam, sampler);
		});
	}

	void D3D12Context::setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture)
	{
		auto& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &texture](EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& shaderParam)
		{
			setShaderTexture(shaderType, shaderData, shaderParam, texture);
		});
	}

	void D3D12Context::commitRenderTargetState()
	{
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

		auto const& DSVHandle = mRenderTargetsState->depthBuffer.DSVHandle;
		if (DSVHandle.isValid())
		{
			mGraphicsCmdList->OMSetRenderTargets(mRenderTargetsState->numColorBuffers, handles, FALSE, &DSVHandle.getCPUHandle());
		}
		else
		{
			mGraphicsCmdList->OMSetRenderTargets(mRenderTargetsState->numColorBuffers, handles, FALSE, nullptr);
		}
	}

	void D3D12Context::commitGraphicsPipelineState(EPrimitive type)
	{
		if (mGfxBoundState == nullptr)
		{
			if (mInputLayoutPending)
			{
				D3D12InputLayout* inputLayoutImpl = static_cast<D3D12InputLayout*>(mInputLayoutPending.get());
				SimplePipelineProgram* program = SimplePipelineProgram::Get(inputLayoutImpl->mAttriableMask, mFixedShaderParams.texture);

				RHISetShaderProgram(program->getRHI());
				program->setParameters(RHICommandListImpl(*this), mFixedShaderParams);
			}
		}

		mGraphicsCmdList->IASetPrimitiveTopology(D3D12Translate::To(type));

		GraphicsRenderStateDesc stateDesc;

		stateDesc.primitiveType = type;
		stateDesc.inputLayout = mInputLayoutPending;

		stateDesc.rasterizerState = mRasterizerStatePending;
		stateDesc.depthStencilState = mDepthStencilStatePending;
		stateDesc.blendState = mBlendStateStatePending;

		auto pipelineState = static_cast<D3D12System*>(GRHISystem)->getPipelineState(stateDesc, mGfxBoundState, mRenderTargetsState);
		if (pipelineState)
		{
			mGraphicsCmdList->SetPipelineState(pipelineState->mResource);
			mPiplineStateCommitted = pipelineState->mResource;
		}

		commitInputStreams(false);
		commitInputBuffer(false);
	}

	void D3D12Context::commitMeshPipelineState()
	{
		if (mGfxBoundState == nullptr)
		{
			return;
		}
		GraphicsRenderStateDesc stateDesc;

		stateDesc.primitiveType = EPrimitive::TriangleList;
		stateDesc.inputLayout = nullptr;

		stateDesc.rasterizerState = mRasterizerStatePending;
		stateDesc.depthStencilState = mDepthStencilStatePending;
		stateDesc.blendState = mBlendStateStatePending;

		auto pipelineState = static_cast<D3D12System*>(GRHISystem)->getPipelineState(stateDesc, mGfxBoundState, mRenderTargetsState);
		if (pipelineState)
		{
			mGraphicsCmdList->SetPipelineState(pipelineState->mResource);
			mPiplineStateCommitted = pipelineState->mResource;
		}

		commitInputStreams(false);
		commitInputBuffer(false);
	}

	void D3D12Context::commitComputePipelineState()
	{
		auto pipelineState = static_cast<D3D12System*>(GRHISystem)->getPipelineState(mComputeBoundState);
		if (pipelineState)
		{
			mGraphicsCmdList->SetPipelineState(pipelineState->mResource);
		}
	}

	void D3D12Context::commitInputStreams(bool bForce)
	{
		if (!mbInpuStreamDirty && !bForce)
			return;

		//mbInpuStreamDirty = false;

		D3D12_VERTEX_BUFFER_VIEW bufferViews[MaxInputStream];
		for (int i = 0; i < mNumInputStream; ++i)
		{
			auto& bufferView = bufferViews[i];
			auto& inputStream = mInputStreams[i];
			if (inputStream.buffer)
			{
				auto& bufferImpl = static_cast<D3D12Buffer&>(*inputStream.buffer);
				if (bufferImpl.isDyanmic())
				{
					bufferView.BufferLocation = bufferImpl.mDynamicAllocation.gpuAddress + inputStream.offset;
					bufferView.SizeInBytes = bufferImpl.mDynamicAllocation.size - inputStream.offset;
				}
				else
				{
					bufferView.BufferLocation = bufferImpl.getResource()->GetGPUVirtualAddress() + inputStream.offset;
					bufferView.SizeInBytes = inputStream.buffer->getSize() - inputStream.offset;
				}
				bufferView.StrideInBytes = (inputStream.stride >= 0) ? inputStream.stride : inputStream.buffer->getElementSize();
			}
			else
			{
				bufferView.BufferLocation = 0;
				bufferView.SizeInBytes = 0;
				bufferView.StrideInBytes = 0;
			}
		}

		mGraphicsCmdList->IASetVertexBuffers(0, mNumInputStream, bufferViews);
	}

	void D3D12Context::commitInputBuffer(bool bForce)
	{
		if (!bIndexBufferStateDirty && !bForce)
			return;

		//bIndexBufferStateDirty = false;
		if (mIndexBufferState.isValid())
		{
			auto& bufferImpl = static_cast<D3D12Buffer&>(*mIndexBufferState);
			D3D12_INDEX_BUFFER_VIEW bufferView;
			if (bufferImpl.isDyanmic())
			{
				bufferView.BufferLocation = bufferImpl.mDynamicAllocation.gpuAddress;
				bufferView.SizeInBytes    = bufferImpl.mDynamicAllocation.size;
			}
			else
			{
				bufferView.BufferLocation = bufferImpl.mResource->GetGPUVirtualAddress();
				bufferView.SizeInBytes = mIndexBufferState->getSize();
			}

			bufferView.Format = D3D12Translate::IndexType(mIndexBufferState);
			mGraphicsCmdList->IASetIndexBuffer(&bufferView);
		}
		else
		{
			mGraphicsCmdList->IASetIndexBuffer(nullptr);
		}
	}

	bool D3D12Context::beginFrame()
	{
		FrameData& frameData = mFrameDataList[mFrameIndex];
		VERIFY_RETURN_FALSE(frameData.beginFrame());
		mGraphicsCmdList = frameData.graphicsCmdList;

		D3D12_RESOURCE_BARRIER barrier = FD3D12Init::TransitionBarrier(
			static_cast<D3D12System*>(GRHISystem)->mSwapChain->mRenderTargetsStates[mFrameIndex].colorBuffers[0].resource,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mGraphicsCmdList->ResourceBarrier(1, &barrier);

		for (int i = 0; i < EShader::Count; ++i)
		{
			mResourceBoundStates[i].restState();
		}


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
		D3D12FenceResourceManager::Get().mFenceValue = mFrameDataList[mFrameIndex].fenceValue;

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
		uint64 lastCompletedFenceValue = mFence->GetCompletedValue();
		D3D12FenceResourceManager::Get().releaseFence(lastCompletedFenceValue);

		D3D12DynamicBufferManager::Get().markFence();

		// Set the fence value for the next frame.
		mFrameDataList[mFrameIndex].fenceValue = currentFenceValue + 1;
		D3D12FenceResourceManager::Get().mFenceValue = mFrameDataList[mFrameIndex].fenceValue;
	}

	bool D3D12ResourceBoundState::initialize(ID3D12DeviceRHI* device)
	{
		return true;
	}

	void D3D12ResourceBoundState::restState()
	{
		mGlobalConstAllocation.ptr = nullptr;
		mShaderData = nullptr;
		mbGlobalConstCommited = false;
	}

	void D3D12ResourceBoundState::postDrawOrDispatchCall()
	{
		if (mGlobalConstAllocation.ptr)
		{
			mbGlobalConstCommited = true;
		}
	}

	bool D3D12ResourceBoundState::updateConstBuffer(D3D12ShaderData& shaderData)
	{
		if (mbGlobalConstCommited || mShaderData != &shaderData)
		{
			mbGlobalConstCommited = false;

			uint32 bufferSize = shaderData.rootSignature.globalCBSize;
			bufferSize = ConstBufferMultipleSize * ((bufferSize + ConstBufferMultipleSize - 1) / ConstBufferMultipleSize);

			bool bCopyPrev = mShaderData == &shaderData;
			void* prevData = bCopyPrev ? mGlobalConstAllocation.cpuAddress : nullptr;
			D3D12DynamicBufferManager::Get().allocFrame(bufferSize, ConstBufferMultipleSize, mGlobalConstAllocation);
			if (prevData)
			{
				FMemory::Copy(mGlobalConstAllocation.cpuAddress, prevData, shaderData.rootSignature.globalCBSize);
			}
			mShaderData = &shaderData;
			return true;
		}
		return false;
	}

	void D3D12ResourceBoundState::updateConstantData(void const* pData, uint32 offset, uint32 size)
	{
		FMemory::Copy(mGlobalConstAllocation.cpuAddress + offset, pData, size);
	}

	template< typename T >
	uint32 GetGUID(T* ptr) { return ptr ? ptr->mGUID : 0; }

	uint32 GetGUID(D3D12RenderTargetsState* ptr) { return ptr ? ptr->formatGUID : 0; }

	void D3D12PipelineStateKey::initialize(GraphicsRenderStateDesc const& renderState, D3D12ShaderBoundState* boundState, D3D12RenderTargetsState* renderTargetsState)
	{
		boundStateKey = boundState->cachedKey;
		rasterizerID = GetGUID(renderState.rasterizerState);
		depthStenceilID = GetGUID(renderState.depthStencilState);
		blendID = GetGUID(renderState.blendState); 
		renderTargetFormatID = GetGUID(renderTargetsState);

		inputLayoutID = GetGUID(renderState.inputLayout);
		primitiveType = (uint32)renderState.primitiveType;
	}

	void D3D12PipelineStateKey::initialize(MeshRenderStateDesc const& renderState, D3D12ShaderBoundState* boundState, D3D12RenderTargetsState* renderTargetsState)
	{
		boundStateKey = boundState->cachedKey;
		rasterizerID = GetGUID(renderState.rasterizerState);
		depthStenceilID = GetGUID(renderState.depthStencilState);
		blendID = GetGUID(renderState.blendState); 
		renderTargetFormatID = GetGUID(renderTargetsState);

		inputLayoutID = 0;
		primitiveType = 0;
	}

	void D3D12PipelineStateKey::initialize(D3D12ShaderBoundState* boundState)
	{
		boundStateKey = boundState->cachedKey;
		value = 0;
	}

	bool D3D12DynamicBuffer::initialize()
	{
		return true;
	}

	void* D3D12DynamicBuffer::lock(uint32 size)
	{
		if (!D3D12DynamicBufferManager::Get().allocFrame(size, D3D12_BUFFER_SIZE_ALIGN, mLockedResource))
			return nullptr;
		
		return mLockedResource.cpuAddress;
	}

	void D3D12DynamicBuffer::unlock(D3D12_VERTEX_BUFFER_VIEW& bufferView)
	{
		CHECK(mLockedResource.ptr);
		bufferView.BufferLocation = mLockedResource.gpuAddress;
		bufferView.SizeInBytes = mLockedResource.size;
		bufferView.StrideInBytes = 0;
		mLockedResource.ptr = nullptr;
	}

	void D3D12DynamicBuffer::unlock(D3D12_INDEX_BUFFER_VIEW& bufferView)
	{
		CHECK(mLockedResource.ptr);
		bufferView.BufferLocation = mLockedResource.gpuAddress;
		bufferView.SizeInBytes    = mLockedResource.size;
		bufferView.Format = DXGI_FORMAT_R32_UINT;
		mLockedResource.ptr = nullptr;
	}

}//namespace Render

#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif
