
#include "D3D12Common.h"
#include "D3D12ShaderCommon.h"
#include "D3D12Command.h"

#pragma comment(lib , "D3D12.lib")
#pragma comment(lib , "DXGI.lib")
#pragma comment(lib , "dxguid.lib")

namespace Render
{
	D3D12_BLEND D3D12Translate::To(EBlend::Factor factor)
	{
		switch( factor )
		{
		case EBlend::One: return D3D12_BLEND_ONE;
		case EBlend::Zero: return D3D12_BLEND_ZERO;
		case EBlend::SrcAlpha: return D3D12_BLEND_SRC_ALPHA;
		case EBlend::OneMinusSrcAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
		case EBlend::DestAlpha: return D3D12_BLEND_DEST_ALPHA;
		case EBlend::OneMinusDestAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
		case EBlend::SrcColor: return  D3D12_BLEND_SRC_COLOR;
		case EBlend::OneMinusSrcColor: return D3D12_BLEND_INV_SRC_COLOR;
		case EBlend::DestColor: return D3D12_BLEND_DEST_COLOR;
		case EBlend::OneMinusDestColor: return D3D12_BLEND_INV_DEST_COLOR;
		default:
			break;
		}
		return D3D12_BLEND_ONE;
	}

	D3D12_BLEND_OP D3D12Translate::To(EBlend::Operation op)
	{
		switch( op )
		{
		case EBlend::Add: return D3D12_BLEND_OP_ADD;
		case EBlend::Sub: return D3D12_BLEND_OP_SUBTRACT;
		case EBlend::ReverseSub: return D3D12_BLEND_OP_REV_SUBTRACT;
		case EBlend::Max: return D3D12_BLEND_OP_MAX;
		case EBlend::Min: return D3D12_BLEND_OP_MIN;
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


	D3D12_FILTER D3D12Translate::To(ESampler::Filter filter)
	{
		switch( filter )
		{
		case ESampler::Point:
			return D3D12_FILTER_MIN_MAG_MIP_POINT;
		case ESampler::Bilinear:
			return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		case ESampler::Trilinear:
			return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		case ESampler::AnisotroicPoint:
		case ESampler::AnisotroicLinear:
			return D3D12_FILTER_ANISOTROPIC;
		}
		return D3D12_FILTER_MIN_MAG_MIP_POINT;
	}


	D3D12_TEXTURE_ADDRESS_MODE D3D12Translate::To(ESampler::AddressMode mode)
	{
		switch( mode )
		{
		case ESampler::Warp:   return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case ESampler::Clamp:  return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case ESampler::Mirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		case ESampler::Border: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
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

	D3D12_STENCIL_OP D3D12Translate::To(EStencil op)
	{
		switch( op )
		{
		case EStencil::Keep:    return D3D12_STENCIL_OP_KEEP;
		case EStencil::Zero:    return D3D12_STENCIL_OP_ZERO;
		case EStencil::Replace: return D3D12_STENCIL_OP_REPLACE;
		case EStencil::Incr:    return D3D12_STENCIL_OP_INCR_SAT;
		case EStencil::IncrWarp:return D3D12_STENCIL_OP_INCR;
		case EStencil::Decr:    return D3D12_STENCIL_OP_DECR_SAT;
		case EStencil::DecrWarp:return D3D12_STENCIL_OP_DECR;
		case EStencil::Invert:  return D3D12_STENCIL_OP_INVERT;
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
		mRenderTargetsStates.resize(bufferCount);

		for (int i = 0; i < bufferCount; ++i)
		{
			auto& state = mRenderTargetsStates[i];
			state.numColorBuffers = 1;
			VERIFY_D3D_RESULT_RETURN_FALSE(mResource->GetBuffer(i, IID_PPV_ARGS(&state.colorBuffers[0].resource)));
			state.colorBuffers[0].RTVHandle = D3D12DescriptorHeapPool::Get().allocRTV(state.colorBuffers[0].resource, nullptr);
			D3D12_RESOURCE_DESC desc = state.colorBuffers[0].resource->GetDesc();
			state.colorBuffers[0].format = desc.Format;

			//#TODO:depth
			state.depthBuffer.format;
			state.depthBuffer.resource;
			//state.depthBuffer.RTVHandle;
		}

		return true;
	}

	D3D12InputLayout::D3D12InputLayout(InputLayoutDesc const& desc)
	{
		mAttriableMask = 0;
		for (auto const& e : desc.mElements)
		{
			if (e.attribute == EVertex::ATTRIBUTE_UNUSED)
				continue;

			D3D12_INPUT_ELEMENT_DESC element;
			element.SemanticName = "ATTRIBUTE";
			element.SemanticIndex = e.attribute;
			element.Format = D3D12Translate::To(EVertex::Format(e.format), e.bNormalized);
			element.InputSlot = e.streamIndex;
			element.AlignedByteOffset = e.offset;
			element.InputSlotClass = e.bIntanceData ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			element.InstanceDataStepRate = e.bIntanceData ? e.instanceStepRate : 0;
			mDescList.push_back(element);

			mAttriableMask |= BIT(e.attribute);		
		}
	}


	D3D12RasterizerState::D3D12RasterizerState(RasterizerStateInitializer const& initializer)
	{
		mDesc.FillMode = D3D12Translate::To(initializer.fillMode);
		mDesc.CullMode = D3D12Translate::To(initializer.cullMode);
		mDesc.FrontCounterClockwise = initializer.frontFace != EFrontFace::Default;

		mDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		mDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		mDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		mDesc.DepthClipEnable = TRUE;

		mDesc.MultisampleEnable = initializer.bEnableScissor;
		mDesc.AntialiasedLineEnable = FALSE;
		mDesc.ForcedSampleCount = 0;
		mDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		bEnableScissor = initializer.bEnableScissor;
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


	bool D3D12Texture1D::initialize(TComPtr< ID3D12Resource >& resource, ETexture::Format format, int length)
	{
		mFormat = format;
		mResource = resource.detach();
		mSize = length;
		return true;
	}

	bool D3D12Texture1D::update(int offset, int length, ETexture::Format format, void* data, int level)
	{
		if (mFormat == format)
		{
			return static_cast<D3D12System*>(GRHISystem)->updateTexture1DSubresources(
				mResource, mFormat, data, offset, length, level
			);
		}
		return false;
	}

	bool D3D12Texture2D::initialize(TComPtr< ID3D12Resource >& resource, ETexture::Format format, int w, int h)
	{
		mFormat = format;
		mResource = resource.detach();
		mSizeX = w;
		mSizeY = h;
		return true;
	}

	bool D3D12Texture2D::update(int ox, int oy, int w, int h, ETexture::Format format, void* data, int level)
	{
		if (mFormat == format)
		{
			return static_cast<D3D12System*>(GRHISystem)->updateTexture2DSubresources(
				mResource, mFormat, data, ox, oy, w, h, w * ETexture::GetFormatSize(mFormat), level
			);
		}
		return false;
	}

	bool D3D12Texture2D::update(int ox, int oy, int w, int h, ETexture::Format format, int dataImageWidth, void* data, int level)
	{
		if (mFormat == format)
		{
			return static_cast<D3D12System*>(GRHISystem)->updateTexture2DSubresources(
				mResource, mFormat, data, ox, oy, w, h, dataImageWidth * ETexture::GetFormatSize(mFormat), level
			);
		}
		return false;
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
