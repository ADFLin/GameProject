#include "RHICommand.h"
#include "RHICommandListImpl.h"

#include "ShaderCore.h"
#include "glew/GL/glew.h"
#include "OpenGLCommon.h"

#include "WGLContext.h"

namespace Render
{
	
	struct OpenGLDeviceState
	{
		OpenGLDeviceState()
		{
			Initialize();
		}
		RHIRasterizerState* rasterizerStateUsage;
		GLRasterizerStateValue rasterizerStateValue;
		RHIDepthStencilState* depthStencilStateUsage;
		GLDepthStencilStateValue depthStencilStateValue;
		RHIBlendState* blendStateUsage;
		GLBlendStateValue blendStateValue;

		bool bScissorRectEnabled;

		void Initialize()
		{
			rasterizerStateUsage = nullptr;
			rasterizerStateValue = GLRasterizerStateValue(ForceInit);
			depthStencilStateUsage = nullptr;
			depthStencilStateValue = GLDepthStencilStateValue(ForceInit);
			blendStateUsage = nullptr;
			blendStateValue = GLBlendStateValue(ForceInit);
			bScissorRectEnabled = false;
		}
	};

	class OpenGLContext : public RHIContext
	{
	public:
		void initialize()
		{


		}
		void RHISetRasterizerState(RHIRasterizerState& rasterizerState);
		void RHISetBlendState(RHIBlendState& blendState);
		void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef);

		void RHISetViewport(int x, int y, int w, int h);
		void RHISetScissorRect(int x, int y, int w, int h);


		void RHIDrawPrimitive(PrimitiveType type, int start, int nv);
		void RHIDrawIndexedPrimitive(PrimitiveType type, int indexStart, int nIndex, uint32 baseVertex);
		void RHIDrawPrimitiveIndirect(PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride);
		void RHIDrawIndexedPrimitiveIndirect(PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride);
		void RHIDrawPrimitiveInstanced(PrimitiveType type, int vStart, int nv, uint32 numInstance, uint32 baseInstance);
		void RHIDrawIndexedPrimitiveInstanced(PrimitiveType type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance);

		void RHIDrawPrimitiveUP(PrimitiveType type, int numVertex, VertexDataInfo dataInfos[], int numData);
		void RHIDrawIndexedPrimitiveUP(PrimitiveType type, int numVerex, VertexDataInfo dataInfos[], int numVertexData, int const* pIndices, int numIndex);

		void RHISetupFixedPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* textures[], int numTexture);

		void RHISetFrameBuffer(RHIFrameBuffer* frameBuffer, RHITextureDepth* overrideDepthTexture )
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

		void RHIClearRenderTarget( int numColor )
		{


		}
		void RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream);
		void RHISetIndexBuffer(RHIIndexBuffer* buffer);
		void RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ);


		//Shader
		void RHISetShaderProgram(RHIShaderProgram* shaderProgram);

		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, int32 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& parameter, float const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix3 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix4 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector3 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector4 const val[], int dim);
		void setShaderMatrix22(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix43(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix34(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);

		void setShaderResourceView(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIShaderResourceView const& resourceView);

		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture);
		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState & sampler);
		void setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler);
		void setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op);

		void setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer);

		static int const IdxTextureAutoBindStart = 2;

		bool commitInputStream()
		{
			if( mLastInputLayout.isValid() )
			{
				mWasBindAttrib = false;
				if( mbUseFixedPipeline )
				{
					OpenGLCast::To(mLastInputLayout)->bindPointer(mUsedInputStreams, mNumInputStream);
				}
				else
				{
					OpenGLCast::To(mLastInputLayout)->bindAttrib(mUsedInputStreams, mNumInputStream);
					mWasBindAttrib = true;
				}
			}
			return true;
		}

		bool commitInputStreamUP(VertexDataInfo dataInfos[], int numData)
		{
			if( !mLastInputLayout.isValid() )
				return false;
			
			for( int i = 0 ; i < numData; ++i )
			{
				mUsedInputStreams[i].offset = (uint32)dataInfos[i].ptr;
				mUsedInputStreams[i].stride = dataInfos[i].stride;
			}
			mNumInputStream = numData;
			mWasBindAttrib = false;
			if( mbUseFixedPipeline )
			{
				OpenGLCast::To(mLastInputLayout)->bindPointerUP(mUsedInputStreams, mNumInputStream);
			}
			else
			{
				OpenGLCast::To(mLastInputLayout)->bindAttribUP(mUsedInputStreams, mNumInputStream);
				mWasBindAttrib = true;
			}
			return true;
		}

		bool commitInputStreamUP()
		{
			if( !mLastInputLayout.isValid() )
				return false;
			return true;
		}
		void resetBindIndex()
		{
			mIdxTextureAutoBind = IdxTextureAutoBindStart;
			mNextUniformSlot = 0;
			mNextStorageSlot = 0;
		}

		int  mIdxTextureAutoBind;
		int  mNextUniformSlot;
		int  mNextStorageSlot;

		RHIIndexBufferRef   mLastIndexBuffer;
		RHIShaderProgramRef mLastShaderProgram;
		RHIFrameBufferRef   mLastFrameBuffer;
		OpenGLDeviceState   mDeviceState;

		bool mbUseFixedPipeline = true;
		bool mWasBindAttrib = false;
		RHIInputLayoutRef   mLastInputLayout;
	
		static int const MaxSimulationInputStreamSlots = 8;
		InputStreamInfo     mUsedInputStreams[MaxSimulationInputStreamSlots];
		int mNumInputStream;

	};

	class OpenGLSystem : public RHISystem
	{
	public:
		RHISytemName getName() const  { return RHISytemName::Opengl; }
		bool initialize(RHISystemInitParam const& initParam);
		void shutdown();
		class ShaderFormat* createShaderFormat();
		bool RHIBeginRender();
		void RHIEndRender(bool bPresent);
		RHICommandList&  getImmediateCommandList()
		{
			return *mImmediateCommandList;
		}
		RHIRenderWindow* RHICreateRenderWindow(PlatformWindowInfo const& info);
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

		RHIShader* RHICreateShader(Shader::Type type);
		RHIShaderProgram* RHICreateShaderProgram();

		RHICommandListImpl* mImmediateCommandList = nullptr;
		class OpenglProfileCore* mProfileCore = nullptr;

		OpenGLContext     mDrawContext;
#if SYS_PLATFORM_WIN
		WindowsGLContext  mGLContext;
#endif
	};
}


