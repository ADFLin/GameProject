#include "RenderContext.h"

#include "Material.h"
#include "RHICommand.h"

namespace Render
{
	void RenderTechnique::setupWorld(RenderContext& context, Matrix4 const& mat)
	{
		if( context.mUsageProgram )
		{
			Matrix4 matInv;
			float det;
			mat.inverseAffine(matInv, det);
			context.mUsageProgram->setParam(SHADER_PARAM(Primitive.localToWorld), mat);
			context.mUsageProgram->setParam(SHADER_PARAM(Primitive.worldToLocal), matInv);
		}
		else
		{
			//Fixed pipeline
			glLoadMatrixf( mat * context.getView().worldToView );
		}
	}

	void RenderContext::setShader(ShaderProgram& program)
	{
		if( mUsageProgram != &program )
		{
			if( mUsageProgram )
			{
				mUsageProgram->unbind();
			}
			mUsageProgram = &program;
			mUsageProgram->bind();
		}
	}

	MaterialShaderProgram* RenderContext::setMaterial( Material* material , VertexFactory* vertexFactory )
	{
		MaterialShaderProgram* program;
		if( material )
		{
			program = mTechique->getMaterialShader(*this,*material->getMaster() , vertexFactory );
			if( program == nullptr || !program->isValid() )
			{
				material = nullptr;
			}
		}

		if( material == nullptr )
		{
			material = GDefalutMaterial;
			program = mTechique->getMaterialShader(*this, *GDefalutMaterial , vertexFactory );
		}

		if( program == nullptr )
		{
			return nullptr;
		}

		if( mUsageProgram != program )
		{
			if( mUsageProgram )
			{
				mUsageProgram->unbind();
			}
			mUsageProgram = program;
			mbUseMaterialShader = true;
			program->bind();
			mCurView->setupShader(*program);
			mTechique->setupMaterialShader(*this, *program);
		}
		material->setupShader(*program);

		return program;
	}


	void ViewInfo::setupTransform(Matrix4 const& inViewMatrix, Matrix4 const& inProjectMatrix)
	{
		worldToView = inViewMatrix;
		float det;
		worldToView.inverse(viewToWorld, det);
		worldPos = TransformPosition(Vector3(0, 0, 0), viewToWorld);
		viewToClip = inProjectMatrix;
		worldToClip = worldToView * viewToClip;

		viewToClip.inverse(clipToView, det);
		clipToWorld = clipToView * viewToWorld;

		direction = TransformVector(Vector3(0, 0, -1), viewToWorld);

		updateFrustumPlanes();

		mbDataDirty = true;
	}

	IntVector2 ViewInfo::getViewportSize() const
	{
		int values[4];
		glGetIntegerv(GL_VIEWPORT, values);
		return IntVector2(values[2], values[3]);
	}

	void ViewInfo::setupShader(ShaderProgram& program)
	{
		//ref ViewParam.sgc
#if 1
		struct ViewBufferData
		{
			DECLARE_BUFFER_STRUCT(ViewBlock);

			Matrix4  worldToView;
			Matrix4  worldToClip;
			Matrix4  viewToWorld;
			Matrix4  viewToClip;
			Matrix4  clipToView;
			Matrix4  clipToWorld;
			Vector4  rectPosAndSizeInv;
			Vector3 worldPos;
			float  realTime;
			Vector3 direction;
			float  gameTime;

		};

		if( !mUniformBuffer.isValid() )
		{
			mUniformBuffer = RHICreateUniformBuffer(sizeof(ViewBufferData) , 1 , BCF_UsageDynamic );
		}

		if( mbDataDirty )
		{
			mbDataDirty = false;

			void* ptr = RHILockBuffer(mUniformBuffer , ELockAccess::WriteDiscard );
			ViewBufferData& data = *(ViewBufferData*)ptr;
			data.worldPos = worldPos;
			data.direction = direction;
			data.worldToView = worldToView;
			data.worldToClip = worldToClip;
			data.viewToWorld = viewToWorld;
			data.viewToClip = viewToClip;
			data.clipToView = clipToView;
			data.clipToWorld = clipToWorld;
			data.gameTime = gameTime;
			data.realTime = realTime;
			data.rectPosAndSizeInv.x = rectOffset.x;
			data.rectPosAndSizeInv.y = rectOffset.y;
			data.rectPosAndSizeInv.z = 1.0 / float(rectSize.x);
			data.rectPosAndSizeInv.w = 1.0 / float(rectSize.y);	
			RHIUnlockBuffer(mUniformBuffer);
		}

		program.setStructuredBufferT<ViewBufferData>(*mUniformBuffer);

#else
		program.setParam(SHADER_PARAM(View.worldPos), worldPos);
		program.setParam(SHADER_PARAM(View.direction), direction);
		program.setParam(SHADER_PARAM(View.worldToView), worldToView);
		program.setParam(SHADER_PARAM(View.worldToClip), worldToClip);
		program.setParam(SHADER_PARAM(View.viewToWorld), viewToWorld);
		program.setParam(SHADER_PARAM(View.viewToClip), viewToClip);
		program.setParam(SHADER_PARAM(View.clipToView), clipToView);
		program.setParam(SHADER_PARAM(View.clipToWorld), clipToWorld);
		program.setParam(SHADER_PARAM(View.gameTime), gameTime);
		program.setParam(SHADER_PARAM(View.realTime), realTime);
		viewportParam.x = rectOffset.x;
		viewportParam.y = rectOffset.y;
		viewportParam.z = 1.0 / float(rectSize.x);
		viewportParam.w = 1.0 / float(rectSize.y);
		program.setParam(SHADER_PARAM(View.viewportPosAndSizeInv), viewportParam);
#endif
	}

	void ViewInfo::updateFrustumPlanes()
	{
		//#NOTE: Dependent RHI
#if 0
		Vector3 centerNearPos = (Vector4(0, 0, -1, 1) * clipToWorld).dividedVector();
		Vector3 centerFarPos = (Vector4(0, 0, 1, 1) * clipToWorld).dividedVector();

		Vector3 posRT = (Vector4(1, 1, 1, 1) * clipToWorld).dividedVector();
		Vector3 posLB = (Vector4(-1, -1, 1, 1) * clipToWorld).dividedVector();
		Vector3 posRB = (Vector4(1, -1, 1, 1) * clipToWorld).dividedVector();
		Vector3 posLT = (Vector4(-1, 1, 1, 1) * clipToWorld).dividedVector();

		frustumPlanes[0] = Plane(-direction, centerNearPos); //ZFar;
		frustumPlanes[1] = Plane(direction, centerFarPos); //ZNear
		frustumPlanes[2] = Plane(worldPos, posRT, posLT); //top
		frustumPlanes[3] = Plane(worldPos, posLB, posRB); //bottom
		frustumPlanes[4] = Plane(worldPos, posRB, posRT); //right
		frustumPlanes[5] = Plane(worldPos, posLT, posLB); //left

#else

		//    -1 < x / w < 1
		//->   w < x < w
		//-> -dot(col3,v) < dot(col0,v) < dot(col3,v)
		//-> dot( col3 + col0 , v ) > 0  , dot( col3 - col0 , v ) > 0 
		Vector4 col0 = clipToWorld.col(0);
		Vector4 col1 = clipToWorld.col(1);
		Vector4 col2 = clipToWorld.col(2);
		Vector4 col3 = clipToWorld.col(3);

		frustumPlanes[0] = Plane::FrameVector4(col3 - col2);  //zFar
		frustumPlanes[1] = Plane::FrameVector4(col3 + col2);  //zNear
		frustumPlanes[2] = Plane::FrameVector4(col3 - col0);  //top
		frustumPlanes[3] = Plane::FrameVector4(col3 + col0);  //bottom
		frustumPlanes[4] = Plane::FrameVector4(col3 - col0);  //right
		frustumPlanes[4] = Plane::FrameVector4(col3 + col0);  //left

#endif
	}


}//namespace Render

