#pragma once
#ifndef VulkanCommand_H_07161496_178F_4AF8_91DD_4BFDB4E4CF04
#define VulkanCommand_H_07161496_178F_4AF8_91DD_4BFDB4E4CF04

#include "RHICommand.h"
#include "RHICommandListImpl.h"

#include "ShaderCore.h"

#include "VulkanCommon.h"

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

namespace Render
{

	
	extern VKAPI_ATTR VkBool32 VKAPI_CALL DebugVKCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	struct EQueueFamily
	{
		enum Type
		{
			Graphics = 0,
			Present = 1,
			Compute = 2,
			Transfer = 3,
			Count,
		};
	};

	struct VulkanDevice
	{
		/** @brief Physical device representation */
		VkPhysicalDevice physicalDevice;
		/** @brief Logical device representation (application's view of the device) */
		VkDevice logicalDevice;
		/** @brief Properties of the physical device including limits that the application can check against */
		VkPhysicalDeviceProperties properties;
		/** @brief Features of the physical device that an application can use to check if a feature is supported */
		VkPhysicalDeviceFeatures supportFeatures;
		/** @brief Features that have been enabled for use on the physical device */
		VkPhysicalDeviceFeatures enableFeatures;
		/** @brief Memory types and heaps of the physical device */
		VkPhysicalDeviceMemoryProperties memoryProperties;
		/** @brief Queue family properties of the physical device */
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		/** @brief List of extensions supported by the device */
		std::vector<std::string> supportedExtensions;

		/** @brief Set to true when the debug marker extension is detected */
		bool enableDebugMarkers = false;

		VkCommandPool commandPool = VK_NULL_HANDLE;

		/**  @brief Typecast to VkDevice */
		operator VkDevice() { return logicalDevice; };

		/**
		* Default constructor
		*
		* @param physicalDevice Physical device that is to be used
		*/
		VulkanDevice();

		/**
		* Default destructor
		*
		* @note Frees the logical device
		*/
		~VulkanDevice();


		bool initialize(VkPhysicalDevice physicalDevice);
	
		
		/**
		* Get the index of a memory type that has all the requested property bits set
		*
		* @param typeBits Bitmask with bits set for each memory type supported by the resource to request for (from VkMemoryRequirements)
		* @param properties Bitmask of properties for the memory type to request
		* @param (Optional) memTypeFound Pointer to a bool that is set to true if a matching memory type has been found
		*
		* @return Index of the requested memory type
		*/
		int32 getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);
			
		/**
		* Get the index of a queue family that supports the requested queue flags
		*
		* @param queueFlags Queue flags to find a queue family index for
		*
		* @return Index of the queue family index that matches the flags
		*
		*/
		bool getQueueFamilyIndex(VkQueueFlagBits queueFlags, uint32_t& outIndex);

		/**
		* Create the logical device based on the assigned physical device, also gets default queue family indices
		*
		* @param enabledFeatures Can be used to enable certain features upon device creation
		* @param pNextChain Optional chain of pointer to extension structures
		* @param useSwapChain Set to false for headless rendering to omit the swapchain device extensions
		* @param requestedQueueTypes Bit flags specifying the queue types to be requested from the device
		*
		* @return VkResult of the device creation call
		*/
		bool createLogicalDevice(VkPhysicalDeviceFeatures inEnabledFeatures, std::vector<const char*> enabledExtensions, void* pNextChain, bool useSwapChain /*= true*/, TArrayView< uint32 const > const& usageQueueIndices);

		/**
		* Create a buffer on the device
		*
		* @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
		* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
		* @param size Size of the buffer in byes
		* @param buffer Pointer to the buffer handle acquired by the function
		* @param memory Pointer to the memory handle acquired by the function
		* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
		*
		* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
		*/
		bool createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data = nullptr);

		/**
		* Create a buffer on the device
		*
		* @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
		* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
		* @param buffer Pointer to a vk::Vulkan buffer object
		* @param size Size of the buffer in byes
		* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
		*
		* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
		*/
		bool createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VulkanBufferData& bufferData, VkDeviceSize size, void *data = nullptr);

		/**
		* Copy buffer data from src to dst using VkCmdCopyBuffer
		*
		* @param src Pointer to the source buffer to copy from
		* @param dst Pointer to the destination buffer to copy tp
		* @param queue Pointer
		* @param copyRegion (Optional) Pointer to a copy region, if NULL, the whole buffer is copied
		*
		* @note Source and destionation pointers must have the approriate transfer usage flags set (TRANSFER_SRC / TRANSFER_DST)
		*/
		void copyBuffer(VkCommandPool pool, VulkanBufferData *src, VulkanBufferData *dst, VkQueue queue, VkBufferCopy *copyRegion = nullptr);

		/**
		* Create a command pool for allocation command buffers from
		*
		* @param queueFamilyIndex Family index of the queue to create the command pool for
		* @param createFlags (Optional) Command pool creation flags (Defaults to VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
		*
		* @note Command buffers allocated from the created pool can only be submitted to a queue with the same family index
		*
		* @return A handle to the created command buffer
		*/
		VkCommandPool createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		/**
		* Allocate a command buffer from the command pool
		*
		* @param level Level of the new command buffer (primary or secondary)
		* @param pool Command pool from which the command buffer will be allocated
		* @param (Optional) begin If true, recording on the new command buffer will be started (vkBeginCommandBuffer) (Defaults to false)
		*
		* @return A handle to the allocated command buffer
		*/
		VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin = false);

		/**
		* Finish command buffer recording and submit it to a queue
		*
		* @param commandBuffer Command buffer to flush
		* @param queue Queue to submit the command buffer to
		* @param pool Command pool on which the command buffer has been created
		* @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
		*
		* @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
		* @note Uses a fence to ensure command buffer has finished executing
		*/
		void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free = true);

		void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true)
		{
			return flushCommandBuffer(commandBuffer, queue, commandPool, free);
		}
		/**
		* Check if an extension is supported by the (physical device)
		*
		* @param extension Name of the extension to check
		*
		* @return True if the extension is supported (present in the list read at device creation time)
		*/
		bool extensionSupported(std::string extension)
		{
			return (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end());
		}

		/**
		* Select the best-fit depth format for this device from a list of possible depth (and stencil) formats
		*
		* @param checkSamplingSupport Check if the format can be sampled from (e.g. for shader reads)
		*
		* @return The depth format that best fits for the current device
		*
		*/
		bool getSupportedDepthFormat(VkFormat& outFormat, bool bCheckSamplingSupport = false);
	};


	class VulkanSwapChain
	{
	public:

		uint32 getImageCount() const { return mImages.size(); }

		bool initialize(VulkanDevice& device, VkSurfaceKHR surface, uint32 const usageQueueFamilyIndices[], bool bVSync);

		void cleanupResource();

		VulkanDevice* mDevice;

		VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;

		struct FrameImage
		{
			VkImageView view;
			VkImage     image;
		};

		std::vector< VkImageView >   mImageViews;
		std::vector< VkImage >       mImages;
		VkExtent2D                   mImageSize;
		VkFormat                     mImageFormat;

	};

	class VulkanSystem : public RHISystem
	{

	public:
		RHISytemName getName() const { return RHISytemName::Vulkan; }

		virtual bool initialize(RHISystemInitParams const& initParam) override;
		virtual void shutdown() override;
		virtual class ShaderFormat* createShaderFormat() override;

		RHICommandList&  getImmediateCommandList()
		{
			return *mImmediateCommandList;
		}
		RHIRenderWindow* RHICreateRenderWindow(PlatformWindowInfo const& info) { return nullptr; }


		std::vector< VkSemaphore > mImageAvailableSemaphores;
		std::vector< VkSemaphore > mRenderFinishedSemaphores;
		std::vector< VkFence >     mInFlightFences;
		const int   MAX_FRAMES_IN_FLIGHT = 2;
		int mCurrentFrame = 0;

	
		bool initRenderResource()
		{
			mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
			mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
			mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				mImageAvailableSemaphores[i] = createSemphore();
				mRenderFinishedSemaphores[i] = createSemphore();
				vkCreateFence(mDevice->logicalDevice, &fenceInfo, gAllocCB, &mInFlightFences[i]);
			}

			return true;
		}

		void cleanupRenderResource()
		{
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				vkDestroySemaphore(mDevice->logicalDevice, mRenderFinishedSemaphores[i], gAllocCB);
				vkDestroySemaphore(mDevice->logicalDevice, mImageAvailableSemaphores[i], gAllocCB);
				vkDestroyFence(mDevice->logicalDevice, mInFlightFences[i], gAllocCB);
			}
			mRenderFinishedSemaphores.clear();
			mImageAvailableSemaphores.clear();
			mInFlightFences.clear();
		}

		VkSemaphore createSemphore()
		{
			VkSemaphoreCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VkSemaphore result;
			VK_VERIFY_FAILCDOE(vkCreateSemaphore(mDevice->logicalDevice, &createInfo, gAllocCB, &result), return VK_NULL_HANDLE; );
			return result;
		}

		uint32_t mRenderImageIndex;
		bool RHIBeginRender();
		void RHIEndRender(bool bPresent);

		RHITexture1D*    RHICreateTexture1D(
			Texture::Format format, int length,
			int numMipLevel, uint32 creationFlags, void* data)
		{
			return nullptr;
		}

		RHITexture2D*    RHICreateTexture2D(
			Texture::Format format, int w, int h,
			int numMipLevel, int numSamples, uint32 creationFlags,
			void* data, int dataAlign);

		bool initalizeTexture2DInternal(VulkanTexture2D* texture, Texture::Format format, int width, int height, int numMipLevel, int numSamples, uint32 createFlags, void* data, int alignment);

		RHITexture3D*    RHICreateTexture3D(
			Texture::Format format, int sizeX, int sizeY, int sizeZ,
			int numMipLevel, int numSamples, uint32 creationFlags,
			void* data)
		{
			return nullptr;
		}
		RHITextureCube*  RHICreateTextureCube(
			Texture::Format format, int size,
			int numMipLevel, uint32 creationFlags,
			void* data[])
		{
			return nullptr;
		}

		RHITexture2DArray* RHICreateTexture2DArray(
			Texture::Format format, int w, int h, int layerSize,
			int numMipLevel, int numSamples, uint32 creationFlags,
			void* data)
		{
			return nullptr;
		}

		RHITextureDepth* RHICreateTextureDepth(
			Texture::DepthFormat format, int w, int h,
			int numMipLevel, int numSamples)
		{
			return nullptr;
		}

		RHIVertexBuffer*  RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data);
		RHIIndexBuffer*   RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data);

		bool initalizeBufferInternal(VulkanVertexBuffer* buffer, uint32 elementSize, uint32 numElements, uint32 creationFlags, void* data);
		bool initalizeBufferInternal(VulkanIndexBuffer* buffer, uint32 elementSize, uint32 numElements, uint32 creationFlags, void* data);

		void* RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size) { return nullptr; }
		void  RHIUnlockBuffer(RHIVertexBuffer* buffer) {}
		void* RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size) { return nullptr; }
		void  RHIUnlockBuffer(RHIIndexBuffer* buffer) {  }

		RHIFrameBuffer*  RHICreateFrameBuffer() { return nullptr; }

		RHIInputLayout*  RHICreateInputLayout(InputLayoutDesc const& desc);

		RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer);

		bool initializeSamplerStateInternal(VulkanSamplerState* samplerState, SamplerStateInitializer const& initializer);

		RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer);
		RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer);
		RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer);

		RHIShader* RHICreateShader(EShader::Type type);
		RHIShaderProgram* RHICreateShaderProgram();

		bool createInstance(std::vector<VkExtensionProperties> const& availableExtensions, bool enableValidation);
#if SYS_PLATFORM_WIN
		VkSurfaceKHR createWindowSurface(HWND hWnd);
#endif

		uint32       mUsageQueueFamilyIndices[EQueueFamily::Count];
		
		VkInstance   mInstance;
		VkSurfaceKHR mWindowSurface = VK_NULL_HANDLE;
		
		VkDebugUtilsMessengerEXT mCallback = VK_NULL_HANDLE;

		std::vector<const char*> enabledDeviceExtensions;
		std::vector<const char*> enabledInstanceExtensions;

		VulkanDevice*    mDevice;
		VulkanSwapChain* mSwapChain;

		//Handle to the device graphics queue that command buffers are submitted to
		VkQueue       mGraphicsQueue;
		VkQueue       mPresentQueue;
		VkCommandPool mGraphicsCommandPool;

		VkFormat depthFormat;

		RHICommandListImpl* mImmediateCommandList = nullptr;

	};



}//namespace Render


#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

#endif // VulkanCommand_H_07161496_178F_4AF8_91DD_4BFDB4E4CF04
