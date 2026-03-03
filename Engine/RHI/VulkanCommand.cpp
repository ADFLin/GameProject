#include "VulkanCommand.h"
#include "VulkanShaderCommon.h"
#include "GpuProfiler.h"
#include "RHIGlobalResource.h"
#include "RHIMisc.h"

#include "MacroCommon.h"
#include "CoreShare.h"

#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

#include <cstdint>
#include <limits>
#include "BitUtility.h"

namespace Render
{
	EXPORT_RHI_SYSTEM_MODULE(RHISystemName::Vulkan, VulkanSystem);

	VkAllocationCallbacks* gAllocCB = nullptr;

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugVKCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		LogWarning(0, pCallbackData->pMessage);
		return VK_FALSE;
	}


	VulkanDevice::VulkanDevice()
	{

	}

	VulkanDevice::~VulkanDevice()
	{
		if (logicalDevice)
		{
			if (mDefaultSampler != VK_NULL_HANDLE)
			{
				vkDestroySampler(logicalDevice, mDefaultSampler, gAllocCB);
			}
			vkDestroyDevice(logicalDevice, gAllocCB);
		}
	}

	bool VulkanDevice::initialize(VkPhysicalDevice physicalDevice)
	{
		assert(physicalDevice);
		this->physicalDevice = physicalDevice;

		// Store Properties features, limits and properties of the physical device for later use
		// Device properties also contain limits and sparse properties
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		// Features should be checked by the examples before using them
		vkGetPhysicalDeviceFeatures(physicalDevice, &supportFeatures);
		// Memory properties are used regularly for creating all kinds of buffers
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		// Queue family properties, used for setting up requested queues upon device creation
		queueFamilyProperties = GetEnumValues(vkGetPhysicalDeviceQueueFamilyProperties, physicalDevice);

		// Get list of supported extensions
		uint32 extCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
		if (extCount > 0)
		{
			TArray<VkExtensionProperties> extensions(extCount);
			if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
			{
				for (auto ext : extensions)
				{
					supportedExtensions.push_back(ext.extensionName);
				}
			}
		}

		return true;
	}

	int32 VulkanDevice::getMemoryTypeIndex(uint32 typeBits, VkMemoryPropertyFlags properties)
	{
		for (int32 index = 0; index < memoryProperties.memoryTypeCount; ++index)
		{
			if ((typeBits & 1) == 1)
			{
				if ((memoryProperties.memoryTypes[index].propertyFlags & properties) == properties)
				{
					return index;
				}
			}
			typeBits >>= 1;
		}
		return INDEX_NONE;
	}

	bool VulkanDevice::getQueueFamilyIndex(VkQueueFlagBits queueFlags, uint32& outIndex )
	{
		// Dedicated queue for compute
		// Try to find a queue family index that supports compute but not graphics
		if (queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			for (uint32 i = 0; i < static_cast<uint32>(queueFamilyProperties.size()); i++)
			{
				if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					outIndex = i;
					return true;
				}
			}
		}

		// Dedicated queue for transfer
		// Try to find a queue family index that supports transfer but not graphics and compute
		if (queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			for (uint32 i = 0; i < static_cast<uint32>(queueFamilyProperties.size()); i++)
			{
				if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					outIndex = i;
					return true;
				}
			}
		}

		// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
		for (uint32 i = 0; i < static_cast<uint32>(queueFamilyProperties.size()); i++)
		{
			if (queueFamilyProperties[i].queueFlags & queueFlags)
			{
				outIndex = i;
				return true;
			}
		}

		return false;
	}

	bool VulkanDevice::createLogicalDevice(VkPhysicalDeviceFeatures inEnabledFeatures, TArray<const char*> enabledExtensions, void* pNextChain, bool useSwapChain /*= true*/, TArrayView< uint32 const > const& usageQueueIndices )
	{
		// enableFeatures = inEnabledFeatures;
		// Desired queues need to be requested upon logical device creation
		// Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
		// requests different queue types
		const float defaultQueuePriority(0.0f);
		TArray<VkDeviceQueueCreateInfo> queueCreateInfos{};

		// Get queue family indices for the requested queue family types
		// Note that the indices may overlap depending on the implementation

		for (auto index : usageQueueIndices)
		{
			VkDeviceQueueCreateInfo queueInfo = {};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = index;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}


		// Create the logical device representation
		TArray<const char*> deviceExtensions(enabledExtensions);
		if (useSwapChain)
		{
			// If the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size());;
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &inEnabledFeatures;

		// If a pNext(Chain) has been passed, we need to add it to the device creation info
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
		if (pNextChain) 
		{
			physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			physicalDeviceFeatures2.features = inEnabledFeatures;
			physicalDeviceFeatures2.pNext = pNextChain;
			deviceCreateInfo.pEnabledFeatures = nullptr;
			deviceCreateInfo.pNext = &physicalDeviceFeatures2;
		}

		// Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
		if (extensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
		{
			deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
			enableDebugMarkers = true;
		}

		if (deviceExtensions.size() > 0)
		{
			deviceCreateInfo.enabledExtensionCount = (uint32)deviceExtensions.size();
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}

		VK_VERIFY_RETURN_FALSE( vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice) );

		{
			VkSamplerCreateInfo samplerInfo = FVulkanInit::samplerCreateInfo();
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.maxLod = 100.0f;
			VK_VERIFY_RETURN_FALSE(vkCreateSampler(logicalDevice, &samplerInfo, gAllocCB, &mDefaultSampler));
		}

		this->enableFeatures = enableFeatures;

		return true;
	}

	bool VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data /*= nullptr*/)
	{
		// Create the buffer handle
		VkBufferCreateInfo bufferCreateInfo = FVulkanInit::bufferCreateInfo(usageFlags, size);
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, buffer));

		// Create the memory backing up the buffer handle
		VkMemoryRequirements memReqs;
		VkMemoryAllocateInfo memAlloc = FVulkanInit::memoryAllocateInfo();
		vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		// Find a memory type index that fits the properties of the buffer
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, memoryPropertyFlags);
		if (memAlloc.memoryTypeIndex == INDEX_NONE)
			return false;

		VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory));

		// If a pointer to the buffer data has been passed, map the buffer and copy over the data
		if (data != nullptr)
		{
			void *mapped;
			VK_CHECK_RESULT(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped));
			memcpy(mapped, data, size);
			// If host coherency hasn't been requested, do a manual flush to make writes visible
			if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				VkMappedMemoryRange mappedRange = FVulkanInit::mappedMemoryRange();
				mappedRange.memory = *memory;
				mappedRange.offset = 0;
				mappedRange.size = size;
				vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange);
			}
			vkUnmapMemory(logicalDevice, *memory);
		}

		// Attach the memory to the buffer object
		VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));

		return true;
	}

	bool VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VulkanBufferData& bufferData, VkDeviceSize size, void *data /*= nullptr*/)
	{
		bufferData.device = logicalDevice;

		// Create the buffer handle
		VkBufferCreateInfo bufferCreateInfo = FVulkanInit::bufferCreateInfo(usageFlags, size);
		VERIFY_RETURN_FALSE(bufferData.buffer.initialize(logicalDevice, bufferCreateInfo));

		// Create the memory backing up the buffer handle
		VkMemoryRequirements memReqs;
		VkMemoryAllocateInfo memAlloc = FVulkanInit::memoryAllocateInfo();
		vkGetBufferMemoryRequirements(logicalDevice, bufferData.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		// Find a memory type index that fits the properties of the buffer
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, memoryPropertyFlags);
		if (memAlloc.memoryTypeIndex == INDEX_NONE)
			return false;
		VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &bufferData.memory));

		bufferData.alignment = memReqs.alignment;
		bufferData.size = size;
		bufferData.usageFlags = usageFlags;
		bufferData.memoryPropertyFlags = memoryPropertyFlags;

		// If a pointer to the buffer data has been passed, map the buffer and copy over the data
		if (data != nullptr)
		{
			VK_CHECK_RESULT(bufferData.map());
			memcpy(bufferData.mapped, data, size);
			if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
				bufferData.flush();

			bufferData.unmap();
		}

		// Initialize a default descriptor that covers the whole buffer size
		bufferData.setupDescriptor();

		// Attach the memory to the buffer object
		VK_VERIFY_RETURN_FALSE( bufferData.bind() );
		return true;
	}

	void VulkanDevice::copyBuffer(VkCommandPool pool, VulkanBufferData *src, VulkanBufferData *dst, VkQueue queue, VkBufferCopy *copyRegion /*= nullptr*/)
	{
		assert(dst->size <= src->size);
		assert(src->buffer);
		VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, pool, true);
		VkBufferCopy bufferCopy{};
		if (copyRegion == nullptr)
		{
			bufferCopy.size = src->size;
		}
		else
		{
			bufferCopy = *copyRegion;
		}

		vkCmdCopyBuffer(copyCmd, src->buffer, dst->buffer, 1, &bufferCopy);

		flushCommandBuffer(copyCmd, queue, pool);
	}

	VkCommandPool VulkanDevice::createCommandPool(uint32 queueFamilyIndex, VkCommandPoolCreateFlags createFlags /*= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT*/)
	{
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
		cmdPoolInfo.flags = createFlags;
		VkCommandPool cmdPool;
		VK_CHECK_RESULT(vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool));
		return cmdPool;
	}

	VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin /*= false*/)
	{
		VkCommandBufferAllocateInfo cmdBufAllocateInfo = FVulkanInit::commandBufferAllocateInfo(pool, level, 1);
		VkCommandBuffer cmdBuffer;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &cmdBuffer));
		// If requested, also start recording for the new command buffer
		if (begin)
		{
			VkCommandBufferBeginInfo cmdBufInfo = FVulkanInit::commandBufferBeginInfo();
			VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
		}
		return cmdBuffer;
	}

	void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free /*= true*/)
	{
		if (commandBuffer == VK_NULL_HANDLE)
		{
			return;
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo = FVulkanInit::submitInfo();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		// Create fence to ensure that the command buffer has finished executing
		VkFenceCreateInfo fenceInfo = FVulkanInit::fenceCreateInfo(VK_FLAGS_NONE);
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));
		// Submit to the queue
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
		// Wait for the fence to signal that command buffer has finished executing
		VK_CHECK_RESULT(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, VK_DEFAULT_FENCE_TIMEOUT));
		vkDestroyFence(logicalDevice, fence, nullptr);
		if (free)
		{
			vkFreeCommandBuffers(logicalDevice, pool, 1, &commandBuffer);
		}
	}

	bool VulkanDevice::getSupportedDepthFormat(VkFormat& outFormat, bool bCheckSamplingSupport)
	{
		// Since all depth formats may be optional, we need to find a suitable depth format to use
		// Start with the highest precision packed format
		TArray<VkFormat> depthFormats =
		{
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};

		for (auto& format : depthFormats)
		{
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
			// Format must support depth stencil attachment for optimal tiling
			if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				if (bCheckSamplingSupport)
				{
					if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) 
					{
						continue;
					}
				}
				outFormat = format;
				return true;
			}
		}

		return false;
	}

	bool VulkanSystem::initialize(RHISystemInitParams const& initParam)
	{
		TArray<VkExtensionProperties> availableExtensions = GetEnumValues(vkEnumerateInstanceExtensionProperties, nullptr);


		bool const bEnableValidation = true;
		if (!createInstance(availableExtensions, bEnableValidation))
		{
			LogError("Could not create Vulkan instance");
			return false;
		}

#if SYS_PLATFORM_WIN
		if (!FPropertyName::Find(availableExtensions, VK_KHR_WIN32_SURFACE_EXTENSION_NAME))
			return false;
		mWindowSurface = createWindowSurface(initParam.hWnd);
#else
		return false;
#endif
		mNumSamples = initParam.numSamples;

		TArray< VkPhysicalDevice > physicalDevices = GetEnumValues(vkEnumeratePhysicalDevices, mInstance);

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		for (auto& device : physicalDevices)
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE && !physicalDevices.empty())
		{
			physicalDevice = physicalDevices[0];
		}

		mDevice = new VulkanDevice();
		if (!mDevice->initialize(physicalDevice))
			return false;

		VERIFY_RETURN_FALSE(mDevice->getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, mUsageQueueFamilyIndices[EQueueFamily::Graphics]));
		VERIFY_RETURN_FALSE(mDevice->getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, mUsageQueueFamilyIndices[EQueueFamily::Compute]));
		mDevice->getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, mUsageQueueFamilyIndices[EQueueFamily::Transfer]);
		uint32 indexCurrent = 0;
		for (auto& queueFamilyProps : mDevice->queueFamilyProperties)
		{
			if (queueFamilyProps.queueCount > 0)
			{
				VkBool32 presentSupport = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, indexCurrent, mWindowSurface, &presentSupport);
				if (presentSupport)
				{
					mUsageQueueFamilyIndices[EQueueFamily::Present] = indexCurrent;
					break;
				}
			}
			++indexCurrent;
		}
		TArray< uint32 > uniqueQueueFamilies;
	
		uniqueQueueFamilies.push_back(mUsageQueueFamilyIndices[EQueueFamily::Graphics]);
		uniqueQueueFamilies.push_back(mUsageQueueFamilyIndices[EQueueFamily::Compute]);
		uniqueQueueFamilies.push_back(mUsageQueueFamilyIndices[EQueueFamily::Present]);
		if (mUsageQueueFamilyIndices[EQueueFamily::Transfer] != INDEX_NONE)
		{
			uniqueQueueFamilies.push_back(mUsageQueueFamilyIndices[EQueueFamily::Graphics]);
		}
		uniqueQueueFamilies.makeUnique();

		//#TODO
		VkPhysicalDeviceFeatures enabledDeviceFeatures = mDevice->supportFeatures;

		void* deviceCreatepNextChain = nullptr;
		VERIFY_RETURN_FALSE( mDevice->createLogicalDevice(enabledDeviceFeatures, enabledDeviceExtensions, deviceCreatepNextChain, true , MakeConstView(uniqueQueueFamilies) ) );

		VERIFY_RETURN_FALSE( mDevice->getSupportedDepthFormat(depthFormat, false) );

		vkGetDeviceQueue(mDevice->logicalDevice, mUsageQueueFamilyIndices[EQueueFamily::Graphics], 0, &mGraphicsQueue);
		VERIFY_RETURN_FALSE(mGraphicsQueue);
		vkGetDeviceQueue(mDevice->logicalDevice, mUsageQueueFamilyIndices[EQueueFamily::Present], 0, &mPresentQueue);
		VERIFY_RETURN_FALSE(mPresentQueue);

		mGraphicsCommandPool = mDevice->createCommandPool(mUsageQueueFamilyIndices[EQueueFamily::Graphics]);

		mSwapChain = new VulkanSwapChain;
		if (!mSwapChain->initialize(*mDevice, mWindowSurface, mUsageQueueFamilyIndices, depthFormat, mNumSamples, false))
		{
			return false;
		}

		VERIFY_RETURN_FALSE(initRenderResource());

		VERIFY_RETURN_FALSE(mDrawContext.initialize(this));
		mImmediateCommandList = new RHICommandListImpl(mDrawContext);

		VulkanDynamicBufferManager::Get().initialize(mDevice);

		// Create swap chain render pass and framebuffers
		VERIFY_RETURN_FALSE(createSwapChainRenderPass());
		VERIFY_RETURN_FALSE(createSwapChainFramebuffers());

		return true;
	}

	bool VulkanSystem::initializeSamplerStateInternal(VulkanSamplerState* samplerState, SamplerStateInitializer const& initializer)
	{
		VkSamplerCreateInfo samplerInfo = FVulkanInit::samplerCreateInfo();
		samplerInfo.magFilter = VulkanTranslate::To(initializer.filter);
		samplerInfo.minFilter = VulkanTranslate::To(initializer.filter);
		samplerInfo.mipmapMode = VulkanTranslate::ToMipmapMode(initializer.filter);
		samplerInfo.addressModeU = VulkanTranslate::To(initializer.addressU);
		samplerInfo.addressModeV = VulkanTranslate::To(initializer.addressV);
		samplerInfo.addressModeW = VulkanTranslate::To(initializer.addressW);

		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		// Set max level-of-detail to mip level count of the texture
		//samplerInfo.maxLod = (useStaging) ? (float)texture.mipLevels : 0.0f;
		samplerInfo.maxLod = 100;
		// Enable anisotropic filtering
		// This feature is optional, so we must check if it's supported on the device
		if (mDevice->enableFeatures.samplerAnisotropy)
		{
			// Use max. level of anisotropy for this example
			samplerInfo.maxAnisotropy = mDevice->properties.limits.maxSamplerAnisotropy;
			samplerInfo.anisotropyEnable = VK_TRUE;
		}
		else
		{
			// The device does not support anisotropic filtering
			samplerInfo.maxAnisotropy = 1.0;
			samplerInfo.anisotropyEnable = VK_FALSE;
		}
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VERIFY_RETURN_FALSE(samplerState->sampler.initialize(mDevice->logicalDevice, samplerInfo));

		return true;
	}

	RHIRasterizerState* VulkanSystem::RHICreateRasterizerState(RasterizerStateInitializer const& initializer)
	{
		return new VulkanRasterizerState(initializer);
	}

	RHIBlendState* VulkanSystem::RHICreateBlendState(BlendStateInitializer const& initializer)
	{
		return new VulkanBlendState(initializer);
	}

	RHIDepthStencilState* VulkanSystem::RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer)
	{
		return new VulkanDepthStencilState(initializer);
	}

	RHIShader* VulkanSystem::RHICreateShader(EShader::Type type)
	{

		return new VulkanShader;
	}

	RHIShaderProgram* VulkanSystem::RHICreateShaderProgram()
	{
		return new VulkanShaderProgram;
	}

	// =============================== VulkanDescriptorPoolManager ===============================

	bool VulkanDescriptorPoolManager::initialize(VkDevice device)
	{
		mDevice = device;
		return true;
	}

	void VulkanDescriptorPoolManager::release()
	{
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			for (auto pool : mFrames[i].activePools)
			{
				vkDestroyDescriptorPool(mDevice, pool, gAllocCB);
			}
			for (auto pool : mFrames[i].freePools)
			{
				vkDestroyDescriptorPool(mDevice, pool, gAllocCB);
			}
			mFrames[i].activePools.clear();
			mFrames[i].freePools.clear();
			mFrames[i].currentPool = VK_NULL_HANDLE;
		}
	}

	VkDescriptorPool VulkanDescriptorPoolManager::createPool()
	{
		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 256 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 128 },
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 128 },
		};

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 256;
		poolInfo.poolSizeCount = ARRAY_SIZE(poolSizes);
		poolInfo.pPoolSizes = poolSizes;

		VkDescriptorPool pool;
		VK_VERIFY_FAILCODE(vkCreateDescriptorPool(mDevice, &poolInfo, gAllocCB, &pool), return VK_NULL_HANDLE;);
		return pool;
	}

	void VulkanDescriptorPoolManager::resetFrame(int frameIndex)
	{
		mCurrentFrameIndex = frameIndex % MAX_FRAMES_IN_FLIGHT;
		auto& frame = mFrames[mCurrentFrameIndex];
		
		for (auto pool : frame.activePools)
		{
			vkResetDescriptorPool(mDevice, pool, 0);
			frame.freePools.push_back(pool);
		}
		frame.activePools.clear();
		frame.currentPool = VK_NULL_HANDLE;
	}

	VkDescriptorSet VulkanDescriptorPoolManager::allocateDescriptorSet(VkDescriptorSetLayout layout)
	{
		auto& frame = mFrames[mCurrentFrameIndex];
		if (frame.currentPool == VK_NULL_HANDLE)
		{
			if (!frame.freePools.empty())
			{
				frame.currentPool = frame.freePools.back();
				frame.freePools.pop_back();
			}
			else
			{
				frame.currentPool = createPool();
			}
			frame.activePools.push_back(frame.currentPool);
		}

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = frame.currentPool;
		allocInfo.pSetLayouts = &layout;
		allocInfo.descriptorSetCount = 1;

		VkDescriptorSet descriptorSet;
		VkResult result = vkAllocateDescriptorSets(mDevice, &allocInfo, &descriptorSet);
		if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
		{
			// Current pool is full, try to get a new one
			if (!frame.freePools.empty())
			{
				frame.currentPool = frame.freePools.back();
				frame.freePools.pop_back();
			}
			else
			{
				frame.currentPool = createPool();
			}
			frame.activePools.push_back(frame.currentPool);

			allocInfo.descriptorPool = frame.currentPool;
			VK_VERIFY_FAILCODE(vkAllocateDescriptorSets(mDevice, &allocInfo, &descriptorSet), return VK_NULL_HANDLE;);
		}
		else if (result != VK_SUCCESS)
		{
			return VK_NULL_HANDLE;
		}

		return descriptorSet;
	}

	// =============================== VulkanContext ===============================

	bool VulkanContext::initialize(VulkanSystem* system)
	{
		mSystem = system;
		mDevice = system->mDevice;
		mDescriptorPoolManager.initialize(mDevice->logicalDevice);
		mBoundDescriptorSet = VK_NULL_HANDLE;
		mBoundCmdBuffer = VK_NULL_HANDLE;
		mBoundPipelineLayout = VK_NULL_HANDLE;
		return true;
	}

	void VulkanContext::release()
	{
		// Destroy all cached pipelines
		for (auto& pair : mPipelineCache)
		{
			if (pair.second != VK_NULL_HANDLE)
			{
				vkDestroyPipeline(mDevice->logicalDevice, pair.second, gAllocCB);
			}
		}
		mPipelineCache.clear();

		for (auto& pair : mComputePipelineCache)
		{
			if (pair.second != VK_NULL_HANDLE)
			{
				vkDestroyPipeline(mDevice->logicalDevice, pair.second, gAllocCB);
			}
		}
		mComputePipelineCache.clear();

		mPendingShaderProgram.release();
		mPendingComputeShader.release();
		mPendingInputLayout.release();
		mPendingRasterizerState.release();
		mPendingBlendState.release();
		mPendingDepthStencilState.release();
		mPendingIndexBuffer.release();

		for (int i = 0; i < MaxInputStream; ++i)
		{
			mInputStreams[i].buffer.release();
		}

		mDescriptorPoolManager.release();
	}

	void VulkanSystem::RHIUnlockBuffer(RHIBuffer* buffer)
	{
		auto& bufferImpl = VulkanCast::To(*buffer);
		if (bufferImpl.mbIsDynamic)
			return;
		bufferImpl.getBufferData().unmap();
	}

	void VulkanContext::RHIUpdateBuffer(RHIBuffer& buffer, int start, int numElements, void* data)
	{
		auto& bufferImpl = VulkanCast::To(buffer);
		uint32 offset = start * buffer.getDesc().elementSize;
		uint32 size = numElements * buffer.getDesc().elementSize;

		VkResult result = bufferImpl.getBufferData().map(size, offset);
		if (result == VK_SUCCESS)
		{
			bufferImpl.getBufferData().copyTo(data, size);
			bufferImpl.getBufferData().unmap();
		}
	}
	void VulkanContext::RHIUpdateTexture(RHITexture2D& texture, int ox, int oy, int w, int h, void* data, int level, int dataWidth)
	{
		static_cast<VulkanSystem*>(GRHISystem)->RHIUpdateTexture(texture, ox, oy, w, h, data, level, dataWidth);
	}


	void VulkanContext::RHIUpdateTexture(RHITextureCube& texture, ETexture::Face face, int ox, int oy, int w, int h, void* data, int level, int dataWidth)
	{
	}


	void VulkanContext::RHISetRasterizerState(RHIRasterizerState& rasterizerState)
	{
		mPendingRasterizerState = &rasterizerState;
	}

	void VulkanContext::RHISetBlendState(RHIBlendState& blendState)
	{
		mPendingBlendState = &blendState;
	}

	void VulkanContext::RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef)
	{
		mPendingDepthStencilState = &depthStencilState;
		mPendingStencilRef = stencilRef;
	}

	void VulkanContext::RHISetViewport(ViewportInfo const& viewport)
	{
		VkViewport vkViewport;
		vkViewport.x = viewport.x;
		// Use negative height to flip Y axis (Requires Vulkan 1.1 or VK_KHR_maintenance1)
		vkViewport.y = viewport.y + viewport.h;
		vkViewport.width = viewport.w;
		vkViewport.height = -viewport.h;
		vkViewport.minDepth = viewport.zNear;
		vkViewport.maxDepth = viewport.zFar;
		vkCmdSetViewport(mActiveCmdBuffer, 0, 1, &vkViewport);

		VkRect2D vkScissor;
		vkScissor.offset.x = (int32_t)viewport.x;
		vkScissor.offset.y = (int32_t)viewport.y;
		vkScissor.extent.width = (uint32_t)viewport.w;
		vkScissor.extent.height = (uint32_t)viewport.h;
		vkCmdSetScissor(mActiveCmdBuffer, 0, 1, &vkScissor);
	}

	void VulkanContext::RHISetViewports(ViewportInfo const viewports[], int numViewports)
	{
		TArray<VkViewport> vkViewports;
		TArray<VkRect2D> vkScissors;
		vkViewports.resize(numViewports);
		vkScissors.resize(numViewports);
		for (int i = 0; i < numViewports; ++i)
		{
			vkViewports[i].x = viewports[i].x;
			// Use negative height to flip Y axis
			vkViewports[i].y = viewports[i].y + viewports[i].h;
			vkViewports[i].width = viewports[i].w;
			vkViewports[i].height = -viewports[i].h;
			vkViewports[i].minDepth = viewports[i].zNear;
			vkViewports[i].maxDepth = viewports[i].zFar;

			vkScissors[i].offset.x = (int32_t)viewports[i].x;
			vkScissors[i].offset.y = (int32_t)viewports[i].y;
			vkScissors[i].extent.width = (uint32_t)viewports[i].w;
			vkScissors[i].extent.height = (uint32_t)viewports[i].h;
		}
		vkCmdSetViewport(mActiveCmdBuffer, 0, numViewports, vkViewports.data());
		vkCmdSetScissor(mActiveCmdBuffer, 0, numViewports, vkScissors.data());
	}

	void VulkanContext::RHISetScissorRect(int x, int y, int w, int h)
	{
		VkRect2D vkScissor;
		vkScissor.offset.x = x;
		vkScissor.offset.y = y;
		vkScissor.extent.width = w;
		vkScissor.extent.height = h;
		vkCmdSetScissor(mActiveCmdBuffer, 0, 1, &vkScissor);
	}

	void VulkanContext::RHISetShaderProgram(RHIShaderProgram* shaderProgram)
	{
		bUseFixedShaderPipeline = false;
		mPendingShaderProgram = shaderProgram;
		mPendingShaderKey.value = 0;
		if (shaderProgram)
		{
			mPendingShaderKey.initialize(*shaderProgram);
		}
		mDescriptorDirty = true;
	}

	void VulkanContext::RHISetComputeShader(RHIShader* shader)
	{
		bUseFixedShaderPipeline = false;
		mPendingComputeShader = shader;
		mPendingShaderProgram = nullptr;
		mPendingShaderKey.value = 0;
		if (shader)
		{
			mPendingShaderKey.initialize(*shader);
		}
		mDescriptorDirty = true;
	}

	void VulkanContext::RHISetGraphicsShaderBoundState(GraphicsShaderStateDesc const& stateDesc)
	{
		bUseFixedShaderPipeline = false;
		mPendingShaderProgram = nullptr;
		mPendingShaderKey.initialize(stateDesc);
		mSystem->getShaderBoundState(stateDesc);
		mDescriptorDirty = true;
	}

	void VulkanContext::RHISetMeshShaderBoundState(MeshShaderStateDesc const& stateDesc)
	{
		bUseFixedShaderPipeline = false;
		mPendingShaderProgram = nullptr;
		mPendingShaderKey.initialize(stateDesc);
		mSystem->getShaderBoundState(stateDesc);
		mDescriptorDirty = true;
	}

	void VulkanContext::RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream)
	{
		mPendingInputLayout = inputLayout;
		mNumInputStream = numInputStream;
		for (int i = 0; i < numInputStream; ++i)
		{
			mInputStreams[i] = inputStreams[i];
		}
		mbInputStreamDirty = true;
	}

	void VulkanContext::RHISetIndexBuffer(RHIBuffer* buffer)
	{
		mPendingIndexBuffer = buffer;
	}

	void VulkanContext::setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture)
	{
		auto& shaderProgramImpl = static_cast<VulkanShaderProgram&>(shaderProgram);
		shaderProgramImpl.mParameterMap.setupShader(param, [this, &texture](int shaderIndex, ShaderParameter const& shaderParam)
		{
			if (shaderParam.bindIndex >= 0 && shaderParam.bindIndex < MaxDescriptorBindings)
			{
				mPendingDescriptors[shaderParam.bindIndex].texture = VulkanCast::To(&texture);
				mPendingDescriptors[shaderParam.bindIndex].level = 0;
				mPendingDescriptors[shaderParam.bindIndex].indexLayer = 0;
				if (mPendingDescriptors[shaderParam.bindIndex].type == VK_DESCRIPTOR_TYPE_MAX_ENUM || mPendingDescriptors[shaderParam.bindIndex].type == VK_DESCRIPTOR_TYPE_SAMPLER)
					mPendingDescriptors[shaderParam.bindIndex].type = (mPendingDescriptors[shaderParam.bindIndex].type == VK_DESCRIPTOR_TYPE_SAMPLER || mPendingDescriptors[shaderParam.bindIndex].sampler != nullptr) ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				mDescriptorDirty = true;
			}
		});
	}

	void VulkanContext::clearShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param)
	{
		auto& shaderProgramImpl = static_cast<VulkanShaderProgram&>(shaderProgram);
		shaderProgramImpl.mParameterMap.setupShader(param, [this](int shaderIndex, ShaderParameter const& shaderParam)
		{
			if (shaderParam.bindIndex >= 0 && shaderParam.bindIndex < MaxDescriptorBindings)
			{
				mPendingDescriptors[shaderParam.bindIndex].texture = nullptr;
				mPendingDescriptors[shaderParam.bindIndex].type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
				mPendingDescriptors[shaderParam.bindIndex].level = 0;
				mPendingDescriptors[shaderParam.bindIndex].indexLayer = 0;
				mDescriptorDirty = true;
			}
		});
	}

	void VulkanContext::setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op)
	{
		auto& shaderProgramImpl = static_cast<VulkanShaderProgram&>(shaderProgram);
		shaderProgramImpl.mParameterMap.setupShader(param, [this, &texture, level](int shaderIndex, ShaderParameter const& shaderParam)
		{
			if (shaderParam.bindIndex >= 0 && shaderParam.bindIndex < MaxDescriptorBindings)
			{
				mPendingDescriptors[shaderParam.bindIndex].texture = VulkanCast::To(&texture);
				mPendingDescriptors[shaderParam.bindIndex].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				mPendingDescriptors[shaderParam.bindIndex].level = level;
				mPendingDescriptors[shaderParam.bindIndex].indexLayer = 0;
				mDescriptorDirty = true;
			}
		});
	}

	void VulkanContext::clearShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param)
	{
		auto& shaderProgramImpl = static_cast<VulkanShaderProgram&>(shaderProgram);
		shaderProgramImpl.mParameterMap.setupShader(param, [this](int shaderIndex, ShaderParameter const& shaderParam)
		{
			if (shaderParam.bindIndex >= 0 && shaderParam.bindIndex < MaxDescriptorBindings)
			{
				mPendingDescriptors[shaderParam.bindIndex].texture = nullptr;
				mPendingDescriptors[shaderParam.bindIndex].type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
				mDescriptorDirty = true;
			}
		});
	}

	void VulkanContext::setShaderRWSubTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, int subIndex, int level, EAccessOp op)
	{
		auto& shaderProgramImpl = static_cast<VulkanShaderProgram&>(shaderProgram);
		shaderProgramImpl.mParameterMap.setupShader(param, [this, &texture, subIndex, level](int shaderIndex, ShaderParameter const& shaderParam)
		{
			if (shaderParam.bindIndex >= 0 && shaderParam.bindIndex < MaxDescriptorBindings)
			{
				mPendingDescriptors[shaderParam.bindIndex].texture = VulkanCast::To(&texture);
				mPendingDescriptors[shaderParam.bindIndex].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				mPendingDescriptors[shaderParam.bindIndex].level = level;
				mPendingDescriptors[shaderParam.bindIndex].indexLayer = subIndex;
				mDescriptorDirty = true;
			}
		});
	}

	void VulkanContext::setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler)
	{
		setShaderTexture(shaderProgram, param, texture);
		setShaderSampler(shaderProgram, paramSampler, sampler);
	}

	void VulkanContext::setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler)
	{
		auto& shaderProgramImpl = static_cast<VulkanShaderProgram&>(shaderProgram);
		shaderProgramImpl.mParameterMap.setupShader(param, [this, &sampler](int shaderIndex, ShaderParameter const& shaderParam)
		{
			if (shaderParam.bindIndex >= 0 && shaderParam.bindIndex < MaxDescriptorBindings)
			{
				mPendingDescriptors[shaderParam.bindIndex].sampler = static_cast<VulkanSamplerState*>(&sampler);
				if (mPendingDescriptors[shaderParam.bindIndex].type == VK_DESCRIPTOR_TYPE_MAX_ENUM || mPendingDescriptors[shaderParam.bindIndex].type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
					mPendingDescriptors[shaderParam.bindIndex].type = (mPendingDescriptors[shaderParam.bindIndex].type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || mPendingDescriptors[shaderParam.bindIndex].texture != nullptr) ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_SAMPLER;
				mDescriptorDirty = true;
			}
		});
	}

	void VulkanContext::setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer)
	{
		auto& shaderProgramImpl = static_cast<VulkanShaderProgram&>(shaderProgram);
		shaderProgramImpl.mParameterMap.setupShader(param, [this, &buffer](int shaderIndex, ShaderParameter const& shaderParam)
		{
			if (shaderParam.bindIndex >= 0 && shaderParam.bindIndex < MaxDescriptorBindings)
			{
				mPendingDescriptors[shaderParam.bindIndex].buffer = static_cast<VulkanBuffer*>(&buffer);
				mPendingDescriptors[shaderParam.bindIndex].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				mDescriptorDirty = true;
			}
		});
	}

	void VulkanContext::setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op)
	{
		auto& shaderProgramImpl = static_cast<VulkanShaderProgram&>(shaderProgram);
		shaderProgramImpl.mParameterMap.setupShader(param, [this, &buffer](int shaderIndex, ShaderParameter const& shaderParam)
		{
			if (shaderParam.bindIndex >= 0 && shaderParam.bindIndex < MaxDescriptorBindings)
			{
				mPendingDescriptors[shaderParam.bindIndex].buffer = static_cast<VulkanBuffer*>(&buffer);
				mPendingDescriptors[shaderParam.bindIndex].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				mDescriptorDirty = true;
			}
		});
	}

	void VulkanContext::setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer)
	{
		setShaderStorageBuffer(shaderProgram, param, buffer, EAccessOp::ReadAndWrite);
	}

	void VulkanContext::clearShaderBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, EAccessOp op)
	{
		auto& shaderProgramImpl = static_cast<VulkanShaderProgram&>(shaderProgram);
		shaderProgramImpl.mParameterMap.setupShader(param, [this](int shaderIndex, ShaderParameter const& shaderParam)
		{
			if (shaderParam.bindIndex >= 0 && shaderParam.bindIndex < MaxDescriptorBindings)
			{
				mPendingDescriptors[shaderParam.bindIndex].buffer = nullptr;
				mPendingDescriptors[shaderParam.bindIndex].type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
				mPendingDescriptors[shaderParam.bindIndex].bIsGlobalConst = false;
				mDescriptorDirty = true;
			}
		});
	}

	void VulkanContext::setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture)
	{
		VulkanShader& shaderImpl = static_cast<VulkanShader&>(shader);
		if (param.bindIndex >= 0 && param.bindIndex < MaxDescriptorBindings)
		{
			mPendingDescriptors[param.bindIndex].texture = VulkanCast::To(&texture);
			mPendingDescriptors[param.bindIndex].level = 0;
			mPendingDescriptors[param.bindIndex].indexLayer = 0;
			mPendingDescriptors[param.bindIndex].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			mDescriptorDirty = true;
		}
	}

	void VulkanContext::setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler)
	{
		setShaderTexture(shader, param, texture);
		setShaderSampler(shader, paramSampler, sampler);
	}

	void VulkanContext::clearShaderTexture(RHIShader& shader, ShaderParameter const& param)
	{
		if (param.bindIndex >= 0 && param.bindIndex < MaxDescriptorBindings)
		{
			mPendingDescriptors[param.bindIndex].texture = nullptr;
			mPendingDescriptors[param.bindIndex].type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
			mDescriptorDirty = true;
		}
	}

	void VulkanContext::setShaderSampler(RHIShader& shader, ShaderParameter const& param, RHISamplerState& sampler)
	{
		if (param.bindIndex >= 0 && param.bindIndex < MaxDescriptorBindings)
		{
			mPendingDescriptors[param.bindIndex].sampler = static_cast<VulkanSamplerState*>(&sampler);
			mPendingDescriptors[param.bindIndex].type = VK_DESCRIPTOR_TYPE_SAMPLER;
			mDescriptorDirty = true;
		}
	}

	void VulkanContext::setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op)
	{
		if (param.bindIndex >= 0 && param.bindIndex < MaxDescriptorBindings)
		{
			mPendingDescriptors[param.bindIndex].texture = VulkanCast::To(&texture);
			mPendingDescriptors[param.bindIndex].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			mPendingDescriptors[param.bindIndex].level = level;
			mPendingDescriptors[param.bindIndex].indexLayer = 0;
			mDescriptorDirty = true;
		}
	}

	void VulkanContext::setShaderRWSubTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, int subIndex, int level, EAccessOp op)
	{
		if (param.bindIndex >= 0 && param.bindIndex < MaxDescriptorBindings)
		{
			mPendingDescriptors[param.bindIndex].texture = VulkanCast::To(&texture);
			mPendingDescriptors[param.bindIndex].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			mPendingDescriptors[param.bindIndex].level = level;
			mPendingDescriptors[param.bindIndex].indexLayer = subIndex;
			mDescriptorDirty = true;
		}
	}

	void VulkanContext::clearShaderRWTexture(RHIShader& shader, ShaderParameter const& param)
	{
		if (param.bindIndex >= 0 && param.bindIndex < MaxDescriptorBindings)
		{
			mPendingDescriptors[param.bindIndex].texture = nullptr;
			mPendingDescriptors[param.bindIndex].type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
			mDescriptorDirty = true;
		}
	}

	void VulkanContext::setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer)
	{
		if (param.bindIndex >= 0 && param.bindIndex < MaxDescriptorBindings)
		{
			mPendingDescriptors[param.bindIndex].buffer = static_cast<VulkanBuffer*>(&buffer);
			mPendingDescriptors[param.bindIndex].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			mDescriptorDirty = true;
		}
	}

	void VulkanContext::setShaderStorageBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op)
	{
		if (param.bindIndex >= 0 && param.bindIndex < MaxDescriptorBindings)
		{
			mPendingDescriptors[param.bindIndex].buffer = static_cast<VulkanBuffer*>(&buffer);
			mPendingDescriptors[param.bindIndex].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			mDescriptorDirty = true;
		}
	}

	void VulkanContext::setShaderAtomicCounterBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer)
	{
		setShaderStorageBuffer(shader, param, buffer, EAccessOp::ReadAndWrite);
	}

	void VulkanContext::clearShaderBuffer(RHIShader& shader, ShaderParameter const& param, EAccessOp op)
	{
		if (param.bindIndex >= 0 && param.bindIndex < MaxDescriptorBindings)
		{
			mPendingDescriptors[param.bindIndex].buffer = nullptr;
			mPendingDescriptors[param.bindIndex].type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
			mPendingDescriptors[param.bindIndex].bIsGlobalConst = false;
			mDescriptorDirty = true;
		}
	}

	void VulkanContext::setShaderValueInternal(ShaderParameter const& param, uint8 const* pData, uint32 size, uint32 elementSize, uint32 stride)
	{
		if (!param.isBound() || param.bindIndex >= MaxDescriptorBindings) return;

		auto& cpuBuf = mPendingUniformBuffers[param.bindIndex];
		uint32 requiredSize = param.offset + size;
		if (cpuBuf.size() < requiredSize)
		{
			cpuBuf.resize(AlignArbitrary<uint32>(requiredSize, 256));
		}

		uint32 curOffset = param.offset;
		while (size > elementSize)
		{
			FMemory::Copy(cpuBuf.data() + curOffset, pData, elementSize);
			pData += elementSize;
			size -= elementSize;
			curOffset += stride;
		}
		if (size > 0)
		{
			FMemory::Copy(cpuBuf.data() + curOffset, pData, size);
		}

		mPendingUniformDirty[param.bindIndex] = true;
		mDescriptorDirty = true;
		
		PendingDescriptor& desc = mPendingDescriptors[param.bindIndex];
		desc.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		desc.bIsGlobalConst = true;
	}

	void VulkanContext::setShaderValueInternal(ShaderParameter const& param, uint8 const* pData, uint32 size)
	{
		if (!param.isBound() || param.bindIndex >= MaxDescriptorBindings) return;

		auto& cpuBuf = mPendingUniformBuffers[param.bindIndex];
		uint32 requiredSize = param.offset + size;
		if (cpuBuf.size() < requiredSize)
		{
			cpuBuf.resize(AlignArbitrary<uint32>(requiredSize, 256));
		}

		FMemory::Copy(cpuBuf.data() + param.offset, pData, size);

		mPendingUniformDirty[param.bindIndex] = true;
		mDescriptorDirty = true;
		
		PendingDescriptor& desc = mPendingDescriptors[param.bindIndex];
		desc.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		desc.bIsGlobalConst = true;
	}

	void VulkanContext::commitInputStreams()
	{
		if (!mbInputStreamDirty)
			return;

		TArray<VkBuffer> vertexBuffers;
		TArray<VkDeviceSize> offsets;

		for (int i = 0; i < mNumInputStream; ++i)
		{
			if (mInputStreams[i].buffer)
			{
				vertexBuffers.push_back(VulkanCast::GetHandle(*mInputStreams[i].buffer));
				offsets.push_back(mInputStreams[i].offset + VulkanCast::To(*mInputStreams[i].buffer).getBindOffset());
			}
		}

		if (!vertexBuffers.empty())
		{
			vkCmdBindVertexBuffers(mActiveCmdBuffer, 0, vertexBuffers.size(), vertexBuffers.data(), offsets.data());
		}

		if (mPendingIndexBuffer)
		{
			VkIndexType indexType = (mPendingIndexBuffer->getDesc().elementSize == 4) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
			vkCmdBindIndexBuffer(mActiveCmdBuffer, VulkanCast::GetHandle(*mPendingIndexBuffer), VulkanCast::To(*mPendingIndexBuffer).getBindOffset(), indexType);
		}

		mbInputStreamDirty = false;
	}


	void VulkanContext::commitDescriptorSets()
	{
		VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		TArray<VkDescriptorSetLayoutBinding> const* descriptorBindings = nullptr;

		VulkanShaderProgram* program = nullptr;
		VulkanShaderBoundState* boundState = nullptr;

		if (mPendingShaderKey.type == ShaderBoundStateKey::eShaderProgram)
		{
			program = VulkanCast::To(mPendingShaderProgram.get());
			if (program)
			{
				setLayout = program->mDescriptorSetLayout;
				pipelineLayout = program->mPipelineLayout;
				descriptorBindings = &program->mDescriptorBindings;
			}
		}
		else if (mPendingShaderKey.type == ShaderBoundStateKey::eCompute)
		{
			VulkanShader* shader = VulkanCast::To(mPendingComputeShader.get());
			if (shader)
			{
				setLayout = shader->mDescriptorSetLayout;
				pipelineLayout = shader->mPipelineLayout;
				descriptorBindings = &shader->mDescriptorBindings;
			}
		}
		else
		{
			auto iter = mSystem->mShaderBoundStateCache.find(mPendingShaderKey);
			if (iter != mSystem->mShaderBoundStateCache.end())
			{
				boundState = iter->second;
				setLayout = boundState->mDescriptorSetLayout;
				pipelineLayout = boundState->mPipelineLayout;
				descriptorBindings = &boundState->mDescriptorBindings;
			}
		}

		if (setLayout == VK_NULL_HANDLE || descriptorBindings == nullptr)
			return;

		VkPipelineBindPoint bindPoint = (mPendingShaderKey.type == ShaderBoundStateKey::eCompute) ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;

		if (!mDescriptorDirty)
		{
			if (mActiveCmdBuffer != mBoundCmdBuffer || mBoundPipelineLayout != pipelineLayout)
			{
				if (mBoundDescriptorSet != VK_NULL_HANDLE)
				{
					vkCmdBindDescriptorSets(mActiveCmdBuffer, bindPoint, pipelineLayout, 0, 1, &mBoundDescriptorSet, 0, nullptr);
					mBoundCmdBuffer = mActiveCmdBuffer;
					mBoundPipelineLayout = pipelineLayout;
				}
			}
			return;
		}

		VkDescriptorSet newSet = mDescriptorPoolManager.allocateDescriptorSet(setLayout);
		if (newSet == VK_NULL_HANDLE)
			return;

		TArray<VkWriteDescriptorSet> writes;
		TArray<VkDescriptorImageInfo> imageInfos;
		TArray<VkDescriptorBufferInfo> bufferInfos;

		// Reserve to avoid reallocation invalidating pointers
		imageInfos.resize(descriptorBindings->size());
		bufferInfos.resize(descriptorBindings->size());
		int imageInfoIndex = 0;
		int bufferInfoIndex = 0;

		for (auto const& binding : *descriptorBindings)
		{
			int i = binding.binding;
			if (i >= MaxDescriptorBindings) continue;

			PendingDescriptor& descInfo = mPendingDescriptors[i];

			VkWriteDescriptorSet write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = newSet;
			write.dstBinding = i;
			write.descriptorCount = 1;
			write.descriptorType = binding.descriptorType;

			if (binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				if (!descInfo.texture) continue;
				VkDescriptorImageInfo& imageInfo = imageInfos[imageInfoIndex++];
				imageInfo.imageView = descInfo.texture->getView(descInfo.level, descInfo.indexLayer, -1);
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.sampler = descInfo.sampler ? descInfo.sampler->getHandle() : mDevice->mDefaultSampler;
				
				write.pImageInfo = &imageInfo;
				writes.push_back(write);
			}
			else if (binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			{
				if (!descInfo.texture) continue;
				VkDescriptorImageInfo& imageInfo = imageInfos[imageInfoIndex++];
				imageInfo.imageView = descInfo.texture->getView(descInfo.level, descInfo.indexLayer);
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageInfo.sampler = VK_NULL_HANDLE;

				write.pImageInfo = &imageInfo;
				writes.push_back(write);
			}
			else if (binding.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			{
				if (!descInfo.texture) continue;
				VkDescriptorImageInfo& imageInfo = imageInfos[imageInfoIndex++];
				imageInfo.imageView = descInfo.texture->getView(descInfo.level, descInfo.indexLayer, -1);
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.sampler = VK_NULL_HANDLE;

				write.pImageInfo = &imageInfo;
				writes.push_back(write);
			}
			else if (binding.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)
			{
				VkDescriptorImageInfo& imageInfo = imageInfos[imageInfoIndex++];
				imageInfo.sampler = descInfo.sampler ? descInfo.sampler->getHandle() : mDevice->mDefaultSampler;
				imageInfo.imageView = VK_NULL_HANDLE;

				write.pImageInfo = &imageInfo;
				writes.push_back(write);
			}
			else if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			{
				VkDescriptorBufferInfo& bufInfo = bufferInfos[bufferInfoIndex++];
				if (descInfo.bIsGlobalConst)
				{
					if (mPendingUniformDirty[i] || descInfo.globalConstAlloc.buffer == VK_NULL_HANDLE)
					{
						auto& cpuBuf = mPendingUniformBuffers[i];
						if (cpuBuf.size() > 0)
						{
							VulkanDynamicBufferManager::Get().allocFrame(cpuBuf.size(), 256, descInfo.globalConstAlloc);
							FMemory::Copy(descInfo.globalConstAlloc.cpuAddress, cpuBuf.data(), cpuBuf.size());
						}
						mPendingUniformDirty[i] = false;
					}

					if (descInfo.globalConstAlloc.buffer == VK_NULL_HANDLE)
						continue;

					bufInfo.buffer = descInfo.globalConstAlloc.buffer;
					bufInfo.offset = descInfo.globalConstAlloc.offset;
					bufInfo.range = mPendingUniformBuffers[i].size();
				}
				else
				{
					if (!descInfo.buffer) continue;
					bufInfo.buffer = descInfo.buffer->getHandle();
					bufInfo.offset = descInfo.buffer->getBindOffset();
					bufInfo.range = descInfo.buffer->getSize();
				}
				write.pBufferInfo = &bufInfo;
				writes.push_back(write);
			}
		}

		if (!writes.empty())
		{
			vkUpdateDescriptorSets(mDevice->logicalDevice, (uint32)writes.size(), writes.data(), 0, nullptr);
		}

		vkCmdBindDescriptorSets(mActiveCmdBuffer, bindPoint,
			pipelineLayout, 0, 1, &newSet, 0, nullptr);
		mBoundDescriptorSet = newSet;
		mBoundCmdBuffer = mActiveCmdBuffer;
		mBoundPipelineLayout = pipelineLayout;
		mDescriptorDirty = false;
	}

	VkPipeline VulkanContext::getOrCreateGraphicsPipeline(VulkanGraphicsPipelineStateKey const& key)
	{
		auto iter = mPipelineCache.find(key);
		if (iter != mPipelineCache.end())
		{
			return iter->second;
		}

		// Build new pipeline
		VulkanShaderBoundState* boundState = nullptr;
		VulkanShaderProgram* program = nullptr;
		if (key.shaderKey.type == ShaderBoundStateKey::eShaderProgram)
		{
			program = VulkanCast::To(mPendingShaderProgram.get());
			if (!program) return VK_NULL_HANDLE;
		}
		else
		{
			auto iter = mSystem->mShaderBoundStateCache.find(key.shaderKey);
			if (iter != mSystem->mShaderBoundStateCache.end())
			{
				boundState = iter->second;
			}
			if (!boundState)
				return VK_NULL_HANDLE;
		}

		VkGraphicsPipelineCreateInfo pipelineInfo = FVulkanInit::pipelineCreateInfo();
		pipelineInfo.renderPass = key.renderPass;

		if (program)
		{
			pipelineInfo.pStages = program->mStages.data();
			pipelineInfo.stageCount = (uint32)program->mStages.size();
			pipelineInfo.layout = program->mPipelineLayout;
		}
		else
		{
			pipelineInfo.pStages = boundState->mStages.data();
			pipelineInfo.stageCount = (uint32)boundState->mStages.size();
			pipelineInfo.layout = boundState->mPipelineLayout;
		}

		// Vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		if (key.inputLayout)
		{
			VulkanInputLayout* inputLayout = VulkanCast::To(key.inputLayout);
			vertexInputState = inputLayout->createInfo;
		}
		pipelineInfo.pVertexInputState = &vertexInputState;

		// Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;
		int patchPointCount = 0;
		inputAssemblyState.topology = VulkanTranslate::To(key.primitiveType, patchPointCount);
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;

		// Tessellation
		VkPipelineTessellationStateCreateInfo tesselationState = {};
		tesselationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		tesselationState.patchControlPoints = patchPointCount;
		pipelineInfo.pTessellationState = &tesselationState;

		// Viewport (dynamic)
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		pipelineInfo.pViewportState = &viewportState;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterState = {};
		rasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterState.lineWidth = 1.0f;
		if (key.rasterizerState)
		{
			rasterState = VulkanCast::To(key.rasterizerState)->createInfo;
		}
		pipelineInfo.pRasterizationState = &rasterState;

		// Multisample
		VkPipelineMultisampleStateCreateInfo multisampleState = {};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VulkanTranslate::ToSampleCount(key.samples);
		multisampleState.minSampleShading = 1.0f;
		if (key.rasterizerState)
		{
			VulkanCast::To(key.rasterizerState)->getMultiSampleState(multisampleState);
		}
		if (key.blendState)
		{
			VulkanCast::To(key.blendState)->getMultiSampleState(multisampleState);
		}
		pipelineInfo.pMultisampleState = &multisampleState;

		// Depth stencil
		VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		if (key.depthStencilState)
		{
			depthStencilState = VulkanCast::To(key.depthStencilState)->createInfo;
		}
		pipelineInfo.pDepthStencilState = &depthStencilState;

		// Color blend
		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		VkPipelineColorBlendAttachmentState colorBlendAttachments[MaxBlendStateTargetCount];
		colorBlendState.pAttachments = colorBlendAttachments;

		if (key.blendState)
		{
			VulkanBlendState* blendState = VulkanCast::To(key.blendState);
			colorBlendState.attachmentCount = blendState->createInfo.attachmentCount;
			for (int i = 0; i < (int)colorBlendState.attachmentCount; ++i)
			{
				colorBlendAttachments[i] = blendState->colorAttachmentStates[i];
			}
		}
		else
		{
			colorBlendState.attachmentCount = 1;
			colorBlendAttachments[0] = {};
			colorBlendAttachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachments[0].blendEnable = VK_TRUE;
			colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
		}
		pipelineInfo.pColorBlendState = &colorBlendState;

		// Dynamic state
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pDynamicStates = dynamicStates;
		dynamicState.dynamicStateCount = ARRAY_SIZE(dynamicStates);
		pipelineInfo.pDynamicState = &dynamicState;

		pipelineInfo.subpass = 0;

		VkPipeline pipeline = VK_NULL_HANDLE;
		VkResult result = vkCreateGraphicsPipelines(mDevice->logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, gAllocCB, &pipeline);
		if (result != VK_SUCCESS)
		{
			LogWarning(0, "VulkanContext::getOrCreateGraphicsPipeline: vkCreateGraphicsPipelines failed: %s",
				FVulkanUtilies::GetResultErrorString(result));
			return VK_NULL_HANDLE;
		}

		mPipelineCache[key] = pipeline;
		return pipeline;
	}

	void VulkanContext::RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ)
	{
		endRenderPass();

		for (int i = 0; i < MaxDescriptorBindings; ++i)
		{
			auto& desc = mPendingDescriptors[i];
			if (desc.texture && desc.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			{
				if (desc.texture->mImageLayout != VK_IMAGE_LAYOUT_GENERAL)
				{
					VkImageSubresourceRange range = {};
					range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					range.baseMipLevel = desc.level;
					range.levelCount = 1; range.baseArrayLayer = desc.indexLayer; range.layerCount = 1;
					FVulkanTexture::SetImageLayout(mActiveCmdBuffer, desc.texture->image, desc.texture->mImageLayout, VK_IMAGE_LAYOUT_GENERAL, range);
					desc.texture->mImageLayout = VK_IMAGE_LAYOUT_GENERAL;
				}
			}
		}

		commitComputeShaderState();
		vkCmdDispatch(mActiveCmdBuffer, numGroupX, numGroupY, numGroupZ);
	}

	void VulkanContext::RHIGenerateMips(RHITextureBase& texture)
	{
		if (texture.getType() != ETexture::Type2D)
			return;

		VulkanTexture& textureImpl = *VulkanCast::To(&texture);
		if (textureImpl.image == VK_NULL_HANDLE)
			return;

		TextureDesc const& desc = texture.getDesc();
		uint32 mipLevels = textureImpl.mMipLevels;
		if (mipLevels <= 1)
			return;

		endRenderPass();

		VkImage image = textureImpl.image;
		VkExtent3D extent = { (uint32)desc.dimension.x, (uint32)desc.dimension.y, 1 };
		FVulkanTexture::GenerateMipmaps(mActiveCmdBuffer, image, extent, mipLevels, 1, textureImpl.mImageLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		textureImpl.mImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	void VulkanContext::commitComputeShaderState()
	{
		if (!mPendingComputeShader)
			return;

		VkPipeline pipeline = getOrCreateComputePipeline(mPendingComputeShader);
		if (pipeline != VK_NULL_HANDLE && pipeline != mBoundPipeline)
		{
			vkCmdBindPipeline(mActiveCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
			mBoundPipeline = pipeline;
		}

		commitDescriptorSets();
	}

	VkPipeline VulkanContext::getOrCreateComputePipeline(RHIShader* computeShader)
	{
		ShaderBoundStateKey key;
		key.initialize(*computeShader);

		auto iter = mComputePipelineCache.find(key);
		if (iter != mComputePipelineCache.end())
		{
			return iter->second;
		}

		VulkanShader* shaderImpl = VulkanCast::To(computeShader);

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = shaderImpl->mPipelineLayout;
		pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		pipelineInfo.stage.module = shaderImpl->mModule;
		pipelineInfo.stage.pName = shaderImpl->mEntryPoint.c_str();

		VkPipeline pipeline;
		VK_VERIFY_FAILCODE(vkCreateComputePipelines(mDevice->logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, gAllocCB, &pipeline), return VK_NULL_HANDLE;);

		mComputePipelineCache[key] = pipeline;
		return pipeline;
	}

	void VulkanContext::commitGraphicsShaderState(EPrimitive primitiveType)
	{
		if (bUseFixedShaderPipeline)
		{
			if (mPendingInputLayout)
			{
				VulkanInputLayout* inputLayoutImpl = VulkanCast::To(mPendingInputLayout.get());
				SimplePipelineProgram* program = SimplePipelineProgram::Get(inputLayoutImpl->mAttriableMask, mFixedShaderParams.texture != nullptr);
				RHISetShaderProgram(program->getRHI());
				program->setParameters(RHICommandListImpl(*this), mFixedShaderParams.transform, mFixedShaderParams.color, mFixedShaderParams.texture, mFixedShaderParams.sampler);
			}
		}

		if (!mPendingShaderProgram || mActiveFrameBuffer == nullptr || mActiveFrameBuffer->mRenderPass == VK_NULL_HANDLE)
			return;

		for (int i = 0; i < MaxDescriptorBindings; ++i)
		{
			auto& desc = mPendingDescriptors[i];
			if (desc.texture && (desc.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || desc.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER))
			{
				if (desc.texture->mImageLayout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				{
					bool bInPass = mInsideRenderPass;
					if (bInPass) endRenderPass();

					VkImageSubresourceRange range = {};
					range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					range.baseMipLevel = desc.level;
					range.levelCount = 1; range.baseArrayLayer = desc.indexLayer; range.layerCount = 1;
					FVulkanTexture::SetImageLayout(mActiveCmdBuffer, desc.texture->image, desc.texture->mImageLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
					desc.texture->mImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					if (bInPass) beginRenderPass();
				}
			}
		}

		beginRenderPass();

		commitInputStreams();

		VulkanGraphicsPipelineStateKey key;
		key.shaderKey = mPendingShaderKey;
		key.inputLayout = mPendingInputLayout;
		key.rasterizerState = mPendingRasterizerState;
		key.blendState = mPendingBlendState;
		key.depthStencilState = mPendingDepthStencilState;
		key.renderPass = mActiveFrameBuffer->mRenderPass;
		key.samples = mActiveFrameBuffer ? mActiveFrameBuffer->getSampleCount() : 1;
		key.primitiveType = primitiveType;

		VkPipeline pipeline = getOrCreateGraphicsPipeline(key);
		if (pipeline != VK_NULL_HANDLE && pipeline != mBoundPipeline)
		{
			vkCmdBindPipeline(mActiveCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			mBoundPipeline = pipeline;
		}

		commitDescriptorSets();
	}

	void VulkanContext::RHISetFixedShaderPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler)
	{
		bUseFixedShaderPipeline = true;
		mFixedShaderParams.transform = transform;
		mFixedShaderParams.color = color;
		mFixedShaderParams.texture = texture;
		mFixedShaderParams.sampler = sampler;
	}

	void VulkanContext::RHIDrawPrimitive(EPrimitive type, int start, int nv)
	{
		commitGraphicsShaderState(type);
		vkCmdDraw(mActiveCmdBuffer, nv, 1, start, 0);
	}

	void VulkanContext::RHIDrawIndexedPrimitive(EPrimitive type, int indexStart, int nIndex, uint32 baseVertex)
	{
		commitGraphicsShaderState(type);
		vkCmdDrawIndexed(mActiveCmdBuffer, nIndex, 1, indexStart, baseVertex, 0);
	}

	void VulkanContext::RHIDrawPrimitiveInstanced(EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance)
	{
		commitGraphicsShaderState(type);
		vkCmdDraw(mActiveCmdBuffer, nv, numInstance, vStart, baseInstance);
	}

	void VulkanContext::RHIDrawIndexedPrimitiveInstanced(EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance)
	{
		commitGraphicsShaderState(type);
		vkCmdDrawIndexed(mActiveCmdBuffer, nIndex, numInstance, indexStart, baseVertex, baseInstance);
	}

	bool VulkanContext::determitPrimitiveTopologyUP(EPrimitive primitive, int num, uint32 const* pIndices, EPrimitive& outPrimitiveDetermited, VulkanBufferAllocation& outIndexAlloc, int& outIndexNum)
	{
		struct MyBuffer
		{
			MyBuffer(VulkanBufferAllocation& allocation)
				:mAllocation(allocation)
			{
			}
			void* lock(uint32 size)
			{
				if (VulkanDynamicBufferManager::Get().allocFrame(size, 16, mAllocation))
				{
					return mAllocation.cpuAddress;
				}
				return nullptr;
			}
			void unlock()
			{
			}
			VulkanBufferAllocation& mAllocation;
		};

		MyBuffer myBuffer(outIndexAlloc);
		int patchPointCount = 0;
		return DetermitPrimitiveTopologyUP(primitive, VulkanTranslate::To(primitive, patchPointCount) != VK_PRIMITIVE_TOPOLOGY_POINT_LIST || primitive == EPrimitive::Points, num, pIndices, myBuffer, outPrimitiveDetermited, outIndexNum);
	}

	void VulkanContext::RHIDrawPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numData, uint32 numInstance)
	{
		EPrimitive determitedPrimitive;
		VulkanBufferAllocation indexAlloc;
		int numIndex;
		if (determitPrimitiveTopologyUP(type, numVertices, nullptr, determitedPrimitive, indexAlloc, numIndex))
		{
			TArray<VkBuffer> vertexBuffers;
			TArray<VkDeviceSize> offsets;

			for (int i = 0; i < numData; ++i)
			{
				VulkanBufferAllocation alloc;
				if (VulkanDynamicBufferManager::Get().allocFrame(dataInfos[i].size, 16, alloc))
				{
					FMemory::Copy(alloc.cpuAddress, dataInfos[i].ptr, dataInfos[i].size);
					vertexBuffers.push_back(alloc.buffer);
					offsets.push_back(alloc.offset);
				}
			}

			commitGraphicsShaderState(determitedPrimitive);
			if (!vertexBuffers.empty())
			{
				vkCmdBindVertexBuffers(mActiveCmdBuffer, 0, (uint32)vertexBuffers.size(), vertexBuffers.data(), offsets.data());
			}

			if (indexAlloc.buffer != VK_NULL_HANDLE)
			{
				vkCmdBindIndexBuffer(mActiveCmdBuffer, indexAlloc.buffer, indexAlloc.offset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(mActiveCmdBuffer, numIndex, numInstance, 0, 0, 0);
			}
			else
			{
				vkCmdDraw(mActiveCmdBuffer, numVertices, numInstance, 0, 0);
			}
		}
	}

	void VulkanContext::RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVerex, VertexDataInfo dataInfos[], int numVertexData, uint32 const* pIndices, int numIndex, uint32 numInstance)
	{
		EPrimitive determitedPrimitive;
		VulkanBufferAllocation indexAlloc;
		int determitedNumIndex;
		if (determitPrimitiveTopologyUP(type, numIndex, pIndices, determitedPrimitive, indexAlloc, determitedNumIndex))
		{
			TArray<VkBuffer> vertexBuffers;
			TArray<VkDeviceSize> offsets;

			for (int i = 0; i < numVertexData; ++i)
			{
				VulkanBufferAllocation alloc;
				if (VulkanDynamicBufferManager::Get().allocFrame(dataInfos[i].size, 16, alloc))
				{
					FMemory::Copy(alloc.cpuAddress, dataInfos[i].ptr, dataInfos[i].size);
					vertexBuffers.push_back(alloc.buffer);
					offsets.push_back(alloc.offset);
				}
			}

			commitGraphicsShaderState(determitedPrimitive);
			if (!vertexBuffers.empty())
			{
				vkCmdBindVertexBuffers(mActiveCmdBuffer, 0, (uint32)vertexBuffers.size(), vertexBuffers.data(), offsets.data());
			}

			if (indexAlloc.buffer != VK_NULL_HANDLE)
			{
				vkCmdBindIndexBuffer(mActiveCmdBuffer, indexAlloc.buffer, indexAlloc.offset, VK_INDEX_TYPE_UINT32);
			}

			vkCmdDrawIndexed(mActiveCmdBuffer, determitedNumIndex, numInstance, 0, 0, 0);
		}
	}

	void VulkanContext::setAsActive()
	{
		if (mSystem && mSystem->mProfileCore)
		{
			mSystem->mProfileCore->mCmdList = mActiveCmdBuffer;
		}
	}

	void VulkanContext::beginRenderPass()
	{
		if (mInsideRenderPass)
			return;

		if (mActiveFrameBuffer == nullptr || mActiveRenderPass == VK_NULL_HANDLE || mActiveCmdBuffer == VK_NULL_HANDLE)
			return;

		VulkanFrameBuffer* vfb = mActiveFrameBuffer;

		// Ensure framebuffer object is created
		if (vfb->bBufferDirty || vfb->mFramebuffer == VK_NULL_HANDLE)
		{
			if (!vfb->createFramebuffer(vfb->mWidth, vfb->mHeight))
			{
				LogWarning(0, "VulkanContext::beginRenderPass: Failed to create framebuffer");
				return;
			}
		}

		// Build clear values
		TArray<VkClearValue> clearValues;
		if (vfb->mColorBuffers.empty() && vfb->mFramebuffer != VK_NULL_HANDLE)
		{
			// Swap chain framebuffer: no RHI color buffers, but the render pass
			// expects a clear value for VK_ATTACHMENT_LOAD_OP_CLEAR
			VkClearValue clearValue = {};
			clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues.push_back(clearValue);

			// Add padding for resolve attachment if MSAA is used for swap chain
			if (mSystem->mSwapChain->mNumSamples > 1)
			{
				VkClearValue padding = {};
				clearValues.push_back(padding);
			}
		}
		else
		{
			for (auto const& bufferInfo : vfb->mColorBuffers)
			{
				VkClearValue clearValue = {};
				clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
				clearValues.push_back(clearValue);
			}
		}
		if (vfb->mDepthBuffer.view != VK_NULL_HANDLE)
		{
			VkClearValue clearValue = {};
			clearValue.depthStencil = { 1.0f, 0 };
			clearValues.push_back(clearValue);
		}


		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = vfb->mRenderPass;
		renderPassBeginInfo.framebuffer = vfb->mFramebuffer;
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = { vfb->mWidth, vfb->mHeight };
		renderPassBeginInfo.pClearValues = clearValues.data();
		renderPassBeginInfo.clearValueCount = clearValues.size();

		vkCmdBeginRenderPass(mActiveCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		mInsideRenderPass = true;

		// Set default viewport and scissor
		VkViewport viewport = { 0.0f, (float)vfb->mHeight, (float)vfb->mWidth, -(float)vfb->mHeight, 0.0f, 1.0f };
		vkCmdSetViewport(mActiveCmdBuffer, 0, 1, &viewport);
		VkRect2D scissor = { {0, 0}, {vfb->mWidth, vfb->mHeight} };
		vkCmdSetScissor(mActiveCmdBuffer, 0, 1, &scissor);
	}

	void VulkanContext::endRenderPass()
	{
		if (mInsideRenderPass)
		{
			vkCmdEndRenderPass(mActiveCmdBuffer);
			mInsideRenderPass = false;
		}
	}

	void VulkanContext::RHISetFrameBuffer(RHIFrameBuffer* frameBuffer)
	{
		// End current render pass if active
		endRenderPass();

		if (frameBuffer == nullptr)
		{
			// Set to swap chain framebuffer (back buffer)
			if (mSwapChainFrameBufferRHI)
			{
				mActiveRenderPass = mSwapChainFrameBufferRHI->mRenderPass;
				mActiveFrameBuffer = mSwapChainFrameBufferRHI;
				mBoundPipeline = VK_NULL_HANDLE;
				beginRenderPass();
			}
			else
			{
				mActiveRenderPass = VK_NULL_HANDLE;
				mActiveFrameBuffer = nullptr;
				mBoundPipeline = VK_NULL_HANDLE;
			}
			return;
		}

		VulkanFrameBuffer* vfb = static_cast<VulkanFrameBuffer*>(frameBuffer);

		// Build or rebuild the render pass if needed
		if (vfb->bPipelineLayoutDirty)
		{
			vfb->createRenderPass();
			vfb->bPipelineLayoutDirty = false;
		}

		// Determine framebuffer dimensions from first color buffer or depth buffer
		if (vfb->bBufferDirty)
		{
			uint32 width = 0, height = 0;
			if (!vfb->mColorBuffers.empty() && vfb->mColorBuffers[0].texture)
			{
				RHITexture2D* tex2D = vfb->mColorBuffers[0].texture->getTexture2D();
				if (tex2D)
				{
					width = tex2D->getSizeX();
					height = tex2D->getSizeY();
				}
			}
			else if (vfb->mDepthBuffer.texture)
			{
				RHITexture2D* tex2D = vfb->mDepthBuffer.texture->getTexture2D();
				if (tex2D)
				{
					width = tex2D->getSizeX();
					height = tex2D->getSizeY();
				}
			}

			if (width > 0 && height > 0)
			{
				vfb->createFramebuffer(width, height);
			}
		}

		mActiveRenderPass = vfb->mRenderPass;
		mActiveFrameBuffer = vfb;
		mBoundPipeline = VK_NULL_HANDLE;

		// Begin render pass immediately  
		beginRenderPass();
	}

	void VulkanContext::RHIClearRenderTargets(EClearBits clearBits, LinearColor colors[], int numColor, float depth, uint8 stenceil)
	{
		beginRenderPass();
		if (!mInsideRenderPass)
			return;

		TArray<VkClearAttachment> clearAttachments;

		if (HaveBits(clearBits, EClearBits::Color))
		{
			for (int i = 0; i < numColor; ++i)
			{
				VkClearAttachment attachment = {};
				attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				attachment.colorAttachment = i;
				attachment.clearValue.color.float32[0] = colors[i].r;
				attachment.clearValue.color.float32[1] = colors[i].g;
				attachment.clearValue.color.float32[2] = colors[i].b;
				attachment.clearValue.color.float32[3] = colors[i].a;
				clearAttachments.push_back(attachment);
			}
		}

		if (HaveBits(clearBits, EClearBits::Depth) || HaveBits(clearBits, EClearBits::Stencil))
		{
			VkClearAttachment attachment = {};
			if (HaveBits(clearBits, EClearBits::Depth))
				attachment.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
			if (HaveBits(clearBits, EClearBits::Stencil))
				attachment.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			attachment.clearValue.depthStencil = { depth, stenceil };
			clearAttachments.push_back(attachment);
		}

		if (!clearAttachments.empty())
		{
			uint32 renderAreaWidth = UINT32_MAX;
			uint32 renderAreaHeight = UINT32_MAX;

			if (mActiveFrameBuffer == mSwapChainFrameBufferRHI)
			{
				renderAreaWidth = mSystem->mSwapChain->mImageSize.width;
				renderAreaHeight = mSystem->mSwapChain->mImageSize.height;
			}
			else if (!mActiveFrameBuffer->mColorBuffers.empty() && mActiveFrameBuffer->mColorBuffers[0].texture)
			{
				if (auto tex = mActiveFrameBuffer->mColorBuffers[0].texture->getTexture2D())
				{
					renderAreaWidth = tex->getSizeX();
					renderAreaHeight = tex->getSizeY();
				}
			}
			else if (mActiveFrameBuffer->mDepthBuffer.texture)
			{
				if (auto tex = mActiveFrameBuffer->mDepthBuffer.texture->getTexture2D())
				{
					renderAreaWidth = tex->getSizeX();
					renderAreaHeight = tex->getSizeY();
				}
			}

			// Clear entire framebuffer area
			VkClearRect clearRect = {};
			clearRect.layerCount = 1;
			clearRect.rect.extent.width = renderAreaWidth;
			clearRect.rect.extent.height = renderAreaHeight;

			vkCmdClearAttachments(mActiveCmdBuffer, clearAttachments.size(), clearAttachments.data(), 1, &clearRect);
		}
	}

	void VulkanContext::RHIResourceTransition(TArrayView<RHIResource*> resources, EResourceTransition transition)
	{
		if (mActiveCmdBuffer == VK_NULL_HANDLE)
			return;

		for (RHIResource* resource : resources)
		{
			if (resource == nullptr)
				continue;

			if (resource->getResourceType() == EResourceType::Texture)
			{
				VulkanTexture* vkTexture = VulkanCast::To(static_cast<RHITextureBase*>(resource));
				VkImageLayout targetLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				switch (transition)
				{
				case EResourceTransition::UAV:
				case EResourceTransition::UAVBarrier:
					targetLayout = VK_IMAGE_LAYOUT_GENERAL;
					break;
				case EResourceTransition::SRV:
					targetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					break;
				case EResourceTransition::CopySrc:
					targetLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					break;
				case EResourceTransition::CopyDest:
					targetLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					break;
				case EResourceTransition::RenderTarget:
					targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					break;
				}

				if (targetLayout != VK_IMAGE_LAYOUT_UNDEFINED)
				{
					if (vkTexture->mImageLayout != targetLayout || transition == EResourceTransition::UAVBarrier)
					{
						bool bWasInside = mInsideRenderPass;
						if (mInsideRenderPass)
							endRenderPass();

						VkImageSubresourceRange range = {};
						range.aspectMask = (ETexture::IsDepthStencil(static_cast<RHITextureBase*>(resource)->getFormat())) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
						range.baseMipLevel = 0;
						range.levelCount = VK_REMAINING_MIP_LEVELS;
						range.baseArrayLayer = 0;
						range.layerCount = VK_REMAINING_ARRAY_LAYERS;

						FVulkanTexture::SetImageLayout(mActiveCmdBuffer, vkTexture->image, vkTexture->mImageLayout, targetLayout, range);
						vkTexture->mImageLayout = targetLayout;

						if (bWasInside)
							beginRenderPass();
					}
				}
			}
			else if (resource->getResourceType() == EResourceType::Buffer)
			{
				VulkanBuffer* vkBuffer = VulkanCast::To(static_cast<RHIBuffer*>(resource));
				if (transition == EResourceTransition::UAV || transition == EResourceTransition::UAVBarrier)
				{
					VkBufferMemoryBarrier barrier = FVulkanInit::bufferMemoryBarrier();
					barrier.buffer = vkBuffer->getHandle();
					barrier.offset = 0;
					barrier.size = VK_WHOLE_SIZE;
					barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

					vkCmdPipelineBarrier(mActiveCmdBuffer,
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						0, 0, nullptr, 1, &barrier, 0, nullptr);
				}
			}
		}
	}


	// =============================== VulkanSystem ===============================

	VulkanShaderBoundState* VulkanSystem::getShaderBoundState(GraphicsShaderStateDesc const& stateDesc)
	{
		ShaderBoundStateKey key;
		key.initialize(stateDesc);

		auto iter = mShaderBoundStateCache.find(key);
		if (iter != mShaderBoundStateCache.end())
			return iter->second;

		VulkanShaderBoundState* boundState = new VulkanShaderBoundState();
		
		TArray<VkDescriptorSetLayoutBinding> mergedBindings;
		auto addStages = [&](RHIShader* shaderRHI)
		{
			if (!shaderRHI) return;
			VulkanShader* shader = VulkanCast::To(shaderRHI);
			VkPipelineShaderStageCreateInfo stage = {};
			stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.stage = VulkanTranslate::To(shader->getType());
			stage.module = shader->mModule;
			stage.pName = shader->mEntryPoint.c_str();
			boundState->mStages.push_back(stage);

			for (auto const& binding : shader->mDescriptorBindings)
			{
				bool bFound = false;
				for (auto& merged : mergedBindings)
				{
					if (merged.binding == binding.binding)
					{
						merged.stageFlags |= binding.stageFlags;
						bFound = true;
						break;
					}
				}
				if (!bFound)
				{
					mergedBindings.push_back(binding);
				}
			}
		};

		addStages(stateDesc.vertex);
		addStages(stateDesc.pixel);
		addStages(stateDesc.geometry);
		addStages(stateDesc.hull);
		addStages(stateDesc.domain);

		boundState->mDescriptorBindings = mergedBindings;
		VkDescriptorSetLayoutCreateInfo layoutInfo = FVulkanInit::descriptorSetLayoutCreateInfo(mergedBindings.data(), (uint32)mergedBindings.size());
		VERIFY_FAILCODE(boundState->mDescriptorSetLayout.initialize(mDevice->logicalDevice, layoutInfo), delete boundState; return nullptr);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = FVulkanInit::pipelineLayoutCreateInfo(&boundState->mDescriptorSetLayout.mHandle, 1);
		VERIFY_FAILCODE(boundState->mPipelineLayout.initialize(mDevice->logicalDevice, pipelineLayoutInfo), boundState->release(mDevice->logicalDevice); delete boundState; return nullptr);

		mShaderBoundStateCache[key] = boundState;
		return boundState;
	}

	VulkanShaderBoundState* VulkanSystem::getShaderBoundState(MeshShaderStateDesc const& stateDesc)
	{
		ShaderBoundStateKey key;
		key.initialize(stateDesc);

		auto iter = mShaderBoundStateCache.find(key);
		if (iter != mShaderBoundStateCache.end())
			return iter->second;

		VulkanShaderBoundState* boundState = new VulkanShaderBoundState();

		TArray<VkDescriptorSetLayoutBinding> mergedBindings;
		auto addStages = [&](RHIShader* shaderRHI)
		{
			if (!shaderRHI) return;
			VulkanShader* shader = VulkanCast::To(shaderRHI);
			VkPipelineShaderStageCreateInfo stage = {};
			stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.stage = VulkanTranslate::To(shader->getType());
			stage.module = shader->mModule;
			stage.pName = shader->mEntryPoint.c_str();
			boundState->mStages.push_back(stage);

			for (auto const& binding : shader->mDescriptorBindings)
			{
				bool bFound = false;
				for (auto& merged : mergedBindings)
				{
					if (merged.binding == binding.binding)
					{
						merged.stageFlags |= binding.stageFlags;
						bFound = true;
						break;
					}
				}
				if (!bFound)
				{
					mergedBindings.push_back(binding);
				}
			}
		};

		addStages(stateDesc.task);
		addStages(stateDesc.mesh);
		addStages(stateDesc.pixel);

		boundState->mDescriptorBindings = mergedBindings;
		VkDescriptorSetLayoutCreateInfo layoutInfo = FVulkanInit::descriptorSetLayoutCreateInfo(mergedBindings.data(), (uint32)mergedBindings.size());
		VERIFY_FAILCODE(boundState->mDescriptorSetLayout.initialize(mDevice->logicalDevice, layoutInfo), delete boundState; return nullptr);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = FVulkanInit::pipelineLayoutCreateInfo(&boundState->mDescriptorSetLayout.mHandle, 1);
		VERIFY_FAILCODE(boundState->mPipelineLayout.initialize(mDevice->logicalDevice, pipelineLayoutInfo), boundState->release(mDevice->logicalDevice); delete boundState; return nullptr);

		mShaderBoundStateCache[key] = boundState;
		return boundState;
	}

	bool VulkanSystem::createInstance(TArray<VkExtensionProperties> const& availableExtensions, bool enableValidation)
	{
		// Validation can also be forced via a define
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "GemeEngine";
		appInfo.pEngineName = "RHIEngine";
		appInfo.apiVersion = VK_API_VERSION_1_1;

		TArray<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

		// Enable surface extensions depending on os
#if SYS_PLATFORM_WIN
		instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

		if (enabledInstanceExtensions.size() > 0)
		{
			for (auto enabledExtension : enabledInstanceExtensions)
			{
				if (FPropertyName::Find(availableExtensions, enabledExtension))
				{
					instanceExtensions.push_back(enabledExtension);

				}
				else
				{
					LogWarning(0, "Can't find Instance extension : %s", enabledExtension);
				}
			}
		}

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = NULL;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		if (instanceExtensions.size() > 0)
		{
			if (enableValidation)
			{
				instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}
			instanceCreateInfo.enabledExtensionCount = (uint32)instanceExtensions.size();
			instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
		}
		if (enableValidation)
		{
			// The VK_LAYER_KHRONOS_validation contains all current validation functionality.
			// Note that on Android this layer requires at least NDK r20 
			const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
			// Check if this layer is available at instance level
			TArray<VkLayerProperties> instanceLayerProperties = GetEnumValues(vkEnumerateInstanceLayerProperties);
			bool validationLayerPresent = FPropertyName::Find(instanceLayerProperties, "VK_LAYER_KHRONOS_validation");

			if (validationLayerPresent) 
			{
				instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
				instanceCreateInfo.enabledLayerCount = 1;
			}
			else
			{
			
				LogWarning(0,"Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled");
				enableValidation = false;
			}
		}

		VK_VERIFY_RETURN_FALSE(vkCreateInstance(&instanceCreateInfo, nullptr, &mInstance));

		bool bHaveDebugUtils = FPropertyName::Find(availableExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		if (enableValidation && bHaveDebugUtils)
		{
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");
			if (func)
			{
				VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
				debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
				debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
				debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
				debugInfo.pfnUserCallback = DebugVKCallback;
				debugInfo.pUserData = nullptr;
				VK_VERIFY_RETURN_FALSE(func(mInstance, &debugInfo, gAllocCB, &mCallback));
			}

		}
		return true;
	}


	void VulkanSystem::shutdown()
	{
		vkDeviceWaitIdle(mDevice->logicalDevice);

		mDrawContext.release();
		cleanupSwapChainRenderPass();
		cleanupRenderResource();

		if (mProfileCore)
		{
			mProfileCore = nullptr;
		}

		for (auto& pair : mShaderBoundStateCache)
		{
			pair.second->release(mDevice->logicalDevice);
			delete pair.second;
		}
		mShaderBoundStateCache.clear();

		if (mImmediateCommandList)
		{
			delete mImmediateCommandList;
			mImmediateCommandList = nullptr;
		}

		VulkanDynamicBufferManager::Get().cleanup();

		if (mSwapChain )
		{
			mSwapChain->cleanupResource();
			delete mSwapChain;
		}

		VK_SAFE_DESTROY(mWindowSurface, vkDestroySurfaceKHR(mInstance, mWindowSurface, gAllocCB));

		if (mDevice)
		{
			delete mDevice;
		}

		if (mCallback != VK_NULL_HANDLE)
		{
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
			func(mInstance, mCallback, gAllocCB);
			mCallback = VK_NULL_HANDLE;
		}
		VK_SAFE_DESTROY(mInstance, vkDestroyInstance(mInstance, gAllocCB));
	}

#if SYS_PLATFORM_WIN
	VkSurfaceKHR VulkanSystem::createWindowSurface(HWND hWnd)
	{
		VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
		surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceInfo.hwnd = hWnd;
		surfaceInfo.hinstance = GetModuleHandleA(NULL);
		VkSurfaceKHR result;
		VK_VERIFY_FAILCODE(vkCreateWin32SurfaceKHR(mInstance, &surfaceInfo, gAllocCB, &result), return VK_NULL_HANDLE;);
		return result;
	}
#endif

	class ShaderFormat* VulkanSystem::createShaderFormat()
	{
		auto result = new ShaderFormatSpirv(mDevice->logicalDevice);
		return result;
	}

	RHIProfileCore* VulkanSystem::createProfileCore()
	{
		if (mProfileCore == nullptr)
		{
			double cycleToMillisecond = (double)mDevice->properties.limits.timestampPeriod / 1e6;
			mProfileCore = new VulkanProfileCore;
			mProfileCore->init(mDevice, cycleToMillisecond);
		}
		return mProfileCore;
	}

	bool VulkanSystem::createSwapChainRenderPass()
	{
		VkAttachmentDescription colorAttachmentDesc = {};
		colorAttachmentDesc.format = mSwapChain->mImageFormat;
		colorAttachmentDesc.samples = VulkanTranslate::ToSampleCount(mSwapChain->mNumSamples);
		colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference resolveAttachmentRef = {};
		resolveAttachmentRef.attachment = 1;
		resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription attachments[3] = {};
		attachments[0] = colorAttachmentDesc;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.colorAttachmentCount = 1;

		uint32 attachmentCount = 1;

		if (mSwapChain->mNumSamples > 1)
		{
			// Attachment 1 will be resolve color
			VkAttachmentDescription resolveAttachmentDesc = colorAttachmentDesc;
			resolveAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
			resolveAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			resolveAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			resolveAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			resolveAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			// Update original color attachment to not present, as it will be resolved
			attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			attachments[1] = resolveAttachmentDesc;
			subpass.pResolveAttachments = &resolveAttachmentRef;
			attachmentCount = 2;
		}

		uint32 depthAttachmentIndex = attachmentCount;
		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = depthAttachmentIndex;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		if (mSwapChain->mDepthFormat != VK_FORMAT_UNDEFINED)
		{
			VkAttachmentDescription depthAttachmentDesc = {};
			depthAttachmentDesc.format = mSwapChain->mDepthFormat;
			depthAttachmentDesc.samples = VulkanTranslate::ToSampleCount(mNumSamples);
			depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			attachments[depthAttachmentIndex] = depthAttachmentDesc;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;
			attachmentCount++;
		}

		VkSubpassDependency dependencies[2] = {};
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = 0;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = 0;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pAttachments = attachments;
		renderPassInfo.attachmentCount = attachmentCount;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pDependencies = dependencies;
		renderPassInfo.dependencyCount = ARRAY_SIZE(dependencies);

		VK_VERIFY_RETURN_FALSE(vkCreateRenderPass(mDevice->logicalDevice, &renderPassInfo, gAllocCB, &mSwapChainRenderPass));
		return true;
	}

	bool VulkanSystem::createSwapChainFramebuffers()
	{
		mSwapChainFramebuffers.resize(mSwapChain->mImageViews.size());

		for (int i = 0; i < mSwapChain->mImageViews.size(); ++i)
		{
			VkImageView attachments[3];
			uint32 attachmentCount = 0;
			if (mSwapChain->mNumSamples > 1)
			{
				attachments[attachmentCount++] = mSwapChain->mMSAAColorView;
				attachments[attachmentCount++] = mSwapChain->mImageViews[i]; // Resolve target
			}
			else
			{
				attachments[attachmentCount++] = mSwapChain->mImageViews[i];
			}

			if (mSwapChain->mDepthView != VK_NULL_HANDLE)
			{
				attachments[attachmentCount++] = mSwapChain->mDepthView;
			}

			VkFramebufferCreateInfo frameBufferInfo = FVulkanInit::framebufferCreateInfo();
			frameBufferInfo.renderPass = mSwapChainRenderPass;
			frameBufferInfo.pAttachments = attachments;
			frameBufferInfo.attachmentCount = attachmentCount;
			frameBufferInfo.width = mSwapChain->mImageSize.width;
			frameBufferInfo.height = mSwapChain->mImageSize.height;
			frameBufferInfo.layers = 1;

			VK_VERIFY_RETURN_FALSE(vkCreateFramebuffer(mDevice->logicalDevice, &frameBufferInfo, gAllocCB, &mSwapChainFramebuffers[i]));
		}

		mRenderFinishedSemaphores.resize(mSwapChain->mImageViews.size());
		for (int i = 0; i < mRenderFinishedSemaphores.size(); ++i)
		{
			mRenderFinishedSemaphores[i] = createSemphore();
		}

		return true;
	}

	void VulkanSystem::cleanupSwapChainRenderPass()
	{
		for (auto sem : mRenderFinishedSemaphores)
		{
			if (sem != VK_NULL_HANDLE)
				vkDestroySemaphore(mDevice->logicalDevice, sem, gAllocCB);
		}
		mRenderFinishedSemaphores.clear();

		for (auto fb : mSwapChainFramebuffers)
		{
			if (fb != VK_NULL_HANDLE)
				vkDestroyFramebuffer(mDevice->logicalDevice, fb, gAllocCB);
		}
		mSwapChainFramebuffers.clear();

		VK_SAFE_DESTROY(mSwapChainRenderPass, vkDestroyRenderPass(mDevice->logicalDevice, mSwapChainRenderPass, gAllocCB));
	}
	void VulkanSystem::recreateSwapChain()
	{
		vkDeviceWaitIdle(mDevice->logicalDevice);
		cleanupSwapChainRenderPass();
		mSwapChain->cleanupResource();
		mSwapChain->initialize(*mDevice, mWindowSurface, mUsageQueueFamilyIndices, depthFormat, mNumSamples, false);
		createSwapChainRenderPass();
		createSwapChainFramebuffers();
	}

	bool VulkanSystem::RHIBeginRender()
	{
		if (mDrawContext.mActiveCmdBuffer != VK_NULL_HANDLE)
		{
			return true;
		}

		vkWaitForFences(mDevice->logicalDevice, 1, &mInFlightFences[mCurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(mDevice->logicalDevice, 1, &mInFlightFences[mCurrentFrame]);

		if (mFrameCount >= MAX_FRAMES_IN_FLIGHT)
		{
			VulkanFenceResourceManager::Get().releaseFence(mFrameCount - MAX_FRAMES_IN_FLIGHT);
		}

		VulkanDynamicBufferManager::Get().markFence();
		VkResult result = vkAcquireNextImageKHR(mDevice->logicalDevice, mSwapChain->mSwapChain, std::numeric_limits<uint64_t>::max(), mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &mRenderImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return RHIBeginRender();
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			return false;
		}

		mDrawContext.mDescriptorPoolManager.resetFrame(mCurrentFrame);
		mDrawContext.mBoundPipeline = VK_NULL_HANDLE;
		mDrawContext.mActiveFrameBuffer = nullptr;
		mDrawContext.mActiveRenderPass = VK_NULL_HANDLE;
		mDrawContext.mInsideRenderPass = false;
		mDrawContext.mDescriptorDirty = true;

		mDrawContext.mActiveCmdBuffer = mCommandBuffers[mCurrentFrame];
		mDrawContext.setAsActive();

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(mDrawContext.mActiveCmdBuffer, &beginInfo);

		// Set up swap chain framebuffer for current frame
		if (mSwapChainRenderPass != VK_NULL_HANDLE && mRenderImageIndex < mSwapChainFramebuffers.size())
		{
			mSwapChainFrameBuffer.mRenderPass = mSwapChainRenderPass;
			mSwapChainFrameBuffer.mFramebuffer = mSwapChainFramebuffers[mRenderImageIndex];
			mSwapChainFrameBuffer.mWidth = mSwapChain->mImageSize.width;
			mSwapChainFrameBuffer.mHeight = mSwapChain->mImageSize.height;
			mSwapChainFrameBuffer.mDevice = mDevice;
			mSwapChainFrameBuffer.mNumSamples = mNumSamples;

			if (mSwapChain->mDepthView != VK_NULL_HANDLE)
			{
				mSwapChainFrameBuffer.mDepthBuffer.view = mSwapChain->mDepthView;
				mSwapChainFrameBuffer.mDepthBuffer.format = mSwapChain->mDepthFormat;
				mSwapChainFrameBuffer.mDepthBuffer.samples = VulkanTranslate::ToSampleCount(mNumSamples);
				mSwapChainFrameBuffer.mDepthBuffer.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				mSwapChainFrameBuffer.mDepthBuffer.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			}
			else
			{
				mSwapChainFrameBuffer.mDepthBuffer.view = VK_NULL_HANDLE;
			}

			mSwapChainFrameBuffer.bPipelineLayoutDirty = false;
			mSwapChainFrameBuffer.bBufferDirty = false;

			if (mProfileCore)
			{
				mProfileCore->onBeginRender(mDrawContext);
			}

			mDrawContext.setSwapChainFrameBuffer(&mSwapChainFrameBuffer);
			mDrawContext.RHISetFrameBuffer(nullptr);
		}
		else
		{
			if (mProfileCore)
			{
				mProfileCore->onBeginRender(mDrawContext);
			}
		}
		return true;
	}

	void VulkanSystem::RHIEndRender(bool bPresent)
	{
		if (mDrawContext.mActiveCmdBuffer == VK_NULL_HANDLE)
		{
			return;
		}

		// If the command buffer was started in RHIBeginRender, its handle will match mCommandBuffers[mCurrentFrame]
		// If it was set from external (Editor), we don't end it here as the external manager handles it.
		if (mDrawContext.mActiveCmdBuffer != mCommandBuffers[mCurrentFrame])
		{
			return;
		}

		// End any active render pass before finishing the command buffer
		mDrawContext.endRenderPass();

		if (mProfileCore)
		{
			mProfileCore->onEndRender(mDrawContext);
		}

		vkEndCommandBuffer(mDrawContext.mActiveCmdBuffer);


		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pCommandBuffers = &mDrawContext.mActiveCmdBuffer;
		submitInfo.commandBufferCount = 1;

		VkSemaphore waitSemaphores[] = { mImageAvailableSemaphores[mCurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.waitSemaphoreCount = ARRAY_SIZE(waitSemaphores);

		VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphores[mRenderImageIndex] };
		submitInfo.pSignalSemaphores = signalSemaphores;
		submitInfo.signalSemaphoreCount = ARRAY_SIZE(signalSemaphores);

		VK_VERIFY_FAILCODE(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFences[mCurrentFrame]), );

		if (bPresent)
		{
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.waitSemaphoreCount = ARRAY_SIZE(signalSemaphores);
			presentInfo.pWaitSemaphores = signalSemaphores;

			VkSwapchainKHR swapChains[] = { mSwapChain->mSwapChain };
			presentInfo.pSwapchains = swapChains;
			presentInfo.swapchainCount = ARRAY_SIZE(swapChains);

			presentInfo.pImageIndices = &mRenderImageIndex;
			presentInfo.pResults = nullptr;
			VkResult result = vkQueuePresentKHR(mPresentQueue, &presentInfo);
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			{
				recreateSwapChain();
			}
			else if (result != VK_SUCCESS)
			{
				LogWarning(0, "vkQueuePresentKHR failed : %s", FVulkanUtilies::GetResultErrorString(result));
			}
		}

		mFrameCount++;
		VulkanFenceResourceManager::Get().mFenceValue = mFrameCount;

		mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		mDrawContext.mActiveCmdBuffer = VK_NULL_HANDLE;
	}

	RHITexture1D* VulkanSystem::RHICreateTexture1D(TextureDesc const& desc, void* data)
	{
		VulkanTexture1D* texture = new VulkanTexture1D(desc);
		if (!initializeTextureInternal(texture, desc, data, 0))
		{
			delete texture;
			return nullptr;
		}
		return texture;
	}

	RHITexture2D*  VulkanSystem::RHICreateTexture2D(TextureDesc const& desc, void* data, int dataAlign)
	{
		VulkanTexture2D* texture = nullptr;

		//#TODO:
		if (desc.format == ETexture::RGB8)
		{
			TArray< uint8 > tempData;

			int pixelLength = desc.dimension.x * desc.dimension.y;
			tempData.resize(pixelLength * sizeof(uint32));

			uint8* dest = tempData.data();
			uint8* src = (uint8*)data;

			for (int i = 0; i < pixelLength; ++i)
			{
				dest[0] = src[0];
				dest[1] = src[1];
				dest[2] = src[2];
				dest[3] = 0xff;
				dest += 4;
				src += 3;
			}

			TextureDesc tempDesc = desc;
			tempDesc.format = ETexture::RGBA8;
			texture = new VulkanTexture2D(tempDesc);
			if (!initializeTextureInternal(texture, tempDesc, tempData.data(), dataAlign))
			{
				delete texture;
				return nullptr;
			}
		}
		else
		{
			texture = new VulkanTexture2D(desc);
			if (!initializeTextureInternal(texture, desc, data, dataAlign))
			{
				delete texture;
				return nullptr;
			}
		}
		return texture;
	}

	RHITexture3D* VulkanSystem::RHICreateTexture3D(TextureDesc const& desc, void* data)
	{
		VulkanTexture3D* texture = new VulkanTexture3D(desc);
		if (!initializeTextureInternal(texture, desc, data, 0))
		{
			delete texture;
			return nullptr;
		}
		return texture;
	}

	RHITextureCube* VulkanSystem::RHICreateTextureCube(TextureDesc const& desc, void* data[])
	{
		// For cube textures, data[] is an array of 6 face pointers
		// We combine them into a single buffer for upload
		void* combinedData = nullptr;
		TArray< uint8 > combinedBuffer;
		if (data != nullptr)
		{
			uint32 faceSize = desc.dimension.x * desc.dimension.y * ETexture::GetFormatSize(desc.format);
			combinedBuffer.resize(faceSize * 6);
			for (int face = 0; face < 6; ++face)
			{
				if (data[face])
				{
					memcpy(combinedBuffer.data() + face * faceSize, data[face], faceSize);
				}
			}
			combinedData = combinedBuffer.data();
		}

		VulkanTextureCube* texture = new VulkanTextureCube(desc);
		if (!initializeTextureInternal(texture, desc, combinedData, 0))
		{
			delete texture;
			return nullptr;
		}
		return texture;
	}

	RHITexture2DArray* VulkanSystem::RHICreateTexture2DArray(TextureDesc const& desc, void* data)
	{
		VulkanTexture2DArray* texture = new VulkanTexture2DArray(desc);
		if (!initializeTextureInternal(texture, desc, data, 0))
		{
			delete texture;
			return nullptr;
		}
		return texture;
	}


	bool VulkanSystem::initializeTextureInternal(VulkanTexture* texture, TextureDesc const& desc, void* data, int alignment)
	{
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkCommandPool commandPool = mGraphicsCommandPool;
		VkQueue copyQueue = mGraphicsQueue;

		texture->mDevice = mDevice->logicalDevice;
		texture->mFormatVK = VulkanTranslate::To(desc.format);
		texture->mImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		int mipLevels = desc.numMipLevel;
		if (desc.creationFlags & TCF_GenerateMips && mipLevels <= 1)
		{
			mipLevels = 0;
			int maxDim = Math::Max(desc.dimension.x, desc.dimension.y);
			while (maxDim > 0) { maxDim >>= 1; mipLevels++; }
		}
		texture->mMipLevels = mipLevels;

		VkFormat formatVK = VulkanTranslate::To(desc.format);
		VkImageUsageFlags imageUsageFlags = FVulkanTexture::TranslateUsage(desc.creationFlags);

		// Determine image type, extent, array layers, and flags based on texture type
		VkImageType imageType;
		VkExtent3D extent;
		uint32 arrayLayers = 1;
		VkImageCreateFlags imageFlags = 0;

		switch (desc.type)
		{
		case ETexture::Type1D:
			imageType = VK_IMAGE_TYPE_1D;
			extent = { uint32(desc.dimension.x), 1, 1 };
			break;
		case ETexture::Type3D:
			imageType = VK_IMAGE_TYPE_3D;
			extent = { uint32(desc.dimension.x), uint32(desc.dimension.y), uint32(desc.dimension.z) };
			break;
		case ETexture::TypeCube:
			imageType = VK_IMAGE_TYPE_2D;
			extent = { uint32(desc.dimension.x), uint32(desc.dimension.y), 1 };
			arrayLayers = 6;
			imageFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			break;
		case ETexture::Type2DArray:
			imageType = VK_IMAGE_TYPE_2D;
			extent = { uint32(desc.dimension.x), uint32(desc.dimension.y), 1 };
			arrayLayers = uint32(desc.dimension.z);
			break;
		case ETexture::Type2D:
		default:
			imageType = VK_IMAGE_TYPE_2D;
			extent = { uint32(desc.dimension.x), uint32(desc.dimension.y), 1 };
			break;
		}
		texture->mArrayLayers = arrayLayers;
		texture->mNumSamples = desc.numSamples;

		// Get device properties for the requested texture format
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(mDevice->physicalDevice, formatVK, &formatProperties);

		VkMemoryAllocateInfo memAlloc = FVulkanInit::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		// Use a separate command buffer for texture loading
		VkCommandBuffer copyCmd = mDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, commandPool, true);

		// Calculate total data size
		uint32 textureDataSize = desc.dimension.x * desc.dimension.y * ETexture::GetFormatSize(desc.format);
		if (desc.type == ETexture::Type3D)
		{
			textureDataSize *= desc.dimension.z;
		}
		else if (desc.type == ETexture::TypeCube)
		{
			textureDataSize *= 6;
		}
		else if (desc.type == ETexture::Type2DArray)
		{
			textureDataSize *= desc.dimension.z;
		}
		else if (desc.type == ETexture::Type1D)
		{
			textureDataSize = desc.dimension.x * ETexture::GetFormatSize(desc.format);
		}

		{
			// Create optimal tiled target image
			VkImageCreateInfo imageCreateInfo = FVulkanInit::imageCreateInfo();
			imageCreateInfo.imageType = imageType;
			imageCreateInfo.format = formatVK;
			imageCreateInfo.mipLevels = mipLevels;
			imageCreateInfo.arrayLayers = arrayLayers;
			imageCreateInfo.samples = VulkanTranslate::ToSampleCount(desc.numSamples);
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = extent;
			imageCreateInfo.usage = imageUsageFlags;
			imageCreateInfo.flags = imageFlags;
			// Ensure that the TRANSFER_DST bit is set for staging
			if (data != nullptr && !(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
			{
				imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}

			VERIFY_RETURN_FALSE(texture->image.initialize(mDevice->logicalDevice, imageCreateInfo));

			vkGetImageMemoryRequirements(mDevice->logicalDevice, texture->image, &memReqs);

			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = mDevice->getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (memAlloc.memoryTypeIndex == INDEX_NONE)
				return false;

			VERIFY_RETURN_FALSE(texture->deviceMemory.initialize(mDevice->logicalDevice, memAlloc));
			VK_CHECK_RESULT(vkBindImageMemory(mDevice->logicalDevice, texture->image, texture->deviceMemory, 0));

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = mipLevels;
			subresourceRange.layerCount = arrayLayers;

			texture->mImageLayout = imageLayout;

			if (data != nullptr)
			{
				// Create a host-visible staging buffer that contains the raw image data
				VkBuffer stagingBuffer;
				VkDeviceMemory stagingMemory;

				VkBufferCreateInfo bufferCreateInfo = FVulkanInit::bufferCreateInfo();
				bufferCreateInfo.size = textureDataSize;
				// This buffer is used as a transfer source for the buffer copy
				bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				VK_CHECK_RESULT(vkCreateBuffer(mDevice->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));

				// Get memory requirements for the staging buffer (alignment, memory type bits)
				vkGetBufferMemoryRequirements(mDevice->logicalDevice, stagingBuffer, &memReqs);

				memAlloc.allocationSize = memReqs.size;
				// Get memory type index for a host visible buffer
				memAlloc.memoryTypeIndex = mDevice->getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				if (memAlloc.memoryTypeIndex == INDEX_NONE)
					return false;

				VK_CHECK_RESULT(vkAllocateMemory(mDevice->logicalDevice, &memAlloc, nullptr, &stagingMemory));
				VK_CHECK_RESULT(vkBindBufferMemory(mDevice->logicalDevice, stagingBuffer, stagingMemory, 0));

				// Copy texture data into staging buffer
				uint8_t *pStageData;
				VK_CHECK_RESULT(vkMapMemory(mDevice->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&pStageData));
				memcpy(pStageData, data, textureDataSize);
				vkUnmapMemory(mDevice->logicalDevice, stagingMemory);

				// Setup buffer copy regions
				TArray<VkBufferImageCopy> bufferCopyRegions;

				if (desc.type == ETexture::TypeCube || desc.type == ETexture::Type2DArray)
				{
					uint32 faceSize = desc.dimension.x * desc.dimension.y * ETexture::GetFormatSize(desc.format);
					for (uint32 layer = 0; layer < arrayLayers; ++layer)
					{
						VkBufferImageCopy bufferCopyRegion = {};
						bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						bufferCopyRegion.imageSubresource.mipLevel = 0;
						bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
						bufferCopyRegion.imageSubresource.layerCount = 1;
						bufferCopyRegion.imageExtent.width = std::max(1u, extent.width);
						bufferCopyRegion.imageExtent.height = std::max(1u, extent.height);
						bufferCopyRegion.imageExtent.depth = 1;
						bufferCopyRegion.bufferOffset = layer * faceSize;
						bufferCopyRegions.push_back(bufferCopyRegion);
					}
				}
				else
				{
					VkBufferImageCopy bufferCopyRegion = {};
					bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					bufferCopyRegion.imageSubresource.mipLevel = 0;
					bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
					bufferCopyRegion.imageSubresource.layerCount = 1;
					bufferCopyRegion.imageExtent = extent;
					bufferCopyRegion.bufferOffset = 0;
					bufferCopyRegions.push_back(bufferCopyRegion);
				}

				// Image barrier for optimal image (target)
				// Optimal image will be used as destination for the copy
				FVulkanTexture::SetImageLayout(
					copyCmd,
					texture->image,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					subresourceRange);

				// Copy mip levels from staging buffer
				vkCmdCopyBufferToImage(
					copyCmd,
					stagingBuffer,
					texture->image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					static_cast<uint32>(bufferCopyRegions.size()),
					bufferCopyRegions.data()
				);

				if ((desc.creationFlags & TCF_GenerateMips) && mipLevels > 1)
				{
					FVulkanTexture::GenerateMipmaps(copyCmd, texture->image, extent, mipLevels, arrayLayers, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout);
				}
				else
				{
					// Change texture image layout to shader read after all mip levels have been copied
					FVulkanTexture::SetImageLayout(
						copyCmd,
						texture->image,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						imageLayout,
						subresourceRange);
				}

				mDevice->flushCommandBuffer(copyCmd, copyQueue, commandPool);

				// Clean up staging resources
				vkFreeMemory(mDevice->logicalDevice, stagingMemory, nullptr);
				vkDestroyBuffer(mDevice->logicalDevice, stagingBuffer, nullptr);
			}
			else
			{
				// No data provided, just transition to the appropriate layout
				FVulkanTexture::SetImageLayout(
					copyCmd,
					texture->image,
					VK_IMAGE_LAYOUT_UNDEFINED,
					imageLayout,
					subresourceRange);

				mDevice->flushCommandBuffer(copyCmd, copyQueue, commandPool);
			}
		}

		// Create image view
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = texture->mViewType;
		viewCreateInfo.format = formatVK;
		if (formatVK == VK_FORMAT_R8_UNORM || formatVK == VK_FORMAT_R16_UNORM)
		{
			viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
		}
		else
		{
			viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		}
		viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, arrayLayers };
		viewCreateInfo.subresourceRange.levelCount = mipLevels;
		viewCreateInfo.image = texture->image;

		VERIFY_RETURN_FALSE(texture->view.initialize(mDevice->logicalDevice, viewCreateInfo));

		return true;
	}

	RHIBuffer* VulkanSystem::RHICreateBuffer(BufferDesc const& desc, void* data)
	{
		VulkanBuffer* buffer = new VulkanBuffer;
		if (!initalizeBufferInternal(buffer, desc, data))
		{
			delete buffer;
			return nullptr;
		}
		return buffer;
	}

	bool VulkanSystem::initalizeBufferInternal(VulkanBuffer* buffer, BufferDesc const& desc, void* data)
	{
		buffer->mDesc = desc;

		VkDeviceSize bufferSize = desc.elementSize * desc.numElements;

		VERIFY_RETURN_FALSE( mDevice->createBuffer(
			FVulkanBuffer::TranslateUsage(desc.creationFlags),
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			buffer->getBufferData(), bufferSize, data) );

		return true;
	}

	void* VulkanSystem::RHILockBuffer(RHIBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		auto& bufferImpl = VulkanCast::To(*buffer);
		if (access == ELockAccess::WriteDiscard || access == ELockAccess::WriteOnly)
		{
			if (bufferImpl.mbIsDynamic)
			{
				auto buddyInfo = bufferImpl.mAllocation.buddyInfo;
				VulkanFenceResourceManager::Get().addFuncRelease(
					[buddyInfo]()
					{
						VulkanDynamicBufferManager::Get().dealloc(buddyInfo);
					}
				);
			}

			uint32 sizeToLock = (size == 0) ? bufferImpl.getDesc().getSize() : size;
			VulkanBufferAllocation allocation;
			if (!VulkanDynamicBufferManager::Get().alloc(sizeToLock, 16, allocation))
				return nullptr;
			bufferImpl.updateAllocation(allocation);
			return allocation.cpuAddress + offset;
		}

		VkDeviceSize mapSize = (size == 0) ? VK_WHOLE_SIZE : size;
		VkResult result = bufferImpl.getBufferData().map(mapSize, offset);
		if (result != VK_SUCCESS)
			return nullptr;
		return (uint8*)bufferImpl.getBufferData().mapped;
	}


	bool VulkanSystem::RHIUpdateTexture(RHITexture2D& texture, int ox, int oy, int w, int h, void* data, int level, int dataWidth)
	{
		auto& textureImpl = static_cast<VulkanTexture2D&>(texture);
		
		int pixelSize = ETexture::GetFormatSize(texture.getDesc().format);
		int srcRowPitch = (dataWidth ? dataWidth : w) * pixelSize;
		int copyPitch = w * pixelSize;
		uint32 textureDataSize = h * copyPitch;

		VkMemoryAllocateInfo memAlloc = FVulkanInit::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;
		VkBufferCreateInfo bufferCreateInfo = FVulkanInit::bufferCreateInfo();
		bufferCreateInfo.size = textureDataSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_VERIFY_RETURN_FALSE(vkCreateBuffer(mDevice->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));

		vkGetBufferMemoryRequirements(mDevice->logicalDevice, stagingBuffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = mDevice->getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VK_CHECK_RESULT(vkAllocateMemory(mDevice->logicalDevice, &memAlloc, nullptr, &stagingMemory));
		VK_CHECK_RESULT(vkBindBufferMemory(mDevice->logicalDevice, stagingBuffer, stagingMemory, 0));

		uint8_t *pStageData;
		VK_CHECK_RESULT(vkMapMemory(mDevice->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&pStageData));
		
		if (srcRowPitch == copyPitch)
		{
			memcpy(pStageData, data, textureDataSize);
		}
		else
		{
			uint8_t* srcRow = (uint8_t*)data;
			uint8_t* dstRow = pStageData;
			for (int i = 0; i < h; ++i)
			{
				memcpy(dstRow, srcRow, copyPitch);
				srcRow += srcRowPitch;
				dstRow += copyPitch;
			}
		}
		vkUnmapMemory(mDevice->logicalDevice, stagingMemory);

		VkCommandPool commandPool = mGraphicsCommandPool;
		VkQueue copyQueue = mGraphicsQueue;
		VkCommandBuffer copyCmd = mDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, commandPool, true);

		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = level;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageOffset = { (int32)ox, (int32)oy, 0 };
		bufferCopyRegion.imageExtent.width = w;
		bufferCopyRegion.imageExtent.height = h;
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = 0;

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = level;
		subresourceRange.levelCount = 1;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.layerCount = 1;

		FVulkanTexture::SetImageLayout(
			copyCmd,
			textureImpl.image,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange);

		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer,
			textureImpl.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferCopyRegion
		);

		FVulkanTexture::SetImageLayout(
			copyCmd,
			textureImpl.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			subresourceRange);

		mDevice->flushCommandBuffer(copyCmd, copyQueue, commandPool);

		vkFreeMemory(mDevice->logicalDevice, stagingMemory, nullptr);
		vkDestroyBuffer(mDevice->logicalDevice, stagingBuffer, nullptr);

		return true;
	}

	RHIInputLayout* VulkanSystem::RHICreateInputLayout(InputLayoutDesc const& desc)
	{
		return new VulkanInputLayout(desc);
	}

	RHISamplerState* VulkanSystem::RHICreateSamplerState(SamplerStateInitializer const& initializer)
	{
		VulkanSamplerState* samplerState = new VulkanSamplerState;
		if (!initializeSamplerStateInternal(samplerState, initializer))
		{
			delete samplerState;
			return nullptr;
		}
		return samplerState;
	}

	RHIFrameBuffer* VulkanSystem::RHICreateFrameBuffer()
	{
		VulkanFrameBuffer* frameBuffer = new VulkanFrameBuffer;
		frameBuffer->mDevice = mDevice;
		return frameBuffer;
	}

	void VulkanSystem::cleanupPipelineCache()
	{
		mDrawContext.release();
	}

	VulkanFrameHeapAllocator::~VulkanFrameHeapAllocator()
	{
		if (mCurrentPage.buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(mDevice->logicalDevice, mCurrentPage.buffer, gAllocCB);
			vkFreeMemory(mDevice->logicalDevice, mCurrentPage.memory, gAllocCB);
		}
		for (auto& page : mFreePages)
		{
			vkDestroyBuffer(mDevice->logicalDevice, page.buffer, gAllocCB);
			vkFreeMemory(mDevice->logicalDevice, page.memory, gAllocCB);
		}
		for (auto& page : mUsedPages)
		{
			vkDestroyBuffer(mDevice->logicalDevice, page.buffer, gAllocCB);
			vkFreeMemory(mDevice->logicalDevice, page.memory, gAllocCB);
		}
	}

	bool VulkanFrameHeapAllocator::alloc(uint32 size, uint32 alignment, VulkanBufferAllocation& outAllocation)
	{
		uint32 allocSize = size;
		if (alignment && (mSizeUsage % alignment) != 0)
		{
			allocSize = size + alignment - mSizeUsage % alignment;
		}

		if (allocSize > mResourceSize)
			return false;

		bool bNeedNewPage = false;
		if (mCurrentPage.buffer == VK_NULL_HANDLE)
		{
			bNeedNewPage = true;
		}
		else if (mSizeUsage + allocSize > mResourceSize)
		{
			mUsedPages.push_back(mCurrentPage);
			mCurrentPage.buffer = VK_NULL_HANDLE;
			mCurrentPage.mappedAddr = nullptr;
			mSizeUsage = 0;
			bNeedNewPage = true;
		}

		if (bNeedNewPage)
		{
			{
				RWLock::WriteLocker locker(mLock);
				if (mFreePages.size())
				{
					mCurrentPage = mFreePages.back();
					mFreePages.pop_back();
				}
			}

			if (mCurrentPage.buffer == VK_NULL_HANDLE)
			{
				if (!createNewPage(mCurrentPage))
					return false;
			}
			mSizeUsage = 0;

			if (alignment && (mSizeUsage % alignment) != 0)
			{
				allocSize = size + alignment - mSizeUsage % alignment;
			}
			else
			{
				allocSize = size;
			}
		}

		uint32 pos = mSizeUsage;
		if (alignment && (mSizeUsage % alignment) != 0)
		{
			pos = AlignArbitrary<uint32>(pos, alignment);
		}

		outAllocation.buffer = mCurrentPage.buffer;
		outAllocation.offset = pos;
		outAllocation.cpuAddress = mCurrentPage.mappedAddr + pos;
		outAllocation.size = size;
		mSizeUsage += allocSize;
		return true;
	}

	void VulkanFrameHeapAllocator::markFence()
	{
		if (mCurrentPage.buffer != VK_NULL_HANDLE && mSizeUsage > 0)
		{
			mUsedPages.push_back(mCurrentPage);
			mCurrentPage.buffer = VK_NULL_HANDLE;
			mCurrentPage.mappedAddr = nullptr;
			mSizeUsage = 0;
		}

		for (auto& page : mUsedPages)
		{
			VulkanFenceResourceManager::Get().addFuncRelease(
				[this, page]()
				{
					RWLock::WriteLocker locker(mLock);
					mFreePages.push_back(page);
				}
			);
		}
		mUsedPages.clear();
	}

	bool VulkanFrameHeapAllocator::createNewPage(Page& outPage)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = mResourceSize;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_VERIFY_RETURN_FALSE(vkCreateBuffer(mDevice->logicalDevice, &bufferInfo, gAllocCB, &outPage.buffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(mDevice->logicalDevice, outPage.buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = mDevice->getMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VK_VERIFY_RETURN_FALSE(vkAllocateMemory(mDevice->logicalDevice, &allocInfo, gAllocCB, &outPage.memory));
		VK_VERIFY_RETURN_FALSE(vkBindBufferMemory(mDevice->logicalDevice, outPage.buffer, outPage.memory, 0));

		vkMapMemory(mDevice->logicalDevice, outPage.memory, 0, mResourceSize, 0, (void**)&outPage.mappedAddr);
		return true;
	}

	void VulkanDynamicBufferManager::cleanup()
	{
		for (auto page : mHeapPages)
		{
			page->cleanup();
			delete page;
		}
		mHeapPages.clear();
		for (auto allocator : mFrameAllocators)
		{
			delete allocator;
		}
		mFrameAllocators.clear();
		mDevice = nullptr;
	}

	void VulkanDynamicBufferManager::markFence()
	{
		for (auto allocator : mFrameAllocators)
		{
			allocator->markFence();
		}
	}

	bool VulkanDynamicBufferManager::addFrameAllocator(uint32 size)
	{
		if (size < 64 * 1024)
			size = 64 * 1024;
		VulkanFrameHeapAllocator* allocator = new VulkanFrameHeapAllocator();
		allocator->initialize(mDevice, size);
		mFrameAllocators.push_back(allocator);
		return true;
	}

	bool VulkanDynamicBufferManager::alloc(uint32 size, uint32 alignment, VulkanBufferAllocation& outAllocation)
	{
		for (auto page : mHeapPages)
		{
			if (page->alloc(size, alignment, outAllocation))
				return true;
		}

		uint32 newPageSize = 1024 * 64;
		while (newPageSize < size)
		{
			newPageSize *= 2;
		};

		VkBuffer buffer;
		VkDeviceMemory memory;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = newPageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_VERIFY_RETURN_FALSE(vkCreateBuffer(mDevice->logicalDevice, &bufferInfo, gAllocCB, &buffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(mDevice->logicalDevice, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = mDevice->getMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VK_VERIFY_RETURN_FALSE(vkAllocateMemory(mDevice->logicalDevice, &allocInfo, gAllocCB, &memory));
		VK_VERIFY_RETURN_FALSE(vkBindBufferMemory(mDevice->logicalDevice, buffer, memory, 0));

		VulkanUploadHeapPage* newPage = new VulkanUploadHeapPage;
		newPage->initialize(mDevice->logicalDevice, newPageSize, buffer, memory);

		mHeapPages.push_back(newPage);
		return newPage->alloc(size, alignment, outAllocation);
	}

	bool VulkanDynamicBufferManager::allocFrame(uint32 size, uint32 alignment, VulkanBufferAllocation& outAllocation)
	{
		for (auto allocator : mFrameAllocators)
		{
			if (allocator->alloc(size, alignment, outAllocation))
				return true;
		}

		if (!addFrameAllocator(size))
			return false;

		if (!mFrameAllocators.back()->alloc(size, alignment, outAllocation))
			return false;

		return true;
	}

	void VulkanDynamicBufferManager::dealloc(VulkanBuddyAllocationInfo const& info)
	{
		if (info.page)
		{
			info.page->dealloc(info);
		}
	}

	bool VulkanSwapChain::initialize(VulkanDevice& device, VkSurfaceKHR surface, uint32 const usageQueueFamilyIndices[], VkFormat depthFormat, int numSamples, bool bVSync)
	{
		mDevice = &device;
		mNumSamples = numSamples;

		//swap chain
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physicalDevice, surface, &capabilities);
		TArray<VkSurfaceFormatKHR> formats = GetEnumValues(vkGetPhysicalDeviceSurfaceFormatsKHR, device.physicalDevice, surface);
		TArray<VkPresentModeKHR> presentModes = GetEnumValues(vkGetPhysicalDeviceSurfacePresentModesKHR, device.physicalDevice, surface);

		auto  ChooseSwapSurfaceFormat = [](TArray<VkSurfaceFormatKHR> const& availableFormats)
		{
			if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
			{
				return VkSurfaceFormatKHR{ VK_FORMAT_B8G8R8A8_UNORM,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
			}
			for (const auto& availableFormat : availableFormats)
			{
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
					availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					return availableFormat;
				}
			}
			return availableFormats[0];
		};
		auto  ChooseSwapPresentMode = [bVSync](TArray<VkPresentModeKHR> const& availablePresentModes)
		{
			VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
			if (!bVSync)
			{
				for (const auto& availablePresentMode : availablePresentModes)
				{
					if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
					{
						bestMode = availablePresentMode;
						break;
					}
					else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
					{
						bestMode = availablePresentMode;
					}
				}
			}
			return bestMode;
		};

		auto ChooseSwapExtent = [](const VkSurfaceCapabilitiesKHR& capabilities) -> VkExtent2D
		{
			return capabilities.currentExtent;
		};

		auto usageFormat = ChooseSwapSurfaceFormat(formats);

		mImageFormat = usageFormat.format;
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(presentModes);
		mImageSize = ChooseSwapExtent(capabilities);
		VkSwapchainCreateInfoKHR swapChainInfo = {};

		uint32 imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		{
			imageCount = capabilities.maxImageCount;
		}

		swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainInfo.surface = surface;
		swapChainInfo.minImageCount = imageCount;
		swapChainInfo.imageFormat = usageFormat.format;
		swapChainInfo.imageColorSpace = usageFormat.colorSpace;
		swapChainInfo.imageExtent = mImageSize;
		swapChainInfo.imageArrayLayers = 1;
		swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapChainInfo.preTransform = capabilities.currentTransform;
		swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainInfo.presentMode = presentMode;
		swapChainInfo.clipped = VK_TRUE;
		swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

		if (usageQueueFamilyIndices[EQueueFamily::Graphics] != usageQueueFamilyIndices[EQueueFamily::Present])
		{
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapChainInfo.pQueueFamilyIndices = usageQueueFamilyIndices;
			swapChainInfo.queueFamilyIndexCount = 2;
		}
		else
		{
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapChainInfo.pQueueFamilyIndices = nullptr;
			swapChainInfo.queueFamilyIndexCount = 0;
		}

		VK_VERIFY_RETURN_FALSE(vkCreateSwapchainKHR(device.logicalDevice, &swapChainInfo, gAllocCB, &mSwapChain));

		//mSwapChainImages = GetEnumValues(vkGetSwapchainImagesKHR, mDevice, mSwapChain);
		vkGetSwapchainImagesKHR(device.logicalDevice, mSwapChain, &imageCount, nullptr);
		mImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device.logicalDevice, mSwapChain, &imageCount, mImages.data());

		for (auto image : mImages)
		{
			VkImageViewCreateInfo imageViewInfo = {};
			imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewInfo.image = image;
			imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewInfo.format = mImageFormat;
			imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewInfo.subresourceRange.baseMipLevel = 0;
			imageViewInfo.subresourceRange.levelCount = 1;
			imageViewInfo.subresourceRange.baseArrayLayer = 0;
			imageViewInfo.subresourceRange.layerCount = 1;

			VkImageView imageView;
			VK_VERIFY_RETURN_FALSE(vkCreateImageView(device.logicalDevice, &imageViewInfo, gAllocCB, &imageView));
			mImageViews.push_back(imageView);
		}

		mDepthFormat = depthFormat;
		if (mDepthFormat != VK_FORMAT_UNDEFINED)
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = mImageSize.width;
			imageInfo.extent.height = mImageSize.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = mDepthFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.samples = VulkanTranslate::ToSampleCount(mNumSamples);
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VK_VERIFY_RETURN_FALSE(vkCreateImage(device.logicalDevice, &imageInfo, gAllocCB, &mDepthImage));

			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(device.logicalDevice, mDepthImage, &memReqs);

			VkMemoryAllocateInfo memAlloc = {};
			memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = device.getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_VERIFY_RETURN_FALSE(vkAllocateMemory(device.logicalDevice, &memAlloc, gAllocCB, &mDepthMemory));
			VK_VERIFY_RETURN_FALSE(vkBindImageMemory(device.logicalDevice, mDepthImage, mDepthMemory, 0));

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = mDepthImage;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = mDepthFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (mDepthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || mDepthFormat == VK_FORMAT_D24_UNORM_S8_UINT)
				viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VK_VERIFY_RETURN_FALSE(vkCreateImageView(device.logicalDevice, &viewInfo, gAllocCB, &mDepthView));
		}

		if (mNumSamples > 1)
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = mImageSize.width;
			imageInfo.extent.height = mImageSize.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = mImageFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
			imageInfo.samples = VulkanTranslate::ToSampleCount(mNumSamples);
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VK_VERIFY_RETURN_FALSE(vkCreateImage(device.logicalDevice, &imageInfo, gAllocCB, &mMSAAColorImage));

			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(device.logicalDevice, mMSAAColorImage, &memReqs);

			VkMemoryAllocateInfo memAlloc = {};
			memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = device.getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_VERIFY_RETURN_FALSE(vkAllocateMemory(device.logicalDevice, &memAlloc, gAllocCB, &mMSAAColorMemory));
			VK_VERIFY_RETURN_FALSE(vkBindImageMemory(device.logicalDevice, mMSAAColorImage, mMSAAColorMemory, 0));

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = mMSAAColorImage;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = mImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VK_VERIFY_RETURN_FALSE(vkCreateImageView(device.logicalDevice, &viewInfo, gAllocCB, &mMSAAColorView));
		}

		return true;
	}

	void VulkanSwapChain::cleanupResource()
	{
		if (!mImageViews.empty())
		{
			for (auto view : mImageViews)
			{
				vkDestroyImageView(mDevice->logicalDevice, view, gAllocCB);
			}
			mImageViews.clear();
		}
		mImages.clear();
		VK_SAFE_DESTROY(mDepthView, vkDestroyImageView(mDevice->logicalDevice, mDepthView, gAllocCB));
		VK_SAFE_DESTROY(mDepthImage, vkDestroyImage(mDevice->logicalDevice, mDepthImage, gAllocCB));
		VK_SAFE_DESTROY(mDepthMemory, vkFreeMemory(mDevice->logicalDevice, mDepthMemory, gAllocCB));

		VK_SAFE_DESTROY(mMSAAColorView, vkDestroyImageView(mDevice->logicalDevice, mMSAAColorView, gAllocCB));
		VK_SAFE_DESTROY(mMSAAColorImage, vkDestroyImage(mDevice->logicalDevice, mMSAAColorImage, gAllocCB));
		VK_SAFE_DESTROY(mMSAAColorMemory, vkFreeMemory(mDevice->logicalDevice, mMSAAColorMemory, gAllocCB));

		VK_SAFE_DESTROY(mSwapChain, vkDestroySwapchainKHR(mDevice->logicalDevice, mSwapChain, gAllocCB));
	}

}//namespace Render


#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif
