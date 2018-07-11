#include "D3D11Common.h"

namespace RenderGL
{
	D3D_PRIMITIVE_TOPOLOGY D3D11Conv::To(PrimitiveType type)
	{
		switch( type )
		{
		case PrimitiveType::Points: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case PrimitiveType::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case PrimitiveType::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case PrimitiveType::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case PrimitiveType::LineStrip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case PrimitiveType::TriangleAdjacency: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
		case PrimitiveType::TriangleFan:
		case PrimitiveType::LineLoop:
		case PrimitiveType::Quad:
		case PrimitiveType::Patchs:
		default:
			LogWarning(0, "D3D11 No Support Primitive %d", (int)type);
			break;
		}
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}

	DXGI_FORMAT D3D11Conv::To(Texture::Format format)
	{
		switch( format )
		{
		case Texture::eRGB8:
		case Texture::eRGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
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
		default:
			LogWarning(0, "D3D11 No Support Texture Format %d", (int)format);
			break;
		}
		return DXGI_FORMAT_UNKNOWN;
	}

	D3D11_BLEND D3D11Conv::To(Blend::Factor factor)
	{
		switch( factor )
		{
		case Blend::eOne: return D3D11_BLEND_ONE;
		case Blend::eZero: return D3D11_BLEND_ZERO;
		case Blend::eSrcAlpha: return D3D11_BLEND_SRC_ALPHA;
		case Blend::eOneMinusSrcAlpha: return D3D11_BLEND_INV_SRC_ALPHA;
		case Blend::eDestAlpha: return D3D11_BLEND_DEST_ALPHA;
		case Blend::eOneMinusDestAlpha: return D3D11_BLEND_INV_DEST_ALPHA;
		case Blend::eSrcColor: return  D3D11_BLEND_SRC_COLOR;
		case Blend::eOneMinusSrcColor: return D3D11_BLEND_INV_SRC_COLOR;
		case Blend::eDestColor: return D3D11_BLEND_DEST_COLOR;
		case Blend::eOneMinusDestColor: return D3D11_BLEND_INV_DEST_COLOR;
		default:
			break;
		}
		return D3D11_BLEND_ONE;
	}

	D3D11_BLEND_OP D3D11Conv::To(Blend::Operation op)
	{
		switch( op )
		{
		case Blend::eAdd: return D3D11_BLEND_OP_ADD;
		case Blend::eSub: return D3D11_BLEND_OP_SUBTRACT;
		case Blend::eReverseSub: return D3D11_BLEND_OP_REV_SUBTRACT;
		case Blend::eMax: return D3D11_BLEND_OP_MAX;
		case Blend::eMin: return D3D11_BLEND_OP_MIN;
		}
		return D3D11_BLEND_OP_ADD;
	}



}//namespace RenderGL