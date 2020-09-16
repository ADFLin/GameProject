#include "SceneView.h"

#include "RHI/ShaderCore.h"
#include "RHI/RHICommand.h"
#include "RHI/ShaderProgram.h"

namespace Render
{
	//ref ViewParam.sgc
	struct GPU_ALIGN ViewBufferData
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(ViewBlock);

		Matrix4  worldToView;
		Matrix4  worldToClip;
		Matrix4  viewToWorld;
		Matrix4  viewToClip;
		Matrix4  clipToView;
		Matrix4  clipToWorld;
		Matrix4  worldToClipPrev;

		Vector4  frustumPlanes[6];

		Vector4  rectPosAndSizeInv;
		Vector3  worldPos;
		float  realTime;
		Vector3 direction;
		float  gameTime;
		int    frameCount;
		int    dummy[3];
	};

	void ViewInfo::updateRHIResource()
	{
		if (!mUniformBuffer.isValid())
		{
			mUniformBuffer = RHICreateVertexBuffer(sizeof(ViewBufferData), 1, BCF_UsageConst | BCF_CpuAccessWrite);
		}

		if (mbDataDirty)
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
			for (int i = 0; i < 6; ++i)
			{
				data.frustumPlanes[i] = frustumPlanes[i];
			}
			data.rectPosAndSizeInv.x = rectOffset.x;
			data.rectPosAndSizeInv.y = rectOffset.y;
			data.rectPosAndSizeInv.z = 1.0 / float(rectSize.x);
			data.rectPosAndSizeInv.w = 1.0 / float(rectSize.y);
			data.frameCount = frameCount;
			RHIUnlockBuffer(mUniformBuffer);
		}
	}

	void ViewInfo::releaseRHIResource()
	{
		mUniformBuffer.release();
	}

	void ViewInfo::setupTransform(Matrix4 const& inViewMatrix, Matrix4 const& inProjectMatrix)
	{
		worldToClipPrev = worldToClip;

		worldToView = inViewMatrix;
		float det;
		worldToView.inverse(viewToWorld, det);
		worldPos = TransformPosition(Vector3(0, 0, 0), viewToWorld);
		viewToClip = AdjProjectionMatrixForRHI(inProjectMatrix);
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

	void ViewInfo::setupShader(RHICommandList& commandList, ShaderProgram& program, StructuredBufferInfo const* pInfo )
	{
		updateRHIResource();
		if (pInfo)
		{
			program.setUniformBuffer(commandList, pInfo->blockName, *mUniformBuffer);
		}
		else
		{
			program.setStructuredUniformBufferT<ViewBufferData>(commandList, *mUniformBuffer);
		}
		
	}

	void ViewInfo::setupShader(RHICommandList& commandList, Shader& shader, StructuredBufferInfo const* pInfo)
	{
		updateRHIResource();
		if (pInfo)
		{
			shader.setUniformBuffer(commandList, pInfo->blockName, *mUniformBuffer);
		}
		else
		{
			shader.setStructuredUniformBufferT<ViewBufferData>(commandList, *mUniformBuffer);
		}
		
	}

	void ViewInfo::updateFrustumPlanes()
	{
#if 1
		Vector3 centerNearPos = (Vector4(0, 0, GRHIClipZMin, 1) * clipToWorld).dividedVector();
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
		frustumPlanes[5] = Plane::FromVector4(col3 + col0);  //left

#endif
	}


}