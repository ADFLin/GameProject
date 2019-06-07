#include "RHIType.h"
#include "RHICommon.h"
#include "RHICommand.h"

#include <vector>

namespace Render
{
	class SimpleRenderState
	{
	public:

		SimpleRenderState()
		{
			mTransformStack.push_back(Matrix4::Identity());
		}

		void setColor(LinearColor const& color)
		{
			mColor = color;
		}
		void setupPipeline( RHICommandList& commandList , RHITexture2D* textre )
		{
			RHISetupFixedPipelineState(commandList, mTransformStack.back(), mColor, &textre, textre ? 1 : 0);
		}

		void pushTranfsorm(Matrix4 const& matrix , bool bAppendPrevTransform = false )
		{
			if( bAppendPrevTransform )
				mTransformStack.push_back(matrix * mTransformStack.back());
			else
				mTransformStack.push_back(matrix);
		}
		void popTransform()
		{
			assert(!mTransformStack.empty());
			mTransformStack.pop_back();
		}

		LinearColor     mColor;
		std::vector< Matrix4 > mTransformStack;
		RHITexture2DRef mTexture;
	};

}//Render