#pragma once
#ifndef VulkanCommand_H_07161496_178F_4AF8_91DD_4BFDB4E4CF04
#define VulkanCommand_H_07161496_178F_4AF8_91DD_4BFDB4E4CF04

#include "RHICommon.h"
#include "VulkanCommon.h"
#include "RHICommandListImpl.h"
#include "ShaderCore.h"
#include "Core/TypeHash.h"
#include "VulkanShaderCommon.h"

#include <unordered_map>
#include <set>

#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif
#include "GpuProfiler.h"

namespace Render
{
	class VulkanProfileCore;
	
	extern VKAPI_ATTR VkBool32 VKAPI_CALL DebugVKCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	// ==================== Graphics Pipeline State Key ====================
	struct VulkanGraphicsPipelineStateKey
	{
		ShaderBoundStateKey   shaderKey;
		RHIInputLayout*       inputLayout;
		RHIRasterizerState*   rasterizerState;
		RHIBlendState*        blendState;
		RHIDepthStencilState* depthStencilState;
		VkRenderPass          renderPass;
		int                   samples;
		EPrimitive            primitiveType;

		bool operator == (VulkanGraphicsPipelineStateKey const& rhs) const
		{
			return shaderKey == rhs.shaderKey
				&& inputLayout == rhs.inputLayout
				&& rasterizerState == rhs.rasterizerState
				&& blendState == rhs.blendState
				&& depthStencilState == rhs.depthStencilState
				&& renderPass == rhs.renderPass
				&& samples == rhs.samples
				&& primitiveType == rhs.primitiveType;
		}

		uint32 getTypeHash() const
		{
			uint32 result = shaderKey.getTypeHash();
			result = HashCombine(result, inputLayout);
			result = HashCombine(result, rasterizerState);
			result = HashCombine(result, blendState);
			result = HashCombine(result, depthStencilState);
			result = HashCombine(result, renderPass);
			result = HashCombine(result, samples);
			result = HashCombine(result, (uint32)primitiveType);
			return result;
		}
	};

	class VulkanDescriptorPoolManager
	{
	public:
		bool initialize(VkDevice device);
		void release();

		void resetFrame(int frameIndex);
		VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout);

	private:
		VkDescriptorPool createPool();

		VkDevice mDevice = VK_NULL_HANDLE;
		
		static constexpr int NUM_INFLIGHT_SUBMISSIONS = 4;
		int mSubmissionIndex = 0;
		struct FrameData {
			TArray<VkDescriptorPool> activePools;
			TArray<VkDescriptorPool> freePools;
			VkDescriptorPool currentPool = VK_NULL_HANDLE;
		};
		FrameData mFrames[NUM_INFLIGHT_SUBMISSIONS];
	};

	// ==================== VulkanContext ====================
	class VulkanContext : public RHIContext
	{
	public:
		bool initialize(VulkanSystem* system);
		void release();


		void RHIUpdateTexture(RHITexture2D& texture, int ox, int oy, int w, int h, void* data, int level, int dataWidth);
		void RHIUpdateTexture(RHITextureCube& texture, ETexture::Face face, int ox, int oy, int w, int h, void* data, int level, int dataWidth);
		void RHIUpdateBuffer(RHIBuffer& buffer, int start, int numElements, void* data);
		void RHIGenerateMips(RHITextureBase& texture);

		// ---- Render State ----
		void RHISetRasterizerState(RHIRasterizerState& rasterizerState);
		void RHISetBlendState(RHIBlendState& blendState);
		void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef);

		void RHISetViewport(ViewportInfo const& viewport);
		void RHISetViewports(ViewportInfo const viewports[], int numViewports);
		void RHISetScissorRect(int x, int y, int w, int h);

		// ---- Draw Commands ----
		void RHIDrawPrimitive(EPrimitive type, int start, int nv);
		void RHIDrawIndexedPrimitive(EPrimitive type, int indexStart, int nIndex, uint32 baseVertex);
		void RHIDrawPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride) {}
		void RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride) {}
		void RHIDrawPrimitiveInstanced(EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance);
		void RHIDrawIndexedPrimitiveInstanced(EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance);

		bool determitPrimitiveTopologyUP(EPrimitive primitive, int num, uint32 const* pIndices, EPrimitive& outPrimitiveDetermited, VulkanBufferAllocation& outIndexAlloc, int& outIndexNum);
		void RHIDrawPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numData, uint32 numInstance);
		void RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVerex, VertexDataInfo dataInfos[], int numVertexData, uint32 const* pIndices, int numIndex, uint32 numInstance);

		void RHIDrawMeshTasks(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ) {}
		void RHIDrawMeshTasksIndirect(RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride) {}

		void RHISetFixedShaderPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler);

		// ---- Framebuffer / RenderPass ----
		void RHISetFrameBuffer(RHIFrameBuffer* frameBuffer);
		void RHIClearRenderTargets(EClearBits clearBits, LinearColor colors[], int numColor, float depth, uint8 stenceil);

		// Set the swap chain framebuffer as the active back buffer target
		void setSwapChainFrameBuffer(VulkanFrameBuffer* swapChainFB) { mSwapChainFrameBufferRHI = swapChainFB; }

		// ---- Input ----
		void RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream);
		void RHISetIndexBuffer(RHIBuffer* buffer);
		void RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ);

		// ---- Resource Transition ----
		void RHIResourceTransition(TArrayView<RHIResource*> resources, EResourceTransition transition);
		void RHICopyResource(RHIResource& dest, RHIResource& src) {}

		void RHIResolveTexture(RHITextureBase& destTexture, int destSubIndex, RHITextureBase& srcTexture, int srcSubIndex) {}
		void RHIFlushCommand() {}

		// ---- Ray Tracing (stubs) ----
		void RHIBuildAccelerationStructure(RHIAccelerationStructure* dst, RHIAccelerationStructure* src, RHIBuffer* scratch, uint32 numInstances) override {}
		void RHISetRayTracingPipelineState(RHIRayTracingPipelineState* rtpso, RHIRayTracingShaderTable* sbt) {}
		void RHIDispatchRays(uint32 width, uint32 height, uint32 depth) {}
		void RHIUpdateTopLevelAccelerationStructureInstances(RHITopLevelAccelerationStructure* tlas, RayTracingInstanceDesc const instances[], uint32 numInstances, uint32 instanceOffset) {}
		void RHISetShaderAccelerationStructure(RHIShader* shader, char const* name, RHIAccelerationStructure* as) {}

		// ---- Shader ----
		void RHISetShaderProgram(RHIShaderProgram* shaderProgram);

		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, int32 const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim); }
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim); }
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix3 const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim, 3 * sizeof(float)); }
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix4 const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim); }
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector2 const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim, 2 * sizeof(float)); }
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector3 const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim, 3 * sizeof(float)); }
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector4 const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim); }
		void setShaderMatrix22(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim, 2 * sizeof(float)); }
		void setShaderMatrix43(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim, 3 * sizeof(float)); }
		void setShaderMatrix34(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim); }

		template< class T >
		void setShaderValueT(RHIShaderProgram& shaderProgram, ShaderParameter const& param, T const val[], int dim)
		{
			auto& shaderProgramImpl = static_cast<VulkanShaderProgram&>(shaderProgram);
			shaderProgramImpl.mParameterMap.setupShader(param, [this, val, dim](int shaderIndex, ShaderParameter const& shaderParam)
			{
				setShaderValueInternal(shaderParam, (uint8 const*)val, dim * sizeof(T));
			});
		}

		template< class T >
		void setShaderValueT(RHIShaderProgram& shaderProgram, ShaderParameter const& param, T const val[], int dim, uint32 elementSize, uint32 stride = 4 * sizeof(float))
		{
			auto& shaderProgramImpl = static_cast<VulkanShaderProgram&>(shaderProgram);
			shaderProgramImpl.mParameterMap.setupShader(param, [this, val, dim, elementSize, stride](int shaderIndex, ShaderParameter const& shaderParam)
			{
				setShaderValueInternal(shaderParam, (uint8 const*)val, dim * sizeof(T), elementSize, stride);
			});
		}

		void setShaderResourceView(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIShaderResourceView const& resourceView);

		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture);
		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler);
		void clearShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param);

		void setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler);

		void setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op);
		void setShaderRWSubTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, int subIndex, int level, EAccessOp op);
		void clearShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param);

		void setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer);
		void setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op);
		void setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer);
		void clearShaderBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, EAccessOp op);

		void RHISetGraphicsShaderBoundState(GraphicsShaderStateDesc const& stateDesc);
		void RHISetMeshShaderBoundState(MeshShaderStateDesc const& stateDesc);
		void RHISetComputeShader(RHIShader* shader);

		void setShaderValue(RHIShader& shader, ShaderParameter const& param, int32 const val[], int dim) { setShaderValueT(shader, param, val, dim); }
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shader, param, val, dim); }
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix3 const val[], int dim) { setShaderValueT(shader, param, val, dim, 3 * sizeof(float)); }
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix4 const val[], int dim) { setShaderValueT(shader, param, val, dim); }
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector2 const val[], int dim) { setShaderValueT(shader, param, val, dim, 2 * sizeof(float)); }
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector3 const val[], int dim) { setShaderValueT(shader, param, val, dim, 3 * sizeof(float)); }
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector4 const val[], int dim) { setShaderValueT(shader, param, val, dim); }
		void setShaderMatrix22(RHIShader& shader, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shader, param, val, dim, 2 * sizeof(float)); }
		void setShaderMatrix43(RHIShader& shader, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shader, param, val, dim, 3 * sizeof(float)); }
		void setShaderMatrix34(RHIShader& shader, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shader, param, val, dim); }

		template< class T >
		void setShaderValueT(RHIShader& shader, ShaderParameter const& param, T const val[], int dim) 
		{
			setShaderValueInternal(param, (uint8 const*)val, dim * sizeof(T));
		}
		template< class T >
		void setShaderValueT(RHIShader& shader, ShaderParameter const& param, T const val[], int dim, uint32 elementSize, uint32 stride = 4 * sizeof(float))
		{
			setShaderValueInternal(param, (uint8 const*)val, dim * sizeof(T), elementSize, stride);
		}

		void setShaderValueInternal(ShaderParameter const& param, uint8 const* pData, uint32 size, uint32 elementSize, uint32 stride = 4 * sizeof(float));
		void setShaderValueInternal(ShaderParameter const& param, uint8 const* pData, uint32 size);

		void setShaderResourceView(RHIShader& shader, ShaderParameter const& param, RHIShaderResourceView const& resourceView);

		void setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture);
		void setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler);
		void clearShaderTexture(RHIShader& shader, ShaderParameter const& param);

		void setShaderSampler(RHIShader& shader, ShaderParameter const& param, RHISamplerState& sampler);
		void setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op);
		void setShaderRWSubTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, int subIndex, int level, EAccessOp op);
		void clearShaderRWTexture(RHIShader& shader, ShaderParameter const& param);

		void setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer);
		void setShaderStorageBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op);
		void setShaderAtomicCounterBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer);
		void clearShaderBuffer(RHIShader& shader, ShaderParameter const& param, EAccessOp op);

		// ---- Internal ----
		void commitGraphicsShaderState(EPrimitive primitiveType);
		void commitComputeShaderState();
		void commitDescriptorSets();
		void commitInputStreams();
		void setAsActive();

		void beginRenderPass();
		void endRenderPass();

		VkPipeline getOrCreateGraphicsPipeline(VulkanGraphicsPipelineStateKey const& key);
		VkPipeline getOrCreateComputePipeline(RHIShader* computeShader);

		VkCommandBuffer getActiveCommandBuffer() const { return mActiveCmdBuffer; }

		// ---- Members ----
		VulkanSystem*   mSystem = nullptr;
		VulkanDevice*   mDevice = nullptr;
		VkCommandBuffer mActiveCmdBuffer = VK_NULL_HANDLE;
		bool mInsideRenderPass = false;

		// Pending state tracking
		ShaderBoundStateKey     mPendingShaderKey;
		RHIShaderProgramRef     mPendingShaderProgram;
		RHIShaderRef           mPendingComputeShader;
		RHIInputLayoutRef       mPendingInputLayout;
		RHIRasterizerStateRef   mPendingRasterizerState;
		RHIBlendStateRef        mPendingBlendState;
		RHIDepthStencilStateRef mPendingDepthStencilState;
		uint32                  mPendingStencilRef = 0;

		// Bound pipeline tracking
		VkPipeline mBoundPipeline = VK_NULL_HANDLE;
		VkDescriptorSet mBoundDescriptorSet = VK_NULL_HANDLE;
		VkCommandBuffer mBoundCmdBuffer = VK_NULL_HANDLE;
		VkPipelineLayout mBoundPipelineLayout = VK_NULL_HANDLE;
		VkRenderPass mActiveRenderPass = VK_NULL_HANDLE;
		VulkanFrameBuffer* mActiveFrameBuffer = nullptr;
		VulkanFrameBuffer* mSwapChainFrameBufferRHI = nullptr;

		// Input stream state
		static constexpr int MaxInputStream = 16;
		InputStreamInfo mInputStreams[MaxInputStream];
		int  mNumInputStream = 0;
		bool mbInputStreamDirty = false;

		RHIBufferRef mPendingIndexBuffer;

		struct PendingDescriptor
		{
			VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
			VulkanTexture* texture = nullptr;
			VkImageView view = VK_NULL_HANDLE;
			VulkanSamplerState* sampler = nullptr;
			VulkanBuffer* buffer = nullptr;

			bool bIsGlobalConst = false;
			VulkanBufferAllocation globalConstAlloc;

			int level = 0;
			int indexLayer = 0;
		};

		static constexpr int MaxDescriptorBindings = 128;
		PendingDescriptor mPendingDescriptors[MaxDescriptorBindings] = {};
		bool mDescriptorDirty = true;
		
		TArray<uint8> mPendingUniformBuffers[MaxDescriptorBindings];
		bool mPendingUniformDirty[MaxDescriptorBindings] = {};
		
		VulkanDescriptorPoolManager mDescriptorPoolManager;

		struct FixedShaderParams
		{
			Matrix4 transform;
			LinearColor color;
			RHITexture2D* texture;
			RHISamplerState* sampler;
		};
		FixedShaderParams mFixedShaderParams;
		bool bUseFixedShaderPipeline = false;

		// Pipeline cache
		std::unordered_map< VulkanGraphicsPipelineStateKey, VkPipeline, MemberFuncHasher > mPipelineCache;
		std::unordered_map< ShaderBoundStateKey, VkPipeline, MemberFuncHasher > mComputePipelineCache;
	};



	class VulkanSwapChainData
	{
	public:

		uint32 getImageCount() const { return (uint32)mImages.size(); }

		bool initialize(VulkanDevice& device, VkSurfaceKHR surface, uint32 const usageQueueFamilyIndices[], VkFormat depthFormat, int numSamples, bool bVSync);

		void cleanupResource();

		VulkanDevice* mDevice;

		VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;

		struct FrameImage
		{
			VkImageView view;
			VkImage     image;
		};

		TArray< VkImageView >   mImageViews;
		TArray< VkImage >       mImages;
		VkExtent2D              mImageSize;
		VkFormat                mImageFormat;
		VkFormat                mDepthFormat;
		int                     mNumSamples = 1;
		VkImage                 mMSAAColorImage = VK_NULL_HANDLE;
		VkImageView             mMSAAColorView = VK_NULL_HANDLE;
		VkDeviceMemory          mMSAAColorMemory = VK_NULL_HANDLE;

		VkImage                 mDepthImage = VK_NULL_HANDLE;
		VkImageView             mDepthView = VK_NULL_HANDLE;
		VkDeviceMemory          mDepthMemory = VK_NULL_HANDLE;

	};





	class VulkanSystem : public RHISystem
	{

	public:
		RHISystemName getName() const { return RHISystemName::Vulkan; }

		bool initialize(RHISystemInitParams const& initParam) override;
		void shutdown() override;
		class ShaderFormat* createShaderFormat() override;
		RHIProfileCore* createProfileCore();

		RHICommandList&  getImmediateCommandList()
		{
			return *mImmediateCommandList;
		}
		RHISwapChain* RHICreateSwapChain(SwapChainCreationInfo const& info) override;


		TArray< VkSemaphore > mImageAvailableSemaphores;
		TArray< VkSemaphore > mRenderFinishedSemaphores;
		TArray< VkFence >     mInFlightFences;
		TArray< VkCommandBuffer > mCommandBuffers;
		const int   NUM_INFLIGHT_SUBMISSIONS = 4;
		int mSubmissionIndex = 0;

	
		bool initRenderResource()
		{
			mImageAvailableSemaphores.resize(NUM_INFLIGHT_SUBMISSIONS);
			mRenderFinishedSemaphores.resize(NUM_INFLIGHT_SUBMISSIONS);
			mInFlightFences.resize(NUM_INFLIGHT_SUBMISSIONS);
			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			for (int i = 0; i < NUM_INFLIGHT_SUBMISSIONS; ++i)
			{
				mImageAvailableSemaphores[i] = createSemphore();
				vkCreateFence(mDevice->logicalDevice, &fenceInfo, gAllocCB, &mInFlightFences[i]);
			}

			mCommandBuffers.resize(NUM_INFLIGHT_SUBMISSIONS);
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = mGraphicsCommandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = (uint32_t) mCommandBuffers.size();
			VK_VERIFY_RETURN_FALSE(vkAllocateCommandBuffers(mDevice->logicalDevice, &allocInfo, mCommandBuffers.data()));

			return true;
		}

		void cleanupRenderResource()
		{
			for (size_t i = 0; i < NUM_INFLIGHT_SUBMISSIONS; i++)
			{
				vkDestroySemaphore(mDevice->logicalDevice, mRenderFinishedSemaphores[i], gAllocCB);
				vkDestroySemaphore(mDevice->logicalDevice, mImageAvailableSemaphores[i], gAllocCB);
				vkDestroyFence(mDevice->logicalDevice, mInFlightFences[i], gAllocCB);
			}
			vkFreeCommandBuffers(mDevice->logicalDevice, mGraphicsCommandPool, (uint32_t)mCommandBuffers.size(), mCommandBuffers.data());
			mCommandBuffers.clear();
			mImageAvailableSemaphores.clear();
			mInFlightFences.clear();
		}

		VkSemaphore createSemphore()
		{
			VkSemaphoreCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VkSemaphore result;
			VK_VERIFY_FAILCODE(vkCreateSemaphore(mDevice->logicalDevice, &createInfo, gAllocCB, &result), return VK_NULL_HANDLE; );
			return result;
		}

		uint32 mRenderImageIndex;
		uint64 mSubmissionCount = 0;
		bool mbAdvanceFrame = false;
		bool mbInRendering = false;
		bool RHIBeginRender(bool bAdvanceFrame);
		void RHIEndRender(bool bPresent);
		bool RHIIsInRendering() const override { return mbInRendering; }

		// Swap chain framebuffer management
		bool createSwapChainRenderPass();
		bool createSwapChainFramebuffers();
		void recreateSwapChain();
		void cleanupSwapChainRenderPass();
		VkRenderPass                mSwapChainRenderPass = VK_NULL_HANDLE;
		TArray< VkFramebuffer >     mSwapChainFramebuffers;
		VulkanFrameBuffer           mSwapChainFrameBuffer;

		bool initializeTextureInternal(VulkanTexture* texture, TextureDesc const& desc, void* data, int alignment);

		RHITexture1D*      RHICreateTexture1D(TextureDesc const& desc, void* data);
		virtual RHITexture2D* RHICreateTexture2D(TextureDesc const& desc, void* data, int dataAlign) override;
		virtual RHIShaderResourceView* RHICreateSRV(RHITexture2D& texture, ETexture::Format format) override;
		RHITexture3D*      RHICreateTexture3D(TextureDesc const& desc, void* data);
		RHITextureCube*    RHICreateTextureCube(TextureDesc const& desc, void* data[]);
		RHITexture2DArray* RHICreateTexture2DArray(TextureDesc const& desc, void* data);

		RHIBuffer*  RHICreateBuffer(BufferDesc const& desc, void* data);

		bool initalizeBufferInternal(VulkanBuffer* buffer, BufferDesc const& desc, void* data);

		void* RHILockBuffer(RHIBuffer* buffer, ELockAccess access, uint32 offset, uint32 size);
		void  RHIUnlockBuffer(RHIBuffer* buffer);

		bool RHIUpdateTexture(RHITexture2D& texture, int ox, int oy, int w, int h, void* data, int level, int dataWidth);

		void RHIReadTexture(RHITexture2D& texture, ETexture::Format format, int level, TArray< uint8 >& outData)
		{

		}

		VulkanShaderBoundState* getShaderBoundState(GraphicsShaderStateDesc const& stateDesc);
		VulkanShaderBoundState* getShaderBoundState(MeshShaderStateDesc const& stateDesc);

		std::unordered_map< ShaderBoundStateKey, VulkanShaderBoundState*, MemberFuncHasher > mShaderBoundStateCache;
		void RHIReadTexture(RHITextureCube& texture, ETexture::Format format, int level, TArray< uint8 >& outData)
		{

		}

		RHIFrameBuffer*  RHICreateFrameBuffer();

		RHIInputLayout*  RHICreateInputLayout(InputLayoutDesc const& desc);

		RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer);

		bool initializeSamplerStateInternal(VulkanSamplerState* samplerState, SamplerStateInitializer const& initializer);

		RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer);
		RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer);
		RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer);

		RHIShader* RHICreateShader(EShader::Type type);
		RHIShaderProgram* RHICreateShaderProgram();

		RHIRayTracingPipelineState* RHICreateRayTracingPipelineState(RayTracingPipelineStateInitializer const& initializer) { return nullptr; }
		RHIBottomLevelAccelerationStructure* RHICreateBottomLevelAccelerationStructure(RayTracingGeometryDesc const* geometries, int numGeometries, EAccelerationStructureBuildFlags flags) { return nullptr; }
		RHITopLevelAccelerationStructure* RHICreateTopLevelAccelerationStructure(uint32 numInstances, EAccelerationStructureBuildFlags flags) { return nullptr; }
		RHIRayTracingShaderTable* RHICreateRayTracingShaderTable(RHIRayTracingPipelineState* pipelineState) { return nullptr; }

		bool createInstance(TArray<VkExtensionProperties> const& availableExtensions, bool enableValidation);
#if SYS_PLATFORM_WIN
		VkSurfaceKHR createWindowSurface(HWND hWnd);
#endif

		uint32       mUsageQueueFamilyIndices[EQueueFamily::Count];
		
		VkInstance   mInstance;
		VkSurfaceKHR mWindowSurface = VK_NULL_HANDLE;
		
		VkDebugUtilsMessengerEXT mCallback = VK_NULL_HANDLE;

		TArray<const char*> enabledDeviceExtensions;
		TArray<const char*> enabledInstanceExtensions;

		VulkanDevice*    mDevice;
		VulkanSwapChainData* mSwapChain;

		//Handle to the device graphics queue that command buffers are submitted to
		VkQueue       mGraphicsQueue;
		VkQueue       mPresentQueue;
		VkCommandPool mGraphicsCommandPool;

		VkFormat depthFormat;
		int      mNumSamples = 1;

		VulkanContext       mDrawContext;
		RHICommandListImpl* mImmediateCommandList = nullptr;

		VulkanProfileCore*  mProfileCore = nullptr;

		// Pipeline cache (cleanup on shutdown)
		void cleanupPipelineCache();
	};


	class VulkanProfileCore : public RHIProfileCore
	{
	public:
		static constexpr int NUM_FRAME_BUFFER = 8;

		VulkanProfileCore()
		{
			mCycleToMillisecond = 0;
			mCurFrameIndex = 0;
		}

		bool init(VulkanDevice* device, double cycleToMillisecond)
		{
			mDevice = device;
			mCycleToMillisecond = cycleToMillisecond;
			for (int i = 0; i < NUM_FRAME_BUFFER; ++i)
			{
				VERIFY_RETURN_FALSE(addChunkResource(i));
				mNextHandle[i] = 1; // 0 is reserved for frame start/end
			}
			return true;
		}

		struct TimeStampChunk
		{
			VkQueryPool queryPool = VK_NULL_HANDLE;
			VkBuffer resultBuffer = VK_NULL_HANDLE;
			VkDeviceMemory resultMemory = VK_NULL_HANDLE;
			uint64* timeStampData = nullptr;

			static constexpr uint32 Size = 512;
			void release(VkDevice device)
			{
				if (timeStampData) vkUnmapMemory(device, resultMemory);
				if (queryPool) vkDestroyQueryPool(device, queryPool, gAllocCB);
				if (resultBuffer) vkDestroyBuffer(device, resultBuffer, gAllocCB);
				if (resultMemory) vkFreeMemory(device, resultMemory, gAllocCB);
			}
		};

		bool addChunkResource(int frameIndex)
		{
			TimeStampChunk chunk;
			VkQueryPoolCreateInfo queryPoolInfo = {};
			queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
			queryPoolInfo.queryCount = TimeStampChunk::Size * 2;
			VERIFY_RETURN_FALSE(vkCreateQueryPool(mDevice->logicalDevice, &queryPoolInfo, gAllocCB, &chunk.queryPool) == VK_SUCCESS);

			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = sizeof(uint64) * TimeStampChunk::Size * 2;
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			VERIFY_RETURN_FALSE(vkCreateBuffer(mDevice->logicalDevice, &bufferInfo, gAllocCB, &chunk.resultBuffer) == VK_SUCCESS);

			VkMemoryRequirements memReqs;
			vkGetBufferMemoryRequirements(mDevice->logicalDevice, chunk.resultBuffer, &memReqs);
			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memReqs.size;
			allocInfo.memoryTypeIndex = mDevice->getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VERIFY_RETURN_FALSE(vkAllocateMemory(mDevice->logicalDevice, &allocInfo, gAllocCB, &chunk.resultMemory) == VK_SUCCESS);
			vkBindBufferMemory(mDevice->logicalDevice, chunk.resultBuffer, chunk.resultMemory, 0);
			vkMapMemory(mDevice->logicalDevice, chunk.resultMemory, 0, VK_WHOLE_SIZE, 0, (void**)&chunk.timeStampData);

			if (mCmdList != VK_NULL_HANDLE && frameIndex == mRecordingFrameIndex)
			{
				vkCmdResetQueryPool(mCmdList, chunk.queryPool, 0, TimeStampChunk::Size * 2);
			}

			mFrameChunks[frameIndex].push_back(chunk);
			return true;
		}

		virtual void onBeginRhiFrame(void* context) override
		{
			onBeginRender(*(VulkanContext*)context);
		}

		virtual void onEndRhiFrame(void* context) override
		{
			onEndRender(*(VulkanContext*)context);
		}

		void onBeginRender(VulkanContext& context)
		{
			VkCommandBuffer cmdBuffer = context.getActiveCommandBuffer();
			mCmdList = cmdBuffer;
			mRecordingFrameIndex = mCurFrameIndex; // Capture current app frame index for this RHI recording session

			if (cmdBuffer != VK_NULL_HANDLE && mFrameChunks[mRecordingFrameIndex].size() > 0)
			{
				if (bRecordingStarted == false)
				{
					for (auto& chunk : mFrameChunks[mRecordingFrameIndex])
					{
						vkCmdResetQueryPool(cmdBuffer, chunk.queryPool, 0, TimeStampChunk::Size * 2);
					}
					// Frame boundary markers
					vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mFrameChunks[mRecordingFrameIndex][0].queryPool, 0);
					bRecordingStarted = true;
				}

				for (uint32 handle : mPendingStartHandles)
				{
					uint32 chunkIndex = handle / TimeStampChunk::Size;
					uint32 index = 2 * (handle % TimeStampChunk::Size);
					if (chunkIndex < mFrameChunks[mRecordingFrameIndex].size())
					{
						vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mFrameChunks[mRecordingFrameIndex][chunkIndex].queryPool, index + 0);
						mActiveHandles.insert(handle);
					}
				}
				mPendingStartHandles.clear();
			}
		}

		virtual void beginFrame() override
		{
			mNextHandle[mCurFrameIndex] = 1;
			mRecordingFrameIndex = mCurFrameIndex;
			bRecordingStarted = false;
			mPendingStartHandles.clear();
		}

		virtual bool endFrame() override
		{
			mNextHandle[mCurFrameIndex] = 1; // 0 is reserved
			mCurFrameIndex = (mCurFrameIndex + 1) % NUM_FRAME_BUFFER;
			bRecordingStarted = false;
			mPendingStartHandles.clear();
			return true;
		}

		void onEndRender(VulkanContext& context)
		{
			VkCommandBuffer cmdBuffer = context.getActiveCommandBuffer();
			if (cmdBuffer != VK_NULL_HANDLE && mFrameChunks[mRecordingFrameIndex].size() > 0)
			{
				for (uint32 handle : mActiveHandles)
				{
					uint32 chunkIndex = handle / TimeStampChunk::Size;
					uint32 index = 2 * (handle % TimeStampChunk::Size);
					if (chunkIndex < mFrameChunks[mRecordingFrameIndex].size())
					{
						vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mFrameChunks[mRecordingFrameIndex][chunkIndex].queryPool, index + 1);
					}
				}
				// Frame boundary markers end
				vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mFrameChunks[mRecordingFrameIndex][0].queryPool, 1);
			}
			mCmdList = VK_NULL_HANDLE;
		}

		virtual void beginReadback() override
		{
		}

		virtual void endReadback() override
		{
		}

		virtual uint32 fetchTiming() override
		{
			uint32 frameIndex = mCurFrameIndex;
			uint32 handle = mNextHandle[frameIndex];
			uint32 chunkIndex = handle / TimeStampChunk::Size;
			uint32 index = 2 * (handle % TimeStampChunk::Size);

			if (index == 0 && handle > 0)
			{
				if (!addChunkResource(frameIndex))
					return uint32(-1);
			}

			uint32 result = (frameIndex << 24) | handle;
			++mNextHandle[frameIndex];
			return result;
		}

		virtual void startTiming(uint32 timingHandle) override
		{
			uint32 frameIndex = timingHandle >> 24;
			uint32 handle = timingHandle & 0xffffff;

			if (mCmdList == VK_NULL_HANDLE)
			{
				if (frameIndex == mRecordingFrameIndex)
					mPendingStartHandles.push_back(handle);
				return;
			}

			uint32 chunkIndex = handle / TimeStampChunk::Size;
			uint32 index = 2 * (handle % TimeStampChunk::Size);
			if (chunkIndex < mFrameChunks[frameIndex].size())
			{
				vkCmdWriteTimestamp(mCmdList, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mFrameChunks[frameIndex][chunkIndex].queryPool, index + 0);
				mActiveHandles.insert(handle);
			}
		}

		virtual void endTiming(uint32 timingHandle) override
		{
			uint32 frameIndex = timingHandle >> 24;
			uint32 handle = timingHandle & 0xffffff;
			mActiveHandles.erase(handle);

			if (mCmdList == VK_NULL_HANDLE)
				return;

			uint32 chunkIndex = handle / TimeStampChunk::Size;
			uint32 index = 2 * (handle % TimeStampChunk::Size);
			if (chunkIndex < mFrameChunks[frameIndex].size())
			{
				vkCmdWriteTimestamp(mCmdList, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mFrameChunks[frameIndex][chunkIndex].queryPool, index + 1);
			}
		}

		virtual bool getTimingDuration(uint32 timingHandle, uint64& outDuration, uint64& outStart) override
		{
			uint32 frameIndex = timingHandle >> 24;
			uint32 handle = timingHandle & 0xffffff;
			uint32 chunkIndex = handle / TimeStampChunk::Size;
			uint32 index = 2 * (handle % TimeStampChunk::Size);

			if (chunkIndex >= mFrameChunks[frameIndex].size())
				return false;

			auto& chunk = mFrameChunks[frameIndex][chunkIndex];
			uint64 results[2] = { 0, 0 };

			// Use VK_QUERY_RESULT_64_BIT to get 64-bit results.
			// Do NOT use VK_QUERY_RESULT_WAIT_BIT to avoid CPU stalling.
			VkResult res = vkGetQueryPoolResults(mDevice->logicalDevice, chunk.queryPool, index, 2, sizeof(results), results, sizeof(uint64), VK_QUERY_RESULT_64_BIT);
			if (res == VK_SUCCESS)
			{
				outDuration = results[1] - results[0];
				outStart = results[0];
				return true;
			}

			return false;
		}

		virtual double getCycleToMillisecond() override { return mCycleToMillisecond; }

		virtual void releaseRHI() override
		{
			for (int i = 0; i < NUM_FRAME_BUFFER; ++i)
			{
				for (auto& chunk : mFrameChunks[i])
				{
					chunk.release(mDevice->logicalDevice);
				}
				mFrameChunks[i].clear();
			}
		}

		double mCycleToMillisecond;
		uint32 mNextHandle[NUM_FRAME_BUFFER];
		TArray< TimeStampChunk > mFrameChunks[NUM_FRAME_BUFFER];
		int mCurFrameIndex;
		int mRecordingFrameIndex = 0;
		bool bRecordingStarted = false;
		TArray<uint32> mPendingStartHandles;
		std::set<uint32> mActiveHandles;
		VkCommandBuffer mCmdList = VK_NULL_HANDLE;
		VulkanDevice* mDevice = nullptr;
	};




	class VulkanSwapChain : public TRefcountResource< RHISwapChain >
		, public VulkanSwapChainData
	{
	public:
		VulkanSwapChain(VulkanSystem* system)
			: mSystem(system)
		{
		}

		~VulkanSwapChain()
		{
			releaseResource();
		}

		bool initialize(SwapChainCreationInfo const& info)
		{
#if SYS_PLATFORM_WIN
			mSurface = mSystem->createWindowSurface(info.windowHandle);
#endif
			if (!VulkanSwapChainData::initialize(*mSystem->mDevice, mSurface, mSystem->mUsageQueueFamilyIndices, mSystem->mSwapChain->mDepthFormat, info.numSamples, false))
				return false;

			return true;
		}

		virtual void resizeBuffer(int w, int h) override
		{
			// Not fully implemented for additional swapchains yet
		}

		virtual RHITexture2D* getBackBufferTexture() override
		{
			return nullptr;
		}

		virtual RHITexture2D* getDepthTexture() override
		{
			return nullptr;
		}

		virtual void present(bool bVSync) override
		{
			// Need custom submit and present logic if used outside main swapchain
		}

		virtual void releaseResource() override
		{
			VulkanSwapChainData::cleanupResource();
			if (mSurface)
			{
				vkDestroySurfaceKHR(mSystem->mInstance, mSurface, nullptr);
				mSurface = VK_NULL_HANDLE;
			}
		}

		VulkanSystem* mSystem;
		VkSurfaceKHR  mSurface = VK_NULL_HANDLE;
	};


}//namespace Render


#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

#endif // VulkanCommand_H_07161496_178F_4AF8_91DD_4BFDB4E4CF04
