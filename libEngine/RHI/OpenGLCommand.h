#include "RHICommand.h"

#include "glew/GL/glew.h"
#include "OpenGLCommon.h"

#include "WGLContext.h"

namespace RenderGL
{

	struct GLDeviceState
	{
		GLDeviceState()
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


	class OpenGLSystem : public RHISystem
	{
	public:
		virtual bool initialize(RHISystemInitParam const& initParam) override;
		virtual void shutdown();

		bool RHIBeginRender();
		void RHIEndRender(bool bPresent);

		RHIRenderWindow* RHICreateRenderWindow(PlatformWindowInfo const& info);
		RHITexture1D*    RHICreateTexture1D(
			Texture::Format format, int length,
			int numMipLevel , uint32 createFlags, void* data);
		RHITexture2D*    RHICreateTexture2D(
			Texture::Format format, int w, int h,
			int numMipLevel, uint32 createFlags, void* data, int dataAlign);
		RHITexture3D*    RHICreateTexture3D(Texture::Format format, int sizeX, int sizeY, int sizeZ, uint32 createFlags, void* data);

		RHITextureDepth* RHICreateTextureDepth(Texture::DepthFormat format, int w, int h);
		RHITextureCube*  RHICreateTextureCube();

		RHIVertexBuffer*  RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data);
		RHIIndexBuffer*   RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data);
		RHIUniformBuffer* RHICreateUniformBuffer(uint32 elementSize, uint32 numElement, uint32 creationFlags, void* data);

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

		void RHISetRasterizerState(RHIRasterizerState& rasterizerState);
		void RHISetBlendState(RHIBlendState& blendState);
		void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef);

		void RHISetViewport(int x, int y, int w, int h);
		void RHISetScissorRect(bool bEnable, int x, int y, int w, int h);
		

		void RHIDrawPrimitive(PrimitiveType type, int start, int nv);
		void RHIDrawIndexedPrimitive(PrimitiveType type, ECompValueType indexType, int indexStart, int nIndex);
		void RHIDrawPrimitiveIndirect(PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride);
		void RHIDrawIndexedPrimitiveIndirect(PrimitiveType type, ECompValueType indexType, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride);

		void RHISetupFixedPipelineState(Matrix4 const& matModelView, Matrix4 const& matProj, int numTexture , RHITexture2D** textures);

		void RHISetFrameBuffer(RHIFrameBuffer& frameBuffer, RHITextureDepth* overrideDepthTexture /*= nullptr*/)
		{
			
		}

		GLDeviceState mDeviceState;
#if SYS_PLATFORM_WIN
		WindowsGLContext mGLContext;
#endif
	};
}


