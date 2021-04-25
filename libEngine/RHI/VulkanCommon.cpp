#include "VulkanCommon.h"

#include "VulkanCommand.h"

#pragma comment(lib , "vulkan-1.lib")

namespace Render
{
	VkPrimitiveTopology VulkanTranslate::To(EPrimitive type, int& outPatchPointCount )
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
		case EPrimitive::LineLoop:
		case EPrimitive::Quad:
		case EPrimitive::Polygon:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		}

		if (type >= EPrimitive::PatchPoint1)
		{
			outPatchPointCount = 1 + int(type) - int(EPrimitive::PatchPoint1);
			return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		}
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	}

	VkBlendFactor VulkanTranslate::To(EBlend::Factor factor)
	{
		switch (factor)
		{
		case EBlend::One:
			return VK_BLEND_FACTOR_ONE;
		case EBlend::Zero:
			return VK_BLEND_FACTOR_ZERO;
		case EBlend::SrcAlpha:
			return VK_BLEND_FACTOR_SRC_ALPHA;
		case EBlend::OneMinusSrcAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case EBlend::DestAlpha:
			return VK_BLEND_FACTOR_DST_ALPHA;
		case EBlend::OneMinusDestAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case EBlend::SrcColor:
			return VK_BLEND_FACTOR_SRC_COLOR;
		case EBlend::OneMinusSrcColor:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case EBlend::DestColor:
			return VK_BLEND_FACTOR_DST_COLOR;
		case EBlend::OneMinusDestColor:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		}

		return VK_BLEND_FACTOR_ONE;
	}

	VkBlendOp VulkanTranslate::To(EBlend::Operation op)
	{
		switch (op)
		{
		case EBlend::Add: return VK_BLEND_OP_ADD;
		case EBlend::Sub: return VK_BLEND_OP_SUBTRACT;
		case EBlend::ReverseSub: return VK_BLEND_OP_REVERSE_SUBTRACT;
		case EBlend::Max: return VK_BLEND_OP_MAX;
		case EBlend::Min: return VK_BLEND_OP_MIN;
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


	VkFilter VulkanTranslate::To(ESampler::Filter filter)
	{
		switch (filter)
		{
		case ESampler::Point: 
			return VK_FILTER_NEAREST;

		case ESampler::Trilinear:
		case ESampler::Bilinear: 		
		case ESampler::AnisotroicPoint:
		case ESampler::AnisotroicLinear:
			return VK_FILTER_LINEAR;
		}

		return VK_FILTER_NEAREST;
	}


	VkFormat VulkanTranslate::To(EVertex::Format format, bool bNormalized)
	{
		switch (format)
		{
		case EVertex::Float1: return VK_FORMAT_R32_SFLOAT;
		case EVertex::Float2: return VK_FORMAT_R32G32_SFLOAT;
		case EVertex::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
		case EVertex::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case EVertex::Half1:  return VK_FORMAT_R16_SFLOAT;
		case EVertex::Half2:  return VK_FORMAT_R16G16_SFLOAT;
		case EVertex::Half3:  return VK_FORMAT_R16G16B16_SFLOAT;
		case EVertex::Half4:  return VK_FORMAT_R16G16B16A16_SFLOAT;
		case EVertex::UInt1:  return VK_FORMAT_R32_UINT;
		case EVertex::UInt2:  return VK_FORMAT_R32G32_UINT;
		case EVertex::UInt3:  return VK_FORMAT_R32G32B32_UINT;
		case EVertex::UInt4:  return VK_FORMAT_R32G32B32A32_UINT;
		case EVertex::Int1:   return VK_FORMAT_R32_SINT;
		case EVertex::Int2:   return VK_FORMAT_R32G32_SINT;
		case EVertex::Int3:   return VK_FORMAT_R32G32B32_SINT;
		case EVertex::Int4:   return VK_FORMAT_R32G32B32A32_SINT;
		case EVertex::UShort1:return (bNormalized) ? VK_FORMAT_R16_UNORM : VK_FORMAT_R16_UINT;
		case EVertex::UShort2:return (bNormalized) ? VK_FORMAT_R16G16_UNORM : VK_FORMAT_R16G16_UINT;
		case EVertex::UShort3:return (bNormalized) ? VK_FORMAT_R16G16B16_UNORM : VK_FORMAT_R16G16B16_UINT;
		case EVertex::UShort4:return (bNormalized) ? VK_FORMAT_R16G16B16A16_UNORM : VK_FORMAT_R16G16B16A16_UINT;
		case EVertex::Short1: return (bNormalized) ? VK_FORMAT_R16_SNORM : VK_FORMAT_R16_SINT;
		case EVertex::Short2: return (bNormalized) ? VK_FORMAT_R16G16_SNORM : VK_FORMAT_R16G16_SINT;
		case EVertex::Short3: return (bNormalized) ? VK_FORMAT_R16G16B16_SNORM : VK_FORMAT_R16G16B16_SINT;
		case EVertex::Short4: return (bNormalized) ? VK_FORMAT_R16G16B16A16_SNORM : VK_FORMAT_R16G16B16A16_SINT;
		case EVertex::UByte1: return (bNormalized) ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8_UINT;
		case EVertex::UByte2: return (bNormalized) ? VK_FORMAT_R8G8_UNORM : VK_FORMAT_R8G8_UINT;
		case EVertex::UByte3: return (bNormalized) ? VK_FORMAT_R8G8B8_UNORM : VK_FORMAT_R8G8B8_UINT;
		case EVertex::UByte4: return (bNormalized) ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_UINT;
		case EVertex::Byte1:  return (bNormalized) ? VK_FORMAT_R8_SNORM : VK_FORMAT_R8_SINT;
		case EVertex::Byte2:  return (bNormalized) ? VK_FORMAT_R8G8_SNORM : VK_FORMAT_R8G8_SINT;
		case EVertex::Byte3:  return (bNormalized) ? VK_FORMAT_R8G8B8_SNORM : VK_FORMAT_R8G8B8_SINT;
		case EVertex::Byte4:  return (bNormalized) ? VK_FORMAT_R8G8B8A8_SNORM : VK_FORMAT_R8G8B8A8_SINT;
		}

		return VK_FORMAT_UNDEFINED;
	}

	VkFormat VulkanTranslate::To(ETexture::Format format)
	{
		switch (format)
		{
		case ETexture::RGBA8:   return VK_FORMAT_R8G8B8A8_UNORM;
		case ETexture::RGB8:    return VK_FORMAT_R8G8B8_UNORM;
		case ETexture::BGRA8:   return VK_FORMAT_B8G8R8A8_UNORM;
		case ETexture::RGB10A2: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		case ETexture::R16:     return VK_FORMAT_R16_UNORM;
		case ETexture::R8:      return VK_FORMAT_R8_UNORM;
		case ETexture::R32F:    return VK_FORMAT_R32_SFLOAT;
		case ETexture::RG32F:   return VK_FORMAT_R32G32_SFLOAT;
		case ETexture::RGB32F:  return VK_FORMAT_R32G32B32_SFLOAT;
		case ETexture::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case ETexture::R16F:    return VK_FORMAT_R16_SFLOAT;
		case ETexture::RG16F:   return VK_FORMAT_R16G16_SFLOAT;
		case ETexture::RGB16F:  return VK_FORMAT_R16G16B16_SFLOAT;
		case ETexture::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case ETexture::R32I:    return VK_FORMAT_R32_SINT;
		case ETexture::R16I:    return VK_FORMAT_R16_SINT;
		case ETexture::R8I:     return VK_FORMAT_R8_SINT;
		case ETexture::R32U:    return VK_FORMAT_R32_UINT;
		case ETexture::R16U:    return VK_FORMAT_R16_UINT;
		case ETexture::R8U:     return VK_FORMAT_R8_UINT;
		case ETexture::RG32I:   return VK_FORMAT_R32G32_SINT;
		case ETexture::RG16I:   return VK_FORMAT_R16G16_SINT;
		case ETexture::RG8I:    return VK_FORMAT_R8G8_SINT;
		case ETexture::RG32U:   return VK_FORMAT_R32G32_UINT;
		case ETexture::RG16U:   return VK_FORMAT_R16G16_UINT;
		case ETexture::RG8U:    return VK_FORMAT_R8G8_UINT;
		case ETexture::RGB32I:  return VK_FORMAT_R32G32B32_SINT;
		case ETexture::RGBA32I: return VK_FORMAT_R32G32B32A32_SINT;
		case ETexture::RGB16I:  return VK_FORMAT_R16G16B16_SINT;
		case ETexture::RGBA16I: return VK_FORMAT_R16G16B16A16_SINT;
		case ETexture::RGB8I:   return VK_FORMAT_R8G8B8_SINT;
		case ETexture::RGBA8I:  return VK_FORMAT_R8G8B8A8_SINT;
		case ETexture::RGB32U:  return VK_FORMAT_R32G32B32_UINT;
		case ETexture::RGBA32U: return VK_FORMAT_R32G32B32A32_UINT;
		case ETexture::RGB16U:  return VK_FORMAT_R16G16B16_UINT;
		case ETexture::RGBA16U: return VK_FORMAT_R16G16B16A16_UINT;
		case ETexture::RGB8U:   return VK_FORMAT_R8G8B8_UINT;
		case ETexture::RGBA8U:  return VK_FORMAT_R8G8B8A8_UINT;
		case ETexture::SRGB:    return VK_FORMAT_R8G8B8_SRGB;
		case ETexture::SRGBA:   return VK_FORMAT_R8G8B8A8_SRGB;


		case ETexture::Depth16:   return VK_FORMAT_D16_UNORM;
		case ETexture::Depth24:   return VK_FORMAT_X8_D24_UNORM_PACK32;
		case ETexture::Depth32F:  return VK_FORMAT_D32_SFLOAT;
		case ETexture::D24S8:     return VK_FORMAT_D24_UNORM_S8_UINT;
		case ETexture::D32FS8:    return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case ETexture::Stencil1:  return VK_FORMAT_S8_UINT;
		case ETexture::Stencil4:  return VK_FORMAT_S8_UINT;
		case ETexture::Stencil8:  return VK_FORMAT_S8_UINT;
		case ETexture::Stencil16: return VK_FORMAT_R16_UINT;
		case ETexture::Depth32:   //return VK_FORMAT_R32_UINT;
		default:


			LogWarning(0, "Vulkam No Support Texture Format %d", (int)format);
		}

		return VK_FORMAT_UNDEFINED;
	}


	VkShaderStageFlagBits VulkanTranslate::To(EShader::Type type)
	{
		switch (type)
		{
		case EShader::Vertex:   return VK_SHADER_STAGE_VERTEX_BIT;
		case EShader::Pixel:    return VK_SHADER_STAGE_FRAGMENT_BIT;
		case EShader::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case EShader::Compute:  return VK_SHADER_STAGE_COMPUTE_BIT;
		case EShader::Hull:     return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case EShader::Domain:   return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		}

		return VK_SHADER_STAGE_VERTEX_BIT;
	}

	VkFrontFace VulkanTranslate::To(EFrontFace face)
	{
		switch (face)
		{
		case EFrontFace::Default: return VK_FRONT_FACE_CLOCKWISE;
		case EFrontFace::Inverse: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		}
		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}

	VkSamplerMipmapMode VulkanTranslate::ToMipmapMode(ESampler::Filter filter)
	{
		switch (filter)
		{
		case ESampler::Point:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;

		case ESampler::Trilinear:
		case ESampler::Bilinear:
		case ESampler::AnisotroicPoint:
		case ESampler::AnisotroicLinear:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}

		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}

	VkSamplerAddressMode VulkanTranslate::To(ESampler::AddressMode mode)
	{
		switch (mode)
		{
		case ESampler::Warp: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case ESampler::Clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case ESampler::Mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case ESampler::Border: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
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

	VkStencilOp VulkanTranslate::To(EStencil op)
	{
		switch (op)
		{
		case EStencil::Keep: return VK_STENCIL_OP_KEEP;
		case EStencil::Zero: return VK_STENCIL_OP_ZERO;
		case EStencil::Replace: return VK_STENCIL_OP_REPLACE;
		case EStencil::Incr: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case EStencil::IncrWarp: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case EStencil::Decr: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case EStencil::DecrWarp: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		case EStencil::Invert: return VK_STENCIL_OP_INVERT;
		}
		return VK_STENCIL_OP_KEEP;
	}

	VkImageUsageFlags FVulkanTexture::TranslateUsage(uint32 createFlags)
	{
		VkImageUsageFlags result = VK_IMAGE_USAGE_SAMPLED_BIT;
		if (createFlags & TextureCreationFlag::TCF_RenderTarget)
			result |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (createFlags & TextureCreationFlag::TCF_CreateUAV)
			result |= VK_IMAGE_USAGE_STORAGE_BIT;

		return result;
	}

	void FVulkanTexture::SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange const& subresourceRange, VkPipelineStageFlags srcStageMask /*= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT*/, VkPipelineStageFlags dstStageMask /*= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT*/)
	{
		// Create an image barrier object
		VkImageMemoryBarrier imageMemoryBarrier = FVulkanInit::imageMemoryBarrier();
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		// Source layouts (old)
		// Source access mask controls actions that have to be finished on the old layout
		// before it will be transitioned to the new layout
		switch (oldImageLayout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			imageMemoryBarrier.srcAccessMask = 0;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source 
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
		}

		// Target layouts (new)
		// Destination access mask controls the dependency for the new image layout
		switch (newImageLayout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (imageMemoryBarrier.srcAccessMask == 0)
			{
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
		}

		// Put barrier inside setup command buffer
		vkCmdPipelineBarrier(
			cmdbuffer,
			srcStageMask,
			dstStageMask,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);
	}

}//namespace Render

