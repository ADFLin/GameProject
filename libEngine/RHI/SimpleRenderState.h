#include "RHIType.h"
#include "RHICommon.h"

#include <vector>

namespace Render
{
	class SimpleRenderState
	{
	public:


		void setupPipeline()
		{

		}

		LinearColor     mColor;
		std::vector< Matrix4 > mTransformStack;
		RHITexture2DRef mTexture;
	};

}//Render