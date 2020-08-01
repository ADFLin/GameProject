#include "D3D11Common.h"
#include "D3D11ShaderCommon.h"

namespace Render
{
	D3D_PRIMITIVE_TOPOLOGY D3D11Translate::To(EPrimitive type)
	{
		if (type >= EPrimitive::PatchPoint1)
		{
			return D3D_PRIMITIVE_TOPOLOGY( D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + ((int)type - (int)EPrimitive::PatchPoint1) );
		}
		switch( type )
		{
		case EPrimitive::Points: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case EPrimitive::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case EPrimitive::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case EPrimitive::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case EPrimitive::LineStrip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case EPrimitive::TriangleAdjacency: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
		case EPrimitive::PatchPoint1: return D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
		case EPrimitive::Polygon:
		case EPrimitive::TriangleFan:
		case EPrimitive::LineLoop:
		case EPrimitive::Quad:

		default:
			LogWarning(0, "D3D11 No Support Primitive %d", (int)type);
			break;
		}
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}

	DXGI_FORMAT D3D11Translate::To(Texture::Format format)
	{
		switch( format )
		{
		//case Texture::eRGB8:
		case Texture::eRGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Texture::eBGRA8: return DXGI_FORMAT_B8G8R8A8_UNORM;
		case Texture::eR16:   return DXGI_FORMAT_R16_UNORM;
		case Texture::eR8:    return DXGI_FORMAT_R8_UNORM;
		case Texture::eR32F:  return DXGI_FORMAT_R32_FLOAT;
		case Texture::eRG32F: return DXGI_FORMAT_R32G32_FLOAT;
		case Texture::eRGB32F: return DXGI_FORMAT_R32G32B32_FLOAT;
		case Texture::eRGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case Texture::eR16F:  return DXGI_FORMAT_R16_FLOAT;
		case Texture::eRG16F: return DXGI_FORMAT_R16G16_FLOAT;
		//case Texture::eRGB16F:
		case Texture::eRGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case Texture::eR32I: return DXGI_FORMAT_R32_SINT;
		case Texture::eR16I: return DXGI_FORMAT_R16_SINT;
		case Texture::eR8I: return DXGI_FORMAT_R8_SINT;
		case Texture::eR32U: return DXGI_FORMAT_R32_UINT;
		case Texture::eR16U: return DXGI_FORMAT_R16_UINT;
		case Texture::eR8U:  return DXGI_FORMAT_R8_UINT;
		case Texture::eRG32I: return DXGI_FORMAT_R32G32_SINT;
		case Texture::eRG16I: return DXGI_FORMAT_R16G16_SINT;
		case Texture::eRG8I: return DXGI_FORMAT_R8G8_SINT;
		case Texture::eRG32U: return DXGI_FORMAT_R32G32_UINT;
		case Texture::eRG16U: return DXGI_FORMAT_R16G16_UINT;
		case Texture::eRG8U:  return DXGI_FORMAT_R8G8_UINT;
		//case Texture::eRGB32I:
		case Texture::eRGBA32I: return DXGI_FORMAT_R32G32B32A32_SINT;
		//case Texture::eRGB16I:
		case Texture::eRGBA16I: return DXGI_FORMAT_R16G16B16A16_SINT;
		//case Texture::eRGB8I:
		case Texture::eRGBA8I:  return DXGI_FORMAT_R8G8B8A8_SINT;
		//case Texture::eRGB32U:
		case Texture::eRGBA32U: return DXGI_FORMAT_R32G32B32A32_UINT;
		//case Texture::eRGB16U:
		case Texture::eRGBA16U: return DXGI_FORMAT_R16G16B16A16_UINT;
		//case Texture::eRGB8U:
		case Texture::eRGBA8U:  return DXGI_FORMAT_R8G8B8A8_UINT;	
		//case Texture::eSRGB:
		case Texture::eSRGBA:   return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

		default:
			LogWarning(0, "D3D11 No Support Texture Format %d", (int)format);
			break;
		}
		return DXGI_FORMAT_UNKNOWN;
	}

	DXGI_FORMAT D3D11Translate::To(Texture::DepthFormat format)
	{
		switch( format )
		{
		case Texture::eDepth16: return DXGI_FORMAT_D16_UNORM;
		case Texture::eDepth24: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case Texture::eDepth32: return DXGI_FORMAT_R32_UINT;
		case Texture::eDepth32F: return DXGI_FORMAT_D32_FLOAT;
		case Texture::eD24S8:    return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case Texture::eD32FS8:   return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case Texture::eStencil1:
		case Texture::eStencil4:
		case Texture::eStencil8:
		case Texture::eStencil16: return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		default:
			LogWarning(0, "D3D11 No Support Texture DepthFormat %d", (int)format);
			break;
		}

		return DXGI_FORMAT_UNKNOWN;

	}

#define STATIC_CHECK_DATA_MAP( NAME , DATA_MAP , MEMBER )\
	constexpr bool Check##NAME##Valid_R(int i, int size)\
	{\
		return (i == size) ? true : ((i == (int)DATA_MAP[i].MEMBER) && Check##NAME##Valid_R(i + 1, size));\
	}\
	constexpr bool Check##NAME##Valid()\
	{\
		return Check##NAME##Valid_R(0, ARRAY_SIZE(DATA_MAP));\
	}\
	static_assert(Check##NAME##Valid(), "Check "#NAME" Failed")

#if _DEBUG
#define DATA_OP( S , D ) { S , D },
#else
#define DATA_OP( S , D ) { D },
#endif

#define DEFINE_DATA_MAP( TYPE , NAME , LIST )\
	static constexpr TYPE NAME[]\
	{\
		LIST(DATA_OP)\
	}



	struct BlendFactorMapInfoD3D11
	{
#if _DEBUG
		Blend::Factor src;
#endif
		D3D11_BLEND   dest;
	};

#define BLEND_FACTOR_LIST_D3D11( op )\
	op(Blend::eOne, D3D11_BLEND_ONE)\
	op(Blend::eZero, D3D11_BLEND_ZERO)\
	op(Blend::eSrcAlpha, D3D11_BLEND_SRC_ALPHA)\
	op(Blend::eOneMinusSrcAlpha, D3D11_BLEND_INV_SRC_ALPHA)\
	op(Blend::eDestAlpha, D3D11_BLEND_DEST_ALPHA)\
	op(Blend::eOneMinusDestAlpha, D3D11_BLEND_INV_DEST_ALPHA)\
	op(Blend::eSrcColor, D3D11_BLEND_SRC_COLOR)\
	op(Blend::eOneMinusSrcColor, D3D11_BLEND_INV_SRC_COLOR)\
	op(Blend::eDestColor, D3D11_BLEND_DEST_COLOR)\
	op(Blend::eOneMinusDestColor, D3D11_BLEND_INV_DEST_COLOR)

	DEFINE_DATA_MAP(BlendFactorMapInfoD3D11, GBlendFactorMapD3D11, BLEND_FACTOR_LIST_D3D11);
#if _DEBUG
	STATIC_CHECK_DATA_MAP(BlendFactorMapValid, GBlendFactorMapD3D11, src);
#endif
	D3D11_BLEND D3D11Translate::To(Blend::Factor factor)
	{
		return GBlendFactorMapD3D11[factor].dest;
	}


	struct BlendOpMapInfoD3D11
	{
#if _DEBUG
		Blend::Operation src;
#endif
		D3D11_BLEND_OP   dest;
	};

#define BLEND_OP_LIST_D3D11( op )\
	op(Blend::eAdd, D3D11_BLEND_OP_ADD)\
	op(Blend::eSub, D3D11_BLEND_OP_SUBTRACT)\
	op(Blend::eReverseSub, D3D11_BLEND_OP_REV_SUBTRACT)\
	op(Blend::eMax, D3D11_BLEND_OP_MAX)\
	op(Blend::eMin, D3D11_BLEND_OP_MIN)

	DEFINE_DATA_MAP(BlendOpMapInfoD3D11, GBlendOpMapD3D11, BLEND_OP_LIST_D3D11);
#if _DEBUG
	STATIC_CHECK_DATA_MAP(BlendOpMapValid, GBlendOpMapD3D11, src);
#endif
	D3D11_BLEND_OP D3D11Translate::To(Blend::Operation op)
	{
		return GBlendOpMapD3D11[op].dest;
	}


	struct CullModeMapInfoD3D11
	{
#if _DEBUG
		ECullMode src;
#endif
		D3D11_CULL_MODE   dest;
	};

#define CULL_MODE_LIST_D3D11( op )\
	op(ECullMode::None, D3D11_CULL_NONE)\
	op(ECullMode::Front, D3D11_CULL_FRONT)\
	op(ECullMode::Back, D3D11_CULL_BACK)
	
	DEFINE_DATA_MAP(CullModeMapInfoD3D11, GCullModeMapD3D11, CULL_MODE_LIST_D3D11);
#if _DEBUG
	STATIC_CHECK_DATA_MAP(CullModeMapValid, GCullModeMapD3D11, src);
#endif
	D3D11_CULL_MODE D3D11Translate::To(ECullMode mode)
	{
		return GCullModeMapD3D11[int(mode)].dest;
	}

	D3D11_FILL_MODE D3D11Translate::To(EFillMode mode)
	{
		switch( mode )
		{
		case EFillMode::Wireframe: return D3D11_FILL_WIREFRAME;
		case EFillMode::Solid: return D3D11_FILL_SOLID;
		case EFillMode::System:
		case EFillMode::Point:
			return D3D11_FILL_SOLID;
		}
		return D3D11_FILL_SOLID;
	}

	DXGI_FORMAT D3D11Translate::To(Vertex::Format format , bool bNormalized)
	{
		switch( format )
		{
		case Vertex::eFloat1: return DXGI_FORMAT_R32_FLOAT;
		case Vertex::eFloat2: return DXGI_FORMAT_R32G32_FLOAT;
		case Vertex::eFloat3: return DXGI_FORMAT_R32G32B32_FLOAT;
		case Vertex::eFloat4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case Vertex::eHalf1:  return DXGI_FORMAT_R16_FLOAT;
		case Vertex::eHalf2:  return DXGI_FORMAT_R16G16_FLOAT;
		//case Vertex::eHalf3:  return DXGI_FORMAT_R16G16B16_FLOAT;
		case Vertex::eHalf4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case Vertex::eUInt1: return DXGI_FORMAT_R32_UINT;
		case Vertex::eUInt2: return DXGI_FORMAT_R32G32_UINT;
		case Vertex::eUInt3: return DXGI_FORMAT_R32G32B32_UINT;
		case Vertex::eUInt4: return DXGI_FORMAT_R32G32B32A32_UINT;
		case Vertex::eInt1: return DXGI_FORMAT_R32_SINT;
		case Vertex::eInt2: return DXGI_FORMAT_R32G32_SINT;
		case Vertex::eInt3: return DXGI_FORMAT_R32G32B32_UINT;
		case Vertex::eInt4: return DXGI_FORMAT_R32G32B32A32_UINT;

		case Vertex::eUShort1: return (bNormalized) ? DXGI_FORMAT_R16_UNORM : DXGI_FORMAT_R16_UINT;
		case Vertex::eUShort2: return (bNormalized) ? DXGI_FORMAT_R16G16_UNORM : DXGI_FORMAT_R16G16_UINT;
		//case Vertex::eUShort3: return (bNormalized) ? DXGI_FORMAT_R16G16B16_UNORM : DXGI_FORMAT_R16G16B16_UINT;
		case Vertex::eUShort4: return (bNormalized) ? DXGI_FORMAT_R16G16B16A16_UNORM : DXGI_FORMAT_R16G16B16A16_UINT;
		case Vertex::eShort1: return (bNormalized) ? DXGI_FORMAT_R16_SNORM : DXGI_FORMAT_R16_SINT;
		case Vertex::eShort2: return (bNormalized) ? DXGI_FORMAT_R16G16_SNORM : DXGI_FORMAT_R16G16_SINT;
		//case Vertex::eShort3: return (bNormalized) ? DXGI_FORMAT_R16G16B16_SNORM : DXGI_FORMAT_R16G16B16_SINT;
		case Vertex::eShort4: return (bNormalized) ? DXGI_FORMAT_R16G16B16A16_SNORM : DXGI_FORMAT_R16G16B16A16_SINT;
		case Vertex::eUByte1: return (bNormalized) ? DXGI_FORMAT_R8_UNORM : DXGI_FORMAT_R8_UINT;
		case Vertex::eUByte2: return (bNormalized) ? DXGI_FORMAT_R8G8_UNORM : DXGI_FORMAT_R8G8_UINT;
		//case Vertex::eUByte3: return (bNormalized) ? DXGI_FORMAT_R8G8B8_UNORM : DXGI_FORMAT_R8G8B8_UINT;
		case Vertex::eUByte4: return (bNormalized) ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UINT;
		case Vertex::eByte1: return (bNormalized) ? DXGI_FORMAT_R8_SNORM : DXGI_FORMAT_R8_SINT;
		case Vertex::eByte2: return (bNormalized) ? DXGI_FORMAT_R8G8_SNORM : DXGI_FORMAT_R8G8_SINT;
		//case Vertex::eByte3: return (bNormalized) ? DXGI_FORMAT_R8G8B8_SNORM : DXGI_FORMAT_R8G8B8_SINT;
		case Vertex::eByte4: return (bNormalized) ? DXGI_FORMAT_R8G8B8A8_SNORM : DXGI_FORMAT_R8G8B8A8_SINT;
		}
		return DXGI_FORMAT_UNKNOWN;
	}

	struct LockAccessMapInfoD3D11
	{
#if _DEBUG
		ELockAccess src;
#endif
		D3D11_MAP   dest;
	};

#define LOCK_ACCESS_LIST_D3D11( op )\
	op(ELockAccess::ReadOnly, D3D11_MAP_READ)\
	op(ELockAccess::ReadWrite, D3D11_MAP_READ_WRITE)\
	op(ELockAccess::WriteOnly, D3D11_MAP_WRITE)\
	op(ELockAccess::WriteDiscard, D3D11_MAP_WRITE_DISCARD)

	DEFINE_DATA_MAP(LockAccessMapInfoD3D11, GLockAccessMapD3D11, LOCK_ACCESS_LIST_D3D11);
#if _DEBUG
	STATIC_CHECK_DATA_MAP(LockAccessMapValid, GLockAccessMapD3D11, src);
#endif
	D3D11_MAP D3D11Translate::To(ELockAccess access)
	{
		return GLockAccessMapD3D11[int(access)].dest;
	}

	D3D11_FILTER D3D11Translate::To(Sampler::Filter filter)
	{
		switch( filter )
		{
		case Sampler::ePoint:
			return D3D11_FILTER_MIN_MAG_MIP_POINT;
		case Sampler::eBilinear:
			return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		case Sampler::eTrilinear:
			return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		case Sampler::eAnisotroicPoint:
		case Sampler::eAnisotroicLinear:
			return D3D11_FILTER_ANISOTROPIC;
		}
		return D3D11_FILTER_MIN_MAG_MIP_POINT;
	}

	D3D11_TEXTURE_ADDRESS_MODE D3D11Translate::To(Sampler::AddressMode mode)
	{
		switch( mode )
		{
		case Sampler::eWarp:   return D3D11_TEXTURE_ADDRESS_WRAP;
		case Sampler::eClamp:  return D3D11_TEXTURE_ADDRESS_CLAMP;
		case Sampler::eMirror: return D3D11_TEXTURE_ADDRESS_MIRROR;
		case Sampler::eBorder: return D3D11_TEXTURE_ADDRESS_BORDER;
		}
		return D3D11_TEXTURE_ADDRESS_WRAP;
	}

	D3D11_COMPARISON_FUNC D3D11Translate::To(ECompareFunc func)
	{
		switch( func )
		{
		case ECompareFunc::Never:        return D3D11_COMPARISON_NEVER;
		case ECompareFunc::Less:         return D3D11_COMPARISON_LESS;
		case ECompareFunc::Equal:        return D3D11_COMPARISON_EQUAL;
		case ECompareFunc::NotEqual:     return D3D11_COMPARISON_NOT_EQUAL;
		case ECompareFunc::LessEqual:    return D3D11_COMPARISON_LESS_EQUAL;
		case ECompareFunc::Greater:      return D3D11_COMPARISON_GREATER;
		case ECompareFunc::GeraterEqual: return D3D11_COMPARISON_GREATER_EQUAL;
		case ECompareFunc::Always:       return D3D11_COMPARISON_ALWAYS;
		}
		return D3D11_COMPARISON_NEVER;
	}

	struct StencilOpMapInfoD3D11
	{
#if _DEBUG
		Stencil::Operation src;
#endif
		D3D11_STENCIL_OP   dest;
	};

#define STENCIL_OP_LIST_D3D11( op )\
	op(Stencil::eKeep, D3D11_STENCIL_OP_KEEP)\
	op(Stencil::eZero, D3D11_STENCIL_OP_ZERO)\
	op(Stencil::eReplace, D3D11_STENCIL_OP_REPLACE)\
	op(Stencil::eIncr, D3D11_STENCIL_OP_INCR_SAT)\
	op(Stencil::eIncrWarp, D3D11_STENCIL_OP_INCR)\
	op(Stencil::eDecr, D3D11_STENCIL_OP_DECR_SAT)\
	op(Stencil::eDecrWarp, D3D11_STENCIL_OP_DECR)\
	op(Stencil::eInvert, D3D11_STENCIL_OP_INVERT)

	DEFINE_DATA_MAP(StencilOpMapInfoD3D11, GStencilOpMapD3D11, STENCIL_OP_LIST_D3D11);
#if _DEBUG
	STATIC_CHECK_DATA_MAP(StencilOpMapValid, GStencilOpMapD3D11, src);
#endif
	D3D11_STENCIL_OP D3D11Translate::To(Stencil::Operation op)
	{
		return GStencilOpMapD3D11[op].dest;
	}


	FixString<32> FD3D11Utility::GetShaderProfile(ID3D11Device* device, EShader::Type type)
	{
		char const* ShaderNames[] = { "vs" , "ps" , "gs" , "cs" , "hs" , "ds" };
		char const* featureName = nullptr;
		switch( device->GetFeatureLevel() )
		{
		case D3D_FEATURE_LEVEL_9_1:
		case D3D_FEATURE_LEVEL_9_2:
		case D3D_FEATURE_LEVEL_9_3:
			featureName = "2_0";
			break;
		case D3D_FEATURE_LEVEL_10_0:
			featureName = "4_0";
			break;
		case D3D_FEATURE_LEVEL_10_1:
			featureName = "5_0";
			break;
		case D3D_FEATURE_LEVEL_11_0:
			featureName = "5_0";
			break;
		}
		FixString<32> result = ShaderNames[type];
		result += "_";
		result += featureName;
		return result;
	}

	void D3D11InputLayout::initialize(InputLayoutDesc const& desc)
	{
		mAttriableMasks = 0;
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

			mDescList.push_back(element);
			mAttriableMasks |= BIT(e.attribute);
		}
	}

	void D3D11InputLayout::releaseResource()
	{
		for (auto& pair : mResourceMap)
		{
			pair.second->Release();
		}
		mResourceMap.clear();
		mUniversalResource->Release();
		mUniversalResource = nullptr;
	}

	ID3D11InputLayout* D3D11InputLayout::GetShaderLayout(ID3D11Device* device, RHIShader* shader)
	{
		auto iter = mResourceMap.find(shader);
		if (iter != mResourceMap.end())
		{
			return iter->second;
		}

		ID3D11InputLayout* inputLayoutResource = nullptr;
		std::vector< uint8 > const& byteCode = static_cast<D3D11Shader*>(shader)->byteCode;
		HRESULT hr = device->CreateInputLayout(mDescList.data(), mDescList.size(), byteCode.data(), byteCode.size(), &inputLayoutResource);
		if (hr != S_OK)
		{
			LogWarning(0 , "Can't create D3D11InputLayout , code = %d" , hr );
			mUniversalResource->AddRef();
			mResourceMap.insert(std::make_pair(shader, mUniversalResource));
			return mUniversalResource;
		}
		mResourceMap.insert(std::make_pair(shader, inputLayoutResource));
		return inputLayoutResource;
	}


	int D3D11FrameBuffer::addTexture(RHITexture2D& target, int level)
	{
		assert(mRenderTargetsState.numColorBuffers + 1 <= D3D11RenderTargetsState::MaxSimulationBufferCount);
		int indexSlot = mRenderTargetsState.numColorBuffers;
		mColorTextures[indexSlot] = &target;
		mRenderTargetsState.colorBuffers[indexSlot] = static_cast<D3D11Texture2D&>(target).getRenderTargetView(level);
		mRenderTargetsState.numColorBuffers += 1;
		bStateDirty = true;
		return indexSlot;
	}

	int D3D11FrameBuffer::addTexture(RHITextureCube& target, Texture::Face face, int level)
	{
		assert(mRenderTargetsState.numColorBuffers + 1 <= D3D11RenderTargetsState::MaxSimulationBufferCount);
		int indexSlot = mRenderTargetsState.numColorBuffers;

		mColorTextures[indexSlot] = &target;
		mRenderTargetsState.colorBuffers[indexSlot] = static_cast<D3D11TextureCube&>(target).getRenderTargetView(face, level);
		mRenderTargetsState.numColorBuffers += 1;
		bStateDirty = true;
		return indexSlot;
	}

	void D3D11FrameBuffer::setTexture(int idx, RHITexture2D& target, int level)
	{
		assert(idx <= mRenderTargetsState.numColorBuffers);
		if (idx == mRenderTargetsState.numColorBuffers)
			mRenderTargetsState.numColorBuffers += 1;

		mColorTextures[idx] = &target;
		mRenderTargetsState.colorBuffers[idx] = static_cast<D3D11Texture2D&>(target).getRenderTargetView(level);
		bStateDirty = true;
	}

	void D3D11FrameBuffer::setTexture(int idx, RHITextureCube& target, Texture::Face face, int level)
	{
		assert(idx <= mRenderTargetsState.numColorBuffers);
		if (idx == mRenderTargetsState.numColorBuffers)
			mRenderTargetsState.numColorBuffers += 1;

		mColorTextures[idx] = &target;
		mRenderTargetsState.colorBuffers[idx] = static_cast<D3D11TextureCube&>(target).getRenderTargetView(face, level);
		bStateDirty = true;
	}

	void D3D11FrameBuffer::setDepth(RHITextureDepth& target)
	{
		mDepthTexture = &target;
		mRenderTargetsState.depthBuffer = static_cast<D3D11TextureDepth&>(target).mDSV;
		bStateDirty = true;
	}

	void D3D11FrameBuffer::removeDepth()
	{
		mDepthTexture = nullptr;
		mRenderTargetsState.depthBuffer = nullptr;
		bStateDirty = true;
	}

}//namespace Render