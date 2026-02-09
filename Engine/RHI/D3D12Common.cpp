
#include "D3D12Common.h"
#include "D3D12ShaderCommon.h"
#include "D3D12Command.h"

#pragma comment(lib , "D3D12.lib")
#pragma comment(lib , "DXGI.lib")
#pragma comment(lib , "dxguid.lib")


namespace Render
{
	template< typename TMapInfo , int Num >
	constexpr bool CheckMapInfoVaild(TMapInfo (&infoList)[Num])
	{
		for (int i = 0; i < Num; ++i)
		{
			if ((int)infoList[i].src != i)
				return false;
		}
		return true;
	}

#if _DEBUG

#define DEFINE_MAP_INFO(NAME, SRC , DST)\
	struct NAME\
	{\
		SRC src;\
		DST dest;\
	};

#define DATA_OP( S , D ) { S , D },
#define DEFINE_DATA_MAP( TYPE , NAME , LIST )\
	static constexpr TYPE NAME[]\
	{\
		LIST(DATA_OP)\
	};\
	static_assert(CheckMapInfoVaild(NAME), "Check "#NAME" Failed");

#else

#define DEFINE_MAP_INFO(NAME, SRC , DST)\
	struct NAME\
	{\
		DST dest;\
	};

#define DATA_OP( S , D ) { D },
#define DEFINE_DATA_MAP( TYPE , NAME , LIST )\
	static constexpr TYPE NAME[]\
	{\
		LIST(DATA_OP)\
	}

#endif

	DEFINE_MAP_INFO(BlendFactorMapInfoD3D12, EBlend::Factor, D3D12_BLEND);

#define BLEND_FACTOR_LIST_D3D12( op )\
	op(EBlend::Zero, D3D12_BLEND_ZERO)\
	op(EBlend::One, D3D12_BLEND_ONE)\
	op(EBlend::SrcAlpha, D3D12_BLEND_SRC_ALPHA)\
	op(EBlend::OneMinusSrcAlpha, D3D12_BLEND_INV_SRC_ALPHA)\
	op(EBlend::DestAlpha, D3D12_BLEND_DEST_ALPHA)\
	op(EBlend::OneMinusDestAlpha, D3D12_BLEND_INV_DEST_ALPHA)\
	op(EBlend::SrcColor, D3D12_BLEND_SRC_COLOR)\
	op(EBlend::OneMinusSrcColor, D3D12_BLEND_INV_SRC_COLOR)\
	op(EBlend::DestColor, D3D12_BLEND_DEST_COLOR)\
	op(EBlend::OneMinusDestColor, D3D12_BLEND_INV_DEST_COLOR)

	DEFINE_DATA_MAP(BlendFactorMapInfoD3D12, GBlendFactorMapD3D12, BLEND_FACTOR_LIST_D3D12);

	D3D12_BLEND D3D12Translate::To(EBlend::Factor factor)
	{
		return GBlendFactorMapD3D12[factor].dest;
	}

	DEFINE_MAP_INFO(BlendOpMapInfoD3D12, EBlend::Operation, D3D12_BLEND_OP);

#define BLEND_OP_LIST_D3D12( op )\
	op(EBlend::Add, D3D12_BLEND_OP_ADD)\
	op(EBlend::Sub, D3D12_BLEND_OP_SUBTRACT)\
	op(EBlend::ReverseSub, D3D12_BLEND_OP_REV_SUBTRACT)\
	op(EBlend::Max, D3D12_BLEND_OP_MAX)\
	op(EBlend::Min, D3D12_BLEND_OP_MIN)

	DEFINE_DATA_MAP(BlendOpMapInfoD3D12, GBlendOpMapD3D12, BLEND_OP_LIST_D3D12);

	D3D12_BLEND_OP D3D12Translate::To(EBlend::Operation op)
	{
		return GBlendOpMapD3D12[op].dest;
	}

	DEFINE_MAP_INFO(CullModeMapInfoD3D12, ECullMode, D3D12_CULL_MODE);

#define CULL_MODE_LIST_D3D12( op )\
	op(ECullMode::None, D3D12_CULL_MODE_NONE)\
	op(ECullMode::Front, D3D12_CULL_MODE_FRONT)\
	op(ECullMode::Back, D3D12_CULL_MODE_BACK)

	DEFINE_DATA_MAP(CullModeMapInfoD3D12, GCullModeMapD3D12, CULL_MODE_LIST_D3D12);


	D3D12_CULL_MODE D3D12Translate::To(ECullMode mode)
	{
		return GCullModeMapD3D12[int(mode)].dest;
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

	DEFINE_MAP_INFO(AddressModeMapInfoD3D12, ESampler::AddressMode, D3D12_TEXTURE_ADDRESS_MODE);

#define TEX_ADDRESS_MODE_LIST_D3D12( op )\
	op(ESampler::Wrap, D3D12_TEXTURE_ADDRESS_MODE_WRAP)\
	op(ESampler::Mirror, D3D12_TEXTURE_ADDRESS_MODE_MIRROR)\
	op(ESampler::Clamp, D3D12_TEXTURE_ADDRESS_MODE_CLAMP)\
	op(ESampler::Border, D3D12_TEXTURE_ADDRESS_MODE_BORDER)\
	op(ESampler::MirrorOnce, D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE)

	DEFINE_DATA_MAP(AddressModeMapInfoD3D12, GAddressModeMapInfoD3D12, TEX_ADDRESS_MODE_LIST_D3D12);

	D3D12_TEXTURE_ADDRESS_MODE D3D12Translate::To(ESampler::AddressMode mode)
	{
		return GAddressModeMapInfoD3D12[mode].dest;
	}

	DEFINE_MAP_INFO(ComparisonFuncMapInfoD3D12, ECompareFunc, D3D12_COMPARISON_FUNC);

#define COMPARISON_FUNC_LIST_D3D12( op )\
	op(ECompareFunc::Never, D3D12_COMPARISON_FUNC_NEVER)\
	op(ECompareFunc::Less, D3D12_COMPARISON_FUNC_LESS)\
	op(ECompareFunc::Equal, D3D12_COMPARISON_FUNC_EQUAL)\
	op(ECompareFunc::LessEqual, D3D12_COMPARISON_FUNC_LESS_EQUAL)\
	op(ECompareFunc::Greater, D3D12_COMPARISON_FUNC_GREATER)\
	op(ECompareFunc::NotEqual, D3D12_COMPARISON_FUNC_NOT_EQUAL)\
	op(ECompareFunc::GreaterEqual, D3D12_COMPARISON_FUNC_GREATER_EQUAL)\
	op(ECompareFunc::Always, D3D12_COMPARISON_FUNC_ALWAYS)

	DEFINE_DATA_MAP(ComparisonFuncMapInfoD3D12, GComparisonFuncMapInfoD3D12, COMPARISON_FUNC_LIST_D3D12);


	D3D12_COMPARISON_FUNC D3D12Translate::To(ECompareFunc func)
	{
		return GComparisonFuncMapInfoD3D12[uint(func)].dest;
	}

	DEFINE_MAP_INFO(StencilOpMapInfoD3D12, EStencil, D3D12_STENCIL_OP);

#define STENCIL_OP_LIST_D3D11( op )\
	op(EStencil::Keep, D3D12_STENCIL_OP_KEEP)\
	op(EStencil::Zero, D3D12_STENCIL_OP_ZERO)\
	op(EStencil::Replace, D3D12_STENCIL_OP_REPLACE)\
	op(EStencil::Incr, D3D12_STENCIL_OP_INCR_SAT)\
	op(EStencil::IncrWarp, D3D12_STENCIL_OP_INCR)\
	op(EStencil::Decr, D3D12_STENCIL_OP_DECR_SAT)\
	op(EStencil::DecrWarp, D3D12_STENCIL_OP_DECR)\
	op(EStencil::Invert, D3D12_STENCIL_OP_INVERT)

	DEFINE_DATA_MAP(StencilOpMapInfoD3D12, GStencilOpMapD3D12, STENCIL_OP_LIST_D3D11);

	D3D12_STENCIL_OP D3D12Translate::To(EStencil op)
	{
		return GStencilOpMapD3D12[uint(op)].dest;
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

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS D3D12Translate::To(EAccelerationStructureBuildFlags flags)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS result = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		if (HaveBits(flags, EAccelerationStructureBuildFlags::AllowUpdate)) result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
		if (HaveBits(flags, EAccelerationStructureBuildFlags::AllowCompaction)) result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
		if (HaveBits(flags, EAccelerationStructureBuildFlags::PreferFastTrace)) result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		if (HaveBits(flags, EAccelerationStructureBuildFlags::PreferFastBuild)) result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
		if (HaveBits(flags, EAccelerationStructureBuildFlags::MinimizeMemory)) result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
		return result;
	}

	D3D12_RAYTRACING_INSTANCE_FLAGS D3D12Translate::To(ERayTracingInstanceFlags flags)
	{
		D3D12_RAYTRACING_INSTANCE_FLAGS result = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		if (HaveBits(flags, ERayTracingInstanceFlags::TriangleCullDisable)) result |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
		if (HaveBits(flags, ERayTracingInstanceFlags::TriangleFrontCounterClockwise)) result |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
		if (HaveBits(flags, ERayTracingInstanceFlags::ForceOpaque)) result |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
		if (HaveBits(flags, ERayTracingInstanceFlags::ForceNonOpaque)) result |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;
		return result;
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
			state.colorBuffers[0].RTVHandle = D3D12DescriptorHeapPool::Alloc<D3D12_RENDER_TARGET_VIEW_DESC>(state.colorBuffers[0].resource, nullptr);
			D3D12_RESOURCE_DESC desc = state.colorBuffers[0].resource->GetDesc();
			state.colorBuffers[0].format = desc.Format;
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

		mDesc.DepthBias = Math::FloorToInt(initializer.depthBias * (float)(1 << 24));
		mDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		mDesc.SlopeScaledDepthBias = initializer.slopeScaleDepthBias;
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
		mDesc.StencilEnable = initializer.bEnableStencil;

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


	D3D12Texture1D::D3D12Texture1D(TextureDesc const& desc, TComPtr< ID3D12Resource >& resource)
		:TD3D12Texture< RHITexture1D >(desc)
	{
		mResource = resource.detach();
	}

	bool D3D12Texture1D::update(int offset, int length, ETexture::Format format, void* data, int level)
	{
		if (getFormat() == format)
		{
			return static_cast<D3D12System*>(GRHISystem)->updateTexture1DSubresources(
				mResource, getFormat(), data, offset, length, level
			);
		}
		return false;
	}

	D3D12Texture2D::D3D12Texture2D(TextureDesc const& desc, TComPtr< ID3D12Resource >& resource)
		:TD3D12Texture< RHITexture2D >(desc)
	{
		mResource = resource.detach();
	}

	D3D12Texture3D::D3D12Texture3D(TextureDesc const& desc, TComPtr< ID3D12Resource >& resource)
		:TD3D12Texture< RHITexture3D >(desc)
	{
		mResource = resource.detach();
	}

	D3D12TextureCube::D3D12TextureCube(TextureDesc const& desc, TComPtr< ID3D12Resource >& resource)
		:TD3D12Texture< RHITextureCube >(desc)
	{
		mResource = resource.detach();
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
		mHandle = D3D12DescriptorHeapPool::Alloc(desc);
	}

	void D3D12Buffer::releaseResource()
	{
		if (mDynamicAllocation.ptr)
		{
			D3D12DynamicBufferManager::Get().dealloc(mDynamicAllocation.buddyInfo);
		}
		D3D12DescriptorHeapPool::FreeHandle(mCBV);
		D3D12DescriptorHeapPool::FreeHandle(mSRV);
		D3D12DescriptorHeapPool::FreeHandle(mUAV);

		TD3D12Resource< RHIBuffer >::releaseResource();
	}

	int D3D12FrameBuffer::addTexture(RHITexture2D& target, int level /*= 0*/)
	{
		int index = getFreeSlot();
		if (index != INDEX_NONE)
		{
			setTextureInternal(index, target, level);
		}
		return INDEX_NONE;
	}

	void D3D12FrameBuffer::setTextureInternal(int index, RHITexture2D& target, int level)
	{
		CHECK(level == 0);
		auto& targetImpl = static_cast<D3D12Texture2D&>(target);
		auto& bufferState = mRenderTargetsState.colorBuffers[index];
		bufferState.resource.assign(targetImpl.getResource());
		D3D12_RESOURCE_DESC desc = bufferState.resource->GetDesc();
		bufferState.format = desc.Format;
		bufferState.RTVHandle = targetImpl.mRTVorDSV;
		bStateDirty = true;
	}

	void D3D12FrameBuffer::setDepth(RHITexture2D& target)
	{
		auto& targetImpl = static_cast<D3D12Texture2D&>(target);
		auto& bufferState = mRenderTargetsState.depthBuffer;
		bufferState.resource.assign(targetImpl.getResource());
		D3D12_RESOURCE_DESC desc = bufferState.resource->GetDesc();
		bufferState.format = desc.Format;
		bufferState.DSVHandle = targetImpl.mRTVorDSV;
		bStateDirty = true;
	}

	void D3D12FrameBuffer::removeDepth()
	{
		auto& bufferState = mRenderTargetsState.depthBuffer;
		if (bufferState.resource.isValid())
		{
			bufferState.resource.reset();
			bufferState.format = DXGI_FORMAT_UNKNOWN;
			bufferState.DSVHandle.reset();
		}
	}

	int D3D12FrameBuffer::getFreeSlot()
	{
		for (int i = 0; i < D3D12RenderTargetsState::MaxSimulationBufferCount; ++i)
		{
			if (!mRenderTargetsState.colorBuffers[i].resource.isValid())
				return i;
		}
		return INDEX_NONE;
	}



}//namespace Render
