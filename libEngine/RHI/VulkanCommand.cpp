#include "VulkanCommand.h"

#include <vector>
#include "MarcoCommon.h"
#include "StdUtility.h"
#include "CoreShare.h"
#include "VulkanShaderCommon.h"

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

namespace Render
{

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
		uint32_t extCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
		if (extCount > 0)
		{
			std::vector<VkExtensionProperties> extensions(extCount);
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

	int32 VulkanDevice::getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
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

	bool VulkanDevice::getQueueFamilyIndex(VkQueueFlagBits queueFlags, uint32_t& outIndex )
	{
		// Dedicated queue for compute
		// Try to find a queue family index that supports compute but not graphics
		if (queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
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
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
			{
				if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					outIndex = i;
					return true;
				}
			}
		}

		// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
		{
			if (queueFamilyProperties[i].queueFlags & queueFlags)
			{
				outIndex = i;
				return true;
			}
		}

		return false;
	}

	bool VulkanDevice::createLogicalDevice(VkPhysicalDeviceFeatures inEnabledFeatures, std::vector<const char*> enabledExtensions, void* pNextChain, bool useSwapChain /*= true*/, TArrayView< uint32 const > const& usageQueueIndices )
	{
		// enableFeatures = inEnabledFeatures;
		// Desired queues need to be requested upon logical device creation
		// Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
		// requests different queue types
		const float defaultQueuePriority(0.0f);
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

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
		std::vector<const char*> deviceExtensions(enabledExtensions);
		if (useSwapChain)
		{
			// If the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
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
			deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}

		VK_VERIFY_RETURN_FALSE( vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice) );

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

	VkCommandPool VulkanDevice::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags /*= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT*/)
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
		std::vector<VkFormat> depthFormats =
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
		std::vector<VkExtensionProperties> availableExtensions = GetEnumValues(vkEnumerateInstanceExtensionProperties, nullptr);

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

		std::vector< VkPhysicalDevice > physicalDevices = GetEnumValues(vkEnumeratePhysicalDevices, mInstance);

		//Choice Device
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


		//TODO: EnabledFeatures
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
		std::vector< uint32 > uniqueQueueFamilies;
	
		uniqueQueueFamilies.push_back(mUsageQueueFamilyIndices[EQueueFamily::Graphics]);
		uniqueQueueFamilies.push_back(mUsageQueueFamilyIndices[EQueueFamily::Compute]);
		uniqueQueueFamilies.push_back(mUsageQueueFamilyIndices[EQueueFamily::Present]);
		if (mUsageQueueFamilyIndices[EQueueFamily::Transfer] != INDEX_NONE)
		{
			uniqueQueueFamilies.push_back(mUsageQueueFamilyIndices[EQueueFamily::Graphics]);
		}
		MakeValueUnique(uniqueQueueFamilies);

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
		if (!mSwapChain->initialize(*mDevice, mWindowSurface, mUsageQueueFamilyIndices, false))
		{
			return false;
		}

		VERIFY_RETURN_FALSE(initRenderResource());

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

	RHIShader* VulkanSystem::RHICreateShader(Shader::Type type)
	{

		return new VulkanShader;
	}

	RHIShaderProgram* VulkanSystem::RHICreateShaderProgram()
	{
		return new VulkanShaderProgram;
	}

	bool VulkanSystem::createInstance(std::vector<VkExtensionProperties> const& availableExtensions, bool enableValidation)
	{
		// Validation can also be forced via a define
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "GemeEngine";
		appInfo.pEngineName = "RHIEngine";
		appInfo.apiVersion = 0;

		std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

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
			instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
			instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
		}
		if (enableValidation)
		{
			// The VK_LAYER_KHRONOS_validation contains all current validation functionality.
			// Note that on Android this layer requires at least NDK r20 
			const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
			// Check if this layer is available at instance level
			std::vector<VkLayerProperties> instanceLayerProperties = GetEnumValues(vkEnumerateInstanceLayerProperties);
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

		cleanupRenderResource();

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
		VK_VERIFY_FAILCDOE(vkCreateWin32SurfaceKHR(mInstance, &surfaceInfo, gAllocCB, &result), return VK_NULL_HANDLE;);
		return result;
	}
#endif

	class ShaderFormat* VulkanSystem::createShaderFormat()
	{
		auto result = new ShaderFormatSpirv(mDevice->logicalDevice);
		return result;
	}

	bool VulkanSystem::RHIBeginRender()
	{
		vkWaitForFences(mDevice->logicalDevice, 1, &mInFlightFences[mCurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(mDevice->logicalDevice, 1, &mInFlightFences[mCurrentFrame]);

		vkAcquireNextImageKHR(mDevice->logicalDevice, mSwapChain->mSwapChain, std::numeric_limits<uint64_t>::max(), mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &mRenderImageIndex);
		return true;
	}

	void VulkanSystem::RHIEndRender(bool bPresent)
	{
		if (bPresent)
		{
			VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphores[mCurrentFrame] };

			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.waitSemaphoreCount = ARRAY_SIZE(signalSemaphores);
			presentInfo.pWaitSemaphores = signalSemaphores;

			VkSwapchainKHR swapChains[] = { mSwapChain->mSwapChain };
			presentInfo.pSwapchains = swapChains;
			presentInfo.swapchainCount = ARRAY_SIZE(swapChains);

			presentInfo.pImageIndices = &mRenderImageIndex;
			presentInfo.pResults = nullptr;
			VK_VERIFY_FAILCDOE(vkQueuePresentKHR(mPresentQueue, &presentInfo), );
		}

		mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	RHITexture2D*  VulkanSystem::RHICreateTexture2D(
		Texture::Format format, int w, int h,
		int numMipLevel, int numSamples, uint32 creationFlags,
		void* data, int dataAlign)
	{
		VulkanTexture2D* texture = new VulkanTexture2D;

		//TODO:
		if (format == Texture::eRGB8)
		{
			std::vector< uint8 > tempData;
			tempData.resize(w * h * 4);

			uint8* dest = tempData.data();
			uint8* src = (uint8*)data;

			for (int i = 0; i < w * h; ++i)
			{
				dest[0] = src[0];
				dest[1] = src[1];
				dest[2] = src[2];
				dest[3] = 0xff;
				dest += 4;
				src += 3;
			}
			if (!initalizeTexture2DInternal(texture, Texture::eRGBA8, w, h, numMipLevel, numSamples, creationFlags, tempData.data(), dataAlign))
			{
				delete texture;
				return nullptr;
			}
		}
		else
		{
			if (!initalizeTexture2DInternal(texture, format, w, h, numMipLevel, numSamples, creationFlags, data, dataAlign))
			{
				delete texture;
				return nullptr;
			}
		}
		return texture;
	}


	bool VulkanSystem::initalizeTexture2DInternal(VulkanTexture2D* texture, Texture::Format format, int width, int height, int numMipLevel, int numSamples, uint32 createFlags, void* data, int alignment)
	{
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkCommandPool commandPool = mGraphicsCommandPool;
		VkQueue copyQueue = mGraphicsQueue;

		texture->mDevice = mDevice->logicalDevice;

		bool const forceLinear = false;

		int mipLevels = 1;

		VkFormat formatVK = VulkanTranslate::To(format);
		VkImageUsageFlags imageUsageFlags = FVulkanTexture::TranslateUsage(createFlags);


		// Get device properites for the requested texture format
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(mDevice->physicalDevice, formatVK, &formatProperties);

		// Only use linear tiling if requested (and supported by the device)
		// Support for linear tiling is mostly limited, so prefer to use
		// optimal tiling instead
		// On most implementations linear tiling will only support a very
		// limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
		VkBool32 useStaging = !forceLinear;

		VkMemoryAllocateInfo memAlloc = FVulkanInit::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		// Use a separate command buffer for texture loading
		VkCommandBuffer copyCmd = mDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, commandPool, true);

		uint32 textureDataSize = width * height * Texture::GetFormatSize(format);

		if (useStaging)
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

			// Setup buffer copy regions for each mip level
			std::vector<VkBufferImageCopy> bufferCopyRegions;

			int const mipLevels = 1;

			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = std::max(1, width);
			bufferCopyRegion.imageExtent.height = std::max(1, height);
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = 0;

			bufferCopyRegions.push_back(bufferCopyRegion);


			// Create optimal tiled target image
			VkImageCreateInfo imageCreateInfo = FVulkanInit::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = formatVK;
			imageCreateInfo.mipLevels = mipLevels;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = { uint32(width), uint32(height), 1 };
			imageCreateInfo.usage = imageUsageFlags;
			// Ensure that the TRANSFER_DST bit is set for staging
			if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
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
			subresourceRange.layerCount = 1;

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
				static_cast<uint32_t>(bufferCopyRegions.size()),
				bufferCopyRegions.data()
			);

			// Change texture image layout to shader read after all mip levels have been copied
			texture->mImageLayout = imageLayout;
			FVulkanTexture::SetImageLayout(
				copyCmd,
				texture->image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				imageLayout,
				subresourceRange);

			mDevice->flushCommandBuffer(copyCmd, copyQueue, commandPool);

			// Clean up staging resources
			vkFreeMemory(mDevice->logicalDevice, stagingMemory, nullptr);
			vkDestroyBuffer(mDevice->logicalDevice, stagingBuffer, nullptr);
		}
		else
		{
			// Prefer using optimal tiling, as linear tiling 
			// may support only a small set of features 
			// depending on implementation (e.g. no mip maps, only one layer, etc.)

			// Check if this support is supported for linear tiling
			assert(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);


			VkImageCreateInfo imageCreateInfo = FVulkanInit::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = formatVK;
			imageCreateInfo.extent = { uint32(width), uint32(height), 1 };
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
			imageCreateInfo.usage = imageUsageFlags;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			VERIFY_RETURN_FALSE(texture->image.initialize(mDevice->logicalDevice, imageCreateInfo));

			// Get memory requirements for this image 
			// like size and alignment
			vkGetImageMemoryRequirements(mDevice->logicalDevice, texture->image, &memReqs);
			// Set memory allocation size to required memory size
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = mDevice->getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
			if (memAlloc.memoryTypeIndex == INDEX_NONE)
				return false;

			VkDeviceMemory mappableMemory;
			// Allocate host memory
			VK_CHECK_RESULT(vkAllocateMemory(mDevice->logicalDevice, &memAlloc, nullptr, &mappableMemory));

			// Bind allocated image for use
			VK_CHECK_RESULT(vkBindImageMemory(mDevice->logicalDevice, texture->image, mappableMemory, 0));

			// Get sub resource layout
			// Mip map count, array layer, etc.
			VkImageSubresource subRes = {};
			subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subRes.mipLevel = 0;

			VkSubresourceLayout subResLayout;
			void *pDataImage;

			// Get sub resources layout 
			// Includes row pitch, size offsets, etc.
			vkGetImageSubresourceLayout(mDevice->logicalDevice, texture->image, &subRes, &subResLayout);

			// Map image memory
			VK_CHECK_RESULT(vkMapMemory(mDevice->logicalDevice, mappableMemory, 0, memReqs.size, 0, &pDataImage));

			// Copy image data into memory
			memcpy(pDataImage, data, memReqs.size);

			vkUnmapMemory(mDevice->logicalDevice, mappableMemory);

			texture->deviceMemory.mHandle = mappableMemory;
			texture->mImageLayout = imageLayout;

			// Setup image memory barrier
			FVulkanTexture::SetImageLayout(copyCmd, texture->image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, imageLayout);

			mDevice->flushCommandBuffer(copyCmd, copyQueue, commandPool);
		}

		// Create image view
		// Textures are not directly accessed by the shaders and
		// are abstracted by image views containing additional
		// information and sub resource ranges
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = formatVK;
		viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		// Linear tiling usually won't support mip maps
		// Only set mip map count if optimal tiling is used
		viewCreateInfo.subresourceRange.levelCount = (useStaging) ? mipLevels : 1;
		viewCreateInfo.image = texture->image;

		VERIFY_RETURN_FALSE(texture->view.initialize(mDevice->logicalDevice, viewCreateInfo));

		return true;
	}

	RHIVertexBuffer*  VulkanSystem::RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		VulkanVertexBuffer* buffer = new VulkanVertexBuffer;
		if (!initalizeBufferInternal(buffer, vertexSize, numVertices, creationFlags, data))
		{
			delete buffer;
			return nullptr;
		}
		return buffer;
	}

	bool VulkanSystem::initalizeBufferInternal(VulkanVertexBuffer* buffer, uint32 elementSize, uint32 numElements, uint32 creationFlags, void* data)
	{
		buffer->mCreationFlags = creationFlags;
		buffer->mNumElements   = numElements;
		buffer->mElementSize   = elementSize;

		VkDeviceSize bufferSize = elementSize * numElements;

		VERIFY_RETURN_FALSE( mDevice->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | FVulkanBuffer::TranslateUsage(creationFlags),
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			buffer->getBufferData(), bufferSize, data) );

		return true;
	}

	RHIIndexBuffer*  VulkanSystem::RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
	{
		VulkanIndexBuffer* buffer = new VulkanIndexBuffer;
		if (!initalizeBufferInternal(buffer, bIntIndex ? sizeof(int32) : sizeof(int16), nIndices, creationFlags, data))
		{
			delete buffer;
			return nullptr;
		}
		return buffer;
	}

	bool VulkanSystem::initalizeBufferInternal(VulkanIndexBuffer* buffer, uint32 elementSize, uint32 numElements, uint32 creationFlags, void* data)
	{
		buffer->mCreationFlags = creationFlags;
		buffer->mNumElements = numElements;
		buffer->mElementSize = elementSize;

		VkDeviceSize bufferSize = elementSize * numElements;

		VERIFY_RETURN_FALSE(mDevice->createBuffer(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | FVulkanBuffer::TranslateUsage(creationFlags),
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			buffer->getBufferData(), bufferSize, data));

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

	bool VulkanSwapChain::initialize(VulkanDevice& device, VkSurfaceKHR surface, uint32 const usageQueueFamilyIndices[], bool bVSync)
	{
		mDevice = &device;

		//swap chain
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physicalDevice, surface, &capabilities);
		std::vector<VkSurfaceFormatKHR> formats = GetEnumValues(vkGetPhysicalDeviceSurfaceFormatsKHR, device.physicalDevice, surface);
		std::vector<VkPresentModeKHR> presentModes = GetEnumValues(vkGetPhysicalDeviceSurfacePresentModesKHR, device.physicalDevice, surface);

		auto  ChooseSwapSurfaceFormat = [](std::vector<VkSurfaceFormatKHR> const& availableFormats)
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
		auto  ChooseSwapPresentMode = [bVSync](std::vector<VkPresentModeKHR> const& availablePresentModes)
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

		uint32_t imageCount = capabilities.minImageCount + 1;
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
		VK_SAFE_DESTROY(mSwapChain, vkDestroySwapchainKHR(mDevice->logicalDevice, mSwapChain, gAllocCB));
	}

}//namespace Render


#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif
