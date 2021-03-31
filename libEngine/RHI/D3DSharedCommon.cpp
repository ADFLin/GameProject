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

	DXGI_FORMAT D3DTranslate::To(ETexture::Format format)
	{
		switch (format)
		{
			//case ETexture::eRGB8:
		case ETexture::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case ETexture::BGRA8: return DXGI_FORMAT_B8G8R8A8_UNORM;
		case ETexture::RGB10A2: return DXGI_FORMAT_R10G10B10A2_UNORM;
		case ETexture::R16:   return DXGI_FORMAT_R16_UNORM;
		case ETexture::R8:    return DXGI_FORMAT_R8_UNORM;
		case ETexture::R32F:  return DXGI_FORMAT_R32_FLOAT;
		case ETexture::RG32F: return DXGI_FORMAT_R32G32_FLOAT;
		case ETexture::RGB32F: return DXGI_FORMAT_R32G32B32_FLOAT;
		case ETexture::RGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case ETexture::R16F:  return DXGI_FORMAT_R16_FLOAT;
		case ETexture::RG16F: return DXGI_FORMAT_R16G16_FLOAT;
			//case ETexture::eRGB16F:
		case ETexture::RGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case ETexture::R32I: return DXGI_FORMAT_R32_SINT;
		case ETexture::R16I: return DXGI_FORMAT_R16_SINT;
		case ETexture::R8I: return DXGI_FORMAT_R8_SINT;
		case ETexture::R32U: return DXGI_FORMAT_R32_UINT;
		case ETexture::R16U: return DXGI_FORMAT_R16_UINT;
		case ETexture::R8U:  return DXGI_FORMAT_R8_UINT;
		case ETexture::RG32I: return DXGI_FORMAT_R32G32_SINT;
		case ETexture::RG16I: return DXGI_FORMAT_R16G16_SINT;
		case ETexture::RG8I: return DXGI_FORMAT_R8G8_SINT;
		case ETexture::RG32U: return DXGI_FORMAT_R32G32_UINT;
		case ETexture::RG16U: return DXGI_FORMAT_R16G16_UINT;
		case ETexture::RG8U:  return DXGI_FORMAT_R8G8_UINT;
			//case ETexture::eRGB32I:
		case ETexture::RGBA32I: return DXGI_FORMAT_R32G32B32A32_SINT;
			//case ETexture::eRGB16I:
		case ETexture::RGBA16I: return DXGI_FORMAT_R16G16B16A16_SINT;
			//case ETexture::eRGB8I:
		case ETexture::RGBA8I:  return DXGI_FORMAT_R8G8B8A8_SINT;
			//case ETexture::eRGB32U:
		case ETexture::RGBA32U: return DXGI_FORMAT_R32G32B32A32_UINT;
			//case ETexture::eRGB16U:
		case ETexture::RGBA16U: return DXGI_FORMAT_R16G16B16A16_UINT;
			//case ETexture::eRGB8U:
		case ETexture::RGBA8U:  return DXGI_FORMAT_R8G8B8A8_UINT;
			//case ETexture::eSRGB:
		case ETexture::SRGBA:   return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

		case ETexture::Depth16:  return DXGI_FORMAT_D16_UNORM;
		case ETexture::Depth32F: return DXGI_FORMAT_D32_FLOAT;
		case ETexture::D24S8:    return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case ETexture::D32FS8:   return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case ETexture::Depth24:  //return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case ETexture::Stencil1:
		case ETexture::Stencil4:
		case ETexture::Stencil8:
		case ETexture::Depth32:
		case ETexture::Stencil16:
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

