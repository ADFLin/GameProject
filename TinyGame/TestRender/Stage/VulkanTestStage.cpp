#include "Stage/TestStageHeader.h"


#include "RHI/ShaderCore.h"
#include "RHI/GpuProfiler.h"
#include "Platform/Windows/WindowsProcess.h"
#include "FileSystem.h"
#include "Core/ScopeExit.h"


//#include "shaderc/shaderc.h"
//#pragma comment(lib , "shaderc_combined.lib")

#include "RHI/VulkanCommon.h"
#include "RHI/VulkanCommand.h"




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


	class VulkanSystemData
	{
	public:
		VkInstance mInstance = VK_NULL_HANDLE;
		VkPhysicalDevice mUsagePhysicalDevice = VK_NULL_HANDLE;

		VkDevice mDevice = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT mCallback = VK_NULL_HANDLE;
		VkQueue mGraphicsQueue = VK_NULL_HANDLE;
		VkQueue mPresentQueue = VK_NULL_HANDLE;

		VkSurfaceKHR   mWindowSurface = VK_NULL_HANDLE;
		VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;

		VkExtent2D             mSwapChainExtent;
		VkFormat               mSwapChainImageFormat;

		VulkanSystem* mSystem;

		bool InitTemp()
		{
			mSystem = static_cast<VulkanSystem*>(gRHISystem);
			mInstance = mSystem->instance;
			mUsagePhysicalDevice = mSystem->mDevice->physicalDevice;
			mDevice = mSystem->mDevice->logicalDevice;

			vkGetDeviceQueue(mDevice, mSystem->mUsageQueueFamilyIndices[EQueueFamily::Graphics], 0, &mGraphicsQueue);
			VERIFY_RETURN_FALSE(mGraphicsQueue);
			vkGetDeviceQueue(mDevice, mSystem->mUsageQueueFamilyIndices[EQueueFamily::Present], 0, &mPresentQueue);
			VERIFY_RETURN_FALSE(mPresentQueue);

			mWindowSurface = mSystem->mWindowSurface;
			mSwapChain = mSystem->mSwapChain->mSwapChain;

			mSwapChainExtent = mSystem->mSwapChain->mImageSize;
			mSwapChainImageFormat = mSystem->mSwapChain->mImageFormat;
			return true;
		}
#if 0
		bool initializeSystem(HWND hWnd)
		{
			{
				VkApplicationInfo appInfo = {};
				appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				appInfo.pApplicationName = "TinyGame";
				appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
				appInfo.pEngineName = "TinyEngine";
				appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
				appInfo.apiVersion = VK_API_VERSION_1_0;

				std::vector<VkLayerProperties> layers = GetEnumValues(vkEnumerateInstanceLayerProperties);
				std::vector<VkExtensionProperties> extensions = GetEnumValues(vkEnumerateInstanceExtensionProperties, nullptr);
				std::vector< char const* > enableLayers =
				{
					"VK_LAYER_LUNARG_standard_validation",
				};

				for( auto str : enableLayers )
				{
					if( !FPropertyName::Find(layers, str) )
					{
						return false;
					}
				}

				std::vector< char const* > enableExtensions;
				FPropertyName::GetAll(extensions, enableExtensions);

				VkInstanceCreateInfo instanceInfo = {};
				instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				instanceInfo.pApplicationInfo = &appInfo;
				instanceInfo.ppEnabledLayerNames = enableLayers.data();
				instanceInfo.enabledLayerCount = enableLayers.size();
				instanceInfo.ppEnabledExtensionNames = enableExtensions.data();
				instanceInfo.enabledExtensionCount = enableExtensions.size();

				VK_VERIFY_RETURN_FALSE(vkCreateInstance(&instanceInfo, gAllocCB, &mInstance));

				bool bHaveDebugUtils = FPropertyName::Find(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
				if ( bHaveDebugUtils )
				{
					auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");
					if( func )
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
#if SYS_PLATFORM_WIN
				if( !FPropertyName::Find(extensions, VK_KHR_WIN32_SURFACE_EXTENSION_NAME) )
					return false;
				mWindowSurface = createWindowSurface(hWnd);
#else
				return false;
#endif
			}


			{

				std::vector< VkPhysicalDevice > devices = GetEnumValues(vkEnumeratePhysicalDevices, mInstance);

				VkPhysicalDevice usageDevice = VK_NULL_HANDLE;
				for( auto& device : devices )
				{
					VkPhysicalDeviceProperties deviceProperties;
					vkGetPhysicalDeviceProperties(device, &deviceProperties);
					VkPhysicalDeviceFeatures deviceFeatures;
					vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
					if( deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
					{
						mUsagePhysicalDevice = device;
						break;
					}
				}

				VERIFY_RETURN_FALSE(mUsagePhysicalDevice);
				vkGetPhysicalDeviceProperties(mUsagePhysicalDevice, &mPhysicalDeviceProp);

				{
					std::vector< VkQueueFamilyProperties > queueFamilies = GetEnumValues(vkGetPhysicalDeviceQueueFamilyProperties, mUsagePhysicalDevice);
					int indexCurrent = 0;
					int indexGraphics = INDEX_NONE;
					int indexCompute = INDEX_NONE;
					int indexPresent = INDEX_NONE;
					for( auto& queueFamilyProp : queueFamilies )
					{
						if( queueFamilyProp.queueCount > 0 )
						{
							if( indexPresent == INDEX_NONE)
							{
								VkBool32 presentSupport = VK_FALSE;
								vkGetPhysicalDeviceSurfaceSupportKHR(mUsagePhysicalDevice, indexCurrent, mWindowSurface, &presentSupport);
								if( presentSupport )
								{
									indexPresent = indexCurrent;
								}
							}
							if( indexGraphics == INDEX_NONE && (queueFamilyProp.queueFlags & VK_QUEUE_GRAPHICS_BIT) )
							{
								indexGraphics = indexCurrent;
							}
							if( indexCompute == INDEX_NONE && (queueFamilyProp.queueFlags & VK_QUEUE_COMPUTE_BIT) )
							{
								indexCompute = indexCurrent;
							}
						}
						++indexCurrent;
					}

					if( indexGraphics == INDEX_NONE || indexCompute == INDEX_NONE || indexPresent == INDEX_NONE)
						return false;
					mUsageQueueFamilyIndices[EQueueFamily::Graphics] = indexGraphics;
					mUsageQueueFamilyIndices[EQueueFamily::Present] = indexPresent;
					mUsageQueueFamilyIndices[EQueueFamily::Compute] = indexCompute;
				}

				std::vector< uint32 > uniqueQueueFamilies =
				{
					mUsageQueueFamilyIndices[EQueueFamily::Graphics] ,
					mUsageQueueFamilyIndices[EQueueFamily::Present] ,
					mUsageQueueFamilyIndices[EQueueFamily::Compute]
				};
				MakeValueUnique(uniqueQueueFamilies);

				std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
				float queuePriority = 1.0f;
				for( auto index : uniqueQueueFamilies )
				{
					VkDeviceQueueCreateInfo queueInfo = {};
					queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
					queueInfo.queueFamilyIndex = index;
					queueInfo.queueCount = 1;
					queueInfo.pQueuePriorities = &queuePriority;
					queueCreateInfos.push_back(queueInfo);
				}


				std::vector<VkLayerProperties> deviceLayers = GetEnumValues(vkEnumerateDeviceLayerProperties, mUsagePhysicalDevice);


				std::vector< char const* > enableLayers =
				{
					"VK_LAYER_LUNARG_standard_validation",
				};

				for( auto str : enableLayers )
				{
					if( !FPropertyName::Find(deviceLayers, str) )
					{
						return false;
					}
				}
				std::vector< char const* > enabledExtensionNames;
				std::vector< VkExtensionProperties > allExtensions;

				{
					auto layerExtensions = GetEnumValues(vkEnumerateDeviceExtensionProperties, mUsagePhysicalDevice, nullptr);
					allExtensions.insert(allExtensions.end(), layerExtensions.begin(), layerExtensions.end());
					for( auto& layer : enableLayers )
					{
						auto layerExtensions = GetEnumValues(vkEnumerateDeviceExtensionProperties, mUsagePhysicalDevice, layer);
						allExtensions.insert(allExtensions.end(), layerExtensions.begin(), layerExtensions.end());
					}
				}

				if( !FPropertyName::Find(allExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME) )
					return false;



				FPropertyName::GetAll(allExtensions, enabledExtensionNames);

				VkPhysicalDeviceFeatures deviceFeatures;
				vkGetPhysicalDeviceFeatures(mUsagePhysicalDevice, &deviceFeatures);

				VkDeviceCreateInfo deviceInfo = {};
				deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
				deviceInfo.queueCreateInfoCount = queueCreateInfos.size();
				deviceInfo.pEnabledFeatures = &deviceFeatures;
				deviceInfo.ppEnabledLayerNames = enableLayers.data();
				deviceInfo.enabledLayerCount = enableLayers.size();
				deviceInfo.ppEnabledExtensionNames = enabledExtensionNames.data();
				deviceInfo.enabledExtensionCount = enabledExtensionNames.size();

				VK_VERIFY_RETURN_FALSE(vkCreateDevice(mUsagePhysicalDevice, &deviceInfo, gAllocCB, &mDevice));


				vkGetDeviceQueue(mDevice, mUsageQueueFamilyIndices[EQueueFamily::Graphics], 0, &mGraphicsQueue);
				VERIFY_RETURN_FALSE(mGraphicsQueue);
				vkGetDeviceQueue(mDevice, mUsageQueueFamilyIndices[EQueueFamily::Present], 0, &mPresentQueue);
				VERIFY_RETURN_FALSE(mPresentQueue);
			}

			return true;
		}

		void cleanupSystem()
		{
			VK_SAFE_DESTROY(mDevice, vkDestroyDevice(mDevice, gAllocCB));
			VK_SAFE_DESTROY(mWindowSurface, vkDestroySurfaceKHR(mInstance, mWindowSurface, gAllocCB));

			if (mCallback != VK_NULL_HANDLE)
			{
				auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
				func(mInstance, mCallback, gAllocCB);
				mCallback = VK_NULL_HANDLE;
			}
			VK_SAFE_DESTROY(mInstance, vkDestroyInstance(mInstance, gAllocCB));
		}
#endif



#if 0
#if SYS_PLATFORM_WIN
		VkSurfaceKHR createWindowSurface(HWND hWnd)
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
		bool createWindowSwapchain()
		{
			//swap chain
			VkSurfaceCapabilitiesKHR capabilities;
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mUsagePhysicalDevice, mWindowSurface, &capabilities);
			std::vector<VkSurfaceFormatKHR> formats = GetEnumValues(vkGetPhysicalDeviceSurfaceFormatsKHR, mUsagePhysicalDevice, mWindowSurface);
			std::vector<VkPresentModeKHR> presentModes = GetEnumValues(vkGetPhysicalDeviceSurfacePresentModesKHR, mUsagePhysicalDevice, mWindowSurface);

			auto  ChooseSwapSurfaceFormat = [](std::vector<VkSurfaceFormatKHR> const& availableFormats)
			{
				if( availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED )
				{
					return VkSurfaceFormatKHR{ VK_FORMAT_B8G8R8A8_UNORM,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
				}
				for( const auto& availableFormat : availableFormats )
				{
					if( availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
					   availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
					{
						return availableFormat;
					}
				}
				return availableFormats[0];
			};
			auto  ChooseSwapPresentMode = [](std::vector<VkPresentModeKHR> const& availablePresentModes)
			{
				VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
				for( const auto& availablePresentMode : availablePresentModes )
				{
					if( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
					{
						bestMode = availablePresentMode;
						break;
					}
					else if( availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR )
					{
						bestMode = availablePresentMode;
					}
				}
				return bestMode;
			};

			auto ChooseSwapExtent = [](const VkSurfaceCapabilitiesKHR& capabilities) -> VkExtent2D
			{
				return capabilities.currentExtent;
			};

			auto usageFormat = ChooseSwapSurfaceFormat(formats);

			mSwapChainImageFormat = usageFormat.format;
			VkPresentModeKHR presentMode = ChooseSwapPresentMode(presentModes);
			mSwapChainExtent = ChooseSwapExtent(capabilities);
			VkSwapchainCreateInfoKHR swapChainInfo = {};

			uint32_t imageCount = capabilities.minImageCount + 1;
			if( capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount )
			{
				imageCount = capabilities.maxImageCount;
			}
			swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapChainInfo.surface = mWindowSurface;
			swapChainInfo.minImageCount = imageCount;
			swapChainInfo.imageFormat = usageFormat.format;
			swapChainInfo.imageColorSpace = usageFormat.colorSpace;
			swapChainInfo.imageExtent = mSwapChainExtent;
			swapChainInfo.imageArrayLayers = 1;
			swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapChainInfo.preTransform = capabilities.currentTransform;
			swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapChainInfo.presentMode = presentMode;
			swapChainInfo.clipped = VK_TRUE;
			swapChainInfo.oldSwapchain = VK_NULL_HANDLE;
			if( mUsageQueueFamilyIndices[EQueueFamily::Graphics] != mUsageQueueFamilyIndices[EQueueFamily::Present] )
			{
				swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				swapChainInfo.pQueueFamilyIndices = mUsageQueueFamilyIndices;
				swapChainInfo.queueFamilyIndexCount = 2;
			}
			else
			{
				swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				swapChainInfo.pQueueFamilyIndices = nullptr;
				swapChainInfo.queueFamilyIndexCount = 0;
			}

			VK_VERIFY_RETURN_FALSE(vkCreateSwapchainKHR(mDevice, &swapChainInfo, gAllocCB, &mSwapChain));

			//mSwapChainImages = GetEnumValues(vkGetSwapchainImagesKHR, mDevice, mSwapChain);
			vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, nullptr);
			mSwapChainImages.resize(imageCount);
			vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, mSwapChainImages.data());

			for( auto image : mSwapChainImages )
			{
				VkImageViewCreateInfo imageViewInfo = {};
				imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				imageViewInfo.image = image;
				imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				imageViewInfo.format = mSwapChainImageFormat;
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
				VK_VERIFY_RETURN_FALSE(vkCreateImageView(mDevice, &imageViewInfo, gAllocCB, &imageView));
				mSwapChainImageViews.push_back(imageView);
			}

			return true;
		}

		void cleanupWindowSwapchain()
		{
			if (!mSwapChainFramebuffers.empty())
			{
				for (auto buffer : mSwapChainFramebuffers)
				{
					vkDestroyFramebuffer(mDevice, buffer, gAllocCB);
				}
				mSwapChainFramebuffers.clear();
			}

			if (!mSwapChainImageViews.empty())
			{
				for (auto view : mSwapChainImageViews)
				{
					vkDestroyImageView(mDevice, view, gAllocCB);
				}
				mSwapChainImageViews.clear();
			}
			mSwapChainImages.clear();

			VK_SAFE_DESTROY(mSwapChain, vkDestroySwapchainKHR(mDevice, mSwapChain, gAllocCB));
		}
#endif
		std::vector< VkFramebuffer > mSwapChainFramebuffers;

		bool createSwapchainFrameBuffers()
		{
			mSwapChainFramebuffers.resize(mSystem->mSwapChain->mImageViews.size());

			for( int i = 0; i < mSystem->mSwapChain->mImageViews.size(); ++i )
			{
				VkImageView attachments[] = { mSystem->mSwapChain->mImageViews[i] };
				VkFramebufferCreateInfo frameBufferInfo = {};
				frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
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

		struct BufferData
		{
			VkBuffer handle;
			VkDeviceMemory memory;
		};
		RHIVertexBufferRef mVertexBuffer;
		RHIIndexBufferRef  mIndexBuffer;

		bool createSimplePipepline()
		{

			setupDescriptorPool();
			setupDescriptorSetLayout();
			setupDescriptorSet();

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
				renderPassInfo.dependencyCount = 2;

				VK_VERIFY_RETURN_FALSE(vkCreateRenderPass(mDevice, &renderPassInfo, gAllocCB, &mSimpleRenderPass));

			}

			{
				VkPipelineLayoutCreateInfo pipelineLayoutInfo = FVulkanInit::pipelineLayoutCreateInfo();
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutInfo.pSetLayouts = nullptr;
				pipelineLayoutInfo.setLayoutCount = 0;
				pipelineLayoutInfo.pPushConstantRanges = nullptr;
				pipelineLayoutInfo.pushConstantRangeCount = 0;
				VK_VERIFY_RETURN_FALSE(vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, gAllocCB, &mEmptyPipelineLayout));
			}

			struct Vertex
			{
				Vector2 pos;
				Vector2 uv;
				Vector3 color;
			};

			Vertex v[] =
			{
				Vector2(0.5, -0.5),Vector2(1, 0),Vector3(1.0, 0.0, 0.0),
				Vector2(0.5, 0.5),Vector2(1, 1), Vector3(0.0, 1.0, 0.0),
				Vector2(-0.5, 0.5),Vector2(0, 1),Vector3(1.0, 1.0, 1.0),
				Vector2(-0.5, -0.5),Vector2(0, 0),Vector3(1.0, 1.0, 1.0),
			};
			VERIFY_RETURN_FALSE(mVertexBuffer = RHICreateVertexBuffer(sizeof(Vertex), ARRAY_SIZE(v), BCF_DefalutValue, v));
			uint32 indices[] =
			{
				0,1,2,
				0,2,3,
			};
			VERIFY_RETURN_FALSE(mIndexBuffer = RHICreateIndexBuffer(ARRAY_SIZE(indices), true , BCF_DefalutValue , indices));
			VkVertexInputBindingDescription vertexInputBindingDescs[] =
			{
				//binding stride , rate
				{ 0 , sizeof(Vertex) , VK_VERTEX_INPUT_RATE_VERTEX },
			};

			VkVertexInputAttributeDescription vertexInputAttrDescs[] =
			{
				//location binding format offset
				{0 , 0, VK_FORMAT_R32G32_SFLOAT , offsetof(Vertex , pos) },
				{1 , 0, VK_FORMAT_R32G32_SFLOAT , offsetof(Vertex , uv) },
				{2 , 0, VK_FORMAT_R32G32B32_SFLOAT , offsetof(Vertex , color) },
			};

			{
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

				VkPipelineRasterizationStateCreateInfo rasterizationState = {};
				rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
				rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
				rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
				rasterizationState.rasterizerDiscardEnable = VK_FALSE;
				rasterizationState.depthBiasEnable = VK_FALSE;
				rasterizationState.depthClampEnable = VK_FALSE;
				rasterizationState.lineWidth = 1.0f;

				VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
				depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				depthStencilState.depthWriteEnable = VK_TRUE;
				depthStencilState.depthTestEnable = VK_TRUE;
				depthStencilState.depthBoundsTestEnable = VK_FALSE;
				depthStencilState.stencilTestEnable = VK_FALSE;


				VkPipelineColorBlendAttachmentState colorAttachmentState = {};
				colorAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				colorAttachmentState.blendEnable = VK_FALSE;
				colorAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
				colorAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
				colorAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				colorAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
				colorAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				colorAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;

				VkPipelineColorBlendStateCreateInfo colorBlendState = {};
				colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				colorBlendState.pAttachments = &colorAttachmentState;
				colorBlendState.attachmentCount = 1;
				colorBlendState.logicOp = VK_LOGIC_OP_COPY;
				colorBlendState.logicOpEnable = VK_FALSE;
				colorBlendState.blendConstants[0] = 0.0f;
				colorBlendState.blendConstants[1] = 0.0f;
				colorBlendState.blendConstants[2] = 0.0f;
				colorBlendState.blendConstants[3] = 0.0f;

				VkPipelineTessellationStateCreateInfo tesselationState = {};
				tesselationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
				tesselationState.patchControlPoints = 0;

				VkPipelineMultisampleStateCreateInfo multisampleState = {};
				multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampleState.sampleShadingEnable = VK_FALSE;
				multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
				multisampleState.minSampleShading = 1.0f;
				multisampleState.pSampleMask = nullptr;
				multisampleState.alphaToCoverageEnable = VK_FALSE;
				multisampleState.alphaToOneEnable = VK_FALSE;

				VkShaderModule vertexModule = compileAndLoadShader("TestShader/Vertex.sgc" , Shader::eVertex);
				if( vertexModule == VK_NULL_HANDLE )
					return false;
				VkShaderModule pixelModule = compileAndLoadShader("TestShader/Pixel.sgc" , Shader::ePixel);
				if( pixelModule == VK_NULL_HANDLE )
					return false;

				VkPipelineShaderStageCreateInfo vertexStage = {};
				vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				vertexStage.module = vertexModule;
				vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
				vertexStage.pName = "main";
				vertexStage.pSpecializationInfo = nullptr;

				VkPipelineShaderStageCreateInfo pixelStage = {};
				pixelStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				pixelStage.module = pixelModule;
				pixelStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				pixelStage.pName = "main";
				pixelStage.pSpecializationInfo = nullptr;
				std::vector < VkPipelineShaderStageCreateInfo > shaderStages = { vertexStage , pixelStage };

				VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
				inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				inputAssemblyState.primitiveRestartEnable = VK_FALSE;
				inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

				VkPipelineVertexInputStateCreateInfo vertexInputState = {};
				vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInputState.pVertexAttributeDescriptions = vertexInputAttrDescs;
				vertexInputState.vertexAttributeDescriptionCount = ARRAY_SIZE(vertexInputAttrDescs);
				vertexInputState.pVertexBindingDescriptions = vertexInputBindingDescs;
				vertexInputState.vertexBindingDescriptionCount = ARRAY_SIZE(vertexInputBindingDescs);

				VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT };
				VkPipelineDynamicStateCreateInfo dynamicState = {};
				dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				dynamicState.pDynamicStates = dynamicStates;
				dynamicState.dynamicStateCount = ARRAY_SIZE(dynamicStates);

				VkGraphicsPipelineCreateInfo pipelineInfo =
					FVulkanInit::pipelineCreateInfo(
						pipelineLayout,
						mSimpleRenderPass,
						0);
				pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				pipelineInfo.pViewportState = &viewportState;
				pipelineInfo.pRasterizationState = &rasterizationState;
				pipelineInfo.pDepthStencilState = &depthStencilState;
				pipelineInfo.pColorBlendState = &colorBlendState;

				pipelineInfo.pTessellationState = &tesselationState;
				pipelineInfo.pMultisampleState = &multisampleState;

				pipelineInfo.pStages = shaderStages.data();
				pipelineInfo.stageCount = shaderStages.size();
				pipelineInfo.pInputAssemblyState = &inputAssemblyState;
				pipelineInfo.pVertexInputState = &vertexInputState;
				pipelineInfo.pDynamicState = &dynamicState;

				pipelineInfo.subpass = 0;
				pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
				pipelineInfo.basePipelineIndex = 0;

				VK_VERIFY_RETURN_FALSE(vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, gAllocCB, &mSimplePipeline));

				VK_SAFE_DESTROY(vertexModule, vkDestroyShaderModule(mDevice, vertexModule, gAllocCB));
				VK_SAFE_DESTROY(pixelModule, vkDestroyShaderModule(mDevice, pixelModule, gAllocCB));
			}

			return true;
		}

		void cleanupSimplePipeline()
		{
			VK_SAFE_DESTROY(mSimpleRenderPass, vkDestroyRenderPass(mDevice, mSimpleRenderPass, gAllocCB));
			VK_SAFE_DESTROY(mEmptyPipelineLayout, vkDestroyPipelineLayout(mDevice, mEmptyPipelineLayout, gAllocCB));
			VK_SAFE_DESTROY(mSimplePipeline, vkDestroyPipeline(mDevice, mSimplePipeline, gAllocCB));
		}

		VkCommandPool mCommandPool = VK_NULL_HANDLE;
		std::vector< VkCommandBuffer > mCommandBuffers;
		bool createRenderCommand()
		{
			{
				VkCommandPoolCreateInfo poolInfo = {};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.queueFamilyIndex = mSystem->mUsageQueueFamilyIndices[EQueueFamily::Graphics];
				poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				VK_VERIFY_RETURN_FALSE(vkCreateCommandPool(mDevice, &poolInfo, gAllocCB, &mCommandPool));
			}

			{
				mCommandBuffers.resize(mSwapChainFramebuffers.size());
				VkCommandBufferAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = mCommandPool;
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
			// Example uses one ubo and one image sampler
			std::vector<VkDescriptorPoolSize> poolSizes =
			{
				FVulkanInit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
			};

			VkDescriptorPoolCreateInfo descriptorPoolInfo =
				FVulkanInit::descriptorPoolCreateInfo(
					static_cast<uint32_t>(poolSizes.size()),
					poolSizes.data(),
					2);

			VK_CHECK_RESULT(vkCreateDescriptorPool(mDevice, &descriptorPoolInfo, nullptr, &descriptorPool));
		}

		void setupDescriptorSetLayout()
		{
			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
			{
				// Binding 1 : Fragment shader image sampler
				FVulkanInit::descriptorSetLayoutBinding(
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					VK_SHADER_STAGE_FRAGMENT_BIT,
					0)
			};

			VkDescriptorSetLayoutCreateInfo descriptorLayout =
				FVulkanInit::descriptorSetLayoutCreateInfo(
					setLayoutBindings.data(),
					static_cast<uint32_t>(setLayoutBindings.size()));

			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(mDevice, &descriptorLayout, nullptr, &descriptorSetLayout));

			VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
				FVulkanInit::pipelineLayoutCreateInfo(
					&descriptorSetLayout,
					1);

			VK_CHECK_RESULT(vkCreatePipelineLayout(mDevice, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
		}

		RHITexture2DRef texture;
		void setupDescriptorSet()
		{
			texture = RHIUtility::LoadTexture2DFromFile("Texture/rocks.jpg");
			
			VkDescriptorSetAllocateInfo allocInfo =
				FVulkanInit::descriptorSetAllocateInfo(
					descriptorPool,
					&descriptorSetLayout,
					1);

			VK_CHECK_RESULT(vkAllocateDescriptorSets(mDevice, &allocInfo, &descriptorSet));

			// Setup a descriptor image info for the current texture to be used as a combined image sampler
			VkDescriptorImageInfo textureDescriptor;
			textureDescriptor.imageView = VulkanCast::To(texture)->view; // The image's view (images are never directly accessed by the shader, but rather through views defining subresources)
			textureDescriptor.sampler = VulkanCast::GetHandle(TStaticSamplerState<>::GetRHI()); // The sampler (Telling the pipeline how to sample the texture, including repeat, border, etc.)
			textureDescriptor.imageLayout = VulkanCast::To(texture)->mImageLayout;	// The current layout of the image (Note: Should always fit the actual use, e.g. shader read)

			std::vector<VkWriteDescriptorSet> writeDescriptorSets =
			{
				FVulkanInit::writeDescriptorSet(
					descriptorSet,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		// The descriptor set will use a combined image sampler (sampler and image could be split)
					0,												// Shader binding point 1
					&textureDescriptor)								// Pointer to the descriptor image for our texture
			};

			vkUpdateDescriptorSets(mDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
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

		void cleanupRenderCommand()
		{
			VK_SAFE_DESTROY(mCommandPool, vkDestroyCommandPool(mDevice, mCommandPool, gAllocCB));

		}

		VkShaderModule loadShader(char const* pCode , size_t codeSize )
		{
			VkShaderModuleCreateInfo moduleInfo = {};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.pCode = (uint32*)pCode;
			moduleInfo.codeSize = codeSize;
			VkShaderModule module;
			VK_VERIFY_FAILCDOE(vkCreateShaderModule(mDevice, &moduleInfo, gAllocCB, &module), return VK_NULL_HANDLE;);

			return module;
		}

		VkSemaphore createSemphore()
		{
			VkSemaphoreCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VkSemaphore result;
			VK_VERIFY_FAILCDOE(vkCreateSemaphore(mDevice, &createInfo, gAllocCB, &result), return VK_NULL_HANDLE; );
			return result;
		}

		bool compileShader(char const* inPath, char const* outPath,  Shader::Type type)
		{
			char const* VulkanSDKDir = SystemPlatform::GetEnvironmentVariable("VULKAN_SDK");
			if( VulkanSDKDir == nullptr )
			{
				VulkanSDKDir = "C:/VulkanSDK/1.1.130.0";
			}
			std::string GlslangValidatorPath = VulkanSDKDir;
			GlslangValidatorPath += "/Bin/glslangValidator.exe";
			static char const* const ShaderNames[] =
			{
				"vert","frag","geom","comp","tesc","tese"
			};

			FixString<1024> command;
			command.format(" -V -S %s -o %s %s", ShaderNames[type], outPath, inPath);

			ChildProcess process;
			if( !process.create(GlslangValidatorPath.c_str(), command) )
			{
				return false;
			}
			process.waitCompletion();
			int32 exitCode;
			if( !process.getExitCode(exitCode) || exitCode != 0 )
			{
				return false;
			}
			return true;
		}

		VkShaderModule compileAndLoadShader(char const* path, Shader::Type type)
		{
#if 1
			std::string fullPath = FileUtility::GetFullPath(path);
			char const* dirEndPos = FileUtility::GetFileName(fullPath.c_str());
			std::string outPath = fullPath.substr(0, dirEndPos - &fullPath[0]) + "/shader.tmp";
			if( !compileShader(fullPath.c_str() , outPath.c_str(), type) )
				return VK_NULL_HANDLE;
			std::vector<char> code;
			if( !FileUtility::LoadToBuffer(outPath.c_str(), code) )
				return VK_NULL_HANDLE;

			return loadShader(code.data(),code.size());
#else
			std::vector<uint8> scoreCode;
			if( !FileUtility::LoadToBuffer(path, scoreCode) )
				return VK_NULL_HANDLE;

			shaderc_compiler_t compilerHandle = shaderc_compiler_initialize();
			shaderc_compile_options_t optionHandle = shaderc_compile_options_initialize();

			ON_SCOPE_EXIT
			{
				shaderc_compiler_release(compilerHandle);
				shaderc_compile_options_release(optionHandle);
			};
			shaderc_shader_kind ShaderKindMap[] =
			{
				shaderc_vertex_shader,
				shaderc_fragment_shader,
				shaderc_geometry_shader,
				shaderc_compute_shader,
				shaderc_tess_control_shader,
				shaderc_tess_evaluation_shader,
			};

			shaderc_compilation_result_t resultHandle = shaderc_compile_into_spv(compilerHandle, scoreCode.data(), scoreCode.size(), ShaderKindMap[type], nullptr, nullptr, optionHandle);
			ON_SCOPE_EXIT
			{
				shaderc_result_release(resultHandle);
			};

			if( shaderc_result_get_compilation_status(resultHandle) != shaderc_compilation_status_success )
			{
				return VK_NULL_HANDLE;
			}

			return loadShader(shaderc_result_get_bytes(resultHandle), shaderc_result_get_length(resultHandle));
#endif
		}

	};


	class TestStage : public StageBase
		            , public VulkanSystemData
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}




		std::vector< VkSemaphore > mImageAvailableSemaphores;
		std::vector< VkSemaphore > mRenderFinishedSemaphores;
		std::vector< VkFence >     mInFlightFences;
		const int   MAX_FRAMES_IN_FLIGHT = 2;
		int mCurrentFrame = 0;


		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			::Global::GetDrawEngine().bUsePlatformBuffer = false;

			if (!::Global::GetDrawEngine().initializeRHI(RHITargetName::Vulkan))
				return false;

			//VERIFY_RETURN_FALSE( initializeSystem( ::Global::GetDrawEngine().getWindow().getHWnd() ) );
			//VERIFY_RETURN_FALSE( createWindowSwapchain() );
			InitTemp();
			VERIFY_RETURN_FALSE( createSimplePipepline() );
			VERIFY_RETURN_FALSE( createSwapchainFrameBuffers() );
			VERIFY_RETURN_FALSE( createRenderCommand() );

			TStaticSamplerState<>::GetRHI();

			mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
			mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
			mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			for( int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
			{
				mImageAvailableSemaphores[i] = createSemphore();
				mRenderFinishedSemaphores[i] = createSemphore();
				vkCreateFence(mDevice, &fenceInfo, gAllocCB, &mInFlightFences[i]);
			}

			::Global::GUI().cleanupWidget();

			restart();
			return true;
		}

		virtual void onEnd()
		{
			vkDeviceWaitIdle(mDevice);

			for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
			{
				vkDestroySemaphore(mDevice, mRenderFinishedSemaphores[i], gAllocCB);
				vkDestroySemaphore(mDevice, mImageAvailableSemaphores[i], gAllocCB);
				vkDestroyFence(mDevice, mInFlightFences[i], gAllocCB);
			}
			mRenderFinishedSemaphores.clear();
			mImageAvailableSemaphores.clear();
			mInFlightFences.clear();

			cleanupRenderCommand();
			cleanupSimplePipeline();
			//cleanupWindowSwapchain();
			//cleanupSystem();
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
			vkWaitForFences(mDevice, 1, &mInFlightFences[mCurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
			vkResetFences(mDevice, 1, &mInFlightFences[mCurrentFrame]);

			uint32_t imageIndex; 
			vkAcquireNextImageKHR(mDevice, mSwapChain,std::numeric_limits<uint64_t>::max(), mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &imageIndex);

			if( bDynamicGenerate )
			{
				generateRenderCommand(mCommandBuffers[imageIndex], mSwapChainFramebuffers[imageIndex]);
			}
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pCommandBuffers = &mCommandBuffers[imageIndex];
			submitInfo.commandBufferCount = 1;

			VkSemaphore waitSemaphores[] = { mImageAvailableSemaphores[mCurrentFrame] };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.waitSemaphoreCount = ARRAY_SIZE(waitSemaphores);
			

			VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphores[mCurrentFrame] };
			submitInfo.pSignalSemaphores = signalSemaphores;
			submitInfo.signalSemaphoreCount = ARRAY_SIZE(signalSemaphores);

			VK_VERIFY_FAILCDOE(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFences[mCurrentFrame]), );

			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.waitSemaphoreCount = ARRAY_SIZE(signalSemaphores);
			presentInfo.pWaitSemaphores = signalSemaphores;

			VkSwapchainKHR swapChains[] = { mSwapChain };
			presentInfo.pSwapchains = swapChains;
			presentInfo.swapchainCount = ARRAY_SIZE(swapChains);

			presentInfo.pImageIndices = &imageIndex;
			presentInfo.pResults = nullptr;
			VK_VERIFY_FAILCDOE(vkQueuePresentKHR(mPresentQueue, &presentInfo) , );

			mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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