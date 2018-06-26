#include "RHICommon.h"


namespace RenderGL
{
	class OpenGLSystem : public RHISystem
	{
	public:
		virtual RHITexture1D*    RHICreateTexture1D(
			Texture::Format format, int length,
			int numMipLevel , void* data) override;
		virtual RHITexture2D*    RHICreateTexture2D(
			Texture::Format format, int w, int h,
			int numMipLevel, void* data, int dataAlign) override;
		virtual RHITexture3D*    RHICreateTexture3D(Texture::Format format, int sizeX, int sizeY, int sizeZ) override;

		virtual RHITextureDepth* RHICreateTextureDepth(Texture::DepthFormat format, int w, int h) override;
		virtual RHITextureCube*  RHICreateTextureCube() override;

		virtual RHIVertexBuffer*  RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data) override;
		virtual RHIIndexBuffer*   RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data) override;
		virtual RHIUniformBuffer* RHICreateUniformBuffer(uint32 size, uint32 creationFlags, void* data) override;
		virtual RHIStorageBuffer* RHICreateStorageBuffer(uint32 size, uint32 creationFlags, void* data) override;
	};
}


