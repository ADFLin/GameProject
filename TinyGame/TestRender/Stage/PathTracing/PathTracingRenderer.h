#pragma once

#ifndef PathTracingRenderer_H_BD925C77_895B_451F_BFD4_B9CD8D6F5EB7
#define PathTracingRenderer_H_BD925C77_895B_451F_BFD4_B9CD8D6F5EB7


#include "PathTracingCommon.h"
#include "PathTracingScene.h"
#include "PathTracingBVH.h"

#include "RHI/RHIRaytracingTypes.h"
#include "RHI/RHICommand.h"
#include "RHI/PrimitiveBatch.h"

#include "DataStructure/BVHTree.h"
#include "Renderer/Mesh.h"

//##TODO MoveTo Cpp
#include "Renderer/IBLResource.h"


namespace Render
{
	class ViewInfo;
	class RHICommandList;
	class FrameRenderTargets;

	class ScreenVS;
}

namespace PathTracing
{
	using namespace Render;

	struct RenderConfig
	{
		EDebugDsiplayMode debugDisplayMode = EDebugDsiplayMode::None; 

		bool bUseMIS = true;
		bool bUseBVH4 = true;
		bool bSplitAccumulate = false;
		bool bUseHardwareRayTracing = true;
		bool bUseDenoise = true;
		bool bUseBloom = false;
		bool bUseACES = false;
		float bloomThreshold = 1.0f;
		float tonemapExposure = 1.0f;

		int boundBoxWarningCount = 500;
		int triangleWarningCount = 50;

		bool bUseGlobalFog = false;
		float fogMaxDistance = 100.0f;
		Vector3 fogAlbedo = Vector3(1, 1, 1);
		Vector3 fogEmissive = Vector3(0, 0, 0);
		float fogInScattering = 1.0f;
		float fogPhaseG = 0.0f;
		float skyDistance = 100.0f;
	};


	struct SceneBVHNodeData
	{
		DECLARE_BUFFER_STRUCT(SceneBVHNodes);
	};

	struct SceneBVH4NodeData
	{
		DECLARE_BUFFER_STRUCT(SceneBVH4Nodes);
	};

	struct PrimitiveIdData
	{
		DECLARE_BUFFER_STRUCT(TriangleIds);
	};

	struct ObjectIdData
	{
		DECLARE_BUFFER_STRUCT(ObjectIds);
	};

	struct EmittingObjectIdData
	{
		DECLARE_BUFFER_STRUCT(EmittingObjectIds);
	};

	struct RenderResourceData
	{
		RHIBufferRef mPrimitiveAABBBuffer;

		TStructuredBuffer< MaterialData > mMaterialBuffer;
		TStructuredBuffer< ObjectData > mObjectBuffer;
		TStructuredBuffer< MeshVertexData > mVertexBuffer;
		TStructuredBuffer< MeshData > mMeshBuffer;
		TStructuredBuffer< BVHNodeData > mBVHNodeBuffer;
		TStructuredBuffer< BVH4NodeData > mBVH4NodeBuffer;

		TStructuredBuffer< BVHNodeData > mSceneBVHNodeBuffer;
		TStructuredBuffer< BVH4NodeData > mSceneBVH4NodeBuffer;
		TStructuredBuffer< int32 > mObjectIdBuffer;
		TStructuredBuffer< int32 > mObjectIdBufferV4;
		TStructuredBuffer< int32 > mEmittingObjectIdBuffer;


		RHITopLevelAccelerationStructureRef mTLAS;
		uint32 mNumTLASInstances = 0;
		uint32 mCurTLASInstanceCount = 0;
		RHIRayTracingPipelineStateRef mRayTracingPSO;
		RHIRayTracingShaderTableRef mSBT;
		TArray< RHIBottomLevelAccelerationStructureRef > mSceneMeshesBLAS;

		RHIBottomLevelAccelerationStructureRef mSphereBLAS;
		RHIBottomLevelAccelerationStructureRef mCubeBLAS;
		RHIBottomLevelAccelerationStructureRef mQuadBLAS;



		void clearScene()
		{
			mNumEmittingObjects = 0;
			mNumObjects = 0;
			mNumMaterials = 0;
			mNumMeshes = 0;

			mNumVertices = 0;
			mNumBVHNodes = 0;
			mNumBVH4Nodes = 0;

			mMeshVertices.clear();
			mMeshBVHNodes.clear();
			mMeshBVH4Nodes.clear();

			mSceneBVHNodes.clear();
			mSceneObjectIds.clear();
		}


		void releaseRHI()
		{
			mMaterialBuffer.releaseResource();
			mObjectBuffer.releaseResource();
			mVertexBuffer.releaseResource();
			mMeshBuffer.releaseResource();

			mBVHNodeBuffer.releaseResource();
			mBVH4NodeBuffer.releaseResource();
			mSceneBVHNodeBuffer.releaseResource();
			mSceneBVH4NodeBuffer.releaseResource();

			mObjectIdBuffer.releaseResource();
			mObjectIdBufferV4.releaseResource();

			mDebugPrimitives.releaseRHI();
		}

		int mNumEmittingObjects = 0;
		int mNumObjects = 0;
		int mNumMaterials = 0;
		int mNumMeshes = 0;

		int mNumVertices = 0;
		int mNumBVHNodes = 0;
		int mNumBVH4Nodes = 0;



		TArray< MeshVertexData > mMeshVertices;
		TArray< BVHNodeData > mMeshBVHNodes;
		TArray< BVH4NodeData > mMeshBVH4Nodes;

		TArray< BVHNodeData > mSceneBVHNodes;
		TArray< int32 > mSceneObjectIds;


		PrimitivesCollection mDebugPrimitives;
		BVHTree mDebugBVH;

	private:
		template< typename T, typename TArrayType >
		bool UpdateBuffer(TStructuredBuffer<T>& buffer, TArrayType const& data, int& currentCount, EStructuredBufferType type, BufferCreationFlags creationFlags, int offset = -1)
		{
			uint32 bufferCapacity = buffer.getElementNum();
			if (bufferCapacity < (uint32)data.size())
			{
				uint32 targetCapacity = (uint32)data.size();
				if (bufferCapacity > 0)
					targetCapacity = targetCapacity * 3 / 2;

				if (!buffer.initializeResource(targetCapacity, type, creationFlags))
					return false;

				RHIUpdateBuffer(*buffer.getRHI(), 0, (uint32)data.size(), (void*)data.data());
				currentCount = (int)data.size();
				return true;
			}

			int updateStart = (offset == -1) ? currentCount : offset;
			if ((uint32)data.size() > (uint32)updateStart)
			{
				RHIUpdateBuffer(*buffer.getRHI(), updateStart, (uint32)data.size() - updateStart, (void*)(data.data() + updateStart));
			}
			currentCount = (int)data.size();
			return false;
		}

	public:
		void buildSceneResource(RHICommandList& commandList, SceneData& sceneData);


		void buildSceneResourceHW(RHICommandList& commandList, SceneData& sceneData);



		bool updateMeshResource(TArrayView< MeshData const > meshes, int indexUpdateStart);
		bool updateSingleMeshResource(TArrayView< MeshData const > meshes, int meshId, SceneData& sceneData);
		bool buildMeshResource(TArrayView< MeshData const > meshes)
		{
			return updateMeshResource(meshes, 0);
		}

		static Math::TAABBox<Vector3> GetAABB(Quaternion const& rotation, Vector3 const halfSize)
		{
			Math::TAABBox<Vector3> result;
			result.invalidate();
			for (uint32 i = 0; i < 8; ++i)
			{
				Vector3 offset;
				offset.x = (i & 0x1) ? halfSize.x : -halfSize.x;
				offset.y = (i & 0x2) ? halfSize.y : -halfSize.y;
				offset.z = (i & 0x4) ? halfSize.z : -halfSize.z;
				result.addPoint(rotation.rotate(offset));
			}
			return result;
		}

		static Math::TAABBox<Vector3> GetAABB2(Vector3 const& center, Quaternion const& rotation, Vector3 const halfSize)
		{
			Math::TAABBox<Vector3> result;
			result.invalidate();
			for (uint32 i = 0; i < 8; ++i)
			{
				Vector3 offset;
				offset.x = (i & 0x1) ? halfSize.x : -halfSize.x;
				offset.y = (i & 0x2) ? halfSize.y : -halfSize.y;
				offset.z = (i & 0x4) ? halfSize.z : -halfSize.z;
				result.addPoint(rotation.rotate(offset + center));
			}
			return result;
		}

		void addDebugAABB(Math::TAABBox<Vector3> const& bound)
		{
			Vector3 posList[8];
			for (uint32 i = 0; i < 8; ++i)
			{
				posList[i].x = (i & 0x1) ? bound.max.x : bound.min.x;
				posList[i].y = (i & 0x2) ? bound.max.y : bound.min.y;
				posList[i].z = (i & 0x4) ? bound.max.z : bound.min.z;
			}

			constexpr int LineIndices[] =
			{
				0,1,1,3,3,2,2,0,
				4,5,5,7,7,6,6,4,
				0,4,1,5,3,7,2,6,
			};
			for (int i = 0; i < ARRAY_SIZE(LineIndices); i += 2)
			{
				mDebugPrimitives.addLine(posList[LineIndices[i]], posList[LineIndices[i + 1]], Vector3(1, 0, 0));
			}

		}


		void showNodeBound(int depth)
		{
			mDebugPrimitives.clear();
			for (int i = 0; i < mDebugBVH.nodes.size(); ++i)
			{
				auto const& node = mDebugBVH.nodes[i];
				if (node.depth == depth)
				{
					addDebugAABB(node.bound);
				}
			}
		}

	};

	class Renderer : public RenderResourceData
	{
	public:
		RHITexture2D* render(RHICommandList& commandList, RenderConfig const& config, ViewInfo& view, FrameRenderTargets& sceneRenderTargets);

		bool initializeRHI(ETexture::Format RTFormat);
		void releaseRHI();

		ScreenVS* mScreenVS;
		class AccumulatePS* mAccumulatePS;
		class DenoisePS* mDenoisePS;

		class PathTracingHardwareRayGen* mLastRayGenShader = nullptr;

		Mesh mQuadMesh;

		RHITexture2DRef mHDRImage;
		IBLResource mIBLResource;
		IBLResourceBuilder mIBLResourceBuilder;


		RHITexture2DRef mDenoiseTexture;
		RHIFrameBufferRef mDenoiseFrameBuffer;

		RHIFrameBufferRef mBloomFrameBuffer;

	};

}//namespace PathTracing


#endif // PathTracingRenderer_H_BD925C77_895B_451F_BFD4_B9CD8D6F5EB7