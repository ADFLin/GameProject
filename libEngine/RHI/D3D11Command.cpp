#include "D3D11Command.h"

#include "D3D11ShaderCommon.h"

#include "LogSystem.h"

namespace Render
{

	bool D3D11System::initialize(RHISystemInitParam const& initParam)
	{
		uint32 flag = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
		VERIFY_D3D11RESULT_RETURN_FALSE(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flag, NULL, 0, D3D11_SDK_VERSION, &mDevice, NULL, &mDeviceContext));

		TComPtr< IDXGIDevice > pDXGIDevice;
		VERIFY_D3D11RESULT_RETURN_FALSE( mDevice->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice) );
		TComPtr< IDXGIAdapter > pDXGIAdapter;
		VERIFY_D3D11RESULT_RETURN_FALSE( pDXGIDevice->GetAdapter(&pDXGIAdapter) );

		DXGI_ADAPTER_DESC adapterDesc;
		VERIFY_D3D11RESULT_RETURN_FALSE( pDXGIAdapter->GetDesc(&adapterDesc) );

		switch( adapterDesc.VendorId )
		{
		case 0x10DE: gRHIDeviceVendorName = DeviceVendorName::NVIDIA; break;
		case 0x8086: gRHIDeviceVendorName = DeviceVendorName::Intel; break;
		case 0x1002: gRHIDeviceVendorName = DeviceVendorName::ATI; break;
		}

		mRenderContext.initialize(mDevice, mDeviceContext);
		mImmediateCommandList = new RHICommandListImpl(mRenderContext);
		return true;
	}

	ShaderFormat* D3D11System::createShaderFormat()
	{
		return new ShaderFormatHLSL( mDevice );
	}

	RHITexture2D* D3D11System::RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 createFlags, void* data, int dataAlign)
	{
		Texture2DCreationResult creationResult;
		if ( createTexture2DInternal(D3D11Conv::To(format), w, h, numMipLevel, numSamples, createFlags , data, Texture::GetFormatSize(format), creationResult ) )
		{
			return new D3D11Texture2D(format, creationResult);
		}
		return nullptr;
	}

	RHIVertexBuffer* D3D11System::RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlag, void* data)
	{
		D3D11_SUBRESOURCE_DATA initData = { 0 };
		initData.pSysMem = data;
		initData.SysMemPitch = 0;
		initData.SysMemSlicePitch = 0;

		D3D11_BUFFER_DESC bufferDesc = { 0 };
		bufferDesc.ByteWidth = vertexSize * numVertices;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		TComPtr<ID3D11Buffer> BufferResource;
		VERIFY_D3D11RESULT( 
			mDevice->CreateBuffer(&bufferDesc, &initData, &BufferResource) ,
			{
				return nullptr;
			});

		D3D11VertexBuffer* buffer = new D3D11VertexBuffer( mDevice , BufferResource.release() , creationFlag );

		return buffer;
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
		std::vector< D3D11_INPUT_ELEMENT_DESC > descList;
		for( auto const& e : desc.mElements )
		{
			D3D11_INPUT_ELEMENT_DESC element;

			element.SemanticName = "ATTRIBUTE";
			element.SemanticIndex = e.attribute;
			element.Format = D3D11Conv::To(Vertex::Format( e.format ), e.bNormalize);
			element.InputSlot = e.idxStream;
			element.AlignedByteOffset = e.offset;
			element.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			element.InstanceDataStepRate = 0;

			descList.push_back(element);
		}

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
		for( auto const& desc : descList )
		{
			FixString< 128 > str;
			str.format("%s v%d : ATTRIBUTE%d;", "float4", desc.SemanticIndex, desc.SemanticIndex);
			vertexCode += str.c_str();
		}
		FixString<512> fakeCode;
		fakeCode.format(FakeCodeTemplate, vertexCode.c_str());

		TComPtr< ID3D10Blob > errorCode;
		TComPtr< ID3D10Blob > byteCode;
		FixString<32> profileName = FD3D11Utility::GetShaderProfile( mDevice , Shader::eVertex);

		uint32 compileFlag = 0 /*| D3D10_SHADER_PACK_MATRIX_ROW_MAJOR*/;
		VERIFY_D3D11RESULT(
			D3DX11CompileFromMemory(fakeCode, FCString::Strlen(fakeCode), "ShaderCode", NULL, NULL, SHADER_ENTRY(MainVS), profileName, compileFlag, 0, NULL, &byteCode, &errorCode, NULL),
			{
				LogWarning(0, "Compile Error %s", errorCode->GetBufferPointer());
				return nullptr;
			}
		);

		TComPtr< ID3D11InputLayout > inputLayoutResource;
		VERIFY_D3D11RESULT(
			mDevice->CreateInputLayout(&descList[0], descList.size(), byteCode->GetBufferPointer(), byteCode->GetBufferSize(), &inputLayoutResource),
			return nullptr;
		);

		return new D3D11InputLayout(inputLayoutResource.release() );
	}

	RHIRasterizerState* D3D11System::RHICreateRasterizerState(RasterizerStateInitializer const& initializer)
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11Conv::To(initializer.fillMode);
		desc.CullMode = D3D11Conv::To(initializer.cullMode);
		desc.FrontCounterClockwise = TRUE;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0;
		desc.SlopeScaledDepthBias = 0;
		desc.DepthClipEnable = FALSE;
		desc.ScissorEnable = initializer.bEnableScissor;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;

		TComPtr<ID3D11RasterizerState> state;
		VERIFY_D3D11RESULT(mDevice->CreateRasterizerState(&desc, &state) , );
		if( state )
		{
			return new D3D11RasterizerState(state.release());
		}
		return nullptr;
	}

	RHIBlendState* D3D11System::RHICreateBlendState(BlendStateInitializer const& initializer)
	{
	
		D3D11_BLEND_DESC desc = { 0 };
		desc.AlphaToCoverageEnable = initializer.bEnableAlphaToCoverage;
		desc.IndependentBlendEnable = true;
		for( int i = 0; i < NumBlendStateTarget; ++i )
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

			targetValueD3D11.BlendEnable = (targetValue.srcColor != Blend::eOne) || (targetValue.srcAlpha != Blend::eOne) ||
				(targetValue.destColor != Blend::eZero) || (targetValue.destAlpha != Blend::eZero);
			targetValueD3D11.BlendOp = D3D11Conv::To(targetValue.op);
			targetValueD3D11.SrcBlend = D3D11Conv::To(targetValue.srcColor);
			targetValueD3D11.DestBlend = D3D11Conv::To(targetValue.destColor);
			targetValueD3D11.BlendOpAlpha = D3D11Conv::To(targetValue.opAlpha);
			targetValueD3D11.SrcBlendAlpha = D3D11Conv::To(targetValue.srcAlpha);
			targetValueD3D11.DestBlendAlpha = D3D11Conv::To(targetValue.destAlpha);

		}

		TComPtr< ID3D11BlendState > stateResource;
		VERIFY_D3D11RESULT(mDevice->CreateBlendState(&desc, &stateResource), );
		if( stateResource )
		{
			return new D3D11BlendState(stateResource.release());
		}
		return nullptr;
	}

	RHIShader* D3D11System::RHICreateShader(Shader::Type type)
	{
		return new D3D11Shader;
	}

	RHIShaderProgram* D3D11System::RHICreateShaderProgram()
	{
		return new D3D11ShaderProgram;
	}

	bool D3D11System::createTexture2DInternal(DXGI_FORMAT format, int width, int height, int numMipLevel, int numSamples, uint32 creationFlag, void* data, uint32 pixelSize, Texture2DCreationResult& outResult)
	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Format = format;
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = (numMipLevel) ? numMipLevel : 1;
		desc.ArraySize = 1;

		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = 0;
		if( creationFlag & TCF_RenderTarget )
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		if( creationFlag & TCF_CreateSRV )
			desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
		if( creationFlag & TCF_CreateUAV )
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

		VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->CreateTexture2D(&desc, &initData, &outResult.resource));
		if( creationFlag & TCF_RenderTarget )
		{
			VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->CreateRenderTargetView(outResult.resource, nullptr, &outResult.RTV));
		}
		if( creationFlag & TCF_CreateSRV )
		{
			VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->CreateShaderResourceView(outResult.resource, nullptr, &outResult.SRV));
		}
		if( creationFlag & TCF_CreateUAV )
		{
			VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->CreateUnorderedAccessView(outResult.resource, nullptr, &outResult.UAV));
		}
		return true;
	}


	template< Shader::Type TypeValue >
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
			mConstBufferDirtyMask = 0;
			for( int index = 0; index < MaxSimulatedBoundedBufferNum && mask; ++index )
			{
				if( mask & BIT(index) )
				{
					mask &= ~BIT(index);
					switch( TypeValue )
					{
					case Shader::eVertex:   context->VSSetConstantBuffers(index, 1, mBoundedBuffers + index); break;
					case Shader::ePixel:    context->PSSetConstantBuffers(index, 1, mBoundedBuffers + index); break;
					case Shader::eGeometry: context->GSSetConstantBuffers(index, 1, mBoundedBuffers + index); break;
					case Shader::eHull:     context->HSSetConstantBuffers(index, 1, mBoundedBuffers + index); break;
					case Shader::eDomain:   context->DSSetConstantBuffers(index, 1, mBoundedBuffers + index); break;
					case Shader::eCompute:  context->CSSetConstantBuffers(index, 1, mBoundedBuffers + index); break;
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
					case Shader::eVertex:   context->VSSetShaderResources(index, 1, mBoundedSRVs + index); break;
					case Shader::ePixel:    context->PSSetShaderResources(index, 1, mBoundedSRVs + index); break;
					case Shader::eGeometry: context->GSSetShaderResources(index, 1, mBoundedSRVs + index); break;
					case Shader::eHull:     context->HSSetShaderResources(index, 1, mBoundedSRVs + index); break;
					case Shader::eDomain:   context->DSSetShaderResources(index, 1, mBoundedSRVs + index); break;
					case Shader::eCompute:  context->CSSetShaderResources(index, 1, mBoundedSRVs + index); break;
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
					case Shader::eVertex:   context->VSSetSamplers(index, 1, mBoundedSamplers + index); break;
					case Shader::ePixel:    context->PSSetSamplers(index, 1, mBoundedSamplers + index); break;
					case Shader::eGeometry: context->GSSetSamplers(index, 1, mBoundedSamplers + index); break;
					case Shader::eHull:     context->HSSetSamplers(index, 1, mBoundedSamplers + index); break;
					case Shader::eDomain:   context->DSSetSamplers(index, 1, mBoundedSamplers + index); break;
					case Shader::eCompute:  context->CSSetSamplers(index, 1, mBoundedSamplers + index); break;
					}
				}
			}
		}
	}

	void D3D11Context::RHISetShaderProgram(RHIShaderProgram* shaderProgram)
	{
		if( shaderProgram == nullptr )
		{
			mBoundedShaderMask = 0;
			for( int i = 0; i < Shader::Count; ++i )
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
			for( int i = 0; i < Shader::Count; ++i )
			{
				if( !shaderProgramImpl.mShaders[i].isValid() )
					break;

				auto& shaderImpl = static_cast<D3D11Shader&>(*shaderProgramImpl.mShaders[i]);
				auto  type = shaderImpl.mResource.type;

				mBoundedShaderMask |= BIT(type);
				if( mBoundedShaders[type].resource != shaderImpl.mResource.ptr )
				{
					mBoundedShaders[type].resource = shaderImpl.mResource.ptr;
					mBoundedShaderDirtyMask |= BIT(type);
				}
			}
		}
	}

	template< Shader::Type TypeValue >
	void D3D11Context::setShader(D3D11ShaderVariant  const& shaderVariant)
	{
		switch( TypeValue )
		{
		case Shader::eVertex:   mDeviceContext->VSSetShader(shaderVariant.vertex, nullptr, 0); break;
		case Shader::ePixel:    mDeviceContext->PSSetShader(shaderVariant.pixel, nullptr, 0); break;
		case Shader::eGeometry: mDeviceContext->GSSetShader(shaderVariant.geometry, nullptr, 0); break;
		case Shader::eHull:     mDeviceContext->HSSetShader(shaderVariant.hull, nullptr, 0); break;
		case Shader::eDomain:   mDeviceContext->DSSetShader(shaderVariant.domain, nullptr, 0); break;
		case Shader::eCompute:  mDeviceContext->CSSetShader(shaderVariant.compute, nullptr, 0); break;
		}
	}

	template < class ValueType >
	void D3D11Context::setShaderValueT(RHIShaderProgram& shaderProgram, ShaderParameter const& param, ValueType const val[], int dim)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(shaderProgram);
		shaderProgramImpl.setupShader(param, [this, val, dim](Shader::Type type, ShaderParameter const& shaderParam)
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
		shaderProgramImpl.setupShader(param, [this, &texture](Shader::Type type, ShaderParameter const& shaderParam)
		{
			mShaderBoundState[type].setTexture(shaderParam, texture);
		});
	}

	void D3D11Context::commitRenderShaderState()
	{
#define SET_SHADER( SHADER_TYPE )\
			if( mBoundedShaderDirtyMask & BIT(SHADER_TYPE) )\
			{\
				setShader< SHADER_TYPE >( mBoundedShaders[SHADER_TYPE] );\
			}

		if( mBoundedShaderDirtyMask )
		{
			SET_SHADER(Shader::eVertex);
			SET_SHADER(Shader::ePixel);
			SET_SHADER(Shader::eGeometry);
			SET_SHADER(Shader::eHull);
			SET_SHADER(Shader::eDomain);
			mBoundedShaderDirtyMask = ( mBoundedShaderDirtyMask & BIT(Shader::eCompute) );
		}
#define COMMIT_SHADER_STATE( SHADER_TYPE )\
		if( mBoundedShaderMask & BIT(SHADER_TYPE) )\
		{\
			mShaderBoundState[SHADER_TYPE].commitState<SHADER_TYPE>(mDeviceContext.get());\
		}

		COMMIT_SHADER_STATE(Shader::eVertex);
		COMMIT_SHADER_STATE(Shader::ePixel);
		COMMIT_SHADER_STATE(Shader::eGeometry);
		COMMIT_SHADER_STATE(Shader::eHull);
		COMMIT_SHADER_STATE(Shader::eDomain);

#undef SET_SHADER
#undef COMMIT_SHADER_STATE
	}

	void D3D11Context::commitComputeState()
	{
		if( mBoundedShaderDirtyMask & BIT(Shader::eCompute) )
		{
			setShader<Shader::eCompute>(mBoundedShaders[Shader::eCompute]);
			mBoundedShaderDirtyMask &= ~BIT(Shader::eCompute);
		}
		mShaderBoundState[Shader::eCompute].commitState<Shader::eCompute>(mDeviceContext.get());
	}

}//namespace Render