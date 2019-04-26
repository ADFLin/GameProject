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
		void RHIDrawIndexedPrimitive(PrimitiveType type, ECompValueType indexType, int indexStart, int nIndex, uint32 baseVertex);
		void RHIDrawPrimitiveIndirect(PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride);
		void RHIDrawIndexedPrimitiveIndirect(PrimitiveType type, ECompValueType indexType, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride);
		void RHIDrawPrimitiveInstanced(PrimitiveType type, int vStart, int nv, int numInstance);
		void RHIDrawPrimitiveUP(PrimitiveType type, int numPrimitive, void* pVertices, int numVerex, int vetexStride)
		{


		}

		void RHIDrawIndexedPrimitiveUP(PrimitiveType type, int numPrimitive, void* pVertices, int numVerex, int vetexStride, int* pIndices, int numIndex)
		{

		}

		void RHISetupFixedPipelineState(Matrix4 const& matModelView, Matrix4 const& matProj, int numTexture, RHITexture2D const** textures);

		void RHISetFrameBuffer(RHIFrameBuffer& frameBuffer, RHITextureDepth* overrideDepthTexture /*= nullptr*/)
		{
			if( mLastFrameBuffer.isValid() )
			{
				OpenGLCast::To(mLastFrameBuffer)->unbind();
			}

			mLastFrameBuffer = &frameBuffer;

			if( mLastFrameBuffer.isValid() )
			{
				OpenGLCast::To(mLastFrameBuffer)->bind();
			}
		}

		void RHIClearRenderTarget( int numColor )
		{


		}

		void RHISetIndexBuffer(RHIIndexBuffer* buffer);
		void RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ);


		//Shader
		void RHISetShaderProgram(RHIShaderProgram* shaderProgram);

		void setParam(RHIShaderProgram& shaderProgram, ShaderParameter const& param, int const val[], int dim);
		void setParam(RHIShaderProgram& shaderProgram, ShaderParameter const& parameter, float const val[], int dim);
		void setParam(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix3 const val[], int dim);
		void setParam(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix4 const val[], int dim);
		void setParam(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector3 const val[], int dim);
		void setParam(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector4 const val[], int dim);
		void setMatrix22(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setMatrix43(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setMatrix34(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);

		void setResourceView(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIShaderResourceView const& resourceView);

		void setTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture);
		void setTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState & sampler);
		void setSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState const& sampler);
		void setRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op);

		void setUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer);

		static int const IdxTextureAutoBindStart = 2;
		void resetBindIndex()
		{
			mIdxTextureAutoBind = IdxTextureAutoBindStart;
			mNextUniformSlot = 0;
			mNextStorageSlot = 0;
		}

		int  mIdxTextureAutoBind;
		int  mNextUniformSlot;
		int  mNextStorageSlot;

		RHIShaderProgramRef mLastShaderProgram;
		RHIFrameBufferRef mLastFrameBuffer;
		OpenGLDeviceState mDeviceState;

	};

	class OpenGLSystem : public RHISystem
	{
	public:
		virtual bool initialize(RHISystemInitParam const& initParam) override;
		virtual void shutdown();
		virtual class ShaderFormat* createShaderFormat();
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


