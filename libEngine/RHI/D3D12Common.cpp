
#include "D3D12Common.h"
#include "D3D12ShaderCommon.h"

namespace Render
{
	D3D_PRIMITIVE_TOPOLOGY D3D12Translate::To(PrimitiveType type)
	{
		switch (type)
		{
		case PrimitiveType::Points: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case PrimitiveType::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case PrimitiveType::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case PrimitiveType::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case PrimitiveType::LineStrip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case PrimitiveType::TriangleAdjacency: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
		case PrimitiveType::Patchs: return D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
		case PrimitiveType::TriangleFan:
		case PrimitiveType::LineLoop:
		case PrimitiveType::Quad:

		default:
			LogWarning(0, "D3D12 No Support Primitive %d", (int)type);
			break;
		}
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}

	DXGI_FORMAT D3D12Translate::To(Texture::Format format)
	{
		switch( format )
		{
		case Texture::eRGB8:
		case Texture::eRGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Texture::eBGRA8: return DXGI_FORMAT_B8G8R8A8_UNORM;
		case Texture::eR16:   return DXGI_FORMAT_R16_UNORM;
		case Texture::eR8:    return DXGI_FORMAT_R8_UNORM;
		case Texture::eR32F:  return DXGI_FORMAT_R32_FLOAT;
		case Texture::eRGB32F: return DXGI_FORMAT_R32G32B32_FLOAT;
		case Texture::eRGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case Texture::eRGB16F:
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
		case Texture::eRGB32I:
		case Texture::eRGBA32I: return DXGI_FORMAT_R32G32B32A32_SINT;
		case Texture::eRGB16I:
		case Texture::eRGBA16I: return DXGI_FORMAT_R16G16B16A16_SINT;
		case Texture::eRGB8I:
		case Texture::eRGBA8I:  return DXGI_FORMAT_R8G8B8A8_SINT;
		case Texture::eRGB32U:
		case Texture::eRGBA32U: return DXGI_FORMAT_R32G32B32A32_UINT;
		case Texture::eRGB16U:
		case Texture::eRGBA16U: return DXGI_FORMAT_R16G16B16A16_UINT;
		case Texture::eRGB8U:
		case Texture::eRGBA8U:  return DXGI_FORMAT_R8G8B8A8_UINT;

		case Texture::eSRGB:    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case Texture::eSRGBA:   return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		default:
			LogWarning(0, "D3D12 No Support Texture Format %d", (int)format);
			break;
		}
		return DXGI_FORMAT_UNKNOWN;
	}

	DXGI_FORMAT D3D12Translate::To(Texture::DepthFormat format)
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
			LogWarning(0, "D3D12 No Support Texture DepthFormat %d", (int)format);
			break;
		}

		return DXGI_FORMAT_UNKNOWN;

	}

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

	DXGI_FORMAT D3D12Translate::To(Vertex::Format format , bool bNormalized)
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

	D3D12_COMPARISON_FUNC D3D12Translate::To(ECompareFun func)
	{
		switch( func )
		{
		case ECompareFun::Never:        return D3D12_COMPARISON_FUNC_NEVER;
		case ECompareFun::Less:         return D3D12_COMPARISON_FUNC_LESS;
		case ECompareFun::Equal:        return D3D12_COMPARISON_FUNC_EQUAL;
		case ECompareFun::NotEqual:     return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case ECompareFun::LessEqual:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case ECompareFun::Greater:      return D3D12_COMPARISON_FUNC_GREATER;
		case ECompareFun::GeraterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case ECompareFun::Always:       return D3D12_COMPARISON_FUNC_ALWAYS;
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


#if 0


	FixString<32> FD3D11Utility::GetShaderProfile(ID3D11Device* device, Shader::Type type)
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

	ID3D11InputLayout* D3D11InputLayout::GetShaderLayout(ID3D11Device* device, RHIShader* shader)
	{
		auto iter = mResourceMap.find(shader);
		if (iter != mResourceMap.end())
		{
			return iter->second;
		}

		ID3D11InputLayout* inputLayoutResource = nullptr;
		std::vector< uint8 > const& byteCode = static_cast<D3D11Shader*>(shader)->byteCode;
		HRESULT hr = device->CreateInputLayout(elements.data(), elements.size(), byteCode.data(), byteCode.size(), &inputLayoutResource);
		if (hr != S_OK)
		{
			LogWarning(0 , "Can't create D3D11InputLayout , code = %d" , hr );
			mResource->AddRef();
			mResourceMap.insert(std::make_pair(shader, mResource));
			return mResource;
		}
		mResourceMap.insert(std::make_pair(shader, inputLayoutResource));
		return inputLayoutResource;
	}
#endif

}//namespace Render
