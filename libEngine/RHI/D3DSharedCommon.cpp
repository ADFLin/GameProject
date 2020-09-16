#include "D3DSharedCommon.h"

namespace Render
{
	D3D_PRIMITIVE_TOPOLOGY D3DTranslate::To(EPrimitive type)
	{
		if (type >= EPrimitive::PatchPoint1)
		{
			return D3D_PRIMITIVE_TOPOLOGY(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + ((int)type - (int)EPrimitive::PatchPoint1));
		}
		switch (type)
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
			LogWarning(0, "D3D No Support Primitive %d", (int)type);
			break;
		}
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}

	DXGI_FORMAT D3DTranslate::To(Texture::Format format)
	{
		switch (format)
		{
			//case Texture::eRGB8:
		case Texture::eRGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Texture::eBGRA8: return DXGI_FORMAT_B8G8R8A8_UNORM;
		case Texture::eRGB10A2: return DXGI_FORMAT_R10G10B10A2_UNORM;
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

		case Texture::eDepth16:  return DXGI_FORMAT_D16_UNORM;
		case Texture::eDepth32F: return DXGI_FORMAT_D32_FLOAT;
		case Texture::eD24S8:    return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case Texture::eD32FS8:   return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case Texture::eDepth24:  //return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case Texture::eStencil1:
		case Texture::eStencil4:
		case Texture::eStencil8:
		case Texture::eDepth32:
		case Texture::eStencil16:
			//return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		default:
			LogWarning(0, "D3D No Support Texture Format %d", (int)format);
			break;
		}
		return DXGI_FORMAT_UNKNOWN;
	}

	DXGI_FORMAT D3DTranslate::To(Vertex::Format format, bool bNormalized)
	{
		switch (format)
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

}

