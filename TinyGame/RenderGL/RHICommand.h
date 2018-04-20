#include "GLCommon.h"

namespace RenderGL
{
	RHITexture2D*    RHICreateTexture2D();
	RHITexture2D*    RHICreateTexture2D(Texture::Format format, int w, int h, 
										int numMipLevel = 0 , void* data = nullptr, int dataAlign = 0);
	RHITextureDepth* RHICreateTextureDepth();
	RHITextureCube*  RHICreateTextureCube();

}//namespace RenderGL
