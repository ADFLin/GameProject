#include "RHICommand.h"


namespace RenderGL
{
	RHITexture2D* RHICreateTexture2D()
	{
		return new RHITexture2D;
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
