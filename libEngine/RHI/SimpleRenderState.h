#ifndef SimpleRenderState_H_E981306D_913C_481A_8828_FC08ABD00308
#define SimpleRenderState_H_E981306D_913C_481A_8828_FC08ABD00308

#include "RHIType.h"
#include "RHICommon.h"
#include "RHICommand.h"

#include <vector>

#define USE_GL_MATRIX_STACK 0

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
#if USE_GL_MATRIX_STACK
			glLoadIdentity();
#else
			mStack.clear();
			mCurrent = Matrix4::Identity();
#endif
		}

		FORCEINLINE void set(Matrix4 const& xform)
		{
#if USE_GL_MATRIX_STACK
			glLoadMatrixf(xform);
#else
			mCurrent = xform;
#endif
		}

		FORCEINLINE void setIdentity()
		{
#if USE_GL_MATRIX_STACK
			glLoadIdentity();
#else
			mCurrent = Matrix4::Identity();
#endif
		}

		FORCEINLINE void transform(Matrix4 const& xform)
		{
#if USE_GL_MATRIX_STACK
			glMultMatrixf(xform);
#else
			mCurrent = xform * mCurrent;
#endif
		}

		FORCEINLINE void translate(Vector3 const& offset)
		{
#if USE_GL_MATRIX_STACK
			glTranslatef(offset.x, offset.y, offset.z);
#else
			transform(Matrix4::Translate(offset));
#endif
		}

		FORCEINLINE void rotate(Quaternion const& q)
		{
			transform(Matrix4::Rotate(q));
		}

		FORCEINLINE void scale(Vector3 const& scale)
		{
#if USE_GL_MATRIX_STACK
			glScalef(scale.x, scale.y, scale.z);
#else
			transform(Matrix4::Scale(scale));
#endif
		}

		void push()
		{
#if USE_GL_MATRIX_STACK
			glPushMatrix();
#else
			assert(!mStack.empty());
			mStack.push_back(mCurrent);
#endif
		}

		void pop()
		{
#if USE_GL_MATRIX_STACK
			glPopMatrix();
#else
			assert(!mStack.empty());
			mCurrent = mStack.back();
			mStack.pop_back();
#endif
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
			RHISetupFixedPipelineState(commandList, ViewProjectMatrix * stack.get() , mColor, &textre, textre ? 1 : 0);
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