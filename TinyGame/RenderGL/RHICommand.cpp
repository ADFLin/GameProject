#include "RHICommand.h"


namespace RenderGL
{
	RHITexture2DRef RHICreateTexture2D()
	{
		return new RHITexture2D;
	}

	RHITextureDepthRef RHICreateTextureDepth()
	{
		return new RHITextureDepth;
	}

	RHITextureCubeRef RHICreateTextureCube()
	{
		return new RHITextureCube;
	}

}
