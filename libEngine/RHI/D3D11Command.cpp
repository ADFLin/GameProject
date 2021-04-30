#include "D3D11Command.h"

#include "D3D11ShaderCommon.h"

#include "LogSystem.h"
#include "GpuProfiler.h"
#include "RHIGlobalResource.h"

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

namespace Render
{

#define RESULT_FAILED( hr ) ( hr ) != S_OK
	class D3D11ProfileCore : public RHIProfileCore
	{
	public:

		D3D11ProfileCore()
		{
			mCycleToMillisecond = 0;
		}

		bool init(TComPtr<ID3D11Device> const& device,
				  TComPtr<ID3D11DeviceContext> const& deviceContext)
		{
			mDevice = device;
			mDeviceContext = deviceContext;

			D3D11_QUERY_DESC desc;
			desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
			desc.MiscFlags = 0;
			if( RESULT_FAILED(mDevice->CreateQuery(&desc, &mQueryDisjoint)) )
				return false;

			return true;
		}
		virtual void beginFrame() override
		{
			mDeviceContext->Begin(mQueryDisjoint);
		}

		virtual bool endFrame() override
		{
			mDeviceContext->End(mQueryDisjoint);

			while( mDeviceContext->GetData(mQueryDisjoint, NULL, 0, 0) == S_FALSE )
			{
				SystemPlatform::Sleep(0); 
			}

			D3D10_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
			mDeviceContext->GetData(mQueryDisjoint, &tsDisjoint, sizeof(tsDisjoint), 0);
			if( tsDisjoint.Disjoint )
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
			TComPtr< ID3D11Query > startQuery;
			TComPtr< ID3D11Query > endQuery;
			if ( RESULT_FAILED(mDevice->CreateQuery(&desc, &startQuery)) ||
				 RESULT_FAILED(mDevice->CreateQuery(&desc, &endQuery)) )
				return RHI_ERROR_PROFILE_HANDLE;

			uint32 result = mStartQueries.size();
			mStartQueries.push_back(std::move(startQuery));
			mEndQueries.push_back(std::move(endQuery));
			return result;
		}

		virtual void startTiming(uint32 timingHandle) override
		{
			mDeviceContext->End(mStartQueries[timingHandle]);
		}

		virtual void endTiming(uint32 timingHandle) override
		{
			mDeviceContext->End(mEndQueries[timingHandle]);
		}

		virtual bool getTimingDuration(uint32 timingHandle, uint64& outDuration) override
		{
			if( RESULT_FAILED(mDeviceContext->GetData(mStartQueries[timingHandle], NULL, 0, 0)) ||
			    RESULT_FAILED(mDeviceContext->GetData(mEndQueries[timingHandle], NULL, 0, 0)) )
				return false;

			UINT64 startData;
			mDeviceContext->GetData(mStartQueries[timingHandle], &startData, sizeof(startData), 0);
			UINT64 endData;
			mDeviceContext->GetData(mEndQueries[timingHandle], &endData, sizeof(endData), 0);
			outDuration = endData - startData;
			return true;
		}

		virtual double getCycleToMillisecond() override
		{
			if( mCycleToMillisecond == 0 )
			{
				mDeviceContext->Begin(mQueryDisjoint);
				mDeviceContext->End(mQueryDisjoint);

				while( RESULT_FAILED( mDeviceContext->GetData(mQueryDisjoint, NULL, 0, 0) ) )
				{
					SystemPlatform::Sleep(0);
				}

				D3D10_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
				mDeviceContext->GetData(mQueryDisjoint, &tsDisjoint, sizeof(tsDisjoint), 0);
				mCycleToMillisecond = double(1000) / tsDisjoint.Frequency;
			}

			return mCycleToMillisecond;
		}	

		virtual void releaseRHI() override
		{
			mQueryDisjoint.reset();
			mStartQueries.clear();
			mEndQueries.clear();
			mDeviceContext.reset();
			mDevice.reset();
		}
		double mCycleToMillisecond;

		TComPtr< ID3D11Query > mQueryDisjoint;

		std::vector< TComPtr< ID3D11Query > > mStartQueries;
		std::vector< TComPtr< ID3D11Query > > mEndQueries;

		TComPtr<ID3D11DeviceContext> mDeviceContext;
		TComPtr<ID3D11Device> mDevice;
	};

	bool D3D11System::initialize(RHISystemInitParams const& initParam)
	{
		uint32 deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
		if ( initParam.bDebugMode )
			deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
		VERIFY_D3D_RESULT_RETURN_FALSE(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, NULL, 0, D3D11_SDK_VERSION, &mDevice, NULL, &mDeviceContext));

		TComPtr< IDXGIDevice > pDXGIDevice;
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice));
		TComPtr< IDXGIAdapter > pDXGIAdapter;
		VERIFY_D3D_RESULT_RETURN_FALSE(pDXGIDevice->GetAdapter(&pDXGIAdapter));

		DXGI_ADAPTER_DESC adapterDesc;
		VERIFY_D3D_RESULT_RETURN_FALSE(pDXGIAdapter->GetDesc(&adapterDesc));

		switch( adapterDesc.VendorId )
		{
		case 0x10DE: GRHIDeviceVendorName = DeviceVendorName::NVIDIA; break;
		case 0x8086: GRHIDeviceVendorName = DeviceVendorName::Intel; break;
		case 0x1002: GRHIDeviceVendorName = DeviceVendorName::ATI; break;
		}

		mbVSyncEnable = initParam.bVSyncEnable;

		GRHIClipZMin = 0;
		GRHIProjectYSign = 1;
		GRHIVericalFlip = -1;

		mRenderContext.initialize(mDevice, mDeviceContext);
		mImmediateCommandList = new RHICommandListImpl(mRenderContext);
#if 1
		D3D11ProfileCore* profileCore = new D3D11ProfileCore;
		if( profileCore->init(mDevice, mDeviceContext) )
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

	void D3D11System::preShutdown()
	{
		mDeviceContext->ClearState();
	}

	void D3D11System::shutdown()
	{
		GpuProfiler::Get().releaseRHIResource();

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

	RHISwapChain* D3D11System::RHICreateSwapChain(SwapChainCreationInfo const& info)
	{
		HRESULT hr;
		TComPtr<IDXGIFactory> factory;
		hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));

		DXGI_SWAP_CHAIN_DESC swapChainDesc; ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
		swapChainDesc.OutputWindow = info.windowHandle;
		swapChainDesc.Windowed = info.bWindowed;
		swapChainDesc.SampleDesc.Count = info.numSamples;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferDesc.Format = D3D11Translate::To(info.colorForamt);
		swapChainDesc.BufferDesc.Width = info.extent.x;
		swapChainDesc.BufferDesc.Height = info.extent.y;
		swapChainDesc.BufferCount = info.bufferCount;
		swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;

		TComPtr<IDXGISwapChain> swapChainResource;
		VERIFY_D3D_RESULT(factory->CreateSwapChain(mDevice, &swapChainDesc, &swapChainResource), return nullptr;);

		int count = swapChainResource.getRefCount();

		Texture2DCreationResult textureCreationResult;
		VERIFY_D3D_RESULT(swapChainResource->GetBuffer(0, IID_PPV_ARGS(&textureCreationResult.resource)), return nullptr;);
		TRefCountPtr< D3D11Texture2D > colorTexture = new D3D11Texture2D(info.colorForamt, textureCreationResult);
		TRefCountPtr< D3D11Texture2D > depthTexture;
		if (info.bCreateDepth)
		{
			depthTexture = (D3D11Texture2D*)RHICreateTextureDepth(info.depthFormat, info.extent.x, info.extent.y, 1, info.numSamples, 0);
		}
		D3D11SwapChain* swapChain = new D3D11SwapChain(swapChainResource, *colorTexture, depthTexture);

		mSwapChain = swapChain;
		return swapChain;
	}

	Render::RHITexture1D* D3D11System::RHICreateTexture1D(ETexture::Format format, int length, int numMipLevel, uint32 createFlags, void* data)
	{
		Texture1DCreationResult creationResult;
		if (createTexture1DInternal(D3D11Translate::To(format), length, numMipLevel, createFlags, data, ETexture::GetFormatSize(format), creationResult))
		{
			return new D3D11Texture1D(format, creationResult);
		}
		return nullptr;
	}

	RHITexture2D* D3D11System::RHICreateTexture2D(ETexture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 createFlags, void* data, int dataAlign)
	{
		Texture2DCreationResult creationResult;
		if( createTexture2DInternal(D3D11Translate::To(format), w, h, numMipLevel, numSamples, createFlags, data, ETexture::GetFormatSize(format), false, creationResult) )
		{
			return new D3D11Texture2D(format, creationResult);
		}
		return nullptr;
	}
	RHITexture3D* D3D11System::RHICreateTexture3D(
		ETexture::Format format, int sizeX, int sizeY, int sizeZ,
		int numMipLevel, int numSamples, uint32 createFlags,
		void* data)
	{
		Texture3DCreationResult creationResult;
		if (createTexture3DInternal(D3D11Translate::To(format), sizeX , sizeY, sizeZ, numMipLevel, numSamples, createFlags, data, ETexture::GetFormatSize(format), creationResult))
		{
			return new D3D11Texture3D(format, creationResult);
		}
		return nullptr;
	}

	RHITextureCube* D3D11System::RHICreateTextureCube(
		ETexture::Format format, int size, int numMipLevel, uint32 creationFlags, void* data[])
	{
		TextureCubeCreationResult creationResult;
		if (createTextureCubeInternal(D3D11Translate::To(format), size, numMipLevel, 1, creationFlags, data, ETexture::GetFormatSize(format), creationResult))
		{
			return new D3D11TextureCube(format, creationResult);
		}
		return nullptr;
	}

	RHITexture2D* D3D11System::RHICreateTextureDepth(ETexture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 creationFlags)
	{
		Texture2DCreationResult creationResult;

		if( createTexture2DInternal(D3D11Translate::To(format) , w, h, numMipLevel, numSamples, creationFlags, nullptr, 0, true, creationResult) )
		{
			return new D3D11Texture2D(format, creationResult, D3D11Texture2D::DepthFormat);
		}

		return nullptr;
	}


	bool CreateResourceView(ID3D11Device* device, int elementSize , int elementNum , uint32 creationFlags, D3D11BufferCreationResult& outResult)
	{
		if (creationFlags & BCF_CreateSRV)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			desc.Buffer.FirstElement = 0;
			desc.Buffer.NumElements = elementNum;
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

	RHIVertexBuffer* D3D11System::RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		D3D11_SUBRESOURCE_DATA initData = { 0 };
		initData.pSysMem = data;
		initData.SysMemPitch = 0;
		initData.SysMemSlicePitch = 0;

		D3D11_BUFFER_DESC bufferDesc = { 0 };
		bufferDesc.ByteWidth = vertexSize * numVertices;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		if (creationFlags & BCF_CpuAccessWrite)
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		//if (creationFlags & BCF_CPUAccessRead)
		//	bufferDesc.Usage = D3D11_USAGE_STAGING;
		bufferDesc.MiscFlags = 0;

		if (creationFlags & BCF_UsageConst)
		{
			bufferDesc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
		}
		else if (creationFlags & BCF_Structured)
		{
			bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			bufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			bufferDesc.StructureByteStride = vertexSize;
		}
		else
		{
			bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			if (creationFlags & BCF_CreateSRV)
				bufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
		}

		if (creationFlags & BCF_CreateUAV)
		{
			bufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
		}

		if (creationFlags & BCF_CpuAccessWrite)
			bufferDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
		if (creationFlags & BCF_CPUAccessRead)
			bufferDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;

		D3D11BufferCreationResult creationResult;
		creationResult.flags = creationFlags;
		TComPtr<ID3D11Buffer> BufferResource;
		VERIFY_D3D_RESULT(
			mDevice->CreateBuffer(&bufferDesc, data ? &initData : nullptr, &creationResult.resource),
			{
				return nullptr;
			}
		);
		CreateResourceView(mDevice, vertexSize , numVertices, creationFlags, creationResult);
		D3D11VertexBuffer* buffer = new D3D11VertexBuffer(numVertices, vertexSize, creationResult);

		return buffer;
	}

	RHIIndexBuffer* D3D11System::RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
	{
		D3D11_SUBRESOURCE_DATA initData = { 0 };
		initData.pSysMem = data;
		initData.SysMemPitch = 0;
		initData.SysMemSlicePitch = 0;

		D3D11_BUFFER_DESC bufferDesc = { 0 };
		bufferDesc.ByteWidth = nIndices * (bIntIndex ? 4 : 2);
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		if (creationFlags & BCF_CpuAccessWrite)
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		//if (creationFlags & BCF_CPUAccessRead )
		//	bufferDesc.Usage = D3D11_USAGE_STAGING;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		if( creationFlags & BCF_CreateSRV )
			bufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

		bufferDesc.CPUAccessFlags = (creationFlags & BCF_CpuAccessWrite) ? D3D11_CPU_ACCESS_WRITE : 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		if (creationFlags & BCF_CpuAccessWrite)
			bufferDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
		if (creationFlags & BCF_CPUAccessRead)
			bufferDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;

		D3D11BufferCreationResult creationResult;
		creationResult.flags = creationFlags;
		VERIFY_D3D_RESULT(
			mDevice->CreateBuffer(&bufferDesc, data ? &initData : nullptr, &creationResult.resource),
			{
				return nullptr;
			}
		);
		CreateResourceView(mDevice, bufferDesc.ByteWidth , nIndices, creationFlags, creationResult);
		D3D11IndexBuffer* buffer = new D3D11IndexBuffer(nIndices, bIntIndex, creationResult);
		return buffer;
	}

	void* D3D11System::lockBufferInternal(ID3D11Resource* resource, ELockAccess access, uint32 offset, uint32 size)
	{
		D3D11_MAPPED_SUBRESOURCE mappedData;
		HRESULT hr = mDeviceContext->Map(resource, 0, D3D11Translate::To(access), 0, &mappedData);
		if( hr != S_OK )
		{
			return nullptr;
		}
		return (uint8*)mappedData.pData + offset;
	}

	void* D3D11System::RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		return lockBufferInternal(D3D11Cast::GetResource(*buffer), access, offset, size);
	}

	void D3D11System::RHIUnlockBuffer(RHIVertexBuffer* buffer)
	{
		mDeviceContext->Unmap(D3D11Cast::GetResource(*buffer), 0);
	}

	RHIFrameBuffer* D3D11System::RHICreateFrameBuffer()
	{
		return new D3D11FrameBuffer;
	}

	void* D3D11System::RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		return lockBufferInternal(D3D11Cast::GetResource(*buffer), access, offset, size);
	}

	void D3D11System::RHIUnlockBuffer(RHIIndexBuffer* buffer)
	{
		mDeviceContext->Unmap(D3D11Cast::GetResource(*buffer), 0);
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
			std::vector< InputElementDesc const* > sortedElements;
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

		TComPtr< ID3D10Blob > errorCode;
		TComPtr< ID3D10Blob > byteCode;
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
		desc.FrontCounterClockwise = TRUE;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0;
		desc.SlopeScaledDepthBias = 0;
		desc.DepthClipEnable = FALSE;
		desc.ScissorEnable = initializer.bEnableScissor;
		desc.MultisampleEnable = initializer.bEnableMultisample;
		desc.AntialiasedLineEnable = FALSE;

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
		desc.StencilEnable = initializer.bEnableStencilTest;
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
		return new D3D11Shader(type);
	}

	RHIShaderProgram* D3D11System::RHICreateShaderProgram()
	{
		return new D3D11ShaderProgram;
	}


	template< class TD3D11_TEXTURE1D_DESC >
	void SetupTextureDesc(TD3D11_TEXTURE1D_DESC& desc, uint32 creationFlags)
	{
		desc.Usage = (creationFlags & TCF_AllowCPUAccess) ? D3D11_USAGE_STAGING : D3D11_USAGE_DEFAULT;
		if (creationFlags & TCF_RenderTarget)
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		if (creationFlags & TCF_CreateSRV)
			desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
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

	bool CreateResourceView(ID3D11Device* device, DXGI_FORMAT format, uint32 creationFlags, Texture2DCreationResult& outResult)
	{
		if (creationFlags & TCF_CreateSRV)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};

			switch (format)
			{
			case DXGI_FORMAT_D16_UNORM:
				format = DXGI_FORMAT_R16_UNORM;
				break;
			case DXGI_FORMAT_D32_FLOAT:
				format = DXGI_FORMAT_R32_FLOAT;
				break;
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
				format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
				format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				break;
			}
			desc.Format = format;
			desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
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


	bool D3D11System::createTexture1DInternal(DXGI_FORMAT format, int width, int numMipLevel, uint32 creationFlags, void* data, uint32 pixelSize, Texture1DCreationResult& outResult)
	{
		D3D11_TEXTURE1D_DESC desc = {};
		desc.Format = format;
		desc.Width = width;
		desc.MipLevels = (numMipLevel) ? numMipLevel : 1;
		desc.ArraySize = 1;
		desc.BindFlags = 0;
		SetupTextureDesc(desc, creationFlags);

		D3D11_SUBRESOURCE_DATA initData = {};
		if (data)
		{
			initData.pSysMem = (void *)data;
			initData.SysMemPitch = width * pixelSize;
			initData.SysMemSlicePitch = width * pixelSize;
		}

		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateTexture1D(&desc, data ? &initData : nullptr, &outResult.resource));
		VERIFY_RETURN_FALSE(CreateResourceView(mDevice, format,  creationFlags, outResult));
		return true;
	}

	bool D3D11System::createTexture2DInternal(DXGI_FORMAT format, int width, int height, int numMipLevel, int numSamples, uint32 creationFlags, void* data, uint32 pixelSize, bool bDepth, Texture2DCreationResult& outResult)
	{
		D3D11_TEXTURE2D_DESC desc = {};


		desc.Format = format;
		desc.Width = width;
		desc.Height = height;

		if (creationFlags & TCF_GenerateMips)
			desc.MipLevels = numMipLevel;
		else
			desc.MipLevels = (numMipLevel) ? numMipLevel : 1;
		
		desc.ArraySize = 1;
		desc.BindFlags = (bDepth) ? D3D11_BIND_DEPTH_STENCIL : 0;
		SetupTextureDesc(desc, creationFlags);

		desc.SampleDesc.Count = numSamples;
		desc.SampleDesc.Quality = 0;

		if (creationFlags & TCF_CreateSRV)
		{
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
		}

		std::vector< D3D11_SUBRESOURCE_DATA > initDataList;
		if( data )
		{
#if 0
			if (creationFlags & TCF_GenerateMips)
			{
				initDataList.resize(1);
				auto& initData = initDataList[0];
				initData.pSysMem = (void *)data;
				initData.SysMemPitch = width * pixelSize;
				initData.SysMemSlicePitch = initData.SysMemPitch * height;
			}
			else
#endif
			{
				initDataList.resize(desc.MipLevels);

				int level = 0;
				for (auto& initData : initDataList)
				{
					initData.pSysMem = (void *)data;
					initData.SysMemPitch = (width >> level) * pixelSize;
					initData.SysMemSlicePitch = initData.SysMemPitch * (height >> level);
					++level;
				}
			}
		}

		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateTexture2D(&desc, data ? initDataList.data() : nullptr , &outResult.resource));
		VERIFY_RETURN_FALSE(CreateResourceView(mDevice, format, creationFlags, outResult));

		if (creationFlags & TCF_GenerateMips)
		{
			mDeviceContext->GenerateMips( outResult.SRV );
		}

		return true;
	}


	bool D3D11System::createTexture3DInternal(DXGI_FORMAT format, int width, int height, int depth, int numMipLevel, int numSamples, uint32 creationFlags, void* data, uint32 pixelSize, Texture3DCreationResult& outResult)
	{
		D3D11_TEXTURE3D_DESC desc = {};
		desc.Format = format;
		desc.Width = width;
		desc.Height = height;
		desc.Depth = depth;
		desc.MipLevels = (numMipLevel) ? numMipLevel : 1;
		desc.BindFlags = 0;
		SetupTextureDesc(desc, creationFlags);

		D3D11_SUBRESOURCE_DATA initData = {};
		if (data)
		{
			initData.pSysMem = (void *)data;
			initData.SysMemPitch = width * pixelSize;
			initData.SysMemSlicePitch = initData.SysMemPitch * height;
		}

		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateTexture3D(&desc, data ? &initData : nullptr, &outResult.resource));
		VERIFY_RETURN_FALSE(CreateResourceView(mDevice, format, creationFlags, outResult));
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

	bool D3D11System::createTextureCubeInternal(DXGI_FORMAT format, int size, int numMipLevel, int numSamples, uint32 creationFlags, void* data[], uint32 pixelSize, TextureCubeCreationResult& outResult)
	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Format = format;
		desc.Width = size;
		desc.Height = size;
		desc.MipLevels = (numMipLevel) ? numMipLevel : 1;
		desc.ArraySize = 6;
		desc.BindFlags = 0;
		SetupTextureDesc(desc, creationFlags);
		desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

		desc.SampleDesc.Count = numSamples;
		desc.SampleDesc.Quality = 0;

		D3D11_SUBRESOURCE_DATA initDataList[6];
		if (data)
		{
			for (int i = 0; i < 6; ++i)
			{
				initDataList[i].pSysMem = (void *)data[i];
				initDataList[i].SysMemPitch = size * pixelSize;
				initDataList[i].SysMemSlicePitch = 0;
			}
		}

		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateTexture2D(&desc, data ? &initDataList[0] : nullptr, &outResult.resource));
		VERIFY_RETURN_FALSE(CreateResourceView(mDevice, format, desc.MipLevels, creationFlags, outResult));
		return true;
	}

	bool D3D11System::createStagingTexture(ID3D11Texture2D* texture, TComPtr< ID3D11Texture2D >& outTexture)
	{
		D3D11_TEXTURE2D_DESC desc;
		texture->GetDesc(&desc);
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ ;
		VERIFY_D3D_RESULT_RETURN_FALSE(mDevice->CreateTexture2D(&desc, nullptr, &outTexture));
		return true;
	}

	bool D3D11ResourceBoundState::initialize(TComPtr< ID3D11Device >& device, TComPtr<ID3D11DeviceContext >& deviceContext)
	{
		::memset(this, 0, sizeof(*this));
		for( int i = 0; i < MaxConstBufferNum; ++i )
		{
			if( !mConstBuffers[i].initializeResource(device) )
			{
				return false;
			}
		}
		return true;
	}

	bool D3D11ResourceBoundState::clearTexture(ShaderParameter const& parameter)
	{
		if (mBoundedSRVs[parameter.mLoc] != nullptr)
		{
			mBoundedSRVs[parameter.mLoc] = nullptr;
			mSRVDirtyMask |= BIT(parameter.mLoc);
			if (mMaxSRVBoundIndex == parameter.mLoc)
			{
				--mMaxSRVBoundIndex;
			}

			return true;
		}
		return false;
	}

	void D3D11ResourceBoundState::setTexture(ShaderParameter const& parameter, RHITextureBase& texture)
	{
		auto resViewImpl = static_cast<D3D11ShaderResourceView*>(texture.getBaseResourceView());
		if( resViewImpl )
		{
			if( mBoundedSRVs[parameter.mLoc] != resViewImpl->getResource() )
			{
				mBoundedSRVs[parameter.mLoc] = resViewImpl->getResource();
				mBoundedSRVResources[parameter.mLoc] = &texture;
				mSRVDirtyMask |= BIT(parameter.mLoc);
				if (mMaxSRVBoundIndex < parameter.mLoc)
					mMaxSRVBoundIndex = parameter.mLoc;
			}
		}
		else
		{
			LogWarning(0, "Texture don't have SRV");
		}
	}

	void D3D11ResourceBoundState::setRWTexture(ShaderParameter const& parameter, RHITextureBase* texture)
	{
		ID3D11UnorderedAccessView* UAV = nullptr;
		if (texture)
		{
			switch (texture->getType())
			{
			case ETexture::Type1D:
				{
					auto& textureImpl = static_cast<D3D11Texture1D&>(*texture);
					UAV = textureImpl.mUAV;
				}
				break;
			case ETexture::Type2D:
				{
					auto& textureImpl = static_cast<D3D11Texture2D&>(*texture);
					UAV = textureImpl.mUAV;
				}
				break;
			case ETexture::Type3D:
				{
					auto& textureImpl = static_cast<D3D11Texture3D&>(*texture);
					UAV = textureImpl.mUAV;
				}
				break;
			}
		}

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
			CHECK(mUAVUsageCount >= 0);
			mBoundedUAVs[parameter.mLoc] = UAV;
			mUAVDirtyMask |= BIT(parameter.mLoc);
		}
	}

	void D3D11ResourceBoundState::setSampler(ShaderParameter const& parameter, RHISamplerState& sampler)
	{
		auto& samplerImpl = D3D11Cast::To(sampler);
		if( mBoundedSamplers[parameter.mLoc] != samplerImpl.getResource() )
		{
			mBoundedSamplers[parameter.mLoc] = samplerImpl.getResource();
			mSamplerDirtyMask |= BIT(parameter.mLoc);
		}
	}

	void D3D11ResourceBoundState::setUniformBuffer(ShaderParameter const& parameter, RHIVertexBuffer& buffer)
	{
		D3D11VertexBuffer& bufferImpl = D3D11Cast::To(buffer);
		if( mBoundedConstBuffers[parameter.mLoc] != bufferImpl.mResource )
		{
			mBoundedConstBuffers[parameter.mLoc] = bufferImpl.mResource;
			mConstBufferDirtyMask |= BIT(parameter.mLoc);
		}
	}


	void D3D11ResourceBoundState::setStructuredBuffer(ShaderParameter const& parameter, RHIVertexBuffer& buffer, EAccessOperator op)
	{
		if (op == AO_READ_ONLY)
		{
			ID3D11ShaderResourceView* SRV = static_cast<D3D11VertexBuffer&>(buffer).mSRV;
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
			ID3D11UnorderedAccessView* UAV = static_cast<D3D11VertexBuffer&>(buffer).mUAV;

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
				CHECK(mUAVUsageCount >= 0);
				mBoundedUAVs[parameter.mLoc] = UAV;
				mUAVDirtyMask |= BIT(parameter.mLoc);
			}
		}
	}

	void D3D11ResourceBoundState::setShaderValue(ShaderParameter const& parameter, void const* value, int valueSize)
	{
		assert(parameter.bindIndex < MaxConstBufferNum);
		mConstBuffers[parameter.bindIndex].setUpdateValue(parameter, value, valueSize);
		mConstBufferValueDirtyMask |= BIT(parameter.bindIndex);
		if( mConstBuffers[parameter.bindIndex].resource.get() != mBoundedConstBuffers[parameter.bindIndex] )
		{
			mBoundedConstBuffers[parameter.bindIndex] = mConstBuffers[parameter.bindIndex].resource.get();
			mConstBufferDirtyMask |= BIT(parameter.bindIndex);
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
			for( int index = 0; index < MaxConstBufferNum && mask; ++index )
			{
				if( mask & BIT(index) )
				{
					mask &= ~BIT(index);
					mConstBuffers[index].updateResource(context);
				}
			}
		}

		if( mConstBufferDirtyMask )
		{
			uint32 mask = mConstBufferDirtyMask;
			mConstBufferDirtyMask = 0;
			for( int index = 0; index < MaxSimulatedBoundedBufferNum && mask; ++index )
			{
				if( mask & BIT(index) )
				{
					mask &= ~BIT(index);
					switch( TypeValue )
					{
					case EShader::Vertex:   context->VSSetConstantBuffers(index, 1, mBoundedConstBuffers + index); break;
					case EShader::Pixel:    context->PSSetConstantBuffers(index, 1, mBoundedConstBuffers + index); break;
					case EShader::Geometry: context->GSSetConstantBuffers(index, 1, mBoundedConstBuffers + index); break;
					case EShader::Hull:     context->HSSetConstantBuffers(index, 1, mBoundedConstBuffers + index); break;
					case EShader::Domain:   context->DSSetConstantBuffers(index, 1, mBoundedConstBuffers + index); break;
					case EShader::Compute:  context->CSSetConstantBuffers(index, 1, mBoundedConstBuffers + index); break;
					}
				}
			}
		}

		commitSAVState<TypeValue>(context);

		if (TypeValue == EShader::Compute)
		{
			commitUAVState(context);
		}


		if( mSamplerDirtyMask )
		{
			uint32 mask = mSamplerDirtyMask;
			mSamplerDirtyMask = 0;
			for( int index = 0; index < MaxSimulatedBoundedSamplerNum && mask; ++index )
			{
				if( mask & BIT(index) )
				{
					mask &= ~BIT(index);
					switch( TypeValue )
					{
					case EShader::Vertex:   context->VSSetSamplers(index, 1, mBoundedSamplers + index); break;
					case EShader::Pixel:    context->PSSetSamplers(index, 1, mBoundedSamplers + index); break;
					case EShader::Geometry: context->GSSetSamplers(index, 1, mBoundedSamplers + index); break;
					case EShader::Hull:     context->HSSetSamplers(index, 1, mBoundedSamplers + index); break;
					case EShader::Domain:   context->DSSetSamplers(index, 1, mBoundedSamplers + index); break;
					case EShader::Compute:  context->CSSetSamplers(index, 1, mBoundedSamplers + index); break;
					}
				}
			}
		}
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

				switch (TypeValue)
				{
				case EShader::Vertex:   context->VSSetConstantBuffers(index, 1, &emptyBuffer); break;
				case EShader::Pixel:    context->PSSetConstantBuffers(index, 1, &emptyBuffer); break;
				case EShader::Geometry: context->GSSetConstantBuffers(index, 1, &emptyBuffer); break;
				case EShader::Hull:     context->HSSetConstantBuffers(index, 1, &emptyBuffer); break;
				case EShader::Domain:   context->DSSetConstantBuffers(index, 1, &emptyBuffer); break;
				case EShader::Compute:  context->CSSetConstantBuffers(index, 1, &emptyBuffer); break;
				}
			}
		}
	}

	template< Render::EShader::Type TypeValue >
	void D3D11ResourceBoundState::postDrawPrimitive(ID3D11DeviceContext* context)
	{

	}


	template< Render::EShader::Type TypeValue >
	void D3D11ResourceBoundState::clearSRVResource(ID3D11DeviceContext* context, RHIResource& resource)
	{
		for (int index = mMaxSRVBoundIndex; index >= 0; --index)
		{
			if (mBoundedSRVResources[index] == &resource)
			{
				mBoundedSRVResources[index] == nullptr;
				mBoundedSRVs[index] = nullptr;
				mSRVDirtyMask &= ~BIT(index);
				ID3D11ShaderResourceView* emptySRV = nullptr;

				if (mMaxSRVBoundIndex == index)
					--mMaxSRVBoundIndex;

				switch (TypeValue)
				{
				case EShader::Vertex:   context->VSSetShaderResources(index, 1, &emptySRV); break;
				case EShader::Pixel:    context->PSSetShaderResources(index, 1, &emptySRV); break;
				case EShader::Geometry: context->GSSetShaderResources(index, 1, &emptySRV); break;
				case EShader::Hull:     context->HSSetShaderResources(index, 1, &emptySRV); break;
				case EShader::Domain:   context->DSSetShaderResources(index, 1, &emptySRV); break;
				case EShader::Compute:  context->CSSetShaderResources(index, 1, &emptySRV); break;
				}
			}
		}
	}
	template< EShader::Type TypeValue >
	void D3D11ResourceBoundState::clearSRVResource(ID3D11DeviceContext* context)
	{
		for (int index = 0; index <= mMaxSRVBoundIndex; ++index)
		{
			mBoundedSRVs[index] = nullptr;
		}
		mSRVDirtyMask = 0;
		mMaxSRVBoundIndex = INDEX_NONE;
		switch (TypeValue)
		{
		case EShader::Vertex:   context->VSSetShaderResources(0, MaxSimulatedBoundedSRVNum, mBoundedSRVs); break;
		case EShader::Pixel:    context->PSSetShaderResources(0, MaxSimulatedBoundedSRVNum, mBoundedSRVs); break;
		case EShader::Geometry: context->GSSetShaderResources(0, MaxSimulatedBoundedSRVNum, mBoundedSRVs); break;
		case EShader::Hull:     context->HSSetShaderResources(0, MaxSimulatedBoundedSRVNum, mBoundedSRVs); break;
		case EShader::Domain:   context->DSSetShaderResources(0, MaxSimulatedBoundedSRVNum, mBoundedSRVs); break;
		case EShader::Compute:  context->CSSetShaderResources(0, MaxSimulatedBoundedSRVNum, mBoundedSRVs); break;
		}
	}

	template< EShader::Type TypeValue >
	void D3D11ResourceBoundState::commitSAVState(ID3D11DeviceContext* context)
	{
		if (mSRVDirtyMask)
		{
			uint32 mask = mSRVDirtyMask;
			mSRVDirtyMask = 0;
			for (int index = 0; index <= mMaxSRVBoundIndex && mask; ++index)
			{
				if (mask & BIT(index))
				{
					mask &= ~BIT(index);
					switch (TypeValue)
					{
					case EShader::Vertex:   context->VSSetShaderResources(index, 1, mBoundedSRVs + index); break;
					case EShader::Pixel:    context->PSSetShaderResources(index, 1, mBoundedSRVs + index); break;
					case EShader::Geometry: context->GSSetShaderResources(index, 1, mBoundedSRVs + index); break;
					case EShader::Hull:     context->HSSetShaderResources(index, 1, mBoundedSRVs + index); break;
					case EShader::Domain:   context->DSSetShaderResources(index, 1, mBoundedSRVs + index); break;
					case EShader::Compute:  context->CSSetShaderResources(index, 1, mBoundedSRVs + index); break;
					}
				}
			}
		}
	}

	void D3D11ResourceBoundState::commitUAVState(ID3D11DeviceContext* context)
	{
		if (mUAVDirtyMask)
		{
			uint32 mask = mUAVDirtyMask;
			mUAVDirtyMask = 0;
			for (int index = 0; index < MaxSimulatedBoundedUAVNum && mask; ++index)
			{
				if (mask & BIT(index))
				{
					mask &= ~BIT(index);
					context->CSSetUnorderedAccessViews(index, 1, mBoundedUAVs + index, nullptr);
				}
			}
		}
	}

	bool D3D11Context::initialize(TComPtr< ID3D11Device >& device, TComPtr<ID3D11DeviceContext >& deviceContext)
	{
		mDevice = device;
		mDeviceContext = deviceContext;
		for( int i = 0; i < EShader::Count; ++i )
		{
			mBoundedShaders[i].resource = nullptr;
			mResourceBoundStates[i].initialize(device, deviceContext);
		}

		uint32 dynamicVBufferSize[] = { sizeof(float) * 512 , sizeof(float) * 1024 , sizeof(float) * 1024 * 4 , sizeof(float) * 1024 * 8 };
		mDynamicVBuffer.initialize(device, D3D11_BIND_VERTEX_BUFFER, dynamicVBufferSize, ARRAY_SIZE(dynamicVBufferSize));
		uint32 dynamicIBufferSize[] = { sizeof(uint32) * 3 * 16 , sizeof(uint32) * 3 * 64  , sizeof(uint32) * 3 * 256 , sizeof(uint32) * 3 * 1024 };
		mDynamicIBuffer.initialize(device, D3D11_BIND_INDEX_BUFFER, dynamicIBufferSize, ARRAY_SIZE(dynamicIBufferSize));
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

	void D3D11Context::RHISetViewport(float x, float y, float w, float h, float zNear, float zFar)
	{
		D3D11_VIEWPORT& vp = mViewportState[0];
		vp.TopLeftX = x;
		vp.TopLeftY = y;
		vp.Width = w;
		vp.Height = h;
		vp.MinDepth = zNear;
		vp.MaxDepth = zFar;
		mDeviceContext->RSSetViewports(1, &vp);
	}

	void D3D11Context::RHISetViewports(ViewportInfo const viewports[], int numViewports)
	{
		assert(numViewports < ARRAY_SIZE(mViewportState));
		for (int i = 0; i < numViewports; ++i)
		{
			mViewportState[i].TopLeftX = viewports[i].x;
			mViewportState[i].TopLeftY = viewports[i].y;
			mViewportState[i].Width = viewports[i].w;
			mViewportState[i].Height = viewports[i].h;
			mViewportState[i].MinDepth = viewports[i].zNear;
			mViewportState[i].MaxDepth = viewports[i].zFar;
		}
		mDeviceContext->RSSetViewports(numViewports, mViewportState);
	}

	void D3D11Context::RHISetScissorRect(int x, int y, int w, int h)
	{
		D3D11_VIEWPORT const& vp = mViewportState[0];
		D3D11_RECT rect;	

		rect.left = x;
		rect.right = x + w;
#if 1
		rect.top = vp.Height - (y + h);
		rect.bottom = vp.Height - (y);
#else
		rect.top = y;
		rect.bottom = y + h;
#endif
		mDeviceContext->RSSetScissorRects(1, &rect);
	}

	void D3D11Context::postDrawPrimitive()
	{
#define CLEAR_SHADER_STATE( SHADER_TYPE )\
		mResourceBoundStates[SHADER_TYPE].postDrawPrimitive<SHADER_TYPE>(mDeviceContext.get());

		CLEAR_SHADER_STATE(EShader::Vertex);
		CLEAR_SHADER_STATE(EShader::Pixel);
		CLEAR_SHADER_STATE(EShader::Geometry);
		CLEAR_SHADER_STATE(EShader::Hull);
		CLEAR_SHADER_STATE(EShader::Domain);

#undef CLEAR_SHADER_STATE
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

	void D3D11Context::RHIDrawPrimitiveIndirect(EPrimitive type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
	{
		commitGraphicsShaderState();
		mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(type));
		postDrawPrimitive();
	}

	void D3D11Context::RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
	{
		commitGraphicsShaderState();
		mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(type));
		if (numCommand)
		{
			mDeviceContext->DrawInstancedIndirect(D3D11Cast::GetResource(*commandBuffer), offset);
		}
		else
		{
			//
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

	bool D3D11Context::determitPrimitiveTopologyUP(EPrimitive primitive, int num, uint32 const* pIndices, D3D_PRIMITIVE_TOPOLOGY& outPrimitiveTopology, ID3D11Buffer** outIndexBuffer, int& outIndexNum)
	{
		if(primitive == EPrimitive::Quad )
		{
			int numQuad = num / 4;
			int indexBufferSize = sizeof(uint32) * numQuad * 6;
			void* pIndexBufferData = mDynamicIBuffer.lock(mDeviceContext, indexBufferSize);
			if( pIndexBufferData == nullptr )
				return false;

			uint32* pData = (uint32*)pIndexBufferData;
			if( pIndices )
			{
				for( int i = 0; i < numQuad; ++i )
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
				for( int i = 0; i < numQuad; ++i )
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
			outPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			*outIndexBuffer = mDynamicIBuffer.unlock(mDeviceContext);
			outIndexNum = numQuad * 6;
			return true;
		}
		else if( primitive == EPrimitive::LineLoop )
		{
			if( pIndices )
			{
				int indexBufferSize = sizeof(uint32) * (num + 1);
				void* pIndexBufferData = mDynamicIBuffer.lock(mDeviceContext, indexBufferSize);
				if( pIndexBufferData == nullptr )
					return false;
				uint32* pData = (uint32*)pIndexBufferData;
				for( int i = 0; i < num; ++i )
				{
					pData[i] = pIndices[i];
				}
				pData[num] = pIndices[0];
				*outIndexBuffer = mDynamicIBuffer.unlock(mDeviceContext);
				outIndexNum = num + 1;

			}

			outPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
			return true;
		}
		else if (primitive == EPrimitive::Polygon)
		{
			if (num <= 2)
				return false;

			int numTriangle = ( num - 2 );

			int indexBufferSize = sizeof(uint32) * numTriangle * 3;
			void* pIndexBufferData = mDynamicIBuffer.lock(mDeviceContext, indexBufferSize);
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
			outPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			*outIndexBuffer = mDynamicIBuffer.unlock(mDeviceContext);
			outIndexNum = numTriangle * 3;
			return true;
		}

		outPrimitiveTopology = D3D11Translate::To(primitive);
		if (outPrimitiveTopology == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
			return false;


		if (pIndices)
		{
			uint32 indexBufferSize = num * sizeof(uint32);
			void* pIndexBufferData = mDynamicIBuffer.lock(mDeviceContext, indexBufferSize);
			if (pIndexBufferData == nullptr)
				return false;

			memcpy(pIndexBufferData, pIndices, indexBufferSize);
			*outIndexBuffer = mDynamicIBuffer.unlock(mDeviceContext);
			outIndexNum = num;

		}
		return true;
	}

	void D3D11Context::RHIDrawPrimitiveUP(EPrimitive type, int numVertex, VertexDataInfo dataInfos[], int numVertexData)
	{
		assert(numVertexData <= MAX_INPUT_STREAM_NUM);
		D3D_PRIMITIVE_TOPOLOGY primitiveTopology;
		ID3D11Buffer* indexBuffer = nullptr;
		int numDrawIndex;
		if( !determitPrimitiveTopologyUP(type, numVertex, nullptr, primitiveTopology, &indexBuffer, numDrawIndex) )
			return;

		if( type == EPrimitive::LineLoop )
			++numVertex;
		uint32 vertexBufferSize = 0;
		for( int i = 0; i < numVertexData; ++i )
		{
			vertexBufferSize += (D3D11BUFFER_ALIGN * dataInfos[i].size + D3D11BUFFER_ALIGN - 1) / D3D11BUFFER_ALIGN;
		}

		uint8* pVBufferData = (uint8*)mDynamicVBuffer.lock(mDeviceContext, vertexBufferSize);
		if( pVBufferData )
		{
			commitGraphicsShaderState();

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
					memcpy(pVBufferData + dataOffset, info.ptr, info.size );
					memcpy(pVBufferData + dataOffset + info.size , info.ptr, info.stride);
					dataOffset += (D3D11BUFFER_ALIGN * ( info.size + info.stride ) + D3D11BUFFER_ALIGN - 1) / D3D11BUFFER_ALIGN;
				}
				else
				{
					memcpy(pVBufferData + dataOffset, info.ptr, info.size);
					dataOffset += (D3D11BUFFER_ALIGN * info.size + D3D11BUFFER_ALIGN - 1) / D3D11BUFFER_ALIGN;
				}
			}

			mDynamicVBuffer.unlock(mDeviceContext);
			mDeviceContext->IASetPrimitiveTopology(primitiveTopology);
			mDeviceContext->IASetVertexBuffers(0, numVertexData, vertexBuffers, strides, offsets);
			if( indexBuffer )
			{
				mDeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
				mDeviceContext->DrawIndexed(numDrawIndex, 0, 0);
			}
			else
			{
				mDeviceContext->Draw(numVertex, 0);
			}
			postDrawPrimitive();
		}
	}

	void D3D11Context::RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVertex, VertexDataInfo dataInfos[], int numVertexData, uint32 const* pIndices, int numIndex)
	{
		D3D_PRIMITIVE_TOPOLOGY primitiveTopology;
		ID3D11Buffer* indexBuffer = nullptr;
		int numDrawIndex;
		if( !determitPrimitiveTopologyUP(type, numIndex, pIndices, primitiveTopology, &indexBuffer, numDrawIndex) )
			return;

		uint32 vertexBufferSize = 0;
		for( int i = 0; i < numVertexData; ++i )
		{
			vertexBufferSize += (D3D11BUFFER_ALIGN * dataInfos[i].size + D3D11BUFFER_ALIGN - 1) / D3D11BUFFER_ALIGN;
		}

		uint8* pVBufferData = (uint8*)mDynamicVBuffer.lock(mDeviceContext, vertexBufferSize);
		if( pVBufferData )
		{
			commitGraphicsShaderState();

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

				memcpy(pVBufferData + dataOffset, info.ptr, info.size);
				dataOffset += (D3D11BUFFER_ALIGN * info.size + D3D11BUFFER_ALIGN - 1) / D3D11BUFFER_ALIGN;
			}

			mDynamicVBuffer.unlock(mDeviceContext);
			mDeviceContext->IASetPrimitiveTopology(primitiveTopology);
			mDeviceContext->IASetVertexBuffers(0, numVertexData, vertexBuffers, strides, offsets);

			mDeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
			mDeviceContext->DrawIndexed(numDrawIndex, 0, 0);
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

				mDeviceContext->ClearRenderTargetView(mRenderTargetsState->colorBuffers[i], (numColor == 1) ? colors[0] : colors[i]);
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
			mDeviceContext->ClearDepthStencilView(mRenderTargetsState->depthBuffer, clearFlag, depth, stenceil);
		}
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

#define GRAPHIC_SHADER_LIST(op)\
	op(EShader::Vertex)\
	op(EShader::Pixel)\
	op(EShader::Geometry)\
	op(EShader::Hull)\
	op(EShader::Domain)

	void D3D11Context::commitGraphicsShaderState()
	{
		if (bUseFixedShaderPipeline)
		{
			if (mInputLayout)
			{
				D3D11InputLayout* inputLayoutImpl = D3D11Cast::To(mInputLayout);
				SimplePipelineProgram* program = SimplePipelineProgram::Get( inputLayoutImpl->mAttriableMask , mFixedShaderParams.texture);

				RHISetShaderProgram(program->getRHIResource());
				program->setParameters(RHICommandListImpl(*this),
					mFixedShaderParams.transform, mFixedShaderParams.color, mFixedShaderParams.texture, mFixedShaderParams.sampler);
			}

		}

		if( mBoundedShaderDirtyMask )
		{
#define SET_SHADER_OP( SHADER_TYPE )\
			if( mBoundedShaderDirtyMask & BIT(SHADER_TYPE) )\
			{\
				setShader< SHADER_TYPE >( mBoundedShaders[SHADER_TYPE] );\
			}

			GRAPHIC_SHADER_LIST(SET_SHADER_OP)
			mBoundedShaderDirtyMask = (mBoundedShaderDirtyMask & BIT(EShader::Compute));

#undef SET_SHADER_OP
		}


		if ( mResourceBoundStates[EShader::Pixel].mUAVUsageCount )
		{
			auto& shaderBoundState = mResourceBoundStates[EShader::Pixel];
			mDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
				D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
				0, shaderBoundState.mUAVUsageCount, shaderBoundState.mBoundedUAVs, nullptr);

		}

#define COMMIT_SHADER_STATE_OP( SHADER_TYPE )\
		if( mBoundedShaderMask & BIT(SHADER_TYPE) )\
		{\
			mResourceBoundStates[SHADER_TYPE].commitState<SHADER_TYPE>(mDeviceContext.get());\
		}

		GRAPHIC_SHADER_LIST(COMMIT_SHADER_STATE_OP)

#undef COMMIT_SHADER_STATE_OP

		if (mInputLayout && mVertexShader)
		{
			mDeviceContext->IASetInputLayout(static_cast<D3D11InputLayout*>(mInputLayout)->GetShaderLayout(mDevice, mVertexShader));
		}
	}

	void D3D11Context::commitComputeState()
	{
#if 0
		mShaderBoundState[EShader::Vertex].clearSRVResource<EShader::Vertex>(mDeviceContext);
		mShaderBoundState[EShader::Pixel].clearSRVResource<EShader::Pixel>(mDeviceContext);
		mShaderBoundState[EShader::Hull].clearSRVResource<EShader::Hull>(mDeviceContext);
		mShaderBoundState[EShader::Domain].clearSRVResource<EShader::Domain>(mDeviceContext);
		mShaderBoundState[EShader::Geometry].clearSRVResource<EShader::Geometry>(mDeviceContext);
#endif

		if( mBoundedShaderDirtyMask & BIT(EShader::Compute) )
		{
			setShader<EShader::Compute>(mBoundedShaders[EShader::Compute]);
			mBoundedShaderDirtyMask &= ~BIT(EShader::Compute);
		}
		mResourceBoundStates[EShader::Compute].commitState<EShader::Compute>(mDeviceContext.get());
	}

	void D3D11Context::RHISetShaderProgram(RHIShaderProgram* shaderProgram)
	{
		bUseFixedShaderPipeline = false;

		if( shaderProgram == nullptr )
		{
			mBoundedShaderMask = 0;
			mVertexShader = nullptr;
			for( int i = 0; i < EShader::Count; ++i )
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
			mBoundedShaderMask = 0;
			auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*shaderProgram);

			if (shaderProgramImpl.mNumShaders == 1 )
			{
				auto& shader = *shaderProgramImpl.mShaders[0];
				if (shader.mResource.type == EShader::Compute)
				{
					if (mBoundedShaders[EShader::Compute].resource != shader.mResource.ptr)
					{
						mBoundedShaders[EShader::Compute].resource = shader.mResource.ptr;
						mBoundedShaderDirtyMask |= BIT(EShader::Compute);
					}
					return;
				}
			}

			uint32 setupMask = 0;
			for( int i = 0; i < shaderProgramImpl.mNumShaders; ++i )
			{
				if( !shaderProgramImpl.mShaders[i].isValid() )
					break;

				auto& shader = *shaderProgramImpl.mShaders[i];
				EShader::Type type = shader.mResource.type;

				if (type == EShader::Vertex)
				{
					mVertexShader = &shader;
				}

				setupMask |= BIT(type);
				mBoundedShaderMask |= BIT(type);
				if( mBoundedShaders[type].resource != shader.mResource.ptr )
				{
					mBoundedShaders[type].resource = shader.mResource.ptr;
					mBoundedShaderDirtyMask |= BIT(type);
				}
			}

			for (int i = 0; i < EShader::Count; ++i)
			{
				if (i == EShader::Compute || ( setupMask & BIT(i) ) )
					continue;

				if ( mBoundedShaders[i].resource )
				{
					mBoundedShaders[i].resource = nullptr;
					mBoundedShaderDirtyMask |= BIT(i);
				}
			}
		}
	}

	void D3D11Context::RHISetGraphicsShaderBoundState(GraphicsShaderStateDesc const& stateDesc)
	{
		bUseFixedShaderPipeline = false;

		auto SetupShader = [this](EShader::Type type, RHIShader* shader)
		{
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
				mBoundedShaderMask |= BIT(type);
				if (mBoundedShaders[type].resource != shaderImpl->mResource.ptr)
				{
					mBoundedShaders[type].resource = shaderImpl->mResource.ptr;
					mBoundedShaderDirtyMask |= BIT(type);
				}
			}
		};

		mVertexShader = stateDesc.vertex;

		SetupShader(EShader::Vertex, stateDesc.vertex);
		SetupShader(EShader::Pixel, stateDesc.pixel);
		SetupShader(EShader::Geometry, stateDesc.geometry);
		SetupShader(EShader::Domain, stateDesc.domain);
		SetupShader(EShader::Hull, stateDesc.hull);
	}

	void D3D11Context::RHISetComputeShader(RHIShader* shader)
	{
		ID3D11DeviceChild* resource = nullptr;
		if (shader)
		{
			D3D11Shader* shaderImpl = static_cast<D3D11Shader*>(shader);
			if (shaderImpl->mResource.type != EShader::Compute)
				return;

			resource = shaderImpl->mResource.ptr;
		}
		
		if (mBoundedShaders[EShader::Compute].resource != resource)
		{
			mBoundedShaders[EShader::Compute].resource = resource;
			mBoundedShaderDirtyMask |= BIT(EShader::Compute);
		}
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

	void D3D11Context::setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		switch (op)
		{
		case Render::AO_READ_ONLY:
#if 0
			setShaderTexture(shaderProgram, param, texture);
			break;
#endif
		case Render::AO_WRITE_ONLY:
		case Render::AO_READ_AND_WRITE:
			shaderProgramImpl.setupShader(param, [this, &texture](EShader::Type type, ShaderParameter const& shaderParam)
			{
				mResourceBoundStates[type].setRWTexture(shaderParam, &texture);
			});
			break;
		}
	}

	void D3D11Context::setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op)
	{
		auto& shaderImpl = static_cast<D3D11Shader&>(shader);
		auto type = shaderImpl.mResource.type;
		mResourceBoundStates[type].setRWTexture(param, &texture);
	}

	void D3D11Context::clearShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this](EShader::Type type, ShaderParameter const& shaderParam)
		{
			mResourceBoundStates[type].setRWTexture(shaderParam, nullptr);
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
		mResourceBoundStates[type].setRWTexture(param, nullptr);
		if (type == EShader::Compute)
		{
			mResourceBoundStates[type].commitUAVState(mDeviceContext);
		}
	}

	void D3D11Context::setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &buffer](EShader::Type type, ShaderParameter const& shaderParam)
		{
			mResourceBoundStates[type].setUniformBuffer(shaderParam, buffer);
		});
	}

	void D3D11Context::setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		auto& shaderImpl = static_cast<D3D11Shader&>(shader);
		auto type = shaderImpl.mResource.type;
		mResourceBoundStates[type].setUniformBuffer(param, buffer);
	}

	void D3D11Context::setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer, EAccessOperator op)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &buffer, op](EShader::Type type, ShaderParameter const& shaderParam)
		{
			mResourceBoundStates[type].setStructuredBuffer(shaderParam, buffer, op);
		});
	}

	void D3D11Context::clearSRVResource(RHIResource& resource)
	{
		mResourceBoundStates[EShader::Vertex].clearSRVResource<EShader::Vertex>(mDeviceContext, resource);
		mResourceBoundStates[EShader::Pixel].clearSRVResource<EShader::Pixel>(mDeviceContext, resource);
		mResourceBoundStates[EShader::Geometry].clearSRVResource<EShader::Geometry>(mDeviceContext, resource);
		mResourceBoundStates[EShader::Compute].clearSRVResource<EShader::Compute>(mDeviceContext, resource);
		mResourceBoundStates[EShader::Hull].clearSRVResource<EShader::Hull>(mDeviceContext, resource);
		mResourceBoundStates[EShader::Domain].clearSRVResource<EShader::Domain>(mDeviceContext, resource);
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

	bool ShaderConstDataBuffer::initializeResource(ID3D11Device* device)
	{
		D3D11_BUFFER_DESC bufferDesc = { 0 };
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.ByteWidth = ( 2048 + 15 ) / 16 * 16;
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

	void ShaderConstDataBuffer::setUpdateValue(ShaderParameter const parameter, void const* value, int valueSize)
	{
		int idxDataEnd = parameter.offset + parameter.size;
		if (updateData.size() <= idxDataEnd)
		{
			updateData.resize(idxDataEnd);
		}

		::memcpy(&updateData[parameter.offset], value, parameter.size);
		if (idxDataEnd > updateDataSize)
		{
			updateDataSize = idxDataEnd;
		}
	}

	void ShaderConstDataBuffer::updateResource(ID3D11DeviceContext* context)
	{
		if (updateDataSize)
		{
			context->UpdateSubresource(resource, 0, nullptr, &updateData[0], updateDataSize, updateDataSize);
			updateDataSize = 0;
		}
	}

}//namespace Render

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif