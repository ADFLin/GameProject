#ifndef SimpleRenderState_H_E981306D_913C_481A_8828_FC08ABD00308
#define SimpleRenderState_H_E981306D_913C_481A_8828_FC08ABD00308

#include "RHIType.h"
#include "RHICommon.h"
#include "RHICommand.h"

#include <vector>

namespace Render
{

	class TransformStack
	{
	public:

		TransformStack()
		{
			mCurrent = Matrix4::Identity();
		}

		void clear()
		{
			mStack.clear();
			mCurrent = Matrix4::Identity();
		}

		FORCEINLINE void set(Matrix4 const& xform)
		{
			mCurrent = xform;
		}

		FORCEINLINE void setIdentity()
		{
			mCurrent = Matrix4::Identity();
		}

		FORCEINLINE void transform(Matrix4 const& xform)
		{
			mCurrent = xform * mCurrent;
		}

		FORCEINLINE void translate(Vector3 const& offset)
		{
			transform(Matrix4::Translate(offset));
		}

		FORCEINLINE void rotate(Quaternion const& q)
		{
			transform(Matrix4::Rotate(q));
		}

		FORCEINLINE void scale(Vector3 const& scale)
		{
			transform(Matrix4::Scale(scale));
		}

		void push()
		{
			mStack.push_back(mCurrent);
		}

		void pop()
		{
			assert(!mStack.empty());
			mCurrent = mStack.back();
			mStack.pop_back();
		}

		Matrix4 const& get() { return mCurrent; }
		Matrix4 mCurrent;
		std::vector< Matrix4 > mStack;
	};


	class SimpleRenderState
	{
	public:

		SimpleRenderState()
		{

		}

		void setColor(LinearColor const& color)
		{
			mColor = color;
		}

		void setupPipeline( RHICommandList& commandList , Matrix4 const& ViewProjectMatrix , RHITexture2D* textre )
		{
			RHISetFixedShaderPipelineState(commandList, ViewProjectMatrix * stack.get() , mColor, textre);
		}


		void commitPipelineState()
		{



		}
		TransformStack  stack;

		LinearColor     mColor;
		RHITexture2DRef mTexture;
	};

}//Render

#endif // SimpleRenderState_H_E981306D_913C_481A_8828_FC08ABD00308