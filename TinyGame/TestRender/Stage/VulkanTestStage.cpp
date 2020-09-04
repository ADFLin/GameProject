#include "Stage/TestStageHeader.h"


#include "RHI/ShaderCore.h"
#include "RHI/GpuProfiler.h"
#include "Platform/Windows/WindowsProcess.h"
#include "FileSystem.h"
#include "Core/ScopeExit.h"

#include "RHI/VulkanCommon.h"
#include "RHI/VulkanCommand.h"
#include "RHI/VulkanShaderCommon.h"

#include "RHI/ShaderProgram.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHIGlobalResource.h"

#include "DrawEngine.h"
#include "GameRenderSetup.h"


namespace RenderVulkan
{
	using namespace Meta;
	using namespace Render;


	struct GpuTiming
	{
		void start(VkCommandBuffer commandBuffer, VkQueryPool queryPool)
		{
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, startIndex);
		}
		void end(VkCommandBuffer commandBuffer, VkQueryPool queryPool)
		{
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, endIndex);
		}
		uint32 startIndex;
		uint32 endIndex;
	};

	class VulkanProfileCore : public RHIProfileCore
	{
		bool initiailize(VkDevice device)
		{
			mDevice = device;

			VkQueryPoolCreateInfo queryPoolInfo;
			queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
			queryPoolInfo.queryCount = mQueryCount;
			queryPoolInfo.pipelineStatistics = 0;
			VK_VERIFY_RETURN_FALSE(vkCreateQueryPool(mDevice, &queryPoolInfo, gAllocCB, &mTimestampQueryPool));
		}


		virtual void beginFrame()
		{
			mIndexNextQuery = 0;
		}

		virtual bool endFrame()
		{
			vkGetQueryPoolResults(mDevice, mTimestampQueryPool, 0, mIndexNextQuery, mIndexNextQuery * sizeof(uint64), mTimesampResult.data(), sizeof(uint64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT );
			return true;
		}

		virtual uint32 fetchTiming()
		{

		}

		virtual void startTiming(uint32 timingHandle)
		{

		}
		virtual void endTiming(uint32 timingHandle)
		{

		}
		virtual bool getTimingDuration(uint32 timingHandle, uint64& outDuration)
		{

			return false;
		}
		virtual double getCycleToSecond()
		{
			return mCycleToSecond;
		}

		std::vector<uint64> mTimesampResult;
		VkDevice mDevice;
		VkQueryPool   mTimestampQueryPool;
		uint32 mQueryCount = 128;
		uint32 mIndexNextQuery;
		double mCycleToSecond;
	};


	class SampleData
	{
	public:

		VkDevice mDevice = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT mCallback = VK_NULL_HANDLE;
		VkExtent2D     mSwapChainExtent;
		VkFormat       mSwapChainImageFormat;

		VulkanSystem* mSystem;

		bool InitBirdge()
		{
			mSystem = static_cast<VulkanSystem*>(GRHISystem);
			mDevice = mSystem->mDevice->logicalDevice;
			mSwapChainExtent = mSystem->mSwapChain->mImageSize;
			mSwapChainImageFormat = mSystem->mSwapChain->mImageFormat;

			return true;
		}
		std::vector< VkFramebuffer > mSwapChainFramebuffers;

		bool createSwapchainFrameBuffers()
		{
			mSwapChainFramebuffers.resize(mSystem->mSwapChain->mImageViews.size());

			for( int i = 0; i < mSystem->mSwapChain->mImageViews.size(); ++i )
			{
				VkImageView attachments[] = { mSystem->mSwapChain->mImageViews[i] };
				VkFramebufferCreateInfo frameBufferInfo = FVulkanInit::framebufferCreateInfo();
				frameBufferInfo.renderPass = mSimpleRenderPass;
				frameBufferInfo.pAttachments = attachments;
				frameBufferInfo.attachmentCount = ARRAY_SIZE(attachments);
				frameBufferInfo.width = mSwapChainExtent.width;
				frameBufferInfo.height = mSwapChainExtent.height;
				frameBufferInfo.layers = 1;

				VK_VERIFY_RETURN_FALSE(vkCreateFramebuffer(mDevice, &frameBufferInfo, gAllocCB, &mSwapChainFramebuffers[i]));
			}

			return true;
		}


		VkPipeline mSimplePipeline = VK_NULL_HANDLE;
		VkPipelineLayout mEmptyPipelineLayout = VK_NULL_HANDLE;
		VkRenderPass mSimpleRenderPass = VK_NULL_HANDLE;

		RHIVertexBufferRef mVertexBuffer;
		RHIIndexBufferRef  mIndexBuffer;

		ShaderProgram mShaderProgram;
		RHIInputLayoutRef mInputLayout;

		bool createSimplePipepline()
		{
			{
				VkPipelineLayoutCreateInfo pipelineLayoutInfo = FVulkanInit::pipelineLayoutCreateInfo();
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutInfo.pSetLayouts = nullptr;
				pipelineLayoutInfo.setLayoutCount = 0;
				pipelineLayoutInfo.pPushConstantRanges = nullptr;
				pipelineLayoutInfo.pushConstantRangeCount = 0;
				VK_VERIFY_RETURN_FALSE(vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, gAllocCB, &mEmptyPipelineLayout));
			}

			{
				VkAttachmentDescription colorAttachmentDesc = {};
				colorAttachmentDesc.format = mSwapChainImageFormat;
				colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
				colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
#if 0

				attachments[1].format = depthFormat;
				attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
				attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
#endif


				VkAttachmentReference colorAttachmentRef = {};
				colorAttachmentRef.attachment = 0;
				colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				VkSubpassDescription subpass = {};
				subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				subpass.pColorAttachments = &colorAttachmentRef;
				subpass.colorAttachmentCount = 1;
				subpass.pInputAttachments = nullptr;
				subpass.inputAttachmentCount = 0;
				subpass.pResolveAttachments = nullptr;
				subpass.pDepthStencilAttachment = nullptr;
				subpass.preserveAttachmentCount = 0;
				subpass.pPreserveAttachments = nullptr;

				VkSubpassDependency dependencies[2] = {};
				dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
				dependencies[0].dstSubpass = 0;
				dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependencies[0].srcAccessMask = 0;
				dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

				dependencies[1].srcSubpass = 0;                                               // Producer of the dependency is our single subpass
				dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;                             // Consumer are all commands outside of the renderpass
				dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // is a storeOp stage for color attachments
				dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;          // Do not block any subsequent work
				dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;         // is a storeOp `STORE` access mask for color attachments
				dependencies[1].dstAccessMask = 0;
				dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

				VkRenderPassCreateInfo renderPassInfo = {};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
				renderPassInfo.pAttachments = &colorAttachmentDesc;
				renderPassInfo.attachmentCount = 1;
				renderPassInfo.pSubpasses = &subpass;
				renderPassInfo.subpassCount = 1;
				renderPassInfo.pDependencies = dependencies;
				renderPassInfo.dependencyCount = ARRAY_SIZE(dependencies);

				VK_VERIFY_RETURN_FALSE(vkCreateRenderPass(mDevice, &renderPassInfo, gAllocCB, &mSimpleRenderPass));

			}

			struct VertexData
			{
				Vector2 pos;
				Vector2 uv;
				Vector3 color;
			};

			VertexData v[] =
			{
				Vector2(0.5, -0.5),Vector2(1, 0),Vector3(1.0, 0.0, 0.0),
				Vector2(0.5, 0.5),Vector2(1, 1), Vector3(0.0, 1.0, 0.0),
				Vector2(-0.5, 0.5),Vector2(0, 1),Vector3(1.0, 1.0, 1.0),
				Vector2(-0.5, -0.5),Vector2(0, 0),Vector3(1.0, 1.0, 1.0),
			};
			VERIFY_RETURN_FALSE(mVertexBuffer = RHICreateVertexBuffer(sizeof(VertexData), ARRAY_SIZE(v), BCF_DefalutValue, v));
			uint32 indices[] =
			{
				0,1,2,
				0,2,3,
			};
			VERIFY_RETURN_FALSE(mIndexBuffer = RHICreateIndexBuffer(ARRAY_SIZE(indices), true , BCF_DefalutValue , indices));


			struct RenderPipelineState
			{
				RHIRasterizerState*   rasterizerState;
				RHIDepthStencilState* depthStencilState;
				RHIBlendState*        blendState;

				GraphicShaderBoundState shaderPipelineState;
				RHIShaderProgram*   shaderProgram;


				EPrimitive          PrimitiveType;
				RHIInputLayout*     inputLayout;
			};

			RenderPipelineState renderState;
			renderState.PrimitiveType = EPrimitive::TriangleList;
			renderState.rasterizerState = &TStaticRasterizerState<>::GetRHI();
			renderState.depthStencilState = &TStaticDepthStencilState<>::GetRHI();
			renderState.blendState = &TStaticBlendState<>::GetRHI();

			VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mShaderProgram, "Shader/Test/VulkanTest", "MainVS", "MainPS"));
			renderState.shaderProgram = mShaderProgram.getRHIResource();

			setupDescriptorPool();

			pipelineLayout = VulkanCast::To(renderState.shaderProgram)->mPipelineLayout;
			descriptorSetLayout = VulkanCast::To(renderState.shaderProgram)->mDescriptorSetLayout;

			setupDescriptorSet();

			InputLayoutDesc desc;
			desc.addElement(0, 0, Vertex::eFloat2);
			desc.addElement(0, 1, Vertex::eFloat2);
			desc.addElement(0, 2, Vertex::eFloat3);
			VERIFY_RETURN_FALSE(mInputLayout = RHICreateInputLayout(desc));
			renderState.inputLayout = mInputLayout;



			{
				VkGraphicsPipelineCreateInfo pipelineInfo = FVulkanInit::pipelineCreateInfo();
	
				pipelineInfo.renderPass = mSimpleRenderPass;

				VkViewport viewport;
				viewport.x = 0;
				viewport.y = 0;
				viewport.width = mSwapChainExtent.width;
				viewport.height = mSwapChainExtent.height;
				viewport.minDepth = 0;
				viewport.maxDepth = 1;

				VkRect2D scissorRect;
				scissorRect.offset = { 0 , 0 };
				scissorRect.extent = mSwapChainExtent;

				VkPipelineViewportStateCreateInfo viewportState = {};
				viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewportState.pViewports = &viewport;
				viewportState.viewportCount = 1;
				viewportState.pScissors = &scissorRect;
				viewportState.scissorCount = 1;
				pipelineInfo.pViewportState = &viewportState;

				pipelineInfo.pRasterizationState = &VulkanCast::To(renderState.rasterizerState)->createInfo;
				pipelineInfo.pDepthStencilState = &VulkanCast::To(renderState.depthStencilState)->createInfo;
				pipelineInfo.pColorBlendState = &VulkanCast::To(renderState.blendState)->createInfo;

				VkPipelineMultisampleStateCreateInfo multisampleState = {};
				multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampleState.sampleShadingEnable = VK_FALSE;
				multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
				multisampleState.minSampleShading = 1.0f;
				multisampleState.pSampleMask = nullptr;
				multisampleState.alphaToCoverageEnable = VK_FALSE;
				multisampleState.alphaToOneEnable = VK_FALSE;
				VulkanCast::To(renderState.rasterizerState)->getMultiSampleState(multisampleState);
				VulkanCast::To(renderState.blendState)->getMultiSampleState(multisampleState);


				pipelineInfo.pMultisampleState = &multisampleState;

				if (renderState.shaderProgram)
				{
					std::vector < VkPipelineShaderStageCreateInfo > const& shaderStages = VulkanCast::To(renderState.shaderProgram)->mStages;
					pipelineInfo.pStages = shaderStages.data();
					pipelineInfo.stageCount = shaderStages.size();
					pipelineInfo.layout = VulkanCast::To(renderState.shaderProgram)->mPipelineLayout;
				}
				else
				{


				}

				VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
				inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				inputAssemblyState.primitiveRestartEnable = VK_FALSE;
				int patchPointCount = 0;
				inputAssemblyState.topology = VulkanTranslate::To(renderState.PrimitiveType, patchPointCount);
				pipelineInfo.pInputAssemblyState = &inputAssemblyState;

				VkPipelineTessellationStateCreateInfo tesselationState = {};
				tesselationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
				tesselationState.patchControlPoints = patchPointCount;
				pipelineInfo.pTessellationState = &tesselationState;

				VkPipelineVertexInputStateCreateInfo const& vertexInputState = VulkanCast::To(mInputLayout)->createInfo;
				if (renderState.inputLayout)
				{
					pipelineInfo.pVertexInputState = &VulkanCast::To(renderState.inputLayout)->createInfo;
				}


				VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT , VK_DYNAMIC_STATE_LINE_WIDTH };
				VkPipelineDynamicStateCreateInfo dynamicState = {};
				dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				dynamicState.pDynamicStates = dynamicStates;
				dynamicState.dynamicStateCount = ARRAY_SIZE(dynamicStates);
				pipelineInfo.pDynamicState = &dynamicState;

				pipelineInfo.subpass = 0;
				pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
				pipelineInfo.basePipelineIndex = 0;

				VK_VERIFY_RETURN_FALSE(vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, gAllocCB, &mSimplePipeline));
			}

			return true;
		}

		void cleanupSimplePipeline()
		{
			VK_SAFE_DESTROY(mSimpleRenderPass, vkDestroyRenderPass(mDevice, mSimpleRenderPass, gAllocCB));
			VK_SAFE_DESTROY(mEmptyPipelineLayout, vkDestroyPipelineLayout(mDevice, mEmptyPipelineLayout, gAllocCB));
			VK_SAFE_DESTROY(mSimplePipeline, vkDestroyPipeline(mDevice, mSimplePipeline, gAllocCB));
		}

		std::vector< VkCommandBuffer > mCommandBuffers;
		bool createRenderCommand()
		{
			{
				mCommandBuffers.resize(mSwapChainFramebuffers.size());
				VkCommandBufferAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = mSystem->mGraphicsCommandPool;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandBufferCount = mCommandBuffers.size();
				VK_VERIFY_RETURN_FALSE(vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffers.data()));
			}
			{
				for( size_t i = 0; i < mCommandBuffers.size(); ++i )
				{
					auto& commandBuffer = mCommandBuffers[i];
					generateRenderCommand(mCommandBuffers[i], mSwapChainFramebuffers[i]);
				}
			}
			return true;
		}

		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout;
		VkDescriptorSet descriptorSet;
		VkDescriptorSetLayout descriptorSetLayout;

		void setupDescriptorPool()
		{
			// Example uses one image sampler
			std::vector<VkDescriptorPoolSize> poolSizes =
			{
				FVulkanInit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
				FVulkanInit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
			};

			VkDescriptorPoolCreateInfo descriptorPoolInfo =
				FVulkanInit::descriptorPoolCreateInfo(
					static_cast<uint32>(poolSizes.size()),
					poolSizes.data(),
					2);

			VK_CHECK_RESULT(vkCreateDescriptorPool(mDevice, &descriptorPoolInfo, gAllocCB, &descriptorPool));
		}

		RHITexture2DRef texture;
		void setupDescriptorSet()
		{

			VkDescriptorSetAllocateInfo allocInfo =
				FVulkanInit::descriptorSetAllocateInfo(
					descriptorPool,
					&descriptorSetLayout,
					1);

			VK_CHECK_RESULT(vkAllocateDescriptorSets(mDevice, &allocInfo, &descriptorSet));

			texture = RHIUtility::LoadTexture2DFromFile("Texture/rocks.jpg");

			// Setup a descriptor image info for the current texture to be used as a combined image sampler
			VkDescriptorImageInfo textureDescriptor;
			textureDescriptor.imageView = VulkanCast::To(texture)->view; // The image's view (images are never directly accessed by the shader, but rather through views defining subresources)
			textureDescriptor.sampler = VulkanCast::GetHandle(TStaticSamplerState<>::GetRHI()); // The sampler (Telling the pipeline how to sample the texture, including repeat, border, etc.)
			textureDescriptor.imageLayout = VulkanCast::To(texture)->mImageLayout;	// The current layout of the image (Note: Should always fit the actual use, e.g. shader read)

			VkDescriptorBufferInfo bufferDescriptor = {};
			std::vector<VkWriteDescriptorSet> writeDescriptorSets =
			{
				FVulkanInit::writeDescriptorSet(
					descriptorSet,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					0,
					&textureDescriptor),
#if 0
				FVulkanInit::writeDescriptorSet(
					descriptorSet,
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       
					1,				
					&bufferDescriptor)
#endif
			};

			vkUpdateDescriptorSets(mDevice, static_cast<uint32>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
		}

		bool generateRenderCommand(VkCommandBuffer commandBuffer , VkFramebuffer frameBuffer )
		{

			VkCommandBufferBeginInfo bufferBeginInfo = {};
			bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			bufferBeginInfo.pInheritanceInfo = nullptr;
			VK_VERIFY_RETURN_FALSE(vkBeginCommandBuffer(commandBuffer, &bufferBeginInfo));

			{
				VkRenderPassBeginInfo passBeginInfo = {};
				passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				passBeginInfo.renderPass = mSimpleRenderPass;
				passBeginInfo.framebuffer = frameBuffer;
				passBeginInfo.renderArea.offset = { 0,0 };
				passBeginInfo.renderArea.extent = mSwapChainExtent;
				VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
				passBeginInfo.pClearValues = &clearColor;
				passBeginInfo.clearValueCount = 1;


				vkCmdBeginRenderPass(commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				VkViewport viewport;
				viewport.x = 0;
				viewport.y = 0;
				viewport.width = mSwapChainExtent.width;
				viewport.height = mSwapChainExtent.height;
				viewport.minDepth = 0;
				viewport.maxDepth = 1;
				vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSimplePipeline);

				VkBuffer vertexBuffers[] = { VulkanCast::GetHandle( mVertexBuffer ) };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, ARRAY_SIZE(vertexBuffers), vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffer, VulkanCast::GetHandle(mIndexBuffer), 0, VK_INDEX_TYPE_UINT32);
				//vkCmdDraw(commandBuffer, 4, 1, 0, 0);
				vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
				vkCmdEndRenderPass(commandBuffer);
			}

			VK_VERIFY_RETURN_FALSE(vkEndCommandBuffer(commandBuffer));

			return true;
		}

	};


	class TestStage : public StageBase
		            , public SampleData
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::Vulkan;
		}
		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			::Global::GetDrawEngine().bUsePlatformBuffer = false;

			//VERIFY_RETURN_FALSE( initializeSystem( ::Global::GetDrawEngine().getWindow().getHWnd() ) );
			//VERIFY_RETURN_FALSE( createWindowSwapchain() );
			InitBirdge();
			VERIFY_RETURN_FALSE( createSimplePipepline() );
			VERIFY_RETURN_FALSE( createSwapchainFrameBuffers() );
			VERIFY_RETURN_FALSE( createRenderCommand() );

			::Global::GUI().cleanupWidget();

			restart();
			return true;
		}

		virtual void onEnd()
		{
			vkDeviceWaitIdle(mDevice);
			cleanupSimplePipeline();
			BaseClass::onEnd();
		}

		void restart() {}
		void tick() {}
		void updateFrame(int frame) {}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		bool bDynamicGenerate = true;

		void onRender(float dFrame)
		{
			if( bDynamicGenerate )
			{
				generateRenderCommand(mCommandBuffers[mSystem->mRenderImageIndex], mSwapChainFramebuffers[mSystem->mRenderImageIndex]);
			}
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pCommandBuffers = &mCommandBuffers[mSystem->mRenderImageIndex];
			submitInfo.commandBufferCount = 1;

			VkSemaphore waitSemaphores[] = { mSystem->mImageAvailableSemaphores[mSystem->mCurrentFrame] };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.waitSemaphoreCount = ARRAY_SIZE(waitSemaphores);
			
			VkSemaphore signalSemaphores[] = { mSystem->mRenderFinishedSemaphores[mSystem->mCurrentFrame] };
			submitInfo.pSignalSemaphores = signalSemaphores;
			submitInfo.signalSemaphoreCount = ARRAY_SIZE(signalSemaphores);

			VK_VERIFY_FAILCDOE(vkQueueSubmit(mSystem->mGraphicsQueue, 1, &submitInfo, mSystem->mInFlightFences[mSystem->mCurrentFrame]), );
		}

		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(KeyMsg const& msg)
		{
			if( !msg.isDown())
				return false;
			switch(msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
			return false;
		}

		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};


	REGISTER_STAGE("Vulkan Test", TestStage, EStageGroup::FeatureDev);
}

char const* VSCode = CODE_STRING(
	#version 450
	#extension GL_ARB_separate_shader_objects : enable
	layout(location = 0) out vec3 fragColor;

	vec2 positions[3] = vec2[](
		vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5)
	);
	vec3 colors[3] = vec3[](
		vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0)
	);
	void main()
	{
		gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
		fragColor = colors[gl_VertexIndex];
	}
);