#pragma once
#ifndef VulkanCommon_H_421FF10B_85F3_4B99_90F9_474EDE8043DF
#define VulkanCommon_H_421FF10B_85F3_4B99_90F9_474EDE8043DF

#include "RHICommon.h"
#include "FunctionTraits.h"


#include <vector>

#include "vulkan/vulkan.h"
#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#include "vulkan/vulkan_win32.h"
#endif


#pragma comment(lib , "vulkan-1.lib")

#define ERROR_MSG_GENERATE( HR , CODE , FILE , LINE )\
	LogWarning(1, "ErrorCode = 0x%x File = %s Line = %s %s ", HR , FILE, #LINE, Render::FVulkanUtilies::GetResultErrorString(CODE))

#define VERIFY_VK_RESULT_INNER( FILE , LINE , CODE ,ERRORCODE )\
	{ VkResult vkResult = CODE; if( vkResult != VK_SUCCESS ){ ERROR_MSG_GENERATE( vkResult , CODE, FILE, LINE ); ERRORCODE } }

#define VK_VERIFY_FAILCDOE( CODE , ERRORCODE ) VERIFY_VK_RESULT_INNER( __FILE__ , __LINE__ , CODE , ERRORCODE )
#define VK_VERIFY_RETURN_FALSE( CODE ) VERIFY_VK_RESULT_INNER( __FILE__ , __LINE__ , CODE , return false; )
#define VK_CHECK_RESULT( CODE ) VERIFY_VK_RESULT_INNER( __FILE__ , __LINE__ , CODE , assert( #CODE && 0 ); )
#define VK_SAFE_DESTROY( VKHANDLE , CODE )\
	if( VKHANDLE != VK_NULL_HANDLE )\
	{\
		CODE;\
		VKHANDLE = VK_NULL_HANDLE;\
	}

#define VK_FLAGS_NONE 0
#define VK_DEFAULT_FENCE_TIMEOUT 100000000000

namespace Render
{
	class VulkanDevice;
	extern VkAllocationCallbacks* gAllocCB;

	template< class ...FuncArgs, class ...Args >
	FORCEINLINE static auto GetEnumValues(VkResult(VKAPI_CALL &Func)(FuncArgs...), Args... args)
	{
		using namespace ::Meta;
		typedef typename GetArgsType< sizeof...(FuncArgs) - 1, FuncArgs... >::Type ArgType;
		typedef typename std::remove_pointer< ArgType >::type PropertyType;
		uint32_t count = 0;
		VK_CHECK_RESULT(Func(std::forward<Args>(args)..., &count, nullptr));
		std::vector< PropertyType > result{ count };
		VK_VERIFY_FAILCDOE(Func(std::forward<Args>(args)..., &count, result.data()), );
		return result;
	}

	template< class ...FuncArgs, class ...Args >
	FORCEINLINE static auto GetEnumValues(void(VKAPI_CALL &Func)(FuncArgs...), Args... args)
	{
		using namespace ::Meta;
		typedef typename GetArgsType< sizeof...(FuncArgs) - 1, FuncArgs... >::Type ArgType;
		typedef typename std::remove_pointer< ArgType >::type PropertyType;

		uint32_t count = 0;
		Func(std::forward<Args>(args)..., &count, nullptr);
		std::vector< PropertyType > result{ count };
		Func(std::forward<Args>(args)..., &count, result.data());
		return result;
	}

	struct FPropertyName
	{
		static char const* Get(VkLayerProperties const& p) { return p.layerName; }
		static char const* Get(VkExtensionProperties const& p) { return p.extensionName; }
		static char const* Get(char const* name) { return name; }

		template< class T >
		static bool Find(std::vector<T> const& v, char const* name)
		{
			for (auto& element : v)
			{
				if (FCString::Compare(FPropertyName::Get(element), name) == 0)
					return true;
			}
			return false;
		}

		template< class T >
		static void GetAll(std::vector<T> const& v, std::vector< char const* >& outNames)
		{
			for (auto& element : v)
			{
				outNames.push_back(FPropertyName::Get(element));
			}
		}
	};

	class FVulkanUtilies
	{
	public:
		static char const* GetResultErrorString(VkResult errorCode)
		{
			switch (errorCode)
			{
#define CASE(r) case VK_ ##r: return #r
				CASE(NOT_READY);
				CASE(TIMEOUT);
				CASE(EVENT_SET);
				CASE(EVENT_RESET);
				CASE(INCOMPLETE);
				CASE(ERROR_OUT_OF_HOST_MEMORY);
				CASE(ERROR_OUT_OF_DEVICE_MEMORY);
				CASE(ERROR_INITIALIZATION_FAILED);
				CASE(ERROR_DEVICE_LOST);
				CASE(ERROR_MEMORY_MAP_FAILED);
				CASE(ERROR_LAYER_NOT_PRESENT);
				CASE(ERROR_EXTENSION_NOT_PRESENT);
				CASE(ERROR_FEATURE_NOT_PRESENT);
				CASE(ERROR_INCOMPATIBLE_DRIVER);
				CASE(ERROR_TOO_MANY_OBJECTS);
				CASE(ERROR_FORMAT_NOT_SUPPORTED);
				CASE(ERROR_SURFACE_LOST_KHR);
				CASE(ERROR_NATIVE_WINDOW_IN_USE_KHR);
				CASE(SUBOPTIMAL_KHR);
				CASE(ERROR_OUT_OF_DATE_KHR);
				CASE(ERROR_INCOMPATIBLE_DISPLAY_KHR);
				CASE(ERROR_VALIDATION_FAILED_EXT);
				CASE(ERROR_INVALID_SHADER_NV);
#undef CASE
			default:
				return "UNKNOWN_ERROR";
			}
		}
	};

	enum class EVulkanResourceType
	{
		eVkDevice ,
		eVkShaderModule ,
		eVkImage ,
		eVkImageView,
		eVkDeviceMemory,
		eVkBuffer,
		eVkBufferView ,
		eVkSampler ,
	};

	template< EVulkanResourceType Type >
	struct TVulkanResourceTraits
	{
		//template < class ...Args >
		//static bool Create(VkDevice device, T& handle , Args&& ...args);
		//static void Destroy(VkDevice device, T& handle);
		//typedef HandleType;
	};

	template < EVulkanResourceType Type >
	struct TVulkanResource
	{
		typedef typename TVulkanResourceTraits< Type >::HandleType HandleType;

		TVulkanResource()
		{
			mHandle = VK_NULL_HANDLE;
		}

		~TVulkanResource()
		{
			if (mHandle != VK_NULL_HANDLE)
			{

			}
		}

		template < class ...Args >
		bool initialize(VkDevice device, Args&& ...args)
		{
			assert(mHandle == VK_NULL_HANDLE);
			if (!TVulkanResourceTraits< Type >::Create(device, mHandle, std::forward<Args>(args)...))
			{
				return false;
			}
			return true;
		}

		void destroy(VkDevice device)
		{
			if (mHandle != VK_NULL_HANDLE)
			{
				TVulkanResourceTraits< Type >::Destroy(device, mHandle);
				mHandle = VK_NULL_HANDLE;
			}
		}

		operator HandleType(){ return mHandle; }
		HandleType  mHandle;
	};
#define VK_RESOURCE_TYPE(Type) TVulkanResource< EVulkanResourceType::e##Type >

	namespace FVulkanInit
	{
		FORCEINLINE VkMemoryAllocateInfo memoryAllocateInfo()
		{
			VkMemoryAllocateInfo memAllocInfo{};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			return memAllocInfo;
		}

		FORCEINLINE VkMappedMemoryRange mappedMemoryRange()
		{
			VkMappedMemoryRange mappedMemoryRange{};
			mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			return mappedMemoryRange;
		}

		FORCEINLINE VkCommandBufferAllocateInfo commandBufferAllocateInfo(
			VkCommandPool commandPool,
			VkCommandBufferLevel level,
			uint32_t bufferCount)
		{
			VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
			commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocateInfo.commandPool = commandPool;
			commandBufferAllocateInfo.level = level;
			commandBufferAllocateInfo.commandBufferCount = bufferCount;
			return commandBufferAllocateInfo;
		}

		FORCEINLINE VkCommandPoolCreateInfo commandPoolCreateInfo()
		{
			VkCommandPoolCreateInfo cmdPoolCreateInfo{};
			cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			return cmdPoolCreateInfo;
		}

		FORCEINLINE VkCommandBufferBeginInfo commandBufferBeginInfo()
		{
			VkCommandBufferBeginInfo cmdBufferBeginInfo{};
			cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			return cmdBufferBeginInfo;
		}

		FORCEINLINE VkCommandBufferInheritanceInfo commandBufferInheritanceInfo()
		{
			VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo{};
			cmdBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			return cmdBufferInheritanceInfo;
		}

		FORCEINLINE VkRenderPassBeginInfo renderPassBeginInfo()
		{
			VkRenderPassBeginInfo renderPassBeginInfo{};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			return renderPassBeginInfo;
		}

		FORCEINLINE VkRenderPassCreateInfo renderPassCreateInfo()
		{
			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			return renderPassCreateInfo;
		}

		/** @brief Initialize an image memory barrier with no image transfer ownership */
		FORCEINLINE VkImageMemoryBarrier imageMemoryBarrier()
		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			return imageMemoryBarrier;
		}

		/** @brief Initialize a buffer memory barrier with no image transfer ownership */
		FORCEINLINE VkBufferMemoryBarrier bufferMemoryBarrier()
		{
			VkBufferMemoryBarrier bufferMemoryBarrier{};
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			return bufferMemoryBarrier;
		}

		FORCEINLINE VkMemoryBarrier memoryBarrier()
		{
			VkMemoryBarrier memoryBarrier{};
			memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			return memoryBarrier;
		}

		FORCEINLINE VkImageCreateInfo imageCreateInfo()
		{
			VkImageCreateInfo imageCreateInfo{};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			return imageCreateInfo;
		}

		FORCEINLINE VkSamplerCreateInfo samplerCreateInfo()
		{
			VkSamplerCreateInfo samplerCreateInfo{};
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.maxAnisotropy = 1.0f;
			return samplerCreateInfo;
		}

		FORCEINLINE VkImageViewCreateInfo imageViewCreateInfo()
		{
			VkImageViewCreateInfo imageViewCreateInfo{};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			return imageViewCreateInfo;
		}

		FORCEINLINE VkFramebufferCreateInfo framebufferCreateInfo()
		{
			VkFramebufferCreateInfo framebufferCreateInfo{};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			return framebufferCreateInfo;
		}

		FORCEINLINE VkSemaphoreCreateInfo semaphoreCreateInfo()
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo{};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			return semaphoreCreateInfo;
		}

		FORCEINLINE VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0)
		{
			VkFenceCreateInfo fenceCreateInfo{};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = flags;
			return fenceCreateInfo;
		}

		FORCEINLINE VkEventCreateInfo eventCreateInfo()
		{
			VkEventCreateInfo eventCreateInfo{};
			eventCreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
			return eventCreateInfo;
		}

		FORCEINLINE VkSubmitInfo submitInfo()
		{
			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			return submitInfo;
		}

		FORCEINLINE VkViewport viewport(
			float width,
			float height,
			float minDepth,
			float maxDepth)
		{
			VkViewport viewport{};
			viewport.width = width;
			viewport.height = height;
			viewport.minDepth = minDepth;
			viewport.maxDepth = maxDepth;
			return viewport;
		}

		FORCEINLINE VkRect2D rect2D(
			int32_t width,
			int32_t height,
			int32_t offsetX,
			int32_t offsetY)
		{
			VkRect2D rect2D{};
			rect2D.extent.width = width;
			rect2D.extent.height = height;
			rect2D.offset.x = offsetX;
			rect2D.offset.y = offsetY;
			return rect2D;
		}

		FORCEINLINE VkBufferCreateInfo bufferCreateInfo()
		{
			VkBufferCreateInfo bufCreateInfo{};
			bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			return bufCreateInfo;
		}

		FORCEINLINE VkBufferCreateInfo bufferCreateInfo(
			VkBufferUsageFlags usage,
			VkDeviceSize size)
		{
			VkBufferCreateInfo bufCreateInfo{};
			bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufCreateInfo.usage = usage;
			bufCreateInfo.size = size;
			return bufCreateInfo;
		}

		FORCEINLINE VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
			uint32_t poolSizeCount,
			VkDescriptorPoolSize* pPoolSizes,
			uint32_t maxSets)
		{
			VkDescriptorPoolCreateInfo descriptorPoolInfo{};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.poolSizeCount = poolSizeCount;
			descriptorPoolInfo.pPoolSizes = pPoolSizes;
			descriptorPoolInfo.maxSets = maxSets;
			return descriptorPoolInfo;
		}

		FORCEINLINE VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
			const std::vector<VkDescriptorPoolSize>& poolSizes,
			uint32_t maxSets)
		{
			VkDescriptorPoolCreateInfo descriptorPoolInfo{};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			descriptorPoolInfo.pPoolSizes = poolSizes.data();
			descriptorPoolInfo.maxSets = maxSets;
			return descriptorPoolInfo;
		}

		FORCEINLINE VkDescriptorPoolSize descriptorPoolSize(
			VkDescriptorType type,
			uint32_t descriptorCount)
		{
			VkDescriptorPoolSize descriptorPoolSize{};
			descriptorPoolSize.type = type;
			descriptorPoolSize.descriptorCount = descriptorCount;
			return descriptorPoolSize;
		}

		inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
			VkDescriptorType type,
			VkShaderStageFlags stageFlags,
			uint32_t binding,
			uint32_t descriptorCount = 1)
		{
			VkDescriptorSetLayoutBinding setLayoutBinding{};
			setLayoutBinding.descriptorType = type;
			setLayoutBinding.stageFlags = stageFlags;
			setLayoutBinding.binding = binding;
			setLayoutBinding.descriptorCount = descriptorCount;
			return setLayoutBinding;
		}

		FORCEINLINE VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
			const VkDescriptorSetLayoutBinding* pBindings,
			uint32_t bindingCount)
		{
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCreateInfo.pBindings = pBindings;
			descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
			return descriptorSetLayoutCreateInfo;
		}

		FORCEINLINE VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
			const std::vector<VkDescriptorSetLayoutBinding>& bindings)
		{
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCreateInfo.pBindings = bindings.data();
			descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			return descriptorSetLayoutCreateInfo;
		}

		FORCEINLINE VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
			const VkDescriptorSetLayout* pSetLayouts,
			uint32_t setLayoutCount = 1)
		{
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
			pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
			return pipelineLayoutCreateInfo;
		}

		FORCEINLINE VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
			uint32_t setLayoutCount = 1)
		{
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
			return pipelineLayoutCreateInfo;
		}

		FORCEINLINE VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(
			VkDescriptorPool descriptorPool,
			const VkDescriptorSetLayout* pSetLayouts,
			uint32_t descriptorSetCount)
		{
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.descriptorPool = descriptorPool;
			descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
			descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
			return descriptorSetAllocateInfo;
		}

		FORCEINLINE VkDescriptorImageInfo descriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
		{
			VkDescriptorImageInfo descriptorImageInfo{};
			descriptorImageInfo.sampler = sampler;
			descriptorImageInfo.imageView = imageView;
			descriptorImageInfo.imageLayout = imageLayout;
			return descriptorImageInfo;
		}

		FORCEINLINE VkWriteDescriptorSet writeDescriptorSet(
			VkDescriptorSet dstSet,
			VkDescriptorType type,
			uint32_t binding,
			VkDescriptorBufferInfo* bufferInfo,
			uint32_t descriptorCount = 1)
		{
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = dstSet;
			writeDescriptorSet.descriptorType = type;
			writeDescriptorSet.dstBinding = binding;
			writeDescriptorSet.pBufferInfo = bufferInfo;
			writeDescriptorSet.descriptorCount = descriptorCount;
			return writeDescriptorSet;
		}

		FORCEINLINE VkWriteDescriptorSet writeDescriptorSet(
			VkDescriptorSet dstSet,
			VkDescriptorType type,
			uint32_t binding,
			VkDescriptorImageInfo *imageInfo,
			uint32_t descriptorCount = 1)
		{
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = dstSet;
			writeDescriptorSet.descriptorType = type;
			writeDescriptorSet.dstBinding = binding;
			writeDescriptorSet.pImageInfo = imageInfo;
			writeDescriptorSet.descriptorCount = descriptorCount;
			return writeDescriptorSet;
		}

		FORCEINLINE VkVertexInputBindingDescription vertexInputBindingDescription(
			uint32_t binding,
			uint32_t stride,
			VkVertexInputRate inputRate)
		{
			VkVertexInputBindingDescription vInputBindDescription{};
			vInputBindDescription.binding = binding;
			vInputBindDescription.stride = stride;
			vInputBindDescription.inputRate = inputRate;
			return vInputBindDescription;
		}

		FORCEINLINE VkVertexInputAttributeDescription vertexInputAttributeDescription(
			uint32_t binding,
			uint32_t location,
			VkFormat format,
			uint32_t offset)
		{
			VkVertexInputAttributeDescription vInputAttribDescription{};
			vInputAttribDescription.location = location;
			vInputAttribDescription.binding = binding;
			vInputAttribDescription.format = format;
			vInputAttribDescription.offset = offset;
			return vInputAttribDescription;
		}

		FORCEINLINE VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo()
		{
			VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
			pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			return pipelineVertexInputStateCreateInfo;
		}

		FORCEINLINE VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
			VkPrimitiveTopology topology,
			VkPipelineInputAssemblyStateCreateFlags flags,
			VkBool32 primitiveRestartEnable)
		{
			VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
			pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			pipelineInputAssemblyStateCreateInfo.topology = topology;
			pipelineInputAssemblyStateCreateInfo.flags = flags;
			pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
			return pipelineInputAssemblyStateCreateInfo;
		}

		FORCEINLINE VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
			VkPolygonMode polygonMode,
			VkCullModeFlags cullMode,
			VkFrontFace frontFace,
			VkPipelineRasterizationStateCreateFlags flags = 0)
		{
			VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
			pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
			pipelineRasterizationStateCreateInfo.cullMode = cullMode;
			pipelineRasterizationStateCreateInfo.frontFace = frontFace;
			pipelineRasterizationStateCreateInfo.flags = flags;
			pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
			pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
			return pipelineRasterizationStateCreateInfo;
		}

		FORCEINLINE VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
			VkColorComponentFlags colorWriteMask,
			VkBool32 blendEnable)
		{
			VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
			pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
			pipelineColorBlendAttachmentState.blendEnable = blendEnable;
			return pipelineColorBlendAttachmentState;
		}

		FORCEINLINE VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
			uint32_t attachmentCount,
			const VkPipelineColorBlendAttachmentState * pAttachments)
		{
			VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
			pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
			pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;
			return pipelineColorBlendStateCreateInfo;
		}

		FORCEINLINE VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
			VkBool32 depthTestEnable,
			VkBool32 depthWriteEnable,
			VkCompareOp depthCompareOp)
		{
			VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
			pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
			pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
			pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
			pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
			return pipelineDepthStencilStateCreateInfo;
		}

		FORCEINLINE VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
			uint32_t viewportCount,
			uint32_t scissorCount,
			VkPipelineViewportStateCreateFlags flags = 0)
		{
			VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
			pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			pipelineViewportStateCreateInfo.viewportCount = viewportCount;
			pipelineViewportStateCreateInfo.scissorCount = scissorCount;
			pipelineViewportStateCreateInfo.flags = flags;
			return pipelineViewportStateCreateInfo;
		}

		FORCEINLINE VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
			VkSampleCountFlagBits rasterizationSamples,
			VkPipelineMultisampleStateCreateFlags flags = 0)
		{
			VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
			pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
			pipelineMultisampleStateCreateInfo.flags = flags;
			return pipelineMultisampleStateCreateInfo;
		}

		FORCEINLINE VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
			const VkDynamicState * pDynamicStates,
			uint32_t dynamicStateCount,
			VkPipelineDynamicStateCreateFlags flags = 0)
		{
			VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
			pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;
			pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
			pipelineDynamicStateCreateInfo.flags = flags;
			return pipelineDynamicStateCreateInfo;
		}

		FORCEINLINE VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
			const std::vector<VkDynamicState>& pDynamicStates,
			VkPipelineDynamicStateCreateFlags flags = 0)
		{
			VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
			pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates.data();
			pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(pDynamicStates.size());
			pipelineDynamicStateCreateInfo.flags = flags;
			return pipelineDynamicStateCreateInfo;
		}

		FORCEINLINE VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo(uint32_t patchControlPoints)
		{
			VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo{};
			pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
			pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;
			return pipelineTessellationStateCreateInfo;
		}

		FORCEINLINE VkGraphicsPipelineCreateInfo pipelineCreateInfo(
			VkPipelineLayout layout,
			VkRenderPass renderPass,
			VkPipelineCreateFlags flags = 0)
		{
			VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineCreateInfo.layout = layout;
			pipelineCreateInfo.renderPass = renderPass;
			pipelineCreateInfo.flags = flags;
			pipelineCreateInfo.basePipelineIndex = -1;
			pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			return pipelineCreateInfo;
		}

		FORCEINLINE VkGraphicsPipelineCreateInfo pipelineCreateInfo()
		{
			VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineCreateInfo.basePipelineIndex = -1;
			pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			return pipelineCreateInfo;
		}

		FORCEINLINE VkComputePipelineCreateInfo computePipelineCreateInfo(
			VkPipelineLayout layout,
			VkPipelineCreateFlags flags = 0)
		{
			VkComputePipelineCreateInfo computePipelineCreateInfo{};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCreateInfo.layout = layout;
			computePipelineCreateInfo.flags = flags;
			return computePipelineCreateInfo;
		}

		FORCEINLINE VkPushConstantRange pushConstantRange(
			VkShaderStageFlags stageFlags,
			uint32_t size,
			uint32_t offset)
		{
			VkPushConstantRange pushConstantRange{};
			pushConstantRange.stageFlags = stageFlags;
			pushConstantRange.offset = offset;
			pushConstantRange.size = size;
			return pushConstantRange;
		}

		FORCEINLINE VkBindSparseInfo bindSparseInfo()
		{
			VkBindSparseInfo bindSparseInfo{};
			bindSparseInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
			return bindSparseInfo;
		}

		/** @brief Initialize a map entry for a shader specialization constant */
		FORCEINLINE VkSpecializationMapEntry specializationMapEntry(uint32_t constantID, uint32_t offset, size_t size)
		{
			VkSpecializationMapEntry specializationMapEntry{};
			specializationMapEntry.constantID = constantID;
			specializationMapEntry.offset = offset;
			specializationMapEntry.size = size;
			return specializationMapEntry;
		}

		/** @brief Initialize a specialization constant info structure to pass to a shader stage */
		FORCEINLINE VkSpecializationInfo specializationInfo(uint32_t mapEntryCount, const VkSpecializationMapEntry* mapEntries, size_t dataSize, const void* data)
		{
			VkSpecializationInfo specializationInfo{};
			specializationInfo.mapEntryCount = mapEntryCount;
			specializationInfo.pMapEntries = mapEntries;
			specializationInfo.dataSize = dataSize;
			specializationInfo.pData = data;
			return specializationInfo;
		}
	};


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
		static VkShaderStageFlagBits To(Shader::Type type);
	};


	class VulkanRasterizerState : public TRefcountResource< RHIRasterizerState >
	{
	public:
		VulkanRasterizerState(RasterizerStateInitializer const& initializer)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			createInfo.polygonMode = VulkanTranslate::To(initializer.fillMode);
			createInfo.cullMode = VulkanTranslate::To(initializer.cullMode);
			createInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
			createInfo.rasterizerDiscardEnable = VK_FALSE;
			createInfo.depthBiasEnable = VK_FALSE;
			createInfo.depthClampEnable = VK_FALSE;
			createInfo.lineWidth = 1.0f;

			multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleState.rasterizationSamples;
			multisampleState.sampleShadingEnable;
			multisampleState.minSampleShading;
			multisampleState.pSampleMask;
			multisampleState.alphaToCoverageEnable;
			multisampleState.alphaToOneEnable;
		}

		VkPipelineMultisampleStateCreateInfo   multisampleState = {};
		VkPipelineRasterizationStateCreateInfo createInfo = {};
	};

	class VulkanDepthStencilState : public TRefcountResource< RHIDepthStencilState >
	{
	public:
		VulkanDepthStencilState(DepthStencilStateInitializer const& initializer)
		{

			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			createInfo.flags = 0;
			createInfo.depthTestEnable = (initializer.depthFunc != ECompareFunc::Always) || (initializer.bWriteDepth);
			createInfo.depthWriteEnable = initializer.bWriteDepth;
			createInfo.depthCompareOp = VulkanTranslate::To(initializer.depthFunc);
			createInfo.depthBoundsTestEnable = VK_FALSE;
			createInfo.stencilTestEnable = initializer.bEnableStencilTest;


			createInfo.front.failOp = VulkanTranslate::To(initializer.stencilFailOp);
			createInfo.front.passOp = VulkanTranslate::To(initializer.zPassOp);
			createInfo.front.depthFailOp = VulkanTranslate::To(initializer.zFailOp); 
			createInfo.front.compareOp = VulkanTranslate::To(initializer.stencilFunc);
			createInfo.front.compareMask = initializer.stencilReadMask;
			createInfo.front.writeMask = initializer.stencilWriteMask;
			createInfo.front.reference = 0xff;

			createInfo.back.failOp = VulkanTranslate::To(initializer.stencilFailOpBack);
			createInfo.back.passOp = VulkanTranslate::To(initializer.zPassOpBack);
			createInfo.back.depthFailOp = VulkanTranslate::To(initializer.zFailOpBack);
			createInfo.back.compareOp = VulkanTranslate::To(initializer.stencilFunBack);
			createInfo.back.compareMask = initializer.stencilReadMask;
			createInfo.back.writeMask = initializer.stencilWriteMask;
			createInfo.back.reference = 0xff;

			createInfo.minDepthBounds = 0.0f;
			createInfo.maxDepthBounds = 1.0f;

		}

		VkPipelineDepthStencilStateCreateInfo createInfo = {};
	};

	class VulkanBlendState : public TRefcountResource< RHIBlendState >
	{
	public:
		VulkanBlendState(BlendStateInitializer const& initializer)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			createInfo.pAttachments = colorAttachmentStates;
			createInfo.attachmentCount = initializer.bEnableIndependent ? MaxBlendStateTargetCount : 1;
			createInfo.logicOp = VK_LOGIC_OP_COPY;
			createInfo.logicOpEnable = VK_FALSE;
			createInfo.blendConstants[0] = 0.0f;
			createInfo.blendConstants[1] = 0.0f;
			createInfo.blendConstants[2] = 0.0f;
			createInfo.blendConstants[3] = 0.0f;

			for (int i = 0; i < createInfo.attachmentCount; ++i)
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

		VkPipelineColorBlendAttachmentState colorAttachmentStates[MaxBlendStateTargetCount];
		VkPipelineColorBlendStateCreateInfo createInfo = {};
	};


	template<>
	struct TVulkanResourceTraits<EVulkanResourceType::eVkSampler>
	{
		typedef VkSampler HandleType;
		static bool Create(VkDevice device, VkSampler& handle, VkSamplerCreateInfo const& info)
		{
			VK_VERIFY_RETURN_FALSE(vkCreateSampler(device, &info, gAllocCB, &handle));
			return true;
		}
		static void Destroy(VkDevice device, VkSampler& handle)
		{
			vkDestroySampler(device, handle, gAllocCB);
		}
	};


	class VulkanSamplerState : public TRefcountResource<RHISamplerState >
	{
	public:
		VkSampler getHandle() { return sampler; }

		VK_RESOURCE_TYPE(VkSampler) sampler;
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

			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			createInfo.pVertexAttributeDescriptions = vertexInputAttrDescs.data();
			createInfo.vertexAttributeDescriptionCount = vertexInputAttrDescs.size();
			createInfo.pVertexBindingDescriptions = vertexInputBindingDescs.data();
			createInfo.vertexBindingDescriptionCount = vertexInputBindingDescs.size();

		}

		std::vector< VkVertexInputAttributeDescription > vertexInputAttrDescs;
		std::vector< VkVertexInputBindingDescription> vertexInputBindingDescs;
		VkPipelineVertexInputStateCreateInfo createInfo = {};
	};


	template<>
	struct TVulkanResourceTraits<EVulkanResourceType::eVkImage>
	{
		typedef VkImage HandleType;
		static bool Create(VkDevice device, VkImage& handle, VkImageCreateInfo const& info)
		{
			VK_VERIFY_RETURN_FALSE(vkCreateImage(device, &info, gAllocCB, &handle));
			return true;
		}
		static void Destroy(VkDevice device, VkImage& handle)
		{
			vkDestroyImage(device, handle, gAllocCB);
		}
	};

	template<>
	struct TVulkanResourceTraits<EVulkanResourceType::eVkImageView>
	{
		typedef VkImageView HandleType;
		static bool Create(VkDevice device, VkImageView& handle, VkImageViewCreateInfo const& info)
		{
			VK_VERIFY_RETURN_FALSE(vkCreateImageView(device, &info, gAllocCB, &handle));
			return true;
		}
		static void Destroy(VkDevice device, VkImageView& handle)
		{
			vkDestroyImageView(device, handle, gAllocCB);
		}
	};


	template<>
	struct TVulkanResourceTraits<EVulkanResourceType::eVkDeviceMemory>
	{
		typedef VkDeviceMemory HandleType;
		static bool Create(VkDevice device, VkDeviceMemory& handle, VkMemoryAllocateInfo const& info)
		{
			VK_VERIFY_RETURN_FALSE(vkAllocateMemory(device, &info, gAllocCB, &handle));
			return true;
		}
		static void Destroy(VkDevice device, VkDeviceMemory& handle)
		{
			vkFreeMemory(device, handle, gAllocCB);
		}
	};

	struct FVulkanTexture
	{
		static VkImageUsageFlags TranslateUsage(uint32 createFlags);

		static void SetImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkImageSubresourceRange const& subresourceRange,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		// Fixed sub resource on first mip level and layer
		static void SetImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageAspectFlags aspectMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
		{
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = aspectMask;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;
			SetImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
		}
	};
	template< class RHITextureType >
	class TVulkanTexture : public TRefcountResource< RHITextureType >
	{
	public:
		VkBuffer getHandle() { return image; }
		void releaseResource()
		{
			image.destroy(mDevice);
			view.destroy(mDevice);
			deviceMemory.destroy(mDevice);
		}

		VK_RESOURCE_TYPE( VkImage )        image;
		VK_RESOURCE_TYPE( VkImageView )    view;
		VK_RESOURCE_TYPE( VkDeviceMemory ) deviceMemory;
		VkDevice       mDevice;
		VkImageLayout  mImageLayout;
	};

	class VulkanTexture2D : public TVulkanTexture< RHITexture2D >
	{
	public:

		virtual bool update(int ox, int oy, int w, int h, Texture::Format format, void* data, int level = 0)
		{
			assert(0);
			return false;
		}
		virtual bool update(int ox, int oy, int w, int h, Texture::Format format, int dataImageWidth, void* data, int level = 0)
		{
			assert(0);
			return false;
		}


	};


	template<>
	struct TVulkanResourceTraits<EVulkanResourceType::eVkBuffer>
	{
		typedef VkBuffer HandleType;
		static bool Create(VkDevice device, VkBuffer& handle, VkBufferCreateInfo const& info)
		{
			VK_VERIFY_RETURN_FALSE(vkCreateBuffer(device, &info, gAllocCB, &handle));
			return true;
		}
		static void Destroy(VkDevice device, VkBuffer& handle)
		{
			vkDestroyBuffer(device, handle, gAllocCB);
		}
	};
	template<>
	struct TVulkanResourceTraits<EVulkanResourceType::eVkBufferView>
	{
		typedef VkBufferView HandleType;
		static bool Create(VkDevice device, VkBufferView& handle, VkBufferViewCreateInfo const& info)
		{
			VK_VERIFY_RETURN_FALSE(vkCreateBufferView(device, &info, gAllocCB, &handle));
			return true;
		}
		static void Destroy(VkDevice device, VkBufferView& handle)
		{
			vkDestroyBufferView(device, handle, gAllocCB);
		}
	};

	struct FVulkanBuffer
	{
		static VkBufferUsageFlags TranslateUsage(uint32 createionFlags )
		{
			VkBufferUsageFlags result = 0;
			return result;
		}
	};

	struct VulkanBufferData
	{
		VkDevice device;
		VK_RESOURCE_TYPE( VkBuffer ) buffer;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkDescriptorBufferInfo descriptor;
		VkDeviceSize size = 0;
		VkDeviceSize alignment = 0;
		void* mapped = nullptr;

		/** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
		VkBufferUsageFlags usageFlags;
		/** @brief Memory propertys flags to be filled by external source at buffer creation (to query at some later point) */
		VkMemoryPropertyFlags memoryPropertyFlags;

		/**
		* Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
		*
		* @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
		* @param offset (Optional) Byte offset from beginning
		*
		* @return VkResult of the buffer mapping call
		*/
		VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
		{
			return vkMapMemory(device, memory, offset, size, 0, &mapped);
		}

		/**
		* Unmap a mapped memory range
		*
		* @note Does not return a result as vkUnmapMemory can't fail
		*/
		void unmap()
		{
			if (mapped)
			{
				vkUnmapMemory(device, memory);
				mapped = nullptr;
			}
		}

		/**
		* Attach the allocated memory block to the buffer
		*
		* @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
		*
		* @return VkResult of the bindBufferMemory call
		*/
		VkResult bind(VkDeviceSize offset = 0)
		{
			return vkBindBufferMemory(device, buffer, memory, offset);
		}

		/**
		* Setup the default descriptor for this buffer
		*
		* @param size (Optional) Size of the memory range of the descriptor
		* @param offset (Optional) Byte offset from beginning
		*
		*/
		void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
		{
			descriptor.offset = offset;
			descriptor.buffer = buffer;
			descriptor.range = size;
		}

		/**
		* Copies the specified data to the mapped buffer
		*
		* @param data Pointer to the data to copy
		* @param size Size of the data to copy in machine units
		*
		*/
		void copyTo(void* data, VkDeviceSize size)
		{
			assert(mapped);
			memcpy(mapped, data, size);
		}

		/**
		* Flush a memory range of the buffer to make it visible to the device
		*
		* @note Only required for non-coherent memory
		*
		* @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
		* @param offset (Optional) Byte offset from beginning
		*
		* @return VkResult of the flush call
		*/
		VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
		{
			VkMappedMemoryRange mappedRange = {};
			mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedRange.memory = memory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
		}

		/**
		* Invalidate a memory range of the buffer to make it visible to the host
		*
		* @note Only required for non-coherent memory
		*
		* @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
		* @param offset (Optional) Byte offset from beginning
		*
		* @return VkResult of the invalidate call
		*/
		VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
		{
			VkMappedMemoryRange mappedRange = {};
			mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedRange.memory = memory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
		}

		/**
		* Release all Vulkan resources held by this buffer
		*/
		void destroy()
		{
			if (buffer)
			{
				vkDestroyBuffer(device, buffer, nullptr);
			}
			if (memory)
			{
				vkFreeMemory(device, memory, nullptr);
			}
		}

	};

	template< class RHIBufferType >
	class TVulkanBuffer : public TRefcountResource< RHIBufferType >
		                , protected VulkanBufferData
	{
	public:

		VkBuffer getHandle() { return buffer; }
		void releaseResource()
		{
			VulkanBufferData::destroy();
		}

		VulkanBufferData& getBufferData() { return *this; }
		friend class VulkanSystem;
	};

	class VulkanVertexBuffer : public TVulkanBuffer< RHIVertexBuffer >
	{
	public:
		virtual void  resetData(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
		{
			assert(0);
		}
		virtual void  updateData(uint32 vStart, uint32 numVertices, void* data)
		{
			assert(0);
		}


	};

	class VulkanIndexBuffer : public TVulkanBuffer< RHIIndexBuffer >
	{
	public:
	};

	template< class TRHIResource >
	struct TVulkanCastTraits {};

	class VulkanShader;
	class VulkanShaderProgram;

	//template<> struct TVulkanCastTraits< RHITexture1D > { typedef VulkanTexture1D CastType; };
	template<> struct TVulkanCastTraits< RHITexture2D > { typedef VulkanTexture2D CastType; };
	//template<> struct TVulkanCastTraits< RHITexture3D > { typedef VulkanTexture3D CastType; };
	//template<> struct TVulkanCastTraits< RHITextureCube > { typedef VulkanTextureCube CastType; };
	//template<> struct TVulkanCastTraits< RHITexture2DArray > { typedef VulkanTexture2DArray CastType; };
	//template<> struct TVulkanCastTraits< RHITextureDepth > { typedef VulkanTextureDepth CastType; };
	template<> struct TVulkanCastTraits< RHIVertexBuffer > { typedef VulkanVertexBuffer CastType; };
	template<> struct TVulkanCastTraits< RHIIndexBuffer > { typedef VulkanIndexBuffer CastType; };
	template<> struct TVulkanCastTraits< RHIInputLayout > { typedef VulkanInputLayout CastType; };
	template<> struct TVulkanCastTraits< RHISamplerState > { typedef VulkanSamplerState CastType; };
	template<> struct TVulkanCastTraits< RHIBlendState > { typedef VulkanBlendState CastType; };
	template<> struct TVulkanCastTraits< RHIRasterizerState > { typedef VulkanRasterizerState CastType; };
	template<> struct TVulkanCastTraits< RHIDepthStencilState > { typedef VulkanDepthStencilState CastType; };
	//template<> struct TVulkanCastTraits< RHIFrameBuffer > { typedef VulkanFrameBuffer CastType; };
	template<> struct TVulkanCastTraits< RHIShader > { typedef VulkanShader CastType; };
	template<> struct TVulkanCastTraits< RHIShaderProgram > { typedef VulkanShaderProgram CastType; };

	struct VulkanCast
	{
		template< class TRHIResource >
		static auto To(TRHIResource* resource)
		{
			return static_cast<TVulkanCastTraits< TRHIResource >::CastType*>(resource);
		}
		template< class TRHIResource >
		static auto To(TRHIResource const* resource)
		{
			return static_cast<TVulkanCastTraits< TRHIResource >::CastType const*>(resource);
		}
		template< class TRHIResource >
		static auto& To(TRHIResource& resource)
		{
			return static_cast<TVulkanCastTraits< TRHIResource >::CastType&>(resource);
		}
		template< class TRHIResource >
		static auto& To(TRHIResource const& resource)
		{
			return static_cast<TVulkanCastTraits< TRHIResource >::CastType const&>(resource);
		}

		template < class TRHIResource >
		static auto To(TRefCountPtr<TRHIResource>& ptr) { return To(ptr.get()); }

		template < class TRHIResource >
		static auto GetHandle(TRHIResource& RHIObject)
		{
			return VulkanCast::To(RHIObject).getHandle();
		}

		template < class TRHIResource >
		static auto GetHandle(TRHIResource const& RHIObject)
		{
			return VulkanCast::To(RHIObject).getHandle();
		}
		template < class TRHIResource >
		static auto GetHandle(TRefCountPtr<TRHIResource>& refPtr)
		{
			return VulkanCast::To(refPtr)->getHandle();
		}
	};
}


#endif // VulkanCommon_H_421FF10B_85F3_4B99_90F9_474EDE8043DF
