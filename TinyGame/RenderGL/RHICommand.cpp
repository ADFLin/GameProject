#include "RHICommand.h"


namespace RenderGL
{
	RHITexture2D* RHICreateTexture2D()
	{
		return new RHITexture2D;
	}

	RHITexture2D* RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, void* data , int dataAlign)
	{
		RHITexture2D* result = new RHITexture2D;
		if( !result->create(format, w, h , numMipLevel, data , dataAlign) )
		{
			delete result;
			return nullptr;
		}
		return result;
	}

	RHITextureDepth* RHICreateTextureDepth()
	{
		return new RHITextureDepth;
	}

	RHITextureCube* RHICreateTextureCube()
	{
		return new RHITextureCube;
	}

}
