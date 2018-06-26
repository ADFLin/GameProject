#include "OpenGLCommand.h"
#include "GLCommon.h"

namespace RenderGL
{
	template< class T, class ...Args >
	T* CreateRHIObjectT(Args ...args)
	{
		T* result = new T;
		if( result && !result->create(std::forward< Args >(args)...) )
		{
			delete result;
			return nullptr;
		}
		return result;
	}

	RHITexture1D* OpenGLSystem::RHICreateTexture1D(Texture::Format format, int length, int numMipLevel, void* data)
	{
		return CreateRHIObjectT< OpenGLTexture1D >(format, length, numMipLevel, data);
	}

	RHITexture2D* OpenGLSystem::RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, void* data, int dataAlign)
	{
		return CreateRHIObjectT< OpenGLTexture2D >(format, w, h, numMipLevel, data, dataAlign);
	}

	RHITexture3D* OpenGLSystem::RHICreateTexture3D(Texture::Format format, int sizeX, int sizeY, int sizeZ)
	{
		return CreateRHIObjectT< OpenGLTexture3D >(format, sizeX, sizeY, sizeZ);
	}

	RHITextureDepth* OpenGLSystem::RHICreateTextureDepth(Texture::DepthFormat format, int w, int h)
	{
		return CreateRHIObjectT< OpenGLTextureDepth >(format, w , h );
	}

	RHITextureCube* OpenGLSystem::RHICreateTextureCube()
	{
		return new OpenGLTextureCube;
	}

	RHIVertexBuffer* OpenGLSystem::RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		return CreateRHIObjectT< OpenGLVertexBuffer >(vertexSize, numVertices, creationFlags, data);
	}

	RHIIndexBuffer* OpenGLSystem::RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
	{
		return CreateRHIObjectT< OpenGLIndexBuffer >(nIndices, bIntIndex, creationFlags, data);
	}

	RHIUniformBuffer* OpenGLSystem::RHICreateUniformBuffer(uint32 size, uint32 creationFlags, void* data)
	{
		return CreateRHIObjectT< OpenGLUniformBuffer >(size, creationFlags, data);
	}

	RHIStorageBuffer* OpenGLSystem::RHICreateStorageBuffer(uint32 size, uint32 creationFlags, void* data)
	{
		return CreateRHIObjectT< OpenGLStorageBuffer >(size, creationFlags, data);
	}

}
