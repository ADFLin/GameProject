#include "D3D12Command.h"
#include "D3D12ShaderCommon.h"

#include "LogSystem.h"
#include "GpuProfiler.h"
#include "MarcoCommon.h"

#pragma comment(lib , "DXGI.lib")

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif
#include "D3D12Utility.h"


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

		if (!mRenderContext.initialize(this))
		{
			return false;
		}
		mImmediateCommandList = new RHICommandListImpl(mRenderContext);

		return true;
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

	RHIVertexBuffer* D3D12System::RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		int bufferSize = vertexSize * numVertices;

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

		return result;
	}

	RHIInputLayout* D3D12System::RHICreateInputLayout(InputLayoutDesc const& desc)
	{
		return new D3D12InputLayout(desc);
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
		return new D3D12Shader;
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

	bool D3D12Context::initialize(D3D12System* system)
	{
		mDevice = system->mDevice;
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

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

	bool D3D12Context::configFromSwapChain(D3D12SwapChain* swapChain)
	{
		int numFrames = swapChain->mRTViews.size();

		mFrameDataList.resize(numFrames);
		mFrameIndex = swapChain->mResource->GetCurrentBackBufferIndex();
		for (int i = 0; i < numFrames; ++i)
		{
			VERIFY_RETURN_FALSE(mFrameDataList[i].init(mDevice));
			if (i != mFrameIndex)
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
		return true;
	}

	void D3D12Context::waitForGpu()
	{
		// Schedule a Signal command in the queue.
		mCommandQueue->Signal(mFence, mFrameDataList[mFrameIndex].fenceValue);

		// Wait until the fence has been processed.
		mFence->SetEventOnCompletion(mFrameDataList[mFrameIndex].fenceValue, mFenceEvent);
		WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);

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

}//namespace Render

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif
