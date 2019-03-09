#include "Stage/TestStageHeader.h"


#include "RHI/ShaderCore.h"
#include "RHI/GpuProfiler.h"
#include "Platform/Windows/WindowsProcess.h"
#include "FileSystem.h"
#include "Core/ScopeExit.h"

#include "vulkan/vulkan.h"
#if SYS_PLATFORM_WIN
#include "vulkan/vulkan_win32.h"
#endif
//#include "shaderc/shaderc.h"


#pragma comment(lib , "vulkan-1.lib")
//#pragma comment(lib , "shaderc_combined.lib")

#define ERROR_MSG_GENERATE( HR , CODE , FILE , LINE )\
	LogWarning(1, "ErrorCode = 0x%x File = %s Line = %s %s ", HR , FILE, #LINE, #CODE)

#define VERIFY_VKRESULT_INNER( FILE , LINE , CODE ,ERRORCODE )\
	{ VkResult vkResult = CODE; if( vkResult != VK_SUCCESS ){ ERROR_MSG_GENERATE( vkResult , CODE, FILE, LINE ); ERRORCODE } }

#define VERIFY_VK_FAILCDOE( CODE , ERRORCODE ) VERIFY_VKRESULT_INNER( __FILE__ , __LINE__ , CODE , ERRORCODE )
#define VERIFY_VK_RETURN_FALSE( CODE ) VERIFY_VKRESULT_INNER( __FILE__ , __LINE__ , CODE , return false; )

#define SAFE_VK_DESTROY( VKHANDLE , CODE )\
	if( VKHANDLE != VK_NULL_HANDLE )\
	{\
		CODE;\
		VKHANDLE = VK_NULL_HANDLE;\
	}


namespace Meta
{
	template< int Index, class ...Args >
	struct GetArgsType {};

	template< int Index, class T, class ...Args >
	struct GetArgsType< Index, T, Args... >
	{
		typedef typename GetArgsType< Index - 1, Args... >::Type Type;
	};

	template< class T, class ...Args >
	struct GetArgsType< 0, T, Args... >
	{
		typedef T Type;
	};

	template< class ...Args >
	struct GetArgsLength {};

	template<>
	struct GetArgsLength<>
	{
		enum { Value = 0 };
	};

	template< class T, class ...Args >
	struct GetArgsLength< T, Args... >
	{
		enum { Value = 1 + GetArgsLength< Args...>::Value };
	};

	template< class ...Args >
	struct TypeList
	{
		enum { Length = GetArgsLength< Args... >::Value; };
	};


	template< class FunType >
	struct FunTraits {};
	template< class RT, class ...Args >
	struct FunTraits< RT(*)(Args...) >
	{
		typedef TypeList< Args... > ArgList;
	};
}

VkAllocationCallbacks* gAllocCB = nullptr;

namespace RenderVulkan
{
	using namespace Meta;
	using namespace Render;

	template< class ...FunArgs, class ...Args >
	FORCEINLINE static auto GetEnumValues(VkResult(VKAPI_CALL &Fun)(FunArgs...), Args... args)
	{
		using namespace Meta;
		typedef typename GetArgsType< GetArgsLength< FunArgs... >::Value - 1, FunArgs... >::Type ArgType;
		typedef typename std::remove_pointer< ArgType >::type PropertyType;
		uint32_t count = 0;
		VERIFY_VK_FAILCDOE(Fun(std::forward<Args>(args)..., &count, nullptr), );
		std::vector< PropertyType > result{ count };
		VERIFY_VK_FAILCDOE(Fun(std::forward<Args>(args)..., &count, result.data()), );
		return result;
	}

	template< class ...FunArgs, class ...Args >
	FORCEINLINE static auto GetEnumValues(void(VKAPI_CALL &Fun)(FunArgs...), Args... args)
	{
		using namespace Meta;
		typedef typename GetArgsType< GetArgsLength< FunArgs... >::Value - 1, FunArgs... >::Type ArgType;
		typedef typename std::remove_pointer< ArgType >::type PropertyType;

		uint32_t count = 0;
		Fun(std::forward<Args>(args)..., &count, nullptr);
		std::vector< PropertyType > result{ count };
		Fun(std::forward<Args>(args)..., &count, result.data());
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
			for( auto& element : v )
			{
				if( strcmp(FPropertyName::Get(element), name) == 0 )
					return true;
			}
			return false;
		}

		template< class T >
		static void GetAll(std::vector<T> const& v, std::vector< char const* >& outNames)
		{
			for( auto& element : v )
			{
				outNames.push_back(FPropertyName::Get(element));
			}
		}
	};

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugVKCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		LogWarning(0, pCallbackData->pMessage);
		return VK_FALSE;
	}


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
			VERIFY_VK_RETURN_FALSE(vkCreateQueryPool(mDevice, &queryPoolInfo, gAllocCB, &mTimestampQueryPool));
		}


		virtual void beginFrame()
		{
			mIndexNextQuery = 0;
		}

		virtual void endFrame()
		{
			vkGetQueryPoolResults(mDevice, mTimestampQueryPool, 0, mIndexNextQuery, mIndexNextQuery * sizeof(uint64), mTimesampResult.data(), sizeof(uint64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT );
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
		virtual bool getTimingDurtion(uint32 timingHandle, uint64& outDurtion)
		{

			
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
		VkPhysicalDeviceProperties mPhysicalDeviceProp;

		VkDevice mDevice = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT mCallback = VK_NULL_HANDLE;
		VkQueue mGraphicsQueue = VK_NULL_HANDLE;
		VkQueue mPresentQueue = VK_NULL_HANDLE;

		struct EQueueFamily
		{
			enum Type
			{
				Graphics = 0,
				Present = 1,
				Compute = 2,
				Count,
			};
		};
		uint32 mUsageQueueFamilyIndices[EQueueFamily::Count];

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

				VERIFY_VK_RETURN_FALSE(vkCreateInstance(&instanceInfo, gAllocCB, &mInstance));

				bool bHaveDebugUtils = FPropertyName::Find(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
				if ( bHaveDebugUtils )
				{
					auto fun = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");
					if( fun )
					{
						VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
						debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
						debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
						debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
						debugInfo.pfnUserCallback = DebugVKCallback;
						debugInfo.pUserData = nullptr;
						VERIFY_VK_RETURN_FALSE(fun(mInstance, &debugInfo, gAllocCB, &mCallback));
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
					int indexGraphics = -1;
					int indexCompute = -1;
					int indexPresent = -1;
					for( auto& queueFamilyProp : queueFamilies )
					{
						if( queueFamilyProp.queueCount > 0 )
						{
							if( indexPresent == -1 )
							{
								VkBool32 presentSupport = VK_FALSE;
								vkGetPhysicalDeviceSurfaceSupportKHR(mUsagePhysicalDevice, indexCurrent, mWindowSurface, &presentSupport);
								if( presentSupport )
								{
									indexPresent = indexCurrent;
								}
							}
							if( indexGraphics == -1 && (queueFamilyProp.queueFlags & VK_QUEUE_GRAPHICS_BIT) )
							{
								indexGraphics = indexCurrent;
							}
							if( indexCompute == -1 && (queueFamilyProp.queueFlags & VK_QUEUE_COMPUTE_BIT) )
							{
								indexCompute = indexCurrent;
							}
						}
						++indexCurrent;
					}

					if( indexGraphics == -1 || indexCompute == -1 || indexPresent == -1 )
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
				deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO;
				deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
				deviceInfo.queueCreateInfoCount = queueCreateInfos.size();
				deviceInfo.pEnabledFeatures = &deviceFeatures;
				deviceInfo.ppEnabledLayerNames = enableLayers.data();
				deviceInfo.enabledLayerCount = enableLayers.size();
				deviceInfo.ppEnabledExtensionNames = enabledExtensionNames.data();
				deviceInfo.enabledExtensionCount = enabledExtensionNames.size();

				VERIFY_VK_RETURN_FALSE(vkCreateDevice(mUsagePhysicalDevice, &deviceInfo, gAllocCB, &mDevice));


				vkGetDeviceQueue(mDevice, mUsageQueueFamilyIndices[EQueueFamily::Graphics], 0, &mGraphicsQueue);
				VERIFY_RETURN_FALSE(mGraphicsQueue);
				vkGetDeviceQueue(mDevice, mUsageQueueFamilyIndices[EQueueFamily::Present], 0, &mPresentQueue);
				VERIFY_RETURN_FALSE(mPresentQueue);
			}

			return true;
		}


		void cleanupSystem()
		{
			SAFE_VK_DESTROY(mDevice, vkDestroyDevice(mDevice, gAllocCB));
			SAFE_VK_DESTROY(mWindowSurface, vkDestroySurfaceKHR(mInstance, mWindowSurface, gAllocCB));

			if( mCallback != VK_NULL_HANDLE )
			{
				auto fun = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
				fun(mInstance, mCallback, gAllocCB);
				mCallback = VK_NULL_HANDLE;
			}
			SAFE_VK_DESTROY(mInstance, vkDestroyInstance(mInstance, gAllocCB));
		}

		VkSurfaceKHR   mWindowSurface = VK_NULL_HANDLE;
		VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;
		std::vector< VkImageView >   mSwapChainImageViews;
		std::vector< VkFramebuffer > mSwapChainFramebuffers;

		VkExtent2D             mSwapChainExtent;
		VkFormat               mSwapChainImageFormat;
		std::vector< VkImage > mSwapChainImages;

#if SYS_PLATFORM_WIN
		VkSurfaceKHR createWindowSurface(HWND hWnd)
		{
			VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
			surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			surfaceInfo.hwnd = hWnd;
			surfaceInfo.hinstance = GetModuleHandleA(NULL);
			VkSurfaceKHR result;
			VERIFY_VK_FAILCDOE(vkCreateWin32SurfaceKHR(mInstance, &surfaceInfo, gAllocCB, &result), return VK_NULL_HANDLE;);
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
						return availablePresentMode;
					}
					else if( availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR )
					{
						bestMode = availablePresentMode;
					}
				}
				return VK_PRESENT_MODE_FIFO_KHR;
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

			VERIFY_VK_RETURN_FALSE(vkCreateSwapchainKHR(mDevice, &swapChainInfo, gAllocCB, &mSwapChain));

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
				VERIFY_VK_RETURN_FALSE(vkCreateImageView(mDevice, &imageViewInfo, gAllocCB, &imageView));
				mSwapChainImageViews.push_back(imageView);
			}

			return true;
		}


		bool createSwapchainFrameBuffers()
		{
			mSwapChainFramebuffers.resize(mSwapChainImageViews.size());

			for( int i = 0; i < mSwapChainImageViews.size(); ++i )
			{
				VkImageView attachments[] = { mSwapChainImageViews[i] };
				VkFramebufferCreateInfo frameBufferInfo = {};
				frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				frameBufferInfo.renderPass = mSimpleRenderPass;
				frameBufferInfo.pAttachments = attachments;
				frameBufferInfo.attachmentCount = ARRAY_SIZE(attachments);
				frameBufferInfo.width = mSwapChainExtent.width;
				frameBufferInfo.height = mSwapChainExtent.height;
				frameBufferInfo.layers = 1;

				VERIFY_VK_RETURN_FALSE(vkCreateFramebuffer(mDevice, &frameBufferInfo, gAllocCB, &mSwapChainFramebuffers[i]));
			}

			return true;
		}

		void cleanupWindowSwapchain()
		{

			if( mSwapChainFramebuffers.empty() )
			{
				for( auto buffer : mSwapChainFramebuffers )
				{
					vkDestroyFramebuffer(mDevice, buffer, gAllocCB);
				}
				mSwapChainFramebuffers.clear();
			}
			mSwapChainImages.clear();

			if( !mSwapChainImageViews.empty() )
			{
				for( auto view : mSwapChainImageViews )
				{
					vkDestroyImageView(mDevice, view, gAllocCB);
				}
				mSwapChainImageViews.clear();
			}

			SAFE_VK_DESTROY(mSwapChain, vkDestroySwapchainKHR(mDevice, mSwapChain, gAllocCB));
		}




		VkPipeline mSimplePipeline = VK_NULL_HANDLE;
		VkPipelineLayout mEmptyPipelineLayout = VK_NULL_HANDLE;
		VkRenderPass mSimpleRenderPass = VK_NULL_HANDLE;


		struct BufferData
		{
			VkBuffer handle;
			VkDeviceMemory memory;
		};
		BufferData mVertexBuffer;
		BufferData mIndexBuffer;

		bool createSimplePipepline()
		{

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

				VkSubpassDependency dependency = {};
				dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
				dependency.dstSubpass = 0;
				dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependency.srcAccessMask = 0;
				dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

				VkRenderPassCreateInfo renderPassInfo = {};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
				renderPassInfo.pAttachments = &colorAttachmentDesc;
				renderPassInfo.attachmentCount = 1;
				renderPassInfo.pSubpasses = &subpass;
				renderPassInfo.subpassCount = 1;
				renderPassInfo.pDependencies = &dependency;
				renderPassInfo.dependencyCount = 1;

				VERIFY_VK_RETURN_FALSE(vkCreateRenderPass(mDevice, &renderPassInfo, gAllocCB, &mSimpleRenderPass));

			}

			{
				VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutInfo.pSetLayouts = nullptr;
				pipelineLayoutInfo.setLayoutCount = 0;
				pipelineLayoutInfo.pPushConstantRanges = nullptr;
				pipelineLayoutInfo.pushConstantRangeCount = 0;
				VERIFY_VK_RETURN_FALSE(vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, gAllocCB, &mEmptyPipelineLayout));
			}

			struct Vertex
			{
				Vector2 pos;
				Vector3 color;
			};

			Vertex v[] =
			{
				Vector2(0.0, -0.5),Vector3(1.0, 0.0, 0.0),
				Vector2(0.5, 0.5), Vector3(0.0, 1.0, 0.0),
				Vector2(-0.5, 0.5),Vector3(1.0, 1.0, 1.0),
			};
			VERIFY_RETURN_FALSE(createBuffer(mVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(Vertex), ARRAY_SIZE(v), v));
			uint32 indices[] =
			{
				0,1,2
			};
			VERIFY_RETURN_FALSE(createBuffer(mIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, sizeof(uint32), ARRAY_SIZE(indices), indices));
			VkVertexInputBindingDescription vertexInputBindingDescs[] =
			{
				//binding stride , rate
				{ 0 , sizeof(Vertex) , VK_VERTEX_INPUT_RATE_VERTEX },
			};

			VkVertexInputAttributeDescription vertexInputAttrDescs[] =
			{
				//location binding format offset
				{0 , 0, VK_FORMAT_R32G32_SFLOAT , offsetof(Vertex , pos) },
				{1 , 0, VK_FORMAT_R32G32B32_SFLOAT , offsetof(Vertex , color) },
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

				VkPipelineDynamicStateCreateInfo dynamicState = {};
				dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				dynamicState.pDynamicStates = nullptr;
				dynamicState.dynamicStateCount = 0;

				VkGraphicsPipelineCreateInfo pipelineInfo = {};
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

				pipelineInfo.layout = mEmptyPipelineLayout;

				pipelineInfo.renderPass = mSimpleRenderPass;
				pipelineInfo.subpass = 0;
				pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
				pipelineInfo.basePipelineIndex = 0;

				VERIFY_VK_RETURN_FALSE(vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, gAllocCB, &mSimplePipeline));

				SAFE_VK_DESTROY(vertexModule, vkDestroyShaderModule(mDevice, vertexModule, gAllocCB));
				SAFE_VK_DESTROY(pixelModule, vkDestroyShaderModule(mDevice, pixelModule, gAllocCB));
			}

			return true;
		}

		void cleanupSimplePipeline()
		{
			SAFE_VK_DESTROY(mSimpleRenderPass, vkDestroyRenderPass(mDevice, mSimpleRenderPass, gAllocCB));
			SAFE_VK_DESTROY(mEmptyPipelineLayout, vkDestroyPipelineLayout(mDevice, mEmptyPipelineLayout, gAllocCB));
			SAFE_VK_DESTROY(mSimplePipeline, vkDestroyPipeline(mDevice, mSimplePipeline, gAllocCB));
		}

		VkCommandPool mCommandPool = VK_NULL_HANDLE;
		std::vector< VkCommandBuffer > mCommandBuffers;
		bool createRenderCommand()
		{
			{
				VkCommandPoolCreateInfo poolInfo = {};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.queueFamilyIndex = mUsageQueueFamilyIndices[EQueueFamily::Graphics];
				poolInfo.flags = 0;
				VERIFY_VK_RETURN_FALSE(vkCreateCommandPool(mDevice, &poolInfo, gAllocCB, &mCommandPool));
			}

			{
				mCommandBuffers.resize(mSwapChainFramebuffers.size());
				VkCommandBufferAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = mCommandPool;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandBufferCount = mCommandBuffers.size();
				VERIFY_VK_RETURN_FALSE(vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffers.data()));
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




		bool generateRenderCommand(VkCommandBuffer commandBuffer , VkFramebuffer frameBuffer )
		{

			VkCommandBufferBeginInfo bufferBeginInfo = {};
			bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			bufferBeginInfo.pInheritanceInfo = nullptr;
			VERIFY_VK_RETURN_FALSE(vkBeginCommandBuffer(commandBuffer, &bufferBeginInfo));

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
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSimplePipeline);

				VkBuffer vertexBuffers[] = { mVertexBuffer.handle };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, ARRAY_SIZE(vertexBuffers), vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffer, mIndexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
				vkCmdDraw(commandBuffer, 3, 1, 0, 0);
				vkCmdEndRenderPass(commandBuffer);
			}

			VERIFY_VK_RETURN_FALSE(vkEndCommandBuffer(commandBuffer));

			return true;
		}

		void cleanupRenderCommand()
		{
			SAFE_VK_DESTROY(mCommandPool, vkDestroyCommandPool(mDevice, mCommandPool, gAllocCB));

		}

		VkShaderModule loadShader(char const* pCode , size_t codeSize )
		{
			VkShaderModuleCreateInfo moduleInfo = {};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.pCode = (uint32*)pCode;
			moduleInfo.codeSize = codeSize;
			VkShaderModule module;
			VERIFY_VK_FAILCDOE(vkCreateShaderModule(mDevice, &moduleInfo, gAllocCB, &module), return VK_NULL_HANDLE;);

			return module;
		}


		bool createBuffer(BufferData& bufferData , VkBufferUsageFlags usage ,  uint32 elementSize, uint32 numElement, void* pData )
		{
			uint32 dataSize = elementSize * numElement;
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			bufferInfo.pQueueFamilyIndices = nullptr;
			bufferInfo.queueFamilyIndexCount = 0;
			bufferInfo.size = dataSize;
			VERIFY_VK_FAILCDOE(vkCreateBuffer(mDevice, &bufferInfo, gAllocCB, &bufferData.handle), return false;);

			VkMemoryRequirements memoryRequirments;
			vkGetBufferMemoryRequirements(mDevice, bufferData.handle, &memoryRequirments);
			VkPhysicalDeviceMemoryProperties memoryProperties;
			vkGetPhysicalDeviceMemoryProperties(mUsagePhysicalDevice, &memoryProperties);
			auto GetTypeIndex =[&](uint32 propertyFlags ) -> uint32
			{
				for( uint32 idx = 0; idx < memoryProperties.memoryTypeCount; ++idx )
				{
					if( (memoryRequirments.memoryTypeBits & (1 << idx) ) && 
					    ( memoryProperties.memoryTypes[idx].propertyFlags & propertyFlags) == propertyFlags )
						return idx;
				}
				::LogError("No memory type");
				return 0;
			};
			VkMemoryAllocateInfo memoryInfo = {};
			memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memoryInfo.allocationSize = memoryRequirments.size;
			memoryInfo.memoryTypeIndex = GetTypeIndex(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			vkAllocateMemory(mDevice, &memoryInfo, gAllocCB, &bufferData.memory);
			vkBindBufferMemory(mDevice, bufferData.handle, bufferData.memory, 0);

			if( pData )
			{
				void* pDest;
				VERIFY_VK_FAILCDOE( vkMapMemory(mDevice , bufferData.memory , 0 , dataSize, 0 , &pDest ) , );
				memcpy(pDest, pData, dataSize);
				vkUnmapMemory(mDevice, bufferData.memory);
			}
			return true;
		}

		void destroyBuffer( BufferData& bufferData )
		{
			vkDestroyBuffer(mDevice, bufferData.handle, gAllocCB);
			vkFreeMemory(mDevice, bufferData.memory, gAllocCB);
		}

		VkSemaphore createSemphore()
		{
			VkSemaphoreCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VkSemaphore result;
			VERIFY_VK_FAILCDOE(vkCreateSemaphore(mDevice, &createInfo, gAllocCB, &result), return VK_NULL_HANDLE; );
			return result;
		}

		bool compileShader(char const* inPath, char const* outPath,  Shader::Type type)
		{

			char const* VulkanSDKDir = SystemPlatform::GetEnvironmentVariable("VULKAN_SDK");
			if( VulkanSDKDir == nullptr )
			{
				VulkanSDKDir = "C:/VulkanSDK/1.1.82.1";
			}
			std::string GlslangValidatorPath = VulkanSDKDir;
			GlslangValidatorPath += "/Bin/glslangValidator.exe";
			static const char const* ShaderNames[] =
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
			process.waitToComplete();
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
			std::vector<char> scoreCode;
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
			::Global::GUI().cleanupWidget();

			::Global::GetDrawEngine()->bUsePlatformBuffer = false;

			VERIFY_RETURN_FALSE( initializeSystem( ::Global::GetDrawEngine()->getWindow().getHWnd() ) );
			VERIFY_RETURN_FALSE( createWindowSwapchain() );
			VERIFY_RETURN_FALSE( createSimplePipepline() );
			VERIFY_RETURN_FALSE( createSwapchainFrameBuffers() );
			VERIFY_RETURN_FALSE( createRenderCommand() );

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
			cleanupWindowSwapchain();
			cleanupSystem();
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

			VERIFY_VK_FAILCDOE(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFences[mCurrentFrame]), );

			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.waitSemaphoreCount = ARRAY_SIZE(signalSemaphores);
			presentInfo.pWaitSemaphores = signalSemaphores;

			VkSwapchainKHR swapChains[] = { mSwapChain };
			presentInfo.pSwapchains = swapChains;
			presentInfo.swapchainCount = ARRAY_SIZE(swapChains);

			presentInfo.pImageIndices = &imageIndex;
			presentInfo.pResults = nullptr;
			VERIFY_VK_FAILCDOE(vkQueuePresentKHR(mPresentQueue, &presentInfo) , );

			mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		}

		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;
			switch( key )
			{
			case Keyboard::eR: restart(); break;
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