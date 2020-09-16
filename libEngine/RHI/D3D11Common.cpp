#include "D3D11Common.h"
#include "D3D11ShaderCommon.h"

#pragma comment(lib , "D3D11.lib")
#pragma comment(lib , "DXGI.lib")
#pragma comment(lib , "dxguid.lib")

namespace Render
{

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


	FixString<32> FD3D11Utility::GetShaderProfile(D3D_FEATURE_LEVEL featureLevel, EShader::Type type)
	{
		char const* ShaderNames[] = { "vs" , "ps" , "gs" , "cs" , "hs" , "ds" ,"as" , "ms" };
		char const* featureName = nullptr;
		switch(featureLevel)
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
		mAttriableMask = 0;
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
			mAttriableMask |= BIT(e.attribute);
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
		mRenderTargetsState.colorResources[indexSlot] = &target;
		mRenderTargetsState.colorBuffers[indexSlot] = static_cast<D3D11Texture2D&>(target).getRenderTargetView(level);
		mRenderTargetsState.numColorBuffers += 1;
		bStateDirty = true;
		return indexSlot;
	}

	int D3D11FrameBuffer::addTexture(RHITextureCube& target, Texture::Face face, int level)
	{
		assert(mRenderTargetsState.numColorBuffers + 1 <= D3D11RenderTargetsState::MaxSimulationBufferCount);
		int indexSlot = mRenderTargetsState.numColorBuffers;

		mRenderTargetsState.colorResources[indexSlot] = &target;
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

		mRenderTargetsState.colorResources[idx] = &target;
		mRenderTargetsState.colorBuffers[idx] = static_cast<D3D11Texture2D&>(target).getRenderTargetView(level);
		bStateDirty = true;
	}

	void D3D11FrameBuffer::setTexture(int idx, RHITextureCube& target, Texture::Face face, int level)
	{
		assert(idx <= mRenderTargetsState.numColorBuffers);
		if (idx == mRenderTargetsState.numColorBuffers)
			mRenderTargetsState.numColorBuffers += 1;

		mRenderTargetsState.colorResources[idx] = &target;
		mRenderTargetsState.colorBuffers[idx] = static_cast<D3D11TextureCube&>(target).getRenderTargetView(face, level);
		bStateDirty = true;
	}

	void D3D11FrameBuffer::setDepth(RHITexture2D& target)
	{
		mRenderTargetsState.depthResource = &target;
		mRenderTargetsState.depthBuffer = static_cast<D3D11Texture2D&>(target).mDSV;
		bStateDirty = true;
	}

	void D3D11FrameBuffer::removeDepth()
	{
		mRenderTargetsState.depthResource = nullptr;
		mRenderTargetsState.depthBuffer = nullptr;
		bStateDirty = true;
	}

}//namespace Render