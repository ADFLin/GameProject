#include "D3D11Common.h"
#include "D3D11ShaderCommon.h"
#include "D3D11Command.h"

#pragma comment(lib , "D3D11.lib")
#pragma comment(lib , "DXGI.lib")
#pragma comment(lib , "dxguid.lib")

namespace Render
{

	template< typename TMapInfo, int Num >
	constexpr bool CehckMapInfoVaild(TMapInfo(&infoList)[Num])
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
	static_assert(CehckMapInfoVaild(NAME), "Check "#NAME" Failed");

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

	DEFINE_MAP_INFO(BlendFactorMapInfoD3D11, EBlend::Factor, D3D11_BLEND);

#define BLEND_FACTOR_LIST_D3D11( op )\
	op(EBlend::Zero, D3D11_BLEND_ZERO)\
	op(EBlend::One, D3D11_BLEND_ONE)\
	op(EBlend::SrcAlpha, D3D11_BLEND_SRC_ALPHA)\
	op(EBlend::OneMinusSrcAlpha, D3D11_BLEND_INV_SRC_ALPHA)\
	op(EBlend::DestAlpha, D3D11_BLEND_DEST_ALPHA)\
	op(EBlend::OneMinusDestAlpha, D3D11_BLEND_INV_DEST_ALPHA)\
	op(EBlend::SrcColor, D3D11_BLEND_SRC_COLOR)\
	op(EBlend::OneMinusSrcColor, D3D11_BLEND_INV_SRC_COLOR)\
	op(EBlend::DestColor, D3D11_BLEND_DEST_COLOR)\
	op(EBlend::OneMinusDestColor, D3D11_BLEND_INV_DEST_COLOR)

	DEFINE_DATA_MAP(BlendFactorMapInfoD3D11, GBlendFactorMapD3D11, BLEND_FACTOR_LIST_D3D11);

	D3D11_BLEND D3D11Translate::To(EBlend::Factor factor)
	{
		return GBlendFactorMapD3D11[factor].dest;
	}


	DEFINE_MAP_INFO(BlendOpMapInfoD3D11, EBlend::Operation, D3D11_BLEND_OP);

#define BLEND_OP_LIST_D3D11( op )\
	op(EBlend::Add, D3D11_BLEND_OP_ADD)\
	op(EBlend::Sub, D3D11_BLEND_OP_SUBTRACT)\
	op(EBlend::ReverseSub, D3D11_BLEND_OP_REV_SUBTRACT)\
	op(EBlend::Max, D3D11_BLEND_OP_MAX)\
	op(EBlend::Min, D3D11_BLEND_OP_MIN)

	DEFINE_DATA_MAP(BlendOpMapInfoD3D11, GBlendOpMapD3D11, BLEND_OP_LIST_D3D11);

	D3D11_BLEND_OP D3D11Translate::To(EBlend::Operation op)
	{
		return GBlendOpMapD3D11[op].dest;
	}

	DEFINE_MAP_INFO(CullModeMapInfoD3D11, ECullMode, D3D11_CULL_MODE);

#define CULL_MODE_LIST_D3D11( op )\
	op(ECullMode::None, D3D11_CULL_NONE)\
	op(ECullMode::Front, D3D11_CULL_FRONT)\
	op(ECullMode::Back, D3D11_CULL_BACK)
	
	DEFINE_DATA_MAP(CullModeMapInfoD3D11, GCullModeMapD3D11, CULL_MODE_LIST_D3D11);

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

	DEFINE_MAP_INFO(LockAccessMapInfoD3D11, ELockAccess, D3D11_MAP);

#define LOCK_ACCESS_LIST_D3D11( op )\
	op(ELockAccess::ReadOnly, D3D11_MAP_READ)\
	op(ELockAccess::ReadWrite, D3D11_MAP_READ_WRITE)\
	op(ELockAccess::WriteOnly, D3D11_MAP_WRITE)\
	op(ELockAccess::WriteDiscard, D3D11_MAP_WRITE_DISCARD)

	DEFINE_DATA_MAP(LockAccessMapInfoD3D11, GLockAccessMapD3D11, LOCK_ACCESS_LIST_D3D11);

	D3D11_MAP D3D11Translate::To(ELockAccess access)
	{
		return GLockAccessMapD3D11[int(access)].dest;
	}

	D3D11_FILTER D3D11Translate::To(ESampler::Filter filter)
	{
		switch( filter )
		{
		case ESampler::Point:
			return D3D11_FILTER_MIN_MAG_MIP_POINT;
		case ESampler::Bilinear:
			return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		case ESampler::Trilinear:
			return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		case ESampler::AnisotroicPoint:
		case ESampler::AnisotroicLinear:
			return D3D11_FILTER_ANISOTROPIC;
		}
		return D3D11_FILTER_MIN_MAG_MIP_POINT;
	}

	D3D11_TEXTURE_ADDRESS_MODE D3D11Translate::To(ESampler::AddressMode mode)
	{
		switch( mode )
		{
		case ESampler::Warp:   return D3D11_TEXTURE_ADDRESS_WRAP;
		case ESampler::Clamp:  return D3D11_TEXTURE_ADDRESS_CLAMP;
		case ESampler::Mirror: return D3D11_TEXTURE_ADDRESS_MIRROR;
		case ESampler::Border: return D3D11_TEXTURE_ADDRESS_BORDER;
		}
		return D3D11_TEXTURE_ADDRESS_WRAP;
	}

	DEFINE_MAP_INFO(CompareFuncMapInfoD3D11, ECompareFunc, D3D11_COMPARISON_FUNC);

#define COMPAREFUNC_OP_LIST_D3D11( op )\
	op(ECompareFunc::Never, D3D11_COMPARISON_NEVER)\
	op(ECompareFunc::Less, D3D11_COMPARISON_LESS)\
	op(ECompareFunc::Equal, D3D11_COMPARISON_EQUAL)\
	op(ECompareFunc::LessEqual, D3D11_COMPARISON_LESS_EQUAL)\
	op(ECompareFunc::Greater, D3D11_COMPARISON_GREATER)\
	op(ECompareFunc::NotEqual, D3D11_COMPARISON_NOT_EQUAL)\
	op(ECompareFunc::GreaterEqual, D3D11_COMPARISON_GREATER_EQUAL)\
	op(ECompareFunc::Always, D3D11_COMPARISON_ALWAYS)
	
	DEFINE_DATA_MAP(CompareFuncMapInfoD3D11, GCompareFuncMapD3D11, COMPAREFUNC_OP_LIST_D3D11);

	D3D11_COMPARISON_FUNC D3D11Translate::To(ECompareFunc func)
	{
		return GCompareFuncMapD3D11[uint(func)].dest;
	}

	DEFINE_MAP_INFO(StencilOpMapInfoD3D11, EStencil, D3D11_STENCIL_OP);

#define STENCIL_OP_LIST_D3D11( op )\
	op(EStencil::Keep, D3D11_STENCIL_OP_KEEP)\
	op(EStencil::Zero, D3D11_STENCIL_OP_ZERO)\
	op(EStencil::Replace, D3D11_STENCIL_OP_REPLACE)\
	op(EStencil::Incr, D3D11_STENCIL_OP_INCR_SAT)\
	op(EStencil::IncrWarp, D3D11_STENCIL_OP_INCR)\
	op(EStencil::Decr, D3D11_STENCIL_OP_DECR_SAT)\
	op(EStencil::DecrWarp, D3D11_STENCIL_OP_DECR)\
	op(EStencil::Invert, D3D11_STENCIL_OP_INVERT)

	DEFINE_DATA_MAP(StencilOpMapInfoD3D11, GStencilOpMapD3D11, STENCIL_OP_LIST_D3D11);

	D3D11_STENCIL_OP D3D11Translate::To(EStencil op)
	{
		return GStencilOpMapD3D11[uint(op)].dest;
	}


	InlineString<32> FD3D11Utility::GetShaderProfile(D3D_FEATURE_LEVEL featureLevel, EShader::Type type)
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
		InlineString<32> result = ShaderNames[type];
		result += "_";
		result += featureName;
		return result;
	}

	void D3D11InputLayout::initialize(InputLayoutDesc const& desc)
	{
		mAttriableMask = 0;
		for (auto const& e : desc.mElements)
		{
			if (e.attribute == EVertex::ATTRIBUTE_UNUSED)
				continue;

			D3D11_INPUT_ELEMENT_DESC element;
			element.SemanticName = "ATTRIBUTE";
			element.SemanticIndex = e.attribute;
			element.Format = D3D11Translate::To(EVertex::Format(e.format), e.bNormalized);
			element.InputSlot = e.streamIndex;
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

	ID3D11InputLayout* D3D11InputLayout::getShaderLayout(ID3D11Device* device, RHIResource* resource , TArray< uint8 > const& shaderByteCode)
	{
		auto iter = mResourceMap.find(resource);
		if (iter != mResourceMap.end())
		{
			return iter->second;
		}

		ID3D11InputLayout* inputLayoutResource = nullptr;
		HRESULT hr = device->CreateInputLayout(mDescList.data(), mDescList.size(), shaderByteCode.data(), shaderByteCode.size(), &inputLayoutResource);
		if (hr != S_OK)
		{
			LogWarning(0 , "Can't create D3D11InputLayout , code = %d" , hr );
			mUniversalResource->AddRef();
			mResourceMap.insert(std::make_pair(resource, mUniversalResource));
			return mUniversalResource;
		}
		mResourceMap.insert(std::make_pair(resource, inputLayoutResource));
		return inputLayoutResource;
	}


	int D3D11FrameBuffer::addTexture(RHITexture2D& target, int level)
	{
		CHECK(mRenderTargetsState.numColorBuffers + 1 <= D3D11RenderTargetsState::MaxSimulationBufferCount);
		int indexSlot = mRenderTargetsState.numColorBuffers;
		mRenderTargetsState.colorResources[indexSlot] = &target;
		mRenderTargetsState.colorBuffers[indexSlot] = static_cast<D3D11Texture2D&>(target).getRenderTargetView(level);
		mRenderTargetsState.numColorBuffers += 1;
		bStateDirty = true;
		return indexSlot;
	}

	int D3D11FrameBuffer::addTexture(RHITextureCube& target, ETexture::Face face, int level)
	{
		CHECK(mRenderTargetsState.numColorBuffers + 1 <= D3D11RenderTargetsState::MaxSimulationBufferCount);
		int indexSlot = mRenderTargetsState.numColorBuffers;
		mRenderTargetsState.colorResources[indexSlot] = &target;
		mRenderTargetsState.colorBuffers[indexSlot] = static_cast<D3D11TextureCube&>(target).getRenderTargetView(face, level);
		mRenderTargetsState.numColorBuffers += 1;
		bStateDirty = true;
		return indexSlot;
	}

	void D3D11FrameBuffer::setTexture(int idx, RHITexture2D& target, int level)
	{
		CHECK(idx <= mRenderTargetsState.numColorBuffers);
		if (idx == mRenderTargetsState.numColorBuffers)
			mRenderTargetsState.numColorBuffers += 1;
		mRenderTargetsState.colorResources[idx] = &target;
		mRenderTargetsState.colorBuffers[idx] = static_cast<D3D11Texture2D&>(target).getRenderTargetView(level);
		bStateDirty = true;
	}

	void D3D11FrameBuffer::setTexture(int idx, RHITextureCube& target, ETexture::Face face, int level)
	{
		CHECK(idx <= mRenderTargetsState.numColorBuffers);
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

	void D3D11SwapChain::updateRenderTargetsState()
	{
		mRenderTargetsState.numColorBuffers = 1;
		mRenderTargetsState.colorBuffers[0] = mColorTexture->getRenderTargetView(0);
		if (mDepthTexture)
		{
			mRenderTargetsState.depthResource = mDepthTexture;
			mRenderTargetsState.depthBuffer = mDepthTexture->mDSV;
		}
	}

	void D3D11SwapChain::resizeBuffer(int w, int h)
	{
		TextureDesc colorDesc = mColorTexture->getDesc();
		mColorTexture.release();
		DXGI_SWAP_CHAIN_DESC desc;
		mResource->GetDesc(&desc);

		//Get E_ACCESSDENIED when window close
		VERIFY_D3D_RESULT(mResource->ResizeBuffers(desc.BufferCount, w, h, DXGI_FORMAT_UNKNOWN, desc.Flags), return; );

		Texture2DCreationResult textureCreationResult;
		VERIFY_D3D_RESULT(mResource->GetBuffer(0, IID_PPV_ARGS(&textureCreationResult.resource)), );
		CreateResourceView(static_cast<D3D11System*>(GRHISystem)->mDevice, desc.BufferDesc.Format, colorDesc.numSamples, colorDesc.creationFlags, textureCreationResult);

		colorDesc.dimension = IntVector3(w, h, 1);
		mColorTexture = new D3D11Texture2D(colorDesc, textureCreationResult);
		if (mDepthTexture.isValid())
		{
			TextureDesc depthDesc = mDepthTexture->getDesc();
			depthDesc.dimension = IntVector3(w, h, 1);
			mDepthTexture = (D3D11Texture2D*)RHICreateTextureDepth(depthDesc);
		}

		updateRenderTargetsState();
	}

}//namespace Render