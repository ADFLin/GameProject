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

#if USE_RHI_RESOURCE_TRACE
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

	class OpenGLShaderBoundStateKey
	{
	public:
		OpenGLShaderBoundStateKey(GraphicsShaderStateDesc const& stateDesc)
		{
			hash = 0x12362dc1;
			numShaders = 0;
			auto CheckShader = [&](void* shader, GLbitfield stageBit)
			{
				if (shader)
				{
					HashCombine(hash, shader);
					stageMask |= stageBit;
					shaders[numShaders] = shader;
					++numShaders;
				}
			};
			CheckShader(stateDesc.vertex, GL_VERTEX_SHADER_BIT);
			CheckShader(stateDesc.pixel, GL_FRAGMENT_SHADER_BIT);
			CheckShader(stateDesc.geometry, GL_GEOMETRY_SHADER_BIT);
			CheckShader(stateDesc.hull, GL_TESS_CONTROL_SHADER_BIT);
			CheckShader(stateDesc.domain, GL_TESS_EVALUATION_SHADER_BIT);
		}

		OpenGLShaderBoundStateKey(MeshShaderStateDesc const& stateDesc)
		{
			hash = 0x12dc3621;
			numShaders = 0;
			auto CheckShader = [&](void* shader, GLbitfield stageBit)
			{
				if (shader)
				{
					HashCombine(hash, shader);
					stageMask |= stageBit;
					shaders[numShaders] = shader;
					++numShaders;
				}
			};

			CheckShader(stateDesc.task, GL_TASK_SHADER_BIT_NV);
			CheckShader(stateDesc.mesh, GL_MESH_SHADER_BIT_NV);
			CheckShader(stateDesc.pixel, GL_FRAGMENT_SHADER_BIT);
		}
		uint32     hash;
		GLbitfield stageMask;
		void*      shaders[EShader::MaxStorageSize];
		int        numShaders;

		bool operator == (OpenGLShaderBoundStateKey const& rhs) const
		{
			if (numShaders != rhs.numShaders)
				return false;

			for (int i = 0; i < numShaders; ++i)
			{
				if (shaders[i] != rhs.shaders[i])
					return false;
			}

			return true;
		}

		uint32 getTypeHash() const { return hash; }
	};

	struct RMPProgramPipeline
	{
		static void Create(GLuint& handle) { glGenProgramPipelines(1, &handle); }
		static void Destroy(GLuint& handle) { glDeleteProgramPipelines(1, &handle); }
	};

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

		TOpenGLObject< RMPProgramPipeline > mGLObject;
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
		void RHIDrawPrimitiveIndirect(EPrimitive type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride);
		void RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride);
		void RHIDrawPrimitiveInstanced(EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance);
		void RHIDrawIndexedPrimitiveInstanced(EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance);

		void RHIDrawPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numData);
		void RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVerex, VertexDataInfo dataInfos[], int numVertexData, uint32 const* pIndices, int numIndex);

		void RHIDrawMeshTasks(int start, int count);
		void RHIDrawMeshTasksIndirect(RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride);

		void RHISetFixedShaderPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler);

		void RHISetFrameBuffer(RHIFrameBuffer* frameBuffer);

		void RHIClearRenderTargets(EClearBits clearBits, LinearColor colors[], int numColor, float depth, uint8 stenceil);

		void RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream);
		void RHISetIndexBuffer(RHIIndexBuffer* buffer);
		void RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ);

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

		void setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op);

		void clearShaderUAV(RHIShaderProgram& shaderProgram, ShaderParameter const& param);

		void clearShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param)
		{


		}

		void setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer, EAccessOperator op);
		void setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer);

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
		void setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op);
		void clearShaderRWTexture(RHIShader& shader, ShaderParameter const& param)
		{


		}


		void setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setShaderStorageBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer, EAccessOperator op);
		void setShaderAtomicCounterBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer);

		template< class TShaderObject >
		void setShaderUniformBufferT(TShaderObject& shaderObject, ShaderParameter const& param, RHIVertexBuffer& buffer);
		template< class TShaderObject >
		void setShaderStorageBufferT(TShaderObject& shader, ShaderParameter const& param, RHIVertexBuffer& buffer, EAccessOperator op);
		template< class TShaderObject > 
		void setShaderRWTextureT(TShaderObject& shaderObject, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op);

		static int const IdxTextureAutoBindStart = 2;


	
		GLenum commitPrimitiveState(EPrimitive type)
		{
			int patchPointCount = 1;
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

		RHIIndexBufferRef   mLastIndexBuffer;
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
		RHISwapChain*    RHICreateSwapChain(SwapChainCreationInfo const& info);
		RHITexture1D*    RHICreateTexture1D(
			ETexture::Format format, int length,
			int numMipLevel, uint32 createFlags, void* data);

		RHITexture2D*    RHICreateTexture2D(
			ETexture::Format format, int w, int h,
			int numMipLevel, int numSamples, uint32 createFlags, void* data, int dataAlign);

		RHITexture3D*      RHICreateTexture3D(ETexture::Format format, int sizeX, int sizeY, int sizeZ, int numMipLevel, int numSamples, uint32 createFlags, void* data);
		RHITextureCube*    RHICreateTextureCube(ETexture::Format format, int size, int numMipLevel, uint32 creationFlags, void* data[]);
		RHITexture2DArray* RHICreateTexture2DArray(ETexture::Format format, int w, int h, int layerSize, int numMipLevel, int numSamples, uint32 creationFlags, void* data);
		RHITexture2D*      RHICreateTextureDepth(ETexture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 creationFlags);

		RHIVertexBuffer*  RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data);
		RHIIndexBuffer*   RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data);

		void* RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size);
		void  RHIUnlockBuffer(RHIVertexBuffer* buffer);
		void* RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size);
		void  RHIUnlockBuffer(RHIIndexBuffer* buffer);

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

		std::unordered_map< OpenGLShaderBoundStateKey, OpenGLShaderBoundState , MemberFuncHasher > mGfxBoundStateMap;

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

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif


#endif // OpenGLCommand_H_B1DE1168_106C_49AB_9275_1AA61D14E11D
