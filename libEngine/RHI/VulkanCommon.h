#pragma once
#ifndef VulkanCommon_H_421FF10B_85F3_4B99_90F9_474EDE8043DF
#define VulkanCommon_H_421FF10B_85F3_4B99_90F9_474EDE8043DF

#include "RHICommon.h"


#include "vulkan/vulkan.h"
#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#include "vulkan/vulkan_win32.h"
#endif

namespace Render
{
	struct VulkanTranslate
	{
		static VkPrimitiveTopology To(EPrimitive type);
		static VkFormat To(Vertex::Format format, bool bNormalized);
		static VkFormat To(Texture::Format format);
		static VkFormat To(Texture::DepthFormat format);
		static VkBlendFactor  To(Blend::Factor factor);
		static VkBlendOp To(Blend::Operation op);
		static VkCullModeFlagBits To(ECullMode mode);
		static VkPolygonMode To(EFillMode mode);
		static VkFilter To(Sampler::Filter filter);
		static VkSamplerMipmapMode ToMipmapMode(Sampler::Filter filter);
		static VkSamplerAddressMode To(Sampler::AddressMode mode);
		static VkCompareOp To(ECompareFunc func);
		static VkStencilOp To(Stencil::Operation op);
	};


	class VulkanRasterizerState : public TRefcountResource<RHIRasterizerState >
	{
	public:
		VulkanRasterizerState(RasterizerStateInitializer const& initializer)
		{
			rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationState.polygonMode = VulkanTranslate::To(initializer.fillMode);
			rasterizationState.cullMode = VulkanTranslate::To(initializer.cullMode);
			rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rasterizationState.rasterizerDiscardEnable = VK_FALSE;
			rasterizationState.depthBiasEnable = VK_FALSE;
			rasterizationState.depthClampEnable = VK_FALSE;
			rasterizationState.lineWidth = 1.0f;

			multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleState.rasterizationSamples;
			multisampleState.sampleShadingEnable;
			multisampleState.minSampleShading;
			multisampleState.pSampleMask;
			multisampleState.alphaToCoverageEnable;
			multisampleState.alphaToOneEnable;
		}

		VkPipelineMultisampleStateCreateInfo   multisampleState = {};
		VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	};

	class VulkanDepthStencilState : public TRefcountResource< RHIDepthStencilState >
	{
	public:
		VulkanDepthStencilState(DepthStencilStateInitializer const& initializer)
		{

			depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilState.flags = 0;
			depthStencilState.depthTestEnable = (initializer.depthFunc != ECompareFunc::Always) || (initializer.bWriteDepth);
			depthStencilState.depthWriteEnable = initializer.bWriteDepth;
			depthStencilState.depthCompareOp = VulkanTranslate::To(initializer.depthFunc);
			depthStencilState.depthBoundsTestEnable = VK_FALSE;
			depthStencilState.stencilTestEnable = initializer.bEnableStencilTest;


			depthStencilState.front.failOp = VulkanTranslate::To(initializer.stencilFailOp);
			depthStencilState.front.passOp = VulkanTranslate::To(initializer.zPassOp);
			depthStencilState.front.depthFailOp = VulkanTranslate::To(initializer.zFailOp); 
			depthStencilState.front.compareOp = VulkanTranslate::To(initializer.stencilFunc);
			depthStencilState.front.compareMask = initializer.stencilReadMask;
			depthStencilState.front.writeMask = initializer.stencilWriteMask;
			depthStencilState.front.reference = 0xff;

			depthStencilState.back.failOp = VulkanTranslate::To(initializer.stencilFailOpBack);
			depthStencilState.back.passOp = VulkanTranslate::To(initializer.zPassOpBack);
			depthStencilState.back.depthFailOp = VulkanTranslate::To(initializer.zFailOpBack);
			depthStencilState.back.compareOp = VulkanTranslate::To(initializer.stencilFunBack);
			depthStencilState.back.compareMask = initializer.stencilReadMask;
			depthStencilState.back.writeMask = initializer.stencilWriteMask;
			depthStencilState.back.reference = 0xff;

			depthStencilState.minDepthBounds = 0.0f;
			depthStencilState.maxDepthBounds = 1.0f;

		}

		VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	};

	class VulkanBlendState : public TRefcountResource< RHIBlendState >
	{
	public:
		VulkanBlendState(BlendStateInitializer const& initializer)
		{
			colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendState.pAttachments = colorAttachmentStates;
			colorBlendState.attachmentCount = initializer.bEnableIndependent ? NumBlendStateTarget : 1;
			colorBlendState.logicOp = VK_LOGIC_OP_COPY;
			colorBlendState.logicOpEnable = VK_FALSE;
			colorBlendState.blendConstants[0] = 0.0f;
			colorBlendState.blendConstants[1] = 0.0f;
			colorBlendState.blendConstants[2] = 0.0f;
			colorBlendState.blendConstants[3] = 0.0f;

			for (int i = 0; i < colorBlendState.attachmentCount; ++i)
			{
				VkPipelineColorBlendAttachmentState& state = colorAttachmentStates[i];
				auto const& targetValue = initializer.targetValues[i];

				state.blendEnable = targetValue.isEnabled();
				state.srcColorBlendFactor = VulkanTranslate::To(targetValue.srcColor);
				state.dstColorBlendFactor = VulkanTranslate::To(targetValue.destColor);
				state.colorBlendOp = VulkanTranslate::To(targetValue.op);
				state.srcAlphaBlendFactor = VulkanTranslate::To(targetValue.srcAlpha);
				state.dstAlphaBlendFactor = VulkanTranslate::To(targetValue.destAlpha);
				state.alphaBlendOp = VulkanTranslate::To(targetValue.opAlpha);

				state.colorWriteMask = 0;
				if (targetValue.writeMask & CWM_R)
					state.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
				if (targetValue.writeMask & CWM_G)
					state.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
				if (targetValue.writeMask & CWM_B)
					state.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
				if (targetValue.writeMask & CWM_A)
					state.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
			}
		}

		VkPipelineColorBlendAttachmentState colorAttachmentStates[NumBlendStateTarget];
		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	};


	class VulkanInputLayout : public TRefcountResource<RHIInputLayout >
	{
	public:
		VulkanInputLayout(InputLayoutDesc const& desc)
		{
			int idxStream = -1;
			bool bStreamInstancedRate = false;
			for (auto const& elemenet : desc.mElements)
			{
				if (elemenet.attribute == Vertex::ATTRIBUTE_UNUSED )
					continue;

				VkVertexInputAttributeDescription attrDesc;
				attrDesc.binding = elemenet.idxStream;
				attrDesc.location = elemenet.attribute;
				attrDesc.offset = elemenet.offset;
				attrDesc.format = VulkanTranslate::To( Vertex::Format( elemenet.format ) , elemenet.bNormalized );
				vertexInputAttrDescs.push_back(attrDesc);

				if (idxStream != elemenet.idxStream)
				{
					if (idxStream != -1)
					{
						VkVertexInputBindingDescription bindingDesc;
						bindingDesc.binding = idxStream;
						bindingDesc.inputRate = bStreamInstancedRate ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
						bindingDesc.stride = desc.getVertexSize(idxStream);
						vertexInputBindingDescs.push_back(bindingDesc);
					}

					idxStream = elemenet.idxStream;
					bStreamInstancedRate = elemenet.bIntanceData;
				}
				else
				{
					if (bStreamInstancedRate != elemenet.bIntanceData)
					{
						LogWarning(0, "Valkan Can't have different input rate in the same stream");
					}
				}
			}

			if (idxStream != -1)
			{
				VkVertexInputBindingDescription bindingDesc;
				bindingDesc.binding = idxStream;
				bindingDesc.inputRate = bStreamInstancedRate ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
				bindingDesc.stride = desc.getVertexSize(idxStream);
				vertexInputBindingDescs.push_back(bindingDesc);
			}

			vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputState.pVertexAttributeDescriptions = vertexInputAttrDescs.data();
			vertexInputState.vertexAttributeDescriptionCount = vertexInputAttrDescs.size();
			vertexInputState.pVertexBindingDescriptions = vertexInputBindingDescs.data();
			vertexInputState.vertexBindingDescriptionCount = vertexInputBindingDescs.size();

		}

		std::vector< VkVertexInputAttributeDescription > vertexInputAttrDescs;
		std::vector< VkVertexInputBindingDescription> vertexInputBindingDescs;
		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
	};
}

#endif // VulkanCommon_H_421FF10B_85F3_4B99_90F9_474EDE8043DF
