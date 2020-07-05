#pragma once
#ifndef OpenGLCommand_H_B1DE1168_106C_49AB_9275_1AA61D14E11D
#define OpenGLCommand_H_B1DE1168_106C_49AB_9275_1AA61D14E11D

#include "RHICommand.h"
#include "RHICommandListImpl.h"

#include "ShaderCore.h"
#include "glew/GL/glew.h"
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
		RHIRasterizerState* rasterizerStateUsage;
		RHIRasterizerState* rasterizerStateCommit;
		GLRasterizerStateValue rasterizerStateValue;
		RHIDepthStencilState* depthStencilStateUsage;
		RHIDepthStencilState* depthStencilStateCommit;
		uint32 stencilRefCommit;
		GLDepthStencilStateValue depthStencilStateValue;
		RHIBlendState* blendStateUsage;
		RHIBlendState* blendStateCommit;
		GLBlendStateValue blendStateValue;

		bool bScissorRectEnabled;

		void Initialize()
		{
			rasterizerStateUsage = nullptr;
			rasterizerStateCommit = nullptr;
			rasterizerStateValue = GLRasterizerStateValue(ForceInit);
			depthStencilStateUsage = nullptr;
			depthStencilStateCommit = nullptr;
			depthStencilStateValue = GLDepthStencilStateValue(ForceInit);
			blendStateUsage = nullptr;
			blendStateCommit = nullptr;
			blendStateValue = GLBlendStateValue(ForceInit);
			bScissorRectEnabled = false;
		}
	};

	class OpenGLShaderPipelineStateKey
	{
	public:
		OpenGLShaderPipelineStateKey(GraphicShaderBoundState const& state)
		{
			hash = 0x123621;
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
			CheckShader(state.vertexShader, GL_VERTEX_SHADER_BIT);
			CheckShader(state.pixelShader, GL_FRAGMENT_SHADER_BIT);
			CheckShader(state.geometryShader, GL_GEOMETRY_SHADER_BIT);
			CheckShader(state.hullShader, GL_TESS_CONTROL_SHADER_BIT);
			CheckShader(state.domainShader, GL_TESS_EVALUATION_SHADER_BIT);
		}
		uint32     hash;
		GLbitfield stageMask;
		void*      shaders[EShader::MaxStorageSize];
		int        numShaders;

		bool operator == (OpenGLShaderPipelineStateKey const& rhs) const
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

	struct RMPShaderPipeline
	{
		static void Create(GLuint& handle) { glGenProgramPipelines(1, &handle); }
		static void Destroy(GLuint& handle) { glDeleteProgramPipelines(1, &handle); }
	};

	class OpenglShaderPipelineState
	{
	public:
		bool create(GraphicShaderBoundState const& state);
		void bind()
		{
			glBindProgramPipeline(mGLObject.mHandle);
		}
		void unbind()
		{
			glBindProgramPipeline(0);
		}

		GLuint getHandle()
		{
			return mGLObject.mHandle;
		}

		TOpenGLObject< RMPShaderPipeline > mGLObject;
	};

	class OpenGLContext : public RHIContext
	{
	public:
		void initialize()
		{


		}

		void shutdown();

		void RHISetRasterizerState(RHIRasterizerState& rasterizerState);
		void RHISetBlendState(RHIBlendState& blendState);
		void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef);

		void RHISetViewport(int x, int y, int w, int h, float zNear, float zFar);
		void RHISetScissorRect(int x, int y, int w, int h);


		void RHIDrawPrimitive(EPrimitive type, int start, int nv);
		void RHIDrawIndexedPrimitive(EPrimitive type, int indexStart, int nIndex, uint32 baseVertex);
		void RHIDrawPrimitiveIndirect(EPrimitive type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride);
		void RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride);
		void RHIDrawPrimitiveInstanced(EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance);
		void RHIDrawIndexedPrimitiveInstanced(EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance);

		void RHIDrawPrimitiveUP(EPrimitive type, int numVertex, VertexDataInfo dataInfos[], int numData);
		void RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVerex, VertexDataInfo dataInfos[], int numVertexData, int const* pIndices, int numIndex);

		void RHISetFixedShaderPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler);

		void RHISetFrameBuffer(RHIFrameBuffer* frameBuffer )
		{
			if( mLastFrameBuffer.isValid() )
			{
				OpenGLCast::To(mLastFrameBuffer)->unbind();
			}

			mLastFrameBuffer = frameBuffer;
			if( mLastFrameBuffer.isValid() )
			{
				OpenGLCast::To(mLastFrameBuffer)->bind();
			}
		}

		void RHIClearRenderTargets(EClearBits clearBits, LinearColor colors[], int numColor, float depth, uint8 stenceil)
		{
			GLbitfield clearBitsGL = 0;
			if (HaveBits(clearBits, EClearBits::Color))
			{
				if (numColor == 1)
				{
					glClearColor(colors[0].r, colors[0].b, colors[0].g, colors[0].a);
					clearBitsGL |= GL_COLOR_BUFFER_BIT;
				}
				else
				{
					for (int i = 0; i < numColor; ++i)
					{
						glClearBufferfv(GL_COLOR, i, colors[i]);
					}
				}

			}
			if (HaveBits(clearBits, EClearBits::Depth))
			{
				glClearDepth(depth);
				clearBitsGL |= GL_DEPTH_BUFFER_BIT;
			}
			if (HaveBits(clearBits, EClearBits::Stencil))
			{
				glClearStencil(stenceil);
				clearBitsGL |= GL_STENCIL_BUFFER_BIT;
			}
			if (clearBitsGL)
			{
				glClear(clearBitsGL);
			}

		}

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
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector3 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector4 const val[], int dim);
		void setShaderMatrix22(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix43(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix34(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);

		void setShaderResourceView(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIShaderResourceView const& resourceView);

		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture);

		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler);
		void setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler);
		void setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op);

		void setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer);

		void RHISetGraphicsShaderBoundState(GraphicShaderBoundState const& state);

		void setShaderValue(RHIShader& shader, ShaderParameter const& param, int32 const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, float const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix3 const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix4 const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector3 const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector4 const val[], int dim);
		void setShaderMatrix22(RHIShader& shader, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix43(RHIShader& shader, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix34(RHIShader& shader, ShaderParameter const& param, float const val[], int dim);

		void setShaderResourceView(RHIShader& shader, ShaderParameter const& param, RHIShaderResourceView const& resourceView);

		void setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture);
		void setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler);
		void setShaderSampler(RHIShader& shader, ShaderParameter const& param, RHISamplerState& sampler);
		void setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op);

		void setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setShaderStorageBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setShaderAtomicCounterBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer);

		static int const IdxTextureAutoBindStart = 2;

		void commitRenderStates();
		void commitSamplerStates();
		void commitRasterizerState();
		void commitBlendState();
		void commitDepthStencilState();

		bool commitInputStream();

		bool commitInputStreamUP(VertexDataInfo dataInfos[], int numData);

		bool commitInputStreamUP()
		{
			if( !mLastInputLayout.isValid() )
				return false;
			return true;
		}
		void resetBindIndex()
		{
			mNextAutoBindSamplerSlotIndex = 0;
			mSimplerSlotDirtyMask = 0;

			mNextUniformSlot = 0;
			mNextStorageSlot = 0;
		}

		
		int  mNextUniformSlot;
		int  mNextStorageSlot;

		RHIIndexBufferRef   mLastIndexBuffer;
		OpenglShaderPipelineState* mLastShaderPipeline = nullptr;
		RHIShader*          mLastActiveShader = nullptr;

		void checkActiveShader(RHIShader& shader)
		{
			if (&shader != mLastActiveShader)
			{
				mLastShaderPipeline->getHandle();
				mLastActiveShader = &shader;
			}
		}

		RHIShaderProgramRef mLastShaderProgram;
		int                 mUsageShaderCount = 0;

		void clearShader(bool bUseShaderPipeline);

		RHIFrameBufferRef   mLastFrameBuffer;
		OpenGLDeviceState   mDeviceState;

		bool mbUseFixedPipeline = true;
		bool mWasBindAttrib = false;
		RHIInputLayoutRef   mLastInputLayout;
	
		static int const MaxSimulationInputStreamSlots = 8;
		InputStreamInfo     mUsedInputStreams[MaxSimulationInputStreamSlots];
		int mNumInputStream;
		uint32 mSimplerSlotDirtyMask = 0;
		class OpenGLSystem* mSystem;

		struct SamplerState 
		{
			uint32 loc;
			GLuint typeEnum;
			GLuint textureHandle;
			GLuint samplerHandle;
			GLuint program;

			bool   bWrite;
		};
		static int constexpr MaxSimulationSamplerSlots = 8;
		int  mNextAutoBindSamplerSlotIndex;
		SamplerState mSamplerStates[MaxSimulationSamplerSlots];

		void setShaderResourceViewInternal(GLuint handle, ShaderParameter const& param, RHIShaderResourceView const& resourceView);
		void setShaderResourceViewInternal(GLuint handle, ShaderParameter const& param, RHIShaderResourceView const& resourceView, RHISamplerState const& sampler);
		void setShaderSamplerInternal(GLuint handle, ShaderParameter const& param, RHISamplerState& sampler);

	};

	class OpenGLSystem : public RHISystem
	{
	public:
		RHISytemName getName() const  { return RHISytemName::OpenGL; }
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
			Texture::Format format, int length,
			int numMipLevel , uint32 createFlags, void* data);

		RHITexture2D*    RHICreateTexture2D(
			Texture::Format format, int w, int h,
			int numMipLevel, int numSamples, uint32 createFlags, void* data, int dataAlign);

		RHITexture3D*      RHICreateTexture3D(Texture::Format format, int sizeX, int sizeY, int sizeZ, int numMipLevel, int numSamples, uint32 createFlags, void* data);
		RHITextureCube*    RHICreateTextureCube(Texture::Format format, int size, int numMipLevel, uint32 creationFlags, void* data[]);
		RHITexture2DArray* RHICreateTexture2DArray(Texture::Format format, int w, int h, int layerSize, int numMipLevel, int numSamples, uint32 creationFlags, void* data);
		RHITextureDepth*   RHICreateTextureDepth(Texture::DepthFormat format, int w, int h, int numMipLevel, int numSamples);

		RHIVertexBuffer*  RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data);
		RHIIndexBuffer*   RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data);

		void* RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size);
		void  RHIUnlockBuffer(RHIVertexBuffer* buffer);
		void* RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size);
		void  RHIUnlockBuffer(RHIIndexBuffer* buffer);

		RHIFrameBuffer*   RHICreateFrameBuffer();
		RHIInputLayout*   RHICreateInputLayout(InputLayoutDesc const& desc);
		RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer);

		RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer);
		RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer);
		RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer);

		RHIShader* RHICreateShader(EShader::Type type);
		RHIShaderProgram* RHICreateShaderProgram();

		RHICommandListImpl* mImmediateCommandList = nullptr;
		class OpenglProfileCore* mProfileCore = nullptr;

		std::unordered_map< OpenGLShaderPipelineStateKey, OpenglShaderPipelineState , MemberFuncHasher > mGfxBoundStateMap;

		OpenglShaderPipelineState* getShaderPipeline(GraphicShaderBoundState const& state);


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
