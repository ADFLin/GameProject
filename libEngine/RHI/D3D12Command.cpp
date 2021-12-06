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
			mNextHandle = 2;
			return true;
		}

		bool addChunkResource()
		{
			TimeStampChunk timeStamp;
			D3D12_QUERY_HEAP_DESC desc;
			desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
			desc.NodeMask = 0x1;
			desc.Count = 1;
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
				D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&timeStamp.reslutResource)));

			mChunks.push_back(std::move(timeStamp));
			return true;
		}

		virtual void beginFrame() override
		{
			//mCmdList->EndQuery(mChunks[0].heap, D3D12_QUERY_TYPE_TIMESTAMP, 0);
		}

		virtual bool endFrame() override
		{
			//mCmdList->EndQuery(mChunks[0].heap, D3D12_QUERY_TYPE_TIMESTAMP, 1);
			for (auto& chunk : mChunks)
			{
				//mCmdList->ResolveQueryData(chunk.heap, D3D12_QUERY_TYPE_TIMESTAMP, 0, TimeStampChunk::Size * 2, chunk.reslutResource, 0);
			}
			return true;
		}

		virtual void beginReadback()
		{
			for (auto& chunk : mChunks)
			{
				chunk.reslutResource->Map(0, NULL, (void**)&chunk.timeStampData);
			}
		}

		virtual void endReadback()
		{
			for (auto& chunk : mChunks)
			{
				chunk.reslutResource->Unmap(0, NULL);
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
			//mCmdList->EndQuery(mChunks[chunkIndex].heap, D3D12_QUERY_TYPE_TIMESTAMP, index + 0);
		}

		virtual void endTiming(uint32 timingHandle) override
		{
			uint32 chunkIndex = timingHandle / TimeStampChunk::Size;
			uint32 index = 2 * (timingHandle % TimeStampChunk::Size);
			//mCmdList->EndQuery(mChunks[chunkIndex].heap, D3D12_QUERY_TYPE_TIMESTAMP, index + 1 );
		}

		virtual bool getTimingDuration(uint32 timingHandle, uint64& outDuration) override
		{
			uint32 chunkIndex = timingHandle / TimeStampChunk::Size;
			uint32 index = 2 * (timingHandle % TimeStampChunk::Size);

			uint64 startData = mChunks[chunkIndex].timeStampData[index];
			uint64 endData = mChunks[chunkIndex].timeStampData[index + 1];
			outDuration = endData - startData;
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
			TComPtr< ID3D12Resource >  reslutResource;
			uint64* timeStampData;
			
			static constexpr uint32 Size = 512;
			void releaseRHI()
			{
				heap.reset();
				reslutResource.reset();
			}
		};
		uint32 mNextHandle;
		std::vector< TimeStampChunk > mChunks;
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

		return true;
	}

	void D3D12System::RHIEndRender(bool bPresent)
	{
		mRenderContext.endFrame();

		mRenderContext.RHIFlushCommand();
		if (bPresent)
		{
			mSwapChain->Present(bPresent);
		}
		mRenderContext.moveToNextFrame(mSwapChain->mResource);
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
		swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		//#FIXME info.SampleCount
		swapChainDesc.SampleDesc.Count = 1;

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

	RHITexture1D* D3D12System::RHICreateTexture1D(ETexture::Format format, int length, int numMipLevel, uint32 createFlags, void* data)
	{
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = (numMipLevel) ? numMipLevel : 1;
		textureDesc.Format = D3D12Translate::To(format);
		textureDesc.Width = length;
		textureDesc.Height = 1;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (createFlags & TCF_CreateUAV)
			textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		if (createFlags & TCF_RenderTarget)
			textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
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
			if (!updateTexture1DSubresources(textureResource, format, data, 0, length, 0))
				return nullptr;
		}


		D3D12Texture1D* texture = new D3D12Texture1D;
		if (!texture->initialize(textureResource, format, length))
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
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;

			//TODO
			srvDesc.Texture1D.MipLevels = 1;

			texture->mSRV = D3D12DescriptorHeapPool::Get().allocSRV(texture->mResource, &srvDesc);
		}
		if (createFlags & TCF_CreateUAV)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = textureDesc.Format;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			uavDesc.Texture1D.MipSlice = 0;
			texture->mUAV = D3D12DescriptorHeapPool::Get().allocUAV(texture->mResource, &uavDesc);
		}

		return texture;


		return nullptr;
	}

	RHITexture2D* D3D12System::RHICreateTexture2D(ETexture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 createFlags, void* data, int dataAlign)
	{

		// Describe and create a Texture2D.
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = (numMipLevel) ? numMipLevel : 1;
		textureDesc.Format = D3D12Translate::To(format);
		textureDesc.Width = w;
		textureDesc.Height = h;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		if (createFlags & TCF_CreateUAV)
			textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		if (createFlags & TCF_RenderTarget)
			textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

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
			if (!updateTexture2DSubresources(textureResource, format, data, 0, 0, w, h, w * ETexture::GetFormatSize(format), 0))
				return nullptr;
		}


		D3D12Texture2D* texture = new D3D12Texture2D;
		if (!texture->initialize(textureResource, format, w, h ))
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

			//TODO
			srvDesc.Texture2D.MipLevels = 1;

			texture->mSRV = D3D12DescriptorHeapPool::Get().allocSRV(texture->mResource, &srvDesc);
		}

		if (createFlags & TCF_CreateUAV)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = textureDesc.Format;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = 0;
			uavDesc.Texture2D.PlaneSlice = 0;
			texture->mUAV = D3D12DescriptorHeapPool::Get().allocUAV(texture->mResource, &uavDesc);
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
			else
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = FD3D12Init::BufferViewDesc(vertexSize);
				result->mViewHandle = D3D12DescriptorHeapPool::Get().allocSRV(resource, &srvDesc);
			}
		}

		return result;
	}

	RHIIndexBuffer*  D3D12System::RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
	{
		int indexSize = bIntIndex ? 4 : 2;
		int bufferSize = indexSize * nIndices;

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
			UINT8* pIndexDataBegin;
			D3D12_RANGE readRange = {};        // We do not intend to read from this resource on the CPU.
			VERIFY_D3D_RESULT(resource->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)), return nullptr; );
			memcpy(pIndexDataBegin, data, bufferSize);
			resource->Unmap(0, nullptr);
		}

		D3D12IndexBuffer* result = new D3D12IndexBuffer;
		if (!result->initialize(resource, mDevice, indexSize, nIndices))
		{
			delete result;
			return nullptr;
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

	void* D3D12System::RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		D3D12_RANGE range = {};
		void* result = nullptr;
		static_cast<D3D12IndexBuffer*>(buffer)->mResource->Map(0, &range, &result);
		return result;
	}

	void D3D12System::RHIUnlockBuffer(RHIIndexBuffer* buffer)
	{
		D3D12_RANGE range = {};
		static_cast<D3D12IndexBuffer*>(buffer)->mResource->Unmap(0, nullptr);
	}

	Render::RHIFrameBuffer* D3D12System::RHICreateFrameBuffer()
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

	Render::RHIShaderProgram* D3D12System::RHICreateShaderProgram()
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

	bool D3D12System::updateTexture1DSubresources(ID3D12Resource* textureResource, ETexture::Format format, void* data, uint32 offset, uint32 length, uint32 level /*= 0*/)
	{
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

	D3D12ShaderBoundState* D3D12System::getShaderBoundState(GraphicsShaderStateDesc const& stateDesc)
	{
		D3D12ShaderBoundStateKey key;
		key.initialize(stateDesc);
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
			info.type = shader->getType();
			info.data = static_cast<D3D12Shader*>(shader);
			info.rootSlotStart = rootParameters.size();
			boundState->mShaders.push_back(info);

			auto shaderImpl = static_cast<D3D12Shader*>(shader);
			bool result = false;
			if (shaderImpl->rootSignature.globalCBRegister != INDEX_NONE)
			{
				D3D12_ROOT_PARAMETER1 parameter = {};
				parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				parameter.Descriptor.ShaderRegister = shaderImpl->rootSignature.globalCBRegister;
				rootParameters.push_back(parameter);
				result = true;
			}

			for (int i = 0; i < shaderImpl->rootSignature.descRanges.size(); ++i)
			{
				D3D12_ROOT_PARAMETER1 parameter = shaderImpl->getRootParameter(i);
				rootParameters.push_back(parameter);
				result = true;
			}
			rootSamplers.insert(rootSamplers.end(), shaderImpl->rootSignature.samplers.begin(), shaderImpl->rootSignature.samplers.end());
			return result;
		};

		if (AddShader(stateDesc.vertex))
			denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;

		if (AddShader(stateDesc.pixel))
			denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		if (AddShader(stateDesc.geometry))
			denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		if (AddShader(stateDesc.hull))
			denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

		if (AddShader(stateDesc.domain))
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

	D3D12ShaderBoundState* D3D12System::getShaderBoundState(MeshShaderStateDesc const& stateDesc)
	{
		D3D12ShaderBoundStateKey key;
		key.initialize(stateDesc);
		auto iter = mShaderBoundStateMap.find(key);
		if (iter != mShaderBoundStateMap.end())
		{
			return iter->second;
		}

		std::unique_ptr< D3D12ShaderBoundState > boundState = std::make_unique< D3D12ShaderBoundState>();

		std::vector< D3D12_STATIC_SAMPLER_DESC > rootSamplers;
		std::vector< D3D12_ROOT_PARAMETER1 > rootParameters;


		D3D12_ROOT_SIGNATURE_FLAGS denyShaderRootAccessFlags =
			D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		auto AddShader = [&](RHIShader* shader) -> bool
		{
			if (shader == nullptr)
				return false;
			D3D12ShaderBoundState::ShaderInfo info;
			info.type = shader->getType();
			info.data = static_cast<D3D12Shader*>(shader);
			info.rootSlotStart = rootParameters.size();
			boundState->mShaders.push_back(info);

			auto shaderImpl = static_cast<D3D12Shader*>(shader);
			bool result = false;
			if (shaderImpl->rootSignature.globalCBRegister != INDEX_NONE)
			{
				D3D12_ROOT_PARAMETER1 parameter = {};
				parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				parameter.Descriptor.ShaderRegister = shaderImpl->rootSignature.globalCBRegister;
				parameter.ShaderVisibility = shaderImpl->rootSignature.visibility;
				rootParameters.push_back(parameter);
				result = true;
			}
			for (int i = 0; i < shaderImpl->rootSignature.descRanges.size(); ++i)
			{
				D3D12_ROOT_PARAMETER1 parameter = shaderImpl->getRootParameter(i);
				rootParameters.push_back(parameter);
				result = true;
			}
			rootSamplers.insert(rootSamplers.end(), shaderImpl->rootSignature.samplers.begin(), shaderImpl->rootSignature.samplers.end());
			return result;
		};

		if (AddShader(stateDesc.task))
			denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
		if (AddShader(stateDesc.mesh))
			denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
		if (AddShader(stateDesc.pixel))
			denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;


		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSignatureDesc.Desc_1_1.NumParameters = rootParameters.size();
		rootSignatureDesc.Desc_1_1.pParameters = rootParameters.data();
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = rootSamplers.size();
		rootSignatureDesc.Desc_1_1.pStaticSamplers = rootSamplers.data();
		rootSignatureDesc.Desc_1_1.Flags = denyShaderRootAccessFlags;

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

	D3D12ShaderBoundState* D3D12System::getShaderBoundState(D3D12ShaderProgram* shaderProgram)
	{
		D3D12ShaderBoundStateKey key;
		key.initialize(shaderProgram);
		auto iter = mShaderBoundStateMap.find(key);
		if (iter != mShaderBoundStateMap.end())
		{
			return iter->second;
		}


		std::unique_ptr< D3D12ShaderBoundState > boundState = std::make_unique< D3D12ShaderBoundState>();

		std::vector< D3D12_STATIC_SAMPLER_DESC > rootSamplers;
		std::vector< D3D12_ROOT_PARAMETER1 > rootParameters;

		D3D12_ROOT_SIGNATURE_FLAGS rootAccessFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		auto AddShaderData = [&](D3D12ShaderProgram::ShaderData& shaderData) -> bool
		{
			D3D12ShaderBoundState::ShaderInfo info;
			info.type = shaderData.type;
			info.data = &shaderData;
			info.rootSlotStart = rootParameters.size();
			boundState->mShaders.push_back(info);

			bool result = false;
			if (shaderData.rootSignature.globalCBRegister != INDEX_NONE)
			{
				D3D12_ROOT_PARAMETER1 parameter = {};
				parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				parameter.Descriptor.ShaderRegister = shaderData.rootSignature.globalCBRegister;
				rootParameters.push_back(parameter);
				result = true;
			}

			for (int i = 0; i < shaderData.rootSignature.descRanges.size(); ++i)
			{
				D3D12_ROOT_PARAMETER1 parameter = shaderData.getRootParameter(i);
				rootParameters.push_back(parameter);
				result = true;
			}
			rootSamplers.insert(rootSamplers.end(), shaderData.rootSignature.samplers.begin(), shaderData.rootSignature.samplers.end());
			return result;
		};

		auto FindShaderTypeMask = [shaderProgram](uint32 shaderMasks) -> uint32
		{
			for (int i = 0; i < shaderProgram->mNumShaders; ++i)
			{
				uint32 mask = BIT(shaderProgram->mShaderDatas[i].type);
				if (shaderMasks & mask)
					return mask;
			}

			return 0;
		};

		auto specialShaderTypeMask = FindShaderTypeMask(BIT(EShader::Mesh) | BIT(EShader::Compute));
		if (specialShaderTypeMask & BIT(EShader::Compute))
		{
			CHECK(shaderProgram->mNumShaders == 1);

		}
		else if (specialShaderTypeMask & BIT(EShader::Mesh))
		{
			CHECK(shaderProgram->mNumShaders <= 3);
			rootAccessFlags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		}
		else
		{
			GraphicsShaderStateDesc stateDesc;
			rootAccessFlags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		}

		for (int i = 0; i < shaderProgram->mNumShaders; ++i)
		{
			auto& shaderData = shaderProgram->mShaderDatas[i];
			if (!AddShaderData(shaderData))
			{
				switch (shaderData.type)
				{
				case EShader::Vertex:   rootAccessFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS; break;
				case EShader::Pixel:    rootAccessFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS; break;
				case EShader::Geometry: rootAccessFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS; break;
				case EShader::Hull:     rootAccessFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS; break;
				case EShader::Domain:   rootAccessFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS; break;
				case EShader::Mesh:     rootAccessFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS; break;
				case EShader::Task:     rootAccessFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS; break;
				}
			}
		}


		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSignatureDesc.Desc_1_1.NumParameters = rootParameters.size();
		rootSignatureDesc.Desc_1_1.pParameters = rootParameters.data();
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = rootSamplers.size();
		rootSignatureDesc.Desc_1_1.pStaticSamplers = rootSamplers.data();
		rootSignatureDesc.Desc_1_1.Flags = rootAccessFlags;

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

	D3D12ShaderBoundState* D3D12System::getShaderBoundState(RHIShader* computeShader)
	{
		D3D12ShaderBoundStateKey key;
		key.initialize(computeShader);
		auto iter = mShaderBoundStateMap.find(key);
		if (iter != mShaderBoundStateMap.end())
		{
			return iter->second;
		}

		std::unique_ptr< D3D12ShaderBoundState > boundState = std::make_unique< D3D12ShaderBoundState>();

		std::vector< D3D12_STATIC_SAMPLER_DESC > rootSamplers;
		std::vector< D3D12_ROOT_PARAMETER1 > rootParameters;


		D3D12_ROOT_SIGNATURE_FLAGS denyShaderRootAccessFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;


		auto AddShader = [&](RHIShader* shader) -> bool
		{
			if (shader == nullptr)
				return false;
			D3D12ShaderBoundState::ShaderInfo info;
			info.type = shader->getType();
			info.data = static_cast<D3D12Shader*>(shader);
			info.rootSlotStart = rootParameters.size();
			boundState->mShaders.push_back(info);

			auto shaderImpl = static_cast<D3D12Shader*>(shader);
			bool result = false;
			if (shaderImpl->rootSignature.globalCBRegister != INDEX_NONE)
			{
				D3D12_ROOT_PARAMETER1 parameter = {};
				parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				parameter.Descriptor.ShaderRegister = shaderImpl->rootSignature.globalCBRegister;
				parameter.ShaderVisibility = shaderImpl->rootSignature.visibility;
				rootParameters.push_back(parameter);
				result = true;
			}
			for (int i = 0; i < shaderImpl->rootSignature.descRanges.size(); ++i)
			{
				D3D12_ROOT_PARAMETER1 parameter = shaderImpl->getRootParameter(i);
				rootParameters.push_back(parameter);
				result = true;
			}
			rootSamplers.insert(rootSamplers.end(), shaderImpl->rootSignature.samplers.begin(), shaderImpl->rootSignature.samplers.end());
			return result;
		};

		AddShader(computeShader);


		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSignatureDesc.Desc_1_1.NumParameters = rootParameters.size();
		rootSignatureDesc.Desc_1_1.pParameters = rootParameters.data();
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = rootSamplers.size();
		rootSignatureDesc.Desc_1_1.pStaticSamplers = rootSamplers.data();
		rootSignatureDesc.Desc_1_1.Flags = denyShaderRootAccessFlags;

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

	D3D12PipelineState* D3D12System::getPipelineState(GraphicsRenderStateDesc const& renderState, D3D12ShaderBoundState* boundState, D3D12RenderTargetsState* renderTargetsState)
	{

		D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12Translate::ToTopologyType(renderState.primitiveType);
		D3D12PipelineStateKey key;
		key.initialize(renderState, boundState, renderTargetsState, topologyType);
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
			case EShader::Vertex:
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS>();
					data = shaderInfo.data->getByteCode();
				}
				break;
			case EShader::Pixel:
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS>();
					data = shaderInfo.data->getByteCode();
				}
				break;
			case EShader::Geometry:
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS>();
					data = shaderInfo.data->getByteCode();
				}
				break;
			case EShader::Hull:
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS>();
					data = shaderInfo.data->getByteCode();
				}
				break;
			case EShader::Domain:
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS>();
					data = shaderInfo.data->getByteCode();
				}
				break;
			}
		}

		if (renderState.inputLayout)
		{
			auto& inputLayout = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT>();
			inputLayout = static_cast<D3D12InputLayout*>(renderState.inputLayout)->getDesc();
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

		RHIRasterizerState* rasterizerStateUsage = renderState.rasterizerState ? renderState.rasterizerState : &TStaticRasterizerState<>::GetRHI();
		streamDesc.RasterizerState = static_cast<D3D12RasterizerState*>(rasterizerStateUsage)->mDesc;
		RHIDepthStencilState* depthStencielStateUsage = renderState.depthStencilState ? renderState.depthStencilState : &TStaticDepthStencilState<>::GetRHI();
		streamDesc.DepthStencilState = static_cast<D3D12DepthStencilState*>(depthStencielStateUsage)->mDesc;
		RHIBlendState* blendStateUsage = renderState.blendState ? renderState.blendState : &TStaticBlendState<>::GetRHI();
		streamDesc.BlendState = static_cast<D3D12BlendState*>(blendStateUsage)->mDesc;

		streamDesc.PrimitiveTopologyType = topologyType;

		if ( renderTargetsState )
		{
			streamDesc.RTFormatArray.NumRenderTargets = renderTargetsState->numColorBuffers;
			for (int i = 0; i < renderTargetsState->numColorBuffers; ++i)
			{
				streamDesc.RTFormatArray.RTFormats[0] = renderTargetsState->colorBuffers[i].format;
			}
		}
		else
		{
			streamDesc.RTFormatArray.NumRenderTargets = 0;
		}

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
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS>();
					data = shaderInfo.data->getByteCode();
				}
				break;
			case EShader::Task:
				{
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS>();
					data = shaderInfo.data->getByteCode();
				}
				break;
			default:
				{

				}
			}
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
			TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER > RasterizerState;
			TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_RHI > DepthStencilState;
			TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND > BlendState;
		};

		auto& streamDesc = stateStream.addDataT< FixedPipelineStateStream >();

		streamDesc.pRootSignature = boundState->mRootSignature;

		RHIRasterizerState* rasterizerStateUsage = renderState.rasterizerState ? renderState.rasterizerState : &TStaticRasterizerState<>::GetRHI();
		streamDesc.RasterizerState = static_cast<D3D12RasterizerState*>(rasterizerStateUsage)->mDesc;
		RHIDepthStencilState* depthStencielStateUsage = renderState.depthStencilState ? renderState.depthStencilState : &TStaticDepthStencilState<>::GetRHI();
		streamDesc.DepthStencilState = static_cast<D3D12DepthStencilState*>(depthStencielStateUsage)->mDesc;
		RHIBlendState* blendStateUsage = renderState.blendState ? renderState.blendState : &TStaticBlendState<>::GetRHI();
		streamDesc.BlendState = static_cast<D3D12BlendState*>(blendStateUsage)->mDesc;

		if (renderTargetsState)
		{
			streamDesc.RTFormatArray.NumRenderTargets = renderTargetsState->numColorBuffers;
			for (int i = 0; i < renderTargetsState->numColorBuffers; ++i)
			{
				streamDesc.RTFormatArray.RTFormats[0] = renderTargetsState->colorBuffers[i].format;
			}
		}
		else
		{
			streamDesc.RTFormatArray.NumRenderTargets = 0;
		}

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


		uint32 dynamicVBufferSize[] = { sizeof(float) * 512 , sizeof(float) * 1024 , sizeof(float) * 1024 * 4 , sizeof(float) * 1024 * 8 };
		VERIFY_RETURN_FALSE( mVertexBufferUP.initialize(mDevice,  dynamicVBufferSize, ARRAY_SIZE(dynamicVBufferSize)) );
		uint32 dynamicIBufferSize[] = { sizeof(uint32) * 3 * 16 , sizeof(uint32) * 3 * 64  , sizeof(uint32) * 3 * 256 , sizeof(uint32) * 3 * 1024 };
		VERIFY_RETURN_FALSE( mIndexBufferUP.initialize(mDevice, dynamicIBufferSize, ARRAY_SIZE(dynamicIBufferSize)) );
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

	bool D3D12Context::determitPrimitiveTopologyUP(EPrimitive primitive, int num, uint32 const* pIndices, EPrimitive& outPrimitiveDetermited, D3D12_INDEX_BUFFER_VIEW& outIndexBufferView, int& outIndexNum, ID3D12Resource*& outIndexResource)
	{
		if (primitive == EPrimitive::Quad)
		{
			int numQuad = num / 4;
			int indexBufferSize = sizeof(uint32) * numQuad * 6;
			void* pIndexBufferData = mIndexBufferUP.lock(mGraphicsCmdList, indexBufferSize);
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
			outIndexResource = mIndexBufferUP.unlock(mGraphicsCmdList, outIndexBufferView);
			outIndexNum = numQuad * 6;
			return true;
		}
		else if (primitive == EPrimitive::LineLoop)
		{
			if (pIndices)
			{
				int indexBufferSize = sizeof(uint32) * (num + 1);
				void* pIndexBufferData = mIndexBufferUP.lock(mGraphicsCmdList, indexBufferSize);
				if (pIndexBufferData == nullptr)
					return false;
				uint32* pData = (uint32*)pIndexBufferData;
				for (int i = 0; i < num; ++i)
				{
					pData[i] = pIndices[i];
				}
				pData[num] = pIndices[0];
				outIndexResource = mIndexBufferUP.unlock(mGraphicsCmdList, outIndexBufferView);
				outIndexNum = num + 1;

			}
			outPrimitiveDetermited = EPrimitive::LineStrip;
			return true;
		}
		else if (primitive == EPrimitive::Polygon)
		{
			if (num <= 2)
				return false;

			int numTriangle = (num - 2);

			int indexBufferSize = sizeof(uint32) * numTriangle * 3;
			void* pIndexBufferData = mIndexBufferUP.lock(mGraphicsCmdList, indexBufferSize);
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
			outIndexResource = mIndexBufferUP.unlock(mGraphicsCmdList, outIndexBufferView);
			outIndexNum = numTriangle * 3;
			return true;
		}

		
		if (D3D12Translate::To(primitive) == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
			return false;

		outPrimitiveDetermited = primitive;

		if (pIndices)
		{
			uint32 indexBufferSize = num * sizeof(uint32);
			void* pIndexBufferData = mIndexBufferUP.lock(mGraphicsCmdList, indexBufferSize);
			if (pIndexBufferData == nullptr)
				return false;

			memcpy(pIndexBufferData, pIndices, indexBufferSize);
			outIndexResource = mIndexBufferUP.unlock(mGraphicsCmdList, outIndexBufferView);
			outIndexNum = num;

		}
		return true;
	}

	void D3D12Context::RHIDrawPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numVertexData)
	{

		assert(numVertexData <= MAX_INPUT_STREAM_NUM);
		EPrimitive determitedPrimitive;
		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		int numDrawIndex;
		ID3D12Resource* indexResource = nullptr;
		if (!determitPrimitiveTopologyUP(type, numVertices, nullptr, determitedPrimitive, indexBufferView, numDrawIndex, indexResource))
			return;

		if (type == EPrimitive::LineLoop)
			++numVertices;

		uint32 vertexBufferSize = 0;
		for (int i = 0; i < numVertexData; ++i)
		{
			vertexBufferSize += (D3D12_BUFFER_SIZE_ALIGN * dataInfos[i].size + D3D12_BUFFER_SIZE_ALIGN - 1) / D3D12_BUFFER_SIZE_ALIGN;
		}

		uint8* pVBufferData = (uint8*)mVertexBufferUP.lock(mGraphicsCmdList, vertexBufferSize);
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
					memcpy(pVBufferData + dataOffset, info.ptr, info.size);
					memcpy(pVBufferData + dataOffset + info.size, info.ptr, info.stride);
					dataOffset += (D3D12_BUFFER_SIZE_ALIGN * (info.size + info.stride) + D3D12_BUFFER_SIZE_ALIGN - 1) / D3D12_BUFFER_SIZE_ALIGN;
				}
				else
				{
					memcpy(pVBufferData + dataOffset, info.ptr, info.size);
					dataOffset += (D3D12_BUFFER_SIZE_ALIGN * info.size + D3D12_BUFFER_SIZE_ALIGN - 1) / D3D12_BUFFER_SIZE_ALIGN;
				}
			}

			D3D12_VERTEX_BUFFER_VIEW bufferView;
			ID3D12Resource* vertexResource = mVertexBufferUP.unlock(mGraphicsCmdList, bufferView);
			D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[MAX_INPUT_STREAM_NUM];
			for (int i = 0; i < numVertexData; ++i)
			{
				vertexBufferViews[i].BufferLocation = bufferView.BufferLocation + offsets[i];
				vertexBufferViews[i].StrideInBytes = dataInfos[i].stride;
				vertexBufferViews[i].SizeInBytes = dataInfos[i].size;
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
		ID3D12Resource* indexResource = nullptr;
		if (!determitPrimitiveTopologyUP(type, numIndex, pIndices, determitedPrimitive, indexBufferView, numDrawIndex, indexResource))
			return;

		uint32 vertexBufferSize = 0;
		for (int i = 0; i < numVertexData; ++i)
		{
			vertexBufferSize += (D3D12_BUFFER_SIZE_ALIGN * dataInfos[i].size + D3D12_BUFFER_SIZE_ALIGN - 1) / D3D12_BUFFER_SIZE_ALIGN;
		}

		uint8* pVBufferData = (uint8*)mVertexBufferUP.lock(mGraphicsCmdList, vertexBufferSize);
		if (pVBufferData)
		{
			commitGraphicsPipelineState(determitedPrimitive);

			uint32 dataOffset = 0;
			UINT offsets[MAX_INPUT_STREAM_NUM];
			for (int i = 0; i < numVertexData; ++i)
			{
				auto const& info = dataInfos[i];
				offsets[i] = dataOffset;
				memcpy(pVBufferData + dataOffset, info.ptr, info.size);
				dataOffset += (D3D12_BUFFER_SIZE_ALIGN * info.size + D3D12_BUFFER_SIZE_ALIGN - 1) / D3D12_BUFFER_SIZE_ALIGN;
			}

			D3D12_VERTEX_BUFFER_VIEW bufferView;
			ID3D12Resource* vertexResource = mVertexBufferUP.unlock(mGraphicsCmdList, bufferView);
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

	void D3D12Context::RHISetIndexBuffer(RHIIndexBuffer* indexBuffer)
	{
		if (indexBuffer)
		{
			D3D12_INDEX_BUFFER_VIEW bufferView;
			bufferView.BufferLocation = static_cast<D3D12IndexBuffer&>(*indexBuffer).mResource->GetGPUVirtualAddress();
			bufferView.SizeInBytes = indexBuffer->getSize();
			bufferView.Format = indexBuffer->isIntType() ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
			mGraphicsCmdList->IASetIndexBuffer(&bufferView);
		}
		else
		{
			mGraphicsCmdList->IASetIndexBuffer(nullptr);
		}
	}

	void D3D12Context::RHIFlushCommand()
	{
		mGraphicsCmdList->Close();
		ID3D12CommandList* ppCommandLists[] = { mGraphicsCmdList };
		mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	void D3D12Context::RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ)
	{
		commitComputePipelineState();
		mGraphicsCmdList->Dispatch(numGroupX, numGroupY, numGroupZ);
		postDrawPrimitive();
	}

	void D3D12Context::RHISetGraphicsShaderBoundState(GraphicsShaderStateDesc const& stateDesc)
	{
		auto newState = static_cast<D3D12System*>(GRHISystem)->getShaderBoundState(stateDesc);
		setShaderBoundState(newState);
	}

	void D3D12Context::RHISetMeshShaderBoundState(MeshShaderStateDesc const& stateDesc)
	{
		auto newState = static_cast<D3D12System*>(GRHISystem)->getShaderBoundState(stateDesc);
		setShaderBoundState(newState);
	}

	void D3D12Context::RHISetComputeShader(RHIShader* shader)
	{
		if (shader)
		{
			auto newState = static_cast<D3D12System*>(GRHISystem)->getShaderBoundState(shader);
			setShaderBoundState(newState);
		}
		else
		{
			setShaderBoundState(nullptr);
		}
	}

	void D3D12Context::RHISetShaderProgram(RHIShaderProgram* shaderProgram)
	{
		if (shaderProgram == nullptr)
		{
			mBoundState = nullptr;
			return;
		}

		auto shaderProgramImpl = static_cast<D3D12ShaderProgram*>(shaderProgram);

		if (shaderProgramImpl->cachedState == nullptr)
		{
			shaderProgramImpl->cachedState = static_cast<D3D12System*>(GRHISystem)->getShaderBoundState(shaderProgramImpl);
		}

		setShaderBoundState(shaderProgramImpl->cachedState);
	}

	void D3D12Context::setShaderBoundState(D3D12ShaderBoundState* newState)
	{
		mBoundState = newState;
		if (newState)
		{
			if (newState->mShaders[0].type == EShader::Compute)
			{
				mGraphicsCmdList->SetComputeRootSignature(mBoundState->mRootSignature);
			}
			else
			{
				mGraphicsCmdList->SetGraphicsRootSignature(mBoundState->mRootSignature);
			}

			mCSUHeapUsageMask = 0;
			mSamplerHeapUsageMask = 0;
			mUsedDescHeaps.clear();

			for (auto& shaderInfo : mBoundState->mShaders)
			{
				mRootSlotStart[shaderInfo.type] = shaderInfo.rootSlotStart;
			}
		}
	}


	void D3D12Context::setShaderValueInternal(EShader::Type shaderType , D3D12ShaderData& shaderData, ShaderParameter const& param, uint8 const* pData, uint32 size)
	{
		auto const& slotInfo = shaderData.rootSignature.slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shaderType] + slotInfo.slotOffset;

		auto& resourceBoundState = mResourceBoundStates[shaderType];

		if (resourceBoundState.mCurrentUpdatedSize == 0)
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
		auto const& slotInfo = shaderData.rootSignature.slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shaderType] + slotInfo.slotOffset;

		auto& resourceBoundState = mResourceBoundStates[shaderType];
		
		if (resourceBoundState.mCurrentUpdatedSize == 0)
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
		auto const& slotInfo = shaderData.rootSignature.slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shaderType] + slotInfo.slotOffset;
		auto handle = static_cast<D3D12Texture2D&>(texture).mSRV;

		if (!(mCSUHeapUsageMask & BIT(handle.chunk->id)))
		{
			mCSUHeapUsageMask |= BIT(handle.chunk->id);
			mUsedDescHeaps.push_back(handle.chunk->resource);
			mGraphicsCmdList->SetDescriptorHeaps(mUsedDescHeaps.size(), mUsedDescHeaps.data());
		}

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
		auto const& slotInfo = shaderData.rootSignature.slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shaderType] + slotInfo.slotOffset;
		auto handle = static_cast<D3D12SamplerState&>(sampler).mHandle;

		if (!(mSamplerHeapUsageMask & BIT(handle.chunk->id)))
		{
			mSamplerHeapUsageMask |= BIT(handle.chunk->id);
			mUsedDescHeaps.push_back(handle.chunk->resource);
			mGraphicsCmdList->SetDescriptorHeaps(mUsedDescHeaps.size(), mUsedDescHeaps.data());
		}

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
		auto const& slotInfo = shaderData.rootSignature.slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shaderType] + slotInfo.slotOffset;
		D3D12PooledHeapHandle handle;
		ID3D12Resource* resource;
		switch (texture.getType())
		{
		case ETexture::Type1D: 
			handle = static_cast<D3D12Texture1D&>(texture).mUAV; 
			resource = static_cast<D3D12Texture1D&>(texture).getResource(); 
			break;
		case ETexture::Type2D:
			handle = static_cast<D3D12Texture2D&>(texture).mUAV; 
			resource = static_cast<D3D12Texture2D&>(texture).getResource(); 
			break;
		}

		mResourceBoundStates[shaderType].mUAVStates[param.bindIndex].resource = resource;

		mGraphicsCmdList->ResourceBarrier(1, &FD3D12Init::TransitionBarrier(resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0));

		if (!(mCSUHeapUsageMask & BIT(handle.chunk->id)))
		{
			mCSUHeapUsageMask |= BIT(handle.chunk->id);
			mUsedDescHeaps.push_back(handle.chunk->resource);
			mGraphicsCmdList->SetDescriptorHeaps(mUsedDescHeaps.size(), mUsedDescHeaps.data());
		}


		if (shaderType == EShader::Compute)
		{
			mGraphicsCmdList->SetComputeRootDescriptorTable(rootSlot, handle.getGPUHandle());
		}
		else
		{
			mGraphicsCmdList->SetGraphicsRootDescriptorTable(rootSlot, handle.getGPUHandle());
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
			mGraphicsCmdList->ResourceBarrier(1, &FD3D12Init::TransitionBarrier(resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, 0));
		}
	}

	void D3D12Context::setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		auto& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &buffer](EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& shaderParam)
		{
			setShaderUniformBuffer(shaderType, shaderData, shaderParam, buffer);
		});
	}

	void D3D12Context::setShaderUniformBuffer(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		auto const& slotInfo = shaderData.rootSignature.slots[param.bindIndex];
		uint32 rootSlot = mRootSlotStart[shaderType] + slotInfo.slotOffset;
		auto handle = static_cast<D3D12VertexBuffer&>(buffer).mViewHandle;

		if (!(mCSUHeapUsageMask & BIT(handle.chunk->id)))
		{
			mCSUHeapUsageMask |= BIT(handle.chunk->id);
			mUsedDescHeaps.push_back(handle.chunk->resource);
			mGraphicsCmdList->SetDescriptorHeaps(mUsedDescHeaps.size(), mUsedDescHeaps.data());
		}

		if (shaderType == EShader::Compute)
		{
			mGraphicsCmdList->SetComputeRootDescriptorTable(rootSlot, handle.getGPUHandle());
		}
		else
		{
			mGraphicsCmdList->SetGraphicsRootDescriptorTable(rootSlot, handle.getGPUHandle());
		}
	}


	void D3D12Context::setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		auto& shaderImpl = static_cast<D3D12Shader&>(shader);
		setShaderUniformBuffer(shaderImpl.getType(), shaderImpl, param, buffer);
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

	void D3D12Context::commitGraphicsPipelineState(EPrimitive type)
	{
		if (mBoundState == nullptr)
		{
			if (mInputLayoutPending)
			{
				D3D12InputLayout* inputLayoutImpl = static_cast<D3D12InputLayout*>(mInputLayoutPending.get());
				SimplePipelineProgram* program = SimplePipelineProgram::Get(inputLayoutImpl->mAttriableMask, mFixedShaderParams.texture);

				RHISetShaderProgram(program->getRHIResource());
				program->setParameters(RHICommandListImpl(*this), mFixedShaderParams);
			}
		}

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

	void D3D12Context::commitMeshPipelineState()
	{
		if (mBoundState == nullptr)
		{
			return;
		}
		GraphicsPipelineStateDesc stateDesc;

		stateDesc.primitiveType = EPrimitive::TriangleList;
		stateDesc.inputLayout = nullptr;

		stateDesc.rasterizerState = mRasterizerStatePending;
		stateDesc.depthStencilState = mDepthStencilStatePending;
		stateDesc.blendState = mBlendStateStatePending;

		auto pipelineState = static_cast<D3D12System*>(GRHISystem)->getPipelineState(stateDesc, mBoundState, mRenderTargetsState);
		if (pipelineState)
		{
			mGraphicsCmdList->SetPipelineState(pipelineState->mResource);
		}
	}

	void D3D12Context::commitComputePipelineState()
	{
		auto pipelineState = static_cast<D3D12System*>(GRHISystem)->getPipelineState(mBoundState);
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

		for (int i = 0; i < EShader::Count; ++i)
		{
			mResourceBoundStates[i].restFrame();
		}

		mIndexBufferUP.beginFrame();
		mVertexBufferUP.beginFrame();
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
		mBufferSize = ConstBufferMultipleSize * ( ( 2048 * 1024 + ConstBufferMultipleSize - 1) / ConstBufferMultipleSize );
		VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateCommittedResource(
			&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&FD3D12Init::BufferDesc(mBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mConstBuffer)));

		D3D12_RANGE readRange = {};        // We do not intend to read from this resource on the CPU.
		VERIFY_D3D_RESULT_RETURN_FALSE(mConstBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mConstDataPtr)));
		return true;
	}

	void D3D12PipelineStateKey::initialize(GraphicsRenderStateDesc const& renderState, D3D12ShaderBoundState* boundState, D3D12RenderTargetsState* renderTargetsState, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
	{
		boundStateKey = boundState->cachedKey;
		rasterizerID = renderState.rasterizerState ? renderState.rasterizerState->mGUID : TStaticRasterizerState<>::GetRHI().mGUID;
		depthStenceilID = renderState.depthStencilState ? renderState.depthStencilState->mGUID : TStaticDepthStencilState<>::GetRHI().mGUID;
		blendID = renderState.blendState ? renderState.blendState->mGUID : TStaticBlendState<>::GetRHI().mGUID;

		renderTargetFormatID = renderTargetsState->formatGUID;

		inputLayoutID = renderState.inputLayout ? renderState.inputLayout->mGUID : 0;
		topologyType = topologyType;
	}

	void D3D12PipelineStateKey::initialize(MeshRenderStateDesc const& renderState, D3D12ShaderBoundState* boundState, D3D12RenderTargetsState* renderTargetsState)
	{
		boundStateKey = boundState->cachedKey;
		rasterizerID = renderState.rasterizerState ? renderState.rasterizerState->mGUID : TStaticRasterizerState<>::GetRHI().mGUID;
		depthStenceilID = renderState.depthStencilState ? renderState.depthStencilState->mGUID : TStaticDepthStencilState<>::GetRHI().mGUID;
		blendID = renderState.blendState ? renderState.blendState->mGUID : TStaticBlendState<>::GetRHI().mGUID;

		renderTargetFormatID = renderTargetsState->formatGUID;

		inputLayoutID = 0;
		topologyType = 0;
	}

	void D3D12PipelineStateKey::initialize(D3D12ShaderBoundState* boundState)
	{
		boundStateKey = boundState->cachedKey;
		value = 0;
	}

	bool D3D12DynamicBuffer::initialize(ID3D12DeviceRHI* device, uint32 bufferSizes[], int numBuffers)
	{
		mDevice = device;
		for (int i = 0; i < numBuffers; ++i)
		{
			VERIFY_RETURN_FALSE(allocNewBuffer(bufferSizes[i]));
		}

		return true;
	}

	bool D3D12DynamicBuffer::allocNewBuffer(uint32 size)
	{
		BufferInfo info;
		info.size = (D3D12_BUFFER_SIZE_ALIGN * size + D3D12_BUFFER_SIZE_ALIGN - 1) / D3D12_BUFFER_SIZE_ALIGN;
		info.allocResource(mDevice);
		mAllocedBuffers.push_back(info);
		return true;
	}

	void* D3D12DynamicBuffer::lock(ID3D12GraphicsCommandListRHI* cmdList, uint32 size)
	{
		int index = 0;
		for (; index < mAllocedBuffers.size(); ++index)
		{
			auto const& bufferInfo = mAllocedBuffers[index];
			if (bufferInfo.size >= size)
			{
				break;
			}
		}

		if (index == mAllocedBuffers.size())
		{
			if (!allocNewBuffer(Math::Max<uint32>(size, mAllocedBuffers.back().size * 3 / 2)))
				return nullptr;
		}

		D3D12_RANGE readRange = {};
		uint8* pData = nullptr;

		mLockedResource = mAllocedBuffers[index].fetchResource(mDevice);
		if (mLockedResource == nullptr)
			return nullptr;


		D3D12_RESOURCE_BARRIER barrier = FD3D12Init::TransitionBarrier(mLockedResource, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
		//cmdList->ResourceBarrier(1, &barrier);
		VERIFY_D3D_RESULT(mLockedResource->Map(0, &readRange, reinterpret_cast<void**>(&pData)), return nullptr;);
		mAllocedBuffers[index].indexNext += 1;
		mLockedIndex = index;
		mLockedSize = size;

		return pData;
	}

	ID3D12Resource* D3D12DynamicBuffer::unlock(ID3D12GraphicsCommandListRHI* cmdList, D3D12_VERTEX_BUFFER_VIEW& bufferView)
	{
		CHECK(mLockedIndex >= 0);
		D3D12_RANGE readRange = { 0 , mLockedSize };
		mLockedResource->Unmap(0, &readRange);
		bufferView.BufferLocation = mLockedResource->GetGPUVirtualAddress();

		bufferView.SizeInBytes = mLockedSize;
		bufferView.StrideInBytes = 0;
		mLockedIndex = -1;

		D3D12_RESOURCE_BARRIER barrier = FD3D12Init::TransitionBarrier(mLockedResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
		//cmdList->ResourceBarrier(1, &barrier);

		return mLockedResource;
	}

	ID3D12Resource* D3D12DynamicBuffer::unlock(ID3D12GraphicsCommandListRHI* cmdList, D3D12_INDEX_BUFFER_VIEW& bufferView)
	{
		CHECK(mLockedIndex >= 0);
		D3D12_RANGE readRange = { 0 , mLockedSize };
		mLockedResource->Unmap(0, &readRange);
		bufferView.BufferLocation = mLockedResource->GetGPUVirtualAddress();
		bufferView.SizeInBytes = mLockedSize;
		bufferView.Format = DXGI_FORMAT_R32_UINT;
		mLockedIndex = -1;

		D3D12_RESOURCE_BARRIER barrier = FD3D12Init::TransitionBarrier(mLockedResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
		//cmdList->ResourceBarrier(1, &barrier);

		return mLockedResource;
	}

	ID3D12Resource* D3D12DynamicBuffer::BufferInfo::allocResource(ID3D12DeviceRHI* device)
	{
		ID3D12Resource* resource = nullptr;
		VERIFY_D3D_RESULT(device->CreateCommittedResource(
			&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&FD3D12Init::BufferDesc(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&resource)), return nullptr; );

		resources.push_back(resource);
		return resource;
}

}//namespace Render

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif
