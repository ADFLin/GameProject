#include "RenderContext.h"

#include "Material.h"
#include "RHICommand.h"

namespace RenderGL
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

			void* ptr = mUniformBuffer->lock(ELockAccess::WriteOnly);
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
			mUniformBuffer->unlock();
		}

		program.setBufferT<ViewBufferData>(*mUniformBuffer);

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
		int values[4];
		glGetIntegerv(GL_VIEWPORT, values);
		Vector4 viewportParam;
		viewportParam.x = values[0];
		viewportParam.y = values[1];
		viewportParam.z = 1.0 / values[2];
		viewportParam.w = 1.0 / values[3];
		program.setParam(SHADER_PARAM(View.viewportPosAndSizeInv), viewportParam);
#endif
	}

	void ViewInfo::updateFrustumPlanes()
	{
		//#NOTE: Dependent RHI
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
		frustumPlanes[4] = Plane(worldPos, posLT, posLB); //left
		frustumPlanes[5] = Plane(worldPos, posRB, posRT); //right
	}


}//namespace RenderGL

