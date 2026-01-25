#include "D3D11Command.h"

#include "D3D11ShaderCommon.h"

#include "LogSystem.h"
#include "GpuProfiler.h"

#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif
#include "BitUtility.h"
#include "RHIMisc.h"
#include "Hardware/DynamicNvAPI.h"

namespace Render
{

	EXPORT_RHI_SYSTEM_MODULE(RHISystemName::D3D11, D3D11System);


#define GRAPHIC_SHADER_LIST(op)\
	op(EShader::Vertex)\
	op(EShader::Pixel)\
	op(EShader::Geometry)\
	op(EShader::Hull)\
	op(EShader::Domain)


	template< EShader::Type >
	struct D3D11ShaderTraits {};

#define D3D11_SHADER_TRAITS( TYPE , PREFIX ) \
	template<>\
	struct D3D11ShaderTraits<TYPE>\
	{\
		static void SetConstBuffers(ID3D11DeviceContext* context, UINT startSlot, UINT numBuffers, ID3D11Buffer *const *ppConstantBuffers)\
		{\
			context->PREFIX##SetConstantBuffers(startSlot, numBuffers, ppConstantBuffers);\
		}\
		static void SetSamplers(ID3D11DeviceContext* context, UINT startSlot, UINT numSamplers, ID3D11SamplerState *const *ppSamplers)\
		{\
			context->PREFIX##SetSamplers(startSlot, numSamplers, ppSamplers);\
		}\
		static void SetShaderResources(ID3D11DeviceContext* context, UINT startSlot, UINT numViews, ID3D11ShaderResourceView *const *ppShaderResourceViews)\
		{\
			context->PREFIX##SetShaderResources(startSlot, numViews, ppShaderResourceViews);\
		}\
	};
	D3D11_SHADER_TRAITS(EShader::Vertex, VS);
	D3D11_SHADER_TRAITS(EShader::Pixel, PS);
	D3D11_SHADER_TRAITS(EShader::Geometry, GS);
	D3D11_SHADER_TRAITS(EShader::Hull, HS);
	D3D11_SHADER_TRAITS(EShader::Domain, DS);
	D3D11_SHADER_TRAITS(EShader::Compute, CS);
#undef D3D11_SHADER_TRAITS

#define RESULT_FAILED( hr ) ( hr ) != S_OK

	template< int MaskCount, typename TFunc>
	void UpdateDirtyState(uint32& inoutMask, TFunc&& func)
	{
		if (inoutMask)
		{
			uint32 mask = inoutMask;
			inoutMask = 0;
#if 1
			int index;
			int count;
			while (FBitUtility::IterateMaskRange< MaskCount >(mask, index, count))
			{
				func(index, count);
			}
#else
			int index;
			while (FBitUtility::IterateMask< MaskCount >(mask, index))
			{
				func(index, 1);
			}
#endif
		}
	}

	class D3D11ProfileCore : public RHIProfileCore
	{
	public:

		D3D11ProfileCore()
		{
			mCycleToMillisecond = 0;
		}

		bool init(TComPtr<ID3D11Device> const& device,
			      TComPtr<ID3D11DeviceContext> const& deviceContextImmdiate,
				  TComPtr<ID3D11DeviceContext> const& deviceContext)
		{
			mDevice = device;
			mDeviceContext = deviceContext;
			mDeviceContextImmdiate = deviceContextImmdiate;

			bDeferredContext = mDeviceContext->GetType() == D3D11_DEVICE_CONTEXT_DEFERRED;

			D3D11_QUERY_DESC desc;
			desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
			desc.MiscFlags = 0;
			VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateQuery(&desc, &mQueryDisjoint));

			return true;
		}
		virtual void beginFrame() override
		{
			mDeviceContextImmdiate->Begin(mQueryDisjoint);
		}

		virtual bool endFrame() override
		{
			mDeviceContextImmdiate->End(mQueryDisjoint);

			static constexpr int QUERY_TIMEOUT_MS = 1000;
			static constexpr int SLEEP_INTERVAL_MS = 1;
			int count = 0;
			int maxRetries = QUERY_TIMEOUT_MS / SLEEP_INTERVAL_MS;
			while (mDeviceContextImmdiate->GetData(mQueryDisjoint, NULL, 0, 0) == S_FALSE)
			{
				if (++count > maxRetries)
				{
					LogWarning(0, "GPU query timeout after %dms", QUERY_TIMEOUT_MS);
					return false;
				}
				SystemPlatform::Sleep(SLEEP_INTERVAL_MS);
			}

			D3D10_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
			mDeviceContextImmdiate->GetData(mQueryDisjoint, &tsDisjoint, sizeof(tsDisjoint), 0);
			if (tsDisjoint.Disjoint)
			{
				return false;
			}
			tsDisjoint.Frequency;	
			return true;
		}

		virtual uint32 fetchTiming() override
		{
			D3D11_QUERY_DESC desc;
			desc.Query = D3D11_QUERY_TIMESTAMP;
			desc.MiscFlags = 0;
			//if (bDeferredContext)
			{
				TComPtr< ID3D11Query > startQuery;
				VERIFY_D3D_RESULT(mDevice->CreateQuery(&desc, &startQuery), return RHI_ERROR_PROFILE_HANDLE;);
				TComPtr< ID3D11Query > endQuery;
				VERIFY_D3D_RESULT(mDevice->CreateQuery(&desc, &endQuery), return RHI_ERROR_PROFILE_HANDLE;);

				uint32 result = mSamples.size();
				mSamples.emplace_back(std::move(startQuery), std::move(endQuery));
				return result;
			}
		}

		virtual void startTiming(uint32 timingHandle) override
		{
			CHECK(timingHandle < mSamples.size());
			mDeviceContext->End(mSamples[timingHandle].startQuery);
		}

		virtual void endTiming(uint32 timingHandle) override
		{
			CHECK(timingHandle < mSamples.size());
			mDeviceContext->End(mSamples[timingHandle].endQuery);
		}

		virtual bool getTimingDuration(uint32 timingHandle, uint64& outDuration) override
		{
			if (timingHandle >= mSamples.size())
			{
				LogError(0, "Invalid timing handle: %u (max: %zu)", timingHandle, mSamples.size());
				return false;
			}

			Sample& sample = mSamples[timingHandle];
			VERIFY_D3D_RESULT_RETURN_FALSE(mDeviceContextImmdiate->GetData(sample.startQuery, NULL, 0, 0));
			VERIFY_D3D_RESULT_RETURN_FALSE(mDeviceContextImmdiate->GetData(sample.endQuery, NULL, 0, 0));

			UINT64 startData;
			mDeviceContextImmdiate->GetData(sample.startQuery, &startData, sizeof(startData), 0);
			UINT64 endData;
			mDeviceContextImmdiate->GetData(sample.endQuery, &endData, sizeof(endData), 0);
			outDuration = endData - startData;
			return true;
		}

		virtual double getCycleToMillisecond() override
		{
			if( mCycleToMillisecond == 0 )
			{
				TComPtr< ID3D11Query > queryDisjoint;
				D3D11_QUERY_DESC desc;
				desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
				desc.MiscFlags = 0;
				VERIFY_D3D_RESULT(mDevice->CreateQuery(&desc, &queryDisjoint), return 0.0;);
				mDeviceContextImmdiate->Begin(queryDisjoint);
				mDeviceContextImmdiate->End(queryDisjoint);

				int retryCount = 0;
				static constexpr int MAX_RETRIES = 1000;
				while (RESULT_FAILED(mDeviceContextImmdiate->GetData(queryDisjoint, NULL, 0, 0)))
				{
					if (++retryCount > MAX_RETRIES)
					{
						LogError(0, "Failed to get GPU frequency after %d retries", MAX_RETRIES);
						return 0.0;
					}
					SystemPlatform::Sleep(1);
				}

				D3D10_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
				mDeviceContextImmdiate->GetData(queryDisjoint, &tsDisjoint, sizeof(tsDisjoint), 0);
				mCycleToMillisecond = double(1000) / tsDisjoint.Frequency;
			}

			return mCycleToMillisecond;
		}	

		virtual void releaseRHI() override
		{
			mQueryDisjoint.reset();
			mSamples.clear();
			mDeviceContext.reset();
			mDeviceContextImmdiate.reset();
			mDevice.reset();
		}
		double mCycleToMillisecond;

		TComPtr< ID3D11Query > mQueryDisjoint;

		struct Sample
		{
			TComPtr< ID3D11Query > startQuery;
			TComPtr< ID3D11Query > endQuery;

			Sample(TComPtr< ID3D11Query >&& startQuery, TComPtr< ID3D11Query >&& endQuery)
				:startQuery(startQuery), endQuery(endQuery)
			{
			}
		};
		TArray< Sample > mSamples;


		bool bDeferredContext;
		TComPtr<ID3D11DeviceContext> mDeviceContext; 
		TComPtr<ID3D11DeviceContext> mDeviceContextImmdiate;
		TComPtr<ID3D11Device> mDevice;
	};





	using MultiDrawInstancedIndirectFunc = void(*)(
		__in ID3D11DeviceContext *pDevContext11,
		__in NvU32                drawCount,
		__in ID3D11Buffer        *pBuffer,
		__in NvU32                alignedByteOffsetForArgs,
		__in NvU32                alignedByteStrideForArgs
		);

	using MultiDrawIndexedInstancedIndirectFunc = void(*)(
		__in ID3D11DeviceContext *pDevContext11,
		__in NvU32                drawCount,
		__in ID3D11Buffer        *pBuffer,
		__in NvU32                alignedByteOffsetForArgs,
		__in NvU32                alignedByteStrideForArgs
		);

	static MultiDrawInstancedIndirectFunc GMultiDrawInstancedIndirect = nullptr;
	static MultiDrawIndexedInstancedIndirectFunc GMultiDrawIndexedInstancedIndirect = nullptr;


	class DeviceVendorFeature
	{
	public:
		virtual ~DeviceVendorFeature() = default;
		virtual bool initialize() { return true; }
		virtual void finalize() {}
	};


	DeviceVendorFeature* GDeviceVendorFeature = nullptr;
	static NvAPI_Status(__cdecl *NvAPI_D3D11_MultiDrawInstancedIndirect)(
		__in ID3D11DeviceContext *pDevContext11,
		__in NvU32                drawCount,
		__in ID3D11Buffer        *pBuffer,
		__in NvU32                alignedByteOffsetForArgs,
		__in NvU32                alignedByteStrideForArgs);

	static NvAPI_Status(__cdecl *NvAPI_D3D11_MultiDrawIndexedInstancedIndirect)(
		__in ID3D11DeviceContext *pDevContext11,
		__in NvU32                drawCount,
		__in ID3D11Buffer        *pBuffer,
		__in NvU32                alignedByteOffsetForArgs,
		__in NvU32                alignedByteStrideForArgs);

	class DeviceVendorFeature_NVIDIA : public DeviceVendorFeature
									 , public DyanmicNvAPI
	{
	public:

		bool initialize()
		{
			DyanmicNvAPI::initialize();
#define QUERY_INTERFACE( NAME )\
	if ( !queryInterface(ID_##NAME , NAME) )\
		return false;

			QUERY_INTERFACE(NvAPI_D3D11_MultiDrawInstancedIndirect);
			QUERY_INTERFACE(NvAPI_D3D11_MultiDrawIndexedInstancedIndirect);
#undef QUERY_INTERFACE

			GMultiDrawInstancedIndirect = MultiDrawInstancedIndirect;
			GMultiDrawIndexedInstancedIndirect = MultiDrawIndexedInstancedIndirect;
			return true;
		}

		void finalize()
		{
			GMultiDrawInstancedIndirect = nullptr;
			GMultiDrawIndexedInstancedIndirect = nullptr;
			DyanmicNvAPI::release();
		}

		static void MultiDrawInstancedIndirect(
			__in ID3D11DeviceContext *pDevContext11,
			__in NvU32                drawCount,
			__in ID3D11Buffer        *pBuffer,
			__in NvU32                alignedByteOffsetForArgs,
			__in NvU32                alignedByteStrideForArgs)
		{
			NvAPI_D3D11_MultiDrawInstancedIndirect(pDevContext11, drawCount, pBuffer, alignedByteOffsetForArgs, alignedByteStrideForArgs);
		}

		static void MultiDrawIndexedInstancedIndirect(
			__in ID3D11DeviceContext *pDevContext11,
			__in NvU32                drawCount,
			__in ID3D11Buffer        *pBuffer,
			__in NvU32                alignedByteOffsetForArgs,
			__in NvU32                alignedByteStrideForArgs)
		{
			NvAPI_D3D11_MultiDrawIndexedInstancedIndirect(pDevContext11, drawCount, pBuffer, alignedByteOffsetForArgs, alignedByteStrideForArgs);
		}
	};


	bool D3D11System::initialize(RHISystemInitParams const& initParam)
	{
		uint32 deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
		if ( initParam.bDebugMode )
			deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	
		VERIFY_D3D_RESULT_RETURN_FALSE(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, NULL, 0, D3D11_SDK_VERSION, &mDevice, NULL, &mDeviceContextImmdiate));


		D3D11_FEATURE_DATA_THREADING featureData;
		mDevice->CheckFeatureSupport(D3D11_FEATURE_THREADING, &featureData, sizeof(featureData));

		if (initParam.bMultithreadingSupported)
		{
			mDevice->CreateDeferredContext(0, &mDeviceContext);
		}
		else
		{
			mDeviceContext = mDeviceContextImmdiate;
		}
		D3D11_FEATURE_DATA_D3D11_OPTIONS3 featureOption3;
		mDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &featureOption3, sizeof(featureOption3));
		if (featureOption3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer)
		{
			GRHISupportVPAndRTArrayIndexFromAnyShaderFeedingRasterizer = true;
		}

		TComPtr< IDXGIDevice > pDXGIDevice;
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice));
		TComPtr< IDXGIAdapter > pDXGIAdapter;
		VERIFY_D3D_RESULT_RETURN_FALSE(pDXGIDevice->GetAdapter(&pDXGIAdapter));

		DXGI_ADAPTER_DESC adapterDesc;
		VERIFY_D3D_RESULT_RETURN_FALSE(pDXGIAdapter->GetDesc(&adapterDesc));

		GRHIDeviceVendorName = FD3DUtils::GetDeivceVenderName(adapterDesc.VendorId);
		if (GRHIDeviceVendorName == DeviceVendorName::NVIDIA)
		{
			GDeviceVendorFeature = new DeviceVendorFeature_NVIDIA;
		}

		if (GDeviceVendorFeature && !GDeviceVendorFeature->initialize())
		{
			delete GDeviceVendorFeature;
			GDeviceVendorFeature = nullptr;
		}

		mbVSyncEnable = initParam.bVSyncEnable;

		GRHIClipZMin = 0;
		GRHIProjectionYSign = 1;
		GRHIViewportOrgToNDCPosY = 1;

		mRenderContext.initialize(mDevice, mDeviceContext);
		mImmediateCommandList = new RHICommandListImpl(mRenderContext);
#if 1
		D3D11ProfileCore* profileCore = new D3D11ProfileCore;
		if( profileCore->init(mDevice, mDeviceContextImmdiate, mDeviceContext) )
		{
			GpuProfiler::Get().setCore(profileCore);
		}
		else
		{
			delete profileCore;
		}
#endif

		return true;
	}

	void D3D11System::clearResourceReference()
	{
		mDeviceContextImmdiate->ClearState();
	}

	void D3D11System::shutdown()
	{
		if (GDeviceVendorFeature)
		{
			GDeviceVendorFeature->finalize();
			delete GDeviceVendorFeature;
			GDeviceVendorFeature = nullptr;
		}

		mInputLayoutMap.clear();
		mRenderContext.release();
		mSwapChain.release();

#if _DEBUG
		ID3D11Debug *d3dDebug;
		HRESULT hr = mDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&d3dDebug));
		if (SUCCEEDED(hr))
		{
			hr = d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
			d3dDebug->Release();
		}
#endif

		mDevice.reset();
	}

	ShaderFormat* D3D11System::createShaderFormat()
	{
		return new ShaderFormatHLSL(mDevice);
	}

	bool D3D11System::RHIBeginRender()
	{
		mRenderContext.markRenderStateDirty();
		return true;
	}

	void D3D11System::RHIEndRender(bool bPresent)
	{
		if (mDeviceContextImmdiate.get() != mDeviceContext.get())
		{
			Mutex::Locker locker(mMutexContext);
			ID3D11CommandList* pd3dCommandList = NULL;
			HRESULT hr;
			hr = mDeviceContext->FinishCommandList(FALSE, &pd3dCommandList);
			mDeviceContext->Flush();
			mDeviceContextImmdiate->ExecuteCommandList(pd3dCommandList, FALSE);
			mDeviceContextImmdiate->Flush();
		}

		if (bPresent && mSwapChain)
		{
			mSwapChain->present(mbVSyncEnable);
		}
	}

	bool CreateResourceView(ID3D11Device* device, DXGI_FORMAT format, int numSamples, uint32 creationFlags, Texture2DCreationResult& outResult)
	{
		if (creationFlags & TCF_CreateSRV)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
			desc.Format = D3D11Translate::ToSRV(format);
			desc.ViewDimension = (numSamples > 1 ) ? D3D_SRV_DIMENSION_TEXTURE2DMS : D3D_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipLevels = -1;
			desc.Texture2D.MostDetailedMip = 0;
			VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateShaderResourceView(outResult.resource, &desc, &outResult.SRV));
		}
		if (creationFlags & TCF_CreateUAV)
		{
			VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateUnorderedAccessView(outResult.resource, nullptr, &outResult.UAV));
		}
		return true;
	}

	RHISwapChain* D3D11System::RHICreateSwapChain(SwapChainCreationInfo const& info)
	{
		HRESULT hr;
		TComPtr<IDXGIFactory> factory;
		hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));

		DXGI_SWAP_CHAIN_DESC swapChainDesc; 
		ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
		swapChainDesc.OutputWindow = info.windowHandle;
		swapChainDesc.Windowed = info.bWindowed;

		DXGI_FORMAT DXColorFormat = D3D11Translate::To(info.colorForamt);
		UINT quality = 0;
		if (info.numSamples > 1)
		{
			hr = mDevice->CheckMultisampleQualityLevels(DXColorFormat, info.numSamples, &quality);
			quality = quality - 1;
		}

		swapChainDesc.SampleDesc.Count = info.numSamples;
		swapChainDesc.SampleDesc.Quality = quality;
		swapChainDesc.BufferDesc.Format = DXColorFormat;
		swapChainDesc.BufferDesc.Width = info.extent.x;
		swapChainDesc.BufferDesc.Height = info.extent.y;
		swapChainDesc.BufferCount = info.bufferCount;
		swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
		if (info.textureCreationFlags & TCF_CreateSRV)
		{
			swapChainDesc.BufferUsage |= DXGI_USAGE_SHADER_INPUT;
		}
		swapChainDesc.SwapEffect = info.numSamples > 1 ? DXGI_SWAP_EFFECT_DISCARD : DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc.Flags = 0;
		if (info.textureCreationFlags & TCF_PlatformGraphicsCompatible)
		{
			if (info.numSamples == 1)
			{
				//swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;
			}
		}

		TComPtr<IDXGISwapChain> swapChainResource;
		VERIFY_D3D_RESULT(factory->CreateSwapChain(mDevice, &swapChainDesc, &swapChainResource), return nullptr;);

		int count = swapChainResource.getRefCount();

		Texture2DCreationResult textureCreationResult;
		VERIFY_D3D_RESULT(swapChainResource->GetBuffer(0, IID_PPV_ARGS(&textureCreationResult.resource)), return nullptr;);
		CreateResourceView(mDevice, DXColorFormat, info.numSamples, info.textureCreationFlags, textureCreationResult);

		TextureDesc colorDesc = TextureDesc::Type2D(info.colorForamt, info.extent.x, info.extent.y).Samples(info.numSamples).Flags(info.textureCreationFlags | TCF_RenderTarget);
		TRefCountPtr< D3D11Texture2D > colorTexture = new D3D11Texture2D(colorDesc, textureCreationResult);
		TRefCountPtr< D3D11Texture2D > depthTexture;
		if (info.bCreateDepth)
		{
			TextureDesc depthDesc = TextureDesc::Type2D(info.depthFormat, info.extent.x, info.extent.y).Samples(info.numSamples).Flags(info.depthCreateionFlags | TCF_RenderTarget);
			depthTexture = (D3D11Texture2D*)RHICreateTexture2D(depthDesc, nullptr, 0);
		}
		D3D11SwapChain* swapChain = new D3D11SwapChain(swapChainResource, *colorTexture, depthTexture);

		mSwapChain = swapChain;
		return swapChain;
	}

	RHITexture1D* D3D11System::RHICreateTexture1D(TextureDesc const& desc, void* data)
	{
		Texture1DCreationResult creationResult;
		if (createTexture1DInternal(desc, data, creationResult))
		{
			return new D3D11Texture1D(desc, creationResult);
		}
		return nullptr;
	}

	RHITexture2D* D3D11System::RHICreateTexture2D(TextureDesc const& desc, void* data, int dataAlign)
	{
		Texture2DCreationResult creationResult;
		if( createTexture2DInternal(desc, data, dataAlign, creationResult) )
		{
			return new D3D11Texture2D(desc, creationResult);
		}
		return nullptr;
	}

	RHITexture3D* D3D11System::RHICreateTexture3D(TextureDesc const& desc, void* data)
	{
		Texture3DCreationResult creationResult;
		if (createTexture3DInternal(desc, data, creationResult))
		{
			return new D3D11Texture3D(desc, creationResult);
		}
		return nullptr;
	}

	RHITexture2DArray* D3D11System::RHICreateTexture2DArray(TextureDesc const& desc, void* data)
	{
		return nullptr;
	}

	RHITextureCube* D3D11System::RHICreateTextureCube(TextureDesc const& desc, void* data[])
	{
		TextureCubeCreationResult creationResult;
		if (createTextureCubeInternal(desc, data, creationResult))
		{
			return new D3D11TextureCube(desc, creationResult);
		}
		return nullptr;
	}

	bool CreateResourceView(ID3D11Device* device, int elementSize, int numElements, uint32 creationFlags, D3D11BufferCreationResult& outResult)
	{
		if (creationFlags & BCF_CreateSRV)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			desc.Buffer.FirstElement = 0;
			desc.Buffer.NumElements = numElements;
			HRESULT hr = device->CreateShaderResourceView(outResult.resource, &desc, &outResult.SRV);
			if (hr != S_OK)
			{
				LogWarning(0, "Can't Create buffer's SRV ! error code : %d", hr);
			}
		}
		if (creationFlags & BCF_CreateUAV)
		{
#if 0
			D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
			desc.Buffer.FirstElement = 0;
			desc.Buffer.NumElements =
#endif
			device->CreateUnorderedAccessView(outResult.resource, NULL, &outResult.UAV);
		}
		return true;
	}

	RHIBuffer* D3D11System::RHICreateBuffer(BufferDesc const& desc, void* data)
	{

		D3D11_SUBRESOURCE_DATA initData = { 0 };
		initData.pSysMem = data;
		initData.SysMemPitch = 0;
		initData.SysMemSlicePitch = 0;

		D3D11_BUFFER_DESC bufferDesc = { 0 };
		bufferDesc.ByteWidth = desc.getSize();
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		if (desc.creationFlags & BCF_CpuAccessWrite)
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		//if (creationFlags & BCF_CpuAccessRead)
		//	bufferDesc.Usage = D3D11_USAGE_STAGING;
		bufferDesc.MiscFlags = 0;
		{
			if (desc.creationFlags & BCF_CreateSRV)
				bufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

			if (desc.creationFlags & BCF_CreateUAV)
				bufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
		}

		if (desc.creationFlags & BCF_UsageVertex)
		{
			bufferDesc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
		}
		if (desc.creationFlags & BCF_UsageIndex)
		{
			bufferDesc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
		}
		if (desc.creationFlags & BCF_UsageConst)
		{
			bufferDesc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
		}
		if (desc.creationFlags & BCF_Structured)
		{
			bufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
			bufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			bufferDesc.StructureByteStride = desc.elementSize;
		}
		if (desc.creationFlags & BCF_DrawIndirectArgs)
		{
			bufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
		}

		if (desc.creationFlags & BCF_CpuAccessWrite)
			bufferDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
		if (desc.creationFlags & BCF_CpuAccessRead)
			bufferDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;

		D3D11BufferCreationResult creationResult;
		TComPtr<ID3D11Buffer> BufferResource;
		VERIFY_D3D_RESULT(
			mDevice->CreateBuffer(&bufferDesc, data ? &initData : nullptr, &creationResult.resource),
			{
				return nullptr;
			}
		);

		CreateResourceView(mDevice, desc.elementSize, desc.numElements, desc.creationFlags, creationResult);
		
		D3D11Buffer* buffer = new D3D11Buffer(desc, creationResult);

		return buffer;
	}

	void* D3D11System::lockBufferInternal(ID3D11Resource* resource, ELockAccess access, uint32 offset, uint32 size)
	{
		D3D11_MAPPED_SUBRESOURCE mappedData;
		HRESULT hr = mDeviceContextImmdiate->Map(resource, 0, D3D11Translate::To(access), 0, &mappedData);
		if( hr != S_OK )
		{
			return nullptr;
		}
		return (uint8*)mappedData.pData + offset;
	}

	void* D3D11System::RHILockBuffer(RHIBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		return lockBufferInternal(D3D11Cast::GetResource(*buffer), access, offset, size);
	}

	void D3D11System::RHIUnlockBuffer(RHIBuffer* buffer)
	{
		mDeviceContextImmdiate->Unmap(D3D11Cast::GetResource(*buffer), 0);
	}

	static void ReadMappedData(ID3D11DeviceContext* context, ID3D11Resource* resource, UINT subresource, int rowSize, int height, uint8* outData)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = context->Map(resource, subresource, D3D11_MAP_READ, 0, &mappedResource);
		if (hr != S_OK)
			return;

		if (mappedResource.RowPitch == rowSize)
		{
			FMemory::Copy(outData, mappedResource.pData, rowSize * height);
		}
		else
		{
			uint8* pData = outData;
			uint8 const* pCopyData = (uint8*)mappedResource.pData;
			for (int i = 0; i < height; ++i)
			{
				FMemory::Copy(pData, pCopyData, rowSize);
				pData += rowSize;
				pCopyData += mappedResource.RowPitch;
			}
		}
		context->Unmap(resource, subresource);
	}

	void D3D11System::RHIReadTexture(RHITexture2D& texture, ETexture::Format format, int level, TArray< uint8 >& outData)
	{
		int formatSize = ETexture::GetFormatSize(format);

		int dataRowSize = Math::Max(texture.getSizeX() >> level, 1) * formatSize;
		int dataHeight = Math::Max(texture.getSizeY() >> level, 1);
		int dataSize = dataRowSize * dataHeight;

		outData.clear();
		outData.addUninitialized(dataSize);

		if (texture.getDesc().creationFlags & TCF_AllowCPUAccess)
		{
			ReadMappedData(mDeviceContextImmdiate, D3D11Cast::GetResource(texture), level, dataRowSize, dataHeight, outData.data());
		}
		else
		{
			TComPtr<ID3D11Texture2D> stagingTexture;
			if (!createStagingTexture(D3D11Cast::GetResource(texture), stagingTexture, level))
			{
				return;
			}
			mDeviceContextImmdiate->CopySubresourceRegion(stagingTexture, 0, 0, 0, 0, D3D11Cast::GetResource(texture), level, nullptr);
			ReadMappedData(mDeviceContextImmdiate, stagingTexture, 0, dataRowSize, dataHeight, outData.data());
		}
	}

	void D3D11System::RHIReadTexture(RHITextureCube& texture, ETexture::Format format, int level, TArray< uint8 >& outData)
	{
		int formatSize = ETexture::GetFormatSize(format);
		int textureSize = Math::Max(texture.getSize() >> level, 1);
		int dataRowSize = textureSize * formatSize;
		int faceDataSize = dataRowSize * textureSize;

		outData.clear();
		outData.addUninitialized(ETexture::FaceCount * faceDataSize);

		bool bAllowCPUAccess = !!(texture.getDesc().creationFlags & TCF_AllowCPUAccess);
		
		TComPtr<ID3D11Texture2D> stagingTexture;
		if (!bAllowCPUAccess)
		{
			if (!createStagingTexture(D3D11Cast::GetResource(texture), stagingTexture, level))
				return;
		}

		uint8* pData = outData.data();
		for (int face = 0; face < ETexture::FaceCount; ++face)
		{
			UINT subresource = D3D11CalcSubresource(level, face, texture.getNumMipLevel());

			if (bAllowCPUAccess)
			{
				ReadMappedData(mDeviceContextImmdiate, D3D11Cast::GetResource(texture), subresource, dataRowSize, textureSize, pData);
			}
			else
			{
				mDeviceContextImmdiate->CopySubresourceRegion(stagingTexture, 0, 0, 0, 0, D3D11Cast::GetResource(texture), subresource, nullptr);
				ReadMappedData(mDeviceContextImmdiate, stagingTexture, 0, dataRowSize, textureSize, pData);
			}
			pData += faceDataSize;
		}
	}

	bool D3D11System::RHIUpdateTexture(RHITexture2D& texture, int ox, int oy, int w, int h, void* data, int level, int dataWidth)
	{
		Mutex::Locker locker(mMutexContext);

		auto& textureImpl = static_cast<D3D11Texture2D&>(texture);

		D3D11_BOX box;
		box.front = 0;
		box.back = 1;
		box.left = ox;
		box.right = ox + w;
		box.top = oy;
		box.bottom = oy + h;
		if ( dataWidth == 0)
		{
			dataWidth = w;
		}
		
		mDeviceContextImmdiate->UpdateSubresource(textureImpl.mResource, level, &box, data, dataWidth * ETexture::GetFormatSize(texture.getFormat()), dataWidth * h * ETexture::GetFormatSize(texture.getFormat()));
		return true;
	}

	void D3D11System::RHIUpdateBuffer(RHIBuffer& buffer, int start, int numElements, void* data)
	{
		auto& bufferImpl = static_cast<D3D11Buffer&>(buffer);
		if ( bufferImpl.isDynamic() )
		{
			void* dstData = RHILockBuffer(&buffer, ELockAccess::WriteDiscard, start * buffer.getElementSize(), numElements * buffer.getElementSize());
			if (dstData )
			{
				FMemory::Copy(dstData, data , numElements * buffer.getElementSize());
				RHIUnlockBuffer(&buffer);
			}
			return;
		}
		bufferImpl.updateData(*mDeviceContextImmdiate, start, numElements, data);
	}

	void D3D11System::RHIGenerateMips(RHITextureBase& texture)
	{
		if (texture.getType() == ETexture::Type2D)
		{
			auto& textureImpl = static_cast<D3D11Texture2D&>(texture);

			if (textureImpl.mSRV.mResource)
			{
				mDeviceContextImmdiate->GenerateMips(textureImpl.mSRV.mResource);
			}
		}
	}

	RHIFrameBuffer* D3D11System::RHICreateFrameBuffer()
	{
		return new D3D11FrameBuffer;
	}

	RHIInputLayout* D3D11System::RHICreateInputLayout(InputLayoutDesc const& desc)
	{
		InputLayoutKey key(desc);
		auto iter = mInputLayoutMap.find(key);
		if( iter != mInputLayoutMap.end() )
		{
			return iter->second;
		}

		InlineString<512> fakeCode;
		{
			char const* FakeCodeTemplate = CODE_STRING(
				struct VSInput
				{
					%s
				};
				void MainVS(in VSInput input, out float4 svPosition : SV_POSITION)
				{
					svPosition = float4(0, 0, 0, 1);
				}
			);

			std::string vertexCode;
			TArray< InputElementDesc const* > sortedElements;
			for (auto const& e : key.elements)
			{
				if (e.attribute == EVertex::ATTRIBUTE_UNUSED)
					continue;

				sortedElements.push_back(&e);
			}
			
			std::sort(sortedElements.begin() , sortedElements.end(),
				[](auto lhs, auto rhs)
				{
					return lhs->attribute < rhs->attribute;
				}
			);
			for (auto e : sortedElements)
			{
				InlineString< 128 > str;
				str.format("float%d v%d : ATTRIBUTE%d;", EVertex::GetComponentNum(e->format), e->attribute, e->attribute);
				vertexCode += str.c_str();
			}

			fakeCode.format(FakeCodeTemplate, vertexCode.c_str());
		}

		TComPtr< ID3DBlob > errorCode;
		TComPtr< ID3DBlob > byteCode;
		InlineString<32> profileName = FD3D11Utility::GetShaderProfile(mDevice->GetFeatureLevel(), EShader::Vertex);

		uint32 compileFlag = 0 /*| D3D10_SHADER_PACK_MATRIX_ROW_MAJOR*/;
		VERIFY_D3D_RESULT(
			D3DCompile(fakeCode, FCString::Strlen(fakeCode), "ShaderCode", NULL, NULL, SHADER_ENTRY(MainVS), profileName, compileFlag, 0, &byteCode, &errorCode),
			{
				LogWarning(0, "Compile Error %s", errorCode->GetBufferPointer());
				return nullptr;
			}
		);

		D3D11InputLayout* inputLayout = new D3D11InputLayout;
		inputLayout->initialize(desc);

		TComPtr< ID3D11InputLayout > inputLayoutResource;
		VERIFY_D3D_RESULT(
			mDevice->CreateInputLayout(inputLayout->mDescList.data(), inputLayout->mDescList.size(), byteCode->GetBufferPointer(), byteCode->GetBufferSize(), &inputLayoutResource),
			return nullptr;
		);
		inputLayout->mUniversalResource = inputLayoutResource.detach();
		mInputLayoutMap.insert( std::make_pair(key, inputLayout) );
		return inputLayout;
	}

	RHISamplerState* D3D11System::RHICreateSamplerState(SamplerStateInitializer const& initializer)
	{
		D3D11_SAMPLER_DESC desc = {};
		desc.Filter   = D3D11Translate::To(initializer.filter);
		desc.AddressU = D3D11Translate::To(initializer.addressU);
		desc.AddressV = D3D11Translate::To(initializer.addressV);
		desc.AddressW = D3D11Translate::To(initializer.addressW);
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		desc.MaxAnisotropy = 16;

		TComPtr<ID3D11SamplerState> samplerResource;
		VERIFY_D3D_RESULT( mDevice->CreateSamplerState(&desc , &samplerResource) , );
		if( samplerResource )
		{
			return new D3D11SamplerState(samplerResource.detach());
		}
		return nullptr;
	}

	RHIRasterizerState* D3D11System::RHICreateRasterizerState(RasterizerStateInitializer const& initializer)
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11Translate::To(initializer.fillMode);
		desc.CullMode = D3D11Translate::To(initializer.cullMode);
		desc.FrontCounterClockwise = initializer.frontFace != EFrontFace::Default;
		desc.DepthBias = Math::FloorToInt(initializer.depthBias * (float)(1 << 24)); 
		desc.DepthBiasClamp = 0;
		desc.SlopeScaledDepthBias = initializer.slopeScaleDepthBias;
		desc.DepthClipEnable = FALSE;
		desc.ScissorEnable = initializer.bEnableScissor;
		desc.MultisampleEnable = initializer.bEnableMultisample;
		desc.AntialiasedLineEnable = initializer.bEnableMultisample;

		TComPtr<ID3D11RasterizerState> stateResource;
		VERIFY_D3D_RESULT(mDevice->CreateRasterizerState(&desc, &stateResource) , );
		if( stateResource )
		{
			return new D3D11RasterizerState(stateResource.detach());
		}
		return nullptr;
	}

	RHIBlendState* D3D11System::RHICreateBlendState(BlendStateInitializer const& initializer)
	{
	
		D3D11_BLEND_DESC desc = { 0 };
		desc.AlphaToCoverageEnable = initializer.bEnableAlphaToCoverage;
		desc.IndependentBlendEnable = initializer.bEnableIndependent;
		for( int i = 0; i < ( initializer.bEnableIndependent ? MaxBlendStateTargetCount : 1 ); ++i )
		{
			auto const& targetValue = initializer.targetValues[i];
			auto& targetValueD3D11 = desc.RenderTarget[i];

			if( targetValue.writeMask & CWM_R )
				targetValueD3D11.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_RED;
			if( targetValue.writeMask & CWM_G )
				targetValueD3D11.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
			if( targetValue.writeMask & CWM_B )
				targetValueD3D11.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
			if( targetValue.writeMask & CWM_A )
				targetValueD3D11.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;

			targetValueD3D11.BlendEnable = targetValue.isEnabled();
			targetValueD3D11.BlendOp = D3D11Translate::To(targetValue.op);
			targetValueD3D11.SrcBlend = D3D11Translate::To(targetValue.srcColor);
			targetValueD3D11.DestBlend = D3D11Translate::To(targetValue.destColor);
			targetValueD3D11.BlendOpAlpha = D3D11Translate::To(targetValue.opAlpha);
			targetValueD3D11.SrcBlendAlpha = D3D11Translate::To(targetValue.srcAlpha);
			targetValueD3D11.DestBlendAlpha = D3D11Translate::To(targetValue.destAlpha);
		}

		TComPtr< ID3D11BlendState > stateResource;
		VERIFY_D3D_RESULT(mDevice->CreateBlendState(&desc, &stateResource), );
		if( stateResource )
		{
			return new D3D11BlendState(stateResource.detach());
		}
		return nullptr;
	}

	RHIDepthStencilState* D3D11System::RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer)
	{
		D3D11_DEPTH_STENCIL_DESC desc;
		desc.DepthEnable = initializer.isDepthEnable();
		desc.DepthWriteMask = initializer.bWriteDepth ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = D3D11Translate::To( initializer.depthFunc );
		desc.StencilEnable = initializer.bEnableStencil;
		desc.StencilReadMask = initializer.stencilReadMask;
		desc.StencilWriteMask = initializer.stencilWriteMask;

		desc.FrontFace.StencilFunc = D3D11Translate::To(initializer.stencilFunc);
		desc.FrontFace.StencilPassOp = D3D11Translate::To(initializer.zPassOp);
		desc.FrontFace.StencilDepthFailOp = D3D11Translate::To( initializer.zFailOp );
		desc.FrontFace.StencilFailOp = D3D11Translate::To( initializer.stencilFailOp );
	
		desc.BackFace.StencilFunc = D3D11Translate::To(initializer.stencilFuncBack);
		desc.BackFace.StencilPassOp = D3D11Translate::To(initializer.zPassOpBack);
		desc.BackFace.StencilDepthFailOp = D3D11Translate::To(initializer.zFailOpBack);
		desc.BackFace.StencilFailOp = D3D11Translate::To(initializer.stencilFailOpBack);
		
		TComPtr< ID3D11DepthStencilState > stateResource;
		VERIFY_D3D_RESULT(mDevice->CreateDepthStencilState(&desc, &stateResource), );
		if( stateResource )
		{
			return new D3D11DepthStencilState(stateResource.detach());
		}
		return nullptr;
	}

	RHIShader* D3D11System::RHICreateShader(EShader::Type type)
	{
		if (type == EShader::Vertex)
			return new D3D11VertexShader();

		return new D3D11Shader(type);
	}

	RHIShaderProgram* D3D11System::RHICreateShaderProgram()
	{
		return new D3D11ShaderProgram;
	}


	template< class TD3D11_TEXTURE_DESC >
	void SetupTextureDesc(TD3D11_TEXTURE_DESC& desc, uint32 creationFlags, bool bDepthFormat = false)
	{
		desc.Usage = (creationFlags & TCF_AllowCPUAccess) ? D3D11_USAGE_DEFAULT : D3D11_USAGE_DEFAULT;

		if (bDepthFormat)
		{
			desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
		}
		else if (creationFlags & TCF_RenderTarget)
		{
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		}
			
		if (creationFlags & TCF_CreateSRV)
		{
			desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
#if 1
			switch (desc.Format)
			{
			case DXGI_FORMAT_D16_UNORM:
				desc.Format = DXGI_FORMAT_R16_TYPELESS;
				break;
			case DXGI_FORMAT_D32_FLOAT:
				desc.Format = DXGI_FORMAT_R32_TYPELESS;
				break;
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
				desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
				break;
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
				desc.Format = DXGI_FORMAT_R32G8X24_TYPELESS;
				break;
			}
#endif
		}
		if (creationFlags & TCF_CreateUAV)
			desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

		if (creationFlags & TCF_GenerateMips)
		{
			desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		}

		desc.CPUAccessFlags = (creationFlags & TCF_AllowCPUAccess) ? D3D11_CPU_ACCESS_READ : 0;
	}

	template< class TTextureCreationResult >
	bool CreateResourceView(ID3D11Device* device, DXGI_FORMAT format, uint32 creationFlags, TTextureCreationResult& outResult)
	{
		if (creationFlags & TCF_CreateSRV)
		{
			VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateShaderResourceView(outResult.resource, nullptr, &outResult.SRV));
		}
		if (creationFlags & TCF_CreateUAV)
		{
			VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateUnorderedAccessView(outResult.resource, nullptr, &outResult.UAV));
		}
		return true;
	}

	bool D3D11System::createTexture1DInternal(TextureDesc const& desc, void* data, Texture1DCreationResult& outResult)
	{
		DXGI_FORMAT format = D3D11Translate::To(desc.format);
		uint32 pixelSize = ETexture::GetFormatSize(desc.format);

		D3D11_TEXTURE1D_DESC d3dDesc = {};
		d3dDesc.Format = format;
		d3dDesc.Width = desc.dimension.x;
		d3dDesc.MipLevels = desc.numMipLevel;
		d3dDesc.ArraySize = 1;
		d3dDesc.BindFlags = 0;
		SetupTextureDesc(d3dDesc, desc.creationFlags);

		D3D11_SUBRESOURCE_DATA initData = {};
		if (data)
		{
			initData.pSysMem = (void *)data;
			initData.SysMemPitch = desc.dimension.x * pixelSize;
			initData.SysMemSlicePitch = initData.SysMemPitch;
		}

		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateTexture1D(&d3dDesc, data ? &initData : nullptr, &outResult.resource));
		VERIFY_RETURN_FALSE(CreateResourceView(mDevice, format,  desc.creationFlags, outResult));
		return true;
	}

	bool D3D11System::createTexture2DInternal(TextureDesc const& desc, void* data, int dataAlign, Texture2DCreationResult& outResult)
	{
		bool bDepthFormat = ETexture::IsDepthStencil(desc.format);
		DXGI_FORMAT format = D3D11Translate::To(desc.format);
		uint32 pixelSize = ETexture::GetFormatSize(desc.format);

		D3D11_TEXTURE2D_DESC d3dDesc = {};

		d3dDesc.Format = format;
		d3dDesc.Width = desc.dimension.x;
		d3dDesc.Height = desc.dimension.y;
		d3dDesc.MipLevels = desc.numMipLevel;
		d3dDesc.ArraySize = 1;
		d3dDesc.BindFlags = 0;
		SetupTextureDesc(d3dDesc, desc.creationFlags, bDepthFormat);

		HRESULT hr;
		UINT maxQuality = 0;
		if (desc.numSamples > 1 && !bDepthFormat)
		{
			hr = mDevice->CheckMultisampleQualityLevels(format, desc.numSamples, &maxQuality);
			maxQuality = D3D11_STANDARD_MULTISAMPLE_PATTERN;
		}
		d3dDesc.SampleDesc.Count = desc.numSamples;
		d3dDesc.SampleDesc.Quality = maxQuality;
#if SYS_PLATFORM_WIN
		if (desc.creationFlags & TCF_PlatformGraphicsCompatible)
		{
			if (format == DXGI_FORMAT_B8G8R8A8_UNORM)
			{
				d3dDesc.MiscFlags |= D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
				if (!bDepthFormat)
				{
					d3dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
				}
			}

		}
#endif

		if (data)
		{
			if (desc.creationFlags & TCF_GenerateMips)
			{
				D3D11_SUBRESOURCE_DATA initData;
				initData.pSysMem = (void *)data;
				initData.SysMemPitch = desc.dimension.x * pixelSize;
				initData.SysMemSlicePitch = initData.SysMemPitch * desc.dimension.y;

				VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateTexture2D(&d3dDesc, nullptr, &outResult.resource));
				mDeviceContextImmdiate->UpdateSubresource(outResult.resource, 0, nullptr, initData.pSysMem, initData.SysMemPitch, initData.SysMemSlicePitch);
			}
			else
			{
				TArray< D3D11_SUBRESOURCE_DATA > initDataList;
				initDataList.resize(d3dDesc.MipLevels);

				uint8* pData = (uint8*)data;
				int level = 0;
				for (auto& initData : initDataList)
				{
					int w = Math::Max(1, desc.dimension.x >> level);
					int h = Math::Max(1, desc.dimension.y >> level);
					initData.pSysMem = (void *)pData;
					initData.SysMemPitch = w * pixelSize;
					initData.SysMemSlicePitch = initData.SysMemPitch * h;

					pData += initData.SysMemSlicePitch;
					++level;
				}

				VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateTexture2D(&d3dDesc, initDataList.data(), &outResult.resource));
			}
		}
		else
		{
			VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateTexture2D(&d3dDesc, nullptr, &outResult.resource));
		}

		VERIFY_RETURN_FALSE(CreateResourceView(mDevice, format, desc.numSamples, desc.creationFlags, outResult));


		if (desc.creationFlags & TCF_GenerateMips)
		{
			TComPtr<ID3D11ShaderResourceView> SRV = outResult.SRV;
			mDeviceContextImmdiate->GenerateMips(SRV);
		}


		if (bDepthFormat)
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
			depthStencilDesc.Format = D3D11Translate::To(desc.format);
			switch (depthStencilDesc.Format)
			{
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
				depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				break;
			}

			if (desc.numSamples > 1)
			{
				depthStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
			}
			else
			{
				depthStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				depthStencilDesc.Texture2D.MipSlice = 0;
			}

			VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateDepthStencilView(outResult.resource, &depthStencilDesc, &outResult.DSV));
		}

		return true;
	}

	bool D3D11System::createTexture3DInternal(TextureDesc const& desc, void* data, Texture3DCreationResult& outResult)
	{
		bool bDepthFormat = ETexture::IsDepthStencil(desc.format);
		DXGI_FORMAT format = D3D11Translate::To(desc.format);
		uint32 pixelSize = ETexture::GetFormatSize(desc.format);

		D3D11_TEXTURE3D_DESC d3dDesc = {};
		d3dDesc.Format = format;
		d3dDesc.Width = desc.dimension.x;
		d3dDesc.Height = desc.dimension.y;
		d3dDesc.Depth = desc.dimension.z;
		d3dDesc.MipLevels = desc.numMipLevel;
		d3dDesc.BindFlags = 0;
		SetupTextureDesc(d3dDesc, desc.creationFlags, bDepthFormat);

		D3D11_SUBRESOURCE_DATA initData = {};
		if (data)
		{
			initData.pSysMem = (void *)data;
			initData.SysMemPitch = desc.dimension.x * pixelSize;
			initData.SysMemSlicePitch = initData.SysMemPitch * desc.dimension.y;
		}

		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateTexture3D(&d3dDesc, data ? &initData : nullptr, &outResult.resource));
		VERIFY_RETURN_FALSE(CreateResourceView(mDevice, format, desc.creationFlags, outResult));
		return true;
	}

	bool CreateResourceView(ID3D11Device* device, DXGI_FORMAT format, int numMipLevel, uint32 creationFlags, TextureCubeCreationResult& outResult)
	{
		if (creationFlags & TCF_CreateSRV)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
			desc.Format = format;
			desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;
			desc.TextureCube.MipLevels = numMipLevel;
			desc.TextureCube.MostDetailedMip = 0;
			VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateShaderResourceView(outResult.resource, &desc, &outResult.SRV));
		}
		if (creationFlags & TCF_CreateUAV)
		{
			VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateUnorderedAccessView(outResult.resource, nullptr, &outResult.UAV));
		}
		return true;
	}

	bool D3D11System::createTextureCubeInternal(TextureDesc const& desc, void* data[], TextureCubeCreationResult& outResult)
	{
		bool bDepthFormat = ETexture::IsDepthStencil(desc.format);
		DXGI_FORMAT format = D3D11Translate::To(desc.format);

		D3D11_TEXTURE2D_DESC d3dDesc = {};
		d3dDesc.Format = format;
		d3dDesc.Width = desc.dimension.x;
		d3dDesc.Height = desc.dimension.x;
		d3dDesc.MipLevels = desc.numMipLevel;
		d3dDesc.ArraySize = 6;
		d3dDesc.BindFlags = 0;
		SetupTextureDesc(d3dDesc, desc.creationFlags, bDepthFormat);
		d3dDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

		d3dDesc.SampleDesc.Count = desc.numSamples;
		d3dDesc.SampleDesc.Quality = 0;

		D3D11_SUBRESOURCE_DATA initDataList[6];
		if (data)
		{
			uint32 pixelSize = ETexture::GetFormatSize(desc.format);
			for (int i = 0; i < 6; ++i)
			{
				initDataList[i].pSysMem = (void *)data[i];
				initDataList[i].SysMemPitch = desc.dimension.x * pixelSize;
				initDataList[i].SysMemSlicePitch = 0;
			}
		}

		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateTexture2D(&d3dDesc, data ? &initDataList[0] : nullptr, &outResult.resource));
		VERIFY_RETURN_FALSE(CreateResourceView(mDevice, format, desc.numMipLevel, desc.creationFlags, outResult));
		return true;
	}

	bool D3D11System::createStagingTexture(ID3D11Texture2D* texture, TComPtr< ID3D11Texture2D >& outTexture ,int level)
	{
		D3D11_TEXTURE2D_DESC desc;
		texture->GetDesc(&desc);
		desc.MipLevels = 1;
		desc.Width = Math::Max(desc.Width >> level, 1u);
		desc.Height = Math::Max(desc.Height >> level, 1u);
		desc.ArraySize = 1;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ ;
		desc.MiscFlags &= ~D3D11_RESOURCE_MISC_TEXTURECUBE;
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateTexture2D(&desc, nullptr, &outTexture));
		return true;
	}

	bool D3D11ResourceBoundState::initialize(EShader::Type type, TComPtr< ID3D11Device >& device, TComPtr<ID3D11DeviceContext >& deviceContext)
	{
		mShaderType = type;
		clear();
		for( int i = 0; i < MaxConstBufferNum; ++i )
		{
			if( !mConstBuffers[i].initializeResource(device) )
			{
				return false;
			}
		}
		return true;
	}

	void D3D11ResourceBoundState::bindShader(D3D11ShaderData& shader)
	{
		for (int i = 0; i < MaxConstBufferNum; ++i)
		{
			mConstBuffers[i].updateBufferSize(shader.globalConstBufferSize);
		}
	}

	bool D3D11ResourceBoundState::clearTexture(ShaderParameter const& parameter)
	{
		return clearTextureByLoc(parameter.mLoc);
	}

	bool D3D11ResourceBoundState::clearTextureByLoc(int loc)
	{
		CHECK(loc < MaxSimulatedBoundedSRVNum);
		if (mBoundedSRVs[loc] != nullptr)
		{
			if (mBoundedSRVResources[loc] && mBoundedSRVResources[loc]->getResourceType() == EResourceType::Texture)
			{
				D3D11Cast::ToTextureBase(static_cast<RHITextureBase&>(*mBoundedSRVResources[loc])).removeBind(mShaderType, loc, false);
			}

			mBoundedSRVs[loc] = nullptr;
			mBoundedSRVResources[loc] = nullptr;
			mSRVDirtyMask |= BIT(loc);

			if (loc == mMaxSRVBoundIndex)
			{
				while (mMaxSRVBoundIndex >= 0 && mBoundedSRVs[mMaxSRVBoundIndex] == nullptr)
				{
					--mMaxSRVBoundIndex;
				}
			}
			return true;
		}
		return false;
	}

	bool D3D11ResourceBoundState::clearUAV(ShaderParameter const& parameter)
	{
		CHECK(parameter.mLoc < MaxSimulatedBoundedUAVNum);
		if (mBoundedUAVs[parameter.mLoc] != nullptr)
		{
			--mUAVUsageCount;
			CHECK(mUAVUsageCount >= 0);
			mBoundedUAVs[parameter.mLoc] = nullptr;
			mUAVDirtyMask |= BIT(parameter.mLoc);
			return true;
		}
		return false;
	}


	void D3D11ResourceBoundState::setTexture(ShaderParameter const& parameter, RHITextureBase& texture)
	{
		CHECK(parameter.mLoc < MaxSimulatedBoundedSRVNum);
		D3D11TextureBase& textureBase =  D3D11Cast::ToTextureBase(texture);
		D3D11ShaderResourceView* resViewImpl = &textureBase.mSRV;
		
		if( resViewImpl )
		{
			if( mBoundedSRVs[parameter.mLoc] != resViewImpl->getResource() )
			{
				if (mBoundedSRVs[parameter.mLoc] != nullptr)
				{
					if (mBoundedSRVResources[parameter.mLoc] && mBoundedSRVResources[parameter.mLoc]->getResourceType() == EResourceType::Texture)
					{
						D3D11Cast::ToTextureBase(static_cast<RHITextureBase&>(*mBoundedSRVResources[parameter.mLoc])).removeBind(mShaderType, parameter.mLoc, false);
					}
				}

				mBoundedSRVs[parameter.mLoc] = resViewImpl->getResource();
				mBoundedSRVResources[parameter.mLoc] = &texture;
				mSRVDirtyMask |= BIT(parameter.mLoc);
				if (mMaxSRVBoundIndex < parameter.mLoc)
					mMaxSRVBoundIndex = parameter.mLoc;

				textureBase.addBind(mShaderType, parameter.mLoc, false);
			}
		}
		else
		{
			if (mBoundedSRVs[parameter.mLoc] != nullptr)
			{
				clearTexture(parameter);
			}
			LogWarning(0, "Texture don't have SRV");
		}
	}

	void D3D11ResourceBoundState::setRWTexture(ShaderParameter const& parameter, RHITextureBase& texture, int level)
	{
		D3D11TextureBase& textureImpl = D3D11Cast::ToTextureBase(texture);
		ID3D11UnorderedAccessView* UAV = textureImpl.mUAV;

		if (mBoundedUAVs[parameter.mLoc] != UAV)
		{
			if (UAV == nullptr)
			{
				mUAVUsageCount -= 1;
			}
			else if (mBoundedUAVs[parameter.mLoc] == nullptr)
			{
				mUAVUsageCount += 1;
			}
			else
			{
				// Replacing existing UAV? We don't seemingly track UAV resources in array like SRVResources.
				// But we should track binds if we want optimization.
				// However, D3D11ResourceBoundState tracking for UAV Resources is missing in original code (only ID3D11UnorderedAccessView* array).
				// We cannot reliably call removeBind on old resource if we don't know what it was.
				// Optimization: We accept that we can't unbind OLD UAVs from tracking if we don't track them in Context.
				// BUT, RHIResourceTransition relies on SRV tracking to unbind SRVs.
				// UAV->SRV transition is rare and uses clearUAVResource which checks pointer.
				// For now, let's only addBind for the NEW texture if it's not null.
			}
			CHECK(mUAVUsageCount >= 0);
			mBoundedUAVs[parameter.mLoc] = UAV;
			mUAVDirtyMask |= BIT(parameter.mLoc);

			if (UAV)
			{
				textureImpl.addBind(mShaderType, parameter.mLoc, true);
			}
		}
	}

	void D3D11ResourceBoundState::setRWSubTexture(ShaderParameter const& parameter, RHITextureBase& texture, int subIndex, int level)
	{
		CHECK(parameter.mLoc < MaxSimulatedBoundedUAVNum);
		D3D11TextureBase& textureImpl = D3D11Cast::ToTextureBase(texture);
		ID3D11UnorderedAccessView* UAV = textureImpl.mUAV;

		if (mBoundedUAVs[parameter.mLoc] != UAV)
		{
			if (mBoundedUAVs[parameter.mLoc] != nullptr)
			{
				--mUAVUsageCount;
			}

			mBoundedUAVs[parameter.mLoc] = UAV;
			if (UAV != nullptr)
			{
				++mUAVUsageCount;
				textureImpl.addBind(mShaderType, parameter.mLoc, true);
			}

			CHECK(mUAVUsageCount >= 0 && mUAVUsageCount <= MaxSimulatedBoundedUAVNum);
			mUAVDirtyMask |= BIT(parameter.mLoc);
		}
	}

	void D3D11ResourceBoundState::clearRWTexture(ShaderParameter const& parameter)
	{
		CHECK(parameter.mLoc < MaxSimulatedBoundedUAVNum);
		if (mBoundedUAVs[parameter.mLoc] != nullptr)
		{
			--mUAVUsageCount;
			CHECK(mUAVUsageCount >= 0);
			mBoundedUAVs[parameter.mLoc] = nullptr;
			mUAVDirtyMask |= BIT(parameter.mLoc);
		}
	}

	void D3D11ResourceBoundState::setSampler(ShaderParameter const& parameter, RHISamplerState& sampler)
	{
		CHECK(parameter.mLoc < MaxSimulatedBoundedSamplerNum);
		auto& samplerImpl = D3D11Cast::To(sampler);
		if( mBoundedSamplers[parameter.mLoc] != samplerImpl.getResource() )
		{
			mBoundedSamplers[parameter.mLoc] = samplerImpl.getResource();
			mSamplerDirtyMask |= BIT(parameter.mLoc);
		}
	}

	void D3D11ResourceBoundState::setUniformBuffer(ShaderParameter const& parameter, RHIBuffer& buffer)
	{
		CHECK(parameter.mLoc < MaxSimulatedBoundedBufferNum);
		D3D11Buffer& bufferImpl = D3D11Cast::To(buffer);
		if( mBoundedConstBuffers[parameter.mLoc] != bufferImpl.mResource )
		{
			mBoundedConstBuffers[parameter.mLoc] = bufferImpl.mResource;
			mConstBufferDirtyMask |= BIT(parameter.mLoc);
		}
	}


	void D3D11ResourceBoundState::setStructuredBuffer(ShaderParameter const& parameter, RHIBuffer& buffer, EAccessOp op)
	{
		if (op == EAccessOp::ReadOnly)
		{
			CHECK(parameter.mLoc < MaxSimulatedBoundedSRVNum);
			ID3D11ShaderResourceView* SRV = static_cast<D3D11Buffer&>(buffer).mSRV;
			if (mBoundedSRVs[parameter.mLoc] != SRV)
			{
				mBoundedSRVs[parameter.mLoc] = SRV;
				mBoundedSRVResources[parameter.mLoc] = &buffer;
				mSRVDirtyMask |= BIT(parameter.mLoc);
				if (mMaxSRVBoundIndex < parameter.mLoc)
					mMaxSRVBoundIndex = parameter.mLoc;
			}
		}
		else
		{
			CHECK(parameter.mLoc < MaxSimulatedBoundedUAVNum);
			ID3D11UnorderedAccessView* UAV = static_cast<D3D11Buffer&>(buffer).mUAV;

			if (mBoundedUAVs[parameter.mLoc] != UAV)
			{
				if (mBoundedUAVs[parameter.mLoc] != nullptr)
				{
					--mUAVUsageCount;
				}

				mBoundedUAVs[parameter.mLoc] = UAV;
				if (UAV != nullptr)
				{
					++mUAVUsageCount;
				}

				CHECK(mUAVUsageCount >= 0 && mUAVUsageCount <= MaxSimulatedBoundedUAVNum);
				mUAVDirtyMask |= BIT(parameter.mLoc);
			}
		}
	}

	void D3D11ResourceBoundState::clearStructuredBuffer(ShaderParameter const& parameter, EAccessOp op)
	{
		if (op == EAccessOp::ReadOnly)
		{
			clearTextureByLoc(parameter.mLoc);
		}
		else
		{
			clearUAV(parameter);
		}
	}


	void D3D11ResourceBoundState::setShaderValue(ShaderParameter const& parameter, void const* value, int valueSize)
	{
		assert(parameter.bindIndex < MaxConstBufferNum);
		uint32 const bindIndex = parameter.bindIndex;
		mConstBuffers[bindIndex].updateValue(parameter, value, valueSize);
		mConstBufferValueDirtyMask |= BIT(bindIndex);
		if(mBoundedConstBuffers[bindIndex] != mConstBuffers[bindIndex].resource.get())
		{
			mBoundedConstBuffers[bindIndex] = mConstBuffers[bindIndex].resource.get();
			mConstBufferDirtyMask |= BIT(bindIndex);
		}
	}

	template< EShader::Type TypeValue >
	void D3D11ResourceBoundState::commitState(ID3D11DeviceContext* context)
	{
#if 0
		mConstBufferValueDirtyMask = 0xf;
		mConstBufferDirtyMask = 0xf;
		mSRVDirtyMask = 0xf;
#endif

		if( mConstBufferValueDirtyMask )
		{
			uint32 mask = mConstBufferValueDirtyMask;
			mConstBufferValueDirtyMask = 0;
			int index;
			while (FBitUtility::IterateMask< MaxConstBufferNum >(mask, index))
			{
				mConstBuffers[index].commit(context);
			}
		}

		UpdateDirtyState<MaxSimulatedBoundedBufferNum>(mConstBufferDirtyMask, [this, context](int index, int count)
		{
			D3D11ShaderTraits<TypeValue>::SetConstBuffers(context, index, count, mBoundedConstBuffers + index);
		});


		UpdateDirtyState<MaxSimulatedBoundedSamplerNum>(mSamplerDirtyMask, [this, context](int index, int count)
		{
			D3D11ShaderTraits<TypeValue>::SetSamplers(context, index, count, mBoundedSamplers + index);
		});
	}

	template< EShader::Type TypeValue >
	void D3D11ResourceBoundState::clearConstantBuffers(ID3D11DeviceContext* context)
	{
		for( int index = 0; index < MaxSimulatedBoundedSamplerNum; ++index )
		{
			if (mBoundedConstBuffers[index])
			{
				mBoundedConstBuffers[index] = nullptr;
				ID3D11Buffer* emptyBuffer = nullptr;
				D3D11ShaderTraits<TypeValue>::SetConstBuffers(context, index, 1, &emptyBuffer);
			}
		}
	}

	template< EShader::Type TypeValue >
	void D3D11ResourceBoundState::postDrawPrimitive(ID3D11DeviceContext* context)
	{

	}

	template< EShader::Type TypeValue >
	void D3D11ResourceBoundState::clearSRVResource(ID3D11DeviceContext* context, RHIResource& resource)
	{
		for (int index = mMaxSRVBoundIndex; index >= 0; --index)
		{
			if (mBoundedSRVResources[index] == &resource)
			{
				mBoundedSRVResources[index] = nullptr;
				mBoundedSRVs[index] = nullptr;

				if (mMaxSRVBoundIndex == index)
					--mMaxSRVBoundIndex;
#if 0
				mSRVDirtyMask &= ~BIT(index);
				ID3D11ShaderResourceView* emptySRV = nullptr;
				D3D11ShaderTraits<TypeValue>::SetShaderResources(context, index, 1, &emptySRV);
#else
				mSRVDirtyMask |= BIT(index);
#endif
			}
		}
	}

	template< EShader::Type TypeValue >
	void D3D11ResourceBoundState::clearSRVResource(ID3D11DeviceContext* context)
	{
#if 0
		for (int index = 0; index <= mMaxSRVBoundIndex; ++index)
		{
			mBoundedSRVs[index] = nullptr;
			mBoundedSRVResources[index] == nullptr;
		}
		mSRVDirtyMask = 0;

		D3D11ShaderTraits<TypeValue>::SetShaderResources(context, 0, MaxSimulatedBoundedSRVNum, mBoundedSRVs);
#else

		for (int index = 0; index <= mMaxSRVBoundIndex; ++index)
		{
			if (mBoundedSRVs[index])
			{
				mBoundedSRVs[index] = nullptr;
				mBoundedSRVResources[index] == nullptr;
				mSRVDirtyMask |= BIT(index);
			}
		}
#endif
		mMaxSRVBoundIndex = INDEX_NONE;
	}

	template< EShader::Type TypeValue >
	void D3D11ResourceBoundState::commitSAVState(ID3D11DeviceContext* context)
	{
		UpdateDirtyState< MaxSimulatedBoundedSRVNum >(mSRVDirtyMask, [this, context](int index, int count)
		{
			D3D11ShaderTraits<TypeValue>::SetShaderResources(context, index, count, mBoundedSRVs + index);
		});
	}

	void D3D11ResourceBoundState::commitUAVState(ID3D11DeviceContext* context)
	{
		UpdateDirtyState< D3D11ResourceBoundState::MaxSimulatedBoundedUAVNum >(mUAVDirtyMask, [this, context](int index, int count)
		{
			context->CSSetUnorderedAccessViews(index, count, mBoundedUAVs + index, nullptr);
		});
	}

	void D3D11ResourceBoundState::clearUAVResource(ID3D11UnorderedAccessView* UAV)
	{
		for (int i = 0; i < D3D11ResourceBoundState::MaxSimulatedBoundedUAVNum; ++i)
		{
			if (mBoundedUAVs[i] == UAV)
			{
				mBoundedUAVs[i] = nullptr;
				mUAVDirtyMask |= BIT(i);
				--mUAVUsageCount;
			}
		}
	}

	template< EShader::Type TypeValue >
	void D3D11ResourceBoundState::clearContext(ID3D11DeviceContext* context)
	{
		ID3D11ShaderResourceView* EmptySRVs[MaxSimulatedBoundedSRVNum] = { nullptr };
		D3D11ShaderTraits<TypeValue>::SetShaderResources(context, 0, MaxSimulatedBoundedSRVNum, EmptySRVs);

		if constexpr (TypeValue == EShader::Compute)
		{
			ID3D11UnorderedAccessView* EmptyUAVs[D3D11ResourceBoundState::MaxSimulatedBoundedUAVNum] = { nullptr };
			context->CSSetUnorderedAccessViews(0, D3D11ResourceBoundState::MaxSimulatedBoundedUAVNum, EmptyUAVs, nullptr);
		}
	}

	bool D3D11Context::initialize(TComPtr< ID3D11Device >& device, TComPtr<ID3D11DeviceContext >& deviceContext)
	{
		mDevice = device;
		mDeviceContext = deviceContext;
		for( int i = 0; i < EShader::CountSM5; ++i )
		{
			mBoundedShaders[i].resource = nullptr;
			mResourceBoundStates[i].initialize((EShader::Type)i, device, deviceContext);
		}

		uint32 dynamicVBufferSize[] = { sizeof(float) * 512 , sizeof(float) * 1024 , sizeof(float) * 1024 * 4 , sizeof(float) * 1024 * 8 };
		mDynamicVBuffer.initialize(device, D3D11_BIND_VERTEX_BUFFER, dynamicVBufferSize);
		uint32 dynamicIBufferSize[] = { sizeof(uint32) * 3 * 16 , sizeof(uint32) * 3 * 64  , sizeof(uint32) * 3 * 256 , sizeof(uint32) * 3 * 1024 };
		mDynamicIBuffer.initialize(device, D3D11_BIND_INDEX_BUFFER, dynamicIBufferSize);
		return true;
	}

	void D3D11Context::release()
	{
		for (auto& state : mResourceBoundStates)
		{
			state.releaseResource();
		}
		mDynamicVBuffer.release();
		mDynamicIBuffer.release();
		mDeviceContext.reset();
		mDevice.reset();
	}

	void D3D11Context::RHISetRasterizerState(RHIRasterizerState& rasterizerState)
	{
		mDeviceContext->RSSetState(D3D11Cast::GetResource(rasterizerState));
	}

	void D3D11Context::RHISetBlendState(RHIBlendState& blendState)
	{
		mDeviceContext->OMSetBlendState(D3D11Cast::GetResource(blendState), Vector4(0, 0, 0, 0), 0xffffffff);
	}

	void D3D11Context::RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef)
	{
		mDeviceContext->OMSetDepthStencilState(D3D11Cast::GetResource(depthStencilState), stencilRef);
	}

	void D3D11Context::RHISetViewport(ViewportInfo const& viewport)
	{
		D3D11_VIEWPORT& vp = mViewportStates[0];
		vp.TopLeftX = viewport.x;
		vp.TopLeftY = viewport.y;
		vp.Width = viewport.w;
		vp.Height = viewport.h;
		vp.MinDepth = viewport.zNear;
		vp.MaxDepth = viewport.zFar;
		mDeviceContext->RSSetViewports(1, &vp);
	}

	void D3D11Context::RHISetViewports(ViewportInfo const viewports[], int numViewports)
	{
		CHECK(numViewports < ARRAY_SIZE(mViewportStates));
		for (int i = 0; i < numViewports; ++i)
		{
			auto& viewportState = mViewportStates[i];
			auto const& vp = viewports[i];
			viewportState.TopLeftX = vp.x;
			viewportState.TopLeftY = vp.y;
			viewportState.Width = vp.w;
			viewportState.Height = vp.h;
			viewportState.MinDepth = vp.zNear;
			viewportState.MaxDepth = vp.zFar;
		}
		mDeviceContext->RSSetViewports(numViewports, mViewportStates);
	}

	void D3D11Context::RHISetScissorRect(int x, int y, int w, int h)
	{
		D3D11_RECT rect;	

		rect.left = x;
		rect.right = x + w;
		rect.top = y;
		rect.bottom = y + h;

		mDeviceContext->RSSetScissorRects(1, &rect);
	}

	void D3D11Context::postDrawPrimitive()
	{
#define POST_DRAW_PRIMITIVE_OP( SHADER_TYPE )\
		mResourceBoundStates[SHADER_TYPE].postDrawPrimitive<SHADER_TYPE>(mDeviceContext.get());

		GRAPHIC_SHADER_LIST(POST_DRAW_PRIMITIVE_OP);

#undef POST_DRAW_PRIMITIVE_OP
	}


	void D3D11Context::RHIDrawPrimitive(EPrimitive type, int start, int nv)
	{
		commitGraphicsShaderState();
		mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(type));
		mDeviceContext->Draw(nv, start);
		postDrawPrimitive();
	}

	void D3D11Context::RHIDrawIndexedPrimitive(EPrimitive type, int indexStart, int nIndex, uint32 baseVertex)
	{
		commitGraphicsShaderState();
		mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(type));
		mDeviceContext->DrawIndexed(nIndex, indexStart, baseVertex);
		postDrawPrimitive();
	}

	void D3D11Context::RHIDrawPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride)
	{
		commitGraphicsShaderState();
		mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(type));
		if (numCommand > 1)
		{
			if (commandStride == 0)
			{
				commandStride = commandBuffer->getElementSize();
			}
			if (GMultiDrawInstancedIndirect)
			{
				GMultiDrawInstancedIndirect(mDeviceContext, numCommand, D3D11Cast::GetResource(*commandBuffer), offset, commandStride);
			}
			else
			{
				int cmdOffset = offset;
				for (int i = 0; i < numCommand; ++i)
				{
					mDeviceContext->DrawInstancedIndirect(D3D11Cast::GetResource(*commandBuffer), cmdOffset);
					cmdOffset += commandStride;
				}
			}
		}
		else
		{
			mDeviceContext->DrawInstancedIndirect(D3D11Cast::GetResource(*commandBuffer), offset);
		}
		postDrawPrimitive();
	}

	void D3D11Context::RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride)
	{

		commitGraphicsShaderState();
		mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(type));
		if (numCommand > 1)
		{
			if (commandStride == 0)
			{
				commandStride = commandBuffer->getElementSize();
			}
			if (GMultiDrawIndexedInstancedIndirect)
			{
				GMultiDrawIndexedInstancedIndirect(mDeviceContext, numCommand, D3D11Cast::GetResource(*commandBuffer), offset, commandStride);
			}
			else
			{
				int cmdOffset = offset;
				for (int i = 0; i < numCommand; ++i)
				{
					mDeviceContext->DrawIndexedInstancedIndirect(D3D11Cast::GetResource(*commandBuffer), cmdOffset);
					cmdOffset += commandStride;
				}
			}
		}
		else
		{
			mDeviceContext->DrawIndexedInstancedIndirect(D3D11Cast::GetResource(*commandBuffer), offset);
		}
		postDrawPrimitive();
	}

	void D3D11Context::RHIDrawPrimitiveInstanced(EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance)
	{
		commitGraphicsShaderState();
		mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(type));
		mDeviceContext->DrawInstanced(nv, numInstance, vStart, baseInstance);
		postDrawPrimitive();
	}

	void D3D11Context::RHIDrawIndexedPrimitiveInstanced(EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance)
	{
		commitGraphicsShaderState();
		mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(type));
		mDeviceContext->DrawIndexedInstanced(nIndex, numInstance, indexStart, baseVertex, baseInstance);
		postDrawPrimitive();
	}

	bool D3D11Context::determitPrimitiveTopologyUP(EPrimitive primitive, int num, uint32 const* pIndices, EPrimitive& outPrimitiveDetermited, ID3D11Buffer** outIndexBuffer, int& outIndexNum)
	{
		struct MyBuffer
		{
			MyBuffer(D3D11DynamicBuffer& buffer, ID3D11DeviceContext* context)
				:mBuffer(buffer),mContext(context)
			{
			}
			void* lock(uint32 size)
			{
				return mBuffer.lock(mContext, size);
			}
			void unlock()
			{
				outIndexBuffer = mBuffer.unlock(mContext);
			}

			D3D11DynamicBuffer&  mBuffer;
			ID3D11DeviceContext* mContext;
			ID3D11Buffer* outIndexBuffer = nullptr;
		};
		MyBuffer buffer(mDynamicIBuffer, mDeviceContext);
		if ( DetermitPrimitiveTopologyUP(primitive, D3D11Translate::To(primitive) != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED, num, pIndices, buffer, outPrimitiveDetermited, outIndexNum))
		{
			*outIndexBuffer = buffer.outIndexBuffer;
			return true;
		}
		return false;
	}

	void D3D11Context::RHIDrawPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numVertexData, uint32 numInstance)
	{
		assert(numVertexData <= MAX_INPUT_STREAM_NUM);
		EPrimitive primitiveTopology;
		ID3D11Buffer* indexBuffer = nullptr;
		int numDrawIndex;
		if( !determitPrimitiveTopologyUP(type, numVertices, nullptr, primitiveTopology, &indexBuffer, numDrawIndex) )
			return;

		uint32 vertexBufferSize = 0;
		if (type == EPrimitive::LineLoop)
		{
			++numVertices;
			for (int i = 0; i < numVertexData; ++i)
			{
				vertexBufferSize += AlignArbitrary<uint32>(dataInfos[i].size + dataInfos[i].stride, D3D11BUFFER_ALIGN);
			}
		}
		else
		{
			for (int i = 0; i < numVertexData; ++i)
			{
				vertexBufferSize += AlignArbitrary<uint32>(dataInfos[i].size, D3D11BUFFER_ALIGN);
			}
		}

		uint8* pVBufferData = (uint8*)mDynamicVBuffer.lock(mDeviceContext, vertexBufferSize);
		if( pVBufferData )
		{
			ID3D11Buffer* vertexBuffer = mDynamicVBuffer.getLockedBuffer();
			uint32 dataOffset = 0;
			UINT strides[MAX_INPUT_STREAM_NUM];
			UINT offsets[MAX_INPUT_STREAM_NUM];
			ID3D11Buffer* vertexBuffers[MAX_INPUT_STREAM_NUM];
			for( int i = 0; i < numVertexData; ++i )
			{
				auto const& info = dataInfos[i];
				strides[i] = dataInfos[i].stride;
				offsets[i] = dataOffset;
				vertexBuffers[i] = vertexBuffer;

				if( type == EPrimitive::LineLoop )
				{
					FMemory::Copy(pVBufferData + dataOffset, info.ptr, info.size );
					FMemory::Copy(pVBufferData + dataOffset + info.size , info.ptr, info.stride);
					dataOffset += AlignArbitrary<uint32>(dataInfos[i].size + dataInfos[i].stride, D3D11BUFFER_ALIGN);
				}
				else
				{
					FMemory::Copy(pVBufferData + dataOffset, info.ptr, info.size);
					dataOffset += AlignArbitrary<uint32>(dataInfos[i].size, D3D11BUFFER_ALIGN);
				}
			}

			mDynamicVBuffer.unlock(mDeviceContext);

			commitGraphicsShaderState();

			mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(primitiveTopology));
			mDeviceContext->IASetVertexBuffers(0, numVertexData, vertexBuffers, strides, offsets);
			if( indexBuffer )
			{
				mDeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
				if (numInstance > 1)
				{
					mDeviceContext->DrawIndexedInstanced(numDrawIndex, numInstance, 0, 0, 0);
				}
				else
				{
					mDeviceContext->DrawIndexed(numDrawIndex, 0, 0);
				}
			}
			else
			{
				mDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
				if (numInstance > 1)
				{
					mDeviceContext->DrawInstanced(numVertices, numInstance, 0, 0);
				}
				else
				{
					mDeviceContext->Draw(numVertices, 0);
				}
			}
			postDrawPrimitive();
		}
	}

	void D3D11Context::RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numVertexData, uint32 const* pIndices, int numIndex, uint32 numInstance)
	{
		EPrimitive primitiveTopology;
		ID3D11Buffer* indexBuffer = nullptr;
		int numDrawIndex;
		if( !determitPrimitiveTopologyUP(type, numIndex, pIndices, primitiveTopology, &indexBuffer, numDrawIndex) )
			return;

		uint32 vertexBufferSize = 0;
		for( int i = 0; i < numVertexData; ++i )
		{
			vertexBufferSize += AlignArbitrary<uint32>(dataInfos[i].size, D3D11BUFFER_ALIGN);
		}

		uint8* pVBufferData = (uint8*)mDynamicVBuffer.lock(mDeviceContext, vertexBufferSize);
		if( pVBufferData )
		{
			ID3D11Buffer* vertexBuffer = mDynamicVBuffer.getLockedBuffer();
			uint32 dataOffset = 0;
			UINT strides[MAX_INPUT_STREAM_NUM];
			UINT offsets[MAX_INPUT_STREAM_NUM];
			ID3D11Buffer* vertexBuffers[MAX_INPUT_STREAM_NUM];
			for( int i = 0; i < numVertexData; ++i )
			{
				auto const& info = dataInfos[i];
				strides[i] = dataInfos[i].stride;
				offsets[i] = dataOffset;
				vertexBuffers[i] = vertexBuffer;
				FMemory::Copy(pVBufferData + dataOffset, info.ptr, info.size);
				dataOffset += AlignArbitrary<uint32>(dataInfos[i].size, D3D11BUFFER_ALIGN);
			}

			mDynamicVBuffer.unlock(mDeviceContext);

			commitGraphicsShaderState();

			mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(primitiveTopology));
			mDeviceContext->IASetVertexBuffers(0, numVertexData, vertexBuffers, strides, offsets);

			mDeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

			if (numInstance > 1)
			{
				mDeviceContext->DrawIndexedInstanced(numDrawIndex, numInstance, 0, 0, 0);
			}
			else
			{
				mDeviceContext->DrawIndexed(numDrawIndex, 0, 0);
			}
			postDrawPrimitive();
		}
	}

	void D3D11Context::RHISetFixedShaderPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler)
	{
		bUseFixedShaderPipeline = true;
		mFixedShaderParams.transform = transform;
		mFixedShaderParams.color = color;
		mFixedShaderParams.texture = texture;
		mFixedShaderParams.sampler = sampler;
	}

	void D3D11Context::RHISetFrameBuffer(RHIFrameBuffer* frameBuffer)
	{
		D3D11RenderTargetsState* newState;
		bool bForceReset = false;
		if (frameBuffer == nullptr)
		{
			D3D11SwapChain* swapChain = static_cast<D3D11System*>(GRHISystem)->mSwapChain.get();
			if (swapChain == nullptr)
			{
				mDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
				mRenderTargetsState = nullptr;
				return;
			}

			newState = &swapChain->mRenderTargetsState;
		}
		else
		{
			D3D11FrameBuffer* frameBufferImpl = static_cast<D3D11FrameBuffer*>(frameBuffer);
			newState = &frameBufferImpl->mRenderTargetsState;
			bForceReset = frameBufferImpl->bStateDirty;
			frameBufferImpl->bStateDirty = false;
		}

		CHECK(newState);
		if (bForceReset || mRenderTargetsState != newState )
		{
			mRenderTargetsState = newState;
			for (int i = 0; i < mRenderTargetsState->numColorBuffers; ++i)
			{
				clearSRVResource(*mRenderTargetsState->colorResources[i]);
			}

			if (mRenderTargetsState->depthBuffer)
			{
				clearSRVResource(*mRenderTargetsState->depthResource);
			}
			mDeviceContext->OMSetRenderTargets(mRenderTargetsState->numColorBuffers, mRenderTargetsState->colorBuffers, mRenderTargetsState->depthBuffer);
		}
	}

	void D3D11Context::RHIClearRenderTargets(EClearBits clearBits, LinearColor colors[], int numColor, float depth, uint8 stenceil)
	{
		if (mRenderTargetsState == nullptr)
			return;

		if (HaveBits(clearBits, EClearBits::Color))
		{
			for (int i = 0; i < numColor; ++i)
			{
				if (i >= mRenderTargetsState->numColorBuffers)
					break;

				if (mRenderTargetsState->colorBuffers[i])
				{
					mDeviceContext->ClearRenderTargetView(mRenderTargetsState->colorBuffers[i], (numColor == 1) ? colors[0] : colors[i]);
				}
			}
		}

		if (HaveBits(clearBits, EClearBits::Depth | EClearBits::Stencil))
		{
			uint32 clearFlag = 0;
			if (HaveBits(clearBits, EClearBits::Depth))
			{
				clearFlag |= D3D11_CLEAR_DEPTH;
			}
			if (HaveBits(clearBits, EClearBits::Stencil))
			{
				clearFlag |= D3D11_CLEAR_STENCIL;
			}
			if (mRenderTargetsState->depthBuffer)
			{
				mDeviceContext->ClearDepthStencilView(mRenderTargetsState->depthBuffer, clearFlag, depth, stenceil);
			}

		}
	}


	ID3D11Resource* GetTextureResource(RHITextureBase& texture)
	{
		D3D11TextureBase& textureImpl = D3D11Cast::ToTextureBase(texture);
		return textureImpl.getD3D11Resource();
	}

	void D3D11Context::RHIResolveTexture(RHITextureBase& destTexture, int destSubIndex, RHITextureBase& srcTexture, int srcSubIndex)
	{
		ID3D11Resource* destResource = GetTextureResource(destTexture);
		ID3D11Resource* srcResource = GetTextureResource(srcTexture);
		mDeviceContext->ResolveSubresource(destResource, destSubIndex, srcResource, srcSubIndex, D3D11Translate::To(destTexture.getFormat()));
	}

	void D3D11Context::RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream)
	{
		int const MaxStreamNum = 16;
		assert(numInputStream <= MaxStreamNum);
		ID3D11Buffer* buffers[MaxStreamNum];
		UINT strides[MaxStreamNum];
		UINT offsets[MaxStreamNum];

		for (int i = 0; i < numInputStream; ++i)
		{
			if (inputStreams[i].buffer)
			{
				buffers[i] = D3D11Cast::GetResource(inputStreams[i].buffer);
				offsets[i] = inputStreams[i].offset;
				strides[i] = inputStreams[i].stride >= 0 ? inputStreams[i].stride : inputStreams[i].buffer->getElementSize();
			}
		}

		mInputLayout = inputLayout;
		if (numInputStream)
		{
			mDeviceContext->IASetVertexBuffers(0, numInputStream, buffers, strides, offsets);
		}
	}


	void D3D11Context::commitGraphicsShaderState()
	{
		if (bUseFixedShaderPipeline)
		{
			if (mInputLayout)
			{
				D3D11InputLayout* inputLayoutImpl = D3D11Cast::To(mInputLayout);
				SimplePipelineProgram* program = SimplePipelineProgram::Get( inputLayoutImpl->mAttriableMask , mFixedShaderParams.texture);

				setGfxShaderProgram(*program->getRHI());
				program->setParameters(RHICommandListImpl(*this), mFixedShaderParams);
			}
		}

		uint32 const GraphicShaderMask = BIT(EShader::Vertex) | BIT(EShader::Pixel) | BIT(EShader::Geometry) | BIT(EShader::Hull) | BIT(EShader::Domain);
		if(mBoundedShaderDirtyMask & GraphicShaderMask)
		{
#define SET_SHADER_OP( SHADER_TYPE )\
			if( mBoundedShaderDirtyMask & BIT(SHADER_TYPE) )\
			{\
				setShader< SHADER_TYPE >( mBoundedShaders[SHADER_TYPE] );\
			}

			GRAPHIC_SHADER_LIST(SET_SHADER_OP)
			mBoundedShaderDirtyMask &= ~GraphicShaderMask;
#undef SET_SHADER_OP
		}

		commitUAVState();
		mResourceBoundStates[EShader::Compute].commitSAVState<EShader::Compute>(mDeviceContext.get());
		mResourceBoundStates[EShader::Compute].commitUAVState(mDeviceContext.get());

#define COMMIT_SHADER_STATE_OP( SHADER_TYPE )\
		if( mGfxBoundedShaderMask & BIT(SHADER_TYPE) )\
		{\
			mResourceBoundStates[SHADER_TYPE].commitState<SHADER_TYPE>(mDeviceContext.get());\
			mResourceBoundStates[SHADER_TYPE].commitSAVState<SHADER_TYPE>(mDeviceContext.get());\
		}

		GRAPHIC_SHADER_LIST(COMMIT_SHADER_STATE_OP)

#undef COMMIT_SHADER_STATE_OP

		if (mInputLayout && mVertexShader)
		{
			mDeviceContext->IASetInputLayout(static_cast<D3D11InputLayout*>(mInputLayout)->getShaderLayout(mDevice, mVertexShader, *mVertexShaderByteCode));
		}
	}

	void D3D11Context::commitComputeState()
	{
#define COMMIT_SAV_OP( SHADER_TYPE ) mResourceBoundStates[SHADER_TYPE].commitSAVState<SHADER_TYPE>(mDeviceContext.get());
		GRAPHIC_SHADER_LIST(COMMIT_SAV_OP);
#undef COMMIT_SAV_OP

		if( mBoundedShaderDirtyMask & BIT(EShader::Compute) )
		{
			setShader<EShader::Compute>(mBoundedShaders[EShader::Compute]);
			mBoundedShaderDirtyMask &= ~BIT(EShader::Compute);
		}
		mResourceBoundStates[EShader::Compute].commitState<EShader::Compute>(mDeviceContext.get());
		mResourceBoundStates[EShader::Compute].commitSAVState<EShader::Compute>(mDeviceContext.get());
		mResourceBoundStates[EShader::Compute].commitUAVState(mDeviceContext.get());
	}

	void D3D11Context::commitUAVState()
	{
		auto& shaderBoundState = mResourceBoundStates[EShader::Pixel];
		if (shaderBoundState.mUAVDirtyMask)
		{
			shaderBoundState.mUAVDirtyMask = 0;
			mDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
				D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
				0, D3D11ResourceBoundState::MaxSimulatedBoundedUAVNum, shaderBoundState.mBoundedUAVs, nullptr);
		}
	}

	void D3D11Context::RHISetShaderProgram(RHIShaderProgram* shaderProgram)
	{
		auto BindShader = [this](EShader::Type type , D3D11ShaderData& shaderData)
		{
			if (mBoundedShaders[type].resource != shaderData.ptr)
			{
				mBoundedShaders[type].resource = shaderData.ptr;
				mResourceBoundStates[type].bindShader(shaderData);
				mBoundedShaderDirtyMask |= BIT(type);
			}
		};

		if( shaderProgram == nullptr )
		{
			mGfxBoundedShaderMask = 0;
			mVertexShader = nullptr;
			for( int i = 0; i < EShader::CountSM5; ++i )
			{
				if ( i == EShader::Compute )
					continue;
				if( mBoundedShaders[i].resource )
				{
					mBoundedShaderDirtyMask |= BIT(i);
					mBoundedShaders[i].resource = nullptr;
				}
			}
		}
		else
		{
			auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*shaderProgram);
			if (shaderProgramImpl.mNumShaders == 1 )
			{
				auto& shaderData = shaderProgramImpl.mShaderDatas[0];
				if(shaderData.type == EShader::Compute)
				{
					BindShader(EShader::Compute, shaderData);
					return;
				}
			}

			bUseFixedShaderPipeline = false;
			setGfxShaderProgram(*shaderProgram);
		}
	}

	void D3D11Context::setGfxShaderProgram(RHIShaderProgram& shaderProgram)
	{
		auto BindShader = [this](EShader::Type type, D3D11ShaderData& shaderData)
		{
			if (mBoundedShaders[type].resource != shaderData.ptr)
			{
				mBoundedShaders[type].resource = shaderData.ptr;
				mResourceBoundStates[type].bindShader(shaderData);
				mBoundedShaderDirtyMask |= BIT(type);
			}
		};

		mGfxBoundedShaderMask = 0;
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);

		uint32 setupMask = 0;
		for (int i = 0; i < shaderProgramImpl.mNumShaders; ++i)
		{
			auto& shaderData = shaderProgramImpl.mShaderDatas[i];
			EShader::Type type = shaderData.type;

			if (type == EShader::Vertex)
			{
				mVertexShader = &shaderProgramImpl;
				mVertexShaderByteCode = &shaderProgramImpl.vertexByteCode;
			}

			setupMask |= BIT(type);
			mGfxBoundedShaderMask |= BIT(type);
			if (mBoundedShaders[type].resource != shaderData.ptr)
			{
				BindShader(type, shaderData);
			}
		}

		for (int i = 0; i < EShader::CountSM5; ++i)
		{
			if (i == EShader::Compute || (setupMask & BIT(i)))
				continue;

			if (mBoundedShaders[i].resource)
			{
				mBoundedShaders[i].resource = nullptr;
				mBoundedShaderDirtyMask |= BIT(i);
			}
		}

	}

	void D3D11Context::RHISetGraphicsShaderBoundState(GraphicsShaderStateDesc const& stateDesc)
	{
		bUseFixedShaderPipeline = false;

		auto SetupShader = [this](EShader::Type type, RHIShader* shader)
		{
			auto BindShader = [this](EShader::Type type, D3D11ShaderData& shaderData)
			{
				if (mBoundedShaders[type].resource != shaderData.ptr)
				{
					mBoundedShaders[type].resource = shaderData.ptr;
					mResourceBoundStates[type].bindShader(shaderData);
					mBoundedShaderDirtyMask |= BIT(type);
				}
			};

			if (shader == nullptr)
			{
				if (mBoundedShaders[type].resource)
				{
					mBoundedShaderDirtyMask |= BIT(type);
					mBoundedShaders[type].resource = nullptr;
				}
			}
			else
			{
				D3D11Shader* shaderImpl = static_cast<D3D11Shader*>(shader);
				mGfxBoundedShaderMask |= BIT(type);
				BindShader(type, shaderImpl->mResource);
			}
		};

		mGfxBoundedShaderMask = 0;
		mVertexShader = stateDesc.vertex;
		if (mVertexShader)
		{
			mVertexShaderByteCode = &static_cast<D3D11VertexShader*>(mVertexShader)->byteCode;
		}

		SetupShader(EShader::Vertex, stateDesc.vertex);
		SetupShader(EShader::Pixel, stateDesc.pixel);
		SetupShader(EShader::Geometry, stateDesc.geometry);
		SetupShader(EShader::Domain, stateDesc.domain);
		SetupShader(EShader::Hull, stateDesc.hull);
	}

	void D3D11Context::RHISetComputeShader(RHIShader* shader)
	{

		auto BindShader = [this](EShader::Type type, D3D11ShaderData& shaderData)
		{
			if (mBoundedShaders[type].resource != shaderData.ptr)
			{
				mBoundedShaders[type].resource = shaderData.ptr;
				mResourceBoundStates[type].bindShader(shaderData);
				mBoundedShaderDirtyMask |= BIT(type);
			}
		};

		ID3D11DeviceChild* resource = nullptr;
		D3D11Shader* shaderImpl = static_cast<D3D11Shader*>(shader);
		if (shader)
		{
			if (shaderImpl->mResource.type != EShader::Compute)
				return;

			resource = shaderImpl->mResource.ptr;
		}	
		BindShader(EShader::Compute, shaderImpl->mResource);
	}

	template< EShader::Type TypeValue >
	void D3D11Context::setShader(D3D11ShaderVariant  const& shaderVariant)
	{
		switch( TypeValue )
		{
		case EShader::Vertex:   mDeviceContext->VSSetShader(shaderVariant.vertex, nullptr, 0); break;
		case EShader::Pixel:    mDeviceContext->PSSetShader(shaderVariant.pixel, nullptr, 0); break;
		case EShader::Geometry: mDeviceContext->GSSetShader(shaderVariant.geometry, nullptr, 0); break;
		case EShader::Hull:     mDeviceContext->HSSetShader(shaderVariant.hull, nullptr, 0); break;
		case EShader::Domain:   mDeviceContext->DSSetShader(shaderVariant.domain, nullptr, 0); break;
		case EShader::Compute:  mDeviceContext->CSSetShader(shaderVariant.compute, nullptr, 0); break;
		}
	}

	template < class ValueType >
	void D3D11Context::setShaderValueT(RHIShaderProgram& shaderProgram, ShaderParameter const& param, ValueType const val[], int dim)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, val, dim](EShader::Type type, ShaderParameter const& shaderParam)
		{
			mResourceBoundStates[type].setShaderValue(shaderParam, val, sizeof(ValueType) * dim);
		});
	}

	void D3D11Context::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, int32 const val[], int dim)
	{
		setShaderValueT(shaderProgram, param, val, dim);
	}

	void D3D11Context::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim)
	{
		setShaderValueT(shaderProgram, param, val, dim);
	}

	void D3D11Context::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix3 const val[], int dim)
	{
		//valid layout ?
		setShaderValueT(shaderProgram, param, val, dim);
	}

	void D3D11Context::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix4 const val[], int dim)
	{
		setShaderValueT(shaderProgram, param, val, dim);
	}

	void D3D11Context::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector2 const val[], int dim)
	{
		//valid layout ?
		setShaderValueT(shaderProgram, param, val, dim);
	}

	void D3D11Context::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector3 const val[], int dim)
	{
		//valid layout ?
		setShaderValueT(shaderProgram, param, val, dim);
	}

	void D3D11Context::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector4 const val[], int dim)
	{
		setShaderValueT(shaderProgram, param, val, dim);
	}

	void D3D11Context::setShaderMatrix22(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim)
	{
		//valid layout ?
		setShaderValueT(shaderProgram, param, val, 2 * 2 * dim);
	}

	void D3D11Context::setShaderMatrix43(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim)
	{
		setShaderValueT(shaderProgram, param, val, 4 * 3 * dim);
	}

	void D3D11Context::setShaderMatrix34(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim)
	{
		//valid layout ?
		setShaderValueT(shaderProgram, param, val, 3 * 4 * dim);
	}

	void D3D11Context::setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &texture](EShader::Type type, ShaderParameter const& shaderParam)
		{
			mResourceBoundStates[type].setTexture(shaderParam, texture);
		});
	}

	void D3D11Context::setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture)
	{
		auto& shaderImpl = static_cast<D3D11Shader&>(shader);
		auto type = shaderImpl.mResource.type;
		mResourceBoundStates[type].setTexture(param, texture);
	}

	void D3D11Context::setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState & sampler)
	{
		auto& shaderImpl = static_cast<D3D11Shader&>(shader);
		auto type = shaderImpl.mResource.type;
		mResourceBoundStates[type].setTexture(param, texture);
		mResourceBoundStates[type].setSampler(paramSampler, sampler);
	}

	void D3D11Context::setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler)
	{
		setShaderTexture(shaderProgram, param, texture);
		setShaderSampler(shaderProgram, paramSampler, sampler);
	}

	void D3D11Context::clearShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this](EShader::Type type, ShaderParameter const& shaderParam)
		{
			D3D11ResourceBoundState& boundState = mResourceBoundStates[type];
			if (boundState.clearTexture(shaderParam) )
			{
				switch (type)
				{
				case EShader::Vertex:   boundState.commitSAVState< EShader::Vertex >(mDeviceContext); break;
				case EShader::Pixel:    boundState.commitSAVState< EShader::Pixel >(mDeviceContext); break;
				case EShader::Geometry: boundState.commitSAVState< EShader::Geometry >(mDeviceContext); break;
				case EShader::Compute:  boundState.commitSAVState< EShader::Compute >(mDeviceContext); break;
				case EShader::Hull:     boundState.commitSAVState< EShader::Hull >(mDeviceContext); break;
				case EShader::Domain:   boundState.commitSAVState< EShader::Domain >(mDeviceContext); break;
				}
			}
		});
	}

	void D3D11Context::clearShaderTexture(RHIShader& shader, ShaderParameter const& param)
	{
		auto& shaderImpl = static_cast<D3D11Shader&>(shader);
		auto type = shaderImpl.mResource.type;
		D3D11ResourceBoundState& boundState = mResourceBoundStates[type];
		if (boundState.clearTexture(param))
		{
			switch (type)
			{
			case EShader::Vertex:   boundState.commitSAVState< EShader::Vertex >(mDeviceContext); break;
			case EShader::Pixel:    boundState.commitSAVState< EShader::Pixel >(mDeviceContext); break;
			case EShader::Geometry: boundState.commitSAVState< EShader::Geometry >(mDeviceContext); break;
			case EShader::Compute:  boundState.commitSAVState< EShader::Compute >(mDeviceContext); break;
			case EShader::Hull:     boundState.commitSAVState< EShader::Hull >(mDeviceContext); break;
			case EShader::Domain:   boundState.commitSAVState< EShader::Domain >(mDeviceContext); break;
			}
		}
	}

	void D3D11Context::setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &sampler](EShader::Type type, ShaderParameter const& shaderParam)
		{
			mResourceBoundStates[type].setSampler(shaderParam, sampler);
		});
	}

	void D3D11Context::setShaderSampler(RHIShader& shader, ShaderParameter const& param, RHISamplerState& sampler)
	{
		auto& shaderImpl = static_cast<D3D11Shader&>(shader);
		auto type = shaderImpl.mResource.type;
		mResourceBoundStates[type].setSampler(param, sampler);
	}

	void D3D11Context::setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		switch (op)
		{
		case EAccessOp::ReadOnly:
#if 0
			setShaderTexture(shaderProgram, param, texture);
			break;
#endif
		case EAccessOp::WriteOnly:
		case EAccessOp::ReadAndWrite:
			shaderProgramImpl.setupShader(param, [this, &texture, level](EShader::Type type, ShaderParameter const& shaderParam)
			{
				mResourceBoundStates[type].setRWTexture(shaderParam, texture, level);
			});
			break;
		}
	}

	void D3D11Context::setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op)
	{
		auto& shaderImpl = static_cast<D3D11Shader&>(shader);
		auto type = shaderImpl.mResource.type;
		mResourceBoundStates[type].setRWTexture(param, texture, level);
	}

	void D3D11Context::setShaderRWSubTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, int subIndex, int level, EAccessOp op)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		switch (op)
		{
		case EAccessOp::ReadOnly:
		case EAccessOp::WriteOnly:
		case EAccessOp::ReadAndWrite:
			shaderProgramImpl.setupShader(param, [this, &texture, subIndex, level](EShader::Type type, ShaderParameter const& shaderParam)
			{
				mResourceBoundStates[type].setRWSubTexture(shaderParam, texture, subIndex, level);
			});
			break;
		}
	}


	void D3D11Context::setShaderRWSubTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, int subIndex, int level, EAccessOp op)
	{
		auto& shaderImpl = static_cast<D3D11Shader&>(shader);
		auto type = shaderImpl.mResource.type;
		mResourceBoundStates[type].setRWSubTexture(param, texture, subIndex, level);
	}


	void D3D11Context::clearShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this](EShader::Type type, ShaderParameter const& shaderParam)
		{
			mResourceBoundStates[type].clearRWTexture(shaderParam);
			if (type == EShader::Compute)
			{
				mResourceBoundStates[type].commitUAVState(mDeviceContext);
			}
		});
	}

	void D3D11Context::clearShaderRWTexture(RHIShader& shader, ShaderParameter const& param)
	{
		auto& shaderImpl = static_cast<D3D11Shader&>(shader);
		auto type = shaderImpl.mResource.type;
		mResourceBoundStates[type].clearRWTexture(param);
		if (type == EShader::Compute)
		{
			mResourceBoundStates[type].commitUAVState(mDeviceContext);
		}
	}

	void D3D11Context::setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &buffer](EShader::Type type, ShaderParameter const& shaderParam)
		{
			mResourceBoundStates[type].setUniformBuffer(shaderParam, buffer);
		});
	}

	void D3D11Context::setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer)
	{
		auto& shaderImpl = static_cast<D3D11Shader&>(shader);
		auto type = shaderImpl.mResource.type;
		mResourceBoundStates[type].setUniformBuffer(param, buffer);
	}

	void D3D11Context::setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &buffer, op](EShader::Type type, ShaderParameter const& shaderParam)
		{
			mResourceBoundStates[type].setStructuredBuffer(shaderParam, buffer, op);
		});
	}

	void D3D11Context::setShaderStorageBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op)
	{
		auto& shaderImpl = static_cast<D3D11Shader&>(shader);
		auto type = shaderImpl.mResource.type;
		mResourceBoundStates[type].setStructuredBuffer(param, buffer, op);
	}

	void D3D11Context::clearShaderBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, EAccessOp op)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, op](EShader::Type type, ShaderParameter const& shaderParam)
		{
			mResourceBoundStates[type].clearStructuredBuffer(shaderParam, op);
		});
	}

	void D3D11Context::clearShaderBuffer(RHIShader& shader, ShaderParameter const& param, EAccessOp op)
	{
		auto& shaderImpl = static_cast<D3D11Shader&>(shader);
		auto type = shaderImpl.mResource.type;
		mResourceBoundStates[type].clearStructuredBuffer(param, op);
	}


	void D3D11Context::RHIResourceTransition(TArrayView<RHIResource*> resources, EResourceTransition transition)
	{
		if (transition == EResourceTransition::SRV)
		{
			// Unbind from UAV if going to SRV
			for (auto resource : resources)
			{
				ID3D11UnorderedAccessView* UAV = nullptr;
				if (resource->getResourceType() == EResourceType::Buffer)
				{
					UAV = static_cast<D3D11Buffer*>(resource->getNativeInternal())->mUAV;
				}
				else if (resource->getResourceType() == EResourceType::Texture)
				{
					auto& textureImpl = D3D11Cast::ToTextureBase(static_cast<RHITextureBase&>(*resource));
					UAV = textureImpl.mUAV;
					
					TArray<D3D11TextureBase::D3D11BindSlot, TInlineAllocator<16>> slotsToUnbind;
					for(auto& slot : textureImpl.mBoundSlots)
					{
						if (slot.isUAV)
							slotsToUnbind.push_back(slot);
					}
					
					// D3D11Context doesn't track UAV resources in mBoundedUAVResources like SRVs, so using BoundSlots is less reliable if we didn't track "removeBind" in all places (we don't track old UAV unbind in setRWTexture fully).
					// But clearUAVResource below is safe: it scans all UAV slots. 
					// Optimizing UAV->SRV is less critical than SRV->UAV because UAV slots are few (8).
				}

				if (UAV)
				{
					// Use scanning for UAVs as we don't have full tracking and loop count is small (8)
					mResourceBoundStates[EShader::Compute].clearUAVResource(UAV);
					mResourceBoundStates[EShader::Pixel].clearUAVResource(UAV);
				}
			}
		}
		else if (transition == EResourceTransition::UAV)
		{
			// SRV -> UAV: Critical Optimization path
			for (auto resource : resources)
			{
				if (resource->getResourceType() == EResourceType::Texture)
				{
					auto& textureImpl = D3D11Cast::ToTextureBase(static_cast<RHITextureBase&>(*resource));
					
					TArray<D3D11TextureBase::D3D11BindSlot, TInlineAllocator<16>> slotsToUnbind;
					for(auto& slot : textureImpl.mBoundSlots)
					{
						if (!slot.isUAV)
							slotsToUnbind.push_back(slot);
					}

					for(auto& slot : slotsToUnbind)
					{
						mResourceBoundStates[slot.stage].clearTextureByLoc(slot.slotIndex);
					}
				}
				else
				{
					// Fallback for Buffers or if optimization fails
					#define OP(STAGE) mResourceBoundStates[STAGE].clearSRVResource<STAGE>(mDeviceContext.get(), *resource);
					GRAPHIC_SHADER_LIST(OP)
					mResourceBoundStates[EShader::Compute].clearSRVResource<EShader::Compute>(mDeviceContext.get(), *resource);
					#undef OP
				}
			}
		}
	}

	void D3D11Context::clearSRVResource(RHIResource& resource)
	{
#define OP(STAGE) mResourceBoundStates[STAGE].clearSRVResource<STAGE>(mDeviceContext, resource);
		GRAPHIC_SHADER_LIST(OP)
		mResourceBoundStates[EShader::Compute].clearSRVResource<EShader::Compute>(mDeviceContext, resource);
#undef OP
	}

	template < class ValueType >
	void D3D11Context::setShaderValueT(RHIShader& shader, ShaderParameter const& param, ValueType const val[], int dim)
	{
		auto& shaderImpl = static_cast<D3D11Shader&>(shader);
		auto type = shaderImpl.mResource.type;
		mResourceBoundStates[type].setShaderValue(param, val, sizeof(ValueType) * dim);
	}

	void D3D11Context::setShaderValue(RHIShader& shader, ShaderParameter const& param, int32 const val[], int dim)
	{
		setShaderValueT(shader, param, val, dim);
	}

	void D3D11Context::setShaderValue(RHIShader& shader, ShaderParameter const& param, float const val[], int dim)
	{
		setShaderValueT(shader, param, val, dim);
	}

	void D3D11Context::setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix4 const val[], int dim)
	{
		setShaderValueT(shader, param, val, dim);
	}

	void D3D11Context::setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix3 const val[], int dim)
	{
		setShaderValueT(shader, param, val, dim);
	}

	void D3D11Context::setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector2 const val[], int dim)
	{
		setShaderValueT(shader, param, val, dim);
	}

	void D3D11Context::setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector3 const val[], int dim)
	{
		setShaderValueT(shader, param, val, dim);
	}

	void D3D11Context::setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector4 const val[], int dim)
	{
		setShaderValueT(shader, param, val, dim);
	}

	void D3D11Context::setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer)
	{

	}

	void D3D11Context::setShaderAtomicCounterBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer)
	{

	}

	void D3D11Context::markRenderStateDirty()
	{
		mRenderTargetsState = nullptr;

		for (int i = 0; i < EShader::CountSM5; ++i)
		{
			mBoundedShaders[i].resource = nullptr;
			mResourceBoundStates[i].clear();
		}
		mResourceBoundStates[EShader::Vertex].clearContext<EShader::Vertex>(mDeviceContext);
		mResourceBoundStates[EShader::Pixel].clearContext<EShader::Pixel>(mDeviceContext);
		mResourceBoundStates[EShader::Geometry].clearContext<EShader::Geometry>(mDeviceContext);
		mResourceBoundStates[EShader::Compute].clearContext<EShader::Compute>(mDeviceContext);
		mResourceBoundStates[EShader::Hull].clearContext<EShader::Hull>(mDeviceContext);
		mResourceBoundStates[EShader::Domain].clearContext<EShader::Domain>(mDeviceContext);

		bUseFixedShaderPipeline = false;
		mVertexShader = nullptr;
		mInputLayout = nullptr;
	}

	static constexpr int32 D3D11_CONSTANT_BUFFER_ALIGN = 16;
	static constexpr int32 D3D11_CONSTANT_BUFFER_INITIAL_SIZE = 2048;
	bool ShaderConstDataBuffer::initializeResource(ID3D11Device* device)
	{
		D3D11_BUFFER_DESC bufferDesc = { 0 };
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.ByteWidth = AlignArbitrary(D3D11_CONSTANT_BUFFER_INITIAL_SIZE, D3D11_CONSTANT_BUFFER_ALIGN);
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateBuffer(&bufferDesc, nullptr, &resource));
		return true;
	}

	void ShaderConstDataBuffer::releaseResource()
	{
		resource.reset();
	}

#if _DEBUG
#define ENSURE(EXPR) EXPR
#else
#define ENSURE(EXPR) true
#endif

	void ShaderConstDataBuffer::updateValue(ShaderParameter const& parameter, void const* value, int valueSize)
	{
		int idxDataEnd = parameter.offset + parameter.size;

		if (!ENSURE(mDataBuffer.size() >= idxDataEnd ))
		{
			mDataBuffer.resize(idxDataEnd);
		}

		FMemory::Copy(&mDataBuffer[parameter.offset], value, parameter.size);
		if (idxDataEnd > mUpdateDataSize)
		{
			mUpdateDataSize = idxDataEnd;
		}
	}

	void ShaderConstDataBuffer::commit(ID3D11DeviceContext* context)
	{ 
		if (mUpdateDataSize)
		{
			context->UpdateSubresource(resource, 0, nullptr, &mDataBuffer[0], 0, 0);
			mUpdateDataSize = 0;
		}
	}

	void ShaderConstDataBuffer::updateBufferSize(int newSize)
	{
		if (mDataBuffer.size() < newSize)
		{
			mDataBuffer.resize(AlignArbitrary(newSize, D3D11_CONSTANT_BUFFER_ALIGN));
		}
	}

}//namespace Render

#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif