#pragma once
#ifndef SceneView_H_28AA38EF_5EEF_43E0_8C90_765F3FF1344B
#define SceneView_H_28AA38EF_5EEF_43E0_8C90_765F3FF1344B

#include "RHI/RHICommon.h"
#include "Math/GeometryPrimitive.h"

namespace Render
{
	using Math::Plane;

	class ShaderProgram;
	class Shader;
	class RHICommandList;
	struct StructuredBufferInfo;

	struct ViewInfo
	{
		Matrix4 worldToClip;
		Matrix4 worldToView;
		Matrix4 viewToWorld;
		Matrix4 viewToClip;
		Matrix4 clipToWorld;
		Matrix4 clipToView;
		Matrix4 translatedWorldToClip;
		Matrix4 clipToTranslatedWorld;
		Matrix4 worldToClipPrev;

		Matrix4 viewToTranslatedWorld;

		Vector3 worldPos;
		Vector3 direction;
		int     frameCount;
		float   gameTime;
		float   realTime;

		IntVector2 rectSize;
		IntVector2 rectOffset;


		Plane frustumPlanes[6];

		RHIBufferRef mUniformBuffer;
		bool   mbDataDirty = true;

		bool frustumTest(Vector3 const& pos, float radius) const
		{
			for (int i = 0; i < 6; ++i)
			{
				if (frustumPlanes[i].calcDistance(pos) > radius)
					return false;
			}
			return true;
		}

		Vector3 getViewForwardDir() const
		{
			return TransformVector(Vector3(0, 0, 1), viewToWorld);
		}
		Vector3 getViewRightDir() const
		{
			return TransformVector(Vector3(1, 0, 0), viewToWorld);
		}
		Vector3 getViewUpDir() const
		{
			return TransformVector(Vector3(0, 1, 0), viewToWorld);
		}

		void updateRHIResource();
		void releaseRHIResource();
		void setupTransform(Vector3 const& viewPos, Quaternion const& viewRotation, Matrix4 const& inProjectMatrix);
		void setupTransform(Matrix4 const& inViewMatrix, Matrix4 const& inProjectMatrix);

		IntVector2 getViewportSize() const;

		
		void  setupShader(RHICommandList& commandList, ShaderProgram& program, StructuredBufferInfo const* info = nullptr );

		void  setupShader(RHICommandList& commandList, Shader& shader, StructuredBufferInfo const* info = nullptr);


		void updateFrustumPlanes();
	};

}
#endif // SceneView_H_28AA38EF_5EEF_43E0_8C90_765F3FF1344B
