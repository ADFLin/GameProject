#include "D3D11Command.h"

#include "D3D11ShaderCommon.h"

#include "LogSystem.h"
#include "GpuProfiler.h"

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
		uint32 flag = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
		VERIFY_D3D11RESULT_RETURN_FALSE(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flag, NULL, 0, D3D11_SDK_VERSION, &mDevice, NULL, &mDeviceContext));

		TComPtr< IDXGIDevice > pDXGIDevice;
		VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice));
		TComPtr< IDXGIAdapter > pDXGIAdapter;
		VERIFY_D3D11RESULT_RETURN_FALSE(pDXGIDevice->GetAdapter(&pDXGIAdapter));

		DXGI_ADAPTER_DESC adapterDesc;
		VERIFY_D3D11RESULT_RETURN_FALSE(pDXGIAdapter->GetDesc(&adapterDesc));

		switch( adapterDesc.VendorId )
		{
		case 0x10DE: gRHIDeviceVendorName = DeviceVendorName::NVIDIA; break;
		case 0x8086: gRHIDeviceVendorName = DeviceVendorName::Intel; break;
		case 0x1002: gRHIDeviceVendorName = DeviceVendorName::ATI; break;
		}

		gRHIClipZMin = 0;
		gRHIProjectYSign = -1;
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

	ShaderFormat* D3D11System::createShaderFormat()
	{
		return new ShaderFormatHLSL(mDevice);
	}

	RHITexture2D* D3D11System::RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 createFlags, void* data, int dataAlign)
	{
		Texture2DCreationResult creationResult;
		if( createTexture2DInternal(D3D11Translate::To(format), w, h, numMipLevel, numSamples, createFlags, data, Texture::GetFormatSize(format), false, creationResult) )
		{
			return new D3D11Texture2D(format, creationResult);
		}
		return nullptr;
	}

	RHITextureDepth* D3D11System::RHICreateTextureDepth(Texture::DepthFormat format, int w, int h, int numMipLevel, int numSamples)
	{
		uint32 creationFlags = 0;
		Texture2DCreationResult creationResult;
		if( createTexture2DInternal(D3D11Translate::To(format), w, h, numMipLevel, numSamples, creationFlags, nullptr, 0, true, creationResult) )
		{
			return new D3D11TextureDepth(format, creationResult);
		}

		return nullptr;
	}

	RHIVertexBuffer* D3D11System::RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		D3D11_SUBRESOURCE_DATA initData = { 0 };
		initData.pSysMem = data;
		initData.SysMemPitch = 0;
		initData.SysMemSlicePitch = 0;

		D3D11_BUFFER_DESC bufferDesc = { 0 };
		bufferDesc.ByteWidth = vertexSize * numVertices;
		bufferDesc.Usage = (creationFlags & BCF_UsageDynamic) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		if (creationFlags & BCF_UsageConst)
		{
			bufferDesc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
		}
		else
		{
			bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			if (creationFlags & BCF_CreateSRV)
				bufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
		}

		bufferDesc.CPUAccessFlags = (creationFlags & BCF_UsageDynamic) ? D3D11_CPU_ACCESS_WRITE : 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		TComPtr<ID3D11Buffer> BufferResource;
		VERIFY_D3D11RESULT(
			mDevice->CreateBuffer(&bufferDesc, data ? &initData : nullptr, &BufferResource),
			{
				return nullptr;
			}
		);

		D3D11VertexBuffer* buffer = new D3D11VertexBuffer(mDevice, BufferResource.release(), numVertices, vertexSize, creationFlags);

		return buffer;
	}

	Render::RHIIndexBuffer* D3D11System::RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
	{
		D3D11_SUBRESOURCE_DATA initData = { 0 };
		initData.pSysMem = data;
		initData.SysMemPitch = 0;
		initData.SysMemSlicePitch = 0;

		D3D11_BUFFER_DESC bufferDesc = { 0 };
		bufferDesc.ByteWidth = nIndices * (bIntIndex ? 4 : 2);
		bufferDesc.Usage = (creationFlags & BCF_UsageDynamic) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		if( creationFlags & BCF_CreateSRV )
			bufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

		bufferDesc.CPUAccessFlags = (creationFlags & BCF_UsageDynamic) ? D3D11_CPU_ACCESS_WRITE : 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		TComPtr<ID3D11Buffer> BufferResource;
		VERIFY_D3D11RESULT(
			mDevice->CreateBuffer(&bufferDesc, data ? &initData : nullptr, &BufferResource),
			{
				return nullptr;
			});

		D3D11IndexBuffer* buffer = new D3D11IndexBuffer(mDevice, BufferResource.release(), nIndices, bIntIndex, creationFlags);
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

		FixString<512> fakeCode;
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
				if (e.attribute == Vertex::ATTRIBUTE_UNUSED)
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
				FixString< 128 > str;
				str.format("float%d v%d : ATTRIBUTE%d;", Vertex::GetComponentNum(e->format), e->attribute, e->attribute);
				vertexCode += str.c_str();
			}

			fakeCode.format(FakeCodeTemplate, vertexCode.c_str());
		}

		TComPtr< ID3D10Blob > errorCode;
		TComPtr< ID3D10Blob > byteCode;
		FixString<32> profileName = FD3D11Utility::GetShaderProfile(mDevice, EShader::Vertex);

		uint32 compileFlag = 0 /*| D3D10_SHADER_PACK_MATRIX_ROW_MAJOR*/;
		VERIFY_D3D11RESULT(
			D3DX11CompileFromMemory(fakeCode, FCString::Strlen(fakeCode), "ShaderCode", NULL, NULL, SHADER_ENTRY(MainVS), profileName, compileFlag, 0, NULL, &byteCode, &errorCode, NULL),
			{
				LogWarning(0, "Compile Error %s", errorCode->GetBufferPointer());
				return nullptr;
			}
		);

		D3D11InputLayout* inputLayout = new D3D11InputLayout;
		for (auto const& e : desc.mElements)
		{
			if (e.attribute == Vertex::ATTRIBUTE_UNUSED)
				continue;

			D3D11_INPUT_ELEMENT_DESC element;
			element.SemanticName = "ATTRIBUTE";
			element.SemanticIndex = e.attribute;
			element.Format = D3D11Translate::To(Vertex::Format(e.format), e.bNormalized);
			element.InputSlot = e.idxStream;
			element.AlignedByteOffset = e.offset;
			element.InputSlotClass = e.bIntanceData ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
			element.InstanceDataStepRate = e.bIntanceData ? e.instanceStepRate : 0;

			inputLayout->mDescList.push_back(element);
		}

		TComPtr< ID3D11InputLayout > inputLayoutResource;
		VERIFY_D3D11RESULT(
			mDevice->CreateInputLayout(inputLayout->mDescList.data(), inputLayout->mDescList.size(), byteCode->GetBufferPointer(), byteCode->GetBufferSize(), &inputLayoutResource),
			return nullptr;
		);
		inputLayout->mUniversalResource = inputLayoutResource.release();
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

		TComPtr<ID3D11SamplerState> samplerResource;
		VERIFY_D3D11RESULT( mDevice->CreateSamplerState(&desc , &samplerResource) , );
		if( samplerResource )
		{
			return new D3D11SamplerState(samplerResource.release());
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
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;

		TComPtr<ID3D11RasterizerState> stateResource;
		VERIFY_D3D11RESULT(mDevice->CreateRasterizerState(&desc, &stateResource) , );
		if( stateResource )
		{
			return new D3D11RasterizerState(stateResource.release());
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
		VERIFY_D3D11RESULT(mDevice->CreateBlendState(&desc, &stateResource), );
		if( stateResource )
		{
			return new D3D11BlendState(stateResource.release());
		}
		return nullptr;
	}

	Render::RHIDepthStencilState* D3D11System::RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer)
	{
		D3D11_DEPTH_STENCIL_DESC desc;
		desc.DepthEnable = (initializer.depthFunc != ECompareFunc::Always) || (initializer.bWriteDepth);
		desc.DepthWriteMask = initializer.bWriteDepth ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = D3D11Translate::To( initializer.depthFunc );
		desc.StencilEnable = initializer.bEnableStencilTest;
		desc.StencilReadMask = initializer.stencilReadMask;
		desc.StencilWriteMask = initializer.stencilWriteMask;

		desc.FrontFace.StencilFunc = D3D11Translate::To(initializer.stencilFunc);
		desc.FrontFace.StencilPassOp = D3D11Translate::To(initializer.zPassOp);
		desc.FrontFace.StencilDepthFailOp = D3D11Translate::To( initializer.zFailOp );
		desc.FrontFace.StencilFailOp = D3D11Translate::To( initializer.stencilFailOp );
	
		desc.BackFace.StencilFunc = D3D11Translate::To(initializer.stencilFunBack);
		desc.BackFace.StencilPassOp = D3D11Translate::To(initializer.zPassOpBack);
		desc.BackFace.StencilDepthFailOp = D3D11Translate::To(initializer.zFailOpBack);
		desc.BackFace.StencilFailOp = D3D11Translate::To(initializer.stencilFailOpBack);
		
		TComPtr< ID3D11DepthStencilState > stateResource;
		VERIFY_D3D11RESULT(mDevice->CreateDepthStencilState(&desc, &stateResource), );
		if( stateResource )
		{
			return new D3D11DepthStencilState(stateResource.release());
		}
		return nullptr;
	}

	RHIShader* D3D11System::RHICreateShader(EShader::Type type)
	{
		return new D3D11Shader;
	}

	RHIShaderProgram* D3D11System::RHICreateShaderProgram()
	{
		return new D3D11ShaderProgram;
	}

	bool D3D11System::createTexture2DInternal(DXGI_FORMAT format, int width, int height, int numMipLevel, int numSamples, uint32 creationFlags, void* data, uint32 pixelSize, bool bDepth , Texture2DCreationResult& outResult)
	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Format = format;
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = (numMipLevel) ? numMipLevel : 1;
		desc.ArraySize = 1;

		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = (bDepth) ? D3D11_BIND_DEPTH_STENCIL : 0;
		if( creationFlags & TCF_RenderTarget )
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		if( creationFlags & TCF_CreateSRV )
			desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
		if( creationFlags & TCF_CreateUAV )
			desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;


		desc.SampleDesc.Count = numSamples;
		desc.SampleDesc.Quality = 0;
		desc.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA initData = {};
		if( data )
		{
			initData.pSysMem = (void *)data;
			initData.SysMemPitch = width * pixelSize;
			initData.SysMemSlicePitch = width * height * pixelSize;
		}

		VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->CreateTexture2D(&desc, data ? &initData : nullptr , &outResult.resource));
		if( creationFlags & TCF_RenderTarget )
		{
			VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->CreateRenderTargetView(outResult.resource, nullptr, &outResult.RTV));
		}
		if( creationFlags & TCF_CreateSRV )
		{
			VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->CreateShaderResourceView(outResult.resource, nullptr, &outResult.SRV));
		}
		if( creationFlags & TCF_CreateUAV )
		{
			VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->CreateUnorderedAccessView(outResult.resource, nullptr, &outResult.UAV));
		}
		return true;
	}


	bool D3D11ShaderBoundState::initialize(TComPtr< ID3D11Device >& device, TComPtr<ID3D11DeviceContext >& deviceContext)
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

	void D3D11ShaderBoundState::setTexture(ShaderParameter const& parameter, RHITextureBase& texture)
	{
		auto resViewImpl = static_cast<D3D11ShaderResourceView*>(texture.getBaseResourceView());
		if( resViewImpl )
		{
			if( mBoundedSRVs[parameter.mLoc] != resViewImpl->getResource() )
			{
				mBoundedSRVs[parameter.mLoc] = resViewImpl->getResource();
				mSRVDirtyMask |= BIT(parameter.mLoc);
			}
		}
	}

	void D3D11ShaderBoundState::setSampler(ShaderParameter const& parameter, RHISamplerState& sampler)
	{
		auto& samplerImpl = D3D11Cast::To(sampler);
		if( mBoundedSamplers[parameter.mLoc] != samplerImpl.getResource() )
		{
			mBoundedSamplers[parameter.mLoc] = samplerImpl.getResource();
			mSamplerDirtyMask |= BIT(parameter.mLoc);
		}
	}

	void D3D11ShaderBoundState::setUniformBuffer(ShaderParameter const& parameter, RHIVertexBuffer& buffer)
	{
		D3D11VertexBuffer& bufferImpl = D3D11Cast::To(buffer);
		if( mBoundedConstBuffers[parameter.mLoc] != bufferImpl.mResource )
		{
			mBoundedConstBuffers[parameter.mLoc] = bufferImpl.mResource;
			mConstBufferDirtyMask |= BIT(parameter.mLoc);
		}

	}

	void D3D11ShaderBoundState::setShaderValue(ShaderParameter const& parameter, void const* value, int valueSize)
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
	void D3D11ShaderBoundState::commitState(ID3D11DeviceContext* context)
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
			//mConstBufferDirtyMask = 0;
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

		if( mSRVDirtyMask)
		{
			uint32 mask = mSRVDirtyMask;
			mSRVDirtyMask = 0;
			for( int index = 0; index < MaxSimulatedBoundedSRVNum && mask; ++index )
			{
				if( mask & BIT(index) )
				{
					mask &= ~BIT(index);
					switch( TypeValue )
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
	void D3D11ShaderBoundState::clearState(ID3D11DeviceContext* context)
	{
		for( int index = 0; index < 2; ++index )
		{
			ID3D11Buffer* emptyBuffer = nullptr;
			switch( TypeValue )
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


	bool D3D11Context::initialize(TComPtr< ID3D11Device >& device, TComPtr<ID3D11DeviceContext >& deviceContext)
	{
		mDevice = device;
		mDeviceContext = deviceContext;
		for( int i = 0; i < EShader::Count; ++i )
		{
			mBoundedShaders[i].resource = nullptr;
			mShaderBoundState[i].initialize(device, deviceContext);
		}

		uint32 dynamicVBufferSize[] = { sizeof(float) * 512 , sizeof(float) * 1024 , sizeof(float) * 1024 * 4 , sizeof(float) * 1024 * 8 };
		mDynamicVBuffer.initialize(device, D3D11_BIND_VERTEX_BUFFER, dynamicVBufferSize, ARRAY_SIZE(dynamicVBufferSize));
		uint32 dynamicIBufferSize[] = { sizeof(uint32) * 3 * 16 , sizeof(uint32) * 3 * 64  , sizeof(uint32) * 3 * 256 , sizeof(uint32) * 3 * 1024 };
		mDynamicIBuffer.initialize(device, D3D11_BIND_INDEX_BUFFER, dynamicIBufferSize, ARRAY_SIZE(dynamicIBufferSize));
		return true;
	}

	void D3D11Context::release()
	{
		mDynamicVBuffer.release();
		mDynamicIBuffer.release();
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

	void D3D11Context::RHISetViewport(int x, int y, int w, int h, float zNear, float zFar)
	{
		D3D11_VIEWPORT vp;
		vp.Width = w;
		vp.Height = h;
		vp.MinDepth = zNear;
		vp.MaxDepth = zFar;
		vp.TopLeftX = x;
		vp.TopLeftY = y;
		mDeviceContext->RSSetViewports(1, &vp);
	}

	void D3D11Context::RHISetScissorRect(int x, int y, int w, int h)
	{
		D3D11_RECT rect;
		rect.top = x;
		rect.left = y;
		rect.bottom = x + w;
		rect.right = y + h;
		mDeviceContext->RSSetScissorRects(1, &rect);
	}

	void D3D11Context::PostDrawPrimitive()
	{
		return;
#define CLEAR_SHADER_STATE( SHADER_TYPE )\
		mShaderBoundState[SHADER_TYPE].clearState<SHADER_TYPE>(mDeviceContext.get());

		CLEAR_SHADER_STATE(EShader::Vertex);
		CLEAR_SHADER_STATE(EShader::Pixel);
		CLEAR_SHADER_STATE(EShader::Geometry);
		CLEAR_SHADER_STATE(EShader::Hull);
		CLEAR_SHADER_STATE(EShader::Domain);

#undef CLEAR_SHADER_STATE
	}


	bool D3D11Context::determitPrimitiveTopologyUP(EPrimitive EPrimitive, int num, int const* pIndices, D3D_PRIMITIVE_TOPOLOGY& outPrimitiveTopology, ID3D11Buffer** outIndexBuffer, int& outIndexNum)
	{
		if( EPrimitive == EPrimitive::Quad )
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
					pData[0] = i + 0;
					pData[1] = i + 1;
					pData[2] = i + 2;
					pData[3] = i + 0;
					pData[4] = i + 2;
					pData[5] = i + 3;
					pData += 6;
				}
			}
			outPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			*outIndexBuffer = mDynamicIBuffer.unlock(mDeviceContext);
			outIndexNum = numQuad * 6;
			return true;
		}
		else if( EPrimitive == EPrimitive::LineLoop )
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

		outPrimitiveTopology = D3D11Translate::To(EPrimitive);
		if( outPrimitiveTopology != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED )
		{
			if( pIndices )
			{
				uint32 indexBufferSize = num * sizeof(uint32);
				void* pIndexBufferData = mDynamicIBuffer.lock(mDeviceContext, indexBufferSize);
				if( pIndexBufferData == nullptr )
					return false;

				memcpy(pIndexBufferData, pIndices, indexBufferSize);
				*outIndexBuffer = mDynamicIBuffer.unlock(mDeviceContext);
			}
			return true;
		}

		return false;
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
			commitRenderShaderState();

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
					memcpy(pVBufferData + dataOffset, info.ptr, info.size + info.stride);
					dataOffset += info.size + info.stride;
				}
				else
				{
					memcpy(pVBufferData + dataOffset, info.ptr, info.size);
					dataOffset += info.size;
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
			PostDrawPrimitive();
		}
	}

	void D3D11Context::RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVertex, VertexDataInfo dataInfos[], int numVertexData, int const* pIndices, int numIndex)
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

		void* pVBufferData = mDynamicVBuffer.lock(mDeviceContext, vertexBufferSize);
		if( pVBufferData )
		{
			commitRenderShaderState();

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

				memcpy(pVBufferData, info.ptr, info.size);
				dataOffset += info.size;
			}

			mDynamicVBuffer.unlock(mDeviceContext);
			mDeviceContext->IASetPrimitiveTopology(primitiveTopology);
			mDeviceContext->IASetVertexBuffers(0, numVertexData, vertexBuffers, strides, offsets);

			mDeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
			mDeviceContext->DrawIndexed(numDrawIndex, 0, 0);
			PostDrawPrimitive();
		}
	}

	void D3D11Context::commitRenderShaderState()
	{
#define CLEAR_SHADER_STATE( SHADER_TYPE )\
		mShaderBoundState[SHADER_TYPE].clearState<SHADER_TYPE>(mDeviceContext.get());

		CLEAR_SHADER_STATE(EShader::Vertex);
		CLEAR_SHADER_STATE(EShader::Pixel);
		CLEAR_SHADER_STATE(EShader::Geometry);
		CLEAR_SHADER_STATE(EShader::Hull);
		CLEAR_SHADER_STATE(EShader::Domain);

#undef CLEAR_SHADER_STATE

#define SET_SHADER( SHADER_TYPE )\
			if( mBoundedShaderDirtyMask & BIT(SHADER_TYPE) )\
			{\
				setShader< SHADER_TYPE >( mBoundedShaders[SHADER_TYPE] );\
			}


		if (mInputLayout && mVertexShader)
		{
			mDeviceContext->IASetInputLayout(static_cast<D3D11InputLayout*>(mInputLayout)->GetShaderLayout(mDevice, mVertexShader));
		}

		if( mBoundedShaderDirtyMask )
		{
			SET_SHADER(EShader::Vertex);
			SET_SHADER(EShader::Pixel);
			SET_SHADER(EShader::Geometry);
			SET_SHADER(EShader::Hull);
			SET_SHADER(EShader::Domain);
			mBoundedShaderDirtyMask = (mBoundedShaderDirtyMask & BIT(EShader::Compute));
		}
#define COMMIT_SHADER_STATE( SHADER_TYPE )\
		if( mBoundedShaderMask & BIT(SHADER_TYPE) )\
		{\
			mShaderBoundState[SHADER_TYPE].commitState<SHADER_TYPE>(mDeviceContext.get());\
		}

		COMMIT_SHADER_STATE(EShader::Vertex);
		COMMIT_SHADER_STATE(EShader::Pixel);
		COMMIT_SHADER_STATE(EShader::Geometry);
		COMMIT_SHADER_STATE(EShader::Hull);
		COMMIT_SHADER_STATE(EShader::Domain);

#undef SET_SHADER
#undef COMMIT_SHADER_STATE
	}

	void D3D11Context::commitComputeState()
	{
		if( mBoundedShaderDirtyMask & BIT(EShader::Compute) )
		{
			setShader<EShader::Compute>(mBoundedShaders[EShader::Compute]);
			mBoundedShaderDirtyMask &= ~BIT(EShader::Compute);
		}
		mShaderBoundState[EShader::Compute].commitState<EShader::Compute>(mDeviceContext.get());
	}

	void D3D11Context::RHISetShaderProgram(RHIShaderProgram* shaderProgram)
	{
		if( shaderProgram == nullptr )
		{
			mBoundedShaderMask = 0;
			mVertexShader = nullptr;
			for( int i = 0; i < EShader::Count; ++i )
			{
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
			for( int i = 0; i < EShader::Count; ++i )
			{
				if( !shaderProgramImpl.mShaders[i].isValid() )
					break;

				auto& shader = *shaderProgramImpl.mShaders[i];
				auto  type = shader.mResource.type;

				if (type == EShader::Vertex)
				{
					mVertexShader = &shader;
				}

				mBoundedShaderMask |= BIT(type);
				if( mBoundedShaders[type].resource != shader.mResource.ptr )
				{
					mBoundedShaders[type].resource = shader.mResource.ptr;
					mBoundedShaderDirtyMask |= BIT(type);
				}
			}
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
			mShaderBoundState[type].setShaderValue(shaderParam, val, sizeof(ValueType) * dim);
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
			mShaderBoundState[type].setTexture(shaderParam, texture);
		});
	}

	void D3D11Context::setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &sampler](EShader::Type type, ShaderParameter const& shaderParam)
		{
			mShaderBoundState[type].setSampler(shaderParam, sampler);
		});
	}

	void D3D11Context::setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, &buffer](EShader::Type type, ShaderParameter const& shaderParam)
		{
			mShaderBoundState[type].setUniformBuffer(shaderParam, buffer);
		});
	}

	template < class ValueType >
	void D3D11Context::setShaderValueT(RHIShader& shader, ShaderParameter const& param, ValueType const val[], int dim)
	{
		auto& shaderImpl = static_cast<D3D11Shader&>(shader);
		mShaderBoundState[shaderImpl.mResource.type].setShaderValue(param, val, sizeof(ValueType) * dim);
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

	void D3D11Context::setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector3 const val[], int dim)
	{
		setShaderValueT(shader, param, val, dim);
	}

	void D3D11Context::setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector4 const val[], int dim)
	{
		setShaderValueT(shader, param, val, dim);
	}

}//namespace Render

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif