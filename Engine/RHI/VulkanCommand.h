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
		RHISystemName getName() const { return RHISystemName::Vulkan; }

		virtual bool initialize(RHISystemInitParams const& initParam) override;
		virtual void shutdown() override;
		virtual class ShaderFormat* createShaderFormat() override;

		RHICommandList&  getImmediateCommandList()
		{
			return *mImmediateCommandList;
		}
		RHISwapChain* RHICreateSwapChain(SwapChainCreationInfo const& info) { return nullptr; }


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

		uint32 mRenderImageIndex;
		bool RHIBeginRender();
		void RHIEndRender(bool bPresent);

		RHITexture1D*    RHICreateTexture1D(
			ETexture::Format format, int length,
			int numMipLevel, uint32 creationFlags, void* data)
		{
			return nullptr;
		}

		RHITexture2D*    RHICreateTexture2D(
			ETexture::Format format, int w, int h,
			int numMipLevel, int numSamples, uint32 creationFlags,
			void* data, int dataAlign);

		bool initalizeTexture2DInternal(VulkanTexture2D* texture, ETexture::Format format, int width, int height, int numMipLevel, int numSamples, uint32 createFlags, void* data, int alignment);

		RHITexture3D*    RHICreateTexture3D(
			ETexture::Format format, int sizeX, int sizeY, int sizeZ,
			int numMipLevel, int numSamples, uint32 creationFlags,
			void* data)
		{
			return nullptr;
		}
		RHITextureCube*  RHICreateTextureCube(
			ETexture::Format format, int size,
			int numMipLevel, uint32 creationFlags,
			void* data[])
		{
			return nullptr;
		}

		RHITexture2DArray* RHICreateTexture2DArray(
			ETexture::Format format, int w, int h, int layerSize,
			int numMipLevel, int numSamples, uint32 creationFlags,
			void* data)
		{
			return nullptr;
		}

		RHITexture2D* RHICreateTextureDepth(
			ETexture::Format format, int w, int h,
			int numMipLevel, int numSamples, uint32 creationFlags)
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

		void RHIReadTexture(RHITexture2D& texture, ETexture::Format format, int level, std::vector< uint8 >& outData)
		{

		}
		void RHIReadTexture(RHITextureCube& texture, ETexture::Format format, int level, std::vector< uint8 >& outData)
		{

		}

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
