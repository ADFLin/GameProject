#include "RHICommand.h"
#include "RHICommandListImpl.h"

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
		void RHISetScissorRect(bool bEnable, int x, int y, int w, int h);


		void RHIDrawPrimitive(PrimitiveType type, int start, int nv);
		void RHIDrawIndexedPrimitive(PrimitiveType type, ECompValueType indexType, int indexStart, int nIndex);
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
		void RHISetIndexBuffer(RHIIndexBuffer* buffer);

		RHIFrameBufferRef mLastFrameBuffer;
		OpenGLDeviceState mDeviceState;

	};

	class OpenGLSystem : public RHISystem
	{
	public:
		virtual bool initialize(RHISystemInitParam const& initParam) override;
		virtual void shutdown();

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

		RHICommandListImpl* mImmediateCommandList = nullptr;
		class OpenglProfileCore* mProfileCore = nullptr;

		OpenGLContext     mDrawContext;
#if SYS_PLATFORM_WIN
		WindowsGLContext  mGLContext;
#endif
	};
}


