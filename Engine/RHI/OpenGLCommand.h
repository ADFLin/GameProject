#pragma once
#ifndef OpenGLCommand_H_B1DE1168_106C_49AB_9275_1AA61D14E11D
#define OpenGLCommand_H_B1DE1168_106C_49AB_9275_1AA61D14E11D

#include "RHICommand.h"
#include "RHIGlobalResource.h"
#include "RHICommandListImpl.h"

#include "ShaderCore.h"
#include "glew/GL/glew.h"
#include "GLExtensions.h"

#include "OpenGLCommon.h"

#include "WGLContext.h"

#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

namespace Render
{
	struct OpenGLDeviceState
	{
		OpenGLDeviceState()
		{
			Initialize();
		}
		RHIRasterizerState* rasterizerStateCommitted;
		RHIRasterizerState* rasterizerStatePending;
		GLRasterizerStateValue rasterizerStateValue;

		RHIDepthStencilState* depthStencilStateCommitted;
		RHIDepthStencilState* depthStencilStatePending;
		uint32 stencilRefPending;
		GLDepthStencilStateValue depthStencilStateValue;

		RHIBlendState* blendStateCommitted;
		RHIBlendState* blendStatePending;
		GLBlendStateValue blendStateValue;

		bool bScissorRectEnabled;

		void Initialize()
		{
			rasterizerStateCommitted = nullptr;
			rasterizerStatePending = nullptr;
			rasterizerStateValue = GLRasterizerStateValue(ForceInit);
			stencilRefPending = 0xff;
			depthStencilStateCommitted = nullptr;
			depthStencilStatePending = nullptr;
			depthStencilStateValue = GLDepthStencilStateValue(ForceInit);
			blendStateCommitted = nullptr;
			blendStatePending = nullptr;
			blendStateValue = GLBlendStateValue(ForceInit);
			bScissorRectEnabled = false;
		}
	};

	namespace GLFactory
	{
		struct ProgramPipeline
		{
			static void Create(GLuint& handle) { glGenProgramPipelines(1, &handle); }
			static void Destroy(GLuint& handle) { glDeleteProgramPipelines(1, &handle); }
		};
	}

	class OpenGLShaderBoundState
	{
	public:
		bool create(GraphicsShaderStateDesc const& stateDesc);
		bool create(MeshShaderStateDesc const& stateDesc);

		void bind() const
		{
			glBindProgramPipeline(mGLObject.mHandle);
		}
		void unbind() const
		{
			glBindProgramPipeline(0);
		}

		GLuint getHandle()
		{
			return mGLObject.mHandle;
		}

		TOpenGLObject< GLFactory::ProgramPipeline > mGLObject;
	};

	class OpenGLContext : public RHIContext
	{
	public:
		void initialize();

		void shutdown();

		void RHISetRasterizerState(RHIRasterizerState& rasterizerState);
		void RHISetBlendState(RHIBlendState& blendState);
		void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef);

		void RHISetViewport(float x, float y, float w, float h, float zNear, float zFar);
		void RHISetViewports(ViewportInfo const viewports[], int numViewports);
		void RHISetScissorRect(int x, int y, int w, int h);


		void RHIDrawPrimitive(EPrimitive type, int start, int nv);
		void RHIDrawIndexedPrimitive(EPrimitive type, int indexStart, int nIndex, uint32 baseVertex);
		void RHIDrawPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride);
		void RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride);
		void RHIDrawPrimitiveInstanced(EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance);
		void RHIDrawIndexedPrimitiveInstanced(EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance);

		void RHIDrawPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numData, uint32 numInstance);
		void RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVerex, VertexDataInfo dataInfos[], int numVertexData, uint32 const* pIndices, int numIndex, uint32 numInstance);

		void RHIDrawMeshTasks(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ);
		void RHIDrawMeshTasksIndirect(RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride);

		void RHISetFixedShaderPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler);

		void RHISetFrameBuffer(RHIFrameBuffer* frameBuffer);

		void RHIClearRenderTargets(EClearBits clearBits, LinearColor colors[], int numColor, float depth, uint8 stenceil);

		void RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream);
		void RHISetIndexBuffer(RHIBuffer* buffer);
		void RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ);

		void RHIClearSRVResource(RHIResource* resource)
		{

		}
		void RHIResourceTransition(TArrayView<RHIResource*> resources, EResourceTransition transition)
		{

		}

		void RHIResolveTexture(RHITextureBase& destTexture, int destSubIndex, RHITextureBase& srcTexture, int srcSubIndex);

		void RHIFlushCommand();

		//Shader
		void RHISetShaderProgram(RHIShaderProgram* shaderProgram);

		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, int32 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix3 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix4 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector2 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector3 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector4 const val[], int dim);
		void setShaderMatrix22(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix43(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix34(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);

		void setShaderResourceView(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIShaderResourceView const& resourceView);

		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture);

		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler);
		void clearShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param)
		{

		}

		void setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler);

		void setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op);
		void setShaderRWSubTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, int subIndex, int level, EAccessOp op);
		void clearShaderUAV(RHIShaderProgram& shaderProgram, ShaderParameter const& param);

		void clearShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param)
		{


		}

		void setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer);
		void setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op);
		void setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer);

		void RHISetGraphicsShaderBoundState(GraphicsShaderStateDesc const& stateDesc);
		void RHISetMeshShaderBoundState(MeshShaderStateDesc const& stateDesc);
		void RHISetComputeShader(RHIShader* shader);

		void setShaderValue(RHIShader& shader, ShaderParameter const& param, int32 const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, float const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix3 const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix4 const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector2 const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector3 const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector4 const val[], int dim);
		void setShaderMatrix22(RHIShader& shader, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix43(RHIShader& shader, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix34(RHIShader& shader, ShaderParameter const& param, float const val[], int dim);

		void setShaderResourceView(RHIShader& shader, ShaderParameter const& param, RHIShaderResourceView const& resourceView);

		void setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture);
		void setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler);
		void clearShaderTexture(RHIShader& shader, ShaderParameter const& param)
		{

		}

		void setShaderSampler(RHIShader& shader, ShaderParameter const& param, RHISamplerState& sampler);
		void setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op);
		void setShaderRWSubTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, int subIndex, int level, EAccessOp op);
		void clearShaderRWTexture(RHIShader& shader, ShaderParameter const& param)
		{


		}


		void setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer);
		void setShaderStorageBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op);
		void setShaderAtomicCounterBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer);

		template< class TShaderObject >
		void setShaderUniformBufferT(TShaderObject& shaderObject, ShaderParameter const& param, RHIBuffer& buffer);
		template< class TShaderObject >
		void setShaderStorageBufferT(TShaderObject& shader, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op);
		template< class TShaderObject > 
		void setShaderRWTextureT(TShaderObject& shaderObject, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op);
		template< class TShaderObject > 
		void setShaderRWSubTextureT(TShaderObject& shaderObject, ShaderParameter const& param, RHITextureBase& texture, int subIndex, int level, EAccessOp op);

		static int const IdxTextureAutoBindStart = 2;

		void markRenderStateDirty();
	
		GLenum commitPrimitiveState(EPrimitive type)
		{
			int patchPointCount = 0;
			GLenum primitiveGL = OpenGLTranslate::To(type, patchPointCount);
			if (patchPointCount)
			{
				glPatchParameteri(GL_PATCH_VERTICES, patchPointCount);
			}
			return primitiveGL;
		}
		void commitGraphicStates();
		void commitSamplerStates();
		void commitRasterizerState();
		void commitBlendState();
		void commitDepthStencilState();

		bool commitInputStream();
		bool commitInputStreamUP(VertexDataInfo dataInfos[], int numData);
		void resetBindIndex()
		{
			mNextSamplerSlotIndex = 0;
			mSimplerSlotDirtyMask = 0;
			for (int i = 0; i < BufferResoureTypeCount; ++i)
			{
				mBufferBindings[i].nextSlot = 0;
			}
		}

		TOpenGLObject<GLFactory::FrameBuffer> mResolveFrameBuffers[2];

		enum EBufferBindingType
		{
			BBT_Uniform ,
			BBT_Storage ,

			BufferResoureTypeCount ,
		};

		struct BufferBindingSlot
		{
			uint32 key;
			GLuint handle;
		};

		struct BufferBindingInfo
		{
			int nextSlot;
			BufferBindingSlot slots[128];

			int fetchSlot(GLuint shaderHandle, int loc, GLuint handle)
			{
				CHECK((shaderHandle & 0xffff0000) == 0);
				CHECK((loc & 0xffff0000) == 0);

				uint32 key = (shaderHandle << 16) | loc;
				for (int indexSlot = 0; indexSlot != nextSlot; ++indexSlot)
				{
					BufferBindingSlot& slot = slots[indexSlot];
					if (slot.key == key)
					{
						if (slot.handle != handle)
						{
							slot.handle = handle;
							return indexSlot;
						}
						else
						{
							return INDEX_NONE;
						}
					}
				}

				int newSlotIndex = nextSlot;
				++nextSlot;
				BufferBindingSlot& newSlot = slots[newSlotIndex];
				newSlot.key = key;
				newSlot.handle = handle;
				return newSlotIndex;
			}
		};
		BufferBindingInfo mBufferBindings[BufferResoureTypeCount];

		RHIBufferRef   mLastIndexBuffer;
		OpenGLShaderBoundState* mLastShaderPipeline = nullptr;
		RHIShader*          mLastActiveShader = nullptr;

		void checkActiveShader(RHIShader& shader)
		{
			if (&shader != mLastActiveShader)
			{
				mLastShaderPipeline->getHandle();
				mLastActiveShader = &shader;
			}
		}
		RHIShaderRef        mLastComputeShader;
		RHIShaderProgramRef mLastShaderProgram;
		int                 mUsageShaderCount = 0;

		void clearShader(bool bUseShaderPipeline);
		bool checkInputStreamStateDirty(bool bForceDirty = false);

		RHIFrameBufferRef   mLastFrameBuffer;
		OpenGLDeviceState   mDeviceState;



		bool mbUseShaderPath = true;
		bool mWasBindAttrib = false;
		RHIInputLayoutRef   mInputLayoutPending;
		RHIInputLayoutRef   mInputLayoutCommitted;
		GLuint              mVAOCommitted = 0;


		InputStreamState mInputStreamStateCommttied;
		InputStreamInfo  mInputStreamsPending[MaxSimulationInputStreamSlots];
		int              mInputStreamCountPending = 0;
		bool             mbHasInputStreamPendingSetted = false;

		SimplePipelineParamValues mFixedShaderParams;

		void commitFixedShaderState();
		
		class OpenGLSystem* mSystem;

		struct SamplerState 
		{
			int32  loc;
			GLuint typeEnum;
			GLuint textureHandle;
			GLuint samplerHandle;
			GLuint shaderHandle;

			bool   bWrite;
		};
		static int constexpr MaxSimulationSamplerSlots = 16;
		int  mNextSamplerSlotIndex;

		int fetchSamplerStateSlot(GLuint shaderHandle, int loc);
		uint32 mSimplerSlotDirtyMask = 0;
		SamplerState mSamplerStates[MaxSimulationSamplerSlots];

		GLint mMaxDrawMeshTasksCount = 0;

		void setShaderResourceViewInternal(GLuint shaderHandle, ShaderParameter const& param, RHIShaderResourceView const& resourceView);
		void setShaderResourceViewInternal(GLuint shaderHandle, ShaderParameter const& param, RHIShaderResourceView const& resourceView, RHISamplerState const& sampler);
		void setShaderSamplerInternal(GLuint shaderHandle, ShaderParameter const& param, RHISamplerState& sampler);

	};


	class OpenGLSystem : public RHISystem
	{
	public:
		RHISystemName getName() const  { return RHISystemName::OpenGL; }
		bool initialize(RHISystemInitParams const& initParam);
		void shutdown();
		class ShaderFormat* createShaderFormat();
		bool RHIBeginRender();
		void RHIEndRender(bool bPresent);

		RHICommandList&  getImmediateCommandList()
		{
			return *mImmediateCommandList;
		}
		RHISwapChain*      RHICreateSwapChain(SwapChainCreationInfo const& info);
		RHITexture1D*      RHICreateTexture1D(TextureDesc const& desc, void* data);
		RHITexture2D*      RHICreateTexture2D(TextureDesc const& desc, void* data, int dataAlign);
		RHITexture3D*      RHICreateTexture3D(TextureDesc const& desc, void* data);
		RHITextureCube*    RHICreateTextureCube(TextureDesc const& desc, void* data[]);
		RHITexture2DArray* RHICreateTexture2DArray(TextureDesc const& desc, void* data);

		RHIBuffer*  RHICreateBuffer(BufferDesc const& desc, void* data);

		void* RHILockBuffer(RHIBuffer* buffer, ELockAccess access, uint32 offset, uint32 size);
		void  RHIUnlockBuffer(RHIBuffer* buffer);

		void RHIReadTexture(RHITexture2D& texture, ETexture::Format format, int level, TArray< uint8 >& outData);
		void RHIReadTexture(RHITextureCube& texture, ETexture::Format format, int level, TArray< uint8 >& outData);

		bool RHIUpdateTexture(RHITexture2D& texture, int ox, int oy, int w, int h, void* data, int level, int dataWidth);
		//void* RHILockTexture(RHITextureBase* texture, ELockAccess access, uint32 offset, uint32 size);
		//void  RHIUnlockTexture(RHITextureBase* texture);

		RHIFrameBuffer*   RHICreateFrameBuffer();
		RHIInputLayout*   RHICreateInputLayout(InputLayoutDesc const& desc);
		RHISamplerState*  RHICreateSamplerState(SamplerStateInitializer const& initializer);

		RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer);
		RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer);
		RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer);

		RHIShader* RHICreateShader(EShader::Type type);
		RHIShaderProgram* RHICreateShaderProgram();

		RHICommandListImpl* mImmediateCommandList = nullptr;
		class OpenglProfileCore* mProfileCore = nullptr;

		std::unordered_map< ShaderBoundStateKey, OpenGLShaderBoundState , MemberFuncHasher > mGfxBoundStateMap;

		OpenGLShaderBoundState* getShaderBoundState(GraphicsShaderStateDesc const& stateDesc);
		OpenGLShaderBoundState* getShaderBoundState(MeshShaderStateDesc const& stateDesc);

		template< class TShaderBoundState >
		OpenGLShaderBoundState* getShaderBoundStateT(TShaderBoundState const& state);

		OpenGLContext     mDrawContext;
#if SYS_PLATFORM_WIN
		WindowsGLContext  mGLContext;
#endif
	};


}

#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif


#endif // OpenGLCommand_H_B1DE1168_106C_49AB_9275_1AA61D14E11D
