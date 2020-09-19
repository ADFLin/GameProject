
#include "D3D12Common.h"
#include "D3D12ShaderCommon.h"

#pragma comment(lib , "D3D12.lib")
#pragma comment(lib , "DXGI.lib")
#pragma comment(lib , "dxguid.lib")

namespace Render
{
	D3D12_BLEND D3D12Translate::To(Blend::Factor factor)
	{
		switch( factor )
		{
		case Blend::eOne: return D3D12_BLEND_ONE;
		case Blend::eZero: return D3D12_BLEND_ZERO;
		case Blend::eSrcAlpha: return D3D12_BLEND_SRC_ALPHA;
		case Blend::eOneMinusSrcAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
		case Blend::eDestAlpha: return D3D12_BLEND_DEST_ALPHA;
		case Blend::eOneMinusDestAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
		case Blend::eSrcColor: return  D3D12_BLEND_SRC_COLOR;
		case Blend::eOneMinusSrcColor: return D3D12_BLEND_INV_SRC_COLOR;
		case Blend::eDestColor: return D3D12_BLEND_DEST_COLOR;
		case Blend::eOneMinusDestColor: return D3D12_BLEND_INV_DEST_COLOR;
		default:
			break;
		}
		return D3D12_BLEND_ONE;
	}

	D3D12_BLEND_OP D3D12Translate::To(Blend::Operation op)
	{
		switch( op )
		{
		case Blend::eAdd: return D3D12_BLEND_OP_ADD;
		case Blend::eSub: return D3D12_BLEND_OP_SUBTRACT;
		case Blend::eReverseSub: return D3D12_BLEND_OP_REV_SUBTRACT;
		case Blend::eMax: return D3D12_BLEND_OP_MAX;
		case Blend::eMin: return D3D12_BLEND_OP_MIN;
		}
		return D3D12_BLEND_OP_ADD;
	}


	D3D12_CULL_MODE D3D12Translate::To(ECullMode mode)
	{
		switch( mode )
		{
		case ECullMode::Front: return D3D12_CULL_MODE_FRONT;
		case ECullMode::Back:  return D3D12_CULL_MODE_BACK;
		case ECullMode::None:  return D3D12_CULL_MODE_NONE;
		}
		return D3D12_CULL_MODE_NONE;
	}

	D3D12_FILL_MODE D3D12Translate::To(EFillMode mode)
	{
		switch( mode )
		{
		case EFillMode::Wireframe: return D3D12_FILL_MODE_WIREFRAME;
		case EFillMode::Solid: return D3D12_FILL_MODE_SOLID;
		case EFillMode::System:
		case EFillMode::Point:
			return D3D12_FILL_MODE_SOLID;
		}
		return D3D12_FILL_MODE_SOLID;
	}


#if 0
	D3D12_MAP D3D12Translate::To(ELockAccess access)
	{
		switch( access )
		{
		case ELockAccess::ReadOnly:  return D3D11_MAP_READ;
		case ELockAccess::ReadWrite: return D3D11_MAP_READ_WRITE;
		case ELockAccess::WriteOnly: return D3D11_MAP_WRITE;
		case ELockAccess::WriteDiscard: return D3D11_MAP_WRITE_DISCARD;
		}
		return D3D12_MAP_READ_WRITE;
	}
#endif


	D3D12_FILTER D3D12Translate::To(Sampler::Filter filter)
	{
		switch( filter )
		{
		case Sampler::ePoint:
			return D3D12_FILTER_MIN_MAG_MIP_POINT;
		case Sampler::eBilinear:
			return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		case Sampler::eTrilinear:
			return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		case Sampler::eAnisotroicPoint:
		case Sampler::eAnisotroicLinear:
			return D3D12_FILTER_ANISOTROPIC;
		}
		return D3D12_FILTER_MIN_MAG_MIP_POINT;
	}


	D3D12_TEXTURE_ADDRESS_MODE D3D12Translate::To(Sampler::AddressMode mode)
	{
		switch( mode )
		{
		case Sampler::eWarp:   return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case Sampler::eClamp:  return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case Sampler::eMirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		case Sampler::eBorder: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		}
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}

	D3D12_COMPARISON_FUNC D3D12Translate::To(ECompareFunc func)
	{
		switch( func )
		{
		case ECompareFunc::Never:        return D3D12_COMPARISON_FUNC_NEVER;
		case ECompareFunc::Less:         return D3D12_COMPARISON_FUNC_LESS;
		case ECompareFunc::Equal:        return D3D12_COMPARISON_FUNC_EQUAL;
		case ECompareFunc::NotEqual:     return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case ECompareFunc::LessEqual:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case ECompareFunc::Greater:      return D3D12_COMPARISON_FUNC_GREATER;
		case ECompareFunc::GeraterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case ECompareFunc::Always:       return D3D12_COMPARISON_FUNC_ALWAYS;
		}
		return D3D12_COMPARISON_FUNC_NEVER;
	}

	D3D12_STENCIL_OP D3D12Translate::To(Stencil::Operation op)
	{
		switch( op )
		{
		case Stencil::eKeep:    return D3D12_STENCIL_OP_KEEP;
		case Stencil::eZero:    return D3D12_STENCIL_OP_ZERO;
		case Stencil::eReplace: return D3D12_STENCIL_OP_REPLACE;
		case Stencil::eIncr:    return D3D12_STENCIL_OP_INCR_SAT;
		case Stencil::eIncrWarp:return D3D12_STENCIL_OP_INCR;
		case Stencil::eDecr:    return D3D12_STENCIL_OP_DECR_SAT;
		case Stencil::eDecrWarp:return D3D12_STENCIL_OP_DECR;
		case Stencil::eInvert:  return D3D12_STENCIL_OP_INVERT;
		}
		return D3D12_STENCIL_OP_KEEP;
	}

	D3D12_SHADER_VISIBILITY D3D12Translate::ToVisibiltiy(EShader::Type type)
	{
		switch (type)
		{
		case EShader::Vertex: return D3D12_SHADER_VISIBILITY_VERTEX;
		case EShader::Pixel:  return D3D12_SHADER_VISIBILITY_PIXEL;
		case EShader::Geometry: return D3D12_SHADER_VISIBILITY_GEOMETRY;
		case EShader::Hull: return D3D12_SHADER_VISIBILITY_HULL;
		case EShader::Domain: return D3D12_SHADER_VISIBILITY_DOMAIN;
		case EShader::Task: return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
		case EShader::Mesh: return D3D12_SHADER_VISIBILITY_MESH;
		}

		return D3D12_SHADER_VISIBILITY_ALL;
	}

	D3D12_PRIMITIVE_TOPOLOGY_TYPE D3D12Translate::ToTopologyType(EPrimitive type)
	{
		if (type >= EPrimitive::PatchPoint1)
		{
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		}
		switch (type)
		{
		case EPrimitive::Points: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		case EPrimitive::TriangleList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		case EPrimitive::TriangleStrip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		case EPrimitive::LineList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		case EPrimitive::LineStrip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		case EPrimitive::TriangleAdjacency: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		case EPrimitive::PatchPoint1: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		case EPrimitive::Polygon:
		case EPrimitive::TriangleFan:
		case EPrimitive::LineLoop:
		case EPrimitive::Quad:

		default:
			LogWarning(0, "D3D No Support Primitive %d", (int)type);
			break;
		}
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	}

	bool D3D12SwapChain::initialize(TComPtr<IDXGISwapChainRHI>& resource, TComPtr<ID3D12DeviceRHI>& device, int bufferCount)
	{
		mResource = resource.detach();

		mRTVHandles.resize(bufferCount);
		mTextures.resize(bufferCount);
		// Create frame resources.
		{
			// Create a RTV for each frame.
			for (UINT n = 0; n < bufferCount; n++)
			{
				VERIFY_D3D_RESULT_RETURN_FALSE(mResource->GetBuffer(n, IID_PPV_ARGS(&mTextures[n])));

				mRTVHandles[n] =  D3D12DescriptorHeapPool::Get().allocRTV(mTextures[n], nullptr);
			}
		}

		return true;
	}

	D3D12InputLayout::D3D12InputLayout(InputLayoutDesc const& desc)
	{
		for (auto const& e : desc.mElements)
		{
			if (e.attribute == Vertex::ATTRIBUTE_UNUSED)
				continue;

			D3D12_INPUT_ELEMENT_DESC element;
			element.SemanticName = "ATTRIBUTE";
			element.SemanticIndex = e.attribute;
			element.Format = D3D12Translate::To(Vertex::Format(e.format), e.bNormalized);
			element.InputSlot = e.idxStream;
			element.AlignedByteOffset = e.offset;
			element.InputSlotClass = e.bIntanceData ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			element.InstanceDataStepRate = e.bIntanceData ? e.instanceStepRate : 0;

			mDescList.push_back(element);
		}
	}


	D3D12RasterizerState::D3D12RasterizerState(RasterizerStateInitializer const& initializer)
	{
		mDesc.FillMode = D3D12Translate::To(initializer.fillMode);
		mDesc.CullMode = D3D12Translate::To(initializer.cullMode);
		mDesc.FrontCounterClockwise = FALSE; //initializer.frontFace;

		mDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		mDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		mDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		mDesc.DepthClipEnable = TRUE;

		mDesc.MultisampleEnable = initializer.bEnableScissor;
		mDesc.AntialiasedLineEnable = FALSE;
		mDesc.ForcedSampleCount = 0;
		mDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}

	D3D12BlendState::D3D12BlendState(BlendStateInitializer const& initializer)
	{
		mDesc.AlphaToCoverageEnable = initializer.bEnableAlphaToCoverage;
		mDesc.IndependentBlendEnable = initializer.bEnableIndependent;

		static_assert(
			(D3D12_COLOR_WRITE_ENABLE_RED == CWM_R) && 
			(D3D12_COLOR_WRITE_ENABLE_GREEN == CWM_G) && 
			(D3D12_COLOR_WRITE_ENABLE_BLUE == CWM_B) && 
			(D3D12_COLOR_WRITE_ENABLE_ALPHA == CWM_A) );

		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			auto& renderTarget = mDesc.RenderTarget[i];

			auto const& targetValue = initializer.targetValues[i];
			renderTarget.BlendEnable = targetValue.isEnabled();
			renderTarget.LogicOpEnable = FALSE;
			renderTarget.LogicOp = D3D12_LOGIC_OP_NOOP;

			renderTarget.SrcBlend = D3D12Translate::To(targetValue.srcColor);
			renderTarget.DestBlend = D3D12Translate::To(targetValue.destColor);
			renderTarget.BlendOp = D3D12Translate::To(targetValue.op);
			renderTarget.SrcBlendAlpha = D3D12Translate::To(targetValue.srcAlpha);
			renderTarget.DestBlendAlpha = D3D12Translate::To(targetValue.destAlpha);
			renderTarget.BlendOpAlpha = D3D12Translate::To(targetValue.opAlpha);
			renderTarget.RenderTargetWriteMask = targetValue.writeMask;
		}
	}

	D3D12DepthStencilState::D3D12DepthStencilState(DepthStencilStateInitializer const& initializer)
	{
		mDesc.DepthEnable = initializer.isDepthEnable();
		mDesc.StencilEnable = initializer.bEnableStencilTest;

		mDesc.DepthFunc = D3D12Translate::To(initializer.depthFunc);

		mDesc.FrontFace.StencilFunc = D3D12Translate::To(initializer.stencilFunc);
		mDesc.FrontFace.StencilPassOp = D3D12Translate::To(initializer.zPassOp);
		mDesc.FrontFace.StencilDepthFailOp = D3D12Translate::To(initializer.zFailOp);
		mDesc.FrontFace.StencilFailOp = D3D12Translate::To(initializer.stencilFailOp);

		mDesc.BackFace.StencilFunc = D3D12Translate::To(initializer.stencilFuncBack);
		mDesc.BackFace.StencilPassOp = D3D12Translate::To(initializer.zPassOpBack);
		mDesc.BackFace.StencilDepthFailOp = D3D12Translate::To(initializer.zFailOpBack);
		mDesc.BackFace.StencilFailOp = D3D12Translate::To(initializer.stencilFailOpBack);

		mDesc.DepthWriteMask = initializer.bWriteDepth ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		mDesc.StencilReadMask = initializer.stencilReadMask;
		mDesc.StencilWriteMask = initializer.stencilWriteMask;

		mDesc.DepthBoundsTestEnable = FALSE;
	}

	bool D3D12Texture2D::initialize(TComPtr< ID3D12Resource >& resource, int w, int h)
	{
		mResource = resource.detach();
		mSizeX = w;
		mSizeY = h;
		return true;
	}

	D3D12SamplerState::D3D12SamplerState(SamplerStateInitializer const& initializer)
	{
		D3D12_SAMPLER_DESC desc;
		desc.Filter = D3D12Translate::To(initializer.filter);
		desc.AddressU = D3D12Translate::To(initializer.addressU);
		desc.AddressV = D3D12Translate::To(initializer.addressV);
		desc.AddressW = D3D12Translate::To(initializer.addressW);
		desc.MipLODBias = 0;
		desc.MaxAnisotropy = 0;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		desc.MinLOD = 0.0f;
		desc.MaxLOD = D3D12_FLOAT32_MAX;

		mDesc = desc;
		mHandle = D3D12DescriptorHeapPool::Get().allocSampler(desc);
	}

}//namespace Render
