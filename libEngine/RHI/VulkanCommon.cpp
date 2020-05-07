#include "VulkanCommon.h"

namespace Render
{
	VkPrimitiveTopology VulkanTranslate::To(EPrimitive type)
	{
		switch (type)
		{
		case EPrimitive::Points: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case EPrimitive::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case EPrimitive::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case EPrimitive::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case EPrimitive::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case EPrimitive::TriangleAdjacency: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
		case EPrimitive::TriangleFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
		case EPrimitive::Patchs: return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

		case EPrimitive::LineLoop:
		case EPrimitive::Quad:
		case EPrimitive::Polygon:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

		}

		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	}

	VkBlendFactor VulkanTranslate::To(Blend::Factor factor)
	{
		switch (factor)
		{
		case Blend::eOne:
			return VK_BLEND_FACTOR_ONE;
		case Blend::eZero:
			return VK_BLEND_FACTOR_ZERO;
		case Blend::eSrcAlpha:
			return VK_BLEND_FACTOR_SRC_ALPHA;
		case Blend::eOneMinusSrcAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case Blend::eDestAlpha:
			return VK_BLEND_FACTOR_DST_ALPHA;
		case Blend::eOneMinusDestAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case Blend::eSrcColor:
			return VK_BLEND_FACTOR_SRC_COLOR;
		case Blend::eOneMinusSrcColor:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case Blend::eDestColor:
			return VK_BLEND_FACTOR_DST_COLOR;
		case Blend::eOneMinusDestColor:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		}

		return VK_BLEND_FACTOR_ONE;
	}

	VkBlendOp VulkanTranslate::To(Blend::Operation op)
	{
		switch (op)
		{
		case Blend::eAdd: return VK_BLEND_OP_ADD;
		case Blend::eSub: return VK_BLEND_OP_SUBTRACT;
		case Blend::eReverseSub: return VK_BLEND_OP_REVERSE_SUBTRACT;
		case Blend::eMax: return VK_BLEND_OP_MAX;
		case Blend::eMin: return VK_BLEND_OP_MIN;
		}

		return VK_BLEND_OP_ADD;
	}

	VkCullModeFlagBits VulkanTranslate::To(ECullMode mode)
	{
		switch (mode)
		{
		case ECullMode::Front: return VK_CULL_MODE_FRONT_BIT;
		case ECullMode::Back:  return VK_CULL_MODE_BACK_BIT;
		case ECullMode::None:  return VK_CULL_MODE_NONE;
		}
		return VK_CULL_MODE_NONE;
	}

	VkPolygonMode VulkanTranslate::To(EFillMode mode)
	{
		switch (mode)
		{
		case EFillMode::Point: return VK_POLYGON_MODE_POINT;
		case EFillMode::Wireframe: return VK_POLYGON_MODE_LINE;
		case EFillMode::Solid: return VK_POLYGON_MODE_FILL;
		case EFillMode::System:
			return VK_POLYGON_MODE_FILL;
		}

		return VK_POLYGON_MODE_FILL;
	}


	VkFilter VulkanTranslate::To(Sampler::Filter filter)
	{
		switch (filter)
		{
		case Sampler::ePoint: 
			return VK_FILTER_NEAREST;

		case Sampler::eTrilinear:
		case Sampler::eBilinear: 		
		case Sampler::eAnisotroicPoint:
		case Sampler::eAnisotroicLinear:
			return VK_FILTER_LINEAR;
		}

		return VK_FILTER_NEAREST;
	}


	VkFormat VulkanTranslate::To(Vertex::Format format, bool bNormalized)
	{
		switch (format)
		{
		case Vertex::eFloat1: return VK_FORMAT_R32_SFLOAT;
		case Vertex::eFloat2: return VK_FORMAT_R32G32_SFLOAT;
		case Vertex::eFloat3: return VK_FORMAT_R32G32B32_SFLOAT;
		case Vertex::eFloat4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case Vertex::eHalf1:  return VK_FORMAT_R16_SFLOAT;
		case Vertex::eHalf2:  return VK_FORMAT_R16G16_SFLOAT;
		case Vertex::eHalf3:  return VK_FORMAT_R16G16B16_SFLOAT;
		case Vertex::eHalf4:  return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Vertex::eUInt1:  return VK_FORMAT_R32_UINT;
		case Vertex::eUInt2:  return VK_FORMAT_R32G32_UINT;
		case Vertex::eUInt3:  return VK_FORMAT_R32G32B32_UINT;
		case Vertex::eUInt4:  return VK_FORMAT_R32G32B32A32_UINT;
		case Vertex::eInt1:   return VK_FORMAT_R32_SINT;
		case Vertex::eInt2:   return VK_FORMAT_R32G32_SINT;
		case Vertex::eInt3:   return VK_FORMAT_R32G32B32_SINT;
		case Vertex::eInt4:   return VK_FORMAT_R32G32B32A32_SINT;
		case Vertex::eUShort1:return (bNormalized) ? VK_FORMAT_R16_UNORM : VK_FORMAT_R16_UINT;
		case Vertex::eUShort2:return (bNormalized) ? VK_FORMAT_R16G16_UNORM : VK_FORMAT_R16G16_UINT;
		case Vertex::eUShort3:return (bNormalized) ? VK_FORMAT_R16G16B16_UNORM : VK_FORMAT_R16G16B16_UINT;
		case Vertex::eUShort4:return (bNormalized) ? VK_FORMAT_R16G16B16A16_UNORM : VK_FORMAT_R16G16B16A16_UINT;
		case Vertex::eShort1: return (bNormalized) ? VK_FORMAT_R16_SNORM : VK_FORMAT_R16_SINT;
		case Vertex::eShort2: return (bNormalized) ? VK_FORMAT_R16G16_SNORM : VK_FORMAT_R16G16_SINT;
		case Vertex::eShort3: return (bNormalized) ? VK_FORMAT_R16G16B16_SNORM : VK_FORMAT_R16G16B16_SINT;
		case Vertex::eShort4: return (bNormalized) ? VK_FORMAT_R16G16B16A16_SNORM : VK_FORMAT_R16G16B16A16_SINT;
		case Vertex::eUByte1: return (bNormalized) ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8_UINT;
		case Vertex::eUByte2: return (bNormalized) ? VK_FORMAT_R8G8_UNORM : VK_FORMAT_R8G8_UINT;
		case Vertex::eUByte3: return (bNormalized) ? VK_FORMAT_R8G8B8_UNORM : VK_FORMAT_R8G8B8_UINT;
		case Vertex::eUByte4: return (bNormalized) ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_UINT;
		case Vertex::eByte1:  return (bNormalized) ? VK_FORMAT_R8_SNORM : VK_FORMAT_R8_SINT;
		case Vertex::eByte2:  return (bNormalized) ? VK_FORMAT_R8G8_SNORM : VK_FORMAT_R8G8_SINT;
		case Vertex::eByte3:  return (bNormalized) ? VK_FORMAT_R8G8B8_SNORM : VK_FORMAT_R8G8B8_SINT;
		case Vertex::eByte4:  return (bNormalized) ? VK_FORMAT_R8G8B8A8_SNORM : VK_FORMAT_R8G8B8A8_SINT;
		}

		return VK_FORMAT_UNDEFINED;
	}

	VkFormat VulkanTranslate::To(Texture::Format format)
	{
		switch (format)
		{
		case Texture::eRGBA8:   return VK_FORMAT_R8G8B8A8_UNORM;
		case Texture::eRGB8:    return VK_FORMAT_R8G8B8_UNORM;
		case Texture::eBGRA8:   return VK_FORMAT_B8G8R8A8_UNORM;
		case Texture::eR16:     return VK_FORMAT_R16_UNORM;
		case Texture::eR8:      return VK_FORMAT_R8_UNORM;
		case Texture::eR32F:    return VK_FORMAT_R32_SFLOAT;
		case Texture::eRGB32F:  return VK_FORMAT_R32G32B32_SFLOAT;
		case Texture::eRGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case Texture::eRGB16F:  return VK_FORMAT_R16G16B16_SFLOAT;
		case Texture::eRGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Texture::eR32I:    return VK_FORMAT_R32_SINT;
		case Texture::eR16I:    return VK_FORMAT_R16_SINT;
		case Texture::eR8I:     return VK_FORMAT_R8_SINT;
		case Texture::eR32U:    return VK_FORMAT_R32_UINT;
		case Texture::eR16U:    return VK_FORMAT_R16_UINT;
		case Texture::eR8U:     return VK_FORMAT_R8_UINT;
		case Texture::eRG32I:   return VK_FORMAT_R32G32_SINT;
		case Texture::eRG16I:   return VK_FORMAT_R16G16_SINT;
		case Texture::eRG8I:    return VK_FORMAT_R8G8_SINT;
		case Texture::eRG32U:   return VK_FORMAT_R32G32_UINT;
		case Texture::eRG16U:   return VK_FORMAT_R16G16_UINT;
		case Texture::eRG8U:    return VK_FORMAT_R8G8_UINT;
		case Texture::eRGB32I:  return VK_FORMAT_R32G32B32_SINT;
		case Texture::eRGBA32I: return VK_FORMAT_R32G32B32A32_SINT;
		case Texture::eRGB16I:  return VK_FORMAT_R16G16B16_SINT;
		case Texture::eRGBA16I: return VK_FORMAT_R16G16B16A16_SINT;
		case Texture::eRGB8I:   return VK_FORMAT_R8G8B8_SINT;
		case Texture::eRGBA8I:  return VK_FORMAT_R8G8B8A8_SINT;
		case Texture::eRGB32U:  return VK_FORMAT_R32G32B32_UINT;
		case Texture::eRGBA32U: return VK_FORMAT_R32G32B32A32_UINT;
		case Texture::eRGB16U:  return VK_FORMAT_R16G16B16_UINT;
		case Texture::eRGBA16U: return VK_FORMAT_R16G16B16A16_UINT;
		case Texture::eRGB8U:   return VK_FORMAT_R8G8B8_UINT;
		case Texture::eRGBA8U:  return VK_FORMAT_R8G8B8A8_UINT;
		case Texture::eSRGB:    return VK_FORMAT_R8G8B8_SRGB;
		case Texture::eSRGBA:   return VK_FORMAT_R8G8B8A8_SRGB;
		}

		return VK_FORMAT_UNDEFINED;
	}

	VkFormat VulkanTranslate::To(Texture::DepthFormat format)
	{
		switch (format)
		{
		case Texture::eDepth16:   return VK_FORMAT_D16_UNORM;
		case Texture::eDepth24:   return VK_FORMAT_X8_D24_UNORM_PACK32;
		case Texture::eDepth32:   return VK_FORMAT_R32_UINT;
		case Texture::eDepth32F:  return VK_FORMAT_D32_SFLOAT;
		case Texture::eD24S8:     return VK_FORMAT_D24_UNORM_S8_UINT;
		case Texture::eD32FS8:    return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case Texture::eStencil1:  return VK_FORMAT_S8_UINT;
		case Texture::eStencil4:  return VK_FORMAT_S8_UINT;
		case Texture::eStencil8:  return VK_FORMAT_S8_UINT;
		case Texture::eStencil16: return VK_FORMAT_R16_UINT;
		}
		return VK_FORMAT_UNDEFINED;
	}

	VkSamplerMipmapMode VulkanTranslate::ToMipmapMode(Sampler::Filter filter)
	{
		switch (filter)
		{
		case Sampler::ePoint:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;

		case Sampler::eTrilinear:
		case Sampler::eBilinear:
		case Sampler::eAnisotroicPoint:
		case Sampler::eAnisotroicLinear:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}

		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}

	VkSamplerAddressMode VulkanTranslate::To(Sampler::AddressMode mode)
	{
		switch (mode)
		{
		case Sampler::eWarp: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case Sampler::eClamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case Sampler::eMirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case Sampler::eBorder: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		}

		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}


	VkCompareOp VulkanTranslate::To(ECompareFunc func)
	{
		switch (func)
		{
		case ECompareFunc::Never: return VK_COMPARE_OP_NEVER;
		case ECompareFunc::Less:  return VK_COMPARE_OP_LESS;
		case ECompareFunc::Equal: return VK_COMPARE_OP_EQUAL;
		case ECompareFunc::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
		case ECompareFunc::LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
		case ECompareFunc::Greater: return VK_COMPARE_OP_GREATER;
		case ECompareFunc::GeraterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case ECompareFunc::Always: return VK_COMPARE_OP_ALWAYS;
		}
		return VK_COMPARE_OP_ALWAYS;
	}

	VkStencilOp VulkanTranslate::To(Stencil::Operation op)
	{
		switch (op)
		{
		case Stencil::eKeep: return VK_STENCIL_OP_KEEP;
		case Stencil::eZero: return VK_STENCIL_OP_ZERO;
		case Stencil::eReplace: return VK_STENCIL_OP_REPLACE;
		case Stencil::eIncr: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case Stencil::eIncrWarp: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case Stencil::eDecr: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case Stencil::eDecrWarp: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		case Stencil::eInvert: return VK_STENCIL_OP_INVERT;
		}
		return VK_STENCIL_OP_KEEP;
	}

}//namespace Render

