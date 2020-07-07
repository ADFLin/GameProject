#include "RenderContext.h"

#include "Material.h"
#include "RHICommand.h"

//#TODO: remove me
#include "OpenGLCommon.h"

namespace Render
{
	void RenderTechnique::setupWorld(RenderContext& context, Matrix4 const& mat)
	{
		if( context.mUsageProgram )
		{
			Matrix4 matInv;
			float det;
			mat.inverseAffine(matInv, det);
			context.mUsageProgram->setParam(context.getCommnadList(), SHADER_PARAM(Primitive.localToWorld), mat);
			context.mUsageProgram->setParam(context.getCommnadList(), SHADER_PARAM(Primitive.worldToLocal), matInv);
		}
		else
		{
			//Fixed pipeline
			glLoadMatrixf( mat * context.getView().worldToView );
		}
	}

	void RenderContext::endRender()
	{
		if( mUsageProgram )
		{
			RHISetShaderProgram(getCommnadList(), nullptr);
			mUsageProgram = nullptr;
		}
	}

	void RenderContext::setShader(ShaderProgram& program)
	{
		if( mUsageProgram != &program )
		{
			mUsageProgram = &program;
			RHISetShaderProgram( getCommnadList() , program.getRHIResource());
		}
	}

	MaterialShaderProgram* RenderContext::setMaterial( Material* material , VertexFactory* vertexFactory )
	{
		MaterialShaderProgram* program;
		RHICommandList& commandList = getCommnadList();

		if( material )
		{
			program = mTechique->getMaterialShader(*this,*material->getMaster() , vertexFactory );
			if( program == nullptr || !program->getRHIResource() )
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
			mUsageProgram = program;
			mbUseMaterialShader = true;
			RHISetShaderProgram(commandList, program->getRHIResource());
			mCurView->setupShader(commandList, *program);
			mTechique->setupMaterialShader(*this, *program);
		}
		material->setupShader(commandList, *program);

		return program;
	}

	//ref ViewParam.sgc
	struct GPU_BUFFER_ALIGN ViewBufferData
	{
		DECLARE_BUFFER_STRUCT(ViewBlock);

		Matrix4  worldToView;
		Matrix4  worldToClip;
		Matrix4  viewToWorld;
		Matrix4  viewToClip;
		Matrix4  clipToView;
		Matrix4  clipToWorld;
		Matrix4  worldToClipPrev;
		Vector4  rectPosAndSizeInv;
		Vector3 worldPos;
		float  realTime;
		Vector3 direction;
		float  gameTime;
		int    frameCount;
		int    dummy[3];
	};

	void ViewInfo::updateBuffer()
	{
		if( !mUniformBuffer.isValid() )
		{
			mUniformBuffer = RHICreateVertexBuffer(sizeof(ViewBufferData), 1, BCF_UsageConst | BCF_UsageDynamic);
		}

		if( mbDataDirty )
		{
			mbDataDirty = false;

			void* ptr = RHILockBuffer(mUniformBuffer, ELockAccess::WriteDiscard);
			ViewBufferData& data = *(ViewBufferData*)ptr;
			data.worldPos = worldPos;
			data.direction = direction;
			data.worldToView = worldToView;
			data.worldToClip = worldToClip;
			data.viewToWorld = viewToWorld;
			data.viewToClip = viewToClip;
			data.clipToView = clipToView;
			data.clipToWorld = clipToWorld;
			data.worldToClipPrev = worldToClipPrev;
			data.gameTime = gameTime;
			data.realTime = realTime;
			data.rectPosAndSizeInv.x = rectOffset.x;
			data.rectPosAndSizeInv.y = rectOffset.y;
			data.rectPosAndSizeInv.z = 1.0 / float(rectSize.x);
			data.rectPosAndSizeInv.w = 1.0 / float(rectSize.y);
			data.frameCount = frameCount;
			RHIUnlockBuffer(mUniformBuffer);
		}
	}

	void ViewInfo::setupTransform(Matrix4 const& inViewMatrix, Matrix4 const& inProjectMatrix)
	{
		worldToClipPrev = worldToClip;

		worldToView = inViewMatrix;
		float det;
		worldToView.inverse(viewToWorld, det);
		worldPos = TransformPosition(Vector3(0, 0, 0), viewToWorld);
		viewToClip = AdjProjectionMatrixForRHI( inProjectMatrix );
		worldToClip = worldToView * viewToClip;

		viewToClip.inverse(clipToView, det);
		clipToWorld = clipToView * viewToWorld;

		direction = getViewForwardDir();

		updateFrustumPlanes();

		mbDataDirty = true;
	}

	IntVector2 ViewInfo::getViewportSize() const
	{
		return rectSize;
	}

	void ViewInfo::setupShader(RHICommandList& commandList, ShaderProgram& program)
	{
		updateBuffer();
		program.setStructuredUniformBufferT<ViewBufferData>(commandList, *mUniformBuffer);
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

		frustumPlanes[0] = Plane::FromVector4(col3 - col2);  //zFar
		frustumPlanes[1] = Plane::FromVector4(col3 - GRHIClipZMin * col2);  //zNear
		frustumPlanes[2] = Plane::FromVector4(col3 - col0);  //top
		frustumPlanes[3] = Plane::FromVector4(col3 + col0);  //bottom
		frustumPlanes[4] = Plane::FromVector4(col3 - col0);  //right
		frustumPlanes[4] = Plane::FromVector4(col3 + col0);  //left

#endif
	}


}//namespace Render

